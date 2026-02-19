"""
CockpitOS Label Creator -- CustomPins.h TUI editor.

Scans InputMapping.h and LEDMapping.h to detect which hardware devices
are in use, then presents a grouped, guided pin configuration screen
with contextual warnings when required pins are missing.

Known groups only (v1): Feature Enables, I2C, HC165, WS2812, TM1637,
Mode Switch, RS485.  Unknown #defines are preserved verbatim.
"""

import os
import re
import sys
import math
import msvcrt
import threading

import ui

# ---------------------------------------------------------------------------
# ANSI constants (mirror ui.py)
# ---------------------------------------------------------------------------
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
_SEL_BG  = "\033[48;5;236m"
WARN_BG  = "\033[43m\033[30m"   # yellow bg + black text


def _w(text):
    sys.stdout.write(text)
    sys.stdout.flush()


def _get_terminal_size():
    try:
        size = os.get_terminal_size()
        return size.columns, size.lines
    except Exception:
        return 120, 30


# ---------------------------------------------------------------------------
# Regexes — reused from latched_editor / led_editor
# ---------------------------------------------------------------------------
_INPUT_LINE_RE = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*'
    r'"(?P<source>[^"]+)"\s*,\s*'
    r'(?P<port>-?\d+|PIN\(\d+\)|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*'
    r'(?P<bit>-?\d+)\s*,\s*'
    r'(?P<hidId>-?\d+)\s*,\s*'
    r'"(?P<cmd>[^"]+)"\s*,\s*'
    r'(?P<value>-?\d+)\s*,\s*'
    r'"(?P<type>[^"]+)"\s*,\s*'
    r'(?P<group>\d+)\s*\}\s*,'
)

_LED_ENTRY_RE = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*DEVICE_(?P<device>[A-Z0-9_]+)\s*,'
    r'\s*\{\s*\.(?P<info_type>[a-zA-Z0-9_]+)\s*=\s*\{\s*(?P<info_values>[^}]+)\}\s*\}'
    r'\s*,\s*(?P<dimmable>true|false)\s*,\s*(?P<activeLow>true|false)\s*\}',
    re.MULTILINE
)

# CustomPins.h: active define
_DEFINE_RE = re.compile(
    r'^#define\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s+(?P<value>\S+(?:\s*\([^)]*\))?)'
    r'(?:\s*//\s*(?P<comment>.*))?$'
)

# CustomPins.h: commented-out define
_COMMENTED_DEFINE_RE = re.compile(
    r'^//\s*#define\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s+(?P<value>\S+(?:\s*\([^)]*\))?)'
    r'(?:\s*//\s*(?P<comment>.*))?$'
)

# Value validation: bare integer or -1 (PIN() macro is NOT used by the editor)
_PIN_VALUE_RE = re.compile(r'^-?\d+$')


# ═══════════════════════════════════════════════════════════════════════════
# PIN GROUP DEFINITIONS — the intelligence layer
# ═══════════════════════════════════════════════════════════════════════════

PIN_GROUPS = [
    {
        "name": "Feature Enables",
        "required_when": [],          # always shown
        "pins": [
            {"define": "ENABLE_TFT_GAUGES", "type": "bool",
             "help": "Enable TFT gauge rendering (only if TFT displays are present)",
             "default": "0"},
            {"define": "ENABLE_PCA9555", "type": "bool",
             "help": "Enable PCA9555 I/O expander logic (only if PCA chips are present)",
             "default": "0"},
        ],
    },
    {
        "name": "I2C Bus",
        "required_when": ["PCA9555"],
        "pins": [
            {"define": "SDA_PIN", "help": "I2C data line for PCA9555 expanders",
             "default": "8"},
            {"define": "SCL_PIN", "help": "I2C clock line for PCA9555 expanders",
             "default": "9"},
        ],
    },
    {
        "name": "HC165 Shift Register",
        "required_when": ["HC165"],
        "pins": [
            {"define": "HC165_BITS", "type": "int",
             "help": "Total input bits across all chained chips (8 per chip, 0 = disabled)",
             "default": "16"},
            {"define": "HC165_CONTROLLER_PL",
             "help": "Latch pin (PL / SH/LD)",
             "default": "39"},
            {"define": "HC165_CONTROLLER_CP",
             "help": "Clock pin (CP / CLK)",
             "default": "38"},
            {"define": "HC165_CONTROLLER_QH",
             "help": "Data output pin (QH)",
             "default": "40"},
        ],
    },
    {
        "name": "WS2812 Addressable LEDs",
        "required_when": ["WS2812"],
        "pins": [
            {"define": "WS2812B_PIN",
             "help": "Data line for WS2812B LED strip",
             "default": "35"},
        ],
    },
    {
        "name": "TM1637 7-Segment Displays",
        "required_when": ["TM1637"],
        "pins": [
            {"define": "LA_DIO_PIN",  "help": "Left Angle display data",    "default": "39"},
            {"define": "LA_CLK_PIN",  "help": "Left Angle display clock",   "default": "37"},
            {"define": "RA_DIO_PIN",  "help": "Right Angle display data",   "default": "40"},
            {"define": "RA_CLK_PIN",  "help": "Right Angle display clock",  "default": "37"},
            {"define": "CA_DIO_PIN",  "help": "Caution Advisory display data",  "default": "36"},
            {"define": "CA_CLK_PIN",  "help": "Caution Advisory display clock", "default": "37"},
            {"define": "JETT_DIO_PIN","help": "Jettison Select display data",   "default": "8"},
            {"define": "JETT_CLK_PIN","help": "Jettison Select display clock",  "default": "9"},
        ],
    },
    {
        "name": "Mode Switch",
        "required_when": [],          # optional, never required
        "pins": [
            {"define": "MODE_SWITCH_PIN",
             "help": "HID mode selector switch (rotary or toggle)",
             "default": "33"},
        ],
    },
    {
        "name": "RS485 Communication",
        "required_when": ["RS485"],
        "pins": [
            {"define": "RS485_TX_PIN", "type": "int",
             "help": "GPIO for TX (connect to RS485 board DI)",
             "default": "17"},
            {"define": "RS485_RX_PIN", "type": "int",
             "help": "GPIO for RX (connect to RS485 board RO)",
             "default": "18"},
            {"define": "RS485_DE_PIN", "type": "int",
             "help": "Direction enable pin (-1 if auto-direction)",
             "default": "-1"},
        ],
    },
    {
        "name": "RS485 Test Pins",
        "required_when": ["RS485"],
        "pins": [
            {"define": "RS485_TEST_BUTTON_GPIO", "type": "int",
             "help": "Test button GPIO (-1 to disable)", "default": "-1"},
            {"define": "RS485_TEST_SWITCH_GPIO", "type": "int",
             "help": "Test switch GPIO (-1 to disable)", "default": "-1"},
            {"define": "RS485_TEST_LED_GPIO", "type": "int",
             "help": "Test LED GPIO (-1 to disable)", "default": "-1"},
        ],
    },
]

# Build a flat lookup: define_name -> (group_name, pin_def)
_KNOWN_DEFINES = {}
for _g in PIN_GROUPS:
    for _p in _g["pins"]:
        _KNOWN_DEFINES[_p["define"]] = (_g["name"], _p)


# ═══════════════════════════════════════════════════════════════════════════
# DEVICE DETECTION — scan InputMapping.h + LEDMapping.h
# ═══════════════════════════════════════════════════════════════════════════

def detect_devices(input_filepath, led_filepath):
    """Scan mapping files and return a dict of detected device info.

    Keys present only if device is detected.  Values carry extra detail
    (addresses, max bit, pin values, etc.) when available.
    """
    detected = {}

    # --- InputMapping.h ---------------------------------------------------
    if os.path.exists(input_filepath):
        with open(input_filepath, "r", encoding="utf-8") as f:
            lines = f.readlines()
        pca_addrs = set()
        hc165_max_bit = -1
        tm1637_dios = set()
        for line in lines:
            m = _INPUT_LINE_RE.search(line)
            if not m:
                continue
            d = m.groupdict()
            src = d["source"]
            if src.startswith("PCA_"):
                pca_addrs.add(src.replace("PCA_", ""))
            elif src == "HC165":
                bit = int(d["bit"])
                if bit > hc165_max_bit:
                    hc165_max_bit = bit
            elif src == "TM1637":
                # port field holds the DIO pin for TM1637 inputs
                tm1637_dios.add(d["port"])
            elif src == "MATRIX":
                detected["MATRIX"] = True

        if pca_addrs:
            detected["PCA9555"] = {"addresses": sorted(pca_addrs)}
        if hc165_max_bit >= 0:
            suggested = int(math.ceil((hc165_max_bit + 1) / 8.0)) * 8
            detected["HC165"] = {"max_bit": hc165_max_bit,
                                  "suggested_bits": suggested}
        if tm1637_dios:
            detected.setdefault("TM1637", {})["input_dios"] = sorted(tm1637_dios)

    # --- LEDMapping.h -----------------------------------------------------
    if os.path.exists(led_filepath):
        with open(led_filepath, "r", encoding="utf-8") as f:
            text = f.read()
        ws2812_pins = set()
        tm_devices = set()  # (clk, dio) tuples
        for m in _LED_ENTRY_RE.finditer(text):
            dev = m.group("device")
            vals = [v.strip() for v in m.group("info_values").split(",")]
            if dev == "PCA9555":
                # address is first value
                if vals:
                    detected.setdefault("PCA9555", {"addresses": []})
                    addr = vals[0]
                    if addr.startswith("0x") or addr.startswith("0X"):
                        addrs = detected["PCA9555"].setdefault("addresses", [])
                        if addr not in addrs:
                            addrs.append(addr)
                    else:
                        detected["PCA9555"]["present"] = True
            elif dev == "TM1637" and len(vals) >= 2:
                tm_devices.add((vals[0], vals[1]))
                detected.setdefault("TM1637", {})["led_devices"] = sorted(
                    tm_devices, key=lambda x: x[1])
            elif dev == "GN1640T":
                detected["GN1640T"] = True
            elif dev == "WS2812" and len(vals) >= 2:
                ws2812_pins.add(vals[1])
            elif dev == "GAUGE":
                detected.setdefault("GAUGE", {}).setdefault("count", 0)
                detected["GAUGE"]["count"] += 1
        if ws2812_pins:
            detected["WS2812"] = {"pins": sorted(ws2812_pins)}
        if tm_devices:
            detected.setdefault("TM1637", {})["led_devices"] = sorted(
                tm_devices, key=lambda x: x[1])

    return detected


# ═══════════════════════════════════════════════════════════════════════════
# CUSTOM PINS PARSER
# ═══════════════════════════════════════════════════════════════════════════

def parse_custom_pins(filepath):
    """Parse CustomPins.h into (known_pins, unknown_lines).

    known_pins: dict { define_name: value_str } for recognized defines.
    unknown_lines: list of raw lines that don't match any known define
                   (preserved verbatim on write).
    """
    known_pins = {}
    unknown_lines = []

    if not os.path.exists(filepath):
        return known_pins, unknown_lines

    with open(filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()

    for line in lines:
        raw = line.rstrip("\n")

        # Try active define
        m = _DEFINE_RE.match(raw.strip())
        if m:
            name = m.group("name")
            val = m.group("value").strip()
            if name in _KNOWN_DEFINES:
                known_pins[name] = val
            else:
                unknown_lines.append(raw)
            continue

        # Try commented define
        mc = _COMMENTED_DEFINE_RE.match(raw.strip())
        if mc:
            name = mc.group("name")
            if name in _KNOWN_DEFINES:
                # Commented = not set, skip (don't add to known_pins)
                pass
            else:
                unknown_lines.append(raw)
            continue

        # Everything else: comments, blank lines, #pragma, #undef, etc.
        unknown_lines.append(raw)

    return known_pins, unknown_lines


def count_pins(filepath):
    """Return count of active (non-commented) #define pins in CustomPins.h."""
    if not os.path.exists(filepath):
        return 0
    known, _ = parse_custom_pins(filepath)
    return len(known)


# ═══════════════════════════════════════════════════════════════════════════
# CUSTOM PINS WRITER
# ═══════════════════════════════════════════════════════════════════════════

def write_custom_pins(filepath, known_pins, unknown_lines, label_set_name=""):
    """Write CustomPins.h from known pin values + preserved unknown lines.

    Groups are written in PIN_GROUPS order.  Only groups that have at
    least one defined pin (or are Feature Enables) are emitted.
    Unknown lines are appended at the bottom.
    """
    out = []
    ls_label = label_set_name or "Label Set"
    out.append(f"// CustomPins.h - Pin configuration for {ls_label}")
    out.append("//")
    out.append("// Managed by the Label Creator Custom Pins editor.")
    out.append("// Unknown defines are preserved at the bottom of this file.")
    out.append("")
    out.append("#pragma once")
    out.append("")

    emitted_defines = set()

    for group in PIN_GROUPS:
        # Collect pins in this group that are defined
        group_pins = []
        for pdef in group["pins"]:
            name = pdef["define"]
            if name in known_pins:
                group_pins.append((name, known_pins[name], pdef))

        # Always emit Feature Enables; skip empty groups otherwise
        if group["name"] != "Feature Enables" and not group_pins:
            continue

        out.append(f"// --- {group['name']} " + "-" * max(1, 60 - len(group['name']) - 7))

        for pdef in group["pins"]:
            name = pdef["define"]
            if name in known_pins:
                val = known_pins[name]
                comment = pdef.get("help", "")
                if comment:
                    out.append(f"#define {name:<40s} {val:<14s} // {comment}")
                else:
                    out.append(f"#define {name:<40s} {val}")
                emitted_defines.add(name)
            elif group["name"] == "Feature Enables":
                # Feature enables always get a default line
                val = pdef.get("default", "0")
                comment = pdef.get("help", "")
                out.append(f"#define {name:<40s} {val:<14s} // {comment}")
                emitted_defines.add(name)

        out.append("")

    # Emit any known pins that weren't in groups (shouldn't happen but safety)
    extras = []
    for name, val in known_pins.items():
        if name not in emitted_defines:
            extras.append(f"#define {name:<40s} {val}")
    if extras:
        out.append("// --- Other Pins " + "-" * 43)
        out.extend(extras)
        out.append("")

    # Emit unknown lines (skip leading blanks / pragma / header comments
    # from the original file since we already wrote our own header)
    filtered_unknown = _filter_unknown_lines(unknown_lines)
    if filtered_unknown:
        out.append("// --- Preserved (not managed by editor) " + "-" * 20)
        out.extend(filtered_unknown)
        out.append("")

    with open(filepath, "w", encoding="utf-8") as f:
        f.write("\n".join(out))


def _filter_unknown_lines(unknown_lines):
    """Remove our own header boilerplate and #pragma from unknown lines.

    Preserves all other content (user comments, unknown #defines, #undef, etc.)
    """
    # Patterns we KNOW are boilerplate (our header or the template header)
    _BOILERPLATE = (
        "#pragma once",
        "// CustomPins.h",
        "// Pin configuration",
        "// Configuration PINS",
        "// Managed by the Label Creator",
        "// Unknown defines are preserved",
        "// This file is automatically",
        "// Uncomment and set",
        "// The PIN(X) macro",     # legacy template line
        "// Use GPIO number",
        "// --- Preserved (not managed",
    )

    result = []
    for line in unknown_lines:
        stripped = line.strip()
        if any(stripped.startswith(bp) for bp in _BOILERPLATE):
            continue
        # Skip decoration-only lines (solid bars of = or -)
        if stripped and all(c in "/= -" for c in stripped):
            continue
        # Skip common section headers from hand-edited files
        if re.match(r'^//\s*---\s.*---\s*$', stripped):
            continue
        if re.match(r'^//\s*(I2C Pins?|HID Switch.*|Main Panel Pins?|'
                    r'Feature Enables|REMOVE FOR PRODUCTION|'
                    r'Auto-Direction Control)\s*$', stripped):
            continue
        result.append(line)

    # Strip leading/trailing blank lines
    while result and not result[0].strip():
        result.pop(0)
    while result and not result[-1].strip():
        result.pop()

    # Collapse consecutive blank lines to a single blank
    collapsed = []
    prev_blank = False
    for line in result:
        is_blank = not line.strip()
        if is_blank and prev_blank:
            continue
        collapsed.append(line)
        prev_blank = is_blank

    return collapsed


# ═══════════════════════════════════════════════════════════════════════════
# BUILD DISPLAY ROWS — flattened list for the TUI
# ═══════════════════════════════════════════════════════════════════════════

# Row types
ROW_GROUP_HEADER = "header"
ROW_PIN          = "pin"
ROW_WARNING      = "warning"
ROW_SPACER       = "spacer"


def _build_rows(known_pins, detected):
    """Build the flat row list for the TUI.

    Each row is a dict with at least:
      type:  ROW_GROUP_HEADER | ROW_PIN | ROW_WARNING
      group: group name
    PIN rows also have: define, value, help, pdef
    WARNING rows also have: message
    HEADER rows also have: detected (bool), required (bool)
    """
    rows = []
    first_group = True

    for group in PIN_GROUPS:
        grp_name = group["name"]

        # Spacer between groups (not before the first one)
        if not first_group:
            rows.append({"type": ROW_SPACER, "group": grp_name})
        first_group = False
        required_devs = group.get("required_when", [])

        # Is any required device detected?
        grp_detected = any(d in detected for d in required_devs)
        grp_required = grp_detected and len(required_devs) > 0

        # Does this group have any defined pins?
        has_pins = any(p["define"] in known_pins for p in group["pins"])

        # All groups are always shown so users can configure any device

        # Detect status string for header
        if grp_required:
            det_names = [d for d in required_devs if d in detected]
            status = f"required: {', '.join(det_names)} detected"
        elif grp_detected:
            det_names = [d for d in required_devs if d in detected]
            status = f"{', '.join(det_names)} detected"
        elif has_pins:
            status = "configured"
        else:
            status = ""

        rows.append({
            "type": ROW_GROUP_HEADER,
            "group": grp_name,
            "detected": grp_detected,
            "required": grp_required,
            "status": status,
        })

        # Pin rows (with per-pin warnings for required groups)
        for pdef in group["pins"]:
            name = pdef["define"]
            val = known_pins.get(name)

            # For Feature Enables, auto-detect hint
            hint = ""
            if pdef.get("type") == "bool" and grp_name == "Feature Enables":
                auto_key = pdef.get("auto_detect", "")
                if auto_key == "PCA9555":
                    if "PCA9555" in detected:
                        hint = "PCA devices detected" if val == "1" else "PCA detected, should be 1"
                    else:
                        hint = "no PCA devices detected"
                elif auto_key == "TFT":
                    hint = ""  # deferred

            rows.append({
                "type": ROW_PIN,
                "group": grp_name,
                "define": name,
                "value": val,          # None if not set
                "help": pdef.get("help", ""),
                "hint": hint,
                "pdef": pdef,
            })

            # Per-pin warning: required group has device detected but this pin is missing
            if grp_required and val is None and pdef.get("type") != "bool":
                det_str = ", ".join(d for d in required_devs if d in detected)
                rows.append({
                    "type": ROW_WARNING,
                    "group": grp_name,
                    "message": f"{name} required ({det_str} detected)",
                })

    return rows


def _count_defined(known_pins):
    """Count defined pins (excluding feature enables)."""
    count = 0
    for name in known_pins:
        if name not in ("ENABLE_TFT_GAUGES", "ENABLE_PCA9555"):
            count += 1
    return count


# ═══════════════════════════════════════════════════════════════════════════
# MAIN TUI EDITOR
# ═══════════════════════════════════════════════════════════════════════════

def edit_custom_pins(filepath, input_filepath, led_filepath,
                     display_filepath=None,
                     label_set_name="", aircraft_name=""):
    """Main entry point: scrollable grouped pin editor."""

    # -- Detect devices from mapping files --------------------------------
    detected = detect_devices(input_filepath, led_filepath)

    # -- Parse existing CustomPins.h --------------------------------------
    if not os.path.exists(filepath):
        ui.header("Custom Pins")
        print()
        ui.warn("CustomPins.h not found in this label set.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    known_pins, unknown_lines = parse_custom_pins(filepath)

    # Detect RS485 from label set name (transport mode, not from mappings)
    if label_set_name and "RS485" in label_set_name.upper():
        detected["RS485"] = True

    # Ensure Feature Enables always present with defaults
    if "ENABLE_TFT_GAUGES" not in known_pins:
        known_pins["ENABLE_TFT_GAUGES"] = "0"
    if "ENABLE_PCA9555" not in known_pins:
        known_pins["ENABLE_PCA9555"] = "0"

    # -- Take a snapshot for dirty detection ------------------------------
    original_pins = dict(known_pins)

    # -- Build row list ---------------------------------------------------
    rows = _build_rows(known_pins, detected)

    # -- Selectable row indices (only PIN rows are selectable) ------------
    selectable = [i for i, r in enumerate(rows) if r["type"] == ROW_PIN]
    if not selectable:
        ui.header("Custom Pins")
        print()
        ui.info("No configurable pin groups for this label set.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    sel_idx = 0      # index into selectable[]
    scroll = 0

    cols, term_rows = _get_terminal_size()
    list_height = term_rows - 7
    if list_height < 5:
        list_height = 5

    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"

    _flash_timer  = [None]
    _flash_active = [False]

    def _total_rows():
        return len(rows)

    def _needs_scroll():
        return _total_rows() > list_height

    def _rebuild():
        nonlocal rows, selectable
        rows = _build_rows(known_pins, detected)
        selectable = [i for i, r in enumerate(rows) if r["type"] == ROW_PIN]

    def _clamp_sel():
        nonlocal sel_idx
        if not selectable:
            sel_idx = 0
            return
        if sel_idx < 0:
            sel_idx = 0
        if sel_idx >= len(selectable):
            sel_idx = len(selectable) - 1

    def _clamp_scroll():
        nonlocal scroll
        if not selectable:
            return
        row_i = selectable[sel_idx]
        if row_i < scroll:
            scroll = row_i
        if row_i >= scroll + list_height:
            scroll = row_i - list_height + 1
        # Also keep scroll in bounds
        max_scroll = max(0, _total_rows() - list_height)
        if scroll > max_scroll:
            scroll = max_scroll
        if scroll < 0:
            scroll = 0

    def _scroll_bar_positions():
        total = _total_rows()
        if total <= list_height:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / total))
        max_s = total - list_height
        if max_s <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_s)
        return (top, top + thumb)

    def _flash_row():
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _flash_active[0] = True
        _draw()
        def _restore():
            _flash_active[0] = False
            _draw()
        _flash_timer[0] = threading.Timer(0.15, _restore)
        _flash_timer[0].daemon = True
        _flash_timer[0].start()

    # -- Column layout ----------------------------------------------------
    # Every row: prefix(3) + body(body_w) + scrollbar(1) = cols
    # prefix = "   " or " > " (always 3 visible chars)
    body_w = max(cols - 4, 30)       # 3 prefix + 1 scrollbar = 4 overhead
    header_w = cols - 2              # "  " indent + "=" * header_w  fits in cols
    name_w = min(40, body_w - 16)    # leave room for val + hint
    val_w = 14

    def _render_row(ri, is_highlighted, scroll_char):
        row = rows[ri]

        if row["type"] == ROW_SPACER:
            return f"{' ' * (cols - 1)}{DIM}{scroll_char}{RESET}"

        if row["type"] == ROW_GROUP_HEADER:
            grp = row["group"]
            status = row.get("status", "")
            label = f"--- {grp} "
            if status:
                label += f"({status}) "
            # pad with dashes to fill body_w, then scrollbar
            pad = max(1, body_w - len(label) + 1)
            return (f"  {CYAN}{BOLD}{label}{'-' * pad}{RESET}"
                    f"{DIM}{scroll_char}{RESET}")

        if row["type"] == ROW_WARNING:
            msg = row["message"]
            inner = f" ! {msg}"[:body_w + 1]
            inner = f"{inner:<{body_w + 1}}"
            return (f"  {WARN_BG}{inner}{RESET}"
                    f"{DIM}{scroll_char}{RESET}")

        # ROW_PIN
        name = row["define"]
        val = row["value"]
        hint = row.get("hint", "")

        val_plain = val if val is not None else "(not set)"
        name_part = f"{name:<{name_w}}"
        val_part = f"{val_plain:<{val_w}}"
        hint_room = max(0, body_w - name_w - val_w)
        hint_part = f"{hint[:hint_room]:<{hint_room}}"

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG} {YELLOW}> "
                    f"{name_part}{val_part}{hint_part}"
                    f"{RESET}{DIM}{scroll_char}{RESET}")
        elif is_highlighted:
            nc = GREEN if val is not None else DIM
            vc = GREEN if val is not None else DIM
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} "
                    f"{nc}{name_part}{RESET}{_SEL_BG}"
                    f"{vc}{val_part}{RESET}{_SEL_BG}"
                    f"{DIM}{hint_part}{RESET}{_SEL_BG}"
                    f"{DIM}{scroll_char}{RESET}")
        else:
            color = RESET if val is not None else DIM
            return (f"   "
                    f"{color}{name_part}{val_part}{RESET}"
                    f"{DIM}{hint_part}{RESET}"
                    f"{DIM}{scroll_char}{RESET}")

    def _draw():
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        # Header
        parts = [label_set_name, aircraft_name]
        ctx = "  (" + ", ".join(p for p in parts if p) + ")" if any(parts) else ""
        n_pins = _count_defined(known_pins)
        title = f"Custom Pins{ctx}"
        counter = f"{n_pins} pin{'s' if n_pins != 1 else ''} defined"
        spacing = header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Rows
        thumb_start, thumb_end = _scroll_bar_positions()

        cur_row = selectable[sel_idx] if selectable else -1

        for vi in range(list_height):
            ri = scroll + vi
            sc = " "
            if _needs_scroll():
                sc = _SCROLL_BLOCK if thumb_start <= vi < thumb_end else _SCROLL_LIGHT
            if ri < _total_rows():
                is_hl = (ri == cur_row)
                _w(_render_row(ri, is_hl, sc) + "\n")
            else:
                _w(f"{'':>{cols - 1}}{DIM}{sc}{RESET}\n")

        _w(f"\n  {DIM}Enter=edit  D=clear pin  Esc=save & exit{RESET}")

    # -- Detect string for initial banner ----------------------------------
    if detected:
        det_summary = ", ".join(sorted(detected.keys()))
    else:
        det_summary = "none"

    _clamp_sel()
    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "H":          # Up
                    if sel_idx > 0:
                        sel_idx -= 1
                    else:
                        _flash_row()
                        continue
                    _clamp_scroll()
                    _draw()
                elif ch2 == "P":        # Down
                    if sel_idx < len(selectable) - 1:
                        sel_idx += 1
                    else:
                        _flash_row()
                        continue
                    _clamp_scroll()
                    _draw()
                elif ch2 == "I":        # Page Up
                    sel_idx = max(0, sel_idx - list_height)
                    _clamp_scroll()
                    _draw()
                elif ch2 == "Q":        # Page Down
                    sel_idx = min(len(selectable) - 1, sel_idx + list_height)
                    _clamp_scroll()
                    _draw()

            elif ch == "\r" or ch == " ":     # Enter/Space = edit pin value
                if not selectable:
                    continue
                ri = selectable[sel_idx]
                row = rows[ri]
                _edit_pin_value(row, known_pins, detected)
                _rebuild()
                _clamp_sel()
                _clamp_scroll()
                _draw()

            elif ch in ("d", "D", "\x04"):    # D/Ctrl+D = clear pin
                if not selectable:
                    continue
                ri = selectable[sel_idx]
                row = rows[ri]
                name = row["define"]
                if name in known_pins:
                    # Feature enables reset to default, not removed
                    if name in ("ENABLE_TFT_GAUGES", "ENABLE_PCA9555"):
                        known_pins[name] = "0"
                    else:
                        del known_pins[name]
                    _rebuild()
                    _clamp_sel()
                    _clamp_scroll()
                _draw()

            elif ch == "\x1b":          # Esc = save and exit
                break

    finally:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)

    # Save if changed
    if known_pins != original_pins:
        ls_short = label_set_name
        if ls_short.startswith("LABEL_SET_"):
            ls_short = ls_short[len("LABEL_SET_"):]
        write_custom_pins(filepath, known_pins, unknown_lines, ls_short or "Label Set")


# ═══════════════════════════════════════════════════════════════════════════
# PIN VALUE EDITOR (modal)
# ═══════════════════════════════════════════════════════════════════════════

def _edit_pin_value(row, known_pins, detected):
    """Modal edit for a single pin value. Modifies known_pins in place."""
    _w(SHOW_CUR)
    name = row["define"]
    pdef = row["pdef"]
    current = known_pins.get(name)
    help_text = pdef.get("help", "")
    default = pdef.get("default", "")
    pin_type = pdef.get("type", "pin")

    print()
    print()
    print(f"  {CYAN}{BOLD}{name}{RESET}")
    if help_text:
        print(f"  {DIM}{help_text}{RESET}")
    print()

    if current is not None:
        print(f"  Current: {GREEN}{current}{RESET}")
    else:
        print(f"  Current: {DIM}(not set){RESET}")

    if default:
        print(f"  Default: {DIM}{default}{RESET}")

    # Auto-detect suggestions
    if name == "HC165_BITS" and "HC165" in detected:
        suggested = detected["HC165"].get("suggested_bits")
        if suggested:
            print(f"  Suggested: {GREEN}{suggested}{RESET} (based on {detected['HC165']['max_bit'] + 1} inputs detected)")

    if name == "ENABLE_PCA9555" and "PCA9555" in detected:
        addrs = detected["PCA9555"].get("addresses", [])
        if addrs:
            print(f"  {YELLOW}PCA9555 chips detected at: {', '.join(addrs)}{RESET}")

    print()

    # Choose prompt based on type
    if pin_type == "bool":
        prompt = f"  Value (0 or 1) [{default}]: "
    elif pin_type == "int":
        prompt = f"  Value (number or -1) [{default}]: "
    else:
        prompt = f"  Value (GPIO number or -1) [{default}]: "

    _w(prompt)

    # Simple line input (no msvcrt.getwch loop needed, just input())
    try:
        raw = input().strip()
    except (EOFError, KeyboardInterrupt):
        return

    if not raw:
        # Empty = use default if pin was not set; otherwise keep current
        if current is None and default:
            known_pins[name] = default
        return

    # Validate
    if pin_type == "bool":
        if raw in ("0", "1"):
            known_pins[name] = raw
        else:
            print(f"  {RED}Invalid: must be 0 or 1{RESET}")
            msvcrt.getwch()
    elif _PIN_VALUE_RE.match(raw):
        known_pins[name] = raw
    else:
        print(f"  {RED}Invalid: enter a GPIO number or -1{RESET}")
        msvcrt.getwch()
