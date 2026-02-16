#!/usr/bin/env python3
"""
CockpitOS Label Creator — Entry point.

Launch the interactive Label Set creation tool.
"""
import sys
import platform

if platform.system() != "Windows":
    print("\n  CockpitOS Label Creator requires Windows.")
    sys.exit(1)

from pathlib import Path

_creator_dir = Path(__file__).resolve().parent / "label_creator"
sys.path.insert(0, str(_creator_dir))

from label_creator import main

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n  Interrupted.")
        sys.exit(0)
