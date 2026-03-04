#!/usr/bin/env python3
"""CockpitOS -- double-click this file to get started."""
import os, sys
from pathlib import Path

setup = str(Path(__file__).resolve().parent / "Setup-START.py")
os.execl(sys.executable, sys.executable, setup)
