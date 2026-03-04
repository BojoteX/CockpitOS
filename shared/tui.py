"""
CockpitOS -- Shared Terminal UI toolkit.

Common TUI functions used by all three CockpitOS tools:
Compiler, Label Creator, and Environment Setup.

Pure presentation layer.  Zero CockpitOS knowledge.
Could be dropped into any Python TUI project.
"""

import os
import sys
import time
import threading
import msvcrt
import ctypes

__all__ = [
    # ANSI constants
    "CYAN", "GREEN", "YELLOW", "RED", "BOLD", "DIM", "RESET", "REV",
    "HIDE_CUR", "SHOW_CUR", "ERASE_LN",
    # Scroll bar characters
    "_SCROLL_BLOCK", "_SCROLL_LIGHT",
    # Terminal setup
    "_enable_ansi",
    # Output helpers
    "_w", "cls", "cprint", "header",
    "success", "warn", "error", "info",
    "big_success", "big_fail",
    # Spinner
    "Spinner",
    # Interactive inputs
    "_RESET_SENTINEL", "text_input", "confirm",
    # Interactive pickers
    "pick_live", "toggle_pick",
    # Scroll utilities
    "scroll_bar_positions", "clamp_scroll",
]

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

# Scroll bar characters (used by Label Creator editors)
_SCROLL_BLOCK = "\u2588"    # full block
_SCROLL_LIGHT = "\u2591"    # light shade

# -----------------------------------------------------------------------------
# Terminal setup
# -----------------------------------------------------------------------------
def _enable_ansi():
    """Enable ANSI escape processing and UTF-8 codepage on Windows 10+."""
    kernel32 = ctypes.windll.kernel32
    h = kernel32.GetStdHandle(-11)          # STD_OUTPUT_HANDLE
    mode = ctypes.c_ulong()
    kernel32.GetConsoleMode(h, ctypes.byref(mode))
    kernel32.SetConsoleMode(h, mode.value | 0x0004)   # ENABLE_VIRTUAL_TERMINAL_PROCESSING
    # Switch console to UTF-8 so box-drawing and block characters render
    # correctly on Windows 10 conhost (default cp1252 can't encode them).
    os.system("chcp 65001 >nul 2>&1")
    try:
        sys.stdout.reconfigure(encoding='utf-8')
    except Exception:
        pass

_enable_ansi()

# -----------------------------------------------------------------------------
# Output helpers
# -----------------------------------------------------------------------------
def _w(text):
    """Write raw text to stdout (no newline)."""
    try:
        sys.stdout.write(text)
        sys.stdout.flush()
    except UnicodeEncodeError:
        sys.stdout.write(text.encode("ascii", "replace").decode())
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
# Interactive inputs
# -----------------------------------------------------------------------------
_RESET_SENTINEL = "__RESET__"

def text_input(prompt, default=None, mask=False):
    """ESC-aware single-line text input using msvcrt.

    Returns:
      str          -- the entered (or default) value
      None         -- ESC pressed (cancel)
      _RESET_SENTINEL -- Ctrl+D pressed (reset to defaults)

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
            if ch == "\x1b":            # ESC -- cancel
                print()
                return None
            elif ch == "\x04":          # Ctrl+D -- reset to defaults
                print()
                return _RESET_SENTINEL
            elif ch == "\r":            # Enter -- accept
                print()
                text = "".join(buf).strip()
                return text if text else default
            elif ch in ("\x08", "\x7f"):  # Backspace
                if buf:
                    buf.pop()
                    _w("\b \b")
            elif ch in ("\xe0", "\x00"):  # special key prefix -- discard
                msvcrt.getwch()
            elif ch >= " ":             # printable
                buf.append(ch)
                _w("*" if mask else ch)
    except KeyboardInterrupt:
        print()
        raise


def confirm(prompt, default_yes=True):
    """Y/n prompt. Returns True, False, or None (ESC)."""
    hint = "Y/n" if default_yes else "y/N"
    _w(f"\n  {prompt} [{hint}]: ")
    try:
        while True:
            ch = msvcrt.getwch()
            if ch == "\x1b":        # ESC -- go back
                print()
                return None
            elif ch == "\r":        # Enter -- use default
                print()
                return default_yes
            elif ch.lower() == "y":
                print("y")
                return True
            elif ch.lower() == "n":
                print("n")
                return False
    except KeyboardInterrupt:
        raise


# -----------------------------------------------------------------------------
# Interactive pickers
# -----------------------------------------------------------------------------
def pick_live(prompt, initial_options, scan_fn, interval=2.0, empty_message="No items found"):
    """Arrow-key picker that refreshes options via a background scan thread.

    scan_fn:  callable() -> [(label, value)]   -- called every `interval` seconds.
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

                elif ch == "\x1b":  # ESC -- go back
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
        if is_sel:
            mark = f"{GREEN}X{RESET}{REV}" if state[key] else " "
            text = f"[{mark}] {label}"
            return f"  {REV}   {text} {RESET}"
        else:
            mark = f"{GREEN}X{RESET}" if state[key] else " "
            text = f"[{mark}] {label}"
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

            elif ch == "\r":        # Enter -- toggle
                key = items[idx][1]
                state[key] = not state[key]
                # Redraw current row to update the checkbox
                _w(f"\033[{total - idx}A")
                _w(f"\r{ERASE_LN}{_render(idx, True)}")
                remaining = total - idx
                if remaining > 0:
                    _w(f"\033[{remaining}B")
                _w("\r")

            elif ch == "\x1b":      # ESC -- save and go back
                _w(SHOW_CUR)
                return state

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


# -----------------------------------------------------------------------------
# Scroll utilities (used by Label Creator editors)
# -----------------------------------------------------------------------------
def scroll_bar_positions(scroll, list_height, total):
    """Return (thumb_start, thumb_end) for the scroll thumb within list_height.

    Args:
        scroll:      current scroll offset (top visible row index)
        list_height: number of visible rows in the viewport
        total:       total number of rows (including off-screen)
    """
    if total <= list_height:
        return (0, list_height)
    # Thumb size: proportional to visible / total, minimum 1
    thumb = max(1, round(list_height * list_height / total))
    # Thumb position: proportional to scroll / max_scroll
    max_scroll = total - list_height
    if max_scroll <= 0:
        top = 0
    else:
        top = round(scroll * (list_height - thumb) / max_scroll)
    return (top, top + thumb)


def clamp_scroll(idx, scroll, list_height):
    """Return adjusted scroll value to keep idx visible in the viewport.

    Args:
        idx:         index of the item that must be visible
        scroll:      current scroll offset
        list_height: number of visible rows
    """
    if idx < scroll:
        return idx
    if idx >= scroll + list_height:
        return idx - list_height + 1
    return scroll
