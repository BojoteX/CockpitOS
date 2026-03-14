"""
CockpitOS Release Helper
Run this after merging a PR to main to trigger a release.
Rebuilds CHANGELOG.md from the full tag history, then triggers the GitHub
Actions release workflow which creates the tag, zip, and GitHub release.
"""

import subprocess
import sys
import re
import os
import json
import datetime

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CHANGELOG_PATH = os.path.join(SCRIPT_DIR, "CHANGELOG.md")

# Housekeeping patterns to filter out of changelogs
SKIP_PATTERNS = ('merge pull request', 'merge branch', 'update changelog',
                 'release version', 'update release')


def run(cmd, capture=True):
    result = subprocess.run(cmd, shell=True, capture_output=capture, text=True, cwd=SCRIPT_DIR)
    if capture:
        return result.stdout.strip(), result.returncode
    return "", result.returncode


def get_repo():
    """Extract owner/repo from git remote origin."""
    out, _ = run("git remote get-url origin")
    m = re.search(r'github\.com[:/](.+?)(?:\.git)?$', out)
    return m.group(1) if m else "BojoteX/CockpitOS"


def get_tags():
    """Parse all semver tags, return sorted list of (major, minor, patch, tag_str)."""
    raw, _ = run("git tag -l")
    if not raw:
        return []
    versions = []
    for t in raw.splitlines():
        clean = t.lstrip("v")
        parts = clean.split(".")
        if len(parts) == 3 and all(p.isdigit() for p in parts):
            versions.append((int(parts[0]), int(parts[1]), int(parts[2]), t))
    versions.sort()
    return versions


def get_tag_date(tag):
    """ISO date of the tag's commit (for PR filtering)."""
    out, rc = run(f'git log -1 --format=%aI {tag}')
    return out if rc == 0 else ""


def get_tag_short_date(tag):
    """YYYY-MM-DD date of the tag's commit."""
    out, rc = run(f'git log -1 --format=%ad --date=short {tag}')
    return out if rc == 0 else ""


def get_merged_prs(repo, since_date):
    """Merged PRs to main since a date, via gh CLI."""
    out, rc = run(
        f'gh pr list --repo {repo} --state merged --base main '
        f'--json number,title,mergedAt --limit 200'
    )
    if rc != 0 or not out:
        return []
    try:
        prs = json.loads(out)
        if since_date:
            prs = [pr for pr in prs if pr.get('mergedAt', '') > since_date]
        prs = [pr for pr in prs
               if not any(pr['title'].lower().startswith(s) for s in SKIP_PATTERNS)]
        prs.sort(key=lambda p: p.get('mergedAt', ''))
        return prs
    except (json.JSONDecodeError, KeyError):
        return []


def get_commits_between(from_ref, to_ref):
    """Commit subjects between two refs, with housekeeping filtered out."""
    out, _ = run(f'git log {from_ref}..{to_ref} --no-merges --format=%s')
    if not out:
        return []
    return [line for line in out.splitlines()
            if not any(line.lower().startswith(s) for s in SKIP_PATTERNS)]


def categorize(items):
    """Auto-categorize items into Added / Improved / Fixed."""
    added, improved, fixed = [], [], []
    add_words = ('add', 'new', 'implement', 'create', 'support', 'introduce', 'enable')
    fix_words = ('fix', 'resolve', 'correct', 'bug', 'patch')
    for item in items:
        lower = item.lower().strip()
        if any(lower.startswith(w) for w in add_words):
            added.append(item)
        elif any(lower.startswith(w) for w in fix_words):
            fixed.append(item)
        else:
            improved.append(item)
    return added, improved, fixed


def build_entry(version, items, date_str=None):
    """Build a formatted changelog entry."""
    added, improved, fixed = categorize(items)
    if date_str is None:
        date_str = datetime.date.today().strftime('%Y-%m-%d')
    lines = [f"## v{version} \u2014 {date_str}", ""]
    if added:
        lines.append("### Added")
        lines.extend(f"- {i}" for i in added)
        lines.append("")
    if improved:
        lines.append("### Improved")
        lines.extend(f"- {i}" for i in improved)
        lines.append("")
    if fixed:
        lines.append("### Fixed")
        lines.extend(f"- {i}" for i in fixed)
        lines.append("")
    return "\n".join(lines)


def rebuild_changelog(versions, new_version=None, new_items=None):
    """Rebuild full CHANGELOG.md from all tags + optional new entry."""
    entries = []

    # New version entry at the top (unreleased changes)
    if new_version and new_items:
        entries.append(build_entry(new_version, new_items))

    # Past releases from tags (newest first)
    for i in range(len(versions) - 1, -1, -1):
        maj, min_, patch, tag = versions[i]
        ver = f"{maj}.{min_}.{patch}"
        date_str = get_tag_short_date(tag)

        if i > 0:
            prev_tag = versions[i - 1][3]
            items = get_commits_between(prev_tag, tag)
        else:
            # First release — limit to avoid noise from entire history
            out, _ = run(f'git log {tag} --no-merges --format=%s -50')
            items = [line for line in (out.splitlines() if out else [])
                     if not any(line.lower().startswith(s) for s in SKIP_PATTERNS)]

        if items:
            entries.append(build_entry(ver, items, date_str))
        else:
            entries.append(f"## v{ver} \u2014 {date_str}\n\n- Release\n")

    return "# Changelog\n\n" + "\n".join(entries)


def main():
    repo = get_repo()

    # ── Fetch ──────────────────────────────────────────────────────────────
    print("\n  Fetching latest from GitHub...")
    run("git fetch origin", capture=False)

    # ── Find latest tag ────────────────────────────────────────────────────
    versions = get_tags()
    if versions:
        last = versions[-1]
        last_ver = f"{last[0]}.{last[1]}.{last[2]}"
        last_tag = last[3]
    else:
        last_ver = "0.0.0"
        last_tag = None

    parts = last_ver.split(".")
    suggested = f"{parts[0]}.{parts[1]}.{int(parts[2]) + 1}"

    print(f"\n  Last release: {last_tag or 'none'}")
    print(f"  Next version: v{suggested}")

    # ── CHANGELOG — immediate, no prompts ─────────────────────────────────
    print("\n  Generating changelog...")

    tag_date = get_tag_date(last_tag) if last_tag else ""
    prs = get_merged_prs(repo, tag_date)

    if prs:
        new_items = [pr['title'] for pr in prs]
        print(f"  Found {len(prs)} merged PR(s) since {last_tag or 'beginning'}")
    else:
        if last_tag:
            new_items = get_commits_between(last_tag, 'origin/main')
        else:
            out, _ = run('git log origin/main --no-merges --format=%s -50')
            new_items = [line for line in (out.splitlines() if out else [])
                         if not any(line.lower().startswith(s) for s in SKIP_PATTERNS)]
        if new_items:
            print(f"  Using {len(new_items)} commit message(s)")
        else:
            print("  No unreleased changes.")

    print(f"  Rebuilding from {len(versions)} past release(s)...")
    full_changelog = rebuild_changelog(versions, new_version=suggested,
                                       new_items=new_items if new_items else None)

    with open(CHANGELOG_PATH, 'w', encoding='utf-8') as f:
        f.write(full_changelog)

    print(f"  CHANGELOG.md written to: {CHANGELOG_PATH}")

    if new_items:
        new_entry = build_entry(suggested, new_items)
        print(f"\n  {'─' * 60}")
        print(f"  Proposed v{suggested} entry:\n")
        for line in new_entry.splitlines():
            print(f"    {line}")
        print(f"  {'─' * 60}")

    # ── Release (optional) ────────────────────────────────────────────────
    print()
    release = input("  Publish a release? [y/N]: ").strip().lower()
    if release not in ("y", "yes"):
        print(f"\n  Done. CHANGELOG.md saved at: {CHANGELOG_PATH}\n")
        input("  Press Enter to exit...")
        sys.exit(0)

    # Version for release (default to suggested)
    version_input = input(f"\n  Version to release [v{suggested}]: ").strip()
    if not version_input:
        version_input = f"v{suggested}"
    if not version_input.startswith("v"):
        version_input = f"v{version_input}"

    if not re.match(r'^v\d+\.\d+\.\d+$', version_input):
        print(f"\n  Invalid version format: {version_input}")
        print("  Expected: v1.2.3\n")
        input("  Press Enter to exit...")
        sys.exit(1)

    version_number = version_input[1:]

    existing, _ = run(f"git tag -l {version_input}")
    if existing:
        print(f"\n  Tag {version_input} already exists.\n")
        input("  Press Enter to exit...")
        sys.exit(1)

    # If version differs from suggested, rebuild with the chosen version
    if version_number != suggested:
        full_changelog = rebuild_changelog(versions, new_version=version_number,
                                           new_items=new_items if new_items else None)
        with open(CHANGELOG_PATH, 'w', encoding='utf-8') as f:
            f.write(full_changelog)
        print(f"  CHANGELOG.md updated for v{version_number}.")

    # Offer to edit
    edit = input("\n  Edit changelog before releasing? [y/N]: ").strip().lower()
    if edit in ("y", "yes"):
        editor = os.environ.get('EDITOR', 'notepad')
        os.system(f'{editor} "{CHANGELOG_PATH}"')
        with open(CHANGELOG_PATH, 'r', encoding='utf-8') as f:
            full_changelog = f.read()
        print("\n  Changelog updated from editor.")

    # ── Confirm ────────────────────────────────────────────────────────────
    print(f"\n  This will trigger GitHub Actions to create tag {version_input}, build zip, and publish release.")
    print(f"  CHANGELOG.md will be auto-generated into the release ZIP from git history.")
    print()
    confirm = input("  Proceed? [y/N]: ").strip().lower()
    if confirm not in ("y", "yes"):
        print("\n  Cancelled.")
        print(f"  CHANGELOG.md saved locally at: {CHANGELOG_PATH}\n")
        input("  Press Enter to exit...")
        sys.exit(0)

    # ── Trigger release workflow ───────────────────────────────────────────
    print("\n  Triggering release workflow...")
    _, rc = run(f'gh workflow run release.yml -f version={version_number}')
    if rc != 0:
        print("  Failed to trigger workflow. Make sure 'gh' CLI is installed and authenticated.")
        input("  Press Enter to exit...")
        sys.exit(1)

    print(f"\n  Release workflow triggered for v{version_number}!")
    print(f"  Watch progress: https://github.com/{repo}/actions/workflows/release.yml")
    print(f"  Release will appear at: https://github.com/{repo}/releases/tag/{version_input}\n")
    input("  Press Enter to exit...")


if __name__ == "__main__":
    main()
