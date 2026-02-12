"""
CockpitOS — Config.h engine.

Owns the tracked variable registry, all Config.h read/write operations,
file safety (backup/restore/atomic writes), transport/role logic,
and board intelligence (cross-validation).
"""

import os
import re
import time
import shutil
from pathlib import Path

from ui import (
    CYAN, DIM, RESET, YELLOW,
    header, info, success, warn, error,
    pick, confirm,
)

# -----------------------------------------------------------------------------
# Paths (shared with other modules via import)
# -----------------------------------------------------------------------------
SKETCH_DIR      = Path(__file__).resolve().parent.parent          # CockpitOS root
CONFIG_H        = SKETCH_DIR / "Config.h"
BACKUP_DIR      = SKETCH_DIR / "compiler" / "backups"
CREDENTIALS_DIR = SKETCH_DIR / ".credentials"
WIFI_H          = CREDENTIALS_DIR / "wifi.h"

# -----------------------------------------------------------------------------
# File Safety — whitelist, backup, and atomic write
# -----------------------------------------------------------------------------
ALLOWED_FILES = {
    "Config.h":  SKETCH_DIR / "Config.h",
    "wifi.h":    WIFI_H,
}

MAX_BACKUPS = 10
_session_backed_up = set()


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
    backups = sorted(
        [f for f in BACKUP_DIR.iterdir() if f.name.startswith(filename_prefix) and f.suffix == ".bak"],
        key=lambda f: f.stat().st_mtime,
        reverse=True,
    )
    for old in backups[MAX_BACKUPS:]:
        try:
            old.unlink()
        except OSError:
            pass


def backup_file(path):
    """Create a timestamped backup of a file. Only backs up once per session."""
    resolved = Path(path).resolve()

    if not _is_allowed_file(resolved):
        return None
    if not resolved.exists():
        return None
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
    """Atomic write: write to .tmp, then rename over the target."""
    resolved = Path(path).resolve()

    if not _is_allowed_file(resolved):
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
        return False


def list_backups(filename="Config.h"):
    """List available backups for a file, newest first."""
    if not BACKUP_DIR.exists():
        return []

    prefix = f"{filename}."
    backups = []
    for f in sorted(BACKUP_DIR.iterdir(), key=lambda f: f.stat().st_mtime, reverse=True):
        if f.name.startswith(prefix) and f.suffix == ".bak":
            parts = f.stem
            ts_part = parts.split(".")[-1]
            try:
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
# Tracked Variable Registry
# -----------------------------------------------------------------------------
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
    # Debug
    "DEBUG_ENABLED":                    "0",
    "VERBOSE_MODE":                     "0",
    "VERBOSE_MODE_WIFI_ONLY":           "0",
    "VERBOSE_MODE_SERIAL_ONLY":         "0",
    "DEBUG_PERFORMANCE":                "0",
}

TRANSPORT_DEFINES = [
    "USE_DCSBIOS_SERIAL", "USE_DCSBIOS_USB", "USE_DCSBIOS_WIFI",
    "USE_DCSBIOS_BLUETOOTH", "RS485_SLAVE_ENABLED",
]

TRANSPORT_KEY = {
    "serial": "USE_DCSBIOS_SERIAL",
    "usb":    "USE_DCSBIOS_USB",
    "wifi":   "USE_DCSBIOS_WIFI",
    "ble":    "USE_DCSBIOS_BLUETOOTH",
    "slave":  "RS485_SLAVE_ENABLED",
}

TRANSPORTS = [
    ("WiFi",                      "wifi"),
    ("USB Native",                "usb"),
    ("Serial (CDC/Socat)",        "serial"),
    ("RS485 Slave",               "slave"),
    ("Bluetooth BLE",             "ble"),
]

_config_snapshot = {}


# -----------------------------------------------------------------------------
# Low-level Config.h read/write
# -----------------------------------------------------------------------------
def read_config_define(name):
    """Read a #define value from Config.h."""
    if not CONFIG_H.exists():
        return None
    text = CONFIG_H.read_text(encoding="utf-8")
    m = re.search(rf'#define\s+{re.escape(name)}\s+(\S+)', text)
    if m:
        return m.group(1)
    return None


def write_config_define(name, value):
    """Update a #define in Config.h, preserving formatting and comments."""
    if not CONFIG_H.exists():
        error("Config.h not found!")
        return False

    backup_file(CONFIG_H)

    text = CONFIG_H.read_text(encoding="utf-8")
    pattern = rf'(#define\s+{re.escape(name)}\s+)\S+(.*)'
    new_text, count = re.subn(pattern, rf'\g<1>{value}\2', text)

    if count == 0:
        warn(f"Could not find #define {name} in Config.h")
        return False

    if not safe_write_file(CONFIG_H, new_text):
        error(f"Failed to write Config.h! Check for Config.h.tmp in project root.")
        return False

    return True


# -----------------------------------------------------------------------------
# Snapshot: in-memory cache of Config.h tracked defines
# -----------------------------------------------------------------------------
def load_config_snapshot():
    """Read all TRACKED_DEFINES from Config.h into the in-memory snapshot."""
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
    """Read a tracked define from the in-memory snapshot."""
    if _config_snapshot and name in _config_snapshot:
        return _config_snapshot[name]
    return read_config_define(name)


def config_set(name, value):
    """Write a tracked define to Config.h AND update the snapshot."""
    value = str(value)
    if write_config_define(name, value):
        _config_snapshot[name] = value
        return True
    return False


# -----------------------------------------------------------------------------
# Transport & Role queries
# -----------------------------------------------------------------------------
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


# -----------------------------------------------------------------------------
# Restore Config.h from backup
# -----------------------------------------------------------------------------
def restore_config():
    """Interactive restore of Config.h from a backup."""
    header("Restore Config.h")

    backups = list_backups("Config.h")
    if not backups:
        warn("No backups found.")
        info(f"Backups are created automatically when the tool modifies Config.h.")
        return False

    info(f"Found {len(backups)} backup(s):\n")

    options = []
    for bak_path, ts_display, size in backups:
        label = f"{ts_display}  ({size:,} bytes)"
        options.append((label, str(bak_path)))

    chosen = pick("Select backup to restore:", options)
    if not chosen:
        return False

    chosen_path = Path(chosen)

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


# -----------------------------------------------------------------------------
# Transport & Role configuration wizard
# -----------------------------------------------------------------------------
def configure_transport_and_role(prefs, board_has_dual_usb_fn, preferred_usb_mode_fn):
    """Interactive Config.h transport and role configuration.

    board_has_dual_usb_fn and preferred_usb_mode_fn are passed in to avoid
    circular imports with boards.py.
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
    if is_master is None:
        return None

    # --- Step 2: Choose Transport ---
    if is_master == "yes":
        header("Choose Transport (WiFi / USB / Socat)")
        info(f"{DIM}RS485 Master requires a real transport to forward data.{RESET}")
        transport_opts = [(label, key) for label, key in TRANSPORTS if key != "slave"]
        prev = current_transport if current_transport != "slave" else "usb"
    else:
        header("Choose Transport (WiFi / USB / Socat / RS485 Slave)")
        transport_opts = list(TRANSPORTS)
        prev = current_transport or "usb"

    transport = pick("Select transport:", transport_opts, default=prev)
    if transport is None:
        return None

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

    old_m = config_get("RS485_MASTER_ENABLED")
    if old_m != str(master_val):
        config_set("RS485_MASTER_ENABLED", str(master_val))
        changes.append(f"RS485_MASTER_ENABLED: {old_m} -> {master_val}")

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
    if board_has_dual_usb_fn(prefs):
        ideal = preferred_usb_mode_fn(transport)
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
    from boards import validate_config_vs_board, FATAL
    issues = validate_config_vs_board(prefs)
    for level, msg in issues:
        if level == FATAL:
            error(f"[FATAL] {msg}")
        else:
            warn(f"[WARNING] {msg}")

    return prefs["transport"], mode
