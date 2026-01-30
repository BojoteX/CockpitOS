#!/usr/bin/env python3
"""
CockpitOS UDP Debug Console
============================

Receives and logs debug messages from CockpitOS devices over WiFi UDP.
ESP32 devices broadcast to 255.255.255.255:4210, which is received on all interfaces.

Usage:
    python CONSOLE_UDP_debug.py                     # Log all messages
    python CONSOLE_UDP_debug.py --ip 192.168.1.50   # Filter by source IP

Author: CockpitOS Project
"""

import socket
import os
import sys
import argparse
from datetime import datetime
import shutil

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

PORT = 4210  # CockpitOS debug broadcast port

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

    # Setup socket - bind to all interfaces
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.settimeout(0.5)  # 500ms timeout so Ctrl+C works on Windows
    sock.bind(('0.0.0.0', PORT))

    # Header (no emojis in the box to avoid alignment issues)
    print()
    print("+----------------------------------------------------------------+")
    print("|            CockpitOS UDP Debug Console                         |")
    print("+----------------------------------------------------------------+")
    print(f"|  Listening on port {PORT} (all interfaces)                       |")
    if args.ip:
        print(f"|  Filtering: {args.ip:<51}|")
    print("|  Logging to: logs/udpLogger.log                                |")
    print("|  Press Ctrl+C to stop                                          |")
    print("+----------------------------------------------------------------+")
    print()

    log_write(f"=== Session started at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')} ===")
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
            timestamp = datetime.now().strftime('%H:%M:%S')
            line = f"[{timestamp}] [{sender_ip}] {msg}"
            print(line)
            log_write(line)

    except KeyboardInterrupt:
        print("\n\nStopped by user")
        log_write(f"\n=== Session ended at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')} ===")

    finally:
        sock.close()


if __name__ == "__main__":
    main()
