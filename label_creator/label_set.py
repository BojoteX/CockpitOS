"""
CockpitOS Label Creator — Label Set creation and management.

Handles copying a template label set, validating names, running
reset_data.py and generate_data.py in separate console windows.
"""

import re
import subprocess
import sys
import shutil
from pathlib import Path

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
LABELS_DIR = Path(__file__).resolve().parent.parent / "src" / "LABELS"
CORE_DIR   = LABELS_DIR / "_core"

# ---------------------------------------------------------------------------
# Name validation
# ---------------------------------------------------------------------------
_NAME_RE = re.compile(r"^[A-Z][A-Z0-9_]*$")


def validate_name(name: str) -> str | None:
    """Validate a label set name.

    Rules:
      - Only uppercase letters, digits, underscore
      - Must start with a letter
      - At least 2 characters
      - Cannot already exist

    Returns an error message or None if valid.
    """
    if len(name) < 2:
        return "Name must be at least 2 characters."
    if not _NAME_RE.match(name):
        return "Only UPPERCASE letters, digits, and underscores allowed. Must start with a letter."
    target = LABELS_DIR / f"LABEL_SET_{name}"
    if target.exists():
        return f"LABEL_SET_{name} already exists."
    return None


def list_label_sets() -> list[str]:
    """Return sorted list of existing label set directory names."""
    results = []
    for d in sorted(LABELS_DIR.iterdir()):
        if d.is_dir() and d.name.startswith("LABEL_SET_"):
            results.append(d.name)
    return results


def get_label_set_dir(name: str) -> Path:
    """Return the full path for a label set name (without LABEL_SET_ prefix)."""
    return LABELS_DIR / f"LABEL_SET_{name}"


def copy_template(template_dir: Path, new_name: str) -> Path:
    """Copy *template_dir* to a new label set directory.

    Returns the path to the new directory.
    """
    target = LABELS_DIR / f"LABEL_SET_{new_name}"
    shutil.copytree(template_dir, target)
    return target


def run_reset_data(label_dir: Path):
    """Run reset_data.py in a separate console window and wait for completion."""
    script = label_dir / "reset_data.py"
    if not script.exists():
        raise FileNotFoundError(f"reset_data.py not found in {label_dir}")

    proc = subprocess.Popen(
        [sys.executable, str(script)],
        cwd=str(label_dir),
        creationflags=subprocess.CREATE_NEW_CONSOLE,
    )
    proc.wait()
    return proc.returncode


def run_generate_data(label_dir: Path):
    """Run generate_data.py in a separate console window and wait for completion."""
    script = label_dir / "generate_data.py"
    if not script.exists():
        raise FileNotFoundError(f"generate_data.py not found in {label_dir}")

    proc = subprocess.Popen(
        [sys.executable, str(script)],
        cwd=str(label_dir),
        creationflags=subprocess.CREATE_NEW_CONSOLE,
    )
    proc.wait()
    return proc.returncode


def remove_existing_aircraft_jsons(label_dir: Path):
    """Remove any .json files in the label set root (not METADATA)."""
    for jf in label_dir.glob("*.json"):
        jf.unlink()


def write_selected_panels(label_dir: Path, selected: list[str], all_panels: list[str]):
    """Write selected_panels.txt with selected panels uncommented, rest commented."""
    lines = [
        "# selected_panels.txt - List panels to include, one per line.",
        "# To enable a panel, remove the leading '#'. Only panels listed in panels.txt are valid.",
        "# Example:",
        "# Master Arm Panel",
        "# Caution Light Panel",
        "#",
        "# To update the list of available panels, just re-run this script.",
        "",
    ]
    for panel in all_panels:
        if panel in selected:
            lines.append(panel)
        else:
            lines.append(f"# {panel}")

    path = label_dir / "selected_panels.txt"
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
