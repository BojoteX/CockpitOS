#!/usr/bin/env python3
"""
CockpitOS Management Tool
==========================
Interactive build, configure, and deploy for CockpitOS firmware.

Double-click this file or run:  python CockpitOS-START.py
"""

import sys
import platform

if platform.system() != "Windows":
    print("\n  CockpitOS Management Tool requires Windows.")
    print("  Linux and macOS are not supported.\n")
    sys.exit(1)

from pathlib import Path

# Add the compiler directory to the module search path so all
# internal modules (ui, config, boards, build, labels, cockpitos)
# can find each other via simple imports.
_compiler_dir = Path(__file__).resolve().parent / "compiler"
sys.path.insert(0, str(_compiler_dir))

from cockpitos import main

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n  Interrupted.")
        sys.exit(0)
