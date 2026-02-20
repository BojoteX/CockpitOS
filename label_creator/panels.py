"""
CockpitOS Label Creator — Interactive panel selection.

Three-level drill-down UI:
  Level 1: Toggle-list panel selector (checkboxes, scroll, boundary flash)
  Level 2: Scrollable control list for a panel (Label, Description, Type)
  Level 3: Individual control detail (inputs & outputs)

Navigation: Up/Down = move, Right = drill deeper, Left/Esc = back.
"""

import os
import sys
import msvcrt
import threading

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
_SEL_BG  = "\033[48;5;236m"    # subtle dark gray row highlight

_CONTROL_TYPE_ICONS = {
    "selector":              "SW",    # switch/selector
    "toggle_switch":         "SW",
    "limited_dial":          "DL",    # dial/pot
    "analog_dial":           "DL",
    "fixed_step_dial":       "DL",
    "led":                   "LED",
    "analog_gauge":          "GA",    # gauge
    "display":               "DSP",   # display
    "metadata":              "META",
    "action":                "ACT",
    "radio":                 "RAD",
}

def _w(text):
    sys.stdout.write(text)
    sys.stdout.flush()


def _get_terminal_size():
    """Return (cols, rows) of the terminal."""
    try:
        size = os.get_terminal_size()
        return size.columns, size.lines
    except Exception:
        return 120, 30


def _short_type(control_type: str) -> str:
    return _CONTROL_TYPE_ICONS.get(control_type, "?")


def _summarize(controls: list[dict]) -> str:
    """Build a compact one-line summary of a panel's controls."""
    types: dict[str, int] = {}
    for ctrl in controls:
        ct = ctrl.get("control_type", "unknown")
        tag = _short_type(ct)
        types[tag] = types.get(tag, 0) + 1
    parts = [f"({count}){tag}" for tag, count in sorted(types.items(), key=lambda x: -x[1])]
    return " ".join(parts)


def _get_controls(aircraft_data: dict, panel_name: str) -> list[dict]:
    """Extract sorted control dicts from aircraft data for a panel."""
    cat_data = aircraft_data.get(panel_name, {})
    controls = []
    for ctrl_id, ctrl_val in sorted(cat_data.items()):
        if isinstance(ctrl_val, dict):
            controls.append(ctrl_val)
    return controls


# ---------------------------------------------------------------------------
# Interactive toggle-list selector
# ---------------------------------------------------------------------------
def select_panels(all_panels: list[str], aircraft_data: dict,
                  pre_selected: list[str] | None = None,
                  aircraft_name: str = "",
                  label_set_name: str = "") -> list[str] | None:
    """Interactive toggle-list panel selector with type-to-filter.

    Each panel is shown as a checkbox row with a compact control summary.

    Controls:
      Up/Down     — navigate
      Right/Left  — open/close detail view of highlighted panel
      Enter       — toggle panel selection
      Type        — filter panels by name
      Backspace   — remove last filter character
      Esc         — clear filter (if active), or save & exit

    Args:
      all_panels:      sorted list of all available panel names.
      aircraft_data:   full aircraft JSON dict (panel_name -> controls).
      pre_selected:    optional list of panel names already selected.
      aircraft_name:   aircraft identifier shown in header.
      label_set_name:  label set name shown in header.

    Returns list of selected panel names.
    """
    if not all_panels:
        return []

    # Build state: {panel_name: bool}
    pre = set(pre_selected) if pre_selected else set()
    state = {name: (name in pre) for name in all_panels}

    # Pre-compute control counts and type summaries for each panel
    ctrl_counts = {}
    panel_summaries = {}
    for name in all_panels:
        controls = _get_controls(aircraft_data, name)
        ctrl_counts[name] = len(controls)
        panel_summaries[name] = _summarize(controls)

    total = len(all_panels)
    filter_text = ""
    filtered = list(all_panels)
    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    # Reserve: 3 header + 1 blank + 1 footer hint + 1 blank = 6 lines
    list_height = rows - 6
    if list_height < 5:
        list_height = 5

    # ── Scroll indicator chars ─────────────────────────────────────────
    _SCROLL_BLOCK = "\u2588"    # █  full block
    _SCROLL_LIGHT = "\u2591"    # ░  light shade

    def _apply_filter():
        nonlocal filtered, idx
        if filter_text:
            ft = filter_text.lower()
            filtered = [name for name in all_panels if ft in name.lower()]
        else:
            filtered = list(all_panels)
        if not filtered:
            idx = 0
        elif idx >= len(filtered):
            idx = len(filtered) - 1

    def _needs_scroll():
        return len(filtered) > list_height

    def _selected_count():
        return sum(1 for v in state.values() if v)

    def _scroll_bar_positions():
        """Return (start_row, end_row) for the scroll thumb within list_height."""
        ftotal = len(filtered)
        if ftotal <= list_height:
            return (0, list_height)
        # Thumb size: proportional to visible / total, minimum 1
        thumb = max(1, round(list_height * list_height / ftotal))
        # Thumb position: proportional to scroll / max_scroll
        max_scroll = ftotal - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    def _render_row(i, is_highlighted, scroll_char):
        """Render a single toggle row with scroll indicator on the right."""
        name = filtered[i]
        mark = f"{GREEN}X{RESET}" if state[name] else " "
        summary = panel_summaries[name]

        # Format: " > [X] Name                       2SW 3GA  ░"
        #   prefix = 2 (pointer) + 4 ([X] ) = 6 chars before name
        prefix_len = 6
        suffix_len = len(summary) + 2 + 3  # 3 = space + scroll_char + margin
        name_width = cols - prefix_len - suffix_len - 2
        if name_width < 20:
            name_width = 20
        trunc_name = name[:name_width]
        pad = name_width - len(trunc_name)

        # Pointer arrow for highlighted row, spaces otherwise
        # Bracket stays at the SAME column regardless of highlight
        # Background highlight spans the full row when selected
        # Yellow flash variant: entire row turns yellow when at boundary
        if is_highlighted and _flash_active[0]:
            flash_mark = f"{YELLOW}X" if state[name] else " "
            return (f"{_SEL_BG}{YELLOW} > [{flash_mark}] {trunc_name}"
                    f"{' ' * pad}  {summary}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            hi_mark = f"{GREEN}X{RESET}{_SEL_BG}" if state[name] else " "
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} [{hi_mark}] {trunc_name}"
                    f"{' ' * pad}  {DIM}{summary}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")
        else:
            pointer = "  "

        return (f" {pointer}[{mark}] {trunc_name}"
                f"{' ' * pad}  {DIM}{summary}{RESET}"
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

    # ── Boundary flash ─────────────────────────────────────────────────
    _flash_timer = [None]   # mutable container for timer reference
    _flash_active = [False] # when True, _draw renders highlighted row in yellow

    def _flash_row():
        """Briefly flash the current row yellow, then restore normal.

        Sets a flag so _draw() renders the highlighted row in yellow,
        then schedules a timer to clear the flag and redraw normally.
        This avoids fragile ANSI cursor-movement math entirely.
        """
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

    _header_w = cols - 4

    def _draw():
        """Full redraw of the selector screen."""
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        ftotal = len(filtered)

        # Header with optional filter display
        sel_count = _selected_count()
        parts = [label_set_name, aircraft_name]
        ctx = "  (" + ", ".join(p for p in parts if p) + ")" if any(parts) else ""
        title = f"Panel Selector{ctx}"
        if filter_text:
            right_text = f"filter: {filter_text}"
            right_ansi = f"{DIM}filter: {RESET}{YELLOW}{filter_text}"
        else:
            right_text = f"{sel_count} / {total} selected"
            right_ansi = f"{GREEN}{right_text}"
        spacing = _header_w - len(title) - len(right_text) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{RESET}{' ' * spacing}{right_ansi}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")

        # Compute scroll bar thumb positions
        thumb_start, thumb_end = _scroll_bar_positions()

        # Panel rows
        if ftotal == 0:
            _w(f"\n  {DIM}No matches.{RESET}\n")
            for _ in range(list_height - 2):
                _w("\n")
        else:
            for row in range(list_height):
                ri = scroll + row
                # Scroll indicator character for this row
                if _needs_scroll():
                    scroll_char = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
                else:
                    scroll_char = " "

                if ri < ftotal:
                    _w(_render_row(ri, ri == idx, scroll_char) + "\n")
                else:
                    _w(ERASE_LN + "\n")

        # Footer
        if filter_text:
            match_info = f"{ftotal}/{total} matches"
        else:
            match_info = f"{total} panels"
        filter_hint = f"  {DIM}\u2190=clear filter{RESET}" if filter_text else ""
        _w(f"\n  {DIM}{match_info}    type to filter  Enter=toggle  Esc=save & exit{RESET}{filter_hint}")

    # Initial draw
    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if not filtered:
                    continue
                old = idx
                if ch2 == "H":          # Up — clamp at top
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                elif ch2 == "P":        # Down — clamp at bottom
                    if idx < len(filtered) - 1:
                        idx += 1
                    else:
                        _flash_row()
                elif ch2 == "I":        # Page Up
                    idx = max(0, idx - list_height)
                elif ch2 == "Q":        # Page Down
                    idx = min(max(0, len(filtered) - 1), idx + list_height)
                elif ch2 == "M":        # Right — detail view (Level 2)
                    _show_detail(aircraft_data, filtered[idx], cols, rows,
                                 label_set_name=label_set_name,
                                 aircraft_name=aircraft_name)
                    _draw()
                    continue
                elif ch2 == "K":        # Left — clear filter
                    if filter_text:
                        filter_text = ""
                        _apply_filter()
                        _clamp_scroll()
                        _draw()
                    continue
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch == "\r":            # Enter — toggle
                if filtered:
                    name = filtered[idx]
                    state[name] = not state[name]
                    _draw()

            elif ch == "\x1b":          # Esc — always save & exit
                if _flash_timer[0]:
                    _flash_timer[0].cancel()
                _w(SHOW_CUR)
                return [name for name in all_panels if state[name]]

            elif ch == "\x08":          # Backspace — remove filter char
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

    except KeyboardInterrupt:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)
        raise


# ---------------------------------------------------------------------------
# Level 2 — Scrollable panel detail (list of controls)
# ---------------------------------------------------------------------------
def _show_detail(aircraft_data: dict, panel_name: str | None, cols: int, rows: int,
                 label_set_name: str = "", aircraft_name: str = ""):
    """Interactive scrollable detail view of a panel's controls.

    Level 2 in the drill-down hierarchy:
      Up/Down  — navigate controls
      Right    — drill into control detail (Level 3)
      Left/Esc — back to panel selector (Level 1)
    """
    if not panel_name:
        return

    controls = _get_controls(aircraft_data, panel_name)
    if not controls:
        return

    total = len(controls)
    idx = 0
    scroll = 0
    header_w = cols - 4

    # Reserve: 3 header + 1 summary + 1 blank + 1 col-header + 1 separator
    #        + list + 1 blank + 1 footer = 9 fixed lines
    list_height = rows - 9
    if list_height < 5:
        list_height = 5

    # Scroll indicator
    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"
    needs_scroll = total > list_height

    # Column widths — fixed so scroll bar stays at rightmost column
    # Layout: " > " (3) + label_w + " " + desc_w + " " + type_w + " " + scroll (1)
    type_w = 18
    label_w = 40
    desc_w = cols - 3 - label_w - 1 - 1 - type_w - 1 - 1
    if desc_w < 10:
        desc_w = 10

    # Boundary flash
    _flash_timer = [None]
    _flash_active = [False]

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

    def _render_ctrl_row(i, is_highlighted, scroll_char):
        ctrl = controls[i]
        ident = ctrl.get("identifier", "?")
        desc = ctrl.get("description", "")
        ctype = ctrl.get("control_type", "?")
        ident_trunc = ident[:label_w] if len(ident) > label_w else ident
        desc_trunc = desc[:desc_w] if len(desc) > desc_w else desc
        ctype_trunc = ctype[:type_w] if len(ctype) > type_w else ctype

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG} {YELLOW}> {ident_trunc:<{label_w}} "
                    f"{desc_trunc:<{desc_w}} {ctype_trunc:<{type_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} "
                    f"{YELLOW}{ident_trunc:<{label_w}}{RESET}{_SEL_BG} "
                    f"{DIM}{desc_trunc:<{desc_w}}{RESET}{_SEL_BG} "
                    f"{ctype_trunc:<{type_w}}"
                    f" {DIM}{scroll_char}{RESET}")

        return (f"   {YELLOW}{ident_trunc:<{label_w}}{RESET} "
                f"{DIM}{desc_trunc:<{desc_w}}{RESET} {ctype_trunc:<{type_w}}"
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

    def _draw():
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        # Header
        summary = _summarize(controls)
        parts = [label_set_name, aircraft_name]
        ctx = "  (" + ", ".join(p for p in parts if p) + ")" if any(parts) else ""
        title = f"{panel_name}{ctx}"
        counter = f"{total} controls - {summary}"
        spacing = header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Column headers (aligned with row content: 3-char pointer area + columns)
        _w(f" {BOLD}  {'Label':<{label_w}} {'Description':<{desc_w}} {'Type'}{RESET}\n")

        # Scroll bar
        thumb_start, thumb_end = _scroll_bar_positions()

        # Control rows
        for row in range(list_height):
            ri = scroll + row
            if needs_scroll:
                sc = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
            else:
                sc = " "
            if ri < total:
                _w(_render_ctrl_row(ri, ri == idx, sc) + "\n")
            else:
                _w(ERASE_LN + "\n")

        # Footer
        _w(f"\n  {DIM}\u2191\u2193=move  \u2192=expand  \u2190=back  Esc=back{RESET}")

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
                elif ch2 == "M":        # Right — drill into control (Level 3)
                    _show_control_detail(controls[idx], panel_name, cols, rows,
                                         label_set_name=label_set_name,
                                         aircraft_name=aircraft_name)
                    _draw()
                    continue
                elif ch2 == "K":        # Left — back to Level 1
                    if _flash_timer[0]:
                        _flash_timer[0].cancel()
                    _w(SHOW_CUR)
                    return
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch == "\x1b":          # Esc — back to Level 1
                if _flash_timer[0]:
                    _flash_timer[0].cancel()
                _w(SHOW_CUR)
                return

    except KeyboardInterrupt:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)
        raise


# ---------------------------------------------------------------------------
# Level 3 — Individual control detail (inputs & outputs)
# ---------------------------------------------------------------------------
def _show_control_detail(ctrl: dict, panel_name: str, cols: int, rows: int,
                         label_set_name: str = "", aircraft_name: str = ""):
    """Interactive detail view of a single control's inputs and outputs.

    Level 3 in the drill-down hierarchy:
      Up/Down  — scroll through inputs/outputs
      Left/Esc — back to panel detail (Level 2)
    """
    ident = ctrl.get("identifier", "?")
    ctype = ctrl.get("control_type", "?")
    desc = ctrl.get("description", "")
    inputs = ctrl.get("inputs", [])
    outputs = ctrl.get("outputs", [])
    header_w = cols - 4

    # Build a flat list of display rows: section headers + items
    # Each item is a tuple: (type, data)
    #   ("section", "Inputs (2)")
    #   ("input", {input dict})
    #   ("output", {output dict})
    #   ("empty", "No inputs")
    display_rows = []

    display_rows.append(("section", f"Inputs ({len(inputs)})"))
    if inputs:
        for inp in inputs:
            display_rows.append(("input", inp))
    else:
        display_rows.append(("empty", "No inputs defined"))

    display_rows.append(("section", f"Outputs ({len(outputs)})"))
    if outputs:
        for out in outputs:
            display_rows.append(("output", out))
    else:
        display_rows.append(("empty", "No outputs defined"))

    total = len(display_rows)
    idx = 0
    scroll = 0

    # Reserve: 3 header + 2 info lines + 1 blank + list + 1 blank + 1 footer = 8
    list_height = rows - 8
    if list_height < 5:
        list_height = 5

    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"
    needs_scroll = total > list_height

    _flash_timer = [None]
    _flash_active = [False]

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

    def _render_detail_row(i, is_highlighted, scroll_char):
        rtype, data = display_rows[i]
        content_w = cols - 8  # margins + pointer + scroll

        if rtype == "section":
            # Section headers are bold, not selectable-looking
            line = f"  {BOLD}{data}{RESET}"
            pad = content_w - len(data) + 2
            if pad < 0:
                pad = 0
            return f"{line}{' ' * pad} {DIM}{scroll_char}{RESET}"

        elif rtype == "empty":
            line = f"    {DIM}{data}{RESET}"
            pad = content_w - len(data)
            if pad < 0:
                pad = 0
            return f"{line}{' ' * pad} {DIM}{scroll_char}{RESET}"

        elif rtype == "input":
            iface = data.get("interface", "?")
            idesc = data.get("description", "")
            max_val = data.get("max_value", "")
            step = data.get("suggested_step", "")
            arg = data.get("argument", "")

            parts = [f"{iface}"]
            if idesc:
                parts.append(f"— {idesc}")
            extras = []
            if max_val != "":
                extras.append(f"max={max_val}")
            if step:
                extras.append(f"step={step}")
            if arg:
                extras.append(f"arg={arg}")
            if extras:
                parts.append(f"[{', '.join(extras)}]")
            text = "  " + " ".join(parts)
            trunc = text[:content_w]
            pad = content_w - len(trunc)

            if is_highlighted and _flash_active[0]:
                return (f"{_SEL_BG} {YELLOW}> {trunc}{RESET}"
                        f"{' ' * pad} {DIM}{scroll_char}{RESET}")
            elif is_highlighted:
                return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} {trunc}"
                        f"{' ' * pad} {DIM}{scroll_char}{RESET}")
            else:
                return (f"   {DIM}{trunc}{RESET}"
                        f"{' ' * pad} {DIM}{scroll_char}{RESET}")

        elif rtype == "output":
            addr = data.get("address", "?")
            odesc = data.get("description", "")
            mask = data.get("mask", "")
            shift = data.get("shift_by", "")
            max_val = data.get("max_value", "")
            otype = data.get("type", "")
            addr_id = data.get("address_mask_shift_identifier", "")

            parts = [f"addr={addr}"]
            if odesc:
                parts.append(f"— {odesc}")
            extras = []
            if mask != "":
                extras.append(f"mask={mask}")
            if shift != "":
                extras.append(f"shift={shift}")
            if max_val != "":
                extras.append(f"max={max_val}")
            if otype:
                extras.append(f"type={otype}")
            if extras:
                parts.append(f"[{', '.join(extras)}]")
            text = "  " + " ".join(parts)
            trunc = text[:content_w]
            pad = content_w - len(trunc)

            if is_highlighted and _flash_active[0]:
                return (f"{_SEL_BG} {YELLOW}> {trunc}{RESET}"
                        f"{' ' * pad} {DIM}{scroll_char}{RESET}")
            elif is_highlighted:
                return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} {trunc}"
                        f"{' ' * pad} {DIM}{scroll_char}{RESET}")
            else:
                return (f"   {DIM}{trunc}{RESET}"
                        f"{' ' * pad} {DIM}{scroll_char}{RESET}")

        return ""

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

    def _draw():
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        # Header
        l3_parts = [label_set_name, aircraft_name]
        l3_ctx = "  (" + ", ".join(p for p in l3_parts if p) + ")" if any(l3_parts) else ""
        l3_title = f"{ident}{l3_ctx}"
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {l3_title}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Info line
        _w(f"  {BOLD}Type:{RESET} {ctype}    "
           f"{BOLD}Description:{RESET} {desc}    "
           f"{DIM}Panel: {panel_name}{RESET}\n")
        _w("\n")

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
                _w(_render_detail_row(ri, ri == idx, sc) + "\n")
            else:
                _w(ERASE_LN + "\n")

        # Footer
        _w(f"\n  {DIM}\u2191\u2193=scroll  \u2190=back  Esc=back{RESET}")

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
                elif ch2 == "K":        # Left — back to Level 2
                    if _flash_timer[0]:
                        _flash_timer[0].cancel()
                    _w(SHOW_CUR)
                    return
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch == "\x1b":          # Esc — back to Level 2
                if _flash_timer[0]:
                    _flash_timer[0].cancel()
                _w(SHOW_CUR)
                return

    except KeyboardInterrupt:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)
        raise
