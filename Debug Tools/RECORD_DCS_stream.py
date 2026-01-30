#!/usr/bin/env python3
"""
CockpitOS DCS-BIOS Stream Recorder
===================================

Records DCS-BIOS UDP multicast stream for offline replay/testing.
Joins multicast group on ALL network interfaces for universal compatibility.

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

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

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
    print("|  This tool requires 'ifaddr' for network interface discovery. |")
    print("|                                                                |")
    print("|  Install it with:                                              |")
    print("|                                                                |")
    print("|      pip install ifaddr                                        |")
    print("|                                                                |")
    print("|  Then run this script again.                                   |")
    print("|                                                                |")
    print("+----------------------------------------------------------------+")
    print()
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
# MULTI-INTERFACE SUPPORT
# ═══════════════════════════════════════════════════════════════════════════════

def get_all_interface_ips():
    """
    Discover all usable IPv4 interface addresses on this machine.
    Filters out loopback (127.x.x.x) and APIPA (169.254.x.x).
    """
    interfaces = []
    
    for adapter in ifaddr.get_adapters():
        for ip in adapter.ips:
            if isinstance(ip.ip, str):
                if ip.ip.startswith('127.') or ip.ip.startswith('169.254.'):
                    continue
                # Shorten name for display
                short_name = adapter.nice_name[:30]
                interfaces.append((short_name, ip.ip))
    
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

    # Setup socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(0.5)  # So Ctrl+C works on Windows
    sock.bind(('0.0.0.0', UDP_PORT))

    # Join multicast group on ALL interfaces
    interfaces = get_all_interface_ips()
    joined_count = 0

    # Header
    print()
    print("+----------------------------------------------------------------+")
    print("|            CockpitOS DCS-BIOS Stream Recorder                  |")
    print("+----------------------------------------------------------------+")
    print(f"|  Multicast group: {MULTICAST_GROUP}:{UDP_PORT}                        |")
    print("|  Joining on interfaces:                                        |")

    if not interfaces:
        # Fallback: join on default
        try:
            mreq = socket.inet_aton(MULTICAST_GROUP) + socket.inet_aton('0.0.0.0')
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            print("|    - default route                                            |")
            joined_count = 1
        except Exception as e:
            print(f"|    - FAILED: {str(e)[:45]:<45}|")
    else:
        for iface_name, ip in interfaces:
            try:
                mreq = socket.inet_aton(MULTICAST_GROUP) + socket.inet_aton(ip)
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
                print(f"|    - {ip:<15} ({iface_name[:30]:<30})|"[:65] + "|")
                joined_count += 1
            except Exception:
                pass  # Silently skip interfaces that fail

    if joined_count == 0:
        print("+----------------------------------------------------------------+")
        print("|  ERROR: Could not join multicast on any interface!            |")
        print("+----------------------------------------------------------------+")
        sys.exit(1)

    print("+----------------------------------------------------------------+")
    print(f"|  Output: {OUTPUT_FILE:<53}|")
    print("|  Press Ctrl+C to stop recording                                |")
    print("+----------------------------------------------------------------+")
    print()

    # Recording state
    log_data = []
    buffer = bytearray()
    last_time = time.time()
    first_packet = True

    try:
        print("Waiting for DCS-BIOS stream...")
        
        while True:
            try:
                data, addr = sock.recvfrom(BUFFER_SIZE)
            except socket.timeout:
                continue

            now = time.time()
            elapsed = 2.0 if first_packet else now - last_time
            last_time = now
            
            if first_packet:
                print(f"Receiving from {addr[0]} - recording started!")
                first_packet = False

            buffer.extend(data)
            new_frames = extract_frames(buffer, elapsed)
            buffer.clear()
            log_data.extend(new_frames)

            # Auto-save every 10 frames
            if len(log_data) % 10 == 0:
                with open(OUTPUT_FILE, "w") as f:
                    json.dump(log_data, f, indent=2)
                print(f"\r  Recorded {len(log_data)} frames...", end="", flush=True)

    except KeyboardInterrupt:
        print("\n\nStopped by user")

    finally:
        with open(OUTPUT_FILE, "w") as f:
            json.dump(log_data, f, indent=2)
        print(f"Saved {len(log_data)} frames to {OUTPUT_FILE}")
        sock.close()


if __name__ == "__main__":
    main()
