"""
CockpitOS -- Terminal UI toolkit (Label Creator).

Imports shared TUI base from shared/tui.py and adds label-creator-specific
pickers: pick(), menu_pick(), pick_filterable().
"""

import re
import msvcrt

# Re-export everything from the shared TUI base so existing
# `from ui import ...` statements in label_creator/*.py keep working.
from tui import *  # noqa: F401,F403

# Also import underscore names explicitly for local use
from tui import _w, HIDE_CUR, SHOW_CUR, ERASE_LN, REV, RESET
from tui import CYAN, GREEN, YELLOW, RED, BOLD, DIM
from tui import _RESET_SENTINEL

# -----------------------------------------------------------------------------
# Label-creator-specific pickers
# -----------------------------------------------------------------------------
_DELETE_SENTINEL = "__DELETE__"


def pick(prompt, options, default=None, on_help=None, descriptions=None,
         on_delete=None):
    """Arrow-key picker. Up/Down to move, Enter to select.

    If on_help is provided, pressing H calls on_help() and then redraws.

    If descriptions is provided, a dynamic description area below the options
    shows beginner-friendly help for the currently highlighted option.
    descriptions is a dict mapping option values to lists of pre-formatted
    display strings (may contain ANSI codes).

    If on_delete is provided, pressing Ctrl+D calls on_delete(value) with the
    currently highlighted option's value.  If it returns True the picker exits
    and returns the special sentinel ``_DELETE_SENTINEL``.
    """
    if not options:
        return None

    idx = 0
    if default is not None:
        for i, (_, v) in enumerate(options):
            if v == default:
                idx = i
                break

    total = len(options)

    # Description area setup
    has_desc = bool(descriptions)
    if has_desc:
        # Fixed height = max description length across all options (+ 1 separator)
        desc_height = max((len(descriptions.get(v, [])) for _, v in options), default=0)
        desc_total = 1 + desc_height    # separator line + description lines
    else:
        desc_total = 0

    # Total lines below the cursor's "bottom" reference point:
    # all option rows + description area (if any)
    bottom_offset = total + desc_total

    def _draw_desc():
        """Render the separator + description area for the current idx."""
        if not has_desc:
            return
        value = options[idx][1]
        lines = descriptions.get(value, [])
        dash = "\u2500" * 50
        _w(f"{ERASE_LN}  {DIM}{dash}{RESET}\n")
        for di in range(desc_height):
            _w(ERASE_LN)
            if di < len(lines):
                _w(f"  {lines[di]}\n")
            else:
                _w("\n")

    def _redraw_desc():
        """Move cursor from bottom to description area, redraw, return to bottom."""
        if not has_desc:
            return
        # Cursor is at bottom (after all options + desc area).
        # Move up desc_total lines to reach the separator.
        _w(f"\033[{desc_total}A")
        _w("\r")
        _draw_desc()
        # Cursor is now back at bottom after _draw_desc prints desc_total lines
        _w("\r")

    def _draw_picker():
        print()
        cprint(BOLD, f"  {prompt}")
        hint = "  (arrows to move, Enter to select, Esc to go back"
        if on_help:
            hint += ", H=help"
        hint += ")"
        cprint(DIM, hint)
        if on_delete:
            _w(f"  {RED}Ctrl+D{RESET}{DIM} = delete selected{RESET}\n")

        _w(HIDE_CUR)
        for i, (label, _) in enumerate(options):
            if i == idx:
                _w(f"  {REV} > {label} {RESET}\n")
            else:
                _w(f"     {label}\n")

        _draw_desc()

    _draw_picker()

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                old = idx
                if ch2 == "H":          # Up
                    idx = (idx - 1) % total
                elif ch2 == "P":        # Down
                    idx = (idx + 1) % total
                else:
                    continue
                if old != idx:
                    # Redraw old row (un-highlight)
                    _w(f"\033[{bottom_offset - old}A")
                    _w(f"\r{ERASE_LN}     {options[old][0]}")
                    # Redraw new row (highlight)
                    if idx > old:
                        _w(f"\033[{idx - old}B")
                    else:
                        _w(f"\033[{old - idx}A")
                    _w(f"\r{ERASE_LN}  {REV} > {options[idx][0]} {RESET}")
                    # Return cursor to bottom
                    remaining = bottom_offset - idx
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")
                    # Update description area
                    _redraw_desc()

            elif ch == "\x1b":      # ESC -- go back
                _w(SHOW_CUR)
                return None

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                return options[idx][1]

            elif ch == "\x04" and on_delete:  # Ctrl+D
                _w(SHOW_CUR)
                val = options[idx][1]
                on_delete(val)
                return _DELETE_SENTINEL

            elif ch in ("h", "H") and on_help:
                _w(SHOW_CUR)
                on_help()
                _draw_picker()
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def menu_pick(items, initial=None):
    """Full-screen arrow-key menu with separators and styled rows.

    Each item is a tuple:
      ("label", "value", "style")   -- selectable row
      ("---",)                      -- separator (not selectable)

    Styles:
      "action"  -- bold green block (green bg + white text when selected)
      "warning" -- bold yellow block (yellow bg + black text when selected)
      "danger"  -- bold red block (red bg + white text when selected)
      "normal"  -- plain text
      "dim"     -- dimmed text

    initial:
      If provided, the menu starts with the cursor on the item whose
      value matches *initial*.  Falls back to the first item if not found.
    """
    # Build display rows and map selectable indices
    rows = []           # (display_normal, display_highlighted, value_or_None)
    selectable = []     # indices into rows[] that are selectable

    # Pre-compute caption alignment: find the longest label among items
    # that have captions, so captions align in a column.
    max_label_w = 0
    for item in items:
        if item[0] not in ("---", "") and len(item) >= 4 and item[3]:
            max_label_w = max(max_label_w, len(item[0]))
    # Separator width: based on longest label+caption or minimum 40
    sep_w = max(max_label_w + 24, 40)

    for item in items:
        if item[0] == "---":
            if len(item) >= 2:
                rows.append(("labeled_sep", item[1], sep_w))   # labeled separator
            else:
                rows.append(("plain_sep", sep_w))   # plain separator
        elif item[0] == "":
            rows.append(("blank",))         # blank spacer line
        else:
            label, value, style = item[0], item[1], item[2]
            caption = item[3] if len(item) >= 4 else ""
            # Align captions: pad label to max_label_w when captions exist
            if caption and max_label_w > 0:
                pad_label = label.ljust(max_label_w + 2)
                cap_suf = f" {DIM}{caption}{RESET}"
            elif caption:
                pad_label = label
                cap_suf = f" {DIM}{caption}{RESET}"
            else:
                pad_label = label
                cap_suf = ""
            if style == "action":
                normal = f"     {GREEN}{BOLD}{pad_label}{RESET}{cap_suf}"
                hilite = f"  \033[42m\033[97m{BOLD} \u25b8 {pad_label}{RESET}{cap_suf}"
            elif style == "warning":
                normal = f"     {YELLOW}{BOLD}{pad_label}{RESET}{cap_suf}"
                hilite = f"  \033[43m\033[30m \u25b8 {pad_label}{RESET}{cap_suf}"
            elif style == "danger":
                normal = f"     {RED}{BOLD}{pad_label}{RESET}{cap_suf}"
                hilite = f"  \033[41m\033[97m{BOLD} \u25b8 {pad_label}{RESET}{cap_suf}"
            elif style == "dim":
                normal = f"     {DIM}{pad_label}{RESET}{cap_suf}"
                hilite = f"  {REV} \u25b8 {pad_label}{RESET}{cap_suf}"
            else:
                normal = f"     {pad_label}{cap_suf}"
                hilite = f"  {REV} \u25b8 {pad_label}{RESET}{cap_suf}"
            selectable.append(len(rows))
            rows.append((normal, hilite, value))

    if not selectable:
        return None

    # Resolve initial cursor position
    sel = 0                 # index into selectable[]
    if initial is not None:
        for si, ri in enumerate(selectable):
            entry = rows[ri]
            if isinstance(entry, tuple) and len(entry) == 3 and entry[2] == initial:
                sel = si
                break
    total_rows = len(rows)

    def _render_row(ri, is_sel):
        entry = rows[ri]
        if entry is None:
            return ""
        if isinstance(entry, tuple) and entry[0] == "blank":
            return ""
        if isinstance(entry, tuple) and entry[0] == "plain_sep":
            w = entry[1]
            dash = "\u2500"
            return f"     {DIM}{dash * w}{RESET}"
        if isinstance(entry, tuple) and entry[0] == "labeled_sep":
            label = entry[1]
            w = entry[2]
            pad = w - len(label) - 2
            left = pad // 2
            right = pad - left
            dash = "\u2500"
            return f"     {DIM}{dash * left} {label} {dash * right}{RESET}"
        normal, hilite, _ = entry
        return hilite if is_sel else normal

    # Initial draw
    _w(HIDE_CUR)
    for ri in range(total_rows):
        is_sel = (ri == selectable[sel])
        _w(_render_row(ri, is_sel) + "\n")

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                old_sel = sel
                if ch2 == "H":          # Up
                    sel = (sel - 1) % len(selectable)
                elif ch2 == "P":        # Down
                    sel = (sel + 1) % len(selectable)
                else:
                    continue
                if old_sel != sel:
                    old_ri = selectable[old_sel]
                    new_ri = selectable[sel]
                    # Redraw old row (un-highlight)
                    _w(f"\033[{total_rows - old_ri}A")
                    _w(f"\r{ERASE_LN}{_render_row(old_ri, False)}")
                    # Redraw new row (highlight)
                    if new_ri > old_ri:
                        _w(f"\033[{new_ri - old_ri}B")
                    else:
                        _w(f"\033[{old_ri - new_ri}A")
                    _w(f"\r{ERASE_LN}{_render_row(new_ri, True)}")
                    # Return cursor to bottom
                    remaining = total_rows - new_ri
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")

            elif ch == "\x1b":      # ESC -- go back
                _w(SHOW_CUR)
                return None

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                _, _, value = rows[selectable[sel]]
                return value
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def pick_filterable(prompt, options, default=None):
    """Arrow-key picker with type-to-filter for long lists.
    Type to narrow, Backspace to remove, Left to clear filter, Enter to select."""
    if not options:
        return None

    filter_text = ""
    filtered = list(options)
    idx = 0
    total_slots = min(len(options), 20)  # max visible rows

    if default is not None:
        for i, (_, v) in enumerate(filtered):
            if v == default:
                idx = i
                break

    _ansi_re = re.compile(r'\033\[[0-9;]*m')

    def _apply():
        nonlocal filtered, idx
        if filter_text:
            ft = filter_text.lower()
            filtered = [(l, v) for l, v in options if ft in _ansi_re.sub("", l).lower()]
        else:
            filtered = list(options)
        if idx >= len(filtered):
            idx = max(0, len(filtered) - 1)

    # Compute viewport offset so selected item is visible
    def _viewport():
        if not filtered:
            return 0
        if idx < total_slots:
            return 0
        return idx - total_slots + 1

    def _draw_full():
        _w(HIDE_CUR)
        fdisp = f"  filter: {filter_text}" if filter_text else ""
        _w(f"{ERASE_LN}  {BOLD}{prompt}{RESET}{fdisp}\n")
        filt_hint = f"  {DIM}\u2190=clear filter{RESET}" if filter_text else ""
        _w(f"{ERASE_LN}  {DIM}(type to filter, arrows to move, Enter to select, Esc to go back){RESET}{filt_hint}\n")
        vp = _viewport()
        for slot in range(total_slots):
            _w(ERASE_LN)
            ri = vp + slot
            if ri < len(filtered):
                label = filtered[ri][0]
                if ri == idx:
                    _w(f"  {REV} > {label} {RESET}\n")
                else:
                    _w(f"     {label}\n")
            else:
                _w("\n")
        count_str = f"  {DIM}{len(filtered)}/{len(options)} matches{RESET}"
        if len(filtered) > total_slots:
            count_str += f"  {DIM}(scroll with arrows){RESET}"
        _w(f"{ERASE_LN}{count_str}\n")

    def _repaint():
        # Move cursor back to top of our block: 2 header + total_slots + 1 count
        _w(f"\033[{total_slots + 3}A")
        _draw_full()

    _draw_full()

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "K":        # Left -- clear filter (always reachable)
                    if filter_text:
                        filter_text = ""
                        _apply()
                        _repaint()
                    continue
                if not filtered:
                    continue
                old = idx
                if ch2 == "H":          # Up
                    idx = (idx - 1) % len(filtered)
                elif ch2 == "P":        # Down
                    idx = (idx + 1) % len(filtered)
                else:
                    continue
                if old != idx:
                    _repaint()

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                if filtered:
                    return filtered[idx][1]

            elif ch == "\x1b":      # Escape -- always go back
                _w(SHOW_CUR)
                return None

            elif ch == "\x08":      # Backspace
                if filter_text:
                    filter_text = filter_text[:-1]
                    _apply()
                    _repaint()

            elif ch.isprintable() and ch != "\t":
                filter_text += ch
                _apply()
                _repaint()
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise
