"""
CockpitOS -- Update checker.

Queries GitHub for the latest release and compares it to the local version.
Results are cached to a temp file for 24 hours to avoid repeated API calls.
Network timeout is 3 seconds -- worst case on cold cache, instant thereafter.
"""

import json
import os
import re
import subprocess
import tempfile
import time
import urllib.request

_CACHE_FILE = os.path.join(tempfile.gettempdir(), ".cockpitos_update_check")
_CACHE_TTL = 86400  # 24 hours
_REPO = "BojoteX/CockpitOS"
_TIMEOUT = 3  # seconds
_RELEASES_URL = f"https://github.com/{_REPO}/releases"

# Where to look for git (project root = two levels up from shared/)
_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


# ---------------------------------------------------------------------------
# Semver helpers
# ---------------------------------------------------------------------------

def _parse_semver(version_str):
    """Extract (major, minor, patch) tuple from a version string."""
    m = re.match(r'v?(\d+)\.(\d+)\.(\d+)', version_str)
    if m:
        return (int(m.group(1)), int(m.group(2)), int(m.group(3)))
    return None


# ---------------------------------------------------------------------------
# Local version (from git tags -- same logic as compiler/config.py)
# ---------------------------------------------------------------------------

def get_local_version():
    """Resolve local version from git tags or version.h.

    In a git repo, git describe is the primary source — it tracks HEAD
    accurately across branches and tag switches.  version.h is gitignored
    and can go stale, so it is only used as a fallback when git describe
    fails, or as the primary source in release ZIP installs (no .git).
    """
    has_git = os.path.isdir(os.path.join(_PROJECT_ROOT, ".git"))

    # Git repo: git describe is the authoritative source
    if has_git:
        try:
            desc = subprocess.run(
                ["git", "describe", "--tags", "--long", "--match", "v*"],
                capture_output=True, text=True,
                cwd=_PROJECT_ROOT, timeout=5
            )
            if desc.returncode == 0 and desc.stdout.strip():
                # Format: v1.2.16-0-gabcdef  or  v1.2.16-3-gabcdef
                parts = desc.stdout.strip().rsplit("-", 2)
                first_tag = parts[0]
                ahead = int(parts[1]) if len(parts) >= 3 else 0
                sha = parts[2].lstrip("g") if len(parts) >= 3 else ""

                ver = first_tag.lstrip("v")
                if ahead > 0:
                    ver += f"+{ahead}.{sha}"
                return ver
        except Exception:
            pass

    # No .git (release ZIP) or git describe failed: use version.h
    vh = os.path.join(_PROJECT_ROOT, "version.h")
    try:
        with open(vh, "r", encoding="utf-8") as f:
            m = re.search(r'VERSION_CURRENT\s+"(.+?)"', f.read())
            if m and not m.group(1).startswith("dev-"):
                return m.group(1)
    except OSError:
        pass

    return "unknown"


# ---------------------------------------------------------------------------
# GitHub release check (cached)
# ---------------------------------------------------------------------------

def _fetch_latest():
    """Query GitHub API for latest release tag. Returns version string or None."""
    try:
        url = f"https://api.github.com/repos/{_REPO}/releases/latest"
        req = urllib.request.Request(url, headers={
            "Accept": "application/vnd.github+json",
            "User-Agent": "CockpitOS-UpdateCheck",
        })
        with urllib.request.urlopen(req, timeout=_TIMEOUT) as resp:
            data = json.loads(resp.read().decode())
            return data.get("tag_name", "").lstrip("v")
    except Exception:
        return None


def _read_cache():
    """Read cached result. Returns (timestamp, latest_version) or (0, None)."""
    try:
        with open(_CACHE_FILE, "r") as f:
            data = json.load(f)
            return data.get("ts", 0), data.get("latest")
    except Exception:
        return 0, None


def _write_cache(latest):
    """Write cache file (atomic via tmp + replace)."""
    tmp = _CACHE_FILE + ".tmp"
    try:
        with open(tmp, "w") as f:
            json.dump({"ts": time.time(), "latest": latest}, f)
        os.replace(tmp, _CACHE_FILE)
    except Exception:
        pass


def check_for_update(current_version):
    """Check if a newer release is available.

    Returns the latest version string if newer, None otherwise.
    Uses a 24-hour cache to avoid repeated API calls.
    """
    current = _parse_semver(current_version)
    if not current:
        return None

    # Check cache first
    ts, cached_latest = _read_cache()
    if time.time() - ts < _CACHE_TTL and cached_latest:
        latest = _parse_semver(cached_latest)
        if latest and latest > current:
            return cached_latest
        return None

    # Cache expired or missing -- fetch from GitHub
    fetched = _fetch_latest()
    if fetched:
        _write_cache(fetched)
        latest = _parse_semver(fetched)
        if latest and latest > current:
            return fetched
    else:
        # Fetch failed -- extend cache to avoid retrying immediately
        if cached_latest:
            _write_cache(cached_latest)

    return None


# ---------------------------------------------------------------------------
# Formatted banner line (ready to print)
# ---------------------------------------------------------------------------

# ANSI codes (duplicated here to avoid importing tui.py -- keeps this module
# lightweight and importable from any tool without side effects)
_YELLOW = "\033[33m"
_DIM = "\033[2m"
_RESET = "\033[0m"


def is_git_repo():
    """Return True if running from a git clone (has .git directory)."""
    return os.path.isdir(os.path.join(_PROJECT_ROOT, ".git"))


def update_available():
    """Return the newer version string if an update is available, None otherwise.

    Lightweight wrapper for menu logic -- use this to decide whether to show
    the "Update CockpitOS" menu item.  Returns None for git repos — developers
    should pull updates via git, not the auto-updater.
    """
    if is_git_repo():
        return None
    ver = get_local_version()
    return check_for_update(ver)


def version_tag():
    """Return a short styled version string for embedding in the banner subtitle.

    Example: "  v1.3.0" (dim, no leading spaces beyond the separator).
    """
    ver = get_local_version()
    return f"  {_DIM}v{ver}{_RESET}"


def version_line():
    """Return a formatted update-available notice for the TUI menu.

    Returns an empty string if no update is available (version is shown
    in the banner subtitle via version_tag() instead).

    Example outputs:
        ""
        "     Update available: v1.3.0"
        "     New release: v1.3.0 — git pull to update"
    """
    ver = get_local_version()

    if is_git_repo():
        newer = check_for_update(ver)
        if newer:
            return (f"     {_YELLOW}New release: v{newer}"
                    f" — git pull to update{_RESET}")
    else:
        newer = check_for_update(ver)
        if newer:
            return f"     {_YELLOW}Update available: v{newer}{_RESET}"

    return ""
