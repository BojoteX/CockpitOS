"""
CockpitOS -- Terminal UI toolkit (Compiler).

Imports shared TUI base from shared/tui.py and adds compiler-specific
pickers: pick(), menu_pick(), pick_filterable().
"""

import re
import msvcrt

# Re-export everything from the shared TUI base so existing
# `from ui import ...` statements in compiler/*.py keep working.
from tui import *  # noqa: F401,F403

# Also import underscore names explicitly for local use
from tui import _w, HIDE_CUR, SHOW_CUR, ERASE_LN, REV, RESET
from tui import CYAN, GREEN, RED, BOLD, DIM

# -----------------------------------------------------------------------------
# Compiler-specific pickers
# -----------------------------------------------------------------------------
def pick(prompt, options, default=None):
    """Arrow-key picker. Up/Down to move, Enter to select."""
    if not options:
        return None

    idx = 0
    if default is not None:
        for i, (_, v) in enumerate(options):
            if v == default:
                idx = i
                break

    total = len(options)
    print()
    cprint(BOLD, f"  {prompt}")
    cprint(DIM, "  (arrows to move, Enter to select, Esc to go back)")

    # Draw all rows
    _w(HIDE_CUR)
    for i, (label, _) in enumerate(options):
        if i == idx:
            _w(f"  {REV} > {label} {RESET}\n")
        else:
            _w(f"     {label}\n")

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
                    _w(f"\033[{total - old}A")
                    _w(f"\r{ERASE_LN}     {options[old][0]}")
                    # Redraw new row (highlight)
                    if idx > old:
                        _w(f"\033[{idx - old}B")
                    else:
                        _w(f"\033[{old - idx}A")
                    _w(f"\r{ERASE_LN}  {REV} > {options[idx][0]} {RESET}")
                    # Return cursor to bottom
                    remaining = total - idx
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")

            elif ch == "\x1b":      # ESC -- go back
                _w(SHOW_CUR)
                return None

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                return options[idx][1]
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


_SEP_COLOR = "\033[38;5;240m"       # subtle gray for separators


def menu_pick(items, initial=None):
    """Full-screen arrow-key menu with separators and styled rows.

    Each item is a tuple:
      ("label", "value", "style")   -- selectable row
      ("---",)                      -- plain separator (not selectable)
      ("---", "Title")              -- labeled separator (not selectable)
      ("",)                         -- blank spacer (not selectable)
      ("action_bar", [options])     -- horizontal action selector (single row)
          options: [("Label", "value"), ("Label", "value", "caption"), ...]

    Styles:
      "action"  -- bold green block
      "normal"  -- plain text
      "dim"     -- dimmed text
    """
    _BG_GREEN = "\033[42m"
    _WHITE    = "\033[97m"

    # Build display rows and map selectable indices
    rows = []           # entries of various types (see below)
    selectable = []     # indices into rows[] that are selectable

    for item in items:
        if len(item) == 1 and item[0] == "":
            rows.append("blank")                        # blank spacer row

        elif item[0] == "---":
            if len(item) >= 2:
                rows.append(("labeled_sep", item[1]))   # labeled separator
            else:
                rows.append(None)   # plain separator

        elif item[0] == "action_bar":
            # Horizontal action selector -- three rows: top border, content, bottom border
            bar_opts = []   # [(label, value, caption), ...]
            for opt in item[1]:
                lbl = opt[0]
                val = opt[1]
                cap = opt[2] if len(opt) >= 3 else ""
                bar_opts.append((lbl, val, cap))
            # Compute box width from content: each cell = len(label)+2, joined by 2-space gaps
            content_w = sum(len(o[0]) + 2 for o in bar_opts) + 2 * (len(bar_opts) - 1)
            rows.append(("action_bar_border", content_w, "top"))
            selectable.append(len(rows))
            # bar_sel stored as list so _render_row can mutate it
            rows.append(("action_bar", bar_opts, [0]))
            rows.append(("action_bar_border", content_w, "bottom"))

        else:
            label, value, style = item[0], item[1], item[2]
            caption = item[3] if len(item) >= 4 else ""
            cap_suf = f" {DIM}{caption}{RESET}" if caption else ""
            if style == "action":
                normal = f"     {GREEN}{BOLD}{label}{RESET}{cap_suf}"
                hilite = f"  {_BG_GREEN}{_WHITE}{BOLD} \u25b8 {label} {RESET}{cap_suf}"
            elif style == "danger":
                normal = f"     {RED}{label}{RESET}{cap_suf}"
                hilite = f"  \033[41m{_WHITE}{BOLD} \u25b8 {label} {RESET}{cap_suf}"
            elif style == "dim":
                normal = f"     {DIM}{label}{RESET}{cap_suf}"
                hilite = f"  {REV} \u25b8 {label} {RESET}{cap_suf}"
            else:
                normal = f"     {label}{cap_suf}"
                hilite = f"  {REV} \u25b8 {label} {RESET}{cap_suf}"
            selectable.append(len(rows))
            rows.append((normal, hilite, value))

    if not selectable:
        return None

    sel = 0                 # index into selectable[]
    if initial is not None:
        for si, ri in enumerate(selectable):
            entry = rows[ri]
            if isinstance(entry, tuple) and entry[0] == "action_bar":
                for bi, (_, val, _) in enumerate(entry[1]):
                    if val == initial:
                        sel = si
                        entry[2][0] = bi
                        break
            elif isinstance(entry, tuple) and len(entry) == 3 and entry[2] == initial:
                sel = si
                break

    total_rows = len(rows)

    def _render_action_bar(bar_opts, bar_sel_list, is_row_sel):
        """Render the horizontal action bar as segmented buttons inside box borders.

        Each option occupies a fixed-width cell so labels never shift.
        Cell: 1 (bracket/space) + label + 1 (bracket/space).
        """
        bs = bar_sel_list[0]
        parts = []
        for i, (lbl, _, cap) in enumerate(bar_opts):
            upper = lbl.upper()
            if is_row_sel:
                if i == bs:
                    parts.append(f"{GREEN}{BOLD}[{upper}]{RESET}")
                else:
                    parts.append(f" {DIM}{upper}{RESET} ")
            else:
                parts.append(f" {DIM}{upper}{RESET} ")
        joined = "  ".join(parts)
        return f"     {_SEP_COLOR}\u2502{RESET} {joined} {_SEP_COLOR}\u2502{RESET}"

    def _render_row(ri, is_sel):
        entry = rows[ri]
        if entry == "blank":
            return ""
        if entry is None:
            line = "\u2500" * 36
            return f"     {_SEP_COLOR}{line}{RESET}"
        if isinstance(entry, tuple) and entry[0] == "labeled_sep":
            label = entry[1]
            total_w = 36
            pad = total_w - len(label) - 2
            left = pad // 2
            right = pad - left
            dash = "\u2500"
            return f"     {_SEP_COLOR}{dash * left} {label} {dash * right}{RESET}"
        if isinstance(entry, tuple) and entry[0] == "action_bar_border":
            w = entry[1]
            hline = "\u2500" * (w + 2)
            if entry[2] == "top":
                return f"     {_SEP_COLOR}\u250c{hline}\u2510{RESET}"
            else:
                return f"     {_SEP_COLOR}\u2514{hline}\u2518{RESET}"
        if isinstance(entry, tuple) and entry[0] == "action_bar":
            return _render_action_bar(entry[1], entry[2], is_sel)
        normal, hilite, _ = entry
        return hilite if is_sel else normal

    def _redraw_single(ri, is_sel):
        """Move cursor to row ri, redraw it, return cursor to bottom."""
        _w(f"\033[{total_rows - ri}A")
        _w(f"\r{ERASE_LN}{_render_row(ri, is_sel)}")
        remaining = total_rows - ri
        if remaining > 0:
            _w(f"\033[{remaining}B")
        _w("\r")

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

                # --- Left / Right on action_bar ---
                cur_ri = selectable[sel]
                cur_entry = rows[cur_ri]
                is_bar = isinstance(cur_entry, tuple) and cur_entry[0] == "action_bar"

                if is_bar and ch2 in ("K", "M"):   # Left / Right
                    bar_opts = cur_entry[1]
                    bar_sel_list = cur_entry[2]
                    old_bs = bar_sel_list[0]
                    if ch2 == "K":       # Left
                        bar_sel_list[0] = (old_bs - 1) % len(bar_opts)
                    else:                # Right
                        bar_sel_list[0] = (old_bs + 1) % len(bar_opts)
                    if bar_sel_list[0] != old_bs:
                        _redraw_single(cur_ri, True)
                    continue

                # --- Up / Down ---
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
                entry = rows[selectable[sel]]
                if isinstance(entry, tuple) and entry[0] == "action_bar":
                    bs = entry[2][0]
                    return entry[1][bs][1]    # value of selected bar option
                _, _, value = entry
                return value
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def pick_filterable(prompt, options, default=None):
    """Arrow-key picker with type-to-filter for long lists.
    Type to narrow, Backspace to remove, Esc to clear filter, Enter to select."""
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
        _w(f"{ERASE_LN}  {DIM}(type to filter, arrows to move, Enter to select, Esc to go back){RESET}\n")
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

            elif ch == "\x1b":      # Escape -- clear filter, or go back
                if filter_text:
                    filter_text = ""
                    _apply()
                    _repaint()
                else:
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
