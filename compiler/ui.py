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
    cprint(DIM, "  (arrow keys to move, Enter to select)")

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

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                return options[idx][1]
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def menu_pick(items):
    """Full-screen arrow-key menu with separators and styled rows.

    Each item is a tuple:
      ("label", "value", "style")   — selectable row
      ("---",)                      — separator (not selectable)

    Styles:
      "action"  — bold green block
      "normal"  — plain text
      "dim"     — dimmed text
    """
    # Build display rows and map selectable indices
    rows = []           # (display_normal, display_highlighted, value_or_None)
    selectable = []     # indices into rows[] that are selectable

    for item in items:
        if item[0] == "---":
            rows.append(None)   # separator
        else:
            label, value, style = item
            if style == "action":
                normal = f"     {GREEN}{BOLD}{label}{RESET}"
                hilite = f"  \033[42m\033[97m{BOLD} \u25b8 {label} {RESET}"
            elif style == "dim":
                normal = f"     {DIM}{label}{RESET}"
                hilite = f"  {REV} \u25b8 {label} {RESET}"
            else:
                normal = f"     {label}"
                hilite = f"  {REV} \u25b8 {label} {RESET}"
            selectable.append(len(rows))
            rows.append((normal, hilite, value))

    if not selectable:
        return None

    sel = 0                 # index into selectable[]
    total_rows = len(rows)

    def _render_row(ri, is_sel):
        if rows[ri] is None:
            return f"     {DIM}\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500{RESET}"
        normal, hilite, _ = rows[ri]
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

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                _, _, value = rows[selectable[sel]]
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
        _w(f"{ERASE_LN}  {DIM}(type to filter, arrows to move, Enter to select){RESET}\n")
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

            elif ch == "\x1b":      # Escape — clear filter
                if filter_text:
                    filter_text = ""
                    _apply()
                    _repaint()

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

def confirm(prompt, default_yes=True):
    hint = "Y/n" if default_yes else "y/N"
    raw = input(f"\n  {prompt} [{hint}]: ").strip().lower()
    if not raw:
        return default_yes
    return raw in ("y", "yes")
