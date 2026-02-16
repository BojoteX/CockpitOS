"""
CockpitOS Label Creator — Aircraft JSON management.

Handles discovering, listing, and deploying aircraft definition JSONs.
Aircraft JSONs live inside existing label sets; this module scans all
LABEL_SET_* directories and deduplicates by filename so the user can
pick which aircraft they want.
"""

import json
import shutil
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
LABELS_DIR = Path(__file__).resolve().parent.parent / "src" / "LABELS"


def _is_valid_panel_json(path: Path) -> bool:
    """Return True if *path* looks like a DCS-BIOS aircraft panel JSON."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
        if not isinstance(data, dict):
            return False
        # Must have at least one category with nested controls that have "outputs"
        for cat_name, cat_val in data.items():
            if isinstance(cat_val, dict):
                for ctrl_name, ctrl_val in cat_val.items():
                    if isinstance(ctrl_val, dict) and "outputs" in ctrl_val:
                        return True
        return False
    except Exception:
        return False


def discover_aircraft() -> list[tuple[str, Path]]:
    """Scan all LABEL_SET_* dirs for aircraft JSONs.

    Returns a deduplicated list of (display_name, source_path) sorted
    alphabetically.  Display name is the stem of the JSON file
    (e.g. "FA-18C_hornet").
    """
    seen: dict[str, Path] = {}

    for ls_dir in sorted(LABELS_DIR.iterdir()):
        if not ls_dir.is_dir() or not ls_dir.name.startswith("LABEL_SET_"):
            continue
        for jf in ls_dir.glob("*.json"):
            # Skip METADATA jsons
            if "METADATA" in str(jf):
                continue
            if jf.stem in seen:
                continue
            if _is_valid_panel_json(jf):
                seen[jf.stem] = jf

    return sorted(seen.items(), key=lambda x: x[0])


def deploy_aircraft_json(source_path: Path, target_dir: Path) -> Path:
    """Copy an aircraft JSON into *target_dir*.

    Returns the path to the copied file.
    """
    dest = target_dir / source_path.name
    shutil.copy2(source_path, dest)
    return dest


def load_aircraft_json(json_path: Path) -> dict:
    """Load and return the parsed aircraft JSON."""
    with open(json_path, "r", encoding="utf-8") as f:
        return json.load(f)


def get_categories(data: dict) -> list[str]:
    """Return sorted list of panel/category names from aircraft JSON."""
    return sorted(data.keys())


def get_controls_for_category(data: dict, category: str) -> list[dict]:
    """Return a list of control dicts for *category*, sorted by identifier."""
    cat = data.get(category, {})
    controls = []
    for ctrl_id, ctrl_val in sorted(cat.items()):
        if isinstance(ctrl_val, dict):
            controls.append(ctrl_val)
    return controls


def summarize_category(data: dict, category: str) -> dict:
    """Return a summary of a category's contents.

    Returns dict with keys:
      total:     int — total controls
      inputs:    int — controls with inputs (buttons, switches, dials, etc.)
      outputs:   int — controls with outputs (LEDs, gauges, displays, etc.)
      types:     dict[str, int] — count per control_type
    """
    controls = get_controls_for_category(data, category)
    types: dict[str, int] = {}
    n_inputs = 0
    n_outputs = 0

    for ctrl in controls:
        ct = ctrl.get("control_type", "unknown")
        types[ct] = types.get(ct, 0) + 1
        if ctrl.get("inputs"):
            n_inputs += 1
        if ctrl.get("outputs"):
            n_outputs += 1

    return {
        "total": len(controls),
        "inputs": n_inputs,
        "outputs": n_outputs,
        "types": types,
    }
