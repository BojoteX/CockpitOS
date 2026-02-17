"""
CockpitOS Label Creator — Label Set creation and management.

Handles copying a template label set, validating names, running
reset_data.py and generate_data.py in separate console windows.
"""

import os
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
_LABEL_SET_PREFIX = "LABEL_SET_"


def sanitize_name(raw: str) -> tuple[str, bool]:
    """Clean up user input into a valid label set name.

    - Uppercase everything
    - Strip 'LABEL_SET_' prefix if the user typed it
    - Replace any non-alphanumeric/underscore character with '_'
    - Collapse runs of underscores into one
    - Strip leading/trailing underscores
    - If the name starts with a digit or underscore, prefix with 'X'

    Returns (cleaned_name, was_modified) where was_modified is True if
    the cleaned name differs from the simple raw.strip().upper().
    """
    name = raw.strip().upper()
    original = name

    # Strip LABEL_SET_ prefix if user included it
    if name.startswith(_LABEL_SET_PREFIX):
        name = name[len(_LABEL_SET_PREFIX):]

    # Replace any character that isn't A-Z, 0-9, or _ with underscore
    name = re.sub(r"[^A-Z0-9_]", "_", name)

    # Collapse multiple underscores into one
    name = re.sub(r"_+", "_", name)

    # Strip leading/trailing underscores
    name = name.strip("_")

    # If it starts with a digit, prefix with X
    if name and name[0].isdigit():
        name = "X" + name

    modified = (name != original)
    return name, modified


def validate_name(name: str) -> str | None:
    """Validate a label set name (after sanitization).

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


_TEMPLATE_DIR = CORE_DIR / "template"


def create_label_set(new_name: str) -> Path:
    """Create a new label set directory with the required wrapper scripts.

    Copies the 3 boilerplate wrappers (reset_data.py, generate_data.py,
    display_gen.py) from _core/template/ into a fresh LABEL_SET_{new_name}
    directory.

    Returns the path to the new directory.
    """
    target = LABELS_DIR / f"LABEL_SET_{new_name}"
    target.mkdir()
    for script in ("reset_data.py", "generate_data.py", "display_gen.py"):
        src = _TEMPLATE_DIR / script
        shutil.copy2(src, target / script)
    return target


def _batch_env() -> dict:
    """Return a copy of the current environment with COCKPITOS_BATCH=1.

    Also forces PYTHONIOENCODING=utf-8 so that Unicode characters (like ✓)
    survive when stdout is redirected to a log file on Windows (cp1252).
    """
    env = os.environ.copy()
    env["COCKPITOS_BATCH"] = "1"
    env["PYTHONIOENCODING"] = "utf-8"
    return env


def run_reset_data(label_dir: Path, same_window: bool = False):
    """Run reset_data.py and wait for completion.

    Passes COCKPITOS_BATCH=1 so the script skips interactive confirmations
    and the final pause. The aircraft selector still runs interactively.

    Args:
        label_dir:    path to the label set directory.
        same_window:  if True, run in the current console;
                      if False, open a separate console window.
    """
    script = label_dir / "reset_data.py"
    if not script.exists():
        raise FileNotFoundError(f"reset_data.py not found in {label_dir}")

    if same_window:
        proc = subprocess.Popen(
            [sys.executable, str(script)],
            cwd=str(label_dir),
            env=_batch_env(),
        )
    else:
        proc = subprocess.Popen(
            [sys.executable, str(script)],
            cwd=str(label_dir),
            creationflags=subprocess.CREATE_NEW_CONSOLE,
            env=_batch_env(),
        )
    proc.wait()
    return proc.returncode


def run_generate_data(label_dir: Path, same_window: bool = False):
    """Run generate_data.py and wait for completion.

    Passes COCKPITOS_BATCH=1 so the script skips interactive pauses.
    Requires aircraft.txt and selected_panels.txt to exist beforehand.

    Args:
        label_dir:    path to the label set directory.
        same_window:  if True, run in current process with output redirected
                      to generate.log; if False, open a separate console window.
    """
    script = label_dir / "generate_data.py"
    if not script.exists():
        raise FileNotFoundError(f"generate_data.py not found in {label_dir}")

    if same_window:
        log_path = CORE_DIR / "generate.log"
        with open(log_path, "w", encoding="utf-8") as log_f:
            proc = subprocess.Popen(
                [sys.executable, str(script)],
                cwd=str(label_dir),
                stdout=log_f,
                stderr=subprocess.STDOUT,
                env=_batch_env(),
            )
            proc.wait()
        return proc.returncode
    else:
        proc = subprocess.Popen(
            [sys.executable, str(script)],
            cwd=str(label_dir),
            creationflags=subprocess.CREATE_NEW_CONSOLE,
            env=_batch_env(),
        )
        proc.wait()
        return proc.returncode


def write_aircraft_txt(label_dir: Path, aircraft_name: str):
    """Write aircraft.txt referencing the selected aircraft by name."""
    path = label_dir / "aircraft.txt"
    path.write_text(aircraft_name + "\n", encoding="utf-8")


def remove_legacy_aircraft_jsons(label_dir: Path):
    """Remove any legacy local .json files from the label set root.

    In the old system, aircraft JSONs lived inside each label set.
    Now they're centralized in _core/aircraft/. This cleans up leftovers
    copied from templates that still have old local JSONs.
    """
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


# ---------------------------------------------------------------------------
# LabelSetConfig.h helpers
# ---------------------------------------------------------------------------
def read_label_set_config(label_dir: Path) -> dict:
    """Read editable fields from LabelSetConfig.h.

    Returns dict with:
      "fullname"      — str or None
      "hid_selector"  — bool or None
    """
    cfg = label_dir / "LabelSetConfig.h"
    result = {"fullname": None, "hid_selector": None}
    if not cfg.exists():
        return result
    text = cfg.read_text(encoding="utf-8")
    m = re.search(r'#define\s+LABEL_SET_FULLNAME\s+"([^"]*)"', text)
    if m:
        result["fullname"] = m.group(1)
    m = re.search(r'#define\s+HAS_HID_MODE_SELECTOR\s+(\d)', text)
    if m:
        result["hid_selector"] = m.group(1) == "1"
    return result


def write_label_set_fullname(label_dir: Path, new_name: str):
    """Update LABEL_SET_FULLNAME in LabelSetConfig.h."""
    cfg = label_dir / "LabelSetConfig.h"
    text = cfg.read_text(encoding="utf-8")
    text = re.sub(
        r'(#define\s+LABEL_SET_FULLNAME\s+)"[^"]*"',
        rf'\1"{new_name}"',
        text,
    )
    cfg.write_text(text, encoding="utf-8")


def write_hid_mode_selector(label_dir: Path, enabled: bool):
    """Update HAS_HID_MODE_SELECTOR in LabelSetConfig.h."""
    cfg = label_dir / "LabelSetConfig.h"
    text = cfg.read_text(encoding="utf-8")
    val = "1" if enabled else "0"
    text = re.sub(
        r'(#define\s+HAS_HID_MODE_SELECTOR\s+)\d',
        rf'\g<1>{val}',
        text,
    )
    cfg.write_text(text, encoding="utf-8")
