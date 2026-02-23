#!/usr/bin/env python3
"""
CockpitOS DCS Bulk Command Sender
==================================

Send multiple DCS-BIOS commands in sequence with optional delay between each.

Two modes:
  CLI:         Pass commands as arguments (original behavior)
  Interactive: No arguments — pick aircraft, browse controls, build a chain

Usage:
    python SEND_bulk_CommandTester.py                                          # Interactive
    python SEND_bulk_CommandTester.py "CMD1 ARG" "CMD2 ARG"                   # CLI
    python SEND_bulk_CommandTester.py --ip 192.168.1.100 "BATTERY_SW 2"       # CLI + IP
    python SEND_bulk_CommandTester.py --debounce 100 "WING_FOLD_PULL 0" ...   # CLI + debounce

Author: CockpitOS Project
"""

import os
import sys
import json
import argparse
import socket
import time
import msvcrt
import ctypes
import threading
import re as _re

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════════
# TERMINAL SETUP
# ═══════════════════════════════════════════════════════════════════════════════

def _enable_ansi():
    """Enable ANSI escape processing on Windows 10+."""
    kernel32 = ctypes.windll.kernel32
    h = kernel32.GetStdHandle(-11)
    mode = ctypes.c_ulong()
    kernel32.GetConsoleMode(h, ctypes.byref(mode))
    kernel32.SetConsoleMode(h, mode.value | 0x0004)

_enable_ansi()

# ═══════════════════════════════════════════════════════════════════════════════
# DEPENDENCY CHECK
# ═══════════════════════════════════════════════════════════════════════════════

try:
    import ifaddr
except ImportError:
    print()
    print("+----------------------------------------------------------------+")
    print("|  ERROR: Missing Required Module: ifaddr                        |")
    print("+----------------------------------------------------------------+")
    print("|                                                                |")
    print("|  This tool requires 'ifaddr' for network interface discovery.  |")
    print("|                                                                |")
    print("|      pip install ifaddr                                        |")
    print("|                                                                |")
    print("+----------------------------------------------------------------+")
    print()
    input("\nPress <ENTER> to exit...")
    sys.exit(1)

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

DCS_PORT = 7778

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SKETCH_DIR = os.path.dirname(SCRIPT_DIR)
AIRCRAFT_DIR = os.path.join(SKETCH_DIR, "src", "LABELS", "_core", "aircraft")

DEFAULT_DEBOUNCE_MS = 50

# ═══════════════════════════════════════════════════════════════════════════════
# ANSI CONSTANTS
# ═══════════════════════════════════════════════════════════════════════════════

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

_SCROLL_BLOCK = "\u2588"
_SCROLL_LIGHT = "\u2591"

_ANSI_RE = _re.compile(r'\033\[[0-9;]*[A-Za-z]')

def _w(s):
    sys.stdout.write(s)
    sys.stdout.flush()

def _visible_len(text):
    return len(_ANSI_RE.sub('', text))

def _cls():
    os.system("cls" if os.name == "nt" else "clear")

def _get_terminal_size():
    try:
        size = os.get_terminal_size()
        return size.columns, size.lines
    except Exception:
        return 120, 30

def _box_line(text):
    """Print a box line with perfect right-side alignment. Inner width = 64 visible chars."""
    visible = _ANSI_RE.sub('', text)
    pad = 64 - len(visible)
    if pad < 0:
        pad = 0
    print(f"|{text}{' ' * pad}|")

# ═══════════════════════════════════════════════════════════════════════════════
# NETWORK INTERFACE DISCOVERY
# ═══════════════════════════════════════════════════════════════════════════════

def shorten_adapter_name(name):
    """Shorten long Windows adapter names to something readable."""
    replacements = [
        ("Network Adapter", ""),
        ("Dual Band Simultaneous (DBS)", ""),
        ("Wi-Fi 7", "WiFi7"),
        ("Wi-Fi 6", "WiFi6"),
        ("Wi-Fi", "WiFi"),
        ("Qualcomm FastConnect 7800", "Qualcomm"),
        ("Intel(R)", "Intel"),
        ("Realtek", "Realtek"),
        ("Virtual", "Virt"),
        ("Ethernet", "Eth"),
        ("Wireless", "WiFi"),
        ("  ", " "),
    ]
    result = name
    for old, new in replacements:
        result = result.replace(old, new)
    return result.strip()[:35]


def get_all_interface_ips():
    """Discover all usable IPv4 interface addresses. Returns list of (name, ip)."""
    interfaces = []
    for adapter in ifaddr.get_adapters():
        for ip in adapter.ips:
            if isinstance(ip.ip, str):
                if ip.ip.startswith('127.') or ip.ip.startswith('169.254.') or ip.ip.startswith('192.168.137.'):
                    continue
                short_name = shorten_adapter_name(adapter.nice_name)
                interfaces.append((short_name, ip.ip))
    return interfaces

# ═══════════════════════════════════════════════════════════════════════════════
# ARROW-KEY PICKER
# ═══════════════════════════════════════════════════════════════════════════════

def arrow_pick(prompt, options):
    """Arrow-key picker. options = list of (label, value). Returns value or None on Esc."""
    if not options:
        return None

    total = len(options)
    idx = 0

    print()
    _w(f"  {BOLD}{prompt}{RESET}\n")
    _w(f"  {DIM}(Up/Down to move, Enter to select, Esc to go back){RESET}\n")

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
                    _w(f"\033[{total - old}A")
                    _w(f"\r{ERASE_LN}     {options[old][0]}")
                    if idx > old:
                        _w(f"\033[{idx - old}B")
                    else:
                        _w(f"\033[{old - idx}A")
                    _w(f"\r{ERASE_LN}  {REV} > {options[idx][0]} {RESET}")
                    remaining = total - idx
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                return options[idx][1]

            elif ch == "\x1b":      # Esc
                _w(SHOW_CUR)
                return None

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        sys.exit(0)


def pick_interface(interfaces):
    """Arrow-key picker for network interfaces. First real NIC is default."""
    options = []
    for name, ip in interfaces:
        options.append((f"{ip:<17} {DIM}({name}){RESET}", ip))
    options.append(("Localhost (127.0.0.1)", "127.0.0.1"))

    result = arrow_pick("Select network interface:", options)
    return result if result else (interfaces[0][1] if interfaces else "127.0.0.1")

# ═══════════════════════════════════════════════════════════════════════════════
# AIRCRAFT DATA
# ═══════════════════════════════════════════════════════════════════════════════

def list_aircraft():
    """Return sorted list of (display_name, filepath) for all aircraft JSONs."""
    results = []
    if not os.path.isdir(AIRCRAFT_DIR):
        return results
    for fname in sorted(os.listdir(AIRCRAFT_DIR)):
        if fname.endswith(".json"):
            display = fname[:-5]
            results.append((display, os.path.join(AIRCRAFT_DIR, fname)))
    return results


def load_aircraft_json(filepath):
    """Load aircraft JSON, return the full dict."""
    with open(filepath, "r", encoding="utf-8") as f:
        return json.load(f)


def collect_input_controls(aircraft_data):
    """Flatten aircraft JSON into a list of controls that have inputs."""
    items = []
    for panel, controls in sorted(aircraft_data.items()):
        if not isinstance(controls, dict):
            continue
        for ctrl_label, ctrl in sorted(controls.items()):
            if not isinstance(ctrl, dict):
                continue
            inputs = ctrl.get("inputs", [])
            if not inputs:
                continue
            ct = ctrl.get("control_type", "")
            if ct in ("metadata", "led", "display", "analog_gauge"):
                continue
            items.append({
                "panel": panel,
                "identifier": ctrl.get("identifier", ctrl_label),
                "description": ctrl.get("description", ""),
                "control_type": ct,
                "inputs": inputs,
            })
    return items

# ═══════════════════════════════════════════════════════════════════════════════
# FULL-SCREEN FILTERABLE AIRCRAFT PICKER
# ═══════════════════════════════════════════════════════════════════════════════

def pick_aircraft():
    """Full-screen filterable picker for aircraft selection.

    Shows: Aircraft Name | Panels | Input Controls
    Returns filepath or None on Esc.
    """
    aircraft = list_aircraft()
    if not aircraft:
        _w(f"\n  {RED}No aircraft JSON files found in:{RESET}\n")
        _w(f"  {DIM}{AIRCRAFT_DIR}{RESET}\n")
        input(f"\n  Press Enter to exit...")
        return None

    # Pre-load summary stats
    ac_stats = []
    for display_name, filepath in aircraft:
        try:
            data = load_aircraft_json(filepath)
            panels = sum(1 for v in data.values() if isinstance(v, dict))
            inputs = 0
            for panel_data in data.values():
                if not isinstance(panel_data, dict):
                    continue
                for ctrl in panel_data.values():
                    if not isinstance(ctrl, dict):
                        continue
                    if ctrl.get("inputs") and ctrl.get("control_type", "") not in ("metadata", "led", "display", "analog_gauge"):
                        inputs += 1
            ac_stats.append({"name": display_name, "path": filepath,
                             "panels": panels, "inputs": inputs})
        except Exception:
            ac_stats.append({"name": display_name, "path": filepath,
                             "panels": 0, "inputs": 0})

    search_strs = [s["name"].lower() for s in ac_stats]

    filter_text = ""
    filtered_indices = list(range(len(ac_stats)))
    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    list_height = rows - 6
    if list_height < 5:
        list_height = 5

    stats_w = 28
    prefix_len = 3
    suffix_len = 2
    name_w = max(cols - prefix_len - stats_w - suffix_len, 20)
    _header_w = cols - 4

    _flash_timer  = [None]
    _flash_active = [False]

    def _apply_filter():
        nonlocal filtered_indices, idx
        if filter_text:
            ft = filter_text.lower()
            filtered_indices = [i for i in range(len(ac_stats)) if ft in search_strs[i]]
        else:
            filtered_indices = list(range(len(ac_stats)))
        if not filtered_indices:
            idx = 0
        elif idx >= len(filtered_indices):
            idx = len(filtered_indices) - 1

    def _needs_scroll():
        return len(filtered_indices) > list_height

    def _scroll_bar_positions():
        ftotal = len(filtered_indices)
        if ftotal <= list_height:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / ftotal))
        max_scroll = ftotal - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    def _clamp_scroll():
        nonlocal scroll
        if not filtered_indices:
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

    def _render_row(ri, is_highlighted, scroll_char):
        item = ac_stats[filtered_indices[ri]]
        name_t = item["name"][:name_w]
        name_pad = name_w - len(name_t)
        stats_t = f"{item['panels']:>3} panels   {item['inputs']:>4} controls"

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG}{YELLOW} > {name_t}"
                    f"{' ' * name_pad}  {stats_t}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} {GREEN}{name_t}{RESET}{_SEL_BG}"
                    f"{' ' * name_pad}  {DIM}{stats_t}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        return (f"   {name_t}"
                f"{' ' * name_pad}  {DIM}{stats_t}{RESET}"
                f" {DIM}{scroll_char}{RESET}")

    def _draw():
        _cls()
        _w(HIDE_CUR)

        ftotal = len(filtered_indices)
        total = len(ac_stats)

        title = f"Select Aircraft"
        if filter_text:
            right_text = f"filter: {filter_text}"
            right_ansi = f"{DIM}filter: {RESET}{YELLOW}{filter_text}"
        else:
            right_text = f"{ftotal} / {total} aircraft"
            right_ansi = f"{GREEN}{ftotal} / {total} aircraft{RESET}"
        spacing = _header_w - len(title) - len(right_text) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{RESET}{' ' * spacing}{right_ansi}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")

        thumb_start, thumb_end = _scroll_bar_positions()

        if ftotal == 0:
            _w(f"\n  {DIM}No matches.{RESET}\n")
            for _ in range(list_height - 2):
                _w("\n")
        else:
            for row in range(list_height):
                ri = scroll + row
                if _needs_scroll():
                    scroll_char = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
                else:
                    scroll_char = " "
                if ri < ftotal:
                    _w(_render_row(ri, ri == idx, scroll_char) + "\n")
                else:
                    _w(ERASE_LN + "\n")

        filter_hint = f"  {DIM}\u2190=clear filter{RESET}" if filter_text else ""
        _w(f"\n  {DIM}type to filter  Enter=select  Esc=exit{RESET}{filter_hint}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "K":
                    if filter_text:
                        filter_text = ""
                        _apply_filter()
                        _clamp_scroll()
                        _draw()
                    continue
                if not filtered_indices:
                    continue
                old = idx
                if ch2 == "H":
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                        continue
                elif ch2 == "P":
                    if idx < len(filtered_indices) - 1:
                        idx += 1
                    else:
                        _flash_row()
                        continue
                elif ch2 == "I":
                    idx = max(0, idx - list_height)
                elif ch2 == "Q":
                    idx = min(max(0, len(filtered_indices) - 1), idx + list_height)
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch == "\r":
                if filtered_indices:
                    if _flash_timer[0]:
                        _flash_timer[0].cancel()
                    _w(SHOW_CUR)
                    return ac_stats[filtered_indices[idx]]["path"]
                continue

            elif ch == "\x1b":
                if _flash_timer[0]:
                    _flash_timer[0].cancel()
                _w(SHOW_CUR)
                return None

            elif ch == "\x08":
                if filter_text:
                    filter_text = filter_text[:-1]
                    _apply_filter()
                    _clamp_scroll()
                    _draw()

            elif ch.isprintable():
                filter_text += ch
                _apply_filter()
                _clamp_scroll()
                _draw()

    except KeyboardInterrupt:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)
        raise

# ═══════════════════════════════════════════════════════════════════════════════
# FULL-SCREEN FILTERABLE CONTROL BROWSER
# ═══════════════════════════════════════════════════════════════════════════════

def pick_control(controls, aircraft_name, chain):
    """Full-screen filterable picker for controls with inputs.

    Shows the current chain at the top, then the filterable control list.
    Returns the control dict or None on Esc.
    """
    if not controls:
        _w(f"\n  {DIM}No controls with inputs found.{RESET}\n")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return None

    search_strs = [
        f"{c['identifier']} {c['panel']} {c['description']} {c['control_type']}".lower()
        for c in controls
    ]

    filter_text = ""
    filtered_indices = list(range(len(controls)))
    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    # Reserve rows for: 3 header + chain lines + 1 blank + list + 2 footer
    chain_rows = min(len(chain), 5)  # show last 5 chain entries max
    overhead = 3 + (chain_rows + 1 if chain else 0) + 2
    list_height = rows - overhead
    if list_height < 5:
        list_height = 5

    type_w = 14
    prefix_len = 3
    suffix_len = 3
    available = cols - prefix_len - suffix_len - type_w - 4
    ident_w = max(available * 3 // 5, 16)
    panel_w = max(available - ident_w, 14)

    _flash_timer  = [None]
    _flash_active = [False]
    _header_w = cols - 4

    def _apply_filter():
        nonlocal filtered_indices, idx
        if filter_text:
            ft = filter_text.lower()
            filtered_indices = [i for i in range(len(controls)) if ft in search_strs[i]]
        else:
            filtered_indices = list(range(len(controls)))
        if not filtered_indices:
            idx = 0
        elif idx >= len(filtered_indices):
            idx = len(filtered_indices) - 1

    def _needs_scroll():
        return len(filtered_indices) > list_height

    def _scroll_bar_positions():
        ftotal = len(filtered_indices)
        if ftotal <= list_height:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / ftotal))
        max_scroll = ftotal - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    def _clamp_scroll():
        nonlocal scroll
        if not filtered_indices:
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

    def _render_row(ri, is_highlighted, scroll_char):
        item = controls[filtered_indices[ri]]
        ident_t = item["identifier"][:ident_w]
        panel_t = item["panel"][:panel_w]
        type_t  = item["control_type"][:type_w]
        ident_pad = ident_w - len(ident_t)

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG}{YELLOW} > {ident_t}"
                    f"{' ' * ident_pad}  {panel_t:<{panel_w}}  {type_t:<{type_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} {GREEN}{ident_t}{RESET}{_SEL_BG}"
                    f"{' ' * ident_pad}  {DIM}{panel_t:<{panel_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{type_t:<{type_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        return (f"   {ident_t}"
                f"{' ' * ident_pad}  {DIM}{panel_t:<{panel_w}}{RESET}  "
                f"{DIM}{type_t:<{type_w}}{RESET}"
                f" {DIM}{scroll_char}{RESET}")

    def _draw():
        _cls()
        _w(HIDE_CUR)

        ftotal = len(filtered_indices)
        total = len(controls)

        # Header
        title = f"Bulk Command Builder  {DIM}({aircraft_name}){RESET}"
        title_plain = f"Bulk Command Builder  ({aircraft_name})"
        if filter_text:
            right_text = f"filter: {filter_text}"
            right_ansi = f"{DIM}filter: {RESET}{YELLOW}{filter_text}"
        else:
            right_text = f"{ftotal} / {total} controls"
            right_ansi = f"{GREEN}{ftotal} / {total} controls{RESET}"
        spacing = _header_w - len(title_plain) - len(right_text) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{RESET}{' ' * spacing}{right_ansi}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")

        # Chain display
        if chain:
            visible = chain[-chain_rows:] if len(chain) > chain_rows else chain
            if len(chain) > chain_rows:
                _w(f"  {DIM}  ... ({len(chain) - chain_rows} more){RESET}\n")
            for i_offset, cmd in enumerate(visible):
                real_idx = len(chain) - len(visible) + i_offset + 1
                _w(f"  {GREEN}  {real_idx}. {cmd}{RESET}\n")
            _w("\n")

        thumb_start, thumb_end = _scroll_bar_positions()

        if ftotal == 0:
            _w(f"\n  {DIM}No matches.{RESET}\n")
            for _ in range(list_height - 2):
                _w("\n")
        else:
            for row in range(list_height):
                ri = scroll + row
                if _needs_scroll():
                    scroll_char = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
                else:
                    scroll_char = " "
                if ri < ftotal:
                    _w(_render_row(ri, ri == idx, scroll_char) + "\n")
                else:
                    _w(ERASE_LN + "\n")

        # Footer
        chain_hint = f"  {DIM}chain: {GREEN}{len(chain)}{RESET}" if chain else ""
        filter_hint = f"  {DIM}\u2190=clear filter{RESET}" if filter_text else ""
        _w(f"\n  {DIM}type to filter  Enter=select  Esc=done{RESET}{filter_hint}{chain_hint}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "K":
                    if filter_text:
                        filter_text = ""
                        _apply_filter()
                        _clamp_scroll()
                        _draw()
                    continue
                if not filtered_indices:
                    continue
                old = idx
                if ch2 == "H":
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                        continue
                elif ch2 == "P":
                    if idx < len(filtered_indices) - 1:
                        idx += 1
                    else:
                        _flash_row()
                        continue
                elif ch2 == "I":
                    idx = max(0, idx - list_height)
                elif ch2 == "Q":
                    idx = min(max(0, len(filtered_indices) - 1), idx + list_height)
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch == "\r":
                if filtered_indices:
                    if _flash_timer[0]:
                        _flash_timer[0].cancel()
                    _w(SHOW_CUR)
                    return controls[filtered_indices[idx]]
                continue

            elif ch == "\x1b":
                if _flash_timer[0]:
                    _flash_timer[0].cancel()
                _w(SHOW_CUR)
                return None

            elif ch == "\x08":
                if filter_text:
                    filter_text = filter_text[:-1]
                    _apply_filter()
                    _clamp_scroll()
                    _draw()

            elif ch.isprintable():
                filter_text += ch
                _apply_filter()
                _clamp_scroll()
                _draw()

    except KeyboardInterrupt:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)
        raise

# ═══════════════════════════════════════════════════════════════════════════════
# COMMAND ACTION BUILDER
# ═══════════════════════════════════════════════════════════════════════════════

SET_STATE_ENUM_LIMIT = 20


def build_actions(control):
    """Build all sendable command actions from a control's inputs array.

    Returns list of (label, command_string) tuples.
    command_string is None for actions that need a text prompt.
    """
    identifier = control["identifier"]
    inputs = control.get("inputs", [])
    actions = []

    for inp in inputs:
        iface = inp.get("interface", "")

        if iface == "action":
            arg = inp.get("argument", "TOGGLE")
            desc = inp.get("description", f"action: {arg}")
            actions.append((
                f"{YELLOW}{arg:<12}{RESET} {DIM}{desc}{RESET}",
                f"{identifier} {arg}"
            ))

        elif iface == "fixed_step":
            desc = inp.get("description", "switch to previous or next state")
            actions.append((
                f"{YELLOW}{'INC':<12}{RESET} {DIM}{desc} (increment){RESET}",
                f"{identifier} INC"
            ))
            actions.append((
                f"{YELLOW}{'DEC':<12}{RESET} {DIM}{desc} (decrement){RESET}",
                f"{identifier} DEC"
            ))

        elif iface == "set_state":
            max_val = inp.get("max_value", 1)
            desc = inp.get("description", "set position")

            if max_val <= SET_STATE_ENUM_LIMIT:
                for pos in range(max_val + 1):
                    actions.append((
                        f"{YELLOW}{str(pos):<12}{RESET} {DIM}{desc} ({pos}/{max_val}){RESET}",
                        f"{identifier} {pos}"
                    ))
            else:
                actions.append((
                    f"{YELLOW}{'0':<12}{RESET} {DIM}{desc} (MIN){RESET}",
                    f"{identifier} 0"
                ))
                quarter = max_val // 4
                half = max_val // 2
                three_q = (max_val * 3) // 4
                actions.append((
                    f"{YELLOW}{str(quarter):<12}{RESET} {DIM}{desc} (25%){RESET}",
                    f"{identifier} {quarter}"
                ))
                actions.append((
                    f"{YELLOW}{str(half):<12}{RESET} {DIM}{desc} (50%){RESET}",
                    f"{identifier} {half}"
                ))
                actions.append((
                    f"{YELLOW}{str(three_q):<12}{RESET} {DIM}{desc} (75%){RESET}",
                    f"{identifier} {three_q}"
                ))
                actions.append((
                    f"{YELLOW}{str(max_val):<12}{RESET} {DIM}{desc} (MAX){RESET}",
                    f"{identifier} {max_val}"
                ))
                actions.append((
                    f"{CYAN}{'custom...':<12}{RESET} {DIM}enter value 0-{max_val}{RESET}",
                    None
                ))

        elif iface == "variable_step":
            max_val = inp.get("max_value", 65535)
            step = inp.get("suggested_step", 3200)
            desc = inp.get("description", "turn the dial")
            actions.append((
                f"{YELLOW}{'+' + str(step):<12}{RESET} {DIM}{desc} (+step){RESET}",
                f"{identifier} +{step}"
            ))
            actions.append((
                f"{YELLOW}{'-' + str(step):<12}{RESET} {DIM}{desc} (-step){RESET}",
                f"{identifier} -{step}"
            ))
            actions.append((
                f"{CYAN}{'custom...':<12}{RESET} {DIM}enter +/- step value{RESET}",
                None
            ))

        elif iface == "set_string":
            desc = inp.get("description", "set string value")
            actions.append((
                f"{CYAN}{'string...':<12}{RESET} {DIM}{desc}{RESET}",
                None
            ))

    return actions


def _prompt_custom_value(identifier):
    """Prompt for a custom value. Returns command string or None."""
    _w(SHOW_CUR)
    _w(f"\n  {BOLD}Enter value for {GREEN}{identifier}{RESET}: ")
    val = ""
    while True:
        ch = msvcrt.getwch()
        if ch == "\r":
            break
        elif ch == "\x1b":
            _w(f"\n  {DIM}Cancelled.{RESET}\n")
            return None
        elif ch == "\x08":
            if val:
                val = val[:-1]
                _w("\b \b")
        elif ch.isprintable():
            val += ch
            _w(ch)

    val = val.strip()
    if not val:
        return None

    _w("\n")
    return f"{identifier} {val}"


def pick_action(control):
    """Show all available commands for a control. Returns command string or None."""
    identifier = control["identifier"]
    actions = build_actions(control)

    if not actions:
        _w(f"\n  {DIM}No sendable inputs for this control.{RESET}\n")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return None

    # Header
    _cls()
    cols, _ = _get_terminal_size()
    header_w = cols - 4
    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
    _w(f"  {CYAN}{BOLD}  {identifier}{RESET}\n")
    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
    _w(f"\n")
    _w(f"     {DIM}Type:{RESET}         {control['control_type']}\n")
    _w(f"     {DIM}Panel:{RESET}        {control['panel']}\n")
    _w(f"     {DIM}Description:{RESET}  {control['description']}\n")

    ifaces = [inp.get("interface", "?") for inp in control.get("inputs", [])]
    _w(f"     {DIM}Interfaces:{RESET}   {', '.join(ifaces)}\n")

    # Replace None sentinels with unique string sentinels
    custom_counter = 0
    options = []
    for label, cmd in actions:
        if cmd is None:
            custom_counter += 1
            options.append((label, f"__CUSTOM_{custom_counter}__"))
        else:
            options.append((label, cmd))
    options.append((f"{DIM}Back{RESET}", "__BACK__"))

    result = arrow_pick("Select command to add to chain:", options)

    if result is None or result == "__BACK__":
        return None

    if result.startswith("__CUSTOM_"):
        return _prompt_custom_value(identifier)

    return result

# ═══════════════════════════════════════════════════════════════════════════════
# UDP SEND
# ═══════════════════════════════════════════════════════════════════════════════

def send_udp(cmd: str, bind_ip: str, dcs_ip: str) -> None:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            sock.bind((bind_ip, 0))
            sock.sendto((cmd + "\n").encode("utf-8"), (dcs_ip, DCS_PORT))
        print(f"  {GREEN}[SENT]{RESET} {cmd}")
    except Exception as e:
        print(f"  {RED}[ERROR]{RESET} {cmd} -- {e}")


def send_single_packet(commands: list, bind_ip: str, dcs_ip: str) -> None:
    """Send all commands in a single UDP packet, newline-separated."""
    try:
        payload = "\n".join(commands) + "\n"
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
            sock.bind((bind_ip, 0))
            sock.sendto(payload.encode("utf-8"), (dcs_ip, DCS_PORT))
        for cmd in commands:
            print(f"  {GREEN}[SENT]{RESET} {cmd}")
        print(f"  {DIM}(all in single UDP packet){RESET}")
    except Exception as e:
        print(f"  {RED}[ERROR]{RESET} {e}")


def send_bulk(commands: list, bind_ip: str, dcs_ip: str, debounce_ms: int) -> None:
    delay = max(0, debounce_ms) / 1000.0

    for i, cmd in enumerate(commands):
        if i > 0 and delay > 0:
            time.sleep(delay)
        send_udp(cmd, bind_ip, dcs_ip)

# ═══════════════════════════════════════════════════════════════════════════════
# CHAIN REVIEW SCREEN
# ═══════════════════════════════════════════════════════════════════════════════

def review_chain(chain, debounce_ms, aircraft_name):
    """Show the full chain and let user choose next action.

    Returns:
        "add"    — go back to control browser to add more
        "send"   — send the chain
        "undo"   — remove last command
        "clear"  — clear entire chain
        "cancel" — discard and exit
    """
    _cls()
    cols, _ = _get_terminal_size()
    header_w = cols - 4

    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
    _w(f"  {CYAN}{BOLD}  Command Chain{RESET}  {DIM}({aircraft_name}){RESET}\n")
    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

    if not chain:
        _w(f"\n  {DIM}Chain is empty.{RESET}\n")
    else:
        _w(f"\n")
        for i, cmd in enumerate(chain, 1):
            _w(f"  {GREEN}{i:>3}.{RESET} {cmd}\n")
            if i < len(chain):
                _w(f"  {DIM}     ~ {debounce_ms}ms ~{RESET}\n")
        _w(f"\n  {DIM}Total: {len(chain)} commands, ~{(len(chain) - 1) * debounce_ms}ms total delay{RESET}\n")

    # Build options
    options = []
    options.append((f"{GREEN}Add more commands{RESET}", "add"))
    if chain:
        options.append((f"{CYAN}SEND chain{RESET}", "send"))
        options.append((f"{YELLOW}Undo last{RESET}", "undo"))
        options.append((f"{YELLOW}Clear all{RESET}", "clear"))
    options.append((f"{DIM}Cancel{RESET}", "cancel"))

    result = arrow_pick("What next?", options)
    return result if result else "cancel"

# ═══════════════════════════════════════════════════════════════════════════════
# DEBOUNCE PICKER
# ═══════════════════════════════════════════════════════════════════════════════

def pick_debounce():
    """Let user pick debounce delay. Returns ms value."""
    options = [
        (f"{CYAN}{'0 ms':<12}{RESET} {DIM}no delay (fire all at once){RESET}", 0),
        (f"{CYAN}{'50 ms':<12}{RESET} {DIM}fast (recommended){RESET}", 50),
        (f"{CYAN}{'100 ms':<12}{RESET} {DIM}moderate{RESET}", 100),
        (f"{CYAN}{'250 ms':<12}{RESET} {DIM}slow (visible stepping){RESET}", 250),
        (f"{CYAN}{'500 ms':<12}{RESET} {DIM}very slow{RESET}", 500),
        (f"{CYAN}{'1000 ms':<12}{RESET} {DIM}1 second between commands{RESET}", 1000),
    ]
    result = arrow_pick("Debounce delay between commands:", options)
    return result if result is not None else DEFAULT_DEBOUNCE_MS

# ═══════════════════════════════════════════════════════════════════════════════
# INTERACTIVE MODE
# ═══════════════════════════════════════════════════════════════════════════════

def interactive_mode(bind_ip, dcs_ip):
    """Full interactive chain builder."""

    # Resolve display name
    if bind_ip == "127.0.0.1":
        bind_label = "localhost (127.0.0.1)"
    else:
        bind_label = bind_ip

    # Pick aircraft
    ac_path = pick_aircraft()
    if ac_path is None:
        return

    aircraft_name = os.path.basename(ac_path)[:-5]

    # Load controls
    _w(f"\n  {DIM}Loading {aircraft_name}...{RESET}")
    aircraft_data = load_aircraft_json(ac_path)
    controls = collect_input_controls(aircraft_data)
    _w(f"  {GREEN}{len(controls)} controls with inputs{RESET}\n")

    if not controls:
        _w(f"\n  {RED}No controls with inputs found.{RESET}\n")
        input(f"\n  Press Enter to exit...")
        return

    # Pick debounce
    debounce_ms = pick_debounce()

    # Chain building loop
    chain = []

    while True:
        # Control browser — pick a control to add
        control = pick_control(controls, aircraft_name, chain)

        if control is None:
            # Esc from browser — go to review
            action = review_chain(chain, debounce_ms, aircraft_name)

            if action == "add":
                continue
            elif action == "send":
                break
            elif action == "undo":
                if chain:
                    removed = chain.pop()
                    _w(f"\n  {YELLOW}Removed:{RESET} {removed}\n")
                    time.sleep(0.3)
                continue
            elif action == "clear":
                chain.clear()
                continue
            else:  # cancel
                _w(f"\n  {DIM}Cancelled.{RESET}\n")
                return

        # Pick action for this control
        cmd = pick_action(control)
        if cmd is None:
            continue  # Back pressed in action picker — return to browser

        chain.append(cmd)
        _w(f"\n  {GREEN}Added #{len(chain)}:{RESET} {cmd}\n")
        _w(f"\n  {DIM}Press any key to add more, or Esc to review chain...{RESET}")
        ch = msvcrt.getwch()
        if ch == "\x1b":
            action = review_chain(chain, debounce_ms, aircraft_name)
            if action == "add":
                continue
            elif action == "send":
                break
            elif action == "undo":
                if chain:
                    chain.pop()
                continue
            elif action == "clear":
                chain.clear()
                continue
            else:
                _w(f"\n  {DIM}Cancelled.{RESET}\n")
                return

    # Send the chain
    if not chain:
        _w(f"\n  {DIM}Nothing to send.{RESET}\n")
        return

    _cls()
    cols, _ = _get_terminal_size()
    header_w = cols - 4
    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
    _w(f"  {CYAN}{BOLD}  Sending Chain{RESET}  {DIM}({aircraft_name}){RESET}\n")
    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
    _w(f"\n  {DIM}Interface: {bind_label}  Target: {dcs_ip}:{DCS_PORT}  Debounce: {debounce_ms}ms{RESET}\n\n")

    send_bulk(chain, bind_ip, dcs_ip, debounce_ms)

    _w(f"\n  {GREEN}{BOLD}Done!{RESET} {DIM}{len(chain)} commands sent.{RESET}\n")
    _w(f"\n  {DIM}Press any key...{RESET}")
    msvcrt.getwch()

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(description="Send DCS-BIOS commands in bulk.")
    parser.add_argument("--ip", default=None, help="DCS PC IP address (default: broadcast)")
    parser.add_argument("--debounce", type=int, default=0, help="Delay in ms between commands (default: 0)")
    parser.add_argument("--single", action="store_true", help="Send all commands in a single UDP packet")
    parser.add_argument("commands", nargs="*", help="Commands to send (each as separate argument)")
    args = parser.parse_args()

    # Pick interface
    interfaces = get_all_interface_ips()
    bind_ip = pick_interface(interfaces)

    # DCS target defaults to broadcast, or --ip override
    dcs_ip = args.ip if args.ip else "255.255.255.255"

    # CLI mode: commands provided as arguments
    if args.commands:
        # Resolve display name
        if bind_ip == "127.0.0.1":
            bind_label = "localhost (127.0.0.1)"
        else:
            bind_label = bind_ip
            for name, ip in interfaces:
                if ip == bind_ip:
                    bind_label = f"{ip} ({name})"
                    break

        # Header
        print()
        print("+----------------------------------------------------------------+")
        _box_line(f"  {BOLD}CockpitOS DCS Bulk Command Sender{RESET}")
        print("+----------------------------------------------------------------+")
        _box_line(f"  Interface: {CYAN}{bind_label}{RESET}")
        _box_line(f"  Target:    {CYAN}{dcs_ip}:{DCS_PORT}{RESET}")
        _box_line(f"  Mode:      {CYAN}{'single packet' if args.single else f'bulk (debounce {args.debounce}ms)'}{RESET}")
        print("+----------------------------------------------------------------+")

        print()
        if args.single:
            send_single_packet(args.commands, bind_ip, dcs_ip)
        else:
            send_bulk(args.commands, bind_ip, dcs_ip, args.debounce)

        print()
        input("  Press <ENTER> to exit...")
        return

    # Interactive mode: no commands — launch chain builder
    interactive_mode(bind_ip, dcs_ip)


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        print(f"\n  {DIM}Interrupted.{RESET}")
        sys.exit(0)
