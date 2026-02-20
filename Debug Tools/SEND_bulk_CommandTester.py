#!/usr/bin/env python3
"""
CockpitOS DCS Bulk Command Sender
==================================

Send multiple DCS-BIOS commands in sequence with optional delay between each.
At startup, presents an arrow-key interface selector so you can pick
which network adapter to send from.

Usage:
    python SEND_bulk_CommandTester.py "CMD1 ARG" "CMD2 ARG"
    python SEND_bulk_CommandTester.py --ip 192.168.1.100 "BATTERY_SW 2"
    python SEND_bulk_CommandTester.py --debounce 100 "WING_FOLD_PULL 0" "WING_FOLD_ROTATE 1"

If no commands provided, uses built-in defaults for testing.

Author: CockpitOS Project
"""

import os
import sys
import argparse
import socket
import time
import msvcrt
import ctypes

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════════
# DEPENDENCY CHECK
# ═══════════════════════════════════════════════════════════════════════════════

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

DEFAULT_COMMANDS = [
    "WING_FOLD_PULL 0",
    "WING_FOLD_ROTATE 1",
]

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
    """Arrow-key picker. options = list of (label, value). Returns value."""
    if not options:
        return None

    total = len(options)
    idx = 0

    print()
    _w(f"  {BOLD}{prompt}{RESET}\n")
    _w(f"  {DIM}(Up/Down to move, Enter to select){RESET}\n")

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

            elif ch == "\x1b":      # Esc — return first option as default
                _w(SHOW_CUR)
                return options[0][1]

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        sys.exit(0)


def pick_interface(interfaces):
    """Arrow-key picker for network interfaces. Returns bind IP string."""
    options = [("Localhost (127.0.0.1)", "127.0.0.1")]
    for name, ip in interfaces:
        options.append((f"{ip:<17} {DIM}({name}){RESET}", ip))

    return arrow_pick("Select network interface:", options)

# ═══════════════════════════════════════════════════════════════════════════════
# UDP SEND
# ═══════════════════════════════════════════════════════════════════════════════

def send_udp(cmd: str, bind_ip: str, dcs_ip: str) -> None:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.bind((bind_ip, 0))
            sock.sendto((cmd + "\n").encode("utf-8"), (dcs_ip, DCS_PORT))
        print(f"  {GREEN}[SENT]{RESET} {cmd}")
    except Exception as e:
        print(f"  {RED}[ERROR]{RESET} {cmd} -- {e}")


def send_bulk(commands: list, bind_ip: str, dcs_ip: str, debounce_ms: int) -> None:
    delay = max(0, debounce_ms) / 1000.0

    for i, cmd in enumerate(commands):
        if i > 0 and delay > 0:
            time.sleep(delay)
        send_udp(cmd, bind_ip, dcs_ip)

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(description="Send DCS-BIOS commands in bulk.")
    parser.add_argument("--ip", default=None, help="DCS PC IP address (default: same as selected interface)")
    parser.add_argument("--debounce", type=int, default=0, help="Delay in ms between commands (default: 0)")
    parser.add_argument("commands", nargs="*", help="Commands to send (each as separate argument)")
    args = parser.parse_args()

    # Pick interface
    interfaces = get_all_interface_ips()
    bind_ip = pick_interface(interfaces)

    # DCS target defaults to the selected interface (same machine), or --ip override
    dcs_ip = args.ip if args.ip else bind_ip

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
    _box_line(f"  Debounce:  {CYAN}{args.debounce}ms{RESET}")
    print("+----------------------------------------------------------------+")

    # Use defaults if no commands provided
    if not args.commands:
        print("\n  No commands provided, using defaults:")
        for c in DEFAULT_COMMANDS:
            print(f"    {c}")
        cmds = DEFAULT_COMMANDS
    else:
        cmds = args.commands

    print()
    send_bulk(cmds, bind_ip, dcs_ip, args.debounce)

    print()
    input("  Press <ENTER> to exit...")


if __name__ == "__main__":
    main()
