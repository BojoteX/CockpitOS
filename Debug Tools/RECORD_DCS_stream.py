#!/usr/bin/env python3
"""
CockpitOS DCS-BIOS Stream Recorder
===================================

Records DCS-BIOS UDP multicast stream for offline replay/testing.
At startup, presents an arrow-key interface selector so you can pick
which network adapter to join multicast on.

Usage:
    python RECORD_DCS_stream.py

Output:
    streams/dcsbios_data.json.LAST

Author: CockpitOS Project
"""

import socket
import time
import json
import os
import sys
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

MULTICAST_GROUP = "239.255.50.10"
UDP_PORT = 5010
BUFFER_SIZE = 4096
OUTPUT_DIR = "streams"
OUTPUT_FILE = os.path.join(OUTPUT_DIR, "dcsbios_data.json.LAST")

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
    """Arrow-key picker for network interfaces. Returns list of (name, ip) to use."""
    options = [("All interfaces", "ALL"),
               ("Localhost (127.0.0.1)", "127.0.0.1")]
    for name, ip in interfaces:
        options.append((f"{ip:<17} {DIM}({name}){RESET}", ip))

    chosen = arrow_pick("Select network interface:", options)

    if chosen == "ALL":
        return interfaces
    elif chosen == "127.0.0.1":
        return [("Localhost", "127.0.0.1")]
    else:
        for name, ip in interfaces:
            if ip == chosen:
                return [(name, ip)]
        return interfaces

# ═══════════════════════════════════════════════════════════════════════════════
# FRAME EXTRACTION
# ═══════════════════════════════════════════════════════════════════════════════

def extract_frames(data, timestamp):
    """Extract DCS-BIOS frames from raw UDP data."""
    SYNC = b'\x55\x55\x55\x55'
    frames = []
    parts = data.split(SYNC)
    if parts and parts[0] == b'':
        parts = parts[1:]
    for part in parts:
        if part:
            frame = SYNC + part
            frames.append({
                "timing": round(timestamp, 5),
                "data": frame.hex().upper()
            })
    return frames

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    # Ensure output directory exists
    if not os.path.exists(OUTPUT_DIR):
        os.makedirs(OUTPUT_DIR)

    # Discover and pick interface
    interfaces = get_all_interface_ips()
    selected_interfaces = pick_interface(interfaces)

    # Setup socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(0.5)  # So Ctrl+C works on Windows
    sock.bind(('0.0.0.0', UDP_PORT))

    # Join multicast group on selected interfaces
    joined = []

    if not selected_interfaces:
        # Fallback: join on default
        try:
            mreq = socket.inet_aton(MULTICAST_GROUP) + socket.inet_aton('0.0.0.0')
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            joined.append(("default route", "0.0.0.0"))
        except Exception as e:
            print(f"  FAILED to join multicast: {e}")
    else:
        for iface_name, ip in selected_interfaces:
            try:
                mreq = socket.inet_aton(MULTICAST_GROUP) + socket.inet_aton(ip)
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
                joined.append((iface_name, ip))
            except Exception:
                pass

    if not joined:
        print("\n  ERROR: Could not join multicast on any interface!")
        sys.exit(1)

    # Build interface label for header
    iface_label = ", ".join(f"{ip} ({name})" for name, ip in joined)

    # Header
    print()
    print("+----------------------------------------------------------------+")
    _box_line(f"  {BOLD}CockpitOS DCS-BIOS Stream Recorder{RESET}")
    print("+----------------------------------------------------------------+")
    _box_line(f"  Multicast: {CYAN}{MULTICAST_GROUP}:{UDP_PORT}{RESET}")
    _box_line(f"  Interface: {CYAN}{iface_label}{RESET}")
    _box_line(f"  Output:    {DIM}{OUTPUT_FILE}{RESET}")
    _box_line(f"  {DIM}Press Ctrl+C to stop recording{RESET}")
    print("+----------------------------------------------------------------+")
    print()

    # Recording state
    log_data = []
    buffer = bytearray()
    last_time = time.time()
    first_packet = True

    try:
        print(f"  {DIM}Waiting for DCS-BIOS stream...{RESET}")

        while True:
            try:
                data, addr = sock.recvfrom(BUFFER_SIZE)
            except socket.timeout:
                continue

            now = time.time()
            elapsed = 2.0 if first_packet else now - last_time
            last_time = now

            if first_packet:
                print(f"  {GREEN}Receiving from {addr[0]} - recording started!{RESET}")
                first_packet = False

            buffer.extend(data)
            new_frames = extract_frames(buffer, elapsed)
            buffer.clear()
            log_data.extend(new_frames)

            # Auto-save every 10 frames
            if len(log_data) % 10 == 0:
                with open(OUTPUT_FILE, "w") as f:
                    json.dump(log_data, f, indent=2)
                print(f"\r  {CYAN}Recorded {len(log_data)} frames...{RESET}", end="", flush=True)

    except KeyboardInterrupt:
        print("\n\nStopped by user")

    finally:
        with open(OUTPUT_FILE, "w") as f:
            json.dump(log_data, f, indent=2)
        print(f"  {GREEN}Saved {len(log_data)} frames to {OUTPUT_FILE}{RESET}")
        sock.close()


if __name__ == "__main__":
    main()
