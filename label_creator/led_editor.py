"""
CockpitOS Label Creator -- LEDMapping.h TUI editor.

Scrollable list of LEDMapping records with per-record editing.
User-editable fields: device, info_type, info_values, dimmable, activeLow.
Hash tables, struct definitions, and enum blocks are untouched.
"""

import os
import re
import sys
import msvcrt
import threading

import ui

# ---------------------------------------------------------------------------
# ANSI constants (mirror ui.py / panels.py)
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

_SECTION_SEP = f"\n  {DIM}\u2500\u2500 Configure \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500{RESET}\n"
_SEL_BG  = "\033[48;5;236m"    # subtle dark gray row highlight

# ---------------------------------------------------------------------------
# Device type options for the picker
# ---------------------------------------------------------------------------
LED_DEVICES = [
    ("NONE (not wired)", "NONE"),
    ("GPIO",             "GPIO"),
    ("PCA9555",          "PCA9555"),
    ("TM1637",           "TM1637"),
    ("GN1640T",          "GN1640T"),
    ("WS2812",           "WS2812"),
    ("GAUGE",            "GAUGE"),
]

# Map device -> info struct type name
_DEVICE_INFO_MAP = {
    "NONE":    "gpioInfo",
    "GPIO":    "gpioInfo",
    "PCA9555": "pcaInfo",
    "TM1637":  "tm1637Info",
    "GN1640T": "gn1640Info",
    "WS2812":  "ws2812Info",
    "GAUGE":   "gaugeInfo",
}


# Strict regex: exactly two hex digits
_HEX2 = re.compile(r'^[0-9A-Fa-f]{2}$')

# ---------------------------------------------------------------------------
# Beginner-friendly descriptions shown in the device type picker
# ---------------------------------------------------------------------------
_DEVICE_DESCRIPTIONS = {
    "NONE": [
        f"{BOLD}Not wired{RESET}",
        f"{DIM}Skip this indicator \u2014 it won't be connected to any{RESET}",
        f"{DIM}physical LED or gauge on your panel.{RESET}",
        f"{DIM}You can always come back and wire it later.{RESET}",
    ],
    "GPIO": [
        f"{CYAN}{BOLD}GPIO{RESET} {DIM}\u2014 Direct connection to an ESP32 pin{RESET}",
        f"{DIM}The simplest wiring: one wire from an LED straight to a{RESET}",
        f"{DIM}pin on the ESP32 board. No extra chips needed.{RESET}",
        f"{DIM}If dimmable, the pin must support PWM (most ESP32 pins do).{RESET}",
        "",
        f"{DIM}Good for: {RESET}single LEDs, simple ON/OFF indicators,",
        f"          warning lights with few outputs.",
    ],
    "PCA9555": [
        f"{CYAN}{BOLD}PCA9555{RESET} {DIM}\u2014 I\u00b2C GPIO expander chip{RESET}",
        f"{DIM}Adds 16 extra output pins via the I\u00b2C bus (just 2 wires).{RESET}",
        f"{DIM}Two 8-bit ports (bank A and B). Standard chip addresses{RESET}",
        f"{DIM}are 0x20\u20130x27. Clones may use other addresses.{RESET}",
        "",
        f"{DIM}Good for: {RESET}LED panels where you need many outputs",
        f"          without using up ESP32 pins.",
    ],
    "TM1637": [
        f"{CYAN}{BOLD}TM1637{RESET} {DIM}\u2014 LED/keypad driver chip{RESET}",
        f"{DIM}Drives a 6\u00d78 LED matrix (6 grid segments \u00d7 8 bits each).{RESET}",
        f"{DIM}Two-wire interface: CLK + DIO. Multiple chips can share{RESET}",
        f"{DIM}the same CLK pin but each needs a unique DIO pin.{RESET}",
        "",
        f"{DIM}Good for: {RESET}annunciator panels with many indicator LEDs",
        f"          grouped together on the same chip.",
    ],
    "GN1640T": [
        f"{CYAN}{BOLD}GN1640T{RESET} {DIM}\u2014 8\u00d78 LED matrix driver chip{RESET}",
        f"{DIM}Drives up to 64 LEDs in an 8-column \u00d7 8-row grid.{RESET}",
        f"{DIM}Each LED is addressed by its column (0\u20137) and row (0\u20137).{RESET}",
        f"{DIM}Single-instance driver chip with built-in scanning.{RESET}",
        "",
        f"{DIM}Good for: {RESET}large LED indicator grids and matrices",
        f"          where many LEDs share one driver.",
    ],
    "WS2812": [
        f"{CYAN}{BOLD}WS2812{RESET} {DIM}\u2014 Addressable RGB LED (NeoPixel){RESET}",
        f"{DIM}Each LED has its own color (R, G, B) and brightness.{RESET}",
        f"{DIM}LEDs chain on a single data pin \u2014 same pin = same strip.{RESET}",
        f"{DIM}Each LED is identified by its index on the strip.{RESET}",
        "",
        f"{DIM}Good for: {RESET}backlighting, colored status indicators,",
        f"          RGB annunciators, and decorative lighting.",
    ],
    "GAUGE": [
        f"{CYAN}{BOLD}GAUGE{RESET} {DIM}\u2014 Analog servo gauge driven by PWM pulse{RESET}",
        f"{DIM}Maps a DCS value (0\u201365535) to a servo position between{RESET}",
        f"{DIM}min and max pulse width. Typical servo: 544\u20132400\u00b5s at 50Hz.{RESET}",
        f"{DIM}The GPIO pin must be PWM-capable.{RESET}",
        "",
        f"{DIM}Good for: {RESET}physical analog gauges, needle instruments,",
        f"          and any indicator that uses a servo motor.",
    ],
}


def _prompt_pca_hex_address(current="0x20"):
    """Prompt for a PCA9555 I2C address with strict validation.

    Returns the address as '0xHH' (e.g. '0x20', '0x5B') or None on cancel.
    Accepts input with or without 0x prefix. Rejects 0x00.
    """
    print()
    ui.info(f"{DIM}Standard PCA9555 addresses: 0x20-0x27{RESET}")
    ui.info(f"{DIM}Clones may use other addresses (e.g. 0x5A, 0x5B){RESET}")

    while True:
        addr = ui.text_input("Address (e.g. 0x20)", default=current)
        if addr is None:
            return None

        addr = addr.strip()

        # Strip 0x/0X prefix to isolate hex digits
        raw_up = addr.upper()
        if raw_up.startswith("0X"):
            hex_part = addr[2:]
        else:
            hex_part = addr

        # Strict: exactly two hex digits
        if not _HEX2.match(hex_part):
            ui.warn(f"Invalid address: {addr}")
            ui.info(f"{DIM}Enter exactly two hex digits, e.g. 0x20, 0x5B, 27{RESET}")
            continue

        # Normalize to 0xHH
        result = "0x" + hex_part.upper()

        # Reject 0x00
        if hex_part.upper() == "00":
            ui.warn("Address 0x00 is not valid (firmware skips it)")
            continue

        return result


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
# Parser — reads LEDMapping.h into (records, raw_lines)
# ---------------------------------------------------------------------------
_ENTRY_RE = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*DEVICE_(?P<device>[A-Z0-9_]+)\s*,'
    r'\s*\{\s*\.(?P<info_type>[a-zA-Z0-9_]+)\s*=\s*\{\s*(?P<info_values>[^}]+)\}\s*\}'
    r'\s*,\s*(?P<dimmable>true|false)\s*,\s*(?P<activeLow>true|false)\s*\}',
    re.MULTILINE
)


def parse_led_mapping(filepath):
    """Parse LEDMapping.h into (records, raw_lines).

    Each record dict:
      label, device, info_type, info_values (str), dimmable (str),
      activeLow (str), line_index (int)
    """
    with open(filepath, "r", encoding="utf-8") as f:
        raw_lines = f.readlines()

    records = []
    for i, line in enumerate(raw_lines):
        m = _ENTRY_RE.search(line)
        if not m:
            continue
        d = m.groupdict()
        records.append({
            "label":       d["label"],
            "device":      d["device"],
            "info_type":   d["info_type"],
            "info_values": d["info_values"].strip(),
            "dimmable":    d["dimmable"],
            "activeLow":   d["activeLow"],
            "line_index":  i,
        })
    return records, raw_lines


# ---------------------------------------------------------------------------
# Comment generator (port from generator_core.py generate_comment)
# ---------------------------------------------------------------------------
def _generate_comment(device, info_type, info_values):
    """Generate a trailing comment describing the LED record."""
    vals = [v.strip() for v in info_values.split(",")]
    if device == "NONE":
        return "// No Info"
    if device == "GPIO" and vals:
        return f"// GPIO {vals[0]}"
    if device == "GAUGE" and len(vals) >= 4:
        return f"// Gauge GPIO {vals[0]} min {vals[1]} max {vals[2]} period {vals[3]}"
    if device == "PCA9555" and len(vals) >= 3:
        return f"// PCA Addr {vals[0]} Port {vals[1]} Bit {vals[2]}"
    if device == "TM1637" and len(vals) >= 4:
        return f"// TM1637 CLK {vals[0]} DIO {vals[1]} Seg {vals[2]} Bit {vals[3]}"
    if device == "GN1640T" and len(vals) >= 3:
        return f"// GN1640 Addr {vals[0]} Col {vals[1]} Row {vals[2]}"
    if device == "WS2812" and len(vals) >= 1:
        return f"// WS2812 Index {vals[0]}"
    return "// No Info"


# ---------------------------------------------------------------------------
# Write-back — only replaces record lines, everything else untouched
# ---------------------------------------------------------------------------
def write_led_mapping(filepath, records, raw_lines):
    """Write updated records back into raw_lines and save to disk.

    Only the lines at each record's line_index are replaced.
    Everything else (header, struct def, enum, hash table) stays untouched.

    The generator uses ",\\n".join() so:
      - Non-last entries have a trailing comma at the very end of the line
      - Last entry has no trailing comma
    We detect this from the original line and preserve the pattern.
    """
    if not records:
        with open(filepath, "w", encoding="utf-8") as f:
            f.writelines(raw_lines)
        return

    max_label = max(len(r["label"]) for r in records)

    for i, r in enumerate(records):
        padded_label = f'"{r["label"]}"'.ljust(max_label + 2)
        dev = r["device"]

        # Detect trailing comma from original line
        orig_line = raw_lines[r["line_index"]].rstrip()
        has_trailing_comma = orig_line.endswith(",")

        comment = _generate_comment(dev, r["info_type"], r["info_values"])

        # Build the struct entry body — always use }}, format (matches generator's
        # preserved-entry path: line 1234-1237 of generator_core.py)
        if dev == "NONE":
            body = (f'  {{ {padded_label}, DEVICE_NONE    , '
                    f'{{.gpioInfo = {{0}}}}, false, false }}')
        else:
            body = (f'  {{ {padded_label}, DEVICE_{dev.ljust(8)}, '
                    f'{{.{r["info_type"]} = {{{r["info_values"]}}}}}, '
                    f'{r["dimmable"]}, {r["activeLow"]} }}')

        # Assemble: body + , + comment + trailing comma (from join)
        if has_trailing_comma:
            new_line = f"{body}, {comment},\n"
        else:
            # Last entry — no trailing comma from join
            new_line = f"{body}, {comment}\n"

        raw_lines[r["line_index"]] = new_line

    with open(filepath, "w", encoding="utf-8") as f:
        f.writelines(raw_lines)


# ---------------------------------------------------------------------------
# Quick count helper (for menu caption)
# ---------------------------------------------------------------------------
def count_wired(filepath):
    """Return (wired, total) counts from a LEDMapping.h file."""
    try:
        records, _ = parse_led_mapping(filepath)
    except Exception:
        return (0, 0)
    wired = sum(1 for r in records if r["device"] != "NONE")
    return (wired, len(records))


# ---------------------------------------------------------------------------
# Compact info summary for display
# ---------------------------------------------------------------------------
def _info_summary(record):
    """Build a short summary string from the device info."""
    dev = record["device"]
    vals = [v.strip() for v in record["info_values"].split(",")]

    if dev == "NONE":
        return "-"
    if dev == "GPIO":
        return f"pin={vals[0]}" if vals else "-"
    if dev == "PCA9555" and len(vals) >= 3:
        return f"addr={vals[0]} p={vals[1]} b={vals[2]}"
    if dev == "TM1637" and len(vals) >= 4:
        return f"clk={vals[0]} dio={vals[1]} s={vals[2]} b={vals[3]}"
    if dev == "GN1640T" and len(vals) >= 3:
        return f"a={vals[0]} c={vals[1]} r={vals[2]}"
    if dev == "WS2812" and len(vals) >= 1:
        return f"idx={vals[0]}"
    if dev == "GAUGE" and len(vals) >= 4:
        return f"gpio={vals[0]} {vals[1]}-{vals[2]}"
    return record["info_values"][:20]


# ---------------------------------------------------------------------------
# Edit flow
# ---------------------------------------------------------------------------
def _edit_record(record):
    """Edit a single LED record.

    Prompts: device type -> device-specific fields -> dimmable -> activeLow.
    Returns True if record was modified.
    """
    label = record["label"]

    ui.header(f"Edit LED: {label}")
    print()
    ui.info(f"Current: DEVICE_{record['device']}  "
            f"{record['info_type']}={{{record['info_values']}}}  "
            f"dim={record['dimmable']}  alow={record['activeLow']}")
    print()

    # Pick device type
    dev = ui.pick("Device Type:", LED_DEVICES, default=record["device"],
                   descriptions=_DEVICE_DESCRIPTIONS)
    if dev is None:
        return False

    if dev == "NONE":
        record["device"]      = "NONE"
        record["info_type"]   = "gpioInfo"
        record["info_values"] = "0"
        record["dimmable"]    = "false"
        record["activeLow"]   = "false"
        return True

    record["device"]    = dev
    record["info_type"] = _DEVICE_INFO_MAP.get(dev, "gpioInfo")

    # Device-specific fields
    if dev == "GPIO":
        # ── GPIO: direct pin control ─────────────────────────────────
        print(_SECTION_SEP)
        pin = ui.text_input("GPIO pin (e.g. PIN(6) or 6)",
                            default=_extract_val(record["info_values"], 0, "0"))
        if pin is None:
            return False
        record["info_values"] = pin.strip()

    elif dev == "PCA9555":
        # ── PCA9555: I2C GPIO expander ───────────────────────────────
        print(_SECTION_SEP)
        addr = _prompt_pca_hex_address(_extract_val(record["info_values"], 0, "0x20"))
        if addr is None: return False
        print()
        ui.info(f"{DIM}Port selects which 8-bit bank on the chip:{RESET}")
        ui.info(f"{DIM}  0 = Port A (pins IO0_0..IO0_7)    1 = Port B (pins IO1_0..IO1_7){RESET}")
        port = ui.text_input("Port (0 or 1)",
                             default=_extract_val(record["info_values"], 1, "0"))
        if port is None: return False
        print()
        ui.info(f"{DIM}Bit position within the port (0-7), one per physical pin.{RESET}")
        bit = ui.text_input("Bit (0-7)",
                            default=_extract_val(record["info_values"], 2, "0"))
        if bit is None: return False
        record["info_values"] = f"{addr}, {port.strip()}, {bit.strip()}"

    elif dev == "TM1637":
        # ── TM1637: LED+keypad driver ────────────────────────────────
        print(_SECTION_SEP)
        clk = ui.text_input("CLK pin (clock GPIO)",
                            default=_extract_val(record["info_values"], 0, "0"))
        if clk is None: return False
        dio = ui.text_input("DIO pin (data GPIO, unique per chip)",
                            default=_extract_val(record["info_values"], 1, "0"))
        if dio is None: return False
        print()
        ui.info(f"{DIM}Segment = grid position on the chip (0-5).{RESET}")
        ui.info(f"{DIM}Bit = which LED within that segment (0-7).{RESET}")
        seg = ui.text_input("Segment (0-5)",
                            default=_extract_val(record["info_values"], 2, "0"))
        if seg is None: return False
        bit = ui.text_input("Bit (0-7)",
                            default=_extract_val(record["info_values"], 3, "0"))
        if bit is None: return False
        record["info_values"] = f"{clk.strip()}, {dio.strip()}, {seg.strip()}, {bit.strip()}"

    elif dev == "GN1640T":
        # ── GN1640T: 8x8 LED matrix driver ──────────────────────────
        print(_SECTION_SEP)
        addr = ui.text_input("Address (0 = default)",
                             default=_extract_val(record["info_values"], 0, "0"))
        if addr is None: return False
        col = ui.text_input("Column (0-7)",
                            default=_extract_val(record["info_values"], 1, "0"))
        if col is None: return False
        row = ui.text_input("Row (0-7)",
                            default=_extract_val(record["info_values"], 2, "0"))
        if row is None: return False
        record["info_values"] = f"{addr.strip()}, {col.strip()}, {row.strip()}"

    elif dev == "WS2812":
        # ── WS2812: addressable RGB LEDs ─────────────────────────────
        print(_SECTION_SEP)
        idx = ui.text_input("LED index on strip (0-based position)",
                            default=_extract_val(record["info_values"], 0, "0"))
        if idx is None: return False
        pin = ui.text_input("Data pin GPIO (same pin = same strip)",
                            default=_extract_val(record["info_values"], 1, "0"))
        if pin is None: return False
        print()
        ui.info(f"{DIM}Default color when LED is ON (0-255 each).{RESET}")
        ui.info(f"{DIM}When OFF the LED is black. Brightness scales the color.{RESET}")
        defR = ui.text_input("Default Red (0-255)",
                             default=_extract_val(record["info_values"], 2, "0"))
        if defR is None: return False
        defG = ui.text_input("Default Green (0-255)",
                             default=_extract_val(record["info_values"], 3, "0"))
        if defG is None: return False
        defB = ui.text_input("Default Blue (0-255)",
                             default=_extract_val(record["info_values"], 4, "0"))
        if defB is None: return False
        defBright = ui.text_input("Default Brightness (0-255)",
                                  default=_extract_val(record["info_values"], 5, "0"))
        if defBright is None: return False
        record["info_values"] = (f"{idx.strip()}, {pin.strip()}, "
                                  f"{defR.strip()}, {defG.strip()}, "
                                  f"{defB.strip()}, {defBright.strip()}")

    elif dev == "GAUGE":
        # ── GAUGE: analog servo gauge ────────────────────────────────
        print(_SECTION_SEP)
        gpio = ui.text_input("Servo GPIO pin (PWM-capable)",
                             default=_extract_val(record["info_values"], 0, "0"))
        if gpio is None: return False
        print()
        ui.info(f"{DIM}Pulse range in microseconds \u2014 defines servo sweep.{RESET}")
        ui.info(f"{DIM}Typical: min=544, max=2400. Adjust per your servo specs.{RESET}")
        minP = ui.text_input("Min pulse \u00b5s (servo at 0%)",
                             default=_extract_val(record["info_values"], 1, "544"))
        if minP is None: return False
        maxP = ui.text_input("Max pulse \u00b5s (servo at 100%)",
                             default=_extract_val(record["info_values"], 2, "2400"))
        if maxP is None: return False
        print()
        ui.info(f"{DIM}Period in microseconds. 20000 = 50Hz (standard RC servo).{RESET}")
        period = ui.text_input("Period \u00b5s (20000 = 50Hz)",
                               default=_extract_val(record["info_values"], 3, "20000"))
        if period is None: return False
        record["info_values"] = f"{gpio.strip()}, {minP.strip()}, {maxP.strip()}, {period.strip()}"

    # Dimmable
    print()
    ui.info(f"{DIM}Dimmable: if true, LED intensity follows the panel dimmer (PWM).{RESET}")
    ui.info(f"{DIM}If false, LED is fully ON or fully OFF.{RESET}")
    dim_opts = [("true", "true"), ("false", "false")]
    dim = ui.pick("Dimmable:", dim_opts, default=record["dimmable"])
    if dim is None:
        return False
    record["dimmable"] = dim

    # Active Low (presented as "Reversed?" for beginners)
    print()
    ui.info(f"{DIM}Is this LED wired in reverse? (ON when signal is LOW){RESET}")
    ui.info(f"{DIM}Most LEDs are {BOLD}not{RESET}{DIM} reversed — pick No unless you know otherwise.{RESET}")
    alow_opts = [("No  (normal, most common)", "false"),
                 ("Yes (reversed / active-low)", "true")]
    alow = ui.pick("Reversed?", alow_opts, default=record["activeLow"])
    if alow is None:
        return False
    record["activeLow"] = alow

    return True


def _extract_val(info_values, index, default="0"):
    """Extract a value at a given index from a comma-separated info_values string."""
    parts = [v.strip() for v in info_values.split(",")]
    if index < len(parts) and parts[index]:
        return parts[index]
    return default


# ---------------------------------------------------------------------------
# Scrollable TUI list view
# ---------------------------------------------------------------------------
def edit_led_mapping(filepath, label_set_name="", aircraft_name=""):
    """Main entry point: scrollable TUI editor for LEDMapping.h.

    Follows panels.py _show_detail() pattern: full-redraw, msvcrt keys,
    scroll bar, boundary flash.
    """
    records, raw_lines = parse_led_mapping(filepath)
    if not records:
        ui.header("LED Mapping")
        print()
        ui.warn("No records found in LEDMapping.h")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    total = len(records)
    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    # Reserve: 3 header + 1 col-header + list + 1 blank + 1 footer = 6 fixed
    list_height = rows - 6
    if list_height < 5:
        list_height = 5

    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"
    needs_scroll = total > list_height

    _flash_timer  = [None]
    _flash_active = [False]

    def _wired_count():
        return sum(1 for r in records if r["device"] != "NONE")

    def _scroll_bar_positions():
        if not needs_scroll:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / total))
        max_scroll = total - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    # Column widths — fill terminal width
    # Layout: " > " (3) + label + device + info + dim + alow + scroll(1)
    dev_w   = 8
    dim_w   = 5
    alow_w  = 5
    # Fixed overhead: prefix(3) + 4 gaps(2 each=8) + scrollbar(1) + margin(1)
    fixed_cols = 3 + (dev_w + dim_w + alow_w) + 8 + 2
    # Split remaining space: ~40% label, ~60% info (at least)
    remaining  = cols - fixed_cols
    label_w    = max(remaining * 2 // 5, 10)
    info_w     = max(remaining - label_w, 15)

    def _render_row(i, is_highlighted, scroll_char):
        r = records[i]
        label_trunc = r["label"][:label_w]
        dev_trunc   = r["device"][:dev_w]
        info_str    = _info_summary(r)[:info_w]
        dim_str     = r["dimmable"][:dim_w]
        alow_str    = r["activeLow"][:alow_w]

        wired = r["device"] != "NONE"

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG} {YELLOW}> {label_trunc:<{label_w}}  "
                    f"{dev_trunc:<{dev_w}}  {info_str:<{info_w}}  "
                    f"{dim_str:<{dim_w}}  {alow_str:<{alow_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            color = GREEN if wired else DIM
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} "
                    f"{color}{label_trunc:<{label_w}}{RESET}{_SEL_BG}  "
                    f"{color}{dev_trunc:<{dev_w}}{RESET}{_SEL_BG}  "
                    f"{color}{info_str:<{info_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{dim_str:<{dim_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{alow_str:<{alow_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        if wired:
            color = GREEN
        else:
            color = DIM

        return (f"   {color}{label_trunc:<{label_w}}{RESET}  "
                f"{color}{dev_trunc:<{dev_w}}{RESET}  "
                f"{color}{info_str:<{info_w}}{RESET}  "
                f"{DIM}{dim_str:<{dim_w}}{RESET}  "
                f"{DIM}{alow_str:<{alow_w}}{RESET}"
                f" {DIM}{scroll_char}{RESET}")

    def _clamp_scroll():
        nonlocal scroll
        if idx < scroll:
            scroll = idx
        if idx >= scroll + list_height:
            scroll = idx - list_height + 1

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

    header_w = cols - 4

    def _draw():
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        # Header
        parts = [label_set_name, aircraft_name]
        ctx = "  (" + ", ".join(p for p in parts if p) + ")" if any(parts) else ""
        wired = _wired_count()
        title = f"LED Mapping{ctx}"
        counter = f"{wired} / {total} wired"
        spacing = header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Column header
        _w(f" {BOLD}  {'Label':<{label_w}}  {'Device':<{dev_w}}  "
           f"{'Info':<{info_w}}  {'Dim':<{dim_w}}  {'ALow':<{alow_w}}{RESET}\n")

        # Scroll bar
        thumb_start, thumb_end = _scroll_bar_positions()

        # Rows
        for row in range(list_height):
            ri = scroll + row
            if needs_scroll:
                sc = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
            else:
                sc = " "
            if ri < total:
                _w(_render_row(ri, ri == idx, sc) + "\n")
            else:
                _w(ERASE_LN + "\n")

        # Footer
        _w(f"\n  {DIM}\u2191\u2193=move  \u2192/Enter=edit  \u2190/Esc=back{RESET}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                old = idx
                if ch2 == "H":          # Up
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                elif ch2 == "P":        # Down
                    if idx < total - 1:
                        idx += 1
                    else:
                        _flash_row()
                elif ch2 == "M":        # Right — edit
                    _w(SHOW_CUR)
                    modified = _edit_record(records[idx])
                    if modified:
                        write_led_mapping(filepath, records, raw_lines)
                        # Re-parse to stay consistent
                        records, raw_lines = parse_led_mapping(filepath)
                        total = len(records)
                        if idx >= total:
                            idx = max(0, total - 1)
                        needs_scroll = total > list_height
                    _clamp_scroll()
                    _draw()
                    continue
                elif ch2 == "K":        # Left — back
                    if _flash_timer[0]:
                        _flash_timer[0].cancel()
                    _w(SHOW_CUR)
                    return
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch == "\r":            # Enter — edit
                _w(SHOW_CUR)
                modified = _edit_record(records[idx])
                if modified:
                    write_led_mapping(filepath, records, raw_lines)
                    records, raw_lines = parse_led_mapping(filepath)
                    total = len(records)
                    if idx >= total:
                        idx = max(0, total - 1)
                    needs_scroll = total > list_height
                _clamp_scroll()
                _draw()

            elif ch == "\x1b":          # Esc — back
                if _flash_timer[0]:
                    _flash_timer[0].cancel()
                _w(SHOW_CUR)
                return

    except KeyboardInterrupt:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)
        raise
