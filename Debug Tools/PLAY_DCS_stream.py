#!/usr/bin/env python3
"""
CockpitOS DCS-BIOS Stream Replay Tool
=====================================

Replays captured DCS-BIOS data streams for offline testing.
Sends multicast on ALL network interfaces for universal compatibility.

Usage:
    python PLAY_DCS_stream.py                    # Interactive menu
    python PLAY_DCS_stream.py --stream LAST      # Play last captured
    python PLAY_DCS_stream.py --speed 2.0        # 2x speed
    python PLAY_DCS_stream.py --fps 60           # Fixed 60 FPS

Author: CockpitOS Project
"""

import socket
import json
import time
import binascii
import os
import sys
import argparse
from datetime import datetime

# ═══════════════════════════════════════════════════════════════════════════════
# DEPENDENCY CHECK
# ═══════════════════════════════════════════════════════════════════════════════

try:
    import ifaddr
except ImportError:
    print()
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║  ❌ Missing Required Module: ifaddr                            ║")
    print("╠════════════════════════════════════════════════════════════════╣")
    print("║                                                                ║")
    print("║  This tool requires 'ifaddr' for network interface discovery. ║")
    print("║                                                                ║")
    print("║  Install it with:                                              ║")
    print("║                                                                ║")
    print("║      pip install ifaddr                                        ║")
    print("║                                                                ║")
    print("║  Then run this script again.                                   ║")
    print("║                                                                ║")
    print("╚════════════════════════════════════════════════════════════════╝")
    print()

    # Press ENTER to exit
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
# MULTI-INTERFACE NETWORK SUPPORT
# ═══════════════════════════════════════════════════════════════════════════════

def get_all_interface_ips():
    """
    Discover all usable IPv4 interface addresses on this machine.
    Returns list of (interface_name, ip_address) tuples.
    
    Filters out:
      - Loopback (127.x.x.x)
      - APIPA/link-local (169.254.x.x) - indicates no network config
    
    Uses ifaddr for bulletproof cross-platform interface enumeration.
    """
    interfaces = []
    
    for adapter in ifaddr.get_adapters():
        for ip in adapter.ips:
            # Only IPv4 addresses (ip.ip is a string for IPv4, tuple for IPv6)
            if isinstance(ip.ip, str):
                # Skip loopback and APIPA (link-local) addresses
                if ip.ip.startswith('127.') or ip.ip.startswith('169.254.'):
                    continue
                # Shorten the adapter name for cleaner output
                short_name = shorten_adapter_name(adapter.nice_name)
                interfaces.append((short_name, ip.ip))
    
    return interfaces


def shorten_adapter_name(name):
    """Shorten long Windows adapter names to something readable."""
    # Common substitutions
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
        ("  ", " "),  # collapse double spaces
    ]
    
    result = name
    for old, new in replacements:
        result = result.replace(old, new)
    
    return result.strip()[:30]  # Max 30 chars


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
    """Display interactive menu and get user selection."""
    print("\n" + "=" * 50)
    print("       DCS-BIOS Stream Selector")
    print("=" * 50)
    
    for idx, (identifier, path, display_name, age) in enumerate(streams, start=1):
        print(f"  [{idx}] {display_name:<30} ({age})")
    
    print("=" * 50)
    
    while True:
        try:
            choice = input("Select stream number: ").strip()
            choice_num = int(choice)
            if 1 <= choice_num <= len(streams):
                return streams[choice_num - 1]
            else:
                print(f"⚠️  Enter a number between 1 and {len(streams)}")
        except ValueError:
            print("⚠️  Enter a valid number")


# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    # === ARGUMENT PARSING ===
    parser = argparse.ArgumentParser(description="DCS-BIOS UDP Replay Tool")
    parser.add_argument("--speed", type=float, default=1.0, 
                        help="Replay speed multiplier (e.g. 2.0 = 2x faster)")
    parser.add_argument("--fps", type=float, 
                        help="Override all delays and force fixed FPS (e.g. 60)")
    parser.add_argument("--stream", type=str, 
                        help="Stream identifier to load directly (bypasses menu)")
    args = parser.parse_args()

    # Change to script directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    # === STREAM SELECTION ===
    streams_path = os.path.join(os.getcwd(), STREAMS_DIR)
    available_streams = discover_streams(streams_path)

    if not available_streams:
        print("\n❌ No stream files found!")
        print(f"   Expected location: {streams_path}")
        print(f"   Expected pattern:  {FILE_PREFIX}<identifier>")
        print("\n💡 Run RECORD_DCS_stream.py to capture a stream.")
        print("   It will create: dcsbios_data.json.LAST")
        sys.exit(1)

    # Determine which stream to use
    if args.stream:
        match = None
        for stream in available_streams:
            if stream[0].upper() == args.stream.upper():
                match = stream
                break
        if not match:
            print(f"\n❌ Stream '{args.stream}' not found!")
            print("   Available streams:", ", ".join(s[0] for s in available_streams))
            sys.exit(1)
        selected = match
        print(f"\n🎯 Auto-selected stream: {selected[2]} ({selected[3]})")
    else:
        selected = select_stream(available_streams)

    identifier, INPUT_JSON_FILE, display_name, age = selected

    # === SETUP SOCKETS FOR ALL INTERFACES ===
    interfaces = get_all_interface_ips()
    multicast_sockets = []

    for iface_name, ip in interfaces:
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
            print(f"\n❌ No usable network interfaces found! ({e})")
            sys.exit(1)

    # === LOAD DATA ===
    with open(INPUT_JSON_FILE, "r") as f:
        frames = json.load(f)

    # === STATUS OUTPUT ===
    print(f"\n[INFO] Loaded {len(frames)} frames from {os.path.basename(INPUT_JSON_FILE)}")
    
    iface_list = ", ".join(f"{ip} ({name})" for ip, name, _ in multicast_sockets)
    print(f"[INFO] Multicast to {MULTICAST_IP}:{UDP_PORT} via: {iface_list}")

    if args.fps:
        print(f"🔁 Fixed replay at {args.fps:.1f} FPS ({1000/args.fps:.2f}ms/frame)")
    else:
        print(f"🔁 Using recorded frame timing scaled by x{args.speed}")

    print("⏳ Press Ctrl+C to stop.\n")

    # === REPLAY LOOP ===
    frame_timestamps = []
    accum_time = 0.0
    for frame in frames:
        frame_timestamps.append(accum_time)
        accum_time += frame.get("timing", 0) / args.speed if not args.fps else (1.0 / args.fps)

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
                    print(f"⚠️ Error sending frame: {e}")

            cycle += 1
            print(f"✅ Completed cycle {cycle}")

    except KeyboardInterrupt:
        print("\n\n🛑 Stopped by user")

    finally:
        for _, _, sock in multicast_sockets:
            try:
                sock.close()
            except Exception:
                pass


if __name__ == "__main__":
    main()
