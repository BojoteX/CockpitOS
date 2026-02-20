#!/usr/bin/env python3
"""
CockpitOS UDP Debug Console
============================

Receives and logs debug messages from CockpitOS devices over WiFi UDP.
ESP32 devices broadcast to 255.255.255.255:4210.

At startup, presents an arrow-key interface selector so you can pick
which network adapter to listen on.

Usage:
    python CONSOLE_UDP_debug.py                     # Interactive interface pick
    python CONSOLE_UDP_debug.py --ip 192.168.1.50   # Filter by source IP

Author: CockpitOS Project
"""

import socket
import os
import sys
import argparse
import msvcrt
import ctypes
from datetime import datetime
import shutil

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

# IP to device name mapping
ip_to_name = {}

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

PORT = 4210  # CockpitOS debug broadcast port

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
# ARROW-KEY INTERFACE PICKER
# ═══════════════════════════════════════════════════════════════════════════════

def pick_interface(interfaces):
    """Arrow-key picker for network interfaces. Returns bind IP string."""

    # Build options: All, localhost, then each discovered interface
    options = [("All interfaces", "0.0.0.0"),
               ("Localhost (127.0.0.1)", "127.0.0.1")]
    for name, ip in interfaces:
        options.append((f"{ip:<17} {DIM}({name}){RESET}", ip))

    total = len(options)
    idx = 0

    print()
    _w(f"  {BOLD}Select network interface:{RESET}\n")
    _w(f"  {DIM}(Up/Down to move, Enter to select){RESET}\n")

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

            elif ch == "\x1b":      # Esc
                _w(SHOW_CUR)
                return options[0][1]  # Default to all

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        sys.exit(0)

# ═══════════════════════════════════════════════════════════════════════════════
# LOG FILE MANAGEMENT
# ═══════════════════════════════════════════════════════════════════════════════

# Ensure logs directory exists
if not os.path.exists("logs"):
    os.makedirs("logs")

LOG_FILE = "logs/udpLogger.log"
PREV_LOG = "logs/prevLog.log"

# Rotate logs: Move last log to prevLog if it exists
if os.path.exists(LOG_FILE):
    if os.path.exists(PREV_LOG):
        os.remove(PREV_LOG)
    shutil.copy2(LOG_FILE, PREV_LOG)
    with open(LOG_FILE, 'w'):
        pass  # Clear current log

def log_write(line):
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(line + "\n")

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    # Argument parsing
    parser = argparse.ArgumentParser(description="CockpitOS UDP Debug Console")
    parser.add_argument("--ip", help="Only log messages from this IP address", default=None)
    args = parser.parse_args()

    # Discover interfaces and let user pick
    interfaces = get_all_interface_ips()
    bind_ip = pick_interface(interfaces)

    # Resolve display name for header
    if bind_ip == "0.0.0.0":
        bind_label = "all interfaces"
    else:
        bind_label = bind_ip
        for name, ip in interfaces:
            if ip == bind_ip:
                bind_label = f"{ip} ({name})"
                break

    # Setup socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(0.5)  # 500ms timeout so Ctrl+C works on Windows
    sock.bind((bind_ip, PORT))

    # Header
    print()
    print("+----------------------------------------------------------------+")
    _box_line(f"  {BOLD}CockpitOS UDP Debug Console{RESET}")
    print("+----------------------------------------------------------------+")
    _box_line(f"  Port:      {CYAN}{PORT}{RESET}")
    _box_line(f"  Interface: {CYAN}{bind_label}{RESET}")
    if args.ip:
        _box_line(f"  Filtering: {CYAN}{args.ip}{RESET}")
    _box_line(f"  Log file:  {DIM}logs/udpLogger.log{RESET}")
    _box_line(f"  {DIM}Press Ctrl+C to stop{RESET}")
    print("+----------------------------------------------------------------+")
    print()

    log_write(f"=== Session started at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')} ===")
    log_write(f"Interface: {bind_label}")
    if args.ip:
        log_write(f"Filtering to IP: {args.ip}")
    log_write("")

    try:
        while True:
            try:
                data, addr = sock.recvfrom(1024)
            except socket.timeout:
                continue  # Check for Ctrl+C, then loop back

            sender_ip, sender_port = addr

            # Skip if filtering and doesn't match
            if args.ip and sender_ip != args.ip:
                continue

            msg = data.decode('utf-8', errors='ignore').strip()

            # Handle registration packets
            if msg.startswith("@@REGISTER:"):
                device_name = msg[11:]
                ip_to_name[sender_ip] = device_name
                timestamp = datetime.now().strftime('%H:%M:%S')
                line = f"[{timestamp}] {GREEN}Registered:{RESET} {sender_ip} -> {CYAN}{device_name}{RESET}"
                print(line)
                log_write(line)
                continue

            timestamp = datetime.now().strftime('%H:%M:%S')
            display_name = ip_to_name.get(sender_ip, sender_ip)
            line = f"{DIM}[{timestamp}]{RESET} [{CYAN}{display_name}{RESET}] {msg}"
            print(line)
            log_write(line)

    except KeyboardInterrupt:
        print("\n\nStopped by user")
        log_write(f"\n=== Session ended at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')} ===")

    finally:
        sock.close()


if __name__ == "__main__":
    main()
