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

# Add shared/ and label_creator/ to the module search path.
_project_root = Path(__file__).resolve().parent
sys.path.insert(0, str(_project_root / "shared"))
sys.path.insert(0, str(_project_root / "label_creator"))

from label_creator import main

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n  Interrupted.")
        sys.exit(0)
