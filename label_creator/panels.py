"""
CockpitOS Label Creator — Interactive panel selection.

Dual-pane TUI: available panels on the left, selected panels on the right.
The user can browse panels, see what controls each contains, and
add/remove panels from their selection.
"""

import os
import sys
import msvcrt
import ctypes

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
    parts = [f"{count}{tag}" for tag, count in sorted(types.items(), key=lambda x: -x[1])]
    return " ".join(parts)


# ---------------------------------------------------------------------------
# Interactive dual-pane selector
# ---------------------------------------------------------------------------
def select_panels(all_panels: list[str], aircraft_data: dict) -> list[str] | None:
    """Interactive dual-pane panel selector.

    Left pane:  available panels (not yet selected)
    Right pane: selected panels
    Current panel's controls shown at bottom.

    Controls:
      Up/Down     — navigate current pane
      Right/Enter — add panel to selected (when on left)
      Left/Del    — remove panel from selected (when on right)
      Tab         — switch between left and right pane
      Space       — preview panel controls
      Esc         — done / confirm selection
      Q           — quit without saving (returns None)

    Returns list of selected panel names, or None if cancelled.
    """
    selected = []
    available = list(all_panels)

    pane = 0        # 0 = left (available), 1 = right (selected)
    idx_left = 0
    idx_right = 0
    scroll_left = 0
    scroll_right = 0
    preview_panel = None    # name of panel being previewed

    cols, rows = _get_terminal_size()
    pane_width = (cols - 3) // 2    # 3 for divider
    list_height = rows - 10         # reserve for header, preview, footer
    if list_height < 5:
        list_height = 5

    def _current_list():
        return available if pane == 0 else selected

    def _current_idx():
        return idx_left if pane == 0 else idx_right

    def _current_scroll():
        return scroll_left if pane == 0 else scroll_right

    def _clamp():
        nonlocal idx_left, idx_right, scroll_left, scroll_right
        if available:
            idx_left = max(0, min(idx_left, len(available) - 1))
        else:
            idx_left = 0
        if selected:
            idx_right = max(0, min(idx_right, len(selected) - 1))
        else:
            idx_right = 0
        # Scroll clamping
        if idx_left < scroll_left:
            scroll_left = idx_left
        if idx_left >= scroll_left + list_height:
            scroll_left = idx_left - list_height + 1
        if idx_right < scroll_right:
            scroll_right = idx_right
        if idx_right >= scroll_right + list_height:
            scroll_right = idx_right - list_height + 1

    def _draw():
        nonlocal preview_panel
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        # Header
        _w(f"  {CYAN}{BOLD}{'=' * (cols - 4)}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  Panel Selector{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * (cols - 4)}{RESET}\n")

        # Column headers
        left_hdr = f" AVAILABLE ({len(available)})"
        right_hdr = f" SELECTED ({len(selected)})"
        left_style = f"{CYAN}{BOLD}" if pane == 0 else DIM
        right_style = f"{GREEN}{BOLD}" if pane == 1 else DIM
        _w(f"  {left_style}{left_hdr:<{pane_width}}{RESET}")
        _w(f" {DIM}|{RESET} ")
        _w(f"{right_style}{right_hdr:<{pane_width}}{RESET}\n")
        _w(f"  {DIM}{'-' * pane_width} | {'-' * pane_width}{RESET}\n")

        # Rows
        for row in range(list_height):
            # Left pane
            li = scroll_left + row
            if li < len(available):
                name = available[li]
                trunc = name[:pane_width - 5]
                is_cur = (pane == 0 and li == idx_left)
                if is_cur:
                    _w(f"  {REV} > {trunc:<{pane_width - 4}} {RESET}")
                    preview_panel = name
                else:
                    _w(f"     {trunc:<{pane_width - 4}} ")
            else:
                _w(f"  {' ' * pane_width}")

            _w(f"{DIM}|{RESET} ")

            # Right pane
            ri = scroll_right + row
            if ri < len(selected):
                name = selected[ri]
                trunc = name[:pane_width - 5]
                is_cur = (pane == 1 and ri == idx_right)
                if is_cur:
                    _w(f"{REV} > {trunc:<{pane_width - 4}} {RESET}")
                    preview_panel = name
                else:
                    _w(f"   {GREEN}{trunc:<{pane_width - 4}}{RESET} ")
            else:
                _w(f" {' ' * pane_width}")

            _w("\n")

        # Divider
        _w(f"  {DIM}{'=' * (cols - 4)}{RESET}\n")

        # Preview: show controls for the currently highlighted panel
        cur_list = _current_list()
        cur_idx = _current_idx()
        if cur_list and cur_idx < len(cur_list):
            panel_name = cur_list[cur_idx]
            cat_data = aircraft_data.get(panel_name, {})
            controls = []
            for ctrl_id, ctrl_val in sorted(cat_data.items()):
                if isinstance(ctrl_val, dict):
                    controls.append(ctrl_val)

            summary = _summarize(controls)
            _w(f"  {BOLD}Preview:{RESET} {CYAN}{panel_name}{RESET}")
            _w(f"  {DIM}({len(controls)} controls: {summary}){RESET}\n")

            # Show up to 4 lines of control details
            max_preview = min(len(controls), 4)
            for i in range(max_preview):
                ctrl = controls[i]
                ctype = ctrl.get("control_type", "?")
                desc = ctrl.get("description", "")
                ident = ctrl.get("identifier", "")
                tag = _short_type(ctype)
                line = f"    {DIM}[{tag:>4}]{RESET} {ident:<35} {DIM}{desc[:40]}{RESET}"
                _w(line + "\n")
            if len(controls) > max_preview:
                _w(f"    {DIM}... and {len(controls) - max_preview} more{RESET}\n")
        else:
            _w(f"  {DIM}(no panel selected){RESET}\n")

        # Footer
        _w(f"\n  {DIM}Tab=switch pane  Enter/Right=add  Del/Left=remove  Space=detail  Esc=done  Q=cancel{RESET}\n")

    # Initial draw
    _clamp()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "H":          # Up
                    if pane == 0:
                        idx_left = max(0, idx_left - 1)
                    else:
                        idx_right = max(0, idx_right - 1)
                elif ch2 == "P":        # Down
                    if pane == 0:
                        idx_left = min(len(available) - 1, idx_left + 1) if available else 0
                    else:
                        idx_right = min(len(selected) - 1, idx_right + 1) if selected else 0
                elif ch2 == "M":        # Right arrow — add panel
                    if pane == 0 and available:
                        panel = available[idx_left]
                        available.remove(panel)
                        selected.append(panel)
                        _clamp()
                    elif pane == 0:
                        pass
                    else:
                        pass    # Right on right pane: no-op
                elif ch2 == "K":        # Left arrow — remove panel
                    if pane == 1 and selected:
                        panel = selected[idx_right]
                        selected.remove(panel)
                        available.append(panel)
                        available.sort()
                        _clamp()
                    elif pane == 1:
                        pass
                    else:
                        pass    # Left on left pane: no-op
                elif ch2 == "S":        # Delete key
                    if pane == 1 and selected:
                        panel = selected[idx_right]
                        selected.remove(panel)
                        available.append(panel)
                        available.sort()
                        _clamp()
                _clamp()
                _draw()

            elif ch == "\t":            # Tab — switch pane
                pane = 1 - pane
                _draw()

            elif ch == "\r":            # Enter — add from left
                if pane == 0 and available:
                    panel = available[idx_left]
                    available.remove(panel)
                    selected.append(panel)
                    _clamp()
                    _draw()
                elif pane == 1:
                    # Enter on right pane: show full detail
                    _show_detail(aircraft_data, selected[idx_right] if selected else None, cols, rows)
                    _draw()

            elif ch == " ":             # Space — show detail
                cur_list = _current_list()
                cur_idx = _current_idx()
                if cur_list and cur_idx < len(cur_list):
                    _show_detail(aircraft_data, cur_list[cur_idx], cols, rows)
                    _draw()

            elif ch == "\x1b":          # Esc — done
                _w(SHOW_CUR)
                return selected

            elif ch.lower() == "q":     # Q — cancel
                _w(SHOW_CUR)
                return None

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def _show_detail(aircraft_data: dict, panel_name: str | None, cols: int, rows: int):
    """Full-screen detail view of a panel's controls."""
    if not panel_name:
        return

    os.system("cls" if os.name == "nt" else "clear")
    _w(HIDE_CUR)

    cat_data = aircraft_data.get(panel_name, {})
    controls = []
    for ctrl_id, ctrl_val in sorted(cat_data.items()):
        if isinstance(ctrl_val, dict):
            controls.append(ctrl_val)

    _w(f"  {CYAN}{BOLD}{'=' * (cols - 4)}{RESET}\n")
    _w(f"  {CYAN}{BOLD}  Panel: {panel_name}{RESET}\n")
    _w(f"  {CYAN}{BOLD}{'=' * (cols - 4)}{RESET}\n\n")

    summary = _summarize(controls)
    _w(f"  {BOLD}{len(controls)} controls{RESET}  {DIM}({summary}){RESET}\n\n")

    _w(f"  {BOLD}{'Type':<6} {'Identifier':<40} {'Description':<45} {'Inputs'}{RESET}\n")
    _w(f"  {DIM}{'-' * 6} {'-' * 40} {'-' * 45} {'-' * 10}{RESET}\n")

    max_lines = rows - 12
    for i, ctrl in enumerate(controls):
        if i >= max_lines:
            _w(f"\n  {DIM}... {len(controls) - max_lines} more controls (press any key to go back){RESET}")
            break

        ctype = ctrl.get("control_type", "?")
        ident = ctrl.get("identifier", "?")
        desc = ctrl.get("description", "")
        tag = _short_type(ctype)

        # Count input interfaces
        inputs = ctrl.get("inputs", [])
        input_types = [inp.get("interface", "?") for inp in inputs]
        input_str = ",".join(input_types) if input_types else "-"

        desc_trunc = desc[:45] if len(desc) > 45 else desc
        _w(f"  {YELLOW}{tag:<6}{RESET} {ident:<40} {DIM}{desc_trunc:<45}{RESET} {input_str}\n")

    if len(controls) <= max_lines:
        _w(f"\n  {DIM}(press any key to go back){RESET}")

    msvcrt.getwch()
    _w(SHOW_CUR)
