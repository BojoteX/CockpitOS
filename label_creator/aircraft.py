"""
CockpitOS Label Creator — Aircraft JSON management.

Handles discovering and loading aircraft definition JSONs from the
centralized library at _core/aircraft/. Each LABEL_SET references
its aircraft via an aircraft.txt file containing the aircraft name.
"""

import json
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
LABELS_DIR   = Path(__file__).resolve().parent.parent / "src" / "LABELS"
AIRCRAFT_DIR = LABELS_DIR / "_core" / "aircraft"


def discover_aircraft() -> list[tuple[str, Path]]:
    """Scan _core/aircraft/ for aircraft JSON files.

    Returns a sorted list of (display_name, json_path) tuples.
    Display name is the stem of the JSON file (e.g. "FA-18C_hornet").
    """
    if not AIRCRAFT_DIR.is_dir():
        return []

    results = []
    for jf in sorted(AIRCRAFT_DIR.iterdir()):
        if jf.suffix.lower() == ".json" and jf.is_file():
            results.append((jf.stem, jf))

    return results


def load_aircraft_json(name_or_path) -> dict:
    """Load and return the parsed aircraft JSON.

    Accepts either:
      - An aircraft name string (e.g. "FA-18C_hornet") — resolved from _core/aircraft/
      - A Path object — loaded directly
    """
    if isinstance(name_or_path, Path):
        path = name_or_path
    else:
        path = AIRCRAFT_DIR / f"{name_or_path}.json"

    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def read_aircraft_txt(label_set_dir: Path) -> str | None:
    """Read the aircraft name from a label set's aircraft.txt.

    Returns the aircraft name string or None if not found.
    """
    txt = label_set_dir / "aircraft.txt"
    if txt.exists():
        name = txt.read_text(encoding="utf-8").strip()
        if name:
            return name
    return None


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
