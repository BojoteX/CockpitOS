#!/usr/bin/env python3

import os
import argparse
import socket
import time

DCS_IP = "192.168.7.166"
DCS_PORT = 7778

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

DEFAULT_COMMANDS = [
    "WING_FOLD_PULL 0",
    "WING_FOLD_ROTATE 1",
]

"""
bulk_send_debug.py

Send multiple UDP commands in order with an optional debounce between each.

Usage:
    bulk_send_debug.py --debounce 0   "CMD1 ARG" "CMD2 ARG"
    bulk_send_debug.py --debounce 100 "WING_FOLD_PULL 0" "WING_FOLD_ROTATE 1"

If no commands are provided, DEFAULT_COMMANDS will be used.
"""

def send_udp(cmd: str) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.sendto((cmd + "\n").encode("utf-8"), (DCS_IP, DCS_PORT))
    print(f"[SENT] {cmd}")


def send_bulk(commands, debounce_ms: int) -> None:
    delay = max(0, debounce_ms) / 1000.0

    for i, cmd in enumerate(commands):
        if i > 0 and delay > 0:
            time.sleep(delay)
        send_udp(cmd)


def main():
    parser = argparse.ArgumentParser(description="Send DCS commands in bulk.")
    parser.add_argument(
        "--debounce", type=int, default=0,
        help="Delay in milliseconds between each command (0 = no wait)"
    )
    parser.add_argument(
        "commands", nargs="*",
        help="Each command is a separate positional argument"
    )

    args = parser.parse_args()

    # If no commands provided → use defaults
    if not args.commands:
        print("[INFO] No commands provided. Using default batch:")
        for c in DEFAULT_COMMANDS:
            print("  ", c)
        cmds = DEFAULT_COMMANDS
    else:
        cmds = args.commands

    send_bulk(cmds, args.debounce)
    input("\nPress <ENTER> to exit...")

if __name__ == "__main__":
    main()
