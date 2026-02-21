"""
CockpitOS — Terminal UI toolkit.

Pure presentation layer. Zero CockpitOS knowledge.
Could be dropped into any Python TUI project.
"""

import os
import sys
import re
import time
import threading
import msvcrt
import ctypes

# -----------------------------------------------------------------------------
# ANSI constants
# -----------------------------------------------------------------------------
CYAN     = "\033[96m"
GREEN    = "\033[92m"
YELLOW   = "\033[93m"
RED      = "\033[91m"
BOLD     = "\033[1m"
DIM      = "\033[2m"
RESET    = "\033[0m"
REV      = "\033[7m"          # reverse video for highlighted row
HIDE_CUR = "\033[?25l"
SHOW_CUR = "\033[?25h"
ERASE_LN = "\033[2K"

# -----------------------------------------------------------------------------
# Terminal setup
# -----------------------------------------------------------------------------
def _enable_ansi():
    """Enable ANSI escape processing on Windows 10+."""
    kernel32 = ctypes.windll.kernel32
    h = kernel32.GetStdHandle(-11)          # STD_OUTPUT_HANDLE
    mode = ctypes.c_ulong()
    kernel32.GetConsoleMode(h, ctypes.byref(mode))
    kernel32.SetConsoleMode(h, mode.value | 0x0004)   # ENABLE_VIRTUAL_TERMINAL_PROCESSING

_enable_ansi()

# -----------------------------------------------------------------------------
# Output helpers
# -----------------------------------------------------------------------------
def _w(text):
    """Write raw text to stdout (no newline)."""
    sys.stdout.write(text)
    sys.stdout.flush()

def cls():
    """Clear the terminal screen."""
    os.system("cls" if os.name == "nt" else "clear")

def cprint(color, text):
    try:
        print(f"{color}{text}{RESET}")
    except UnicodeEncodeError:
        print(f"{color}{text.encode('ascii', 'replace').decode()}{RESET}")

def header(text):
    cls()
    width = max(len(text) + 4, 56)
    cprint(CYAN, "=" * width)
    cprint(CYAN + BOLD, f"  {text}")
    cprint(CYAN, "=" * width)

def success(text):
    cprint(GREEN, f"  [OK] {text}")

def warn(text):
    cprint(YELLOW, f"  [!]  {text}")

def error(text):
    cprint(RED, f"  [X]  {text}")

def info(text):
    print(f"       {text}")

def big_success(title, details=None):
    """Big, unmissable success banner."""
    width = 60
    print()
    cprint(GREEN + BOLD, "  " + "=" * width)
    cprint(GREEN + BOLD, "  " + " " * width)
    cprint(GREEN + BOLD, f"    >>> {title} <<<".center(width + 2))
    cprint(GREEN + BOLD, "  " + " " * width)
    cprint(GREEN + BOLD, "  " + "=" * width)
    if details:
        for d in details:
            cprint(GREEN, f"    {d}")
    print()

def big_fail(title, details=None):
    """Big, unmissable failure banner."""
    width = 60
    print()
    cprint(RED + BOLD, "  " + "=" * width)
    cprint(RED + BOLD, "  " + " " * width)
    cprint(RED + BOLD, f"    XXX {title} XXX".center(width + 2))
    cprint(RED + BOLD, "  " + " " * width)
    cprint(RED + BOLD, "  " + "=" * width)
    if details:
        for d in details:
            cprint(RED, f"    {d}")
    print()

# -----------------------------------------------------------------------------
# Spinner
# -----------------------------------------------------------------------------
class Spinner:
    """Animated spinner that runs in a background thread."""
    FRAMES = ["|", "/", "-", "\\"]

    def __init__(self, message="Compiling"):
        self._message = message
        self._stop = threading.Event()
        self._thread = None
        self._start_time = 0

    def start(self):
        self._start_time = time.time()
        self._stop.clear()
        self._thread = threading.Thread(target=self._spin, daemon=True)
        self._thread.start()

    def _spin(self):
        i = 0
        while not self._stop.is_set():
            elapsed = time.time() - self._start_time
            mins, secs = divmod(int(elapsed), 60)
            ts = f"{mins}:{secs:02d}"
            frame = self.FRAMES[i % len(self.FRAMES)]
            print(f"\r  {CYAN}{frame}{RESET} {self._message}... {DIM}{ts}{RESET}   ", end="", flush=True)
            i += 1
            self._stop.wait(0.15)
        # Clear the spinner line
        print(f"\r{' ' * 60}\r", end="", flush=True)

    def stop(self):
        self._stop.set()
        if self._thread:
            self._thread.join()

# -----------------------------------------------------------------------------
# Interactive pickers
# -----------------------------------------------------------------------------
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

            elif ch == "\x1b":      # ESC — go back
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

_DELETE_SENTINEL = "__DELETE__"


def menu_pick(items, initial=None):
    """Full-screen arrow-key menu with separators and styled rows.

    Each item is a tuple:
      ("label", "value", "style")   — selectable row
      ("---",)                      — separator (not selectable)

    Styles:
      "action"  — bold green block (green bg + white text when selected)
      "warning" — bold yellow block (yellow bg + black text when selected)
      "danger"  — bold red block (red bg + white text when selected)
      "normal"  — plain text
      "dim"     — dimmed text

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

            elif ch == "\x1b":      # ESC — go back
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
                if ch2 == "K":        # Left — clear filter (always reachable)
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

            elif ch == "\x1b":      # Escape — always go back
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

def pick_live(prompt, initial_options, scan_fn, interval=2.0, empty_message="No items found"):
    """Arrow-key picker that refreshes options via a background scan thread.

    scan_fn:  callable() -> [(label, value)]   — called every `interval` seconds.
    Returns the selected value on Enter, or None on ESC.
    """
    SPINNER_FRAMES = ["|", "/", "-", "\\"]
    MAX_ROWS = 10

    options = list(initial_options)
    idx = 0
    lock = threading.Lock()
    stop_event = threading.Event()
    updated_event = threading.Event()
    spin_idx = [0]

    def _scanner():
        while not stop_event.is_set():
            stop_event.wait(interval)
            if stop_event.is_set():
                break
            try:
                new_opts = scan_fn()
            except Exception:
                continue
            with lock:
                nonlocal options, idx
                if new_opts != options:
                    old_val = options[idx][1] if idx < len(options) else None
                    options = new_opts
                    # Preserve selection by value
                    idx = 0
                    for i, (_, v) in enumerate(options):
                        if v == old_val:
                            idx = i
                            break
                    updated_event.set()

    def _draw():
        """Render the picker. Caller MUST hold `lock`."""
        _w(HIDE_CUR)
        frame = SPINNER_FRAMES[spin_idx[0] % len(SPINNER_FRAMES)]
        _w(f"\r{ERASE_LN}  {BOLD}{prompt}{RESET}\n")
        _w(f"{ERASE_LN}  {DIM}(arrows to move, Enter to select, Esc to go back){RESET}\n")
        if not options:
            _w(f"{ERASE_LN}     {DIM}{empty_message}{RESET}\n")
            for _ in range(MAX_ROWS - 1):
                _w(f"{ERASE_LN}\n")
        else:
            for slot in range(MAX_ROWS):
                _w(ERASE_LN)
                if slot < len(options):
                    label = options[slot][0]
                    if slot == idx:
                        _w(f"  {REV} > {label} {RESET}\n")
                    else:
                        _w(f"     {label}\n")
                else:
                    _w("\n")
        count = len(options)
        _w(f"{ERASE_LN}  {DIM}{count} found  {CYAN}{frame}{RESET}{DIM}  scanning...{RESET}\n")

    def _redraw():
        """Move cursor up and redraw. Caller MUST hold `lock`."""
        _w(f"\033[{MAX_ROWS + 3}A")
        _draw()

    # Initial draw
    print()
    with lock:
        _draw()

    # Start background scanner
    thread = threading.Thread(target=_scanner, daemon=True)
    thread.start()

    # Periodic spinner refresh (every 0.3s even if no new data)
    last_spin = time.time()

    try:
        while True:
            now = time.time()

            # Refresh spinner animation or background scan updates
            needs_redraw = False
            if now - last_spin >= 0.3:
                spin_idx[0] += 1
                last_spin = now
                needs_redraw = True
            if updated_event.is_set():
                updated_event.clear()
                needs_redraw = True
            if needs_redraw:
                with lock:
                    _redraw()

            # Non-blocking key check
            if msvcrt.kbhit():
                ch = msvcrt.getwch()
                if ch in ("\xe0", "\x00"):
                    ch2 = msvcrt.getwch()
                    with lock:
                        if not options:
                            continue
                        old = idx
                        if ch2 == "H":      # Up
                            idx = (idx - 1) % len(options)
                        elif ch2 == "P":    # Down
                            idx = (idx + 1) % len(options)
                        else:
                            continue
                        if old != idx:
                            _redraw()

                elif ch == "\x1b":  # ESC — go back
                    stop_event.set()
                    thread.join(timeout=3)
                    _w(SHOW_CUR)
                    return None

                elif ch == "\r":    # Enter
                    stop_event.set()
                    thread.join(timeout=3)
                    _w(SHOW_CUR)
                    with lock:
                        if options:
                            return options[idx][1]
                        return None
            else:
                time.sleep(0.05)    # prevent busy-wait

    except KeyboardInterrupt:
        stop_event.set()
        thread.join(timeout=3)
        _w(SHOW_CUR)
        raise


def confirm(prompt, default_yes=False):
    """Y/n prompt. Returns True, False, or None (ESC).

    Requires explicit 'Y' to confirm. Enter defaults to No.
    """
    _w(f"\n  {prompt} [y/N]: ")
    try:
        while True:
            ch = msvcrt.getwch()
            if ch == "\x1b":        # ESC — go back
                print()
                return None
            elif ch == "\r":        # Enter — always No (safe default)
                print("n")
                return False
            elif ch.lower() == "y":
                print("y")
                return True
            elif ch.lower() == "n":
                print("n")
                return False
    except KeyboardInterrupt:
        raise


_RESET_SENTINEL = "__RESET__"

def text_input(prompt, default=None, mask=False):
    """ESC-aware single-line text input using msvcrt.

    Returns:
      str          — the entered (or default) value
      None         — ESC pressed (cancel)
      _RESET_SENTINEL — Ctrl+D pressed (reset to defaults)

    If Enter is pressed with no input, returns *default* (which may be None).
    When *mask* is True, typed characters are echoed as '*'.
    """
    suffix = ""
    if default:
        suffix = f" [{('*' * min(len(default), 8)) if mask else default}]"
    _w(f"  {prompt}{suffix}: ")
    buf = []
    try:
        while True:
            ch = msvcrt.getwch()
            if ch == "\x1b":            # ESC — cancel
                print()
                return None
            elif ch == "\x04":          # Ctrl+D — reset to defaults
                print()
                return _RESET_SENTINEL
            elif ch == "\r":            # Enter — accept
                print()
                text = "".join(buf).strip()
                return text if text else default
            elif ch in ("\x08", "\x7f"):  # Backspace
                if buf:
                    buf.pop()
                    _w("\b \b")
            elif ch in ("\xe0", "\x00"):  # special key prefix — discard
                msvcrt.getwch()
            elif ch >= " ":             # printable
                buf.append(ch)
                _w("*" if mask else ch)
    except KeyboardInterrupt:
        print()
        raise


def toggle_pick(prompt, items):
    """Toggle picker. Arrow keys navigate, Enter toggles, ESC returns final state.

    items: [(label, key, current_bool), ...]
    Returns: {key: bool} with the final toggle states.
    """
    if not items:
        return {}

    state = {key: val for _, key, val in items}
    idx = 0
    total = len(items)

    def _render(i, is_sel):
        label, key, _ = items[i]
        mark = f"{GREEN}X{RESET}" if state[key] else " "
        text = f"[{mark}] {label}"
        if is_sel:
            return f"  {REV} {text} {RESET}"
        else:
            return f"     {text}"

    print()
    cprint(BOLD, f"  {prompt}")
    cprint(DIM, "  (arrows to move, Enter to toggle, Esc to save & go back)")

    _w(HIDE_CUR)
    for i in range(total):
        _w(_render(i, i == idx) + "\n")

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
                    # Redraw old row
                    _w(f"\033[{total - old}A")
                    _w(f"\r{ERASE_LN}{_render(old, False)}")
                    # Redraw new row
                    if idx > old:
                        _w(f"\033[{idx - old}B")
                    else:
                        _w(f"\033[{old - idx}A")
                    _w(f"\r{ERASE_LN}{_render(idx, True)}")
                    # Return cursor to bottom
                    remaining = total - idx
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")

            elif ch == "\r":        # Enter — toggle
                key = items[idx][1]
                state[key] = not state[key]
                # Redraw current row to update the checkbox
                _w(f"\033[{total - idx}A")
                _w(f"\r{ERASE_LN}{_render(idx, True)}")
                remaining = total - idx
                if remaining > 0:
                    _w(f"\033[{remaining}B")
                _w("\r")

            elif ch == "\x1b":      # ESC — save and go back
                _w(SHOW_CUR)
                return state

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise
