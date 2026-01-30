#!/usr/bin/env python3
"""
CockpitOS DCS Command Tester
=============================

Interactive menu to send DCS-BIOS commands for testing.

Usage:
    python SEND_CommandTester.py                     # Use localhost
    python SEND_CommandTester.py --ip 192.168.1.100  # Specify DCS PC IP

Author: CockpitOS Project
"""

import time
import socket
import argparse
import os

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

DCS_PORT = 7778

# ═══════════════════════════════════════════════════════════════════════════════
# COMMAND DEFINITIONS
# ═══════════════════════════════════════════════════════════════════════════════

BATTERY_CMDS = {
    "OFF":   "BATTERY_SW 1",
    "ON":    "BATTERY_SW 2",
    "ORIDE": "BATTERY_SW 0",
}

LTDR_CMDS = {
    "SAFE":   "LTD_R_SW 0",
    "ARM":    "LTD_R_SW 1",
    "INC":    "LTD_R_SW INC",
    "DEC":    "LTD_R_SW DEC",
    "TOGGLE": "LTD_R_SW TOGGLE",
}

GAIN_CMDS = {
    "NORM":  "GAIN_SWITCH 0",
    "ORIDE": "GAIN_SWITCH 1",
}

HOOK_CMDS = {
    "DOWN": "HOOK_LEVER 0",
    "UP":   "HOOK_LEVER 1",
}

# ═══════════════════════════════════════════════════════════════════════════════
# UDP SEND
# ═══════════════════════════════════════════════════════════════════════════════

DCS_IP = "127.0.0.1"  # Will be set by args

def send_udp(cmd: str) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto((cmd + "\n").encode(), (DCS_IP, DCS_PORT))
        print(f"  [SENT] {cmd}")

# ═══════════════════════════════════════════════════════════════════════════════
# MENU HELPERS
# ═══════════════════════════════════════════════════════════════════════════════

def numbered_choice(title: str, options: dict) -> str | None:
    """Show a numbered menu. Returns selected key or None."""
    keys = list(options.keys())
    print(f"\n  == {title} ==")
    for i, k in enumerate(keys, 1):
        print(f"  {i}. {k}")
    print("  0. Back")

    choice = input("  Select: ").strip()

    if choice.isdigit():
        n = int(choice)
        if n == 0:
            return None
        if 1 <= n <= len(keys):
            return keys[n - 1]
        print("  Invalid selection.")
        return None

    key = choice.upper()
    if key in options:
        return key

    print("  Invalid selection.")
    return None

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    global DCS_IP
    
    parser = argparse.ArgumentParser(description="DCS-BIOS Command Tester")
    parser.add_argument("--ip", default="127.0.0.1", help="DCS PC IP address (default: 127.0.0.1)")
    args = parser.parse_args()
    
    DCS_IP = args.ip

    print()
    print("+----------------------------------------------------------------+")
    print("|            CockpitOS DCS Command Tester                        |")
    print("+----------------------------------------------------------------+")
    print(f"|  Target: {DCS_IP}:{DCS_PORT}".ljust(65) + "|")
    print("+----------------------------------------------------------------+")

    while True:
        print("\n  === Command Menu ===")
        print("  1. BATTERY_SW")
        print("  2. LTD/R")
        print("  3. GAIN_SWITCH")
        print("  4. HOOK")
        print("  5. Exit")
        top = input("  Select: ").strip()

        if top == "1":
            sel = numbered_choice("BATTERY_SW", BATTERY_CMDS)
            if sel:
                send_udp(BATTERY_CMDS[sel])

        elif top == "2":
            sel = numbered_choice("LTD/R", LTDR_CMDS)
            if sel:
                send_udp(LTDR_CMDS[sel])

        elif top == "3":
            sel = numbered_choice("GAIN_SWITCH", GAIN_CMDS)
            if sel:
                if sel == "ORIDE":
                    send_udp("GAIN_SWITCH_COVER 1")
                    time.sleep(0.100)
                    send_udp(GAIN_CMDS[sel])
                elif sel == "NORM":
                    send_udp(GAIN_CMDS[sel])
                    time.sleep(0.100)
                    send_udp("GAIN_SWITCH_COVER 0")

        elif top == "4":
            sel = numbered_choice("HOOK_LEVER", HOOK_CMDS)
            if sel:
                send_udp(HOOK_CMDS[sel])

        elif top == "5":
            print("\n  Goodbye!")
            break

        else:
            print("  Invalid option.")


if __name__ == "__main__":
    main()
