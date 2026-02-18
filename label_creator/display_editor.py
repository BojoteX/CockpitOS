"""
CockpitOS Label Creator -- DisplayMapping.cpp TUI editor.

Scrollable list of display field definitions with per-record editing.
User-editable fields: renderType, fieldType, minValue, maxValue, barCount,
clearFunc.  All other fields (label, segMap, numDigits, segsPerDigit,
driver, deviceType, renderFunc) are read-only / auto-generated.

Hash tables, struct definitions, and lookup functions are untouched.
"""

import os
import re
import sys
import msvcrt
import threading

import ui

# ---------------------------------------------------------------------------
# ANSI constants (mirror ui.py / other editors)
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

_SEL_BG  = "\033[48;5;236m"    # subtle dark-gray row highlight
_SECTION_SEP = (
    f"\n  {DIM}\u2500\u2500 Configure "
    f"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
    f"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
    f"\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500{RESET}\n"
)

# ---------------------------------------------------------------------------
# Render-type options with beginner-friendly descriptions
# ---------------------------------------------------------------------------
RENDER_TYPES = [
    ("7-Segment",              "FIELD_RENDER_7SEG"),
    ("7-Segment (shared)",     "FIELD_RENDER_7SEG_SHARED"),
    ("Label / indicator",      "FIELD_RENDER_LABEL"),
    ("Bingo fuel",             "FIELD_RENDER_BINGO"),
    ("Bargraph / pointer bar", "FIELD_RENDER_BARGRAPH"),
    ("Fuel display",           "FIELD_RENDER_FUEL"),
    ("RPM display",            "FIELD_RENDER_RPM"),
    ("Alphanumeric fuel",      "FIELD_RENDER_ALPHA_NUM_FUEL"),
    ("Custom render",          "FIELD_RENDER_CUSTOM"),
]

_RENDER_DESCRIPTIONS = {
    "FIELD_RENDER_7SEG": [
        f"{CYAN}{BOLD}Standard 7-segment{RESET}",
        f"{DIM}Converts incoming ASCII characters to 7-segment patterns{RESET}",
        f"{DIM}using the seg7_ascii font table. Each digit is mapped to{RESET}",
        f"{DIM}its segment addresses via the SegmentMap array.{RESET}",
        f"{DIM}Use for: numeric readouts (RPM, temperature, oil pressure).{RESET}",
    ],
    "FIELD_RENDER_7SEG_SHARED": [
        f"{CYAN}{BOLD}Shared 7-segment{RESET}",
        f"{DIM}Like standard 7-seg but this field shares physical display{RESET}",
        f"{DIM}space with another field (overlay). The firmware caches the{RESET}",
        f"{DIM}base value and restores it when the overlay clears.{RESET}",
        f"{DIM}Use for: SP/CODES overlaying TEMP regions on IFEI.{RESET}",
    ],
    "FIELD_RENDER_LABEL": [
        f"{CYAN}{BOLD}Label / indicator{RESET}",
        f"{DIM}Single on/off segment \u2014 a texture or status indicator{RESET}",
        f"{DIM}that lights up when the field has a non-blank value.{RESET}",
        f"{DIM}Uses a single SegmentMap entry (not an array).{RESET}",
        f"{DIM}Use for: RPM_TEXTURE, FF_TEXTURE, BINGO_TEXTURE, etc.{RESET}",
    ],
    "FIELD_RENDER_BINGO": [
        f"{CYAN}{BOLD}Bingo fuel display{RESET}",
        f"{DIM}Special 5-digit renderer with custom segment handling{RESET}",
        f"{DIM}for the BINGO fuel quantity readout.{RESET}",
        f"{DIM}Use for: IFEI bingo fuel display specifically.{RESET}",
    ],
    "FIELD_RENDER_BARGRAPH": [
        f"{CYAN}{BOLD}Bargraph / pointer bar{RESET}",
        f"{DIM}Maps a numeric value (0\u2013100) to a bar of lit segments.{RESET}",
        f"{DIM}Tracks the current pointer position and clears the{RESET}",
        f"{DIM}previous position when the value changes.{RESET}",
        f"{DIM}Use for: nozzle position indicators.{RESET}",
    ],
    "FIELD_RENDER_FUEL": [
        f"{CYAN}{BOLD}Fuel display{RESET}",
        f"{DIM}7-segment fuel readout with overlay support. Another{RESET}",
        f"{DIM}field (like T or TIME_SET_MODE) can overlay the same{RESET}",
        f"{DIM}physical display region. Caches base values for restore.{RESET}",
        f"{DIM}Use for: FUEL_UP / FUEL_DOWN on IFEI.{RESET}",
    ],
    "FIELD_RENDER_RPM": [
        f"{CYAN}{BOLD}RPM display{RESET}",
        f"{DIM}Special renderer for RPM readouts that handles the{RESET}",
        f"{DIM}hundreds digit differently (single segment for '1').{RESET}",
        f"{DIM}The first digit position only shows a bar, not a{RESET}",
        f"{DIM}full 7-segment character.{RESET}",
        f"{DIM}Use for: IFEI RPM left/right displays.{RESET}",
    ],
    "FIELD_RENDER_ALPHA_NUM_FUEL": [
        f"{CYAN}{BOLD}Alphanumeric fuel{RESET}",
        f"{DIM}14-segment fuel display with overlay support. Uses{RESET}",
        f"{DIM}seg14_ascii font table for full alphanumeric rendering.{RESET}",
        f"{DIM}Same overlay logic as FIELD_RENDER_FUEL but with 14 segs.{RESET}",
        f"{DIM}Use for: fuel fields that may show letters (e.g. T).{RESET}",
    ],
    "FIELD_RENDER_CUSTOM": [
        f"{CYAN}{BOLD}Custom render function{RESET}",
        f"{DIM}Uses a custom render function you define in your Panel.cpp.{RESET}",
        f"{DIM}The renderFunc pointer calls your code directly.{RESET}",
        f"{DIM}Use for: any display field that needs unique logic.{RESET}",
    ],
}

# ---------------------------------------------------------------------------
# FieldType options
# ---------------------------------------------------------------------------
FIELD_TYPES = [
    ("Label (on/off indicator)",    "FIELD_LABEL"),
    ("String (text characters)",    "FIELD_STRING"),
    ("Numeric (integer values)",    "FIELD_NUMERIC"),
    ("Mixed (text + numbers)",      "FIELD_MIXED"),
    ("Bargraph (bar segments)",     "FIELD_BARGRAPH"),
]

_FIELD_TYPE_DESCRIPTIONS = {
    "FIELD_LABEL": [
        f"{CYAN}{BOLD}Label{RESET}",
        f"{DIM}Single on/off indicator. Turns a segment on when{RESET}",
        f"{DIM}the field has a non-blank value.{RESET}",
    ],
    "FIELD_STRING": [
        f"{CYAN}{BOLD}String{RESET}",
        f"{DIM}Text characters rendered through the font table.{RESET}",
        f"{DIM}Most common type for segmented display fields.{RESET}",
    ],
    "FIELD_NUMERIC": [
        f"{CYAN}{BOLD}Numeric{RESET}",
        f"{DIM}Integer values. Can use minValue/maxValue for range.{RESET}",
    ],
    "FIELD_MIXED": [
        f"{CYAN}{BOLD}Mixed{RESET}",
        f"{DIM}Fields that contain both text and numeric data.{RESET}",
    ],
    "FIELD_BARGRAPH": [
        f"{CYAN}{BOLD}Bargraph{RESET}",
        f"{DIM}Bar segments driven by a numeric value. Uses barCount{RESET}",
        f"{DIM}to specify the number of bar segments.{RESET}",
    ],
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


# ═══════════════════════════════════════════════════════════════════════════
# Parser — reads DisplayMapping.cpp fieldDefs[] into (records, raw_lines)
# ═══════════════════════════════════════════════════════════════════════════
_FIELD_RE = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*'
    r'(?P<segMap>[^,]+)\s*,\s*'
    r'(?P<numDigits>\d+)\s*,\s*'
    r'(?P<segsPerDigit>\d+)\s*,\s*'
    r'(?P<minValue>-?\d+)\s*,\s*'
    r'(?P<maxValue>-?\d+)\s*,\s*'
    r'(?P<fieldType>\w+)\s*,\s*'
    r'(?P<barCount>\d+)\s*,\s*'
    r'(?P<driver>[^,]+)\s*,\s*'
    r'(?P<deviceType>\w+)\s*,\s*'
    r'(?P<renderFunc>\w+)\s*,\s*'
    r'(?P<clearFunc>\w+)\s*,\s*'
    r'(?P<renderType>\w+)\s*\}'
)


def parse_display_mapping(filepath):
    """Parse DisplayMapping.cpp into (records, raw_lines).

    Each record dict:
      label, segMap, numDigits (str), segsPerDigit (str),
      minValue (str), maxValue (str), fieldType, barCount (str),
      driver, deviceType, renderFunc, clearFunc, renderType,
      line_index (int)
    """
    with open(filepath, "r", encoding="utf-8") as f:
        raw_lines = f.readlines()

    records = []
    in_array = False
    for i, line in enumerate(raw_lines):
        if "const DisplayFieldDefLabel fieldDefs[]" in line:
            in_array = True
            continue
        if in_array and "};" in line and "{" not in line:
            break
        if not in_array:
            continue

        m = _FIELD_RE.search(line)
        if not m:
            continue

        d = m.groupdict()
        records.append({
            "label":        d["label"],
            "segMap":       d["segMap"].strip(),
            "numDigits":    d["numDigits"],
            "segsPerDigit": d["segsPerDigit"],
            "minValue":     d["minValue"],
            "maxValue":     d["maxValue"],
            "fieldType":    d["fieldType"],
            "barCount":     d["barCount"],
            "driver":       d["driver"].strip(),
            "deviceType":   d["deviceType"],
            "renderFunc":   d["renderFunc"],
            "clearFunc":    d["clearFunc"],
            "renderType":   d["renderType"],
            "line_index":   i,
        })

    return records, raw_lines


# ═══════════════════════════════════════════════════════════════════════════
# Write-back — only replaces record lines, everything else untouched
# ═══════════════════════════════════════════════════════════════════════════
def write_display_mapping(filepath, records, raw_lines):
    """Write updated records back into raw_lines and save to disk.

    Only the lines at each record's line_index are replaced.
    Everything else (hash tables, includes, functions) stays untouched.
    """
    if not records:
        with open(filepath, "w", encoding="utf-8") as f:
            f.writelines(raw_lines)
        return

    for i, r in enumerate(records):
        # Detect trailing comma from original line
        orig_line = raw_lines[r["line_index"]].rstrip()
        has_trailing_comma = orig_line.endswith(",")

        body = (
            f'{{ "{r["label"]}", {r["segMap"]}, '
            f'{r["numDigits"]}, {r["segsPerDigit"]}, '
            f'{r["minValue"]}, {r["maxValue"]}, '
            f'{r["fieldType"]}, {r["barCount"]}, '
            f'{r["driver"]}, {r["deviceType"]}, '
            f'{r["renderFunc"]}, {r["clearFunc"]}, '
            f'{r["renderType"]} }}'
        )

        if has_trailing_comma:
            new_line = f"{body},\n"
        else:
            new_line = f"{body}\n"

        raw_lines[r["line_index"]] = new_line

    with open(filepath, "w", encoding="utf-8") as f:
        f.writelines(raw_lines)


# ═══════════════════════════════════════════════════════════════════════════
# Quick count helper (for menu caption)
# ═══════════════════════════════════════════════════════════════════════════
def count_configured(filepath):
    """Return (configured, total) counts from a DisplayMapping.cpp file.

    A record is 'configured' if its segMap pointer is non-nullptr
    (i.e., it has a real segment map backing it).
    """
    try:
        records, _ = parse_display_mapping(filepath)
    except Exception:
        return (0, 0)

    total = len(records)
    configured = sum(1 for r in records if r["segMap"] != "nullptr")

    return (configured, total)


# ═══════════════════════════════════════════════════════════════════════════
# Row summary helpers
# ═══════════════════════════════════════════════════════════════════════════
def _dims_str(record):
    """Format dimensions like '[3×7]' or 'label'."""
    nd = int(record["numDigits"])
    sp = int(record["segsPerDigit"])
    if nd == 0 and sp == 0:
        return "label"
    return f"[{nd}\u00d7{sp}]"


def _render_short(record):
    """Short render type name for list display."""
    rt = record["renderType"]
    names = {
        "FIELD_RENDER_7SEG":           "7SEG",
        "FIELD_RENDER_7SEG_SHARED":    "7SEG-SH",
        "FIELD_RENDER_LABEL":          "LABEL",
        "FIELD_RENDER_BINGO":          "BINGO",
        "FIELD_RENDER_BARGRAPH":       "BARGRAPH",
        "FIELD_RENDER_FUEL":           "FUEL",
        "FIELD_RENDER_RPM":            "RPM",
        "FIELD_RENDER_ALPHA_NUM_FUEL": "A-FUEL",
        "FIELD_RENDER_CUSTOM":         "CUSTOM",
    }
    return names.get(rt, rt)


def _type_short(record):
    """Short field type name for list display."""
    ft = record["fieldType"]
    names = {
        "FIELD_LABEL":    "LABEL",
        "FIELD_STRING":   "STRING",
        "FIELD_NUMERIC":  "NUMERIC",
        "FIELD_MIXED":    "MIXED",
        "FIELD_BARGRAPH": "BARGRAPH",
    }
    return names.get(ft, ft)


def _is_configured(record):
    """Return True if the record has a real segment map (non-nullptr).

    Any entry with a valid segMap pointer is 'configured' — it has real
    hardware backing it.  Entries with nullptr are generator skeletons
    that have no segment map yet.
    """
    return record["segMap"] != "nullptr"


# ═══════════════════════════════════════════════════════════════════════════
# Per-record edit form
# ═══════════════════════════════════════════════════════════════════════════
def _edit_record(record):
    """Edit a single display field definition. Returns True if modified."""
    label = record["label"]

    ui.header(f"Display Field: {label}")
    print()

    # Read-only summary
    ui.info(f"  {DIM}Label:{RESET}       {CYAN}{label}{RESET}")
    ui.info(f"  {DIM}Segment Map:{RESET} {CYAN}{record['segMap']}{RESET}")
    ui.info(f"  {DIM}Dimensions:{RESET}  {CYAN}{_dims_str(record)}{RESET}")
    ui.info(f"  {DIM}Device:{RESET}      {CYAN}{record['deviceType']}{RESET}")
    ui.info(f"  {DIM}Driver:{RESET}      {CYAN}{record['driver']}{RESET}")
    ui.info(f"  {DIM}Renderer:{RESET}    {CYAN}{record['renderFunc']}{RESET}")

    print(_SECTION_SEP)

    modified = False

    # ── Render Type ────────────────────────────────────────────────────────
    rt = ui.pick(
        "Render Type:",
        RENDER_TYPES,
        default=record["renderType"],
        descriptions=_RENDER_DESCRIPTIONS,
    )
    if rt is None:
        return False
    if rt != record["renderType"]:
        record["renderType"] = rt
        modified = True

    # ── Field Type ─────────────────────────────────────────────────────────
    ft = ui.pick(
        "Field Type:",
        FIELD_TYPES,
        default=record["fieldType"],
        descriptions=_FIELD_TYPE_DESCRIPTIONS,
    )
    if ft is None:
        return modified
    if ft != record["fieldType"]:
        record["fieldType"] = ft
        modified = True

    # ── minValue / maxValue ────────────────────────────────────────────────
    # Only relevant for FIELD_NUMERIC — the firmware does:
    #   valid = (value >= minValue && value <= maxValue)
    # so 0,0 means ONLY the value 0 passes.  For non-numeric types the
    # range check is skipped entirely, so 0,0 is harmless — skip prompt.
    if record["fieldType"] == "FIELD_NUMERIC":
        print()
        ui.info(f"{DIM}Firmware rejects values outside [min … max].{RESET}")

        # Auto-fix dangerous 0,0 default
        cur_min = int(record["minValue"])
        cur_max = int(record["maxValue"])
        if cur_min == 0 and cur_max == 0:
            record["maxValue"] = "99999"
            modified = True
            ui.warn("Max was 0 (would block all values). Auto-set to 99999.")

        min_s = ui.text_input("Min value", default=record["minValue"])
        if min_s is None or min_s == ui._RESET_SENTINEL:
            return modified
        try:
            min_val = int(min_s)
        except ValueError:
            ui.error("Invalid integer. Keeping current value.")
            min_val = int(record["minValue"])
        if str(min_val) != record["minValue"]:
            record["minValue"] = str(min_val)
            modified = True

        max_s = ui.text_input("Max value", default=record["maxValue"])
        if max_s is None or max_s == ui._RESET_SENTINEL:
            return modified
        try:
            max_val = int(max_s)
        except ValueError:
            ui.error("Invalid integer. Keeping current value.")
            max_val = int(record["maxValue"])
        if str(max_val) != record["maxValue"]:
            record["maxValue"] = str(max_val)
            modified = True

        if max_val <= min_val:
            ui.warn(f"Max ({max_val}) \u2264 Min ({min_val}) — firmware will "
                    f"reject most values!")

    # ── barCount (only for BARGRAPH render type) ───────────────────────────
    if record["renderType"] in ("FIELD_RENDER_BARGRAPH",):
        print()
        ui.info(f"{DIM}Number of bar segments for bargraph rendering.{RESET}")
        bar_s = ui.text_input("Bar count", default=record["barCount"])
        if bar_s is not None and bar_s != ui._RESET_SENTINEL:
            try:
                bar_val = int(bar_s)
                if bar_val < 0:
                    raise ValueError
            except ValueError:
                ui.error("Invalid. Keeping current value.")
                bar_val = int(record["barCount"])
            if str(bar_val) != record["barCount"]:
                record["barCount"] = str(bar_val)
                modified = True

    # ── clearFunc ──────────────────────────────────────────────────────────
    print()
    ui.info(f"{DIM}Clear function \u2014 called when the field is cleared.{RESET}")
    ui.info(f"{DIM}Usually nullptr (dispatcher handles clearing).{RESET}")

    # Build options based on the device type
    prefix = record["deviceType"].replace("DISPLAY_", "")
    clear_name = f"clear{prefix}Dispatcher"
    clear_opts = [
        ("nullptr (default)",          "nullptr"),
        (f"{clear_name} (dispatcher)", clear_name),
    ]
    cf = ui.pick("Clear function:", clear_opts, default=record["clearFunc"])
    if cf is not None and cf != record["clearFunc"]:
        record["clearFunc"] = cf
        modified = True

    return modified


# ═══════════════════════════════════════════════════════════════════════════
# Scrollable TUI — main editor
# ═══════════════════════════════════════════════════════════════════════════
def edit_display_mapping(filepath, label_set_name="", aircraft_name=""):
    """Main entry point: scrollable TUI editor for DisplayMapping.cpp.

    Follows led_editor.py pattern: full-redraw, msvcrt keys,
    scroll bar, boundary flash.
    """
    records, raw_lines = parse_display_mapping(filepath)
    if not records:
        ui.header("Display Mapping")
        print()
        ui.warn("No display field records found in DisplayMapping.cpp")
        ui.info(f"{DIM}Make sure a Segment Map exists and the label set{RESET}")
        ui.info(f"{DIM}has been generated (or regenerated).{RESET}")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    total = len(records)
    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    # Reserve: 3 header + 1 col-header + list + 1 blank + 2 footer = 7 fixed
    list_height = rows - 7
    if list_height < 5:
        list_height = 5

    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"
    needs_scroll = total > list_height

    _flash_timer  = [None]
    _flash_active = [False]

    def _configured_count():
        return sum(1 for r in records if _is_configured(r))

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
    # Layout: " > " (3) + label + dims + render + type + scroll(1)
    dims_w   = 8
    render_w = 10
    type_w   = 9
    # Fixed overhead: prefix(3) + gaps(2 each * 3 = 6) + scrollbar(1) + margin(2)
    fixed_cols = 3 + dims_w + render_w + type_w + 6 + 3
    remaining = cols - fixed_cols
    label_w = max(remaining, 15)

    def _render_row(i, is_highlighted, scroll_char):
        r = records[i]
        label_trunc = r["label"][:label_w]
        dims  = _dims_str(r)[:dims_w]
        rtype = _render_short(r)[:render_w]
        ftype = _type_short(r)[:type_w]

        configured = _is_configured(r)

        if is_highlighted and _flash_active[0]:
            return (
                f"{_SEL_BG} {YELLOW}> {label_trunc:<{label_w}}  "
                f"{dims:<{dims_w}}  {rtype:<{render_w}}  {ftype:<{type_w}}"
                f" {scroll_char}{RESET}"
            )
        elif is_highlighted:
            color = GREEN if configured else DIM
            return (
                f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} "
                f"{color}{label_trunc:<{label_w}}{RESET}{_SEL_BG}  "
                f"{color}{dims:<{dims_w}}{RESET}{_SEL_BG}  "
                f"{color}{rtype:<{render_w}}{RESET}{_SEL_BG}  "
                f"{color}{ftype:<{type_w}}{RESET}{_SEL_BG}"
                f" {DIM}{scroll_char}{RESET}"
            )

        color = GREEN if configured else DIM
        return (
            f"   {color}{label_trunc:<{label_w}}{RESET}  "
            f"{color}{dims:<{dims_w}}{RESET}  "
            f"{color}{rtype:<{render_w}}{RESET}  "
            f"{color}{ftype:<{type_w}}{RESET}"
            f" {DIM}{scroll_char}{RESET}"
        )

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
        ctx = ("  (" + ", ".join(p for p in parts if p) + ")"
               if any(parts) else "")
        conf = _configured_count()
        title = f"Display Mapping{ctx}"
        counter = f"{conf} / {total} configured"
        spacing = header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}"
           f"{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Column header
        _w(f" {BOLD}  {'Label':<{label_w}}  {'Dims':<{dims_w}}  "
           f"{'Render':<{render_w}}  {'Type':<{type_w}}{RESET}\n")

        # Scroll bar
        thumb_start, thumb_end = _scroll_bar_positions()

        # Rows
        for row in range(list_height):
            ri = scroll + row
            if needs_scroll:
                sc = (_SCROLL_BLOCK
                      if thumb_start <= row < thumb_end
                      else _SCROLL_LIGHT)
            else:
                sc = " "
            if ri < total:
                _w(_render_row(ri, ri == idx, sc) + "\n")
            else:
                _w(ERASE_LN + "\n")

        # Footer
        _w(f"\n  {DIM}\u2191\u2193=move  \u2192/Enter=edit  "
           f"\u2190/Esc=back{RESET}\n"
           f"  {RED}Ctrl+D{RESET} {DIM}= delete selected entry{RESET}")

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
                    did_modify = _edit_record(records[idx])
                    if did_modify:
                        write_display_mapping(filepath, records, raw_lines)
                        # Re-parse to stay consistent
                        records, raw_lines = parse_display_mapping(filepath)
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
                did_modify = _edit_record(records[idx])
                if did_modify:
                    write_display_mapping(filepath, records, raw_lines)
                    records, raw_lines = parse_display_mapping(filepath)
                    total = len(records)
                    if idx >= total:
                        idx = max(0, total - 1)
                    needs_scroll = total > list_height
                _clamp_scroll()
                _draw()

            elif ch == "\x04":          # Ctrl+D — delete entry
                if total == 0:
                    continue
                _w(SHOW_CUR)
                r = records[idx]
                label = r["label"]
                print()
                print()
                ui.warn(f"Delete display entry {BOLD}{label}{RESET}?")
                ui.info(f"{DIM}This removes the preserved line from "
                        f"DisplayMapping.cpp.{RESET}")
                ui.info(f"{DIM}It will be regenerated with defaults "
                        f"on next generate, or{RESET}")
                ui.info(f"{DIM}omitted entirely if no segment map "
                        f"matches.{RESET}")
                print()
                ok = ui.confirm(f"Delete {label}?", default_yes=False)
                if ok:
                    # Remove the record's line from raw_lines
                    li = r["line_index"]
                    raw_lines[li] = ""
                    # Write back and re-parse
                    with open(filepath, "w", encoding="utf-8") as f:
                        f.writelines(raw_lines)
                    records, raw_lines = parse_display_mapping(filepath)
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
