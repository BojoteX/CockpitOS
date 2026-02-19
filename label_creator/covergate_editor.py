"""
CockpitOS Label Creator -- CoverGates.h TUI editor.

Scrollable list of cover gate entries with add/edit/delete.
Guided picker flow for adding/editing entries from InputMapping.h.
Auto-saves on exit.
"""

import os
import re
import sys
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
# Parser — reads CoverGates.h
# ---------------------------------------------------------------------------
_GATE_RE = re.compile(
    r'\{\s*"(?P<action>[^"]+)"\s*,\s*'
    r'(?:(?:"(?P<release>[^"]+)")|(?P<null>nullptr))\s*,\s*'
    r'"(?P<cover>[^"]+)"\s*,\s*'
    r'CoverGateKind::(?P<kind>\w+)\s*,\s*'
    r'(?P<delay>\d+)\s*,\s*'
    r'(?P<close>\d+)\s*\}'
)


def parse_covergates(filepath):
    """Return list of covergate entry dicts from CoverGates.h."""
    if not os.path.exists(filepath):
        return []
    with open(filepath, "r", encoding="utf-8") as f:
        text = f.read()
    entries = []
    for m in _GATE_RE.finditer(text):
        d = m.groupdict()
        entries.append({
            "action":    d["action"],
            "release":   d.get("release"),   # None if nullptr
            "cover":     d["cover"],
            "kind":      d["kind"],
            "delay_ms":  int(d["delay"]),
            "close_ms":  int(d["close"]),
        })
    return entries


def count_covergates(filepath, input_filepath):
    """Return count of valid cover gate entries (labels must exist in InputMapping.h)."""
    entries = parse_covergates(filepath)
    valid = _get_all_input_labels(input_filepath)
    return len([e for e in entries if _entry_is_valid(e, valid)])


# ---------------------------------------------------------------------------
# Writer — writes CoverGates.h
# ---------------------------------------------------------------------------
def write_covergates(filepath, entries):
    """Write CoverGates.h with the given list of entry dicts."""
    lines = []
    lines.append("// CoverGates.h -- Per-label-set cover gate configuration")
    lines.append("// Defines selectors/buttons that are physically guarded by a cover.")
    lines.append("// Edit via the Label Creator tool or manually.")
    lines.append("")
    lines.append("#pragma once")
    lines.append("")
    lines.append("inline const CoverGateDef kCoverGates[] = {")
    for e in entries:
        rel = f'"{e["release"]}"' if e["release"] else "nullptr"
        lines.append(
            f'    {{ "{e["action"]}", {rel:<30s}, '
            f'"{e["cover"]}", '
            f'CoverGateKind::{e["kind"]}, '
            f'{e["delay_ms"]:>4}, {e["close_ms"]:>4} }},'
        )
    lines.append("};")
    lines.append("inline const unsigned kCoverGateCount = sizeof(kCoverGates) / sizeof(kCoverGates[0]);")
    lines.append("")
    with open(filepath, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


# ---------------------------------------------------------------------------
# InputMapping.h parser — full record extraction
# ---------------------------------------------------------------------------
_INPUT_FULL_RE = re.compile(
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


def _parse_inputs_full(input_filepath):
    """Parse InputMapping.h returning all entries (wired and unwired)."""
    if not os.path.exists(input_filepath):
        return []
    with open(input_filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()
    inputs = []
    for line in lines:
        m = _INPUT_FULL_RE.search(line)
        if not m:
            continue
        d = m.groupdict()
        inputs.append({
            "label":       d["label"],
            "source":      d["source"],
            "controlType": d["type"],
            "group":       int(d["group"]),
            "value":       int(d["value"]),
        })
    return inputs


# ---------------------------------------------------------------------------
# Filter helpers
# ---------------------------------------------------------------------------
def _get_selectors(inputs):
    """Return dict of group_id -> [positions sorted by value]. All sources."""
    groups = {}
    for inp in inputs:
        if inp["controlType"] == "selector" and inp["group"] > 0:
            groups.setdefault(inp["group"], []).append(inp)
    for g in groups:
        groups[g].sort(key=lambda x: x["value"])
    return groups


def _get_momentary_buttons(inputs):
    """Return momentary controls that are NOT covers. All sources."""
    return [inp for inp in inputs
            if inp["controlType"] == "momentary"
            and "COVER" not in inp["label"].upper()]


def _get_covers(inputs):
    """Return ALL momentary controls with COVER in the label (including unwired)."""
    return [inp for inp in inputs
            if inp["controlType"] == "momentary"
            and "COVER" in inp["label"].upper()]


def _get_all_input_labels(input_filepath):
    """Return set of ALL label strings in InputMapping.h (any controlType/source)."""
    if not os.path.exists(input_filepath):
        return set()
    with open(input_filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()
    labels = set()
    for line in lines:
        m = _INPUT_FULL_RE.search(line)
        if m:
            labels.add(m.group("label"))
    return labels


def _entry_is_valid(entry, valid_labels):
    """Check that all labels in a covergate entry exist in InputMapping.h."""
    if entry["action"] not in valid_labels:
        return False
    if entry["cover"] not in valid_labels:
        return False
    if entry["release"] is not None and entry["release"] not in valid_labels:
        return False
    return True


# ---------------------------------------------------------------------------
# Scrollable single-selection picker with type-to-filter
# ---------------------------------------------------------------------------
def _pick_from_list(title, items, pre_select=None):
    """Full-screen scrollable picker with type-to-filter.

    Returns selected item dict or None on Esc (when no filter is active).
    First Esc clears filter; second Esc cancels.
    """
    if not items:
        return None

    filter_text = ""
    filtered = list(items)
    idx = 0
    scroll = 0

    # Pre-select
    if pre_select:
        for i, item in enumerate(filtered):
            if item["label"] == pre_select:
                idx = i
                break

    cols, rows = _get_terminal_size()
    list_height = rows - 7      # header(3) + colhdr(1) + list + blank(1) + footer(1)
    if list_height < 3:
        list_height = 3

    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"

    _flash_timer  = [None]
    _flash_active = [False]

    # Column widths: label gets most space, source gets ~12 on the right
    src_w = 12
    fixed = 3 + src_w + 4 + 1   # prefix(3) + source + gap(4) + scrollbar(1)
    label_w = max(cols - fixed, 20)

    def _apply_filter():
        nonlocal filtered, idx
        if filter_text:
            ft = filter_text.lower()
            filtered = [item for item in items if ft in item["label"].lower()]
        else:
            filtered = list(items)
        if not filtered:
            idx = 0
        elif idx >= len(filtered):
            idx = len(filtered) - 1

    def _needs_scroll():
        return len(filtered) > list_height

    def _scroll_bar_positions():
        total = len(filtered)
        if total <= list_height:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / total))
        max_scroll = total - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    def _render_row(i, is_highlighted, scroll_char):
        item = filtered[i]
        lbl = item["label"][:label_w]
        src = item.get("source", "")[:src_w]

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG} {YELLOW}> {lbl:<{label_w}}  "
                    f"{src:<{src_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} "
                    f"{GREEN}{lbl:<{label_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{src:<{src_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        return (f"   {lbl:<{label_w}}  "
                f"{DIM}{src:<{src_w}}{RESET}"
                f" {DIM}{scroll_char}{RESET}")

    def _clamp_scroll():
        nonlocal scroll
        if not filtered:
            scroll = 0
            return
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

        total = len(filtered)

        # Header with optional filter display
        fdisp = f"  {DIM}filter: {RESET}{YELLOW}{filter_text}{RESET}" if filter_text else ""
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{RESET}{fdisp}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Column header
        _w(f"  {DIM}   {'Label':<{label_w}}  {'Source':<{src_w}}{RESET}\n")

        if total == 0:
            _w(f"\n  {DIM}No matches.{RESET}\n")
            for _ in range(list_height - 2):
                _w("\n")
        else:
            thumb_start, thumb_end = _scroll_bar_positions()
            for row in range(list_height):
                ri = scroll + row
                if _needs_scroll():
                    sc = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
                else:
                    sc = " "
                if ri < total:
                    _w(_render_row(ri, ri == idx, sc) + "\n")
                else:
                    _w(f"{'':>{cols - 1}}{DIM}{sc}{RESET}\n")

        # Footer
        if filter_text:
            counter = f"{total}/{len(items)} matches"
        else:
            counter = f"{len(items)} item{'s' if len(items) != 1 else ''}"
        _w(f"\n  {DIM}{counter}    type to filter  Enter=select  Esc=back{RESET}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if not filtered:
                    continue
                if ch2 == "H":          # Up
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                        continue
                    _clamp_scroll()
                    _draw()
                elif ch2 == "P":        # Down
                    if idx < len(filtered) - 1:
                        idx += 1
                    else:
                        _flash_row()
                        continue
                    _clamp_scroll()
                    _draw()
                elif ch2 == "I":        # Page Up
                    idx = max(0, idx - list_height)
                    _clamp_scroll()
                    _draw()
                elif ch2 == "Q":        # Page Down
                    idx = min(max(0, len(filtered) - 1), idx + list_height)
                    _clamp_scroll()
                    _draw()

            elif ch == "\r":            # Enter = select
                if filtered:
                    return filtered[idx]

            elif ch == "\x1b":          # Esc = clear filter or cancel
                if filter_text:
                    filter_text = ""
                    _apply_filter()
                    _clamp_scroll()
                    _draw()
                else:
                    return None

            elif ch == "\x08":          # Backspace = remove filter char
                if filter_text:
                    filter_text = filter_text[:-1]
                    _apply_filter()
                    _clamp_scroll()
                    _draw()

            elif ch.isprintable() and ch != "\t":
                filter_text += ch
                _apply_filter()
                _clamp_scroll()
                _draw()

    finally:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)


# ---------------------------------------------------------------------------
# Kind picker — Switch or Button
# ---------------------------------------------------------------------------
def _pick_kind(pre_select=None):
    """Pick Switch (Selector) or Button (Momentary). Returns kind string or None."""
    items = [
        {"label": "Switch (Selector)",    "source": "2-pos switch behind a cover"},
        {"label": "Button (Momentary)",   "source": "Push button behind a cover"},
    ]
    pre = None
    if pre_select == "Selector":
        pre = "Switch (Selector)"
    elif pre_select == "ButtonMomentary":
        pre = "Button (Momentary)"

    pick = _pick_from_list("What kind of covered control?", items, pre_select=pre)
    if pick is None:
        return None
    if pick["label"].startswith("Switch"):
        return "Selector"
    return "ButtonMomentary"


# ---------------------------------------------------------------------------
# Warning helper
# ---------------------------------------------------------------------------
def _show_warning(msg):
    """Display a warning and wait for keypress."""
    os.system("cls" if os.name == "nt" else "clear")
    _w(f"\n  {YELLOW}[!] {msg}{RESET}\n")
    _w(f"\n  {DIM}Press any key to continue...{RESET}")
    msvcrt.getwch()


# ---------------------------------------------------------------------------
# Guided add/edit flow
# ---------------------------------------------------------------------------
def _edit_entry(entry=None, input_filepath=None):
    """Guided step-by-step picker to create or edit a cover gate entry.

    Returns entry dict, or None if user cancelled at any step.
    """
    inputs = _parse_inputs_full(input_filepath)
    if not inputs:
        _show_warning("No controls found in InputMapping.h. Wire some controls first.")
        return None

    # --- Step 0: Pick kind ---
    pre_kind = entry["kind"] if entry else None
    kind = _pick_kind(pre_select=pre_kind)
    if kind is None:
        return None

    # Determine if pre-selection should carry over from existing entry
    # (only if kind hasn't changed)
    same_kind = entry and entry["kind"] == kind

    if kind == "Selector":
        return _flow_selector(inputs, entry if same_kind else None)
    else:
        return _flow_button(inputs, entry if same_kind else None)


def _flow_selector(inputs, entry):
    """Guided flow for adding/editing a Selector cover gate."""
    selector_groups = _get_selectors(inputs)
    if not selector_groups:
        _show_warning("No selector switches found in InputMapping.h.")
        return None

    # Build flat list of all selector positions for ON pick
    all_positions = []
    for group_id in sorted(selector_groups.keys()):
        all_positions.extend(selector_groups[group_id])

    # --- Step 1: Pick ON/active position ---
    pre_action = entry["action"] if entry else None
    on_pick = _pick_from_list(
        "Step 1/3: Select the ON/active position for the switch",
        all_positions, pre_select=pre_action)
    if on_pick is None:
        return None

    # --- Step 2: Determine OFF position ---
    same_group = selector_groups.get(on_pick["group"], [])
    remaining = [p for p in same_group if p["label"] != on_pick["label"]]

    if len(remaining) == 0:
        _show_warning(
            f"Selector group {on_pick['group']} has only one wired position. "
            "Cannot determine OFF position.")
        return None
    elif len(remaining) == 1:
        off_pick = remaining[0]     # auto-select
    else:
        pre_release = entry["release"] if entry else None
        off_pick = _pick_from_list(
            "Step 2/3: Select the OFF position for the switch",
            remaining, pre_select=pre_release)
        if off_pick is None:
            return None

    # --- Step 3: Pick cover ---
    covers = _get_covers(inputs)
    if not covers:
        _show_warning(
            "No cover controls found in InputMapping.h. "
            "A cover is a momentary control with 'COVER' in its label.")
        return None

    pre_cover = entry["cover"] if entry else None
    cover_pick = _pick_from_list(
        "Step 3/3: Select the cover for this switch",
        covers, pre_select=pre_cover)
    if cover_pick is None:
        return None

    return {
        "action":   on_pick["label"],
        "release":  off_pick["label"],
        "cover":    cover_pick["label"],
        "kind":     "Selector",
        "delay_ms": 500,
        "close_ms": 500,
    }


def _flow_button(inputs, entry):
    """Guided flow for adding/editing a ButtonMomentary cover gate."""
    buttons = _get_momentary_buttons(inputs)
    if not buttons:
        _show_warning("No momentary buttons found in InputMapping.h (excluding covers).")
        return None

    # --- Step 1: Pick the button ---
    pre_action = entry["action"] if entry else None
    button_pick = _pick_from_list(
        "Step 1/2: Select the button",
        buttons, pre_select=pre_action)
    if button_pick is None:
        return None

    # --- Step 2: Pick cover ---
    covers = _get_covers(inputs)
    if not covers:
        _show_warning(
            "No cover controls found in InputMapping.h. "
            "A cover is a momentary control with 'COVER' in its label.")
        return None

    pre_cover = entry["cover"] if entry else None
    cover_pick = _pick_from_list(
        "Step 2/2: Select the cover for this button",
        covers, pre_select=pre_cover)
    if cover_pick is None:
        return None

    return {
        "action":   button_pick["label"],
        "release":  None,
        "cover":    cover_pick["label"],
        "kind":     "ButtonMomentary",
        "delay_ms": 350,
        "close_ms": 300,
    }


# ---------------------------------------------------------------------------
# TUI editor — scrollable list with add/edit/delete
# ---------------------------------------------------------------------------
def edit_covergates(filepath, input_filepath, label_set_name="", aircraft_name=""):
    """Main entry point: scrollable list editor for CoverGates.h."""

    entries = parse_covergates(filepath)

    # Strip invalid entries (labels not in InputMapping.h = source of truth)
    valid_labels = _get_all_input_labels(input_filepath)
    before_count = len(entries)
    entries = [e for e in entries if _entry_is_valid(e, valid_labels)]
    if len(entries) != before_count:
        write_covergates(filepath, entries)

    original = [dict(e) for e in entries]      # deep copy for dirty check

    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    list_height = rows - 7      # header(3) + colhdr(1) + list + blank(1) + footer(1)
    if list_height < 5:
        list_height = 5

    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"

    _flash_timer  = [None]
    _flash_active = [False]

    def _needs_scroll():
        return len(entries) > list_height

    def _scroll_bar_positions():
        total = len(entries)
        if total <= list_height:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / total))
        max_scroll = total - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    # Column widths
    kind_w = 16
    delay_w = 10      # "500/500ms"
    # Fixed: prefix(3) + kind + delay + gaps(6) + scrollbar(1) + margin(1) = 3+kind+delay+8
    fixed = 3 + kind_w + delay_w + 8
    remaining = cols - fixed
    action_w = max(remaining * 2 // 5, 12)
    cover_w  = max(remaining - action_w, 12)

    def _render_row(i, is_highlighted, scroll_char):
        e = entries[i]
        action_trunc = e["action"][:action_w]
        cover_trunc  = f"cover:{e['cover']}"[:cover_w]
        kind_trunc   = e["kind"][:kind_w]
        delay_str    = f"{e['delay_ms']}/{e['close_ms']}ms"[:delay_w]

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG} {YELLOW}> {action_trunc:<{action_w}}  "
                    f"{kind_trunc:<{kind_w}}  {cover_trunc:<{cover_w}}  "
                    f"{delay_str:<{delay_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} "
                    f"{GREEN}{action_trunc:<{action_w}}{RESET}{_SEL_BG}  "
                    f"{GREEN}{kind_trunc:<{kind_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{cover_trunc:<{cover_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{delay_str:<{delay_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        return (f"   {GREEN}{action_trunc:<{action_w}}{RESET}  "
                f"{GREEN}{kind_trunc:<{kind_w}}{RESET}  "
                f"{DIM}{cover_trunc:<{cover_w}}{RESET}  "
                f"{DIM}{delay_str:<{delay_w}}{RESET}"
                f" {DIM}{scroll_char}{RESET}")

    def _clamp_scroll():
        nonlocal scroll, idx
        total = len(entries)
        if total == 0:
            idx = 0
            scroll = 0
            return
        if idx >= total:
            idx = total - 1
        if idx < 0:
            idx = 0
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

        total = len(entries)

        # Header
        parts = [label_set_name, aircraft_name]
        ctx = "  (" + ", ".join(p for p in parts if p) + ")" if any(parts) else ""
        title = f"Cover Gates{ctx}"
        counter = f"{total} gate{'s' if total != 1 else ''}"
        spacing = header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Column header
        _w(f"  {DIM}   {'Action':<{action_w}}  {'Kind':<{kind_w}}  "
           f"{'Cover':<{cover_w}}  {'Delay':<{delay_w}}{RESET}\n")

        if total == 0:
            _w(f"\n  {DIM}No cover gates defined. Press A to add one.{RESET}\n")
            for _ in range(list_height - 2):
                _w("\n")
        else:
            thumb_start, thumb_end = _scroll_bar_positions()
            for row in range(list_height):
                ri = scroll + row
                if _needs_scroll():
                    sc = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
                else:
                    sc = " "
                if ri < total:
                    _w(_render_row(ri, ri == idx, sc) + "\n")
                else:
                    _w(f"{'':>{cols - 1}}{DIM}{sc}{RESET}\n")

        _w(f"\n  {DIM}Enter=edit  A=add  D=delete  Esc=save & exit{RESET}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "H":          # Up
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                        continue
                    _clamp_scroll()
                    _draw()
                elif ch2 == "P":        # Down
                    if len(entries) > 0 and idx < len(entries) - 1:
                        idx += 1
                    else:
                        _flash_row()
                        continue
                    _clamp_scroll()
                    _draw()
                elif ch2 == "I":        # Page Up
                    idx = max(0, idx - list_height)
                    _clamp_scroll()
                    _draw()
                elif ch2 == "Q":        # Page Down
                    idx = min(max(0, len(entries) - 1), idx + list_height)
                    _clamp_scroll()
                    _draw()

            elif ch in ("a", "A"):      # Add new entry
                result = _edit_entry(None, input_filepath)
                if result:
                    entries.append(result)
                    idx = len(entries) - 1
                _clamp_scroll()
                _draw()

            elif ch == "\r":            # Enter = edit selected
                if entries:
                    result = _edit_entry(entries[idx], input_filepath)
                    if result:
                        entries[idx] = result
                _clamp_scroll()
                _draw()

            elif ch in ("d", "D", "\x04"):     # D or Ctrl+D = delete
                if entries:
                    _w(SHOW_CUR)
                    _w(f"\n\n  {YELLOW}Delete '{entries[idx]['action']}'? (y/N): {RESET}")
                    confirm = msvcrt.getwch()
                    if confirm in ("y", "Y"):
                        entries.pop(idx)
                    _clamp_scroll()
                    _draw()

            elif ch == "\x1b":          # Esc = save and exit
                break

    finally:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)

    # Save if changed
    if entries != original:
        write_covergates(filepath, entries)
