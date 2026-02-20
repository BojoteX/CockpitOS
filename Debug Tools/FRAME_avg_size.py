#!/usr/bin/env python3
"""
CockpitOS DCS-BIOS Frame Size Analyzer
=======================================

Analyzes captured DCS-BIOS stream files to calculate average frame size.
Useful for understanding bandwidth requirements.
Arrow-key stream selection when multiple streams are available.

Usage:
    python FRAME_avg_size.py                    # Pick stream interactively
    python FRAME_avg_size.py --stream SHORT     # Analyze specific stream

Author: CockpitOS Project
"""

import json
import os
import sys
import msvcrt
import ctypes
import argparse
import time

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

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

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

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
# ARROW-KEY PICKER
# ═══════════════════════════════════════════════════════════════════════════════

def arrow_pick(prompt, options):
    """Arrow-key picker. options = list of (label, value). Returns value or None on Esc."""
    if not options:
        return None

    total = len(options)
    idx = 0

    print()
    _w(f"  {BOLD}{prompt}{RESET}\n")
    _w(f"  {DIM}(Up/Down to move, Enter to select, Esc to quit){RESET}\n")

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

            elif ch == "\x1b":      # Esc
                _w(SHOW_CUR)
                return None

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        sys.exit(0)

# ═══════════════════════════════════════════════════════════════════════════════
# STREAM DISCOVERY
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
    """Find available stream files. Returns list of (identifier, full_path)."""
    streams = []

    if not os.path.isdir(streams_path):
        return streams

    for filename in os.listdir(streams_path):
        if filename.startswith(FILE_PREFIX):
            identifier = filename[len(FILE_PREFIX):]
            full_path = os.path.join(streams_path, filename)
            streams.append((identifier, full_path))

    streams.sort(key=lambda x: (x[0] != "LAST", x[0]))
    return streams

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(description="Analyze DCS-BIOS frame sizes")
    parser.add_argument("--stream", type=str, default=None, help="Stream identifier (default: interactive pick)")
    args = parser.parse_args()

    # Header
    print()
    print("+----------------------------------------------------------------+")
    _box_line(f"  {BOLD}CockpitOS Frame Size Analyzer{RESET}")
    print("+----------------------------------------------------------------+")

    # Find stream files
    streams_path = os.path.join(os.getcwd(), STREAMS_DIR)
    available = discover_streams(streams_path)

    if not available:
        _box_line(f"  {RED}No stream files found!{RESET}")
        _box_line(f"  Expected in: {STREAMS_DIR}/")
        print("+----------------------------------------------------------------+")
        input("\n  Press <ENTER> to exit...")
        sys.exit(1)

    # Select stream
    if args.stream:
        # CLI argument
        match = None
        for identifier, path in available:
            if identifier.upper() == args.stream.upper():
                match = (identifier, path)
                break
        if not match:
            _box_line(f"  {RED}Stream '{args.stream}' not found!{RESET}")
            _box_line(f"  Available: {', '.join(s[0] for s in available)}")
            print("+----------------------------------------------------------------+")
            input("\n  Press <ENTER> to exit...")
            sys.exit(1)
        identifier, filepath = match
    elif len(available) == 1:
        # Only one stream, auto-select
        identifier, filepath = available[0]
    else:
        # Arrow-key picker
        options = []
        for ident, path in available:
            age = get_relative_age(path)
            if ident == "LAST":
                label = f"{CYAN}LAST{RESET} (Last captured)      {DIM}({age}){RESET}"
            else:
                label = f"{CYAN}{ident:<28}{RESET} {DIM}({age}){RESET}"
            options.append((label, ident))

        chosen = arrow_pick("Select stream to analyze:", options)

        if chosen is None:
            print(f"\n  {DIM}Cancelled.{RESET}")
            sys.exit(0)

        # Find the matching stream
        filepath = None
        identifier = chosen
        for ident, path in available:
            if ident == chosen:
                filepath = path
                break

    _box_line(f"  Analyzing: {CYAN}{identifier}{RESET}")
    print("+----------------------------------------------------------------+")

    # Load and parse
    try:
        with open(filepath, "r") as f:
            data = json.load(f)
    except Exception as e:
        print(f"\n  {RED}ERROR:{RESET} Failed to load file: {e}")
        input("\n  Press <ENTER> to exit...")
        sys.exit(1)

    # Handle different JSON structures
    if isinstance(data, dict) and "frames" in data:
        frames = data["frames"]
    elif isinstance(data, list):
        frames = data
    else:
        print(f"\n  {RED}ERROR:{RESET} Unrecognized JSON structure")
        input("\n  Press <ENTER> to exit...")
        sys.exit(1)

    # Calculate frame sizes (hex string length / 2 = actual bytes)
    frame_sizes = []
    for frame in frames:
        if isinstance(frame, dict) and "data" in frame:
            hex_data = frame.get("data", "")
            byte_count = len(hex_data) // 2  # 2 hex chars = 1 byte
            frame_sizes.append(byte_count)

    if len(frame_sizes) < 3:
        print(f"\n  {RED}ERROR:{RESET} Not enough frames to analyze (need at least 3)")
        input("\n  Press <ENTER> to exit...")
        sys.exit(1)

    # Statistics
    frame_sizes.sort()
    trimmed = frame_sizes[1:-1]  # Discard min and max outliers

    avg_size = sum(trimmed) / len(trimmed)
    min_size = min(frame_sizes)
    max_size = max(frame_sizes)
    total_bytes = sum(frame_sizes)

    print()
    print(f"  {DIM}Frames{RESET}")
    print(f"    Total:            {BOLD}{len(frame_sizes)}{RESET}")
    print(f"    Analyzed:         {len(trimmed)} {DIM}(excluding min/max outliers){RESET}")
    print()
    print(f"  {DIM}Size{RESET}")
    print(f"    Average:          {GREEN}{avg_size:.1f} bytes{RESET}")
    print(f"    Min:              {CYAN}{min_size} bytes{RESET}")
    print(f"    Max:              {CYAN}{max_size} bytes{RESET}")
    print(f"    Total:            {total_bytes:,} bytes ({total_bytes/1024:.1f} KB)")
    print()

    input("  Press <ENTER> to exit...")


if __name__ == "__main__":
    main()
