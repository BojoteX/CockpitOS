#!/usr/bin/env python3
"""
CockpitOS Environment Setup
==============================
Installs the ESP32 Arduino core and required libraries using the
bundled arduino-cli. No Arduino IDE needed.

Components are installed into STANDARD Arduino locations so the
compiler tool (and any future Arduino IDE) share the same environment:
  - ESP32 core + index  → %LOCALAPPDATA%/Arduino15/  (standard)
  - Libraries           → ~/Documents/Arduino/libraries/  (standard)
  - arduino-cli binary  → compiler/arduino-cli/  (bundled with project)

Double-click this file or run:  python Setup-START.py
"""

import sys
import platform

if platform.system() != "Windows":
    print("\n  CockpitOS Setup requires Windows.")
    print("  Linux and macOS are not supported.\n")
    sys.exit(1)

import os
import json
import time
import shutil
import subprocess
import msvcrt
import ctypes
import re
import zipfile
import tempfile
import urllib.request
import winreg
from pathlib import Path

# =============================================================================
#  MANIFEST — Source of truth for recommended versions
#  arduino-cli v1.4.1 is bundled at compiler/arduino-cli/arduino-cli.exe
# =============================================================================
MANIFEST = {
    "esp32_core": {
        "platform": "esp32:esp32",
        "version": "3.3.6",
        "board_manager_url": "https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json",
    },
    "lovyangfx": {
        "library": "LovyanGFX",
        "version": "1.2.19",
    },
    "hid_manager_deps": {
        "packages": {
            "hid":      "hidapi",
            "filelock": "filelock",
            "curses":   "windows-curses",
            "ifaddr":   "ifaddr",
        },
    },
    "dcsbios": {
        # release: "latest"       — latest official release (default)
        #          "snapshot"     — development / test release (including pre-releases)
        #          "v0.8.4"      — exact version tag
        "release": "latest",
        # build_from_lua: when True, compiles JSONs from DCS-BIOS LUA modules
        # using the bundled lua5.1 interpreter instead of copying pre-built JSONs
        "build_from_lua": False,
    },
}

# Frozen defaults — always available as reference for the user
MANIFEST_DEFAULTS = {
    "esp32_core_version": "3.3.6",
    "lovyangfx_version":  "1.2.19",
    "dcsbios_release":    "latest",
}

# =============================================================================
#  Paths
# =============================================================================
SCRIPT_DIR = Path(__file__).resolve().parent
CLI_EXE = SCRIPT_DIR / "compiler" / "arduino-cli" / "arduino-cli.exe"

# =============================================================================
#  ANSI (mirrors compiler/ui.py)
# =============================================================================
CYAN     = "\033[96m"
GREEN    = "\033[92m"
YELLOW   = "\033[93m"
RED      = "\033[91m"
BOLD     = "\033[1m"
DIM      = "\033[2m"
RESET    = "\033[0m"
REV      = "\033[7m"
HIDE_CUR = "\033[?25l"
SHOW_CUR = "\033[?25h"
ERASE_LN = "\033[2K"


def _enable_ansi():
    """Enable ANSI escape processing on Windows 10+."""
    kernel32 = ctypes.windll.kernel32
    h = kernel32.GetStdHandle(-11)          # STD_OUTPUT_HANDLE
    mode = ctypes.c_ulong()
    kernel32.GetConsoleMode(h, ctypes.byref(mode))
    kernel32.SetConsoleMode(h, mode.value | 0x0004)

_enable_ansi()


def _w(text):
    """Write raw text to stdout (no newline)."""
    try:
        sys.stdout.write(text)
        sys.stdout.flush()
    except UnicodeEncodeError:
        sys.stdout.write(text.encode("ascii", "replace").decode())
        sys.stdout.flush()


def cls():
    os.system("cls")


def info(text):
    _w(f"       {text}\n")


def success(text):
    _w(f"  {GREEN}[OK]{RESET} {text}\n")


def warn(text):
    _w(f"  {YELLOW}[!]{RESET}  {text}\n")


def error(text):
    _w(f"  {RED}[X]{RESET}  {text}\n")


def step_banner(num, total, text):
    _w(f"\n  {CYAN}[{num}/{total}]{RESET} {BOLD}{text}{RESET}\n\n")


def big_success(title, details=None):
    cols = min(shutil.get_terminal_size((80, 24)).columns, 72)
    border = "=" * cols
    _w(f"\n{GREEN}{border}{RESET}\n")
    _w(f"{GREEN}{BOLD}  {title}{RESET}\n")
    if details:
        for d in details:
            _w(f"{GREEN}  {d}{RESET}\n")
    _w(f"{GREEN}{border}{RESET}\n\n")


def big_fail(title, details=None):
    cols = min(shutil.get_terminal_size((80, 24)).columns, 72)
    border = "=" * cols
    _w(f"\n{RED}{border}{RESET}\n")
    _w(f"{RED}{BOLD}  {title}{RESET}\n")
    if details:
        for d in details:
            _w(f"{RED}  {d}{RESET}\n")
    _w(f"{RED}{border}{RESET}\n\n")


def confirm(prompt):
    """Y/n prompt. Returns True (Enter/Y) or False (N/Esc)."""
    _w(f"\n  {prompt} [{BOLD}Y{RESET}/n]: ")
    while True:
        ch = msvcrt.getwch()
        if ch in ("\r", "y", "Y"):
            _w("y\n")
            return True
        if ch in ("n", "N", "\x1b"):
            _w("n\n")
            return False


SEP_COLOR = "\033[38;5;240m"       # subtle gray for separators

def pick_action(prompt, options):
    """Arrow-key picker matching compiler tool style.

    options: list of tuples:
      ("label", "value")              — selectable row (normal style)
      ("label", "value", "dim")       — selectable row (dim style)
      ("---", "Label Text")           — labeled separator (not selectable)
      ("---",)                        — plain separator line (not selectable)
      ("",)                           — blank spacer row (not selectable)

    Returns selected value, or None on ESC.
    """
    if not options:
        return None

    # Build rows
    rows = []
    selectable = []     # indices into rows[] that are selectable

    for item in options:
        if len(item) == 1 and item[0] == "":
            rows.append("blank")
        elif item[0] == "---":
            if len(item) >= 2:
                label = item[1]
                total_w = 36
                pad = total_w - len(label) - 2
                left = pad // 2
                right = pad - left
                dash = "\u2500"
                rows.append(f"     {SEP_COLOR}{dash * left} {label} {dash * right}{RESET}")
            else:
                line = "\u2500" * 36
                rows.append(f"     {SEP_COLOR}{line}{RESET}")
        else:
            label, value = item[0], item[1]
            style = item[2] if len(item) >= 3 else "normal"
            selectable.append(len(rows))
            rows.append(("selectable", label, value, style))

    if not selectable:
        return None

    sel = 0                 # index into selectable[]
    total_rows = len(rows)

    def _render_row(ri, is_sel):
        entry = rows[ri]
        if entry == "blank":
            return ""
        if isinstance(entry, str):
            return entry
        _, label, _, style = entry
        if is_sel:
            return f"  {REV} > {label} {RESET}"
        if style == "dim":
            return f"     {DIM}{label}{RESET}"
        return f"     {label}"

    print()
    _w(f"  {BOLD}{prompt}{RESET}\n")
    _w(f"  {DIM}(arrows to move, Enter to select, Esc to go back){RESET}\n")

    # Initial draw
    _w(HIDE_CUR)
    for ri in range(total_rows):
        is_sel = (ri == selectable[sel])
        _w(_render_row(ri, is_sel) + "\n")

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                old_sel = sel
                if ch2 == "H":          # Up
                    sel = (sel - 1) % len(selectable)
                elif ch2 == "P":        # Down
                    sel = (sel + 1) % len(selectable)
                else:
                    continue
                if old_sel != sel:
                    old_ri = selectable[old_sel]
                    new_ri = selectable[sel]
                    # Redraw old row (un-highlight)
                    _w(f"\033[{total_rows - old_ri}A")
                    _w(f"\r{ERASE_LN}{_render_row(old_ri, False)}")
                    # Redraw new row (highlight)
                    if new_ri > old_ri:
                        _w(f"\033[{new_ri - old_ri}B")
                    else:
                        _w(f"\033[{old_ri - new_ri}A")
                    _w(f"\r{ERASE_LN}{_render_row(new_ri, True)}")
                    # Return cursor to bottom
                    remaining = total_rows - new_ri
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")

            elif ch == "\x1b":      # ESC — go back
                _w(SHOW_CUR)
                return None

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                _, _, value, _ = rows[selectable[sel]]
                return value
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def version_tuple(v):
    """Parse '3.3.6' into (3, 3, 6) for comparison."""
    try:
        return tuple(int(x) for x in v.split("."))
    except (ValueError, AttributeError):
        return (0,)


def show_banner():
    cls()
    _w("\n")
    _w(f"{CYAN}{BOLD}        ____            _          _ _    ___  ____{RESET}\n")
    _w(f"{CYAN}{BOLD}       / ___|___   ___ | | ___ __ (_) |_ / _ \\/ ___|{RESET}\n")
    _w(f"{CYAN}{BOLD}      | |   / _ \\ / __|| |/ / '_ \\| | __| | | \\___ \\{RESET}\n")
    _w(f"{CYAN}{BOLD}      | |__| (_) | (__ |   <| |_) | | |_| |_| |___) |{RESET}\n")
    _w(f"{CYAN}{BOLD}       \\____\\___/ \\___|_|\\_\\ .__/|_|\\__|\\___/|____/{RESET}\n")
    _w(f"{CYAN}{BOLD}                            |_|{RESET}\n")
    _w(f"{CYAN}{BOLD}              Environment Setup{RESET}\n\n")


# =============================================================================
#  arduino-cli wrapper
# =============================================================================
def run_cli(*args, capture=True, timeout=600):
    """Run the bundled arduino-cli. Uses standard Arduino dirs (no overrides)."""
    cmd = [str(CLI_EXE)] + list(args)
    try:
        result = subprocess.run(
            cmd,
            capture_output=capture,
            text=True,
            timeout=timeout,
            creationflags=subprocess.CREATE_NO_WINDOW if capture else 0,
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Timed out"
    except FileNotFoundError:
        return -1, "", f"arduino-cli not found at {CLI_EXE}"


# =============================================================================
#  Version queries
# =============================================================================
def get_installed_core_version(platform_id):
    """Return installed version string for a core, or None."""
    rc, out, _ = run_cli("core", "list", "--format", "json")
    if rc != 0:
        return None
    try:
        data = json.loads(out)
        platforms = data.get("platforms", data) if isinstance(data, dict) else data
        for core in platforms:
            cid = core.get("id", "")
            ver = core.get("installed_version", "")
            if cid == platform_id and ver:
                return ver
    except (json.JSONDecodeError, TypeError, AttributeError):
        pass
    return None


def get_installed_lib_version(lib_name):
    """Return installed version string for a library, or None."""
    rc, out, _ = run_cli("lib", "list", "--format", "json")
    if rc != 0:
        return None
    try:
        data = json.loads(out)
        libs = data.get("installed_libraries", data) if isinstance(data, dict) else data
        for entry in libs:
            lib = entry.get("library", entry)
            name = lib.get("name", "")
            ver = lib.get("version", "")
            if name == lib_name and ver:
                return ver
    except (json.JSONDecodeError, TypeError, AttributeError):
        pass
    return None


def get_cli_version():
    """Return arduino-cli version string, or None."""
    rc, out, _ = run_cli("version", "--format", "json")
    if rc != 0:
        return None
    try:
        return json.loads(out).get("VersionString")
    except (json.JSONDecodeError, KeyError):
        return None


# =============================================================================
#  HID Manager dependency helpers
# =============================================================================
def get_missing_hid_deps():
    """Return list of (import_name, pip_name) tuples for missing HID Manager deps."""
    packages = MANIFEST["hid_manager_deps"]["packages"]
    missing = []
    for mod, pip_name in packages.items():
        try:
            __import__(mod)
        except ImportError:
            missing.append((mod, pip_name))
    return missing


def run_pip(*args, timeout=120):
    """Run pip via sys.executable -m pip. Returns (returncode, stdout, stderr)."""
    cmd = [sys.executable, "-m", "pip"] + list(args)
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=timeout,
            creationflags=subprocess.CREATE_NO_WINDOW,
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return -1, "", "Timed out"
    except FileNotFoundError:
        return -1, "", "Python executable not found"


# =============================================================================
#  DCS World detection
# =============================================================================
DCSBIOS_LINE = 'dofile(lfs.writedir() .. [[Scripts\\DCS-BIOS\\BIOS.lua]])'
DCSBIOS_REPO = "DCS-Skunkworks/dcs-bios"
DCSBIOS_API_BASE = f"https://api.github.com/repos/{DCSBIOS_REPO}/releases"
DCSBIOS_DEV_DIR = SCRIPT_DIR / "Dev" / "DCS-BIOS-LUA"
AIRCRAFT_JSON_DIR = SCRIPT_DIR / "src" / "LABELS" / "_core" / "aircraft"
METADATA_JSON_DIR = SCRIPT_DIR / "src" / "LABELS" / "_core" / "metadata"
LUA51_EXE = SCRIPT_DIR / "lua51" / "lua5.1.exe"
LUA51_COMPILER = SCRIPT_DIR / "lua51" / "compile_dcsbios.lua"


def _dcsbios_release_url():
    """Build the GitHub API URL based on the manifest release strategy.

    Returns (api_url, strategy_label):
      "latest"    -> /releases/latest, "latest official"
      "snapshot"  -> /releases?per_page=1, "development / test"
      "v0.8.4"   -> /releases/tags/v0.8.4, "version v0.8.4"
    """
    strategy = MANIFEST["dcsbios"].get("release", "latest")

    if strategy == "latest":
        return f"{DCSBIOS_API_BASE}/latest", "latest official"
    elif strategy == "snapshot":
        return f"{DCSBIOS_API_BASE}?per_page=1", "development / test"
    else:
        # Specific version tag
        tag = strategy if strategy.startswith("v") else f"v{strategy}"
        return f"{DCSBIOS_API_BASE}/tags/{tag}", f"version {tag}"


def _get_saved_games_folder():
    """Get the Windows Saved Games folder using the Shell API.

    Uses SHGetKnownFolderPath with FOLDERID_SavedGames to handle cases
    where the user has relocated their Saved Games folder to another drive.
    Falls back to %USERPROFILE%\\Saved Games if the API call fails.
    """
    fallback = Path(os.environ.get("USERPROFILE", "")) / "Saved Games"
    try:
        import ctypes.wintypes
        # FOLDERID_SavedGames = {4C5C32FF-BB9D-43B0-B5B4-2D72E54EAAA4}
        guid_bytes = (ctypes.c_ubyte * 16)(
            0xFF, 0x32, 0x5C, 0x4C, 0x9D, 0xBB, 0xB0, 0x43,
            0xB5, 0xB4, 0x2D, 0x72, 0xE5, 0x4E, 0xAA, 0xA4,
        )
        buf = ctypes.c_wchar_p()
        hr = ctypes.windll.shell32.SHGetKnownFolderPath(
            ctypes.byref(guid_bytes), 0, None, ctypes.byref(buf)
        )
        if hr == 0 and buf.value:
            result = Path(buf.value)
            ctypes.windll.ole32.CoTaskMemFree(buf)
            if result.is_dir():
                return result
    except Exception:
        pass
    return fallback


def _find_dcs_from_registry():
    """Search Windows registry for DCS World Saved Games paths.

    Checks Eagle Dynamics registry keys for both stable and OpenBeta.
    Returns list of (label, saved_games_path) tuples.
    """
    results = []
    saved_games = _get_saved_games_folder()
    keys_to_check = [
        (r"SOFTWARE\Eagle Dynamics\DCS World", "DCS"),
        (r"SOFTWARE\Eagle Dynamics\DCS World OpenBeta", "DCS.openbeta"),
    ]

    for reg_path, folder_name in keys_to_check:
        try:
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, reg_path)
            try:
                install_path, _ = winreg.QueryValueEx(key, "Path")
                key.Close()
                # Registry confirms DCS exists — check for Saved Games folder
                saved = saved_games / folder_name
                if saved.is_dir():
                    results.append((folder_name, saved))
            except OSError:
                key.Close()
        except OSError:
            pass

    return results


def _find_dcs_from_saved_games():
    """Fallback: scan Saved Games for DCS directories."""
    results = []
    saved_games = _get_saved_games_folder()
    if not saved_games.is_dir():
        return results

    for entry in saved_games.iterdir():
        if not entry.is_dir():
            continue
        name = entry.name
        # Match DCS, DCS.openbeta, DCS.server, etc.
        if name == "DCS" or name.startswith("DCS."):
            results.append((name, entry))

    return results


def find_dcs_saved_games():
    """Find DCS World Saved Games directories.

    Tries registry first, falls back to directory scanning.
    Returns list of (label, path) tuples, deduplicated.
    """
    results = _find_dcs_from_registry()

    # Fallback: scan Saved Games directly
    if not results:
        results = _find_dcs_from_saved_games()
    else:
        # Merge any dirs found by scanning that registry missed
        reg_paths = {p for _, p in results}
        for label, path in _find_dcs_from_saved_games():
            if path not in reg_paths:
                results.append((label, path))

    return results


def _read_dcsbios_version_from_dir(bios_dir):
    """Extract DCS-BIOS version from a DCS-BIOS installation directory.

    Priority:
    1. .dcsbios_version marker (written by this tool)
    2. CommonData.lua getVersion() return value
    3. None if directory doesn't look like a DCS-BIOS install
    """
    if not bios_dir.is_dir() or not (bios_dir / "BIOS.lua").exists():
        return None

    # 1. Our own marker
    marker = bios_dir / ".dcsbios_version"
    if marker.exists():
        try:
            return marker.read_text().strip()
        except OSError:
            pass

    # 2. Parse version from CommonData.lua
    common_data = bios_dir / "lib" / "modules" / "common_modules" / "CommonData.lua"
    if common_data.exists():
        try:
            content = common_data.read_text(encoding="utf-8")
            # Match:  return "0.8.4"  (inside getVersion function)
            m = re.search(r'function\s+getVersion\s*\(\s*\).*?return\s+"([^"]+)"',
                          content, re.DOTALL)
            if m:
                return m.group(1)
        except OSError:
            pass

    return "installed"


def get_dcsbios_version(dcs_path):
    """Return installed DCS-BIOS version string, or None."""
    return _read_dcsbios_version_from_dir(dcs_path / "Scripts" / "DCS-BIOS")


def get_dcsbios_dev_version():
    """Return DCS-BIOS version from the local Dev/DCS-BIOS-LUA fallback, or None."""
    return _read_dcsbios_version_from_dir(DCSBIOS_DEV_DIR / "DCS-BIOS")


# =============================================================================
#  DCS-BIOS MTU / MAX_PAYLOAD_SIZE patch
# =============================================================================
DCSBIOS_RECOMMENDED_MTU = 1460  # Must match Config.h UDP_MAX_SIZE


def _get_connection_manager_path(dcs_path):
    """Return Path to ConnectionManager.lua for a DCS saved games path, or None."""
    p = dcs_path / "Scripts" / "DCS-BIOS" / "lib" / "ConnectionManager.lua"
    return p if p.exists() else None


def get_dcsbios_mtu(dcs_path):
    """Read MAX_PAYLOAD_SIZE from ConnectionManager.lua. Returns int or None."""
    lua = _get_connection_manager_path(dcs_path)
    if lua is None:
        return None
    try:
        for line in lua.read_text(encoding="utf-8").splitlines():
            m = re.match(r'\s*MAX_PAYLOAD_SIZE\s*=\s*(\d+)', line)
            if m:
                return int(m.group(1))
    except OSError:
        pass
    return None


def patch_dcsbios_mtu(dcs_path):
    """Patch MAX_PAYLOAD_SIZE to DCSBIOS_RECOMMENDED_MTU. Returns (ok, message)."""
    lua = _get_connection_manager_path(dcs_path)
    if lua is None:
        return False, "ConnectionManager.lua not found"
    try:
        content = lua.read_text(encoding="utf-8")
        new_content, count = re.subn(
            r'(MAX_PAYLOAD_SIZE\s*=\s*)\d+',
            rf'\g<1>{DCSBIOS_RECOMMENDED_MTU}',
            content,
        )
        if count == 0:
            return False, "MAX_PAYLOAD_SIZE not found in file"
        lua.write_text(new_content, encoding="utf-8")
        return True, f"Patched to {DCSBIOS_RECOMMENDED_MTU}"
    except OSError as e:
        return False, str(e)


def _pick_dcs_path(dcs_list):
    """Let user pick a DCS installation if multiple found. Returns (label, path) or None."""
    if len(dcs_list) == 1:
        return dcs_list[0]

    options = [(label, str(path)) for label, path in dcs_list]
    _w(f"\n  {BOLD}Multiple DCS installations found:{RESET}\n")
    choice = pick_action("Which DCS installation?", options)
    if choice is None:
        return None
    # Find the matching tuple
    for label, path in dcs_list:
        if str(path) == choice:
            return (label, path)
    return None


# =============================================================================
#  Retry wrapper for network operations
# =============================================================================
MAX_RETRIES = 3
RETRY_DELAYS = [5, 15, 30]       # seconds between attempts

def retry(label, fn, retries=MAX_RETRIES):
    """Call fn() up to `retries` times, with delay between failures.

    fn() must return (rc, stdout, stderr).
    Shows a countdown between attempts so the user knows it's working.
    Returns the final (rc, stdout, stderr).
    """
    for attempt in range(retries):
        rc, out, err = fn()
        if rc == 0:
            return rc, out, err
        if attempt < retries - 1:
            delay = RETRY_DELAYS[min(attempt, len(RETRY_DELAYS) - 1)]
            warn(f"{label} failed (attempt {attempt + 1}/{retries}): {err.strip()[:120]}")
            info(f"Retrying in {delay}s...")
            # Visible countdown
            for remaining in range(delay, 0, -1):
                _w(f"\r       {DIM}Waiting... {remaining}s{RESET}  ")
                time.sleep(1)
            _w(f"\r{ERASE_LN}")
    return rc, out, err


# =============================================================================
#  Status display (shown on main screen, like compiler tool)
# =============================================================================
def show_status():
    """Show current environment state below the banner.

    Returns True if any DCS-BIOS installation has a mismatched MTU value
    (i.e. the "Fix DCS-BIOS MTU" menu option should be shown).
    """
    cli_ver = get_cli_version()
    core_ver = get_installed_core_version(MANIFEST["esp32_core"]["platform"])
    lib_ver = get_installed_lib_version(MANIFEST["lovyangfx"]["library"])

    rec_core = MANIFEST["esp32_core"]["version"]
    rec_lib = MANIFEST["lovyangfx"]["version"]

    # CLI
    if cli_ver:
        _w(f"     \U0001f527 {CYAN}arduino-cli v{cli_ver}{RESET}  {DIM}(bundled){RESET}\n")
    else:
        _w(f"     \U0001f527 {RED}arduino-cli NOT FOUND{RESET}\n")

    # ESP32 Core
    if core_ver is None:
        _w(f"     \U0001f4e6 {RED}ESP32 Core: NOT INSTALLED{RESET}\n")
    elif version_tuple(core_ver) < version_tuple(rec_core):
        _w(f"     \U0001f4e6 {YELLOW}ESP32 Core: v{core_ver}{RESET}  {DIM}(update available: {rec_core}){RESET}\n")
    else:
        _w(f"     \U0001f4e6 {CYAN}ESP32 Core: v{core_ver}{RESET}\n")

    # LovyanGFX
    if lib_ver is None:
        _w(f"     \U0001f4da {RED}LovyanGFX: NOT INSTALLED{RESET}\n")
    elif version_tuple(lib_ver) < version_tuple(rec_lib):
        _w(f"     \U0001f4da {YELLOW}LovyanGFX: v{lib_ver}{RESET}  {DIM}(update available: {rec_lib}){RESET}\n")
    else:
        _w(f"     \U0001f4da {CYAN}LovyanGFX: v{lib_ver}{RESET}\n")

    # HID Manager deps
    missing_deps = get_missing_hid_deps()
    total_deps = len(MANIFEST["hid_manager_deps"]["packages"])
    if not missing_deps:
        _w(f"     \U0001f517 {CYAN}HID Manager deps: all installed{RESET}\n")
    elif len(missing_deps) == total_deps:
        _w(f"     \U0001f517 {RED}HID Manager deps: NOT INSTALLED{RESET}\n")
    else:
        names = ", ".join(pip for _, pip in missing_deps)
        _w(f"     \U0001f517 {YELLOW}HID Manager deps: {len(missing_deps)} missing ({names}){RESET}\n")

    # DCS-BIOS + MTU check
    mtu_needs_fix = False
    dcs_list = find_dcs_saved_games()
    if dcs_list:
        for dcs_label, dcs_path in dcs_list:
            ver = get_dcsbios_version(dcs_path)
            if ver:
                _w(f"     \U0001f3ae {CYAN}DCS-BIOS ({dcs_label}): {ver}{RESET}\n")
                # MTU check for this installation
                mtu = get_dcsbios_mtu(dcs_path)
                if mtu is not None:
                    if mtu == DCSBIOS_RECOMMENDED_MTU:
                        _w(f"     \U0001f4e1 {GREEN}DCS-BIOS MTU: {mtu}{RESET}  {DIM}(OK){RESET}\n")
                    else:
                        _w(f"     \U0001f4e1 {YELLOW}DCS-BIOS MTU: {mtu}{RESET}  {DIM}(should be {DCSBIOS_RECOMMENDED_MTU} — use Fix option below){RESET}\n")
                        mtu_needs_fix = True
            else:
                _w(f"     \U0001f3ae {YELLOW}DCS-BIOS ({dcs_label}): not installed{RESET}\n")
    else:
        dev_ver = get_dcsbios_dev_version()
        if dev_ver:
            _w(f"     \U0001f3ae {CYAN}DCS-BIOS (local): {dev_ver}{RESET}  {DIM}(Dev/DCS-BIOS-LUA){RESET}\n")
        else:
            _w(f"     \U0001f3ae {DIM}DCS-BIOS: not installed{RESET}\n")

    _w("\n")
    return mtu_needs_fix


# =============================================================================
#  Setup actions
# =============================================================================
def ensure_board_index():
    """Make sure Espressif board manager URL is configured and index is updated."""
    board_url = MANIFEST["esp32_core"]["board_manager_url"]

    rc, _, err = run_cli("config", "init", "--overwrite")
    if rc != 0:
        error(f"Failed to initialize arduino-cli config: {err.strip()}")
        return False

    rc, _, err = run_cli("config", "add", "board_manager.additional_urls", board_url)
    if rc != 0:
        error(f"Failed to add board manager URL: {err.strip()}")
        return False

    info("Updating board index...")
    rc, _, err = retry("Board index update",
                       lambda: run_cli("core", "update-index"))
    if rc != 0:
        error(f"Failed to update board index: {err.strip()}")
        return False
    success("Board index updated")
    return True


def action_setup():
    """Full setup: install missing components, upgrade outdated ones."""
    show_banner()
    total = 4
    ok = True

    # --- Step 1: Verify CLI ---
    step_banner(1, total, "Verify arduino-cli")
    if not CLI_EXE.exists():
        error("Bundled arduino-cli not found at:")
        error(f"  {CLI_EXE}")
        error("Your CockpitOS download may be incomplete.")
        return False
    cli_ver = get_cli_version()
    if not cli_ver:
        error("arduino-cli exists but failed to run.")
        return False
    success(f"arduino-cli v{cli_ver} (bundled)")

    if not ensure_board_index():
        return False

    # --- Step 2: ESP32 Core ---
    step_banner(2, total, "ESP32 Arduino Core")
    m = MANIFEST["esp32_core"]
    installed = get_installed_core_version(m["platform"])
    recommended = m["version"]

    if installed is None:
        info(f"ESP32 core is {BOLD}not installed{RESET}.")
        if confirm(f"Install {CYAN}{m['platform']}@{recommended}{RESET}?"):
            info(f"Installing {CYAN}{m['platform']}@{recommended}{RESET}...")
            info(f"{DIM}This downloads ~350 MB of toolchains. Please be patient.{RESET}")
            rc, _, err = retry("ESP32 core install",
                               lambda: run_cli("core", "install", f"{m['platform']}@{recommended}", timeout=1200))
            if rc != 0:
                error(f"Install failed: {err.strip()}")
                ok = False
            else:
                success(f"{m['platform']}@{recommended} installed")
        else:
            warn("Skipped. ESP32 core is required to compile CockpitOS.")
            ok = False
    elif version_tuple(installed) < version_tuple(recommended):
        warn(f"{m['platform']}@{installed} is older than recommended {recommended}.")
        if confirm(f"Update to {CYAN}{m['platform']}@{recommended}{RESET}?"):
            info(f"Installing {CYAN}{m['platform']}@{recommended}{RESET}...")
            rc, _, err = retry("ESP32 core update",
                               lambda: run_cli("core", "install", f"{m['platform']}@{recommended}", timeout=1200))
            if rc != 0:
                error(f"Update failed: {err.strip()}")
                ok = False
            else:
                success(f"{m['platform']}@{recommended} installed")
        else:
            info(f"Keeping {m['platform']}@{installed}.")
    else:
        success(f"{m['platform']}@{installed}")

    # --- Step 3: LovyanGFX ---
    step_banner(3, total, "LovyanGFX Library")
    m = MANIFEST["lovyangfx"]
    installed = get_installed_lib_version(m["library"])
    recommended = m["version"]

    if installed is None:
        info(f"{m['library']} is {BOLD}not installed{RESET}.")
        if confirm(f"Install {CYAN}{m['library']}@{recommended}{RESET}?"):
            info(f"Installing {CYAN}{m['library']}@{recommended}{RESET}...")
            rc, _, err = retry("LovyanGFX install",
                               lambda: run_cli("lib", "install", f"{m['library']}@{recommended}"))
            if rc != 0:
                error(f"Install failed: {err.strip()}")
                ok = False
            else:
                success(f"{m['library']}@{recommended} installed")
        else:
            warn("Skipped. LovyanGFX is required for TFT display support.")
            ok = False
    elif version_tuple(installed) < version_tuple(recommended):
        warn(f"{m['library']}@{installed} is older than recommended {recommended}.")
        if confirm(f"Update to {CYAN}{m['library']}@{recommended}{RESET}?"):
            info(f"Installing {CYAN}{m['library']}@{recommended}{RESET}...")
            rc, _, err = retry("LovyanGFX update",
                               lambda: run_cli("lib", "install", f"{m['library']}@{recommended}"))
            if rc != 0:
                error(f"Update failed: {err.strip()}")
                ok = False
            else:
                success(f"{m['library']}@{recommended} installed")
        else:
            info(f"Keeping {m['library']}@{installed}.")
    else:
        success(f"{m['library']}@{installed}")

    # --- Step 4: HID Manager Python Dependencies ---
    step_banner(4, total, "HID Manager Python Dependencies")
    missing_deps = get_missing_hid_deps()

    if not missing_deps:
        success("All HID Manager dependencies installed")
    else:
        names = ", ".join(pip for _, pip in missing_deps)
        info(f"Missing packages: {BOLD}{names}{RESET}")
        if confirm(f"Install missing HID Manager dependencies via pip?"):
            pip_names = [pip for _, pip in missing_deps]
            info(f"Installing {CYAN}{' '.join(pip_names)}{RESET}...")
            rc, _, err = retry("pip install",
                               lambda: run_pip("install", *pip_names))
            if rc != 0:
                error(f"pip install failed: {err.strip()[:200]}")
                ok = False
            else:
                still_missing = get_missing_hid_deps()
                if still_missing:
                    names2 = ", ".join(pip for _, pip in still_missing)
                    error(f"Some packages still missing after install: {names2}")
                    ok = False
                else:
                    success("All HID Manager dependencies installed")
        else:
            warn("Skipped. HID Manager requires these packages to run.")
            ok = False

    _w("\n")
    if ok:
        big_success("Setup complete!", [
            "",
            "You can now use the CockpitOS tools:",
            f"  python CockpitOS-START.py    {DIM}Compiler tool{RESET}{GREEN}",
            f"  python LabelCreator-START.py {DIM}Label creator{RESET}{GREEN}",
        ])
    else:
        big_fail("Setup finished with issues", [
            "Some components were skipped or failed.",
            "Re-run setup or install manually via Arduino IDE.",
        ])

    _w(f"  {DIM}Press any key to return to menu...{RESET}")
    msvcrt.getwch()
    return ok


def action_reset_to_manifest():
    """Reset all components to exact manifest versions."""
    show_banner()
    _w(f"  {BOLD}Reset to Recommended Versions{RESET}\n\n")
    info("This will install the exact versions from the CockpitOS manifest,")
    info("replacing whatever is currently installed:\n")

    m_core = MANIFEST["esp32_core"]
    m_lib = MANIFEST["lovyangfx"]
    cur_core = get_installed_core_version(m_core["platform"]) or "none"
    cur_lib = get_installed_lib_version(m_lib["library"]) or "none"

    missing_deps = get_missing_hid_deps()
    dep_status = f"{len(missing_deps)} missing" if missing_deps else "all installed"

    # Detect downgrades
    downgrades = []
    if cur_core != "none" and version_tuple(cur_core) > version_tuple(m_core["version"]):
        downgrades.append(f"ESP32 Core {cur_core} -> {m_core['version']}")
    if cur_lib != "none" and version_tuple(cur_lib) > version_tuple(m_lib["version"]):
        downgrades.append(f"LovyanGFX {cur_lib} -> {m_lib['version']}")

    info(f"  {CYAN}ESP32 Core{RESET}   {cur_core} {DIM}->{RESET} {BOLD}{m_core['version']}{RESET}")
    info(f"  {CYAN}LovyanGFX{RESET}    {cur_lib} {DIM}->{RESET} {BOLD}{m_lib['version']}{RESET}")
    info(f"  {CYAN}HID Deps{RESET}     {dep_status} {DIM}-> reinstall all{RESET}")
    _w("\n")

    if not confirm("Proceed with reset?"):
        info("Cancelled.")
        _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
        msvcrt.getwch()
        return

    if downgrades:
        _w("\n")
        warn(f"{YELLOW}This will DOWNGRADE the following components:{RESET}")
        for d in downgrades:
            warn(f"  {d}")
        warn("This may break other Arduino projects that depend on newer versions.")
        _w("\n")
        if not confirm(f"{RED}Are you sure you want to downgrade?{RESET}"):
            info("Cancelled.")
            _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
            msvcrt.getwch()
            return

    if not ensure_board_index():
        _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
        msvcrt.getwch()
        return

    # Core
    _w("\n")
    info(f"Installing {CYAN}{m_core['platform']}@{m_core['version']}{RESET}...")
    rc, _, err = retry("ESP32 core install",
                       lambda: run_cli("core", "install", f"{m_core['platform']}@{m_core['version']}", timeout=1200))
    if rc != 0:
        error(f"Failed: {err.strip()}")
    else:
        success(f"{m_core['platform']}@{m_core['version']} installed")

    # Library
    info(f"Installing {CYAN}{m_lib['library']}@{m_lib['version']}{RESET}...")
    rc, _, err = retry("LovyanGFX install",
                       lambda: run_cli("lib", "install", f"{m_lib['library']}@{m_lib['version']}"))
    if rc != 0:
        error(f"Failed: {err.strip()}")
    else:
        success(f"{m_lib['library']}@{m_lib['version']} installed")

    # HID Manager deps
    pip_names = list(MANIFEST["hid_manager_deps"]["packages"].values())
    info(f"Installing {CYAN}{' '.join(pip_names)}{RESET}...")
    rc, _, err = retry("pip install",
                       lambda: run_pip("install", "--force-reinstall", *pip_names))
    if rc != 0:
        error(f"pip install failed: {err.strip()[:200]}")
    else:
        success("HID Manager dependencies installed")

    _w("\n")
    big_success("Reset complete!", [
        f"  ESP32 Core: {m_core['version']}",
        f"  LovyanGFX:  {m_lib['version']}",
        f"  HID Deps:   {' '.join(pip_names)}",
    ])

    _w(f"  {DIM}Press any key to return to menu...{RESET}")
    msvcrt.getwch()


# =============================================================================
#  DCS-BIOS download action
# =============================================================================
def _fetch_json(url, timeout=30):
    """Fetch a URL and parse JSON response. Returns (data, error_string)."""
    try:
        req = urllib.request.Request(url, headers={"Accept": "application/json",
                                                   "User-Agent": "CockpitOS-Setup"})
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return json.loads(resp.read().decode()), None
    except Exception as e:
        return None, str(e)


def _download_file(url, dest_path, timeout=300):
    """Download a file with progress display. Returns True on success."""
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "CockpitOS-Setup"})
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            total = int(resp.headers.get("Content-Length", 0))
            downloaded = 0
            chunk_size = 65536

            with open(dest_path, "wb") as f:
                while True:
                    chunk = resp.read(chunk_size)
                    if not chunk:
                        break
                    f.write(chunk)
                    downloaded += len(chunk)
                    if total > 0:
                        pct = downloaded * 100 // total
                        mb = downloaded / (1024 * 1024)
                        total_mb = total / (1024 * 1024)
                        _w(f"\r       {DIM}Downloading... {mb:.1f} / {total_mb:.1f} MB ({pct}%){RESET}  ")
                    else:
                        mb = downloaded / (1024 * 1024)
                        _w(f"\r       {DIM}Downloading... {mb:.1f} MB{RESET}  ")

            _w(f"\r{ERASE_LN}")
            return True
    except Exception as e:
        _w(f"\r{ERASE_LN}")
        error(f"Download failed: {e}")
        return False


def _patch_export_lua(scripts_dir):
    """Ensure Export.lua contains the DCS-BIOS dofile line.

    - If Export.lua doesn't exist, create it with just the line.
    - If it exists but doesn't contain the line, insert it as the first line.
    - If it already contains the line, do nothing.
    - Backs up existing Export.lua to Export.lua.bak before modifying.
    """
    export_lua = scripts_dir / "Export.lua"

    if not export_lua.exists():
        export_lua.write_text(DCSBIOS_LINE + "\n", encoding="utf-8")
        success("Created Export.lua with DCS-BIOS hook")
        return True

    # Read existing content
    content = export_lua.read_text(encoding="utf-8")

    # Check if already present (flexible match)
    if "DCS-BIOS" in content and "BIOS.lua" in content:
        success("Export.lua already contains DCS-BIOS hook")
        return True

    # Backup before modifying
    backup = scripts_dir / "Export.lua.bak"
    shutil.copy2(export_lua, backup)
    info(f"Backed up Export.lua to Export.lua.bak")

    # Insert as first line
    new_content = DCSBIOS_LINE + "\n" + content
    export_lua.write_text(new_content, encoding="utf-8")
    success("Added DCS-BIOS hook to Export.lua (inserted as first line)")
    return True


def _copy_aircraft_jsons(source_json_dir):
    """Copy aircraft JSON files from DCS-BIOS doc/json/ to _core/aircraft/.

    Skips metadata/common JSONs (CommonData, MetadataStart, MetadataEnd,
    AircraftAliases, NS430, SuperCarrier, FC3) — only aircraft modules.
    Returns (copied_count, skipped_names).
    """
    SKIP = {"CommonData", "MetadataStart", "MetadataEnd",
            "AircraftAliases", "NS430", "SuperCarrier", "FC3"}

    if not source_json_dir.is_dir():
        return 0, []

    AIRCRAFT_JSON_DIR.mkdir(parents=True, exist_ok=True)

    copied = 0
    skipped = []
    for jf in sorted(source_json_dir.glob("*.json")):
        stem = jf.stem
        if stem in SKIP:
            skipped.append(stem)
            continue
        dest = AIRCRAFT_JSON_DIR / jf.name
        shutil.copy2(jf, dest)
        copied += 1

    return copied, skipped


# Names of non-aircraft JSONs that belong in _core/metadata/
_METADATA_JSONS = {"CommonData", "MetadataStart", "MetadataEnd"}


def _copy_metadata_jsons(source_json_dir):
    """Copy metadata JSON files from DCS-BIOS doc/json/ to _core/metadata/.

    These are non-aircraft JSONs (CommonData, MetadataStart, MetadataEnd)
    that every label set needs in its METADATA/ directory.
    Returns count of files copied.
    """
    if not source_json_dir.is_dir():
        return 0

    METADATA_JSON_DIR.mkdir(parents=True, exist_ok=True)

    copied = 0
    for jf in sorted(source_json_dir.glob("*.json")):
        if jf.stem in _METADATA_JSONS:
            shutil.copy2(jf, METADATA_JSON_DIR / jf.name)
            copied += 1

    return copied


def _compile_jsons_from_lua(bios_root):
    """Compile aircraft JSONs from DCS-BIOS LUA modules using bundled lua5.1.

    Invokes lua51/compile_dcsbios.lua which loads each aircraft module,
    extracts its documentation table, and writes JSON files to _core/aircraft/.

    Returns (compiled_count, failed_count, output_lines).
    """
    if not LUA51_EXE.is_file():
        error(f"lua5.1.exe not found at {LUA51_EXE}")
        return 0, 0, []
    if not LUA51_COMPILER.is_file():
        error(f"compile_dcsbios.lua not found at {LUA51_COMPILER}")
        return 0, 0, []

    bios_root = Path(bios_root)
    if not (bios_root / "BIOS.lua").exists():
        error(f"BIOS.lua not found in {bios_root}")
        return 0, 0, []

    AIRCRAFT_JSON_DIR.mkdir(parents=True, exist_ok=True)

    try:
        result = subprocess.run(
            [str(LUA51_EXE), str(LUA51_COMPILER), str(bios_root), str(AIRCRAFT_JSON_DIR)],
            capture_output=True, text=True, timeout=120,
            cwd=str(LUA51_EXE.parent),
        )
    except subprocess.TimeoutExpired:
        error("LUA compilation timed out after 120 seconds.")
        return 0, 0, []
    except OSError as e:
        error(f"Failed to run lua5.1.exe: {e}")
        return 0, 0, []

    lines = result.stdout.strip().splitlines()
    compiled = 0
    failed = 0

    for line in lines:
        if line.startswith("OK "):
            compiled += 1
        elif line.startswith("FAIL "):
            failed += 1
        elif line.startswith("DONE "):
            parts = line.split()
            if len(parts) >= 3:
                try:
                    compiled = int(parts[1])
                    failed = int(parts[2])
                except ValueError:
                    pass

    if result.returncode != 0 and result.stderr:
        error(f"Compiler stderr: {result.stderr.strip()}")

    return compiled, failed, lines


def _find_extracted_bios(extract_dir):
    """Locate the DCS-BIOS folder inside extracted ZIP contents.

    Returns Path to DCS-BIOS dir, or None.
    """
    # Direct child
    candidate = extract_dir / "DCS-BIOS"
    if candidate.is_dir():
        return candidate

    # Wrapped in a top-level folder
    for child in extract_dir.iterdir():
        if child.is_dir():
            candidate = child / "DCS-BIOS"
            if candidate.is_dir():
                return candidate

    return None


def action_fix_dcsbios_mtu():
    """Patch DCS-BIOS MAX_PAYLOAD_SIZE to match firmware UDP_MAX_SIZE (1460).

    The default DCS-BIOS value (2048) exceeds the ESP32 firmware receive buffer,
    causing IP fragmentation and data loss over WiFi. This patches
    ConnectionManager.lua to use the correct value.
    """
    show_banner()
    _w(f"\n  {BOLD}Fix DCS-BIOS MTU (MAX_PAYLOAD_SIZE){RESET}\n\n")

    dcs_list = find_dcs_saved_games()
    if not dcs_list:
        warn("No DCS World installations found.")
        _w("\n  Press any key...")
        msvcrt.getwch()
        return

    patched = 0
    for dcs_label, dcs_path in dcs_list:
        mtu = get_dcsbios_mtu(dcs_path)
        if mtu is None:
            info(f"{dcs_label}: DCS-BIOS not installed — skipping")
            continue
        if mtu == DCSBIOS_RECOMMENDED_MTU:
            success(f"{dcs_label}: already {DCSBIOS_RECOMMENDED_MTU} — OK")
            continue

        info(f"{dcs_label}: current value is {YELLOW}{mtu}{RESET}, patching to {GREEN}{DCSBIOS_RECOMMENDED_MTU}{RESET}...")
        ok, msg = patch_dcsbios_mtu(dcs_path)
        if ok:
            success(f"{dcs_label}: {msg}")
            patched += 1
        else:
            error(f"{dcs_label}: {msg}")

    if patched:
        _w(f"\n  {GREEN}{BOLD}Done!{RESET} {patched} installation(s) patched.\n")
    else:
        _w(f"\n  Nothing to patch.\n")

    _w("\n  Press any key...")
    msvcrt.getwch()


def action_download_dcsbios():
    """Download the latest DCS-BIOS release and install it.

    If DCS World is installed: installs to Saved Games/DCS/Scripts/DCS-BIOS/
    and patches Export.lua.
    If DCS World is NOT installed: installs to Dev/DCS-BIOS-LUA/ as a local copy.

    In both cases, copies the pre-compiled aircraft JSONs into
    src/LABELS/_core/aircraft/ so the Label Creator can use them.
    """
    show_banner()
    _w(f"  {BOLD}Download / Update DCS-BIOS{RESET}\n\n")

    # --- Detect DCS ---
    dcs_list = find_dcs_saved_games()
    install_to_dcs = False
    dcs_label = None
    dcs_path = None
    scripts_dir = None
    dcsbios_dir = None

    if dcs_list:
        picked = _pick_dcs_path(dcs_list)
        if picked is None:
            return
        dcs_label, dcs_path = picked
        scripts_dir = dcs_path / "Scripts"
        dcsbios_dir = scripts_dir / "DCS-BIOS"
        install_to_dcs = True

        cur_ver = get_dcsbios_version(dcs_path)
        if cur_ver:
            info(f"Current DCS-BIOS: {CYAN}{cur_ver}{RESET}")
        else:
            info(f"DCS-BIOS: {YELLOW}not installed{RESET}")
        info(f"Target: {DIM}{dcs_path}{RESET}")
    else:
        warn("DCS World not found. Will install DCS-BIOS locally for JSON access.")
        dcsbios_dir = DCSBIOS_DEV_DIR / "DCS-BIOS"
        dcs_label = "Dev/DCS-BIOS-LUA"

        dev_ver = get_dcsbios_dev_version()
        if dev_ver:
            info(f"Current DCS-BIOS (local): {CYAN}{dev_ver}{RESET}")
        else:
            info(f"DCS-BIOS (local): {YELLOW}not installed{RESET}")
        info(f"Target: {DIM}{DCSBIOS_DEV_DIR}{RESET}")

    _w("\n")

    # --- Fetch release info based on manifest strategy ---
    api_url, strategy_label = _dcsbios_release_url()
    info(f"Checking DCS-BIOS ({strategy_label})...")
    raw_data, fetch_err = _fetch_json(api_url)
    if raw_data is None:
        error(f"Failed to query GitHub releases: {fetch_err}")
        _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
        msvcrt.getwch()
        return

    # Snapshot endpoint returns an array — unwrap first element
    if isinstance(raw_data, list):
        if not raw_data:
            error("No releases found on GitHub.")
            _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
            msvcrt.getwch()
            return
        release_data = raw_data[0]
    else:
        release_data = raw_data

    tag = release_data.get("tag_name", "unknown")
    release_name = release_data.get("name", tag)
    is_prerelease = release_data.get("prerelease", False)

    # Find the DCS-BIOS ZIP asset (not Arduino_Tools)
    asset_url = None
    asset_size = 0
    for asset in release_data.get("assets", []):
        aname = asset.get("name", "")
        if aname.startswith("DCS-BIOS") and aname.endswith(".zip"):
            asset_url = asset.get("browser_download_url")
            asset_size = asset.get("size", 0)
            break

    if not asset_url:
        error(f"Could not find DCS-BIOS ZIP in the {strategy_label} release.")
        error("Release assets may have changed format.")
        _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
        msvcrt.getwch()
        return

    size_mb = asset_size / (1024 * 1024) if asset_size else 0
    pre_tag = f"  {YELLOW}(pre-release){RESET}" if is_prerelease else ""
    info(f"Release: {BOLD}{release_name}{RESET}{pre_tag}" +
         (f"  ({size_mb:.1f} MB)" if size_mb else ""))
    _w("\n")

    if not confirm(f"Download and install DCS-BIOS {CYAN}{tag}{RESET} to {dcs_label}?"):
        info("Cancelled.")
        _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
        msvcrt.getwch()
        return

    # --- Download ---
    _w("\n")
    with tempfile.TemporaryDirectory() as tmpdir:
        zip_path = Path(tmpdir) / "dcsbios.zip"

        info(f"Downloading {CYAN}{tag}{RESET}...")
        if not _download_file(asset_url, zip_path):
            _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
            msvcrt.getwch()
            return
        success("Download complete")

        # --- Extract ---
        info("Extracting...")
        try:
            extract_dir = Path(tmpdir) / "extracted"
            with zipfile.ZipFile(zip_path, "r") as zf:
                zf.extractall(extract_dir)
        except zipfile.BadZipFile:
            error("Downloaded file is not a valid ZIP.")
            _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
            msvcrt.getwch()
            return

        extracted_bios = _find_extracted_bios(extract_dir)
        if extracted_bios is None:
            error("Unexpected ZIP structure — DCS-BIOS folder not found inside.")
            _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
            msvcrt.getwch()
            return

        # --- Install DCS-BIOS ---
        if install_to_dcs:
            # Install to Saved Games/DCS/Scripts/DCS-BIOS/
            scripts_dir.mkdir(parents=True, exist_ok=True)
        else:
            # Install to Dev/DCS-BIOS-LUA/
            DCSBIOS_DEV_DIR.mkdir(parents=True, exist_ok=True)

        if dcsbios_dir.is_dir():
            existing_ver = _read_dcsbios_version_from_dir(dcsbios_dir) or "unknown"
            _w("\n")
            warn(f"Existing DCS-BIOS {CYAN}{existing_ver}{RESET} found at {DIM}{dcsbios_dir}{RESET}")
            if not confirm(f"Replace existing installation with {CYAN}{tag}{RESET}?"):
                info("Cancelled — existing DCS-BIOS was not modified.")
                _w(f"\n  {DIM}Press any key to return to menu...{RESET}")
                msvcrt.getwch()
                return
            info("Removing previous DCS-BIOS installation...")
            shutil.rmtree(dcsbios_dir)

        info("Installing DCS-BIOS...")
        shutil.copytree(extracted_bios, dcsbios_dir)

        # Write version marker
        marker = dcsbios_dir / ".dcsbios_version"
        marker.write_text(tag, encoding="utf-8")
        success(f"DCS-BIOS {tag} installed to {dcs_label}")

        # --- Patch Export.lua (DCS installs only) ---
        if install_to_dcs:
            _w("\n")
            _patch_export_lua(scripts_dir)

        # --- Aircraft + Metadata JSONs ---
        _w("\n")
        build_from_lua = MANIFEST["dcsbios"].get("build_from_lua", False)
        if build_from_lua:
            info("Compiling aircraft JSONs from LUA modules...")
            compiled, failed, _ = _compile_jsons_from_lua(dcsbios_dir)
            if compiled > 0:
                success(f"{compiled} aircraft JSONs compiled from LUA")
            if failed > 0:
                warn(f"{failed} modules failed to compile")
            if compiled == 0 and failed == 0:
                warn("No aircraft modules were compiled.")
            json_method = "compiled from LUA"
        else:
            source_json_dir = dcsbios_dir / "doc" / "json"
            if source_json_dir.is_dir():
                info("Copying pre-built aircraft JSON files...")
                count, _ = _copy_aircraft_jsons(source_json_dir)
                if count > 0:
                    success(f"{count} aircraft JSON files updated in _core/aircraft/")
                else:
                    warn("No aircraft JSON files found in release.")
            else:
                warn("DCS-BIOS doc/json/ directory not found in release.")
                warn("Aircraft JSON files were not updated.")
            json_method = "copied from release"

        # Metadata JSONs are always copied from pre-built (not compiled)
        source_json_dir = dcsbios_dir / "doc" / "json"
        meta_count = _copy_metadata_jsons(source_json_dir)
        if meta_count > 0:
            success(f"{meta_count} metadata JSON files updated in _core/metadata/")

    _w("\n")
    details = [f"  Location: {dcsbios_dir}"]
    if install_to_dcs:
        details.append(f"  Export.lua: patched")
    details.append(f"  Aircraft JSONs: {json_method}")
    big_success(f"DCS-BIOS {tag} installed!", details)

    _w(f"  {DIM}Press any key to return to menu...{RESET}")
    msvcrt.getwch()


# =============================================================================
#  Advanced — Version Pinning
# =============================================================================
def _input_text(prompt, default=""):
    """Inline text input with Backspace/Esc/Enter support.

    Displays prompt, lets user type a string. Returns the string on Enter,
    or None on Esc. Shows the default value dimmed as a hint.
    """
    _w(SHOW_CUR)
    hint = f"  {DIM}(default: {default}){RESET}" if default else ""
    _w(f"  {prompt}{hint}: ")
    buf = []
    while True:
        ch = msvcrt.getwch()
        if ch == "\r":          # Enter
            text = "".join(buf).strip()
            _w("\n")
            return text if text else default
        elif ch == "\x1b":      # Esc
            _w("\n")
            return None
        elif ch == "\x08":      # Backspace
            if buf:
                buf.pop()
                _w("\b \b")
        elif ch.isprintable() and ch != "\t":
            buf.append(ch)
            _w(ch)


def _validate_semver(text):
    """Check if text looks like a semver version string (e.g. '3.3.6', '1.2.19').

    Returns the cleaned version string or None if invalid.
    """
    # Strip leading 'v' if present
    cleaned = text.strip().lstrip("v").strip()
    if not cleaned:
        return None
    # Must be digits and dots, at least N.N
    if not re.match(r'^\d+\.\d+(\.\d+)*$', cleaned):
        return None
    return cleaned


def action_advanced_versions():
    """Advanced sub-menu for pinning component versions."""
    while True:
        show_banner()

        cur_core = MANIFEST["esp32_core"]["version"]
        cur_lib = MANIFEST["lovyangfx"]["version"]
        cur_bios = MANIFEST["dcsbios"]["release"]

        def_core = MANIFEST_DEFAULTS["esp32_core_version"]
        def_lib = MANIFEST_DEFAULTS["lovyangfx_version"]
        def_bios = MANIFEST_DEFAULTS["dcsbios_release"]

        # Show current manifest state
        _w(f"  {BOLD}Advanced — Version Pinning{RESET}\n\n")
        _w(f"     {CYAN}ESP32 Core{RESET}    {BOLD}{cur_core}{RESET}")
        if cur_core != def_core:
            _w(f"  {DIM}(default: {def_core}){RESET}")
        _w("\n")
        _w(f"     {CYAN}LovyanGFX{RESET}     {BOLD}{cur_lib}{RESET}")
        if cur_lib != def_lib:
            _w(f"  {DIM}(default: {def_lib}){RESET}")
        _w("\n")
        _w(f"     {CYAN}DCS-BIOS{RESET}      {BOLD}{cur_bios}{RESET}")
        if cur_bios != def_bios:
            _w(f"  {DIM}(default: {def_bios}){RESET}")
        _w("\n")

        choice = pick_action("Select component to configure:", [
            ("ESP32 Core version",    "core"),
            ("LovyanGFX version",     "lib"),
            ("DCS-BIOS release",      "bios"),
            ("",),
            ("Reset all to defaults", "defaults", "dim"),
            ("",),
            ("Back",                  "back",     "dim"),
        ])

        if choice in ("back", None):
            return

        if choice == "defaults":
            MANIFEST["esp32_core"]["version"] = def_core
            MANIFEST["lovyangfx"]["version"] = def_lib
            MANIFEST["dcsbios"]["release"] = def_bios
            show_banner()
            success("All versions reset to defaults")
            _w(f"\n  {DIM}Press any key...{RESET}")
            msvcrt.getwch()
            continue

        if choice == "core":
            _edit_core_version()
        elif choice == "lib":
            _edit_lib_version()
        elif choice == "bios":
            _edit_bios_release()


def _edit_core_version():
    """Sub-menu for ESP32 Core version."""
    cur = MANIFEST["esp32_core"]["version"]
    default = MANIFEST_DEFAULTS["esp32_core_version"]

    show_banner()
    _w(f"  {BOLD}ESP32 Core Version{RESET}\n\n")
    _w(f"     Current: {BOLD}{cur}{RESET}\n")
    _w(f"     Default: {DIM}{default}{RESET}\n")
    _w(f"     Format:  {DIM}X.Y.Z  (e.g. 3.3.6, 3.1.1){RESET}\n")

    action = pick_action("Choose version strategy:", [
        (f"Keep current ({cur})",   "keep"),
        (f"Reset to default ({default})", "default"),
        ("Enter a specific version",  "custom"),
    ])

    if action in ("keep", None):
        return

    if action == "default":
        MANIFEST["esp32_core"]["version"] = default
        success(f"ESP32 Core version set to {BOLD}{default}{RESET}")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return

    # Custom version input
    _w("\n")
    while True:
        text = _input_text("Enter ESP32 Core version", default=cur)
        if text is None:
            return
        ver = _validate_semver(text)
        if ver:
            MANIFEST["esp32_core"]["version"] = ver
            success(f"ESP32 Core version set to {BOLD}{ver}{RESET}")
            _w(f"\n  {DIM}Press any key...{RESET}")
            msvcrt.getwch()
            return
        else:
            error(f"Invalid version format: \"{text}\"")
            info(f"Expected format: {DIM}X.Y.Z{RESET}  (e.g. {default})")
            _w("\n")


def _edit_lib_version():
    """Sub-menu for LovyanGFX version."""
    cur = MANIFEST["lovyangfx"]["version"]
    default = MANIFEST_DEFAULTS["lovyangfx_version"]

    show_banner()
    _w(f"  {BOLD}LovyanGFX Version{RESET}\n\n")
    _w(f"     Current: {BOLD}{cur}{RESET}\n")
    _w(f"     Default: {DIM}{default}{RESET}\n")
    _w(f"     Format:  {DIM}X.Y.Z  (e.g. 1.2.19, 1.1.16){RESET}\n")

    action = pick_action("Choose version strategy:", [
        (f"Keep current ({cur})",   "keep"),
        (f"Reset to default ({default})", "default"),
        ("Enter a specific version",  "custom"),
    ])

    if action in ("keep", None):
        return

    if action == "default":
        MANIFEST["lovyangfx"]["version"] = default
        success(f"LovyanGFX version set to {BOLD}{default}{RESET}")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return

    # Custom version input
    _w("\n")
    while True:
        text = _input_text("Enter LovyanGFX version", default=cur)
        if text is None:
            return
        ver = _validate_semver(text)
        if ver:
            MANIFEST["lovyangfx"]["version"] = ver
            success(f"LovyanGFX version set to {BOLD}{ver}{RESET}")
            _w(f"\n  {DIM}Press any key...{RESET}")
            msvcrt.getwch()
            return
        else:
            error(f"Invalid version format: \"{text}\"")
            info(f"Expected format: {DIM}X.Y.Z{RESET}  (e.g. {default})")
            _w("\n")


def _edit_bios_release():
    """Sub-menu for DCS-BIOS release strategy, build toggle, and wipe."""
    while True:
        cur = MANIFEST["dcsbios"]["release"]
        default = MANIFEST_DEFAULTS["dcsbios_release"]
        build = MANIFEST["dcsbios"].get("build_from_lua", False)

        show_banner()
        _w(f"  {BOLD}DCS-BIOS Settings{RESET}\n\n")
        _w(f"     {CYAN}Release{RESET}          {BOLD}{cur}{RESET}")
        if cur != default:
            _w(f"  {DIM}(default: {default}){RESET}")
        _w("\n")
        build_label = f"{GREEN}ON{RESET}" if build else f"{DIM}OFF{RESET}"
        _w(f"     {CYAN}Build from LUA{RESET}   {build_label}\n")

        action = pick_action("DCS-BIOS options:", [
            ("Change release strategy",            "release"),
            (f"Toggle build JSONs from LUA ({('ON' if build else 'OFF')})",
                                                   "toggle_build"),
            ("Build JSON from LUA now",            "build_now"),
            ("Re-import aircraft JSONs (pre-built)", "reimport"),
            ("",),
            ("Wipe DCS-BIOS + aircraft JSONs",     "wipe",  "dim"),
            ("",),
            ("Back",                               "back",  "dim"),
        ])

        if action in ("back", None):
            return

        if action == "release":
            _edit_bios_release_strategy()
        elif action == "toggle_build":
            _toggle_build_from_lua()
        elif action == "build_now":
            _build_jsons_from_lua_now()
        elif action == "reimport":
            _reimport_aircraft_jsons()
        elif action == "wipe":
            _wipe_dcsbios()


def _edit_bios_release_strategy():
    """Pick release strategy: latest, snapshot, or pinned version."""
    cur = MANIFEST["dcsbios"]["release"]

    show_banner()
    _w(f"  {BOLD}DCS-BIOS Release Strategy{RESET}\n\n")
    _w(f"     Current: {BOLD}{cur}{RESET}\n")

    action = pick_action("Choose release strategy:", [
        ("Latest Official Release",          "latest"),
        ("Development / Test Release", "snapshot"),
        ("Pin to a specific version",      "custom"),
    ])

    if action is None:
        return

    if action == "latest":
        MANIFEST["dcsbios"]["release"] = "latest"
        success(f"DCS-BIOS release set to {BOLD}Latest Official Release{RESET}")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return

    if action == "snapshot":
        MANIFEST["dcsbios"]["release"] = "snapshot"
        success(f"DCS-BIOS release set to {BOLD}Development / Test Release{RESET}")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return

    # Custom version tag input
    _w("\n")
    _w(f"     {DIM}Enter a release tag as it appears on GitHub{RESET}\n")
    _w(f"     {DIM}Format: v0.8.4 or 0.8.4 (the 'v' prefix is optional){RESET}\n\n")
    while True:
        text = _input_text("Enter DCS-BIOS version tag", default=cur if cur not in ("latest", "snapshot") else "")
        if text is None:
            return
        text = text.strip()
        if not text:
            error("Version tag cannot be empty.")
            _w("\n")
            continue
        cleaned = text.lstrip("v").strip()
        if not re.match(r'^[\d][\d.]*\d$', cleaned):
            error(f"Invalid version tag: \"{text}\"")
            info(f"Expected format: {DIM}v0.8.4{RESET} or {DIM}0.8.4{RESET}")
            _w("\n")
            continue
        tag = f"v{cleaned}"
        MANIFEST["dcsbios"]["release"] = tag
        success(f"DCS-BIOS release pinned to {BOLD}{tag}{RESET}")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return


def _find_all_dcsbios_dirs():
    """Return list of (label, bios_dir) for all DCS-BIOS installations.

    Checks DCS Saved Games paths and the local Dev fallback.
    """
    dirs = []
    for dcs_label, dcs_path in find_dcs_saved_games():
        bios_dir = dcs_path / "Scripts" / "DCS-BIOS"
        if bios_dir.is_dir() and (bios_dir / "BIOS.lua").exists():
            dirs.append((dcs_label, bios_dir))
    dev_bios = DCSBIOS_DEV_DIR / "DCS-BIOS"
    if dev_bios.is_dir() and (dev_bios / "BIOS.lua").exists():
        dirs.append(("Dev/DCS-BIOS-LUA", dev_bios))
    return dirs


def _toggle_build_from_lua():
    """Toggle the 'build JSONs from LUA' setting.

    When ON: aircraft JSONs are compiled from DCS-BIOS LUA modules using the
    bundled lua5.1 interpreter. This happens during download and reimport.

    When OFF: pre-compiled JSONs from the DCS-BIOS release ZIP are used.
    """
    build = MANIFEST["dcsbios"].get("build_from_lua", False)
    new_state = not build
    MANIFEST["dcsbios"]["build_from_lua"] = new_state

    # No pause needed — _edit_bios_release() loop redraws immediately


def _pick_bios_dir():
    """Pick a DCS-BIOS installation. Returns (label, bios_dir) or None."""
    bios_dirs = _find_all_dcsbios_dirs()
    if not bios_dirs:
        error("No DCS-BIOS installation found.")
        info("Install DCS-BIOS first from the main menu.")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return None

    if len(bios_dirs) == 1:
        return bios_dirs[0]

    options = [(lbl, str(bd)) for lbl, bd in bios_dirs]
    choice = pick_action("Which DCS-BIOS installation?", options)
    if choice is None:
        return None
    for lbl, bd in bios_dirs:
        if str(bd) == choice:
            return (lbl, bd)
    return None


def _build_jsons_from_lua_now():
    """Compile aircraft JSONs from DCS-BIOS LUA modules right now."""
    show_banner()
    _w(f"  {BOLD}Build Aircraft JSONs from LUA{RESET}\n\n")

    picked = _pick_bios_dir()
    if picked is None:
        return
    label, bios_dir = picked

    info(f"Source: {DIM}{bios_dir}{RESET}")
    _w("\n")

    if not confirm("Compile aircraft JSONs from LUA modules?"):
        return

    _w("\n")
    info("Compiling aircraft JSONs from LUA modules...")
    compiled, failed, _ = _compile_jsons_from_lua(bios_dir)
    if compiled > 0:
        success(f"{compiled} aircraft JSONs compiled from LUA")
    if failed > 0:
        warn(f"{failed} modules failed to compile")
    if compiled == 0 and failed == 0:
        warn("No aircraft modules were compiled.")

    _w(f"\n  {DIM}Press any key...{RESET}")
    msvcrt.getwch()


def _reimport_aircraft_jsons():
    """Re-copy pre-built aircraft JSONs from an existing DCS-BIOS installation."""
    show_banner()
    _w(f"  {BOLD}Re-import Aircraft JSONs{RESET}\n\n")

    picked = _pick_bios_dir()
    if picked is None:
        return
    label, bios_dir = picked

    json_dir = bios_dir / "doc" / "json"
    if not json_dir.is_dir():
        error("No doc/json/ directory found in this DCS-BIOS install.")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return

    count = sum(1 for f in json_dir.glob("*.json"))
    info(f"Source: {DIM}{json_dir}{RESET}  ({count} JSON files)")
    _w("\n")

    if not confirm(f"Re-import {count} JSON files into _core/aircraft/?"):
        return

    _w("\n")
    copied, _ = _copy_aircraft_jsons(json_dir)
    if copied > 0:
        success(f"{copied} aircraft JSON files imported into _core/aircraft/")
    else:
        warn("No aircraft JSON files were copied.")

    meta_count = _copy_metadata_jsons(json_dir)
    if meta_count > 0:
        success(f"{meta_count} metadata JSON files imported into _core/metadata/")

    _w(f"\n  {DIM}Press any key...{RESET}")
    msvcrt.getwch()


def _wipe_dcsbios():
    """Remove DCS-BIOS installation and all cached JSONs from _core/."""
    show_banner()
    _w(f"  {BOLD}Wipe DCS-BIOS + Cached JSONs{RESET}\n\n")

    # Gather what exists
    bios_dirs = _find_all_dcsbios_dirs()
    json_files = list(AIRCRAFT_JSON_DIR.glob("*.json")) if AIRCRAFT_JSON_DIR.is_dir() else []
    meta_files = list(METADATA_JSON_DIR.glob("*.json")) if METADATA_JSON_DIR.is_dir() else []

    if not bios_dirs and not json_files and not meta_files:
        info("Nothing to wipe — no DCS-BIOS installation or cached JSONs found.")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return

    # Show what will be deleted
    if bios_dirs:
        _w(f"  {BOLD}DCS-BIOS installations:{RESET}\n")
        for label, bios_dir in bios_dirs:
            ver = _read_dcsbios_version_from_dir(bios_dir)
            ver_str = f" ({ver})" if ver else ""
            _w(f"     {RED}{label}{ver_str}{RESET}  {DIM}{bios_dir}{RESET}\n")
        _w("\n")

    if json_files:
        _w(f"  {BOLD}Aircraft JSONs:{RESET}\n")
        _w(f"     {RED}{len(json_files)} files{RESET} in {DIM}{AIRCRAFT_JSON_DIR}{RESET}\n")
        _w("\n")

    if meta_files:
        _w(f"  {BOLD}Metadata JSONs:{RESET}\n")
        _w(f"     {RED}{len(meta_files)} files{RESET} in {DIM}{METADATA_JSON_DIR}{RESET}\n")
        _w("\n")

    warn("This will permanently delete the above files.")
    _w("\n")
    if not confirm(f"{RED}Are you sure you want to wipe everything?{RESET}"):
        info("Cancelled.")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return

    _w("\n")

    # Wipe DCS-BIOS installations
    for label, bios_dir in bios_dirs:
        try:
            shutil.rmtree(bios_dir)
            success(f"Removed DCS-BIOS from {label}")
        except OSError as e:
            error(f"Failed to remove {label}: {e}")

    # Wipe aircraft JSONs
    if json_files:
        removed = 0
        for jf in json_files:
            try:
                jf.unlink()
                removed += 1
            except OSError:
                pass
        if removed > 0:
            success(f"Removed {removed} aircraft JSON files from _core/aircraft/")
        else:
            error("Failed to remove aircraft JSON files.")

    # Wipe metadata JSONs
    if meta_files:
        removed = 0
        for jf in meta_files:
            try:
                jf.unlink()
                removed += 1
            except OSError:
                pass
        if removed > 0:
            success(f"Removed {removed} metadata JSON files from _core/metadata/")

    # Reset build toggle
    MANIFEST["dcsbios"]["build_from_lua"] = False

    _w("\n")
    big_success("DCS-BIOS wiped", [
        "Re-download DCS-BIOS from the main menu when ready.",
    ])
    _w(f"  {DIM}Press any key...{RESET}")
    msvcrt.getwch()


# =============================================================================
#  Single-instance guard
# =============================================================================
_WINDOW_TITLE = "CockpitOS Environment Setup"
_lock_path = None          # set in main(), cleaned up before exec


def _bring_existing_to_front():
    """Find an existing Setup window and bring it to the foreground."""
    user32 = ctypes.windll.user32
    hwnd = user32.FindWindowW(None, _WINDOW_TITLE)
    if hwnd:
        SW_RESTORE = 9
        user32.ShowWindow(hwnd, SW_RESTORE)
        user32.SetForegroundWindow(hwnd)
        return True
    return False


# =============================================================================
#  Tool switcher — launch sibling CockpitOS tools
# =============================================================================
def _launch_tool(script_name):
    """Replace the current process with a sibling CockpitOS tool.

    Uses os.execl so there is no nesting — the old tool is gone,
    the new one takes over the same console window.
    """
    script = SCRIPT_DIR / script_name
    if not script.exists():
        error(f"{script_name} not found.")
        return
    # Clean up our lock file before we disappear
    if _lock_path:
        _lock_path.unlink(missing_ok=True)
    cls()
    os.execl(sys.executable, sys.executable, str(script))


# =============================================================================
#  Main menu loop
# =============================================================================
def main():
    import atexit

    # --- Single-instance guard (lock file) ---
    global _lock_path
    lock_path = SCRIPT_DIR / ".setup.lock"
    _lock_path = lock_path

    def _is_python_process(pid):
        """Check if a PID belongs to a running Python process."""
        try:
            kernel32 = ctypes.windll.kernel32
            psapi = ctypes.windll.psapi
            handle = kernel32.OpenProcess(0x0400 | 0x0010, False, pid)
            if not handle:
                return False
            try:
                buf = ctypes.create_unicode_buffer(260)
                if psapi.GetModuleFileNameExW(handle, None, buf, 260):
                    return "python" in buf.value.lower()
                return False
            finally:
                kernel32.CloseHandle(handle)
        except Exception:
            return False

    try:
        lock_fd = os.open(str(lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, str(os.getpid()).encode())
        os.close(lock_fd)
    except FileExistsError:
        still_running = False
        try:
            old_pid = int(lock_path.read_text().strip())
            if _is_python_process(old_pid):
                if _bring_existing_to_front():
                    still_running = True
        except (ValueError, OSError):
            pass

        if still_running:
            print("\n  CockpitOS Setup is already running. Switching to existing window.")
            sys.exit(0)

        # Stale lock — reclaim it
        lock_path.unlink(missing_ok=True)
        lock_fd = os.open(str(lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, str(os.getpid()).encode())
        os.close(lock_fd)

    atexit.register(lambda: lock_path.unlink(missing_ok=True))

    # Set window title so other instances can find us
    ctypes.windll.kernel32.SetConsoleTitleW(_WINDOW_TITLE)

    while True:
        show_banner()
        mtu_needs_fix = show_status()

        menu_options = [
            ("Setup / Update environment",         "setup"),
            ("Reset to recommended versions",      "reset"),
            ("Download / Update DCS-BIOS",         "dcsbios"),
        ]

        if mtu_needs_fix:
            menu_options.append(
                (f"{YELLOW}Fix DCS-BIOS MTU{RESET}",  "fix_mtu"),
            )

        menu_options += [
            ("Advanced — Version Pinning",         "advanced",  "dim"),
            ("",),
            ("---", "Switch Tool"),
            ("Label Creator",                      "label"),
            ("Compile Tool",                       "compiler"),
            ("",),
            ("Exit",                               "exit",      "dim"),
        ]

        choice = pick_action("What would you like to do?", menu_options)

        if choice == "setup":
            action_setup()
        elif choice == "reset":
            action_reset_to_manifest()
        elif choice == "dcsbios":
            action_download_dcsbios()
        elif choice == "fix_mtu":
            action_fix_dcsbios_mtu()
        elif choice == "advanced":
            action_advanced_versions()
        elif choice == "compiler":
            _launch_tool("CockpitOS-START.py")
        elif choice == "label":
            _launch_tool("LabelCreator-START.py")
        elif choice in ("exit", None):
            cls()
            break


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        _w(f"{SHOW_CUR}\n\n  {YELLOW}Interrupted.{RESET}\n")
        sys.exit(0)
