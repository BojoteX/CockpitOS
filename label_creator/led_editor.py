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
    dev = ui.pick("Device Type:", LED_DEVICES, default=record["device"])
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
        pin = ui.text_input("GPIO Pin", default=_extract_val(record["info_values"], 0, "0"))
        if pin is None:
            return False
        record["info_values"] = pin.strip()

    elif dev == "PCA9555":
        addr = ui.text_input("Address (e.g. 0x20)", default=_extract_val(record["info_values"], 0, "0x20"))
        if addr is None: return False
        port = ui.text_input("Port (0 or 1)", default=_extract_val(record["info_values"], 1, "0"))
        if port is None: return False
        bit = ui.text_input("Bit (0-7)", default=_extract_val(record["info_values"], 2, "0"))
        if bit is None: return False
        record["info_values"] = f"{addr.strip()}, {port.strip()}, {bit.strip()}"

    elif dev == "TM1637":
        clk = ui.text_input("CLK Pin", default=_extract_val(record["info_values"], 0, "0"))
        if clk is None: return False
        dio = ui.text_input("DIO Pin", default=_extract_val(record["info_values"], 1, "0"))
        if dio is None: return False
        seg = ui.text_input("Segment", default=_extract_val(record["info_values"], 2, "0"))
        if seg is None: return False
        bit = ui.text_input("Bit", default=_extract_val(record["info_values"], 3, "0"))
        if bit is None: return False
        record["info_values"] = f"{clk.strip()}, {dio.strip()}, {seg.strip()}, {bit.strip()}"

    elif dev == "GN1640T":
        addr = ui.text_input("Address", default=_extract_val(record["info_values"], 0, "0"))
        if addr is None: return False
        col = ui.text_input("Column", default=_extract_val(record["info_values"], 1, "0"))
        if col is None: return False
        row = ui.text_input("Row", default=_extract_val(record["info_values"], 2, "0"))
        if row is None: return False
        record["info_values"] = f"{addr.strip()}, {col.strip()}, {row.strip()}"

    elif dev == "WS2812":
        idx = ui.text_input("Index", default=_extract_val(record["info_values"], 0, "0"))
        if idx is None: return False
        pin = ui.text_input("Pin", default=_extract_val(record["info_values"], 1, "0"))
        if pin is None: return False
        defR = ui.text_input("Default R", default=_extract_val(record["info_values"], 2, "0"))
        if defR is None: return False
        defG = ui.text_input("Default G", default=_extract_val(record["info_values"], 3, "0"))
        if defG is None: return False
        defB = ui.text_input("Default B", default=_extract_val(record["info_values"], 4, "0"))
        if defB is None: return False
        defBright = ui.text_input("Default Brightness", default=_extract_val(record["info_values"], 5, "0"))
        if defBright is None: return False
        record["info_values"] = (f"{idx.strip()}, {pin.strip()}, "
                                  f"{defR.strip()}, {defG.strip()}, "
                                  f"{defB.strip()}, {defBright.strip()}")

    elif dev == "GAUGE":
        gpio = ui.text_input("GPIO Pin", default=_extract_val(record["info_values"], 0, "0"))
        if gpio is None: return False
        minP = ui.text_input("Min Pulse", default=_extract_val(record["info_values"], 1, "544"))
        if minP is None: return False
        maxP = ui.text_input("Max Pulse", default=_extract_val(record["info_values"], 2, "2400"))
        if maxP is None: return False
        period = ui.text_input("Period", default=_extract_val(record["info_values"], 3, "20000"))
        if period is None: return False
        record["info_values"] = f"{gpio.strip()}, {minP.strip()}, {maxP.strip()}, {period.strip()}"

    # Dimmable
    dim_opts = [("true", "true"), ("false", "false")]
    dim = ui.pick("Dimmable:", dim_opts, default=record["dimmable"])
    if dim is None:
        return False
    record["dimmable"] = dim

    # Active Low
    alow_opts = [("true", "true"), ("false", "false")]
    alow = ui.pick("Active Low:", alow_opts, default=record["activeLow"])
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

    # Column widths
    label_w = max(len(r["label"]) for r in records)
    label_w = max(label_w, 10)
    label_w = min(label_w, 30)
    dev_w   = 8
    info_w  = 30
    dim_w   = 5
    alow_w  = 5

    def _render_row(i, is_highlighted, scroll_char):
        r = records[i]
        label_trunc = r["label"][:label_w]
        dev_trunc   = r["device"][:dev_w]
        info_str    = _info_summary(r)[:info_w]
        dim_str     = r["dimmable"][:dim_w]
        alow_str    = r["activeLow"][:alow_w]

        wired = r["device"] != "NONE"

        if is_highlighted and _flash_active[0]:
            return (f" {YELLOW}> {label_trunc:<{label_w}}  "
                    f"{dev_trunc:<{dev_w}}  {info_str:<{info_w}}  "
                    f"{dim_str:<{dim_w}}  {alow_str:<{alow_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            pointer = f"{CYAN}>{RESET}"
        else:
            pointer = " "

        if wired:
            color = GREEN
        else:
            color = DIM

        return (f" {pointer} {color}{label_trunc:<{label_w}}{RESET}  "
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
