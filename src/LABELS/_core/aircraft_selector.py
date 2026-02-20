#!/usr/bin/env python3
"""
CockpitOS — Aircraft Selector (standalone TUI picker).

Self-contained arrow-key picker for selecting an aircraft JSON
from _core/aircraft/. Used by reset_core.py and generator_core.py.

Pattern borrowed from compiler/ui.py pick() but kept standalone
so _core scripts have no external dependencies.
"""
import os, sys
try:
    import msvcrt, ctypes       # Windows-only (TUI picker)
except ImportError:
    msvcrt = None               # Linux/CI — load_aircraft_json still works
    ctypes = None

# ── ANSI constants ──────────────────────────────────────────────────────────
BOLD     = "\033[1m"
DIM      = "\033[2m"
RESET    = "\033[0m"
REV      = "\033[7m"
CYAN     = "\033[96m"
GREEN    = "\033[92m"
HIDE_CUR = "\033[?25l"
SHOW_CUR = "\033[?25h"
ERASE_LN = "\033[2K"

def _enable_ansi():
    """Enable ANSI escape processing on Windows 10+."""
    try:
        kernel32 = ctypes.windll.kernel32
        h = kernel32.GetStdHandle(-11)
        mode = ctypes.c_ulong()
        kernel32.GetConsoleMode(h, ctypes.byref(mode))
        kernel32.SetConsoleMode(h, mode.value | 0x0004)
    except Exception:
        pass  # non-Windows or old terminal

_enable_ansi()

def _w(text):
    sys.stdout.write(text)
    sys.stdout.flush()


def select_aircraft(core_dir, current=None):
    """Interactive arrow-key picker for aircraft JSONs.

    Args:
        core_dir:  Path to _core/ directory (contains aircraft/ subdir).
        current:   Previously selected aircraft name (pre-highlighted).

    Returns:
        Aircraft name (str, e.g. "FA-18C_hornet") or None if user presses Esc.
    """
    aircraft_dir = os.path.join(core_dir, "aircraft")
    if not os.path.isdir(aircraft_dir):
        print(f"ERROR: Aircraft directory not found: {aircraft_dir}")
        return None

    # Discover available aircraft JSONs
    options = []
    for fname in sorted(os.listdir(aircraft_dir)):
        if fname.lower().endswith(".json"):
            name = os.path.splitext(fname)[0]
            options.append(name)

    if not options:
        print(f"ERROR: No aircraft JSON files found in {aircraft_dir}")
        return None

    if len(options) == 1:
        # Only one aircraft available — auto-select
        print(f"\n  Only one aircraft available: {CYAN}{options[0]}{RESET}")
        return options[0]

    # Find default index
    idx = 0
    if current:
        for i, name in enumerate(options):
            if name == current:
                idx = i
                break

    total = len(options)

    print()
    _w(f"  {BOLD}Select the aircraft for this LABEL_SET:{RESET}\n")
    _w(f"  {DIM}(arrows to move, Enter to select, Esc to cancel){RESET}\n")

    # Draw all rows
    _w(HIDE_CUR)
    for i, name in enumerate(options):
        if i == idx:
            _w(f"  {REV} > {name} {RESET}\n")
        else:
            _w(f"     {name}\n")

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                old = idx
                if ch2 == "H":        # Up
                    idx = (idx - 1) % total
                elif ch2 == "P":      # Down
                    idx = (idx + 1) % total
                else:
                    continue
                if old != idx:
                    # Redraw old row
                    _w(f"\033[{total - old}A")
                    _w(f"\r{ERASE_LN}     {options[old]}")
                    # Redraw new row
                    if idx > old:
                        _w(f"\033[{idx - old}B")
                    else:
                        _w(f"\033[{old - idx}A")
                    _w(f"\r{ERASE_LN}  {REV} > {options[idx]} {RESET}")
                    # Return cursor to bottom
                    remaining = total - idx
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")

            elif ch == "\x1b":    # Esc
                _w(SHOW_CUR)
                return None

            elif ch == "\r":      # Enter
                _w(SHOW_CUR)
                print(f"\n  {GREEN}Selected: {options[idx]}{RESET}")
                return options[idx]
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


def load_aircraft_json(core_dir, aircraft_name):
    """Load and return parsed aircraft JSON from _core/aircraft/.

    Args:
        core_dir:       Path to _core/ directory.
        aircraft_name:  Aircraft name (e.g. "FA-18C_hornet").

    Returns:
        (json_filename, data_dict) or (None, None) if not found.
    """
    json_path = os.path.join(core_dir, "aircraft", aircraft_name + ".json")
    if not os.path.isfile(json_path):
        return None, None
    try:
        import json
        with open(json_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        return aircraft_name + ".json", data
    except Exception as e:
        print(f"ERROR: Could not load {json_path}: {e}")
        return None, None
