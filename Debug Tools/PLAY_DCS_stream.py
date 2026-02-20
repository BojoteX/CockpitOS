#!/usr/bin/env python3
"""
CockpitOS DCS-BIOS Stream Replay Tool
=====================================

Replays captured DCS-BIOS data streams for offline testing.
Sends multicast on a user-selected network interface (or all).

By default, replays at a fixed 30 FPS — matching DCS-BIOS's internal tick
rate — to produce a stable, realistic stream for HID Manager testing.
The recorded per-frame timing is inherently noisy (OS-level timestamp
jitter, Windows timer granularity) and does not represent the true DCS
export rate, so we don't use it unless explicitly requested via --raw-timing.

Usage:
    python PLAY_DCS_stream.py                    # 30 FPS (default)
    python PLAY_DCS_stream.py --stream LAST      # Play last captured
    python PLAY_DCS_stream.py --fps 60           # Fixed 60 FPS
    python PLAY_DCS_stream.py --raw-timing       # Use recorded timing
    python PLAY_DCS_stream.py --raw-timing --speed 2.0  # Recorded timing 2x

Author: CockpitOS Project
"""

import socket
import json
import time
import binascii
import os
import sys
import msvcrt
import ctypes
import argparse
from datetime import datetime

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

MULTICAST_IP = "239.255.50.10"
UDP_PORT = 5010
STREAMS_DIR = "streams"
FILE_PREFIX = "dcsbios_data.json."

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
# ARROW-KEY PICKER (reusable)
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

# ═══════════════════════════════════════════════════════════════════════════════
# MULTI-INTERFACE NETWORK SUPPORT
# ═══════════════════════════════════════════════════════════════════════════════

def get_all_interface_ips():
    """
    Discover all usable IPv4 interface addresses on this machine.
    Returns list of (interface_name, ip_address) tuples.
    """
    interfaces = []

    for adapter in ifaddr.get_adapters():
        for ip in adapter.ips:
            if isinstance(ip.ip, str):
                if ip.ip.startswith('127.') or ip.ip.startswith('169.254.'):
                    continue
                short_name = shorten_adapter_name(adapter.nice_name)
                interfaces.append((short_name, ip.ip))

    return interfaces


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


def pick_interface(interfaces):
    """Arrow-key picker for network interfaces. Returns list of (name, ip) to use."""
    options = [("All interfaces", "ALL"),
               ("Localhost (127.0.0.1)", "127.0.0.1")]
    for name, ip in interfaces:
        options.append((f"{ip:<17} {DIM}({name}){RESET}", ip))

    chosen = arrow_pick("Select network interface:", options)

    if chosen == "ALL":
        return interfaces  # all of them
    elif chosen == "127.0.0.1":
        return [("Localhost", "127.0.0.1")]
    else:
        for name, ip in interfaces:
            if ip == chosen:
                return [(name, ip)]
        return interfaces  # fallback


def create_multicast_socket(interface_ip, ttl=1):
    """
    Create a UDP socket configured to send multicast on a specific interface.
    Uses IP_MULTICAST_IF to direct outgoing multicast to the specified NIC.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(interface_ip))
    return sock


# ═══════════════════════════════════════════════════════════════════════════════
# STREAM DISCOVERY & SELECTION
# ═══════════════════════════════════════════════════════════════════════════════

def get_relative_age(file_path):
    """Calculate human-readable age string for a file."""
    mtime = os.path.getmtime(file_path)
    age_seconds = time.time() - mtime

    if age_seconds < 60:
        return f"{int(age_seconds)} seconds old"
    elif age_seconds < 3600:
        minutes = int(age_seconds / 60)
        return f"{minutes} minute{'s' if minutes != 1 else ''} old"
    elif age_seconds < 86400:
        hours = int(age_seconds / 3600)
        return f"{hours} hour{'s' if hours != 1 else ''} old"
    elif age_seconds < 2592000:  # 30 days
        days = int(age_seconds / 86400)
        return f"{days} day{'s' if days != 1 else ''} old"
    else:
        months = int(age_seconds / 2592000)
        return f"{months} month{'s' if months != 1 else ''} old"


def discover_streams(streams_path):
    """Returns list of tuples: (identifier, full_path, display_name, age_string)"""
    streams = []

    if not os.path.isdir(streams_path):
        return streams

    for filename in os.listdir(streams_path):
        if filename.startswith(FILE_PREFIX):
            identifier = filename[len(FILE_PREFIX):]
            full_path = os.path.join(streams_path, filename)

            if identifier == "LAST":
                display_name = "LAST (Last captured stream)"
            else:
                display_name = identifier

            age_string = get_relative_age(full_path)
            streams.append((identifier, full_path, display_name, age_string))

    # Sort: LAST first (if present), then alphabetically
    streams.sort(key=lambda x: (x[0] != "LAST", x[0]))
    return streams


def select_stream(streams):
    """Arrow-key picker for stream selection."""
    options = []
    for identifier, path, display_name, age in streams:
        label = f"{display_name:<32} {DIM}({age}){RESET}"
        options.append((label, identifier))

    chosen = arrow_pick("Select stream:", options)

    for s in streams:
        if s[0] == chosen:
            return s

    return streams[0]  # fallback


# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    # === ARGUMENT PARSING ===
    parser = argparse.ArgumentParser(description="DCS-BIOS UDP Replay Tool")
    parser.add_argument("--fps", type=float, default=30.0,
                        help="Fixed replay rate in FPS (default: 30, matching DCS-BIOS tick rate)")
    parser.add_argument("--raw-timing", action="store_true",
                        help="Use recorded per-frame timing instead of fixed FPS")
    parser.add_argument("--speed", type=float, default=1.0,
                        help="Speed multiplier for --raw-timing mode (e.g. 2.0 = 2x faster)")
    parser.add_argument("--stream", type=str,
                        help="Stream identifier to load directly (bypasses menu)")
    args = parser.parse_args()

    # Change to script directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    # === INTERFACE SELECTION ===
    interfaces = get_all_interface_ips()
    selected_interfaces = pick_interface(interfaces)

    # === STREAM SELECTION ===
    streams_path = os.path.join(os.getcwd(), STREAMS_DIR)
    available_streams = discover_streams(streams_path)

    if not available_streams:
        print("\nNo stream files found!")
        print(f"   Expected location: {streams_path}")
        print(f"   Expected pattern:  {FILE_PREFIX}<identifier>")
        print("\n   Run RECORD_DCS_stream.py to capture a stream.")
        sys.exit(1)

    # Determine which stream to use
    if args.stream:
        match = None
        for stream in available_streams:
            if stream[0].upper() == args.stream.upper():
                match = stream
                break
        if not match:
            print(f"\nStream '{args.stream}' not found!")
            print("   Available streams:", ", ".join(s[0] for s in available_streams))
            sys.exit(1)
        selected = match
    else:
        selected = select_stream(available_streams)

    identifier, INPUT_JSON_FILE, display_name, age = selected

    # === SETUP SOCKETS FOR SELECTED INTERFACES ===
    multicast_sockets = []

    for iface_name, ip in selected_interfaces:
        try:
            sock = create_multicast_socket(ip)
            multicast_sockets.append((ip, iface_name, sock))
        except Exception:
            pass  # Silently skip interfaces that fail

    if not multicast_sockets:
        # Fallback: create a socket without interface binding
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
            multicast_sockets.append(("default", "default route", sock))
        except Exception as e:
            print(f"\nNo usable network interfaces found! ({e})")
            sys.exit(1)

    # === LOAD DATA ===
    with open(INPUT_JSON_FILE, "r") as f:
        frames = json.load(f)

    # === STATUS OUTPUT ===
    iface_list = ", ".join(f"{ip} ({name})" for ip, name, _ in multicast_sockets)

    if args.raw_timing:
        timing_label = f"recorded x{args.speed}"
    else:
        timing_label = f"{args.fps:.0f} FPS ({1000/args.fps:.1f}ms/frame)"

    print()
    print("+----------------------------------------------------------------+")
    _box_line(f"  {BOLD}CockpitOS DCS-BIOS Stream Replay{RESET}")
    print("+----------------------------------------------------------------+")
    _box_line(f"  Stream:    {CYAN}{display_name}{RESET}")
    _box_line(f"  Frames:    {CYAN}{len(frames)}{RESET}")
    _box_line(f"  Target:    {CYAN}{MULTICAST_IP}:{UDP_PORT}{RESET}")
    _box_line(f"  Interface: {CYAN}{iface_list}{RESET}")
    _box_line(f"  Timing:    {CYAN}{timing_label}{RESET}")
    _box_line(f"  {DIM}Press Ctrl+C to stop{RESET}")
    print("+----------------------------------------------------------------+")
    print()

    # === REPLAY LOOP ===
    frame_timestamps = []
    accum_time = 0.0
    for frame in frames:
        frame_timestamps.append(accum_time)
        if args.raw_timing:
            accum_time += frame.get("timing", 0) / args.speed
        else:
            accum_time += 1.0 / args.fps

    stream_start_time = time.perf_counter()
    cycle = 0

    try:
        while True:
            for i, frame in enumerate(frames):
                hex_data = frame.get("data", "")
                if not hex_data:
                    continue

                target_time = stream_start_time + frame_timestamps[i] + (cycle * accum_time)
                now = time.perf_counter()
                wait_time = target_time - now

                if wait_time > 0:
                    time.sleep(wait_time)

                try:
                    binary_data = binascii.unhexlify(hex_data)
                    for _, _, sock in multicast_sockets:
                        sock.sendto(binary_data, (MULTICAST_IP, UDP_PORT))
                except Exception as e:
                    print(f"  {RED}[ERROR]{RESET} {e}")

            cycle += 1
            print(f"  {GREEN}Completed cycle {cycle}{RESET}")

    except KeyboardInterrupt:
        print("\n\nStopped by user")

    finally:
        for _, _, sock in multicast_sockets:
            try:
                sock.close()
            except Exception:
                pass


if __name__ == "__main__":
    main()
