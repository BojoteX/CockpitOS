"""
CockpitOS Label Creator -- InputMapping.h TUI editor.

Scrollable list of InputMapping records with per-record editing.
Only user-editable fields are modified: source, port, bit, hidId.
Hash tables, struct definitions, and TrackedSelectorLabels are untouched.
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
# Input source options for the picker
# ---------------------------------------------------------------------------
INPUT_SOURCES = [
    ("NONE (not wired)", "NONE"),
    ("GPIO",             "GPIO"),
    ("HC165",            "HC165"),
    ("TM1637",           "TM1637"),
    ("PCA_0x20",         "PCA_0x20"),
    ("PCA_0x21",         "PCA_0x21"),
    ("PCA_0x22",         "PCA_0x22"),
    ("PCA_0x23",         "PCA_0x23"),
    ("PCA_0x24",         "PCA_0x24"),
    ("PCA_0x25",         "PCA_0x25"),
    ("PCA_0x26",         "PCA_0x26"),
    ("PCA_0x27",         "PCA_0x27"),
    ("PCA_0x5A",         "PCA_0x5A"),
    ("PCA_0x5B",         "PCA_0x5B"),
]


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
# Parser — reads InputMapping.h into (records, raw_lines)
# ---------------------------------------------------------------------------
_LINE_RE = re.compile(
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


def parse_input_mapping(filepath):
    """Parse InputMapping.h into (records, raw_lines).

    Each record dict:
      label, source, port (str), bit (int), hidId (int),
      cmd, value (int), controlType, group (int), line_index (int)
    """
    with open(filepath, "r", encoding="utf-8") as f:
        raw_lines = f.readlines()

    records = []
    for i, line in enumerate(raw_lines):
        m = _LINE_RE.search(line)
        if not m:
            continue
        d = m.groupdict()
        records.append({
            "label":       d["label"],
            "source":      d["source"],
            "port":        d["port"].strip(),     # string to preserve PIN() / constants
            "bit":         int(d["bit"]),
            "hidId":       int(d["hidId"]),
            "cmd":         d["cmd"],
            "value":       int(d["value"]),
            "controlType": d["type"],
            "group":       int(d["group"]),
            "line_index":  i,
        })
    return records, raw_lines


# ---------------------------------------------------------------------------
# Write-back — only replaces record lines, everything else untouched
# ---------------------------------------------------------------------------
def _fmt_port(port_str):
    """Format port value for aligned output.

    Tries to format as right-aligned integer; falls back to raw string
    for PIN() macros and named constants.
    """
    try:
        return f"{int(port_str):>2}"
    except ValueError:
        return port_str


def write_input_mapping(filepath, records, raw_lines):
    """Write updated records back into raw_lines and save to disk.

    Only the lines at each record's line_index are replaced.
    Everything else (header, struct def, hash table, TrackedSelectorLabels)
    stays byte-for-byte untouched.
    """
    if not records:
        with open(filepath, "w", encoding="utf-8") as f:
            f.writelines(raw_lines)
        return

    # Compute alignment widths across all records
    max_label = max(len(r["label"]) for r in records)
    max_cmd   = max(len(r["cmd"])   for r in records)
    max_type  = max(len(r["controlType"]) for r in records)

    for r in records:
        lblf = f'"{r["label"]}"'.ljust(max_label + 2)
        cmdf = f'"{r["cmd"]}"'.ljust(max_cmd + 2)
        ctf  = f'"{r["controlType"]}"'.ljust(max_type + 2)
        port_fmt = _fmt_port(r["port"])
        val_str  = f'{r["value"]:>5}'

        new_line = (
            f'    {{ {lblf}, "{r["source"]}" , {port_fmt} , '
            f'{r["bit"]:>2} , {r["hidId"]:>3} , '
            f'{cmdf}, {val_str} , {ctf}, {r["group"]:>2} }},\n'
        )
        raw_lines[r["line_index"]] = new_line

    with open(filepath, "w", encoding="utf-8") as f:
        f.writelines(raw_lines)


# ---------------------------------------------------------------------------
# Quick count helper (for menu caption)
# ---------------------------------------------------------------------------
def count_wired(filepath):
    """Return (wired, total) counts from an InputMapping.h file."""
    try:
        records, _ = parse_input_mapping(filepath)
    except Exception:
        return (0, 0)
    wired = sum(1 for r in records if r["source"] != "NONE")
    return (wired, len(records))


# ---------------------------------------------------------------------------
# Edit flow helpers
# ---------------------------------------------------------------------------
def _edit_standalone(record):
    """Edit a standalone (non-selector-group) record.

    Prompts: source -> port -> bit -> hidId.
    If source set to NONE, auto-clears port/bit/hidId.
    Returns True if record was modified.
    """
    label = record["label"]
    ctype = record["controlType"]

    ui.header(f"Edit: {label}")
    print()
    ui.info(f"Type: {ctype}    Command: {record['cmd']}")
    ui.info(f"Current: source={record['source']}  port={record['port']}  "
            f"bit={record['bit']}  hidId={record['hidId']}")
    print()

    # Pick source
    src = ui.pick("Source:", INPUT_SOURCES, default=record["source"])
    if src is None:
        return False

    if src == "NONE":
        record["source"] = "NONE"
        record["port"]   = "0"
        record["bit"]    = 0
        record["hidId"]  = -1
        return True

    record["source"] = src

    # Port
    port_val = ui.text_input("Port (pin number or PIN(x))", default=record["port"])
    if port_val is None:
        return False
    record["port"] = port_val.strip()

    # Bit
    bit_val = ui.text_input("Bit", default=str(record["bit"]))
    if bit_val is None:
        return False
    try:
        record["bit"] = int(bit_val)
    except ValueError:
        record["bit"] = 0

    # HID ID
    hid_val = ui.text_input("HID ID (-1 = none)", default=str(record["hidId"]))
    if hid_val is None:
        return False
    try:
        record["hidId"] = int(hid_val)
    except ValueError:
        record["hidId"] = -1

    return True


def _edit_selector_group(records, group_records):
    """Edit a selector group — all positions sharing the same group ID.

    Shows all positions, user picks source once, assigns port once,
    then assigns bit per position (-1 = fallback).
    Returns True if any record was modified.
    """
    cmd = group_records[0]["cmd"]
    group_id = group_records[0]["group"]

    ui.header(f"Edit Selector Group: {cmd}")
    print()
    ui.info(f"Group {group_id} — {len(group_records)} positions")
    print()

    # Show current state of all positions
    for i, r in enumerate(group_records):
        suffix = r["label"].replace(cmd + "_", "") if r["label"].startswith(cmd + "_") else r["label"]
        status = f"{GREEN}wired{RESET}" if r["source"] != "NONE" else f"{DIM}unwired{RESET}"
        print(f"    {i+1}. {suffix:<20} src={r['source']:<10} port={r['port']:<8} "
              f"bit={r['bit']:>3}  hid={r['hidId']:>3}  [{status}]")

    print()

    # Pick source for the group
    current_src = group_records[0]["source"]
    src = ui.pick("Source for this group:", INPUT_SOURCES, default=current_src)
    if src is None:
        return False

    if src == "NONE":
        for r in group_records:
            r["source"] = "NONE"
            r["port"]   = "0"
            r["bit"]    = 0
            r["hidId"]  = -1
        return True

    # Port (shared across all positions)
    current_port = group_records[0]["port"]
    port_val = ui.text_input("Port (shared for all positions)", default=current_port)
    if port_val is None:
        return False
    port_val = port_val.strip()

    # Apply source + port to all
    for r in group_records:
        r["source"] = src
        r["port"]   = port_val

    # Bit per position
    print()
    ui.info("Assign bit for each position (-1 = fallback/default position):")
    for r in group_records:
        suffix = r["label"].replace(cmd + "_", "") if r["label"].startswith(cmd + "_") else r["label"]
        bit_val = ui.text_input(f"  {suffix} bit", default=str(r["bit"]))
        if bit_val is None:
            return False
        try:
            r["bit"] = int(bit_val)
        except ValueError:
            r["bit"] = 0

    # HID per position (optional, ask once if they want to set it)
    print()
    set_hid = ui.confirm("Set HID IDs for each position?")
    if set_hid:
        for r in group_records:
            suffix = r["label"].replace(cmd + "_", "") if r["label"].startswith(cmd + "_") else r["label"]
            hid_val = ui.text_input(f"  {suffix} HID ID", default=str(r["hidId"]))
            if hid_val is None:
                break
            try:
                r["hidId"] = int(hid_val)
            except ValueError:
                r["hidId"] = -1

    return True


# ---------------------------------------------------------------------------
# Scrollable TUI list view
# ---------------------------------------------------------------------------
def edit_input_mapping(filepath, label_set_name="", aircraft_name=""):
    """Main entry point: scrollable TUI editor for InputMapping.h.

    Follows panels.py _show_detail() pattern: full-redraw, msvcrt keys,
    scroll bar, boundary flash.
    """
    records, raw_lines = parse_input_mapping(filepath)
    if not records:
        ui.header("Input Mapping")
        print()
        ui.warn("No records found in InputMapping.h")
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
        return sum(1 for r in records if r["source"] != "NONE")

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
    # Layout: " > " (3) + label + source + port + bit + hid + type + scroll
    label_w = max(len(r["label"]) for r in records)
    label_w = max(label_w, 10)
    label_w = min(label_w, 35)
    src_w   = 10
    port_w  = 10
    bit_w   = 4
    hid_w   = 4
    type_w  = 15

    def _render_row(i, is_highlighted, scroll_char):
        r = records[i]
        label_trunc = r["label"][:label_w]
        src_trunc   = r["source"][:src_w]
        port_trunc  = r["port"][:port_w]
        bit_str     = str(r["bit"])
        hid_str     = str(r["hidId"])
        type_trunc  = r["controlType"][:type_w]

        wired = r["source"] != "NONE"

        if is_highlighted and _flash_active[0]:
            return (f" {YELLOW}> {label_trunc:<{label_w}}  "
                    f"{src_trunc:<{src_w}}  {port_trunc:<{port_w}}  "
                    f"{bit_str:>{bit_w}}  {hid_str:>{hid_w}}  "
                    f"{type_trunc:<{type_w}}"
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
                f"{color}{src_trunc:<{src_w}}{RESET}  "
                f"{color}{port_trunc:<{port_w}}{RESET}  "
                f"{color}{bit_str:>{bit_w}}{RESET}  "
                f"{color}{hid_str:>{hid_w}}{RESET}  "
                f"{DIM}{type_trunc:<{type_w}}{RESET}"
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
        title = f"Input Mapping{ctx}"
        counter = f"{wired} / {total} wired"
        spacing = header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Column header
        _w(f" {BOLD}  {'Label':<{label_w}}  {'Source':<{src_w}}  "
           f"{'Port':<{port_w}}  {'Bit':>{bit_w}}  {'HID':>{hid_w}}  "
           f"{'Type':<{type_w}}{RESET}\n")

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
                    modified = _do_edit(records, idx)
                    if modified:
                        write_input_mapping(filepath, records, raw_lines)
                        # Re-parse to stay consistent
                        records, raw_lines = parse_input_mapping(filepath)
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
                modified = _do_edit(records, idx)
                if modified:
                    write_input_mapping(filepath, records, raw_lines)
                    records, raw_lines = parse_input_mapping(filepath)
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


def _do_edit(records, idx):
    """Dispatch to the appropriate edit flow for the record at idx.

    For selector records with group > 0, gathers all positions in the
    same group and edits them together.
    For standalone records, edits individually.

    Returns True if any record was modified.
    """
    record = records[idx]
    ctype = record["controlType"]
    group = record["group"]

    if ctype == "selector" and group > 0:
        # Gather all positions in this group
        group_records = [r for r in records if r["group"] == group]
        return _edit_selector_group(records, group_records)
    else:
        return _edit_standalone(record)
