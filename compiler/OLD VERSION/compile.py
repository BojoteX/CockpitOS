#!/usr/bin/env python3
"""
CockpitOS Compiler - Interactive build helper for Arduino CLI
Wraps arduino-cli to compile CockpitOS with full board/option control.
Handles label set selection, generate_data.py, Config.h transport/role, and compilation.
"""

import subprocess
import sys
import os
import json
import time
import shutil
import re
import threading
import msvcrt
import ctypes
from pathlib import Path

# -----------------------------------------------------------------------------
# Paths (adjust if your installation differs)
# -----------------------------------------------------------------------------
ARDUINO_CLI = Path(r"C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe")
SKETCH_DIR  = Path(__file__).resolve().parent.parent          # CockpitOS root
_ARDUINO_DATA = Path(os.environ.get("LOCALAPPDATA", "")) / "Arduino15" / "CockpitOS"
BUILD_DIR   = _ARDUINO_DATA / "build"                         # Build output (outside OneDrive)
CACHE_DIR   = _ARDUINO_DATA / "cache"                         # Compilation cache (outside OneDrive)
PREFS_FILE  = SKETCH_DIR / "compiler" / "compiler_prefs.json" # Saved preferences
CONFIG_H    = SKETCH_DIR / "Config.h"                         # CockpitOS Config.h
LABELS_DIR  = SKETCH_DIR / "src" / "LABELS"                   # Label sets root
ACTIVE_SET  = LABELS_DIR / "active_set.h"                     # Currently active label set
BACKUP_DIR  = SKETCH_DIR / "compiler" / "backups"             # Timestamped backups
OUTPUT_DIR  = SKETCH_DIR / "compiler" / "output"              # Final .bin files for the user

# -----------------------------------------------------------------------------
# File Safety — whitelist, backup, and atomic write
# -----------------------------------------------------------------------------
# Only files in this set can be modified by the tool. Any attempt to write
# outside the whitelist is blocked. Add new entries here if future features
# need to modify additional files (e.g., label set wizard).
ALLOWED_FILES = {
    "Config.h":  SKETCH_DIR / "Config.h",
}

MAX_BACKUPS = 10                     # Keep last N backups, prune older ones
_session_backed_up = set()           # Tracks which files have been backed up THIS session

# -----------------------------------------------------------------------------
# Tracked Variable Registry — every Config.h #define the tool knows about
# -----------------------------------------------------------------------------
# Config.h is the source of truth. On startup, all tracked defines are read
# into an in-memory snapshot. The tool reads from the snapshot and writes
# through save_config_value() which updates both Config.h and the snapshot.
#
# Format: { "DEFINE_NAME": default_value }
# Default values are used ONLY when a define is missing from Config.h (e.g.,
# user has an older Config.h). They match what Config.h ships with.
TRACKED_DEFINES = {
    # Transport (exactly one must be 1, rest 0)
    "USE_DCSBIOS_SERIAL":               "0",
    "USE_DCSBIOS_USB":                  "0",
    "USE_DCSBIOS_WIFI":                 "1",
    "USE_DCSBIOS_BLUETOOTH":            "0",
    "RS485_SLAVE_ENABLED":              "0",
    # Role
    "RS485_MASTER_ENABLED":             "0",
    # RS485 Slave
    "RS485_SLAVE_ADDRESS":              "1",
    # RS485 Master
    "RS485_SMART_MODE":                 "0",
    "RS485_MAX_SLAVE_ADDRESS":          "127",
    # RS485 Task config
    "RS485_USE_TASK":                   "1",
    "RS485_TASK_CORE":                  "1",
    "RS485_USE_ISR_MODE":               "1",
    # RS485 Pins
    "RS485_TX_PIN":                     "17",
    "RS485_RX_PIN":                     "18",
    "RS485_DE_PIN":                     "-1",
    # Debug
    "DEBUG_ENABLED":                    "0",
    "VERBOSE_MODE":                     "0",
}

# Transport-related subsets (derived from the registry)
TRANSPORT_DEFINES = ["USE_DCSBIOS_SERIAL", "USE_DCSBIOS_USB", "USE_DCSBIOS_WIFI", "USE_DCSBIOS_BLUETOOTH", "RS485_SLAVE_ENABLED"]

TRANSPORT_KEY = {
    "serial": "USE_DCSBIOS_SERIAL",
    "usb":    "USE_DCSBIOS_USB",
    "wifi":   "USE_DCSBIOS_WIFI",
    "ble":    "USE_DCSBIOS_BLUETOOTH",
    "slave":  "RS485_SLAVE_ENABLED",
}

# In-memory snapshot — populated by load_config_snapshot() on startup
_config_snapshot = {}


def _is_allowed_file(path):
    """Check if a file path is in the ALLOWED_FILES whitelist."""
    resolved = Path(path).resolve()
    for allowed in ALLOWED_FILES.values():
        if resolved == allowed.resolve():
            return True
    return False


def _prune_backups(filename_prefix):
    """Keep only the newest MAX_BACKUPS backups for a given file prefix."""
    if not BACKUP_DIR.exists():
        return
    # Collect all backups matching this file prefix
    backups = sorted(
        [f for f in BACKUP_DIR.iterdir() if f.name.startswith(filename_prefix) and f.suffix == ".bak"],
        key=lambda f: f.stat().st_mtime,
        reverse=True,
    )
    # Remove oldest beyond the limit
    for old in backups[MAX_BACKUPS:]:
        try:
            old.unlink()
        except OSError:
            pass


def backup_file(path):
    """Create a timestamped backup of a file. Only backs up once per session.

    Returns the backup path on success, None if skipped or failed.
    Skips silently if this file was already backed up in this session
    (preserves the "before tool touched it" state).
    """
    resolved = Path(path).resolve()

    if not _is_allowed_file(resolved):
        return None

    if not resolved.exists():
        return None

    # Once-per-session guard: preserve the state BEFORE the tool touched anything
    if str(resolved) in _session_backed_up:
        return None

    BACKUP_DIR.mkdir(parents=True, exist_ok=True)

    timestamp = time.strftime("%Y%m%d_%H%M%S")
    backup_name = f"{resolved.name}.{timestamp}.bak"
    backup_path = BACKUP_DIR / backup_name

    try:
        shutil.copy2(resolved, backup_path)
        _session_backed_up.add(str(resolved))
        _prune_backups(f"{resolved.name}.")
        return backup_path
    except OSError:
        return None


def safe_write_file(path, content):
    """Atomic write: write to .tmp, then rename over the target.

    On Windows, os.replace() is atomic within the same volume.
    If the rename fails, the .tmp file is left for manual recovery.

    Returns True on success, False on failure.
    """
    resolved = Path(path).resolve()

    if not _is_allowed_file(resolved):
        # Refuse to write files not in the whitelist
        return False

    tmp_path = resolved.with_suffix(resolved.suffix + ".tmp")

    try:
        tmp_path.write_text(content, encoding="utf-8")
    except OSError:
        return False

    try:
        os.replace(str(tmp_path), str(resolved))
        return True
    except OSError:
        # .tmp written but rename failed — leave .tmp for manual recovery.
        # Original Config.h is untouched (this is the safety guarantee).
        return False


def list_backups(filename="Config.h"):
    """List available backups for a file, newest first.

    Returns list of (backup_path, timestamp_str, size_bytes).
    """
    if not BACKUP_DIR.exists():
        return []

    prefix = f"{filename}."
    backups = []
    for f in sorted(BACKUP_DIR.iterdir(), key=lambda f: f.stat().st_mtime, reverse=True):
        if f.name.startswith(prefix) and f.suffix == ".bak":
            # Extract timestamp from name: Config.h.20250211_143022.bak
            parts = f.stem  # Config.h.20250211_143022
            ts_part = parts.split(".")[-1]  # 20250211_143022
            try:
                # Format nicely: 2025-02-11 14:30:22
                ts_display = f"{ts_part[:4]}-{ts_part[4:6]}-{ts_part[6:8]} {ts_part[9:11]}:{ts_part[11:13]}:{ts_part[13:15]}"
            except (IndexError, ValueError):
                ts_display = ts_part
            backups.append((f, ts_display, f.stat().st_size))

    return backups


def diff_backup_vs_current(backup_path, current_path):
    """Compare a backup to the current file. Returns list of changed #define names."""
    try:
        backup_text = Path(backup_path).read_text(encoding="utf-8")
        current_text = Path(current_path).read_text(encoding="utf-8")
    except OSError:
        return []

    define_re = re.compile(r'#define\s+(\S+)\s+(\S+)')

    backup_defines = {m.group(1): m.group(2) for m in define_re.finditer(backup_text)}
    current_defines = {m.group(1): m.group(2) for m in define_re.finditer(current_text)}

    changes = []
    all_keys = set(backup_defines.keys()) | set(current_defines.keys())
    for key in sorted(all_keys):
        old = backup_defines.get(key)
        new = current_defines.get(key)
        if old != new:
            changes.append((key, old, new))

    return changes


# -----------------------------------------------------------------------------
# Config snapshot — load all tracked defines from Config.h into memory
# -----------------------------------------------------------------------------
def load_config_snapshot():
    """Read all TRACKED_DEFINES from Config.h into the in-memory snapshot.

    Called once on startup. Config.h is the source of truth.
    If a tracked define is missing from Config.h, uses the default
    from TRACKED_DEFINES and warns the user.
    """
    global _config_snapshot
    _config_snapshot = {}
    missing = []

    for name, default in TRACKED_DEFINES.items():
        val = read_config_define(name)
        if val is not None:
            _config_snapshot[name] = val
        else:
            _config_snapshot[name] = default
            missing.append(name)

    return missing


def config_get(name):
    """Read a tracked define from the in-memory snapshot.

    Falls back to reading Config.h directly if snapshot not loaded.
    """
    if _config_snapshot and name in _config_snapshot:
        return _config_snapshot[name]
    # Fallback: read directly (shouldn't happen after startup)
    return read_config_define(name)


def config_set(name, value):
    """Write a tracked define to Config.h AND update the snapshot.

    Returns True on success, False on failure.
    """
    value = str(value)
    if write_config_define(name, value):
        _config_snapshot[name] = value
        return True
    return False


# CockpitOS hard defaults for board options (always applied)
COCKPITOS_DEFAULTS = {
    "CDCOnBoot":       "default",   # Disabled - REQUIRED
    "PartitionScheme": "no_ota",    # No OTA (2MB APP/2MB SPIFFS)
    "CPUFreq":         "240",       # 240MHz
    "FlashMode":       "qio",       # QIO 80MHz
    "UploadSpeed":     "921600",    # Fastest upload
    "UploadMode":      "default",   # UART0 / Hardware CDC
    "DebugLevel":      "none",      # No core debug
    "LoopCore":        "1",         # Arduino on Core 1
    "EventsCore":      "1",         # Events on Core 1
    "EraseFlash":      "none",      # Don't erase
    "MSCOnBoot":       "default",   # MSC disabled
    "DFUOnBoot":       "default",   # DFU disabled
}

# -----------------------------------------------------------------------------
# Board Intelligence — dual-USB detection and cross-validation
# -----------------------------------------------------------------------------
# Some ESP32 variants (S3, P4) have BOTH USB-OTG (TinyUSB) and Hardware CDC.
# The correct USB mode depends entirely on the transport selection:
#   USB transport   → USB-OTG (TinyUSB) is REQUIRED
#   Anything else   → HW CDC is preferred (lighter, allows Serial debug)
#
# Boards without dual USB simply don't have a USBMode option — no action needed.

def board_has_dual_usb(prefs):
    """Check if the selected board has multiple USB modes (S3, P4, etc.).

    Detects dynamically from arduino-cli board details — no hardcoded
    chip names. If the board has a "USBMode" option with 2+ values,
    it's a dual-USB board.

    Returns True/False. Returns False if no board is configured.
    """
    fqbn = prefs.get("fqbn")
    if not fqbn:
        return False
    board_options = get_board_options(fqbn)
    usb_opt = board_options.get("USBMode")
    if usb_opt and len(usb_opt.get("values", [])) >= 2:
        return True
    return False


def preferred_usb_mode(transport):
    """Return the ideal USBMode value for a given transport.

    USB transport → "default"  (USB-OTG / TinyUSB)
    Everything else → "hwcdc"  (Hardware CDC — lighter, Serial debug works)

    These are the arduino-cli option values, not display labels.
    """
    if transport == "usb":
        return "default"      # USB-OTG (TinyUSB)
    return "hwcdc"            # Hardware CDC


# Cross-validation: Config.h settings vs. board options
# Returns a list of (level, message) tuples:
#   level = "FATAL"   → must block compile
#   level = "WARNING" → inform user, allow override
FATAL   = "FATAL"
WARNING = "WARNING"


def validate_config_vs_board(prefs):
    """Cross-validate Config.h settings against board options.

    Checks for dangerous mismatches between the two configuration
    domains (firmware defines and arduino-cli board options).

    Returns: list of (level, message) tuples.
      level is FATAL or WARNING.
      Empty list = all good.
    """
    issues = []
    opts = prefs.get("options", {})
    transport = read_current_transport()
    role = read_current_role()
    dual_usb = board_has_dual_usb(prefs)

    # --- FATAL: CDC on Boot must be disabled ---
    cdc = opts.get("CDCOnBoot")
    if cdc and cdc != "default":
        issues.append((FATAL,
            "CDC On Boot is ENABLED. CockpitOS requires CDC On Boot = Disabled.\n"
            "  Config.h will fail to compile (#error directive). "
            "Change it in Board Options."))

    # --- FATAL: USB transport + HW CDC on dual-USB board ---
    if dual_usb and transport == "usb":
        usb_mode = opts.get("USBMode", "")
        if usb_mode == "hwcdc":
            issues.append((FATAL,
                "USB transport requires USB-OTG (TinyUSB), but USB Mode is set to HW CDC.\n"
                "  The firmware will not enumerate as a USB device. "
                "Change USB Mode in Board Options."))

    # --- WARNING: non-USB transport + USB-OTG on dual-USB board ---
    if dual_usb and transport != "usb":
        usb_mode = opts.get("USBMode", "")
        if usb_mode == "default":   # "default" = USB-OTG (TinyUSB)
            transport_name = transport_label(transport) if transport else "your transport"
            issues.append((WARNING,
                f"USB Mode is set to USB-OTG (TinyUSB), but transport is {transport_name}.\n"
                "  HW CDC is lighter and allows Serial debugging when not using USB transport.\n"
                "  Consider switching USB Mode to HW CDC in Board Options."))

    return issues


# -----------------------------------------------------------------------------
# Transport / Role definitions for Config.h
#
# The assert in Config.h is:
#   (BLE + WiFi + USB + Serial + RS485_SLAVE) == 1
#
# RS485 Slave IS a transport — selecting it zeroes out the other four.
# RS485 Master is NOT a transport — it's a role that requires one of the
# four real transports (Serial, USB, WiFi, BLE) to forward data.
# Standalone just picks a transport directly.
# -----------------------------------------------------------------------------
TRANSPORTS = [
    ("WiFi",                      "wifi"),
    ("USB Native",                "usb"),
    ("Serial (CDC/Socat)",        "serial"),
    ("RS485 Slave",               "slave"),
    ("Bluetooth BLE",             "ble"),
]

# TRANSPORT_DEFINES and TRANSPORT_KEY are now in the Tracked Variable
# Registry section above (TRANSPORT_DEFINES, TRANSPORT_KEY).

# -----------------------------------------------------------------------------
# ANSI helpers & terminal setup
# -----------------------------------------------------------------------------
CYAN    = "\033[96m"
GREEN   = "\033[92m"
YELLOW  = "\033[93m"
RED     = "\033[91m"
BOLD    = "\033[1m"
DIM     = "\033[2m"
RESET   = "\033[0m"
REV     = "\033[7m"          # reverse video for highlighted row
HIDE_CUR = "\033[?25l"
SHOW_CUR = "\033[?25h"
ERASE_LN = "\033[2K"

def _enable_ansi():
    """Enable ANSI escape processing on Windows 10+."""
    kernel32 = ctypes.windll.kernel32
    h = kernel32.GetStdHandle(-11)          # STD_OUTPUT_HANDLE
    mode = ctypes.c_ulong()
    kernel32.GetConsoleMode(h, ctypes.byref(mode))
    kernel32.SetConsoleMode(h, mode.value | 0x0004)   # ENABLE_VIRTUAL_TERMINAL_PROCESSING

_enable_ansi()

def _w(text):
    """Write raw text to stdout (no newline)."""
    sys.stdout.write(text)
    sys.stdout.flush()

def cls():
    """Clear the terminal screen."""
    os.system("cls" if os.name == "nt" else "clear")

def cprint(color, text):
    try:
        print(f"{color}{text}{RESET}")
    except UnicodeEncodeError:
        print(f"{color}{text.encode('ascii', 'replace').decode()}{RESET}")

def header(text):
    cls()
    width = max(len(text) + 4, 56)
    cprint(CYAN, "=" * width)
    cprint(CYAN + BOLD, f"  {text}")
    cprint(CYAN, "=" * width)

def success(text):
    cprint(GREEN, f"  [OK] {text}")

def warn(text):
    cprint(YELLOW, f"  [!]  {text}")

def error(text):
    cprint(RED, f"  [X]  {text}")

def info(text):
    print(f"       {text}")

def big_success(title, details=None):
    """Big, unmissable success banner."""
    width = 60
    print()
    cprint(GREEN + BOLD, "  " + "=" * width)
    cprint(GREEN + BOLD, "  " + " " * width)
    cprint(GREEN + BOLD, f"    >>> {title} <<<".center(width + 2))
    cprint(GREEN + BOLD, "  " + " " * width)
    cprint(GREEN + BOLD, "  " + "=" * width)
    if details:
        for d in details:
            cprint(GREEN, f"    {d}")
    print()

def big_fail(title, details=None):
    """Big, unmissable failure banner."""
    width = 60
    print()
    cprint(RED + BOLD, "  " + "=" * width)
    cprint(RED + BOLD, "  " + " " * width)
    cprint(RED + BOLD, f"    XXX {title} XXX".center(width + 2))
    cprint(RED + BOLD, "  " + " " * width)
    cprint(RED + BOLD, "  " + "=" * width)
    if details:
        for d in details:
            cprint(RED, f"    {d}")
    print()

class Spinner:
    """Animated spinner that runs in a background thread."""
    FRAMES = ["|", "/", "-", "\\"]

    def __init__(self, message="Compiling"):
        self._message = message
        self._stop = threading.Event()
        self._thread = None
        self._start_time = 0

    def start(self):
        self._start_time = time.time()
        self._stop.clear()
        self._thread = threading.Thread(target=self._spin, daemon=True)
        self._thread.start()

    def _spin(self):
        i = 0
        while not self._stop.is_set():
            elapsed = time.time() - self._start_time
            mins, secs = divmod(int(elapsed), 60)
            ts = f"{mins}:{secs:02d}"
            frame = self.FRAMES[i % len(self.FRAMES)]
            print(f"\r  {CYAN}{frame}{RESET} {self._message}... {DIM}{ts}{RESET}   ", end="", flush=True)
            i += 1
            self._stop.wait(0.15)
        # Clear the spinner line
        print(f"\r{' ' * 60}\r", end="", flush=True)

    def stop(self):
        self._stop.set()
        if self._thread:
            self._thread.join()

def pick(prompt, options, default=None):
    """Arrow-key picker. Up/Down to move, Enter to select."""
    if not options:
        return None

    idx = 0
    if default is not None:
        for i, (_, v) in enumerate(options):
            if v == default:
                idx = i
                break

    total = len(options)
    print()
    cprint(BOLD, f"  {prompt}")
    cprint(DIM, "  (arrow keys to move, Enter to select)")

    # Draw all rows
    _w(HIDE_CUR)
    for i, (label, _) in enumerate(options):
        if i == idx:
            _w(f"  {REV} > {label} {RESET}\n")
        else:
            _w(f"     {label}\n")

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                old = idx
                if ch2 == "H":          # Up
                    idx = (idx - 1) % total
                elif ch2 == "P":        # Down
                    idx = (idx + 1) % total
                else:
                    continue
                if old != idx:
                    # Redraw old row (un-highlight)
                    _w(f"\033[{total - old}A")
                    _w(f"\r{ERASE_LN}     {options[old][0]}")
                    # Redraw new row (highlight)
                    if idx > old:
                        _w(f"\033[{idx - old}B")
                    else:
                        _w(f"\033[{old - idx}A")
                    _w(f"\r{ERASE_LN}  {REV} > {options[idx][0]} {RESET}")
                    # Return cursor to bottom
                    remaining = total - idx
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                return options[idx][1]
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def menu_pick(items):
    """Full-screen arrow-key menu with separators and styled rows.

    Each item is a tuple:
      ("label", "value", "style")   — selectable row
      ("---",)                      — separator (not selectable)

    Styles:
      "action"  — bold green block   ██ COMPILE ██
      "normal"  — plain text
      "dim"     — dimmed text
    """
    # Build display rows and map selectable indices
    rows = []           # (display_normal, display_highlighted, value_or_None)
    selectable = []     # indices into rows[] that are selectable

    for item in items:
        if item[0] == "---":
            rows.append(None)   # separator
        else:
            label, value, style = item
            if style == "action":
                normal = f"     {GREEN}{BOLD}{label}{RESET}"
                hilite = f"  {REV} \u25b8 {GREEN}{BOLD}{label}{RESET}{REV} {RESET}"
            elif style == "dim":
                normal = f"     {DIM}{label}{RESET}"
                hilite = f"  {REV} \u25b8 {label} {RESET}"
            else:
                normal = f"     {label}"
                hilite = f"  {REV} \u25b8 {label} {RESET}"
            selectable.append(len(rows))
            rows.append((normal, hilite, value))

    if not selectable:
        return None

    sel = 0                 # index into selectable[]
    total_rows = len(rows)

    def _render_row(ri, is_sel):
        if rows[ri] is None:
            return f"     {DIM}\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500{RESET}"
        normal, hilite, _ = rows[ri]
        return hilite if is_sel else normal

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

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                _, _, value = rows[selectable[sel]]
                return value
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def pick_filterable(prompt, options, default=None):
    """Arrow-key picker with type-to-filter for long lists.
    Type to narrow, Backspace to remove, Esc to clear filter, Enter to select."""
    if not options:
        return None

    filter_text = ""
    filtered = list(options)
    idx = 0
    total_slots = min(len(options), 20)  # max visible rows

    if default is not None:
        for i, (_, v) in enumerate(filtered):
            if v == default:
                idx = i
                break

    _ansi_re = re.compile(r'\033\[[0-9;]*m')

    def _apply():
        nonlocal filtered, idx
        if filter_text:
            ft = filter_text.lower()
            filtered = [(l, v) for l, v in options if ft in _ansi_re.sub("", l).lower()]
        else:
            filtered = list(options)
        if idx >= len(filtered):
            idx = max(0, len(filtered) - 1)

    # Compute viewport offset so selected item is visible
    def _viewport():
        if not filtered:
            return 0
        if idx < total_slots:
            return 0
        return idx - total_slots + 1

    def _draw_full():
        _w(HIDE_CUR)
        fdisp = f"  filter: {filter_text}" if filter_text else ""
        _w(f"{ERASE_LN}  {BOLD}{prompt}{RESET}{fdisp}\n")
        _w(f"{ERASE_LN}  {DIM}(type to filter, arrows to move, Enter to select){RESET}\n")
        vp = _viewport()
        for slot in range(total_slots):
            _w(ERASE_LN)
            ri = vp + slot
            if ri < len(filtered):
                label = filtered[ri][0]
                if ri == idx:
                    _w(f"  {REV} > {label} {RESET}\n")
                else:
                    _w(f"     {label}\n")
            else:
                _w("\n")
        count_str = f"  {DIM}{len(filtered)}/{len(options)} matches{RESET}"
        if len(filtered) > total_slots:
            count_str += f"  {DIM}(scroll with arrows){RESET}"
        _w(f"{ERASE_LN}{count_str}\n")

    def _repaint():
        # Move cursor back to top of our block: 2 header + total_slots + 1 count
        _w(f"\033[{total_slots + 3}A")
        _draw_full()

    _draw_full()

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if not filtered:
                    continue
                old = idx
                if ch2 == "H":          # Up
                    idx = (idx - 1) % len(filtered)
                elif ch2 == "P":        # Down
                    idx = (idx + 1) % len(filtered)
                else:
                    continue
                if old != idx:
                    _repaint()

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                if filtered:
                    return filtered[idx][1]

            elif ch == "\x1b":      # Escape — clear filter
                if filter_text:
                    filter_text = ""
                    _apply()
                    _repaint()

            elif ch == "\x08":      # Backspace
                if filter_text:
                    filter_text = filter_text[:-1]
                    _apply()
                    _repaint()

            elif ch.isprintable() and ch != "\t":
                filter_text += ch
                _apply()
                _repaint()
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise

def confirm(prompt, default_yes=True):
    hint = "Y/n" if default_yes else "y/N"
    raw = input(f"\n  {prompt} [{hint}]: ").strip().lower()
    if not raw:
        return default_yes
    return raw in ("y", "yes")

# -----------------------------------------------------------------------------
# Preferences persistence
# -----------------------------------------------------------------------------
def load_prefs():
    if PREFS_FILE.exists():
        try:
            return json.loads(PREFS_FILE.read_text(encoding="utf-8"))
        except Exception:
            pass
    return {}

def save_prefs(prefs):
    PREFS_FILE.parent.mkdir(parents=True, exist_ok=True)
    PREFS_FILE.write_text(json.dumps(prefs, indent=2), encoding="utf-8")

# -----------------------------------------------------------------------------
# arduino-cli wrappers
# -----------------------------------------------------------------------------
def run_cli(*args, capture=True, timeout=600):
    cmd = [str(ARDUINO_CLI)] + list(args)
    try:
        r = subprocess.run(cmd, capture_output=capture, text=True, encoding="utf-8",
                           errors="replace", timeout=timeout)
        return r
    except FileNotFoundError:
        error(f"arduino-cli not found at: {ARDUINO_CLI}")
        error("Update ARDUINO_CLI path in this script.")
        sys.exit(1)
    except subprocess.TimeoutExpired:
        error(f"Command timed out after {timeout}s")
        return None

def get_board_options(fqbn):
    """Parse board details to get available options and their values."""
    r = run_cli("board", "details", "--fqbn", fqbn)
    if not r or r.returncode != 0:
        return {}

    options = {}
    current_option_key = None
    current_option_name = None
    current_values = []
    current_default = None

    # Strip ANSI codes for parsing
    clean = re.sub(r'\033\[[0-9;]*m', '', r.stdout)

    for line in clean.splitlines():
        m = re.match(r'^Option:\s+(.+?)\s{2,}(\S+)\s*$', line)
        if m:
            if current_option_key and current_values:
                options[current_option_key] = {
                    "name": current_option_name,
                    "values": current_values,
                    "default": current_default,
                }
            current_option_name = m.group(1).strip()
            current_option_key = m.group(2).strip()
            current_values = []
            current_default = None
            continue

        if current_option_key:
            vm = re.match(r'^\s+(.+?)\s{2,}(?:\u2714\s+)?(\S+=\S+)\s*$', line)
            if vm:
                label = vm.group(1).strip()
                kv = vm.group(2).strip()
                val = kv.split("=", 1)[1] if "=" in kv else kv
                current_values.append((label, val))
                if "\u2714" in line:
                    current_default = val
                continue

    if current_option_key and current_values:
        options[current_option_key] = {
            "name": current_option_name,
            "values": current_values,
            "default": current_default,
        }

    return options

# =============================================================================
#  LABEL SET - Discovery, selection, and generate_data.py
# =============================================================================
def discover_label_sets():
    """Find all LABEL_SET_* directories under src/LABELS/."""
    sets = []
    if LABELS_DIR.exists():
        for d in sorted(LABELS_DIR.iterdir()):
            if d.is_dir() and d.name.startswith("LABEL_SET_"):
                name = d.name.replace("LABEL_SET_", "")
                has_gen = (d / "generate_data.py").exists()
                sets.append((name, d, has_gen))
    return sets

def read_active_label_set():
    """Read the current LABEL_SET from active_set.h."""
    if ACTIVE_SET.exists():
        text = ACTIVE_SET.read_text(encoding="utf-8")
        m = re.search(r'#define\s+LABEL_SET\s+(\S+)', text)
        if m:
            return m.group(1)
    return None

def select_label_set(prefs):
    """Interactive label set selection."""
    header("Label Set Selection")

    sets = discover_label_sets()
    if not sets:
        error("No LABEL_SET_* directories found in src/LABELS/")
        return None

    active = read_active_label_set()
    if active:
        info(f"Currently active: {CYAN}{active}{RESET}")

    options = []
    for name, path, has_gen in sets:
        marker = ""
        if not has_gen:
            marker = f" {DIM}(no generate_data.py){RESET}"
        options.append((f"{name}{marker}", name))

    chosen = pick("Select the LABEL SET to build:", options, default=active)
    return chosen

def run_generate_data(label_set_name):
    """Launch generate_data.py in its own console window and wait for it to finish."""
    header(f"Running generate_data.py for {label_set_name}")

    label_dir = LABELS_DIR / f"LABEL_SET_{label_set_name}"
    gen_script = label_dir / "generate_data.py"

    if not gen_script.exists():
        error(f"generate_data.py not found in {label_dir}")
        return False

    info(f"Directory: {label_dir}")
    info(f"Script:    {gen_script}")
    print()
    info("Opening generator in a new window...")
    info("Complete it there, then come back here to continue.")
    print()

    # Launch in its own console window — the script is fully interactive
    # (multiple input() prompts, sub-processes) so let the user drive it.
    proc = subprocess.Popen(
        [sys.executable, str(gen_script)],
        cwd=str(label_dir),
        creationflags=subprocess.CREATE_NEW_CONSOLE,
    )
    proc.wait()

    # generator_core.py always calls sys.exit(1) even on success,
    # so we verify by checking the actual output files instead.
    new_active = read_active_label_set()
    data_h = label_dir / "DCSBIOSBridgeData.h"

    if new_active == label_set_name and data_h.exists():
        success(f"generate_data.py completed for {label_set_name}")
        info(f"active_set.h -> {CYAN}{label_set_name}{RESET}")
        return True
    elif new_active == label_set_name:
        warn(f"active_set.h updated but DCSBIOSBridgeData.h not found")
        return True
    else:
        error(f"generate_data.py did not produce expected output")
        if new_active:
            error(f"active_set.h shows: {new_active} (expected {label_set_name})")
        return False

# =============================================================================
#  CONFIG.H - Transport / Role / Settings editor
# =============================================================================
def read_config_define(name):
    """Read a #define value from Config.h."""
    if not CONFIG_H.exists():
        return None
    text = CONFIG_H.read_text(encoding="utf-8")
    # Match: #define NAME    value  // optional comment
    m = re.search(rf'#define\s+{re.escape(name)}\s+(\S+)', text)
    if m:
        return m.group(1)
    return None

def write_config_define(name, value):
    """Update a #define in Config.h, preserving formatting and comments.

    Safety: backs up Config.h on the first write of each session,
    then uses atomic write (temp + rename) for the actual update.
    """
    if not CONFIG_H.exists():
        error("Config.h not found!")
        return False

    # Back up before first modification this session
    backup_file(CONFIG_H)

    text = CONFIG_H.read_text(encoding="utf-8")

    # Pattern: #define NAME <old_value> <rest of line>
    # We replace only the value, keeping alignment and comments
    pattern = rf'(#define\s+{re.escape(name)}\s+)\S+(.*)'
    new_text, count = re.subn(pattern, rf'\g<1>{value}\2', text)

    if count == 0:
        warn(f"Could not find #define {name} in Config.h")
        return False

    if not safe_write_file(CONFIG_H, new_text):
        error(f"Failed to write Config.h! Check for Config.h.tmp in project root.")
        return False

    return True


def restore_config():
    """Interactive restore of Config.h from a backup."""
    header("Restore Config.h")

    backups = list_backups("Config.h")
    if not backups:
        warn("No backups found.")
        info(f"Backups are created automatically when the tool modifies Config.h.")
        return False

    info(f"Found {len(backups)} backup(s):\n")

    # Build picker options
    options = []
    for bak_path, ts_display, size in backups:
        label = f"{ts_display}  ({size:,} bytes)"
        options.append((label, str(bak_path)))

    chosen = pick("Select backup to restore:", options)
    if not chosen:
        return False

    chosen_path = Path(chosen)

    # Show what changed between backup and current
    changes = diff_backup_vs_current(chosen_path, CONFIG_H)
    if changes:
        print()
        info(f"Differences (backup vs current):")
        for name, old_val, new_val in changes:
            old_str = old_val if old_val is not None else "(missing)"
            new_str = new_val if new_val is not None else "(missing)"
            info(f"  {name}: {CYAN}{old_str}{RESET} -> {CYAN}{new_str}{RESET}")
        print()
        info(f"Restoring will revert Config.h to the backup values above.")
    else:
        print()
        info("No #define differences found between backup and current Config.h.")
        info("Files may differ in comments or whitespace.")

    if not confirm("Restore this backup?", default_yes=False):
        info("Restore cancelled.")
        return False

    # Back up current state before overwriting (so restore itself is reversible)
    backup_file(CONFIG_H)

    try:
        content = chosen_path.read_text(encoding="utf-8")
        if safe_write_file(CONFIG_H, content):
            success("Config.h restored successfully.")
            return True
        else:
            error("Failed to write restored Config.h!")
            return False
    except OSError as e:
        error(f"Failed to read backup: {e}")
        return False


def read_current_transport():
    """Determine which transport is active (from snapshot)."""
    if config_get("RS485_SLAVE_ENABLED") == "1":
        return "slave"
    elif config_get("USE_DCSBIOS_SERIAL") == "1":
        return "serial"
    elif config_get("USE_DCSBIOS_USB") == "1":
        return "usb"
    elif config_get("USE_DCSBIOS_WIFI") == "1":
        return "wifi"
    elif config_get("USE_DCSBIOS_BLUETOOTH") == "1":
        return "ble"
    return None

def read_current_role():
    """Determine current device mode (from snapshot)."""
    master = config_get("RS485_MASTER_ENABLED")
    slave  = config_get("RS485_SLAVE_ENABLED")

    if slave == "1":
        return "slave"
    elif master == "1":
        return "master"
    else:
        return "standalone"

def transport_label(t):
    """Get friendly name for a transport key."""
    names = {
        "serial": "Serial (CDC/Socat)",
        "usb":    "USB Native",
        "wifi":   "WiFi",
        "ble":    "Bluetooth BLE",
        "slave":  "RS485 Slave",
    }
    return names.get(t, str(t) if t else "?")

def role_label(r):
    """Get friendly name for a role/mode key."""
    names = {"standalone": "Standalone", "master": "RS485 Master", "slave": "RS485 Slave"}
    return names.get(r, str(r) if r else "?")

def configure_transport_and_role(prefs):
    """Interactive Config.h transport and role configuration.

    Flow:
      1. Ask "Is this an RS485 Master?" (Yes/No)
      2. Choose Transport (WiFi / USB / Socat / RS485 Slave)
         - If RS485 Master: RS485 Slave option is hidden (master needs a real transport)
      3. If RS485 Slave chosen: ask for slave address

    Enforces the Config.h assert:
      (BLE + WiFi + USB + Serial + RS485_SLAVE) == 1
    """
    header("Firmware Configuration (Config.h)")

    current_transport = read_current_transport()
    current_role = read_current_role()

    info(f"Current: {CYAN}{role_label(current_role)}{RESET}")
    if current_role != "slave":
        info(f"         {CYAN}{transport_label(current_transport)}{RESET}")

    # --- Step 1: Is this an RS485 Master? ---
    is_master_default = "yes" if current_role == "master" else "no"
    is_master = pick("Is this an RS485 Master?", [
        ("Yes", "yes"),
        ("No",  "no"),
    ], default=is_master_default)

    # --- Step 2: Choose Transport ---
    if is_master == "yes":
        header("Choose Transport (WiFi / USB / Socat)")
        info(f"{DIM}RS485 Master requires a real transport to forward data.{RESET}")
        # Filter out RS485 Slave — master can't be slave
        transport_opts = [(label, key) for label, key in TRANSPORTS if key != "slave"]
        prev = current_transport if current_transport != "slave" else "usb"
    else:
        header("Choose Transport (WiFi / USB / Socat / RS485 Slave)")
        transport_opts = list(TRANSPORTS)
        prev = current_transport or "usb"

    transport = pick("Select transport:", transport_opts, default=prev)

    # Determine mode from the two answers
    if is_master == "yes":
        mode = "master"
    elif transport == "slave":
        mode = "slave"
    else:
        mode = "standalone"

    # Info tips
    if transport == "usb":
        info(f"{DIM}USB transport: USB Mode will be set to USB-OTG (TinyUSB){RESET}")
    if mode == "master" and transport == "wifi":
        info(f"{DIM}Master + WiFi: RS485 runs in loop() (WiFi occupies Core 0){RESET}")
    elif mode == "master":
        info(f"{DIM}Master + {transport_label(transport)}: RS485 task on Core 0{RESET}")

    # --- Step 3: Slave address (only for slave) ---
    slave_address = None
    if mode == "slave":
        header("RS485 Slave Configuration")
        current_addr = config_get("RS485_SLAVE_ADDRESS") or "1"
        raw = input(f"\n  Slave address [1-126] (Enter={current_addr}): ").strip()
        if raw:
            try:
                addr = int(raw)
                if 1 <= addr <= 126:
                    slave_address = addr
                else:
                    warn("Invalid range. Using current.")
                    slave_address = int(current_addr)
            except ValueError:
                slave_address = int(current_addr)
        else:
            slave_address = int(current_addr)

    # --- Apply all five transport defines so exactly ONE is 1 ---
    changes = []

    if mode == "slave":
        active_key = "RS485_SLAVE_ENABLED"
    else:
        active_key = TRANSPORT_KEY[transport]

    for define in TRANSPORT_DEFINES:
        target = 1 if define == active_key else 0
        old = config_get(define)
        if old != str(target):
            config_set(define, str(target))
            changes.append(f"{define}: {old} -> {target}")

    # --- RS485 Master/Slave flags ---
    master_val = 1 if mode == "master" else 0
    slave_role_val = 1 if mode == "slave" else 0   # redundant with transport, but explicit

    old_m = config_get("RS485_MASTER_ENABLED")
    if old_m != str(master_val):
        config_set("RS485_MASTER_ENABLED", str(master_val))
        changes.append(f"RS485_MASTER_ENABLED: {old_m} -> {master_val}")

    # RS485_SLAVE_ENABLED already set above in the transport loop

    # --- Slave address ---
    if slave_address is not None:
        old = config_get("RS485_SLAVE_ADDRESS")
        if old != str(slave_address):
            config_set("RS485_SLAVE_ADDRESS", str(slave_address))
            changes.append(f"RS485_SLAVE_ADDRESS: {old} -> {slave_address}")

    # --- RS485 task config for master (auto-tune based on transport) ---
    if mode == "master":
        if transport == "wifi":
            config_set("RS485_USE_TASK", "0")
            changes.append("RS485_USE_TASK -> 0 (loop, best for WiFi)")
        else:
            config_set("RS485_USE_TASK", "1")
            config_set("RS485_TASK_CORE", "0")
            changes.append("RS485_USE_TASK -> 1, RS485_TASK_CORE -> 0")

    # --- Smart USB mode for dual-USB boards ---
    if board_has_dual_usb(prefs):
        ideal = preferred_usb_mode(transport)
        current_usb = prefs.get("options", {}).get("USBMode")
        if current_usb != ideal:
            prefs.setdefault("options", {})["USBMode"] = ideal
            usb_label = "USB-OTG (TinyUSB)" if ideal == "default" else "HW CDC"
            changes.append(f"USBMode -> {usb_label} (auto, matches {transport_label(transport)})")

    # Save in prefs
    prefs["transport"] = transport if transport else "slave"
    prefs["role"] = mode

    print()
    if changes:
        success("Config.h updated:")
        for c in changes:
            info(f"  {c}")
    else:
        info("No changes needed - Config.h already matches.")

    # Immediate cross-validation feedback
    issues = validate_config_vs_board(prefs)
    for level, msg in issues:
        if level == FATAL:
            error(f"[FATAL] {msg}")
        else:
            warn(f"[WARNING] {msg}")

    return prefs["transport"], mode

# =============================================================================
#  Board selection & options
# =============================================================================
def get_all_boards():
    """Query arduino-cli for every installed board. Returns [(name, fqbn)]."""
    r = run_cli("board", "listall", "--format", "json")
    if not r or r.returncode != 0:
        return []
    try:
        data = json.loads(r.stdout)
        return [(b["name"], b["fqbn"]) for b in data.get("boards", [])]
    except (json.JSONDecodeError, KeyError):
        return []


def select_board(prefs):
    header("Board Selection")
    info("Querying installed boards...")

    boards = get_all_boards()
    if not boards:
        warn("Could not list boards. Enter FQBN manually.")
        fqbn = input("\n  FQBN (e.g. esp32:esp32:esp32s3): ").strip()
        if not fqbn:
            error("No FQBN entered.")
            sys.exit(1)
        return fqbn

    saved_fqbn = prefs.get("fqbn")

    # Build display list: just the board name — clean and readable
    options = [(name, fqbn) for name, fqbn in boards]

    header("Board Selection")
    if saved_fqbn:
        saved_name = next((n for n, f in boards if f == saved_fqbn), saved_fqbn)
        info(f"Current: {CYAN}{saved_name}{RESET}  {DIM}({saved_fqbn}){RESET}")

    fqbn = pick_filterable(f"Select board ({len(boards)} available):", options, default=saved_fqbn)

    friendly = next((n for n, f in boards if f == fqbn), fqbn)
    prefs["board_name"] = friendly
    success(f"Board: {friendly}  ({fqbn})")
    return fqbn

def configure_options(fqbn, prefs):
    header("Board Options")
    info("Fetching available options from arduino-cli...")

    board_options = get_board_options(fqbn)
    if not board_options:
        warn("Could not retrieve board options. Using CockpitOS defaults.")
        return dict(COCKPITOS_DEFAULTS)

    saved_options = prefs.get("options", {})
    selected = {}

    # Apply CockpitOS hard defaults first
    for key, val in COCKPITOS_DEFAULTS.items():
        if key in board_options:
            selected[key] = val

    # Smart USB mode: set based on current transport (only on dual-USB boards)
    if "USBMode" in board_options and len(board_options["USBMode"].get("values", [])) >= 2:
        transport = read_current_transport()
        ideal = preferred_usb_mode(transport)
        selected["USBMode"] = ideal
        usb_label = "USB-OTG (TinyUSB)" if ideal == "default" else "HW CDC"
        info(f"{DIM}USB Mode -> {usb_label} (auto, matches {transport_label(transport)}){RESET}")

    info(f"{DIM}CockpitOS defaults applied: CDC=Off, Partition=NoOTA 2MB, 240MHz, QIO{RESET}")

    mode = pick("Board options mode:", [
        ("Quick - use CockpitOS defaults (recommended)", "quick"),
        ("Full - configure ALL board options", "full"),
    ], default="quick")

    if mode == "quick":
        for key, opt in board_options.items():
            if key not in selected:
                selected[key] = saved_options.get(key, opt["default"])
        return selected

    # Full mode — show everything, allow overriding defaults
    keys_to_show = [k for k in board_options]
    info(f"{YELLOW}Full mode: you can override CockpitOS defaults (not recommended){RESET}")

    for i, key in enumerate(keys_to_show):
        opt = board_options[key]
        current_val = selected.get(key, saved_options.get(key, opt["default"]))
        header(f"Board Options ({i+1}/{len(keys_to_show)})")
        chosen = pick(f"{opt['name']}:", opt["values"], default=current_val)
        selected[key] = chosen

    # Fill remaining with defaults
    for key, opt in board_options.items():
        if key not in selected:
            selected[key] = saved_options.get(key, opt["default"])

    return selected

def build_firmware_name():
    """Build a descriptive firmware filename from current config.

    Convention (ALL CAPS):
      Standalone:   LABEL_TRANSPORT.bin               e.g. MAIN_USB.bin
      RS485 Master: RS485_MASTER_LABEL_TRANSPORT.bin   e.g. RS485_MASTER_MAIN_USB.bin
      RS485 Slave:  RS485_SLAVE_LABEL.bin              e.g. RS485_SLAVE_MAIN.bin
    """
    label = (read_active_label_set() or "UNKNOWN").upper()
    role = read_current_role()
    transport = read_current_transport()

    transport_names = {
        "serial": "SERIAL", "usb": "USB", "wifi": "WIFI", "ble": "BLE",
    }

    if role == "slave":
        return f"RS485_SLAVE_{label}"
    elif role == "master":
        t = transport_names.get(transport, (transport or "UNKNOWN").upper())
        return f"RS485_MASTER_{label}_{t}"
    else:
        t = transport_names.get(transport, (transport or "UNKNOWN").upper())
        return f"{label}_{t}"


# =============================================================================
#  Compile
# =============================================================================
def compile_sketch(fqbn, options, verbose=False, export_bin=False):
    header("Compiling CockpitOS")

    if options:
        opts_str = ",".join(f"{k}={v}" for k, v in options.items() if v is not None)
        full_fqbn = f"{fqbn}:{opts_str}" if opts_str else fqbn
    else:
        full_fqbn = fqbn

    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    CACHE_DIR.mkdir(parents=True, exist_ok=True)

    # Show build summary
    active_ls = read_active_label_set() or "?"
    transport = read_current_transport() or "?"
    role = read_current_role() or "?"

    fw_name = build_firmware_name()
    info(f"Firmware:  {CYAN}{fw_name}.bin{RESET}")
    info(f"Label Set: {CYAN}{active_ls}{RESET}")
    info(f"Transport: {CYAN}{transport_label(transport)}{RESET}")
    info(f"Role:      {CYAN}{role_label(role)}{RESET}")
    info(f"FQBN:      {full_fqbn[:80]}{'...' if len(full_fqbn) > 80 else ''}")
    print()

    cmd = [
        str(ARDUINO_CLI), "compile",
        "--fqbn", full_fqbn,
        "--build-path", str(BUILD_DIR),
        "--build-cache-path", str(CACHE_DIR),
        str(SKETCH_DIR),
    ]

    if verbose:
        cmd.append("--verbose")

    if export_bin:
        cmd.extend(["--output-dir", str(SKETCH_DIR / "Binaries")])

    # Start compilation with spinner (non-verbose) or streamed output (verbose)
    start = time.time()
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        bufsize=1,
    )

    output_lines = []
    warning_count = 0
    error_count = 0

    if verbose:
        # Verbose: stream everything live
        cprint(DIM, "  -- Compiler output -----------------------------------------------")
        print()
        for line in proc.stdout:
            output_lines.append(line)
            stripped = line.rstrip()
            if not stripped:
                continue
            if "error:" in stripped.lower():
                cprint(RED, f"  {stripped}")
                error_count += 1
            elif "warning:" in stripped.lower():
                cprint(YELLOW, f"  {stripped}")
                warning_count += 1
            else:
                print(f"  {DIM}{stripped}{RESET}")
        print()
        cprint(DIM, "  -------------------------------------------------------------")
    else:
        # Non-verbose: show spinner, collect output silently
        spinner = Spinner("Compiling")
        spinner.start()
        for line in proc.stdout:
            output_lines.append(line)
            if "error:" in line.lower():
                error_count += 1
            elif "warning:" in line.lower():
                warning_count += 1
        spinner.stop()

    proc.wait()
    elapsed = time.time() - start

    if proc.returncode == 0:
        # Collect size info
        size_lines = []
        for line in output_lines:
            if "Sketch uses" in line or "Global variables" in line:
                size_lines.append(line.strip())

        # Find the .bin, rename to descriptive ALL-CAPS name, copy to output/
        fw_name = build_firmware_name()
        bin_info = None
        for f in BUILD_DIR.iterdir():
            if f.suffix == ".bin" and "merged" not in f.name and f.name.startswith("CockpitOS"):
                # Keep a copy in BUILD_DIR (for upload_sketch)
                build_dest = BUILD_DIR / f"{fw_name}.bin"
                shutil.copy2(f, build_dest)

                # Copy final binary to compiler/output/ for easy access
                OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
                output_dest = OUTPUT_DIR / f"{fw_name}.bin"
                shutil.copy2(f, output_dest)

                size_bytes = output_dest.stat().st_size
                bin_info = f"Binary: {fw_name}.bin ({size_bytes:,} bytes)"
                break

        details = [f"Time: {elapsed:.1f}s"]
        details.extend(size_lines)
        if bin_info:
            details.append(bin_info)
            details.append(f"Output: compiler{os.sep}output{os.sep}")
        if warning_count:
            details.append(f"Warnings: {warning_count}")

        big_success("BUILD SUCCESSFUL", details)
        return True
    else:
        error_lines = [l.rstrip() for l in output_lines if "error:" in l.lower()]
        details = [f"Exit code {proc.returncode}  ({elapsed:.1f}s)"]
        details.extend(error_lines[-10:])

        big_fail("BUILD FAILED", details)
        return False

# =============================================================================
#  Upload
# =============================================================================
def scan_ports():
    """Scan for connected devices via arduino-cli. Returns [(display, address)]."""
    r = run_cli("board", "list", "--format", "json")
    if not r or r.returncode != 0:
        return []
    try:
        data = json.loads(r.stdout)
        entries = data.get("detected_ports", [])
        ports = []
        for entry in entries:
            p = entry.get("port", {})
            addr = p.get("address", "")
            proto_label = p.get("protocol_label", "")
            vid = p.get("properties", {}).get("vid", "")
            board_name = ""
            if entry.get("matching_boards"):
                board_name = entry["matching_boards"][0].get("name", "")

            if addr:
                parts = [addr]
                if board_name:
                    parts.append(board_name)
                elif proto_label:
                    parts.append(proto_label)
                if vid:
                    parts.append(f"VID:{vid}")
                display = " - ".join(parts)
                ports.append((display, addr))
        return ports
    except (json.JSONDecodeError, KeyError):
        return []


def upload_sketch(fqbn, options, port=None):
    header("Upload to Device")

    if not port:
        while True:
            info("Scanning for devices...")
            ports = scan_ports()

            if ports:
                port = pick("Select port:", ports)
                break
            else:
                warn("No devices found.")
                action = pick("What to do?", [
                    ("Scan again (connect your board first)", "retry"),
                    ("Enter port manually", "manual"),
                    ("Cancel upload", "cancel"),
                ], default="retry")

                if action == "retry":
                    header("Upload to Device")
                    continue
                elif action == "manual":
                    port = input("\n  Enter port (e.g. COM3): ").strip()
                    break
                else:
                    return False

    if not port:
        error("No port specified.")
        return False

    if options:
        opts_str = ",".join(f"{k}={v}" for k, v in options.items() if v is not None)
        full_fqbn = f"{fqbn}:{opts_str}" if opts_str else fqbn
    else:
        full_fqbn = fqbn

    info(f"Port: {port}")
    info(f"FQBN: {full_fqbn[:80]}")
    print()

    cmd = [
        str(ARDUINO_CLI), "upload",
        "--fqbn", full_fqbn,
        "--port", port,
        "--input-dir", str(BUILD_DIR),
    ]

    start = time.time()
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    for line in proc.stdout:
        stripped = line.rstrip()
        if stripped:
            print(f"  {stripped}")
    proc.wait()
    elapsed = time.time() - start

    if proc.returncode == 0:
        big_success("UPLOAD SUCCESSFUL", [f"Port: {port}", f"Time: {elapsed:.1f}s"])
        return True
    else:
        big_fail("UPLOAD FAILED", [f"Exit code {proc.returncode}", f"Port: {port}"])
        return False

# =============================================================================
#  Clean
# =============================================================================
def clean_build():
    header("Clean Build")
    removed = 0
    for d in [BUILD_DIR, CACHE_DIR]:
        if d.exists():
            shutil.rmtree(d)
            info(f"Removed: {d}")
            removed += 1
    # Clean output binaries
    if OUTPUT_DIR.exists():
        for f in OUTPUT_DIR.iterdir():
            if f.suffix == ".bin":
                f.unlink()
                info(f"Removed: {f.name}")
                removed += 1
    if removed:
        success("Build directories cleaned.")
    else:
        info("Nothing to clean.")

# =============================================================================
#  Show full config summary (Config.h + active_set.h + board prefs)
# =============================================================================
def show_config(prefs):
    header("Current Configuration")
    print()

    # Label Set
    active_ls = read_active_label_set()
    info(f"Label Set:   {CYAN}{active_ls or '(not set)'}{RESET}")

    # Transport & Role from Config.h
    transport = read_current_transport()
    role = read_current_role()
    info(f"Transport:   {CYAN}{transport_label(transport) if transport else '(unknown)'}{RESET}")
    info(f"Role:        {CYAN}{role_label(role) if role else '(unknown)'}{RESET}")

    # RS485 details if applicable
    if role == "master":
        smart = config_get("RS485_SMART_MODE") or "0"
        max_addr = config_get("RS485_MAX_SLAVE_ADDRESS") or "127"
        use_task = config_get("RS485_USE_TASK") or "?"
        task_core = config_get("RS485_TASK_CORE") or "?"
        info(f"  RS485 Smart Mode:  {smart}")
        info(f"  Max Slave Address: {max_addr}")
        info(f"  Use Task: {use_task}  Core: {task_core}")
    elif role == "slave":
        addr = config_get("RS485_SLAVE_ADDRESS") or "?"
        isr = config_get("RS485_USE_ISR_MODE") or "?"
        info(f"  Slave Address: {addr}")
        info(f"  ISR Mode: {isr}")

    # RS485 pins
    if role in ("master", "slave"):
        tx = config_get("RS485_TX_PIN") or "?"
        rx = config_get("RS485_RX_PIN") or "?"
        de = config_get("RS485_DE_PIN") or "?"
        info(f"  Pins: TX={tx}  RX={rx}  DE={de}")

    print()

    # Board
    fqbn = prefs.get("fqbn", "(not set)")
    friendly = prefs.get("board_name")
    board_str = f"{friendly} ({fqbn})" if friendly else fqbn
    info(f"Board:       {CYAN}{board_str}{RESET}")

    dual_usb = board_has_dual_usb(prefs)
    if dual_usb:
        info(f"  {DIM}Dual USB board (USB-OTG + HW CDC){RESET}")

    opts = prefs.get("options", {})
    if opts:
        # Show only the interesting ones
        key_opts = ["PartitionScheme", "FlashSize", "FlashMode", "USBMode", "CPUFreq", "PSRAM"]
        for k in key_opts:
            if k in opts:
                info(f"  {k} = {opts[k]}")

    # Debug flags
    debug = config_get("DEBUG_ENABLED") or "0"
    verbose = config_get("VERBOSE_MODE") or "0"
    if debug != "0" or verbose != "0":
        print()
        warn(f"Debug flags active: DEBUG_ENABLED={debug}, VERBOSE_MODE={verbose}")

    # Cross-validation
    issues = validate_config_vs_board(prefs)
    if issues:
        print()
        for level, msg in issues:
            if level == FATAL:
                error(f"[FATAL] {msg}")
            else:
                warn(f"[WARNING] {msg}")

# =============================================================================
#  Compile flow — always confirms label set, then compiles
# =============================================================================
def do_compile(prefs, verbose=False):
    """Compile flow. Always asks for label set (defaults to current), then builds."""

    # Always ask for label set — but default to current so Enter skips through
    label_set = select_label_set(prefs)
    if not label_set:
        return False

    # Board must be configured
    if not prefs.get("fqbn"):
        warn("No board configured yet.")
        fqbn = select_board(prefs)
        prefs["fqbn"] = fqbn
        opts = configure_options(fqbn, prefs)
        prefs["options"] = opts
        save_prefs(prefs)
    else:
        fqbn = prefs["fqbn"]
        opts = prefs.get("options", {})

    # --- Pre-compile cross-validation ---
    issues = validate_config_vs_board(prefs)
    has_fatal = False
    for level, msg in issues:
        if level == FATAL:
            error(f"[FATAL] {msg}")
            has_fatal = True
        else:
            warn(f"[WARNING] {msg}")
    if has_fatal:
        error("Cannot compile — fix the fatal issue(s) above first.")
        return False
    if any(lv == WARNING for lv, _ in issues):
        if not confirm("Warnings detected. Continue compiling anyway?", default_yes=True):
            return False

    # Run generate_data.py
    ok = run_generate_data(label_set)
    if not ok:
        if not confirm("generate_data.py had issues. Continue compiling anyway?", default_yes=False):
            return False

    # Compile
    return compile_sketch(fqbn, opts, verbose=verbose)

# =============================================================================
#  Main menu
# =============================================================================
def main():
    os.system("")  # Enable ANSI on Windows

    if not ARDUINO_CLI.exists():
        error(f"arduino-cli not found at: {ARDUINO_CLI}")
        error("Update the ARDUINO_CLI path at the top of this script.")
        sys.exit(1)

    prefs = load_prefs()

    # Load Config.h snapshot — source of truth for all tracked defines
    if CONFIG_H.exists():
        missing = load_config_snapshot()
        if missing:
            warn(f"Config.h is missing {len(missing)} tracked define(s):")
            for m in missing:
                info(f"  {m} (using default: {TRACKED_DEFINES[m]})")
            info(f"Your Config.h may be outdated. Defaults applied in memory.")
            input(f"\n  {DIM}Press Enter to continue...{RESET}")
    else:
        error("Config.h not found! The tool cannot operate without it.")
        sys.exit(1)

    while True:
        # Read live state for display
        active_ls = read_active_label_set() or "(none)"
        transport = read_current_transport()
        role = read_current_role()
        board = prefs.get("fqbn", "(not configured)")
        friendly_board = prefs.get("board_name", board)

        cls()
        print()
        cprint(CYAN + BOLD, "        ____            _          _ _    ___  ____")
        cprint(CYAN + BOLD, "       / ___|___   ___ | | ___ __ (_) |_ / _ \\/ ___|")
        cprint(CYAN + BOLD, "      | |   / _ \\ / __|| |/ / '_ \\| | __| | | \\___ \\")
        cprint(CYAN + BOLD, "      | |__| (_) | (__ |   <| |_) | | |_| |_| |___) |")
        cprint(CYAN + BOLD, "       \\____\\___/ \\___|_|\\_\\ .__/|_|\\__|\\___/|____/")
        cprint(CYAN + BOLD, "                            |_|")
        print()
        if role == "slave":
            print(f"     Label Set: {CYAN}{active_ls}{RESET}  |  "
                  f"Mode: {CYAN}RS485 Slave{RESET}")
        else:
            print(f"     Label Set: {CYAN}{active_ls}{RESET}  |  "
                  f"{CYAN}{role_label(role)}{RESET}  |  "
                  f"Transport: {CYAN}{transport_label(transport)}{RESET}")
        print(f"     Board: {CYAN}{friendly_board}{RESET}")
        print()

        choice = menu_pick([
            ("COMPILE",              "compile",   "action"),
            ("UPLOAD",               "upload",    "action"),
            ("---",),
            ("Transport / Role",     "transport", "normal"),
            ("Board / Board Options","board",     "normal"),
            ("---",),
            ("Show config",          "config",    "dim"),
            ("Restore Config.h",    "restore",   "dim"),
            ("Clean build",          "clean",     "dim"),
            ("Exit",                 "exit",      "dim"),
        ])

        if choice == "exit":
            cprint(DIM, "  Bye!")
            break

        elif choice == "transport":
            configure_transport_and_role(prefs)
            save_prefs(prefs)
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "board":
            fqbn = select_board(prefs)
            prefs["fqbn"] = fqbn
            opts = configure_options(fqbn, prefs)
            prefs["options"] = opts
            save_prefs(prefs)
            success("Board configuration saved.")
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "compile":
            do_compile(prefs)
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "upload":
            if not prefs.get("fqbn"):
                warn("No board configured. Set up Board first.")
                input(f"\n  {DIM}Press Enter to continue...{RESET}")
                continue
            if not BUILD_DIR.exists():
                warn("No build found. Compile first.")
                input(f"\n  {DIM}Press Enter to continue...{RESET}")
                continue
            upload_sketch(prefs["fqbn"], prefs.get("options", {}))
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "config":
            show_config(prefs)
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "restore":
            restore_config()
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "clean":
            clean_build()
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n{DIM}  Interrupted.{RESET}")
        sys.exit(0)
