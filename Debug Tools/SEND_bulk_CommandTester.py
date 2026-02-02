#!/usr/bin/env python3
"""
CockpitOS DCS Bulk Command Sender
==================================

Send multiple DCS-BIOS commands in sequence with optional delay between each.

Usage:
    python SEND_bulk_CommandTester.py "CMD1 ARG" "CMD2 ARG"
    python SEND_bulk_CommandTester.py --ip 192.168.1.100 "BATTERY_SW 2"
    python SEND_bulk_CommandTester.py --debounce 100 "WING_FOLD_PULL 0" "WING_FOLD_ROTATE 1"

If no commands provided, uses built-in defaults for testing.

Author: CockpitOS Project
"""

import os
import argparse
import socket
import time

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

DCS_PORT = 7778

DEFAULT_COMMANDS = [
    "WING_FOLD_PULL 0",
    "WING_FOLD_ROTATE 1",
]

# ═══════════════════════════════════════════════════════════════════════════════
# UDP SEND
# ═══════════════════════════════════════════════════════════════════════════════

def send_udp(cmd: str, dcs_ip: str) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto((cmd + "\n").encode("utf-8"), (dcs_ip, DCS_PORT))
    print(f"  [SENT] {cmd}")


def send_bulk(commands: list, dcs_ip: str, debounce_ms: int) -> None:
    delay = max(0, debounce_ms) / 1000.0

    for i, cmd in enumerate(commands):
        if i > 0 and delay > 0:
            time.sleep(delay)
        send_udp(cmd, dcs_ip)

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(description="Send DCS-BIOS commands in bulk.")
    parser.add_argument("--ip", default="127.0.0.1", help="DCS PC IP address (default: 127.0.0.1)")
    parser.add_argument("--debounce", type=int, default=0, help="Delay in ms between commands (default: 0)")
    parser.add_argument("commands", nargs="*", help="Commands to send (each as separate argument)")
    args = parser.parse_args()

    # Header
    print()
    print("+----------------------------------------------------------------+")
    print("|            CockpitOS DCS Bulk Command Sender                   |")
    print("+----------------------------------------------------------------+")
    print(f"|  Target: {args.ip}:{DCS_PORT}".ljust(65) + "|")
    print(f"|  Debounce: {args.debounce}ms".ljust(65) + "|")
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
    send_bulk(cmds, args.ip, args.debounce)
    
    print()
    input("  Press <ENTER> to exit...")


if __name__ == "__main__":
    main()
