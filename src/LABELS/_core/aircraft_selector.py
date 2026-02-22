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
    """Interactive filterable picker for aircraft JSONs.

    Type to filter the list, arrows to navigate, Enter to select, Esc to cancel.
    Left arrow clears the filter, Backspace removes last character.

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
    all_options = []
    for fname in sorted(os.listdir(aircraft_dir)):
        if fname.lower().endswith(".json"):
            name = os.path.splitext(fname)[0]
            all_options.append(name)

    if not all_options:
        print(f"ERROR: No aircraft JSON files found in {aircraft_dir}")
        return None

    if len(all_options) == 1:
        print(f"\n  Only one aircraft available: {CYAN}{all_options[0]}{RESET}")
        return all_options[0]

    # State
    filter_text = ""
    filtered = list(all_options)
    idx = 0
    total_slots = min(len(all_options), 20)  # max visible rows

    # Pre-highlight current aircraft
    if current:
        for i, name in enumerate(filtered):
            if name == current:
                idx = i
                break

    def _apply():
        nonlocal filtered, idx
        if filter_text:
            ft = filter_text.lower()
            filtered = [n for n in all_options if ft in n.lower()]
        else:
            filtered = list(all_options)
        if idx >= len(filtered):
            idx = max(0, len(filtered) - 1)

    def _viewport():
        if not filtered:
            return 0
        if idx < total_slots:
            return 0
        return idx - total_slots + 1

    def _draw_full():
        _w(HIDE_CUR)
        fdisp = f"  filter: {filter_text}" if filter_text else ""
        _w(f"{ERASE_LN}  {BOLD}Select the aircraft for this LABEL_SET:{RESET}{fdisp}\n")
        filt_hint = f"  {DIM}\u2190=clear filter{RESET}" if filter_text else ""
        _w(f"{ERASE_LN}  {DIM}(type to filter, arrows to move, Enter to select, Esc to cancel){RESET}{filt_hint}\n")
        vp = _viewport()
        for slot in range(total_slots):
            _w(ERASE_LN)
            ri = vp + slot
            if ri < len(filtered):
                name = filtered[ri]
                if ri == idx:
                    _w(f"  {REV} > {name} {RESET}\n")
                else:
                    _w(f"     {name}\n")
            else:
                _w("\n")
        count_str = f"  {DIM}{len(filtered)}/{len(all_options)} matches{RESET}"
        if len(filtered) > total_slots:
            count_str += f"  {DIM}(scroll with arrows){RESET}"
        _w(f"{ERASE_LN}{count_str}\n")

    def _repaint():
        # Move cursor back to top of block: 2 header + total_slots + 1 count
        _w(f"\033[{total_slots + 3}A")
        _draw_full()

    print()
    _draw_full()

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "K":        # Left — clear filter
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
                    print(f"\n  {GREEN}Selected: {filtered[idx]}{RESET}")
                    return filtered[idx]

            elif ch == "\x1b":      # Esc
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
