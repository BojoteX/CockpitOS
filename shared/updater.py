"""
CockpitOS -- Self-updater.

Downloads a release ZIP from GitHub, extracts to a staging directory, overlays
files onto the project root (skipping user-owned files), and restarts the tool.

A manifest file in %TEMP% tracks progress for crash recovery.  The overlay only
ADDS or OVERWRITES files -- it never deletes.  A partial overlay leaves a
working (if mixed-version) install that self-heals on the next update attempt.
"""

import fnmatch
import json
import os
import shutil
import sys
import tempfile
import time
import urllib.request
import zipfile

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

_REPO = "BojoteX/CockpitOS"
_DOWNLOAD_URL = "https://github.com/{repo}/releases/download/v{ver}/CockpitOS.zip"
_DOWNLOAD_TIMEOUT = 300          # 5 minutes for ~37 MB
_CHUNK_SIZE = 65536              # 64 KB

_PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

_STAGING_DIR = os.path.join(tempfile.gettempdir(), "cockpitos_update_staging")
_ZIP_PATH = os.path.join(tempfile.gettempdir(), "cockpitos_update.zip")
_MANIFEST = os.path.join(tempfile.gettempdir(), ".cockpitos_update_manifest.json")

# ANSI (duplicated to avoid importing tui -- keeps this module self-contained)
_CYAN = "\033[36m"
_GREEN = "\033[32m"
_YELLOW = "\033[33m"
_RED = "\033[31m"
_DIM = "\033[2m"
_BOLD = "\033[1m"
_RESET = "\033[0m"
_ERASE_LN = "\033[2K"

# Files to never overwrite when they already exist locally
_SKIP_FILES = frozenset({
    "src/LABELS/active_set.h",
    "compiler/compiler_prefs.json",
    "label_creator/creator_prefs.json",
    "HID Manager/settings.ini",
})

# Glob patterns to never overwrite
_SKIP_PATTERNS = [
    "src/LABELS/*/.last_run",
    ".credentials/*",
]


# ---------------------------------------------------------------------------
# Output helpers (thin wrappers so the updater can run standalone)
# ---------------------------------------------------------------------------

def _w(text):
    sys.stdout.write(text)
    sys.stdout.flush()


def _info(msg):
    _w(f"     {_CYAN}>{_RESET} {msg}\n")


def _success(msg):
    _w(f"     {_GREEN}>{_RESET} {msg}\n")


def _warn(msg):
    _w(f"     {_YELLOW}>{_RESET} {msg}\n")


def _error(msg):
    _w(f"     {_RED}>{_RESET} {msg}\n")


def _confirm(prompt):
    _w(f"     {_YELLOW}>{_RESET} {prompt} [Y/n]: ")
    try:
        import msvcrt
        while True:
            ch = msvcrt.getwch()
            if ch in ("y", "Y", "\r"):
                _w("Y\n")
                return True
            if ch in ("n", "N", "\x1b"):
                _w("N\n")
                return False
    except ImportError:
        return input().strip().lower() in ("", "y", "yes")


# ---------------------------------------------------------------------------
# Manifest (atomic writes for crash safety)
# ---------------------------------------------------------------------------

def _write_manifest(data):
    tmp = _MANIFEST + ".tmp"
    try:
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump(data, f)
        os.replace(tmp, _MANIFEST)  # atomic on NTFS
    except Exception:
        pass


def _read_manifest():
    try:
        with open(_MANIFEST, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return None


def _cleanup_manifest():
    for p in (_MANIFEST, _MANIFEST + ".tmp"):
        try:
            os.unlink(p)
        except OSError:
            pass


# ---------------------------------------------------------------------------
# Skip logic
# ---------------------------------------------------------------------------

def _should_skip(rel_path):
    """Return True if the file should NOT be overwritten during overlay."""
    normalized = rel_path.replace("\\", "/")

    is_protected = normalized in _SKIP_FILES
    if not is_protected:
        is_protected = any(fnmatch.fnmatch(normalized, p) for p in _SKIP_PATTERNS)

    if not is_protected:
        return False

    # Only skip if the local file already exists (fresh installs get defaults)
    local = os.path.join(_PROJECT_ROOT, normalized.replace("/", os.sep))
    return os.path.exists(local)


# ---------------------------------------------------------------------------
# Download
# ---------------------------------------------------------------------------

def _download_zip(version):
    """Download CockpitOS.zip with progress display. Returns True on success."""
    url = _DOWNLOAD_URL.format(repo=_REPO, ver=version)
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "CockpitOS-Updater"})
        with urllib.request.urlopen(req, timeout=_DOWNLOAD_TIMEOUT) as resp:
            total = int(resp.headers.get("Content-Length", 0))
            downloaded = 0

            with open(_ZIP_PATH, "wb") as f:
                while True:
                    chunk = resp.read(_CHUNK_SIZE)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)
                    if total > 0:
                        pct = downloaded * 100 // total
                        mb = downloaded / (1024 * 1024)
                        total_mb = total / (1024 * 1024)
                        _w(f"\r     {_DIM}Downloading... "
                           f"{mb:.1f} / {total_mb:.1f} MB ({pct}%){_RESET}  ")
                    else:
                        mb = downloaded / (1024 * 1024)
                        _w(f"\r     {_DIM}Downloading... {mb:.1f} MB{_RESET}  ")

            _w(f"\r{_ERASE_LN}")
            return True
    except Exception as e:
        _w(f"\r{_ERASE_LN}")
        _error(f"Download failed: {e}")
        return False


# ---------------------------------------------------------------------------
# Verify
# ---------------------------------------------------------------------------

def _verify_zip():
    """Verify ZIP integrity. Returns True if valid."""
    try:
        if not zipfile.is_zipfile(_ZIP_PATH):
            return False
        with zipfile.ZipFile(_ZIP_PATH, "r") as zf:
            bad = zf.testzip()
            if bad:
                return False
            # Sentinel check -- CockpitOS.ino must be in the archive
            names = zf.namelist()
            if "CockpitOS.ino" not in names and "./CockpitOS.ino" not in names:
                return False
        return True
    except Exception:
        return False


# ---------------------------------------------------------------------------
# Extract
# ---------------------------------------------------------------------------

def _extract_to_staging():
    """Extract ZIP to staging directory. Returns file count or -1 on failure."""
    try:
        if os.path.isdir(_STAGING_DIR):
            shutil.rmtree(_STAGING_DIR)

        with zipfile.ZipFile(_ZIP_PATH, "r") as zf:
            zf.extractall(_STAGING_DIR)
            return len([n for n in zf.namelist() if not n.endswith("/")])
    except Exception as e:
        _error(f"Extraction failed: {e}")
        return -1


# ---------------------------------------------------------------------------
# Overlay
# ---------------------------------------------------------------------------

def _overlay_files():
    """Copy files from staging to project root, skipping protected files.

    Returns (copied_count, skipped_count).
    """
    copied = 0
    skipped = 0
    errors = 0

    # Count total files for progress
    total = 0
    for root, dirs, files in os.walk(_STAGING_DIR):
        total += len(files)

    processed = 0
    for root, dirs, files in os.walk(_STAGING_DIR):
        for fname in files:
            src = os.path.join(root, fname)
            rel = os.path.relpath(src, _STAGING_DIR).replace("\\", "/")
            # Strip leading ./ if present
            if rel.startswith("./"):
                rel = rel[2:]
            dst = os.path.join(_PROJECT_ROOT, rel.replace("/", os.sep))

            processed += 1
            if processed % 50 == 0 or processed == total:
                _w(f"\r     {_DIM}Updating files... {processed}/{total}{_RESET}  ")

            if _should_skip(rel):
                skipped += 1
                continue

            try:
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.copy2(src, dst)
                copied += 1
            except PermissionError:
                # File in use (e.g., arduino-cli.exe) -- skip, non-fatal
                errors += 1
            except Exception:
                errors += 1

    _w(f"\r{_ERASE_LN}")

    if errors:
        _warn(f"{errors} file(s) could not be updated (in use or permission denied)")

    return copied, skipped


# ---------------------------------------------------------------------------
# Cleanup
# ---------------------------------------------------------------------------

def _cleanup_staging():
    """Remove staging directory and ZIP file."""
    try:
        if os.path.isdir(_STAGING_DIR):
            shutil.rmtree(_STAGING_DIR)
    except Exception:
        pass
    try:
        if os.path.isfile(_ZIP_PATH):
            os.unlink(_ZIP_PATH)
    except Exception:
        pass


# ---------------------------------------------------------------------------
# Restart
# ---------------------------------------------------------------------------

def _restart_tool(lock_path, caller_script):
    """Clean up lock file and os.execl into the caller script."""
    if lock_path:
        try:
            lock_path.unlink(missing_ok=True)
        except Exception:
            pass
    try:
        os.execl(sys.executable, sys.executable, str(caller_script))
    except Exception as e:
        _error(f"Could not restart automatically: {e}")
        _info(f"Please close this window and double-click START.py to launch the updated version.")


# ---------------------------------------------------------------------------
# Public API: interrupted update recovery
# ---------------------------------------------------------------------------

def check_interrupted_update():
    """Check for an interrupted update from a previous session.

    Returns the manifest dict if found, None otherwise.
    Silently cleans up non-destructive phases (download/extract).
    """
    manifest = _read_manifest()
    if not manifest:
        return None

    phase = manifest.get("phase", "")

    # Phases that made no changes to the install -- clean up silently
    if phase in ("downloading", "extracting"):
        _cleanup_staging()
        _cleanup_manifest()
        return None

    # Phases that need user attention
    if phase in ("overlaying", "complete"):
        return manifest

    # Unknown phase -- clean up
    _cleanup_staging()
    _cleanup_manifest()
    return None


def resume_interrupted_update(manifest, lock_path, caller_script):
    """Resume or clean up an interrupted update."""
    phase = manifest.get("phase", "")
    version = manifest.get("version", "unknown")

    if phase == "complete":
        # Overlay finished but restart didn't happen -- just clean up
        _info(f"Previous update to v{version} completed successfully.")
        _cleanup_staging()
        _cleanup_manifest()
        return

    if phase == "overlaying":
        if os.path.isdir(_STAGING_DIR):
            _warn(f"A previous update to v{version} was interrupted during installation.")
            print()
            if _confirm(f"Resume the update to v{version}?"):
                print()
                _info("Resuming update...")
                copied, skipped = _overlay_files()
                _cleanup_staging()
                _cleanup_manifest()
                print()
                _success(f"Update to v{version} complete! "
                         f"{copied} files updated, {skipped} user files preserved.")
                _info("Restarting...")
                time.sleep(1)
                _restart_tool(lock_path, caller_script)
            else:
                _info("Cleaning up. You can re-run the update from the menu.")
                _cleanup_staging()
                _cleanup_manifest()
        else:
            _warn("Previous update staging files are missing.")
            _info("Cleaning up. You can re-run the update from the menu.")
            _cleanup_manifest()
        return


# ---------------------------------------------------------------------------
# Public API: perform update
# ---------------------------------------------------------------------------

def perform_update(target_version, lock_path, caller_script):
    """Download, extract, overlay, and restart into the new version.

    Called when the user selects "Update CockpitOS" from any tool menu.
    Either restarts the process on success or returns on failure/cancel.
    """
    from shared.update_check import get_local_version

    current = get_local_version()
    print()
    _info(f"Current version: {_CYAN}v{current}{_RESET}")
    _info(f"New version:     {_CYAN}v{target_version}{_RESET}")
    print()

    # Git repo warning
    if os.path.isdir(os.path.join(_PROJECT_ROOT, ".git")):
        _warn("Git repository detected. Source files will be overwritten.")
        _info("Your .git history and branches are NOT affected.")
        print()

    if not _confirm(f"Update CockpitOS to v{target_version}?"):
        _info("Update cancelled.")
        time.sleep(1)
        return

    print()

    # Clean up any stale state from previous attempts
    _cleanup_staging()
    _cleanup_manifest()

    # ── Phase 1: Download ─────────────────────────────────────────────────
    _write_manifest({"phase": "downloading", "version": target_version,
                     "started_at": time.time()})

    _info(f"Downloading CockpitOS v{target_version}...")
    if not _download_zip(target_version):
        _cleanup_staging()
        _cleanup_manifest()
        _error("Update failed during download.")
        time.sleep(2)
        return

    if not _verify_zip():
        _error("Downloaded file is corrupt or not a valid CockpitOS release.")
        _cleanup_staging()
        _cleanup_manifest()
        time.sleep(2)
        return

    _success("Download complete.")

    # ── Phase 2: Extract ──────────────────────────────────────────────────
    _write_manifest({"phase": "extracting", "version": target_version,
                     "started_at": time.time()})

    _info("Extracting...")
    file_count = _extract_to_staging()
    if file_count < 0:
        _cleanup_staging()
        _cleanup_manifest()
        _error("Update failed during extraction.")
        time.sleep(2)
        return

    _success(f"Extracted {file_count} files.")

    # ── Phase 3: Overlay ──────────────────────────────────────────────────
    _write_manifest({"phase": "overlaying", "version": target_version,
                     "staging_dir": _STAGING_DIR, "started_at": time.time()})

    _info("Updating files...")
    copied, skipped = _overlay_files()

    _write_manifest({"phase": "complete", "version": target_version,
                     "started_at": time.time()})

    # ── Phase 4: Cleanup & restart ────────────────────────────────────────
    _cleanup_staging()
    _cleanup_manifest()

    print()
    print(f"     {_GREEN}{_BOLD}CockpitOS updated to v{target_version}!{_RESET}")
    print(f"     {_DIM}{copied} files updated, {skipped} user files preserved{_RESET}")
    print()
    _info("Restarting...")
    time.sleep(2)

    _restart_tool(lock_path, caller_script)
