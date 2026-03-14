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
    """Resolve local version from git tags. Returns a string like '1.2.12'."""
    try:
        tag = subprocess.run(
            ["git", "tag", "--sort=-v:refname", "-l", "v*"],
            capture_output=True, text=True,
            cwd=_PROJECT_ROOT, timeout=5
        )
        if tag.returncode == 0 and tag.stdout.strip():
            first_tag = tag.stdout.strip().splitlines()[0]
            sha = subprocess.run(
                ["git", "rev-parse", "--short", "HEAD"],
                capture_output=True, text=True,
                cwd=_PROJECT_ROOT, timeout=5
            ).stdout.strip()

            ver = first_tag.lstrip("v")
            if sha:
                count = subprocess.run(
                    ["git", "rev-list", "--count", f"{first_tag}..HEAD"],
                    capture_output=True, text=True,
                    cwd=_PROJECT_ROOT, timeout=5
                ).stdout.strip()
                ahead = int(count) if count.isdigit() else 0
                if ahead > 0:
                    ver += f"+{ahead}.{sha}"
            return ver
    except Exception:
        pass

    # Fallback: try reading version.h
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
    """Write cache file."""
    try:
        with open(_CACHE_FILE, "w") as f:
            json.dump({"ts": time.time(), "latest": latest}, f)
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


def update_available():
    """Return the newer version string if an update is available, None otherwise.

    Lightweight wrapper for menu logic -- use this to decide whether to show
    the "Update CockpitOS" menu item.
    """
    ver = get_local_version()
    return check_for_update(ver)


def version_line():
    """Return a formatted version + update-available line for the TUI menu.

    Example outputs:
        "     v1.2.12"
        "     v1.2.12    Update available: v1.3.0"
    """
    ver = get_local_version()
    line = f"     {_DIM}v{ver}{_RESET}"

    newer = check_for_update(ver)
    if newer:
        line += f"    {_YELLOW}Update available: v{newer}{_RESET}"

    return line
