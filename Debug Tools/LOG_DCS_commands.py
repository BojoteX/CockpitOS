#!/usr/bin/env python3
"""
CockpitOS DCS Command Logger
=============================

Listens for DCS-BIOS commands sent by panels (UDP port 7778).
Useful for debugging what commands your panel is sending.

WARNING: Do NOT run this while DCS is running - it will intercept commands!

Usage:
    python LOG_DCS_commands.py

Author: CockpitOS Project
"""

import socket
import os
from datetime import datetime

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

PORT = 7778  # DCS-BIOS command import port

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    # Setup socket - bind to all interfaces
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(0.5)  # 500ms timeout so Ctrl+C works on Windows
    sock.bind(('0.0.0.0', PORT))

    # Header
    print()
    print("+----------------------------------------------------------------+")
    print("|            CockpitOS DCS Command Logger                        |")
    print("+----------------------------------------------------------------+")
    print("|  WARNING: Do NOT run while DCS is running!                     |")
    print("|           This will intercept commands meant for DCS.          |")
    print("+----------------------------------------------------------------+")
    print(f"|  Listening on port {PORT} (all interfaces)                       |")
    print("|  Press Ctrl+C to stop                                          |")
    print("+----------------------------------------------------------------+")
    print()

    try:
        while True:
            try:
                data, addr = sock.recvfrom(1024)
            except socket.timeout:
                continue  # Check for Ctrl+C, then loop back

            sender_ip, sender_port = addr
            msg = data.decode('utf-8', errors='ignore').strip()
            timestamp = datetime.now().strftime('%H:%M:%S')

            print(f"[{timestamp}] [{sender_ip}] {msg}")

    except KeyboardInterrupt:
        print("\n\nStopped by user")

    finally:
        sock.close()


if __name__ == "__main__":
    main()
