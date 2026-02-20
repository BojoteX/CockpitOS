#!/usr/bin/env python3
"""
CockpitOS DCS Command Tester
=============================

Interactive arrow-key menu to send DCS-BIOS commands for testing.
At startup, select a network interface, then navigate command categories
and values with arrow keys.

Usage:
    python SEND_CommandTester.py                     # Interactive
    python SEND_CommandTester.py --ip 192.168.1.100  # Specify DCS PC IP

Author: CockpitOS Project
"""

import time
import socket
import argparse
import os
import sys
import msvcrt
import ctypes

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

def _w(s):
    sys.stdout.write(s)
    sys.stdout.flush()

import re as _re
_ANSI_RE = _re.compile(r'\033\[[0-9;]*[A-Za-z]')

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
                if ip.ip.startswith('127.') or ip.ip.startswith('169.254.'):
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
    """Arrow-key picker for network interfaces. Returns bind IP string."""
    options = [("Localhost (127.0.0.1)", "127.0.0.1")]
    for name, ip in interfaces:
        options.append((f"{ip:<17} {DIM}({name}){RESET}", ip))

    result = arrow_pick("Select network interface:", options)
    return result if result else "127.0.0.1"

# ═══════════════════════════════════════════════════════════════════════════════
# COMMAND DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════════

COMMAND_GROUPS = [
    ("BATTERY_SW", [
        ("OFF",   "BATTERY_SW 1"),
        ("ON",    "BATTERY_SW 2"),
        ("ORIDE", "BATTERY_SW 0"),
    ]),
    ("LTD/R", [
        ("SAFE",   "LTD_R_SW 0"),
        ("ARM",    "LTD_R_SW 1"),
        ("INC",    "LTD_R_SW INC"),
        ("DEC",    "LTD_R_SW DEC"),
        ("TOGGLE", "LTD_R_SW TOGGLE"),
    ]),
    ("GAIN_SWITCH", [
        ("NORM",  "GAIN_NORM"),
        ("ORIDE", "GAIN_ORIDE"),
    ]),
    ("HOOK_LEVER", [
        ("DOWN", "HOOK_LEVER 0"),
        ("UP",   "HOOK_LEVER 1"),
    ]),
]

# ═══════════════════════════════════════════════════════════════════════════════
# UDP SEND
# ═══════════════════════════════════════════════════════════════════════════════

BIND_IP = "127.0.0.1"
DCS_IP = "127.0.0.1"

def send_udp(cmd: str) -> None:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.bind((BIND_IP, 0))
            sock.sendto((cmd + "\n").encode(), (DCS_IP, DCS_PORT))
            print(f"  {GREEN}[SENT]{RESET} {cmd}")
    except Exception as e:
        print(f"  {RED}[ERROR]{RESET} {cmd} -- {e}")


def send_gain(position):
    """Handle GAIN_SWITCH which requires cover open/close sequence."""
    if position == "GAIN_ORIDE":
        send_udp("GAIN_SWITCH_COVER 1")
        time.sleep(0.100)
        send_udp("GAIN_SWITCH 1")
    elif position == "GAIN_NORM":
        send_udp("GAIN_SWITCH 0")
        time.sleep(0.100)
        send_udp("GAIN_SWITCH_COVER 0")

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    global DCS_IP, BIND_IP

    parser = argparse.ArgumentParser(description="DCS-BIOS Command Tester")
    parser.add_argument("--ip", default=None, help="DCS PC IP address (default: same as selected interface)")
    args = parser.parse_args()

    # Pick interface
    interfaces = get_all_interface_ips()
    BIND_IP = pick_interface(interfaces)

    # DCS target defaults to the selected interface, or --ip override
    DCS_IP = args.ip if args.ip else BIND_IP

    # Resolve display name
    if BIND_IP == "127.0.0.1":
        bind_label = "localhost (127.0.0.1)"
    else:
        bind_label = BIND_IP
        for name, ip in interfaces:
            if ip == BIND_IP:
                bind_label = f"{ip} ({name})"
                break

    # Header
    print()
    print("+----------------------------------------------------------------+")
    _box_line(f"  {BOLD}CockpitOS DCS Command Tester{RESET}")
    print("+----------------------------------------------------------------+")
    _box_line(f"  Interface: {CYAN}{bind_label}{RESET}")
    _box_line(f"  Target:    {CYAN}{DCS_IP}:{DCS_PORT}{RESET}")
    print("+----------------------------------------------------------------+")

    # Build top-level menu options
    top_options = [(f"{CYAN}{name}{RESET}", name) for name, _ in COMMAND_GROUPS]
    top_options.append((f"{DIM}Exit{RESET}", "EXIT"))

    # Main loop
    while True:
        choice = arrow_pick("Select command group:", top_options)

        if choice is None or choice == "EXIT":
            print(f"\n  {DIM}Goodbye!{RESET}")
            break

        # Find the group
        for group_name, commands in COMMAND_GROUPS:
            if group_name == choice:
                # Build sub-menu
                sub_options = [(f"{label:<10} {DIM}-> {cmd}{RESET}", cmd) for label, cmd in commands]
                sub_choice = arrow_pick(f"{BOLD}{group_name}{RESET} -- select value:", sub_options)

                if sub_choice is None:
                    continue  # Esc -> back to top menu

                # Handle GAIN special case
                if sub_choice in ("GAIN_NORM", "GAIN_ORIDE"):
                    send_gain(sub_choice)
                else:
                    send_udp(sub_choice)
                break


if __name__ == "__main__":
    main()
