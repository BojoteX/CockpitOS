"""
CockpitOS — Label set management.

Discovery, selection, and generate_data.py execution.
"""

import subprocess
import sys
import re

from ui import (
    CYAN, DIM, RESET,
    header, info, success, warn, error,
    pick,
)
from config import SKETCH_DIR

from pathlib import Path

LABELS_DIR  = SKETCH_DIR / "src" / "LABELS"
ACTIVE_SET  = LABELS_DIR / "active_set.h"


def discover_label_sets():
    """Find all LABEL_SET_* directories under src/LABELS/."""
    sets = []
    if LABELS_DIR.exists():
        for d in sorted(LABELS_DIR.iterdir()):
            if d.is_dir() and d.name.startswith("LABEL_SET_"):
                name = d.name.replace("LABEL_SET_", "")
                has_gen = (d / "generate_data.py").exists()
                sets.append((name, d, has_gen))
    return sets


def read_active_label_set():
    """Read the current LABEL_SET from active_set.h."""
    if ACTIVE_SET.exists():
        text = ACTIVE_SET.read_text(encoding="utf-8")
        m = re.search(r'#define\s+LABEL_SET\s+(\S+)', text)
        if m:
            return m.group(1)
    return None


def select_label_set(prefs):
    """Interactive label set selection."""
    header("Label Set Selection")

    sets = discover_label_sets()
    if not sets:
        error("No LABEL_SET_* directories found in src/LABELS/")
        return None

    active = read_active_label_set()
    if active:
        info(f"Currently active: {CYAN}{active}{RESET}")

    options = []
    for name, path, has_gen in sets:
        marker = ""
        if not has_gen:
            marker = f" {DIM}(no generate_data.py){RESET}"
        options.append((f"{name}{marker}", name))

    chosen = pick("Select the LABEL SET to build:", options, default=active)
    return chosen


def run_generate_data(label_set_name):
    """Launch generate_data.py in its own console window and wait for it to finish."""
    header(f"Running generate_data.py for {label_set_name}")

    label_dir = LABELS_DIR / f"LABEL_SET_{label_set_name}"
    gen_script = label_dir / "generate_data.py"

    if not gen_script.exists():
        error(f"generate_data.py not found in {label_dir}")
        return False

    info(f"Directory: {label_dir}")
    info(f"Script:    {gen_script}")
    print()
    info("Opening generator in a new window...")
    info("Complete it there, then come back here to continue.")
    print()

    proc = subprocess.Popen(
        [sys.executable, str(gen_script)],
        cwd=str(label_dir),
        creationflags=subprocess.CREATE_NEW_CONSOLE,
    )
    proc.wait()

    new_active = read_active_label_set()
    data_h = label_dir / "DCSBIOSBridgeData.h"

    if new_active == label_set_name and data_h.exists():
        success(f"generate_data.py completed for {label_set_name}")
        info(f"active_set.h -> {CYAN}{label_set_name}{RESET}")
        return True
    elif new_active == label_set_name:
        warn(f"active_set.h updated but DCSBIOSBridgeData.h not found")
        return True
    else:
        error(f"generate_data.py did not produce expected output")
        if new_active:
            error(f"active_set.h shows: {new_active} (expected {label_set_name})")
        return False
