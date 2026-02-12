"""
CockpitOS — Board management.

Arduino CLI interaction, board discovery, option configuration,
board intelligence (dual-USB detection), and cross-validation.
"""

import subprocess
import sys
import re
import json

from ui import (
    CYAN, DIM, YELLOW, RESET,
    header, info, success, warn, error,
    pick, pick_filterable,
)
from config import (
    read_current_transport, read_current_role,
    transport_label,
)

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
from config import SKETCH_DIR

from pathlib import Path
import os

ARDUINO_CLI = Path(r"C:\Program Files\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe")

# -----------------------------------------------------------------------------
# CockpitOS hard defaults for board options (always applied)
# -----------------------------------------------------------------------------
COCKPITOS_DEFAULTS = {
    # CDCOnBoot is NOT here — its "disabled" value varies per board (S3="default", S2="dis_cdc").
    # It is resolved dynamically via _cdc_disabled_value() in configure_options() and validation.
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
# Cross-validation levels
# -----------------------------------------------------------------------------
FATAL   = "FATAL"
WARNING = "WARNING"

# -----------------------------------------------------------------------------
# Board-aware helpers
# -----------------------------------------------------------------------------
def _cdc_disabled_value(board_options):
    """Find the CDCOnBoot value that means 'Disabled' for this board.

    On S3: "default" = Disabled.  On S2: "dis_cdc" = Disabled.
    Returns the value string, or None if the board has no CDCOnBoot option.
    """
    cdc_opt = board_options.get("CDCOnBoot")
    if not cdc_opt:
        return None
    for label, val in cdc_opt.get("values", []):
        if "disable" in label.lower():
            return val
    return None


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
        error("Update ARDUINO_CLI path in boards.py.")
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


# -----------------------------------------------------------------------------
# Board intelligence
# -----------------------------------------------------------------------------
def board_has_dual_usb(prefs):
    """Check if the selected board has multiple USB modes (S3, P4, etc.)."""
    fqbn = prefs.get("fqbn")
    if not fqbn:
        return False
    board_options = get_board_options(fqbn)
    usb_opt = board_options.get("USBMode")
    if usb_opt and len(usb_opt.get("values", [])) >= 2:
        return True
    return False


def preferred_usb_mode(transport):
    """Return the ideal USBMode value for a given transport."""
    if transport == "usb":
        return "default"      # USB-OTG (TinyUSB)
    return "hwcdc"            # Hardware CDC


def validate_config_vs_board(prefs):
    """Cross-validate Config.h settings against board options."""
    issues = []
    opts = prefs.get("options", {})
    transport = read_current_transport()
    dual_usb = board_has_dual_usb(prefs)

    # FATAL: CDC on Boot must be disabled (value varies per board)
    cdc = opts.get("CDCOnBoot")
    if cdc:
        fqbn = prefs.get("fqbn")
        if fqbn:
            board_options = get_board_options(fqbn)
            cdc_off = _cdc_disabled_value(board_options)
            if cdc_off and cdc != cdc_off:
                issues.append((FATAL,
                    "CDC On Boot is ENABLED. CockpitOS requires CDC On Boot = Disabled.\n"
                    "  Config.h will fail to compile (#error directive). "
                    "Change it in Board Options."))

    # FATAL: USB transport + HW CDC on dual-USB board
    if dual_usb and transport == "usb":
        usb_mode = opts.get("USBMode", "")
        if usb_mode == "hwcdc":
            issues.append((FATAL,
                "USB transport requires USB-OTG (TinyUSB), but USB Mode is set to HW CDC.\n"
                "  The firmware will not enumerate as a USB device. "
                "Change USB Mode in Board Options."))

    # WARNING: non-USB transport + USB-OTG on dual-USB board
    if dual_usb and transport != "usb":
        usb_mode = opts.get("USBMode", "")
        if usb_mode == "default":
            transport_name = transport_label(transport) if transport else "your transport"
            issues.append((WARNING,
                f"USB Mode is set to USB-OTG (TinyUSB), but transport is {transport_name}.\n"
                "  HW CDC is lighter and allows Serial debugging when not using USB transport.\n"
                "  Consider switching USB Mode to HW CDC in Board Options."))

    return issues


# -----------------------------------------------------------------------------
# Board selection & option configuration
# -----------------------------------------------------------------------------
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

    options = [(name, fqbn) for name, fqbn in boards]

    header("Board Selection")
    if saved_fqbn:
        saved_name = next((n for n, f in boards if f == saved_fqbn), saved_fqbn)
        info(f"Current: {CYAN}{saved_name}{RESET}  {DIM}({saved_fqbn}){RESET}")

    fqbn = pick_filterable(f"Select board ({len(boards)} available):", options, default=saved_fqbn)
    if fqbn is None:
        return None

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

    # CDC On Boot: always disabled (value varies per board)
    cdc_off = _cdc_disabled_value(board_options)
    if cdc_off is not None:
        selected["CDCOnBoot"] = cdc_off

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
    if mode is None:
        return None

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
        if chosen is None:
            return None
        selected[key] = chosen

    # Fill remaining with defaults
    for key, opt in board_options.items():
        if key not in selected:
            selected[key] = saved_options.get(key, opt["default"])

    return selected
