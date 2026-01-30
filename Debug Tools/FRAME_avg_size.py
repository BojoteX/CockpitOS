#!/usr/bin/env python3
"""
CockpitOS DCS-BIOS Frame Size Analyzer
=======================================

Analyzes captured DCS-BIOS stream files to calculate average frame size.
Useful for understanding bandwidth requirements.

Usage:
    python avg_frame_size.py                    # Analyze LAST stream
    python avg_frame_size.py --stream SHORT     # Analyze specific stream

Author: CockpitOS Project
"""

import json
import os
import sys
import argparse

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

STREAMS_DIR = "streams"
FILE_PREFIX = "dcsbios_data.json."

# ═══════════════════════════════════════════════════════════════════════════════
# STREAM DISCOVERY
# ═══════════════════════════════════════════════════════════════════════════════

def discover_streams(streams_path):
    """Find available stream files."""
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
    parser.add_argument("--stream", type=str, default="LAST", help="Stream identifier (default: LAST)")
    args = parser.parse_args()

    # Header
    print()
    print("+----------------------------------------------------------------+")
    print("|            CockpitOS Frame Size Analyzer                       |")
    print("+----------------------------------------------------------------+")

    # Find stream file
    streams_path = os.path.join(os.getcwd(), STREAMS_DIR)
    available = discover_streams(streams_path)
    
    if not available:
        print("|  ERROR: No stream files found!                                |")
        print(f"|  Expected in: {STREAMS_DIR}/".ljust(65) + "|")
        print("+----------------------------------------------------------------+")
        sys.exit(1)

    # Match requested stream
    match = None
    for identifier, path in available:
        if identifier.upper() == args.stream.upper():
            match = (identifier, path)
            break
    
    if not match:
        print(f"|  ERROR: Stream '{args.stream}' not found!".ljust(65) + "|")
        print("|  Available:".ljust(65) + "|")
        for identifier, _ in available:
            print(f"|    - {identifier}".ljust(65) + "|")
        print("+----------------------------------------------------------------+")
        sys.exit(1)

    identifier, filepath = match
    print(f"|  Analyzing: {identifier}".ljust(65) + "|")
    print("+----------------------------------------------------------------+")

    # Load and parse
    try:
        with open(filepath, "r") as f:
            data = json.load(f)
    except Exception as e:
        print(f"\n  ERROR: Failed to load file: {e}")
        sys.exit(1)

    # Handle different JSON structures
    if isinstance(data, dict) and "frames" in data:
        frames = data["frames"]
    elif isinstance(data, list):
        frames = data
    else:
        print("\n  ERROR: Unrecognized JSON structure")
        sys.exit(1)

    # Calculate frame sizes (hex string length / 2 = actual bytes)
    frame_sizes = []
    for frame in frames:
        if isinstance(frame, dict) and "data" in frame:
            hex_data = frame.get("data", "")
            byte_count = len(hex_data) // 2  # 2 hex chars = 1 byte
            frame_sizes.append(byte_count)

    if len(frame_sizes) < 3:
        print("\n  ERROR: Not enough frames to analyze (need at least 3)")
        sys.exit(1)

    # Statistics
    frame_sizes.sort()
    trimmed = frame_sizes[1:-1]  # Discard min and max outliers
    
    avg_size = sum(trimmed) / len(trimmed)
    min_size = min(frame_sizes)
    max_size = max(frame_sizes)
    total_bytes = sum(frame_sizes)

    print()
    print(f"  Total frames:       {len(frame_sizes)}")
    print(f"  Analyzed frames:    {len(trimmed)} (excluding min/max outliers)")
    print()
    print(f"  Average frame size: {avg_size:.1f} bytes")
    print(f"  Min frame size:     {min_size} bytes")
    print(f"  Max frame size:     {max_size} bytes")
    print(f"  Total data:         {total_bytes:,} bytes ({total_bytes/1024:.1f} KB)")
    print()

    input("  Press <ENTER> to exit...")


if __name__ == "__main__":
    main()
