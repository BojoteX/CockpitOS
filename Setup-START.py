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
    """Show current environment state below the banner."""
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

    _w("\n")


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
    cls()
    os.execl(sys.executable, sys.executable, str(script))


# =============================================================================
#  Main menu loop
# =============================================================================
def main():
    ctypes.windll.kernel32.SetConsoleTitleW("CockpitOS Environment Setup")

    while True:
        show_banner()
        show_status()

        choice = pick_action("What would you like to do?", [
            ("Setup / Update environment",         "setup"),
            ("Reset to recommended versions",      "reset"),
            ("",),
            ("---", "Switch Tool"),
            ("Label Creator",                      "label"),
            ("Compile Tool",                       "compiler"),
            ("",),
            ("Exit",                               "exit",      "dim"),
        ])

        if choice == "setup":
            action_setup()
        elif choice == "reset":
            action_reset_to_manifest()
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
