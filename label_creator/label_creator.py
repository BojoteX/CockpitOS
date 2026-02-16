"""
CockpitOS Label Creator — Main orchestrator.

Interactive TUI tool for creating new Label Sets.
Flow:
  1. Name the new label set (strict regex validation)
  2. Select template to copy from
  3. Select aircraft (JSON)
  4. Interactive panel selection (dual-pane)
  5. Write selected_panels.txt
  6. Run reset_data.py + generate_data.py

Follows the same architectural pattern as the compiler tool.
"""

import json
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Module imports
# ---------------------------------------------------------------------------
import ui
import aircraft
import label_set
import panels as panels_mod

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SKETCH_DIR = Path(__file__).resolve().parent.parent
LABELS_DIR = SKETCH_DIR / "src" / "LABELS"
PREFS_FILE = Path(__file__).resolve().parent / "creator_prefs.json"

# ---------------------------------------------------------------------------
# Preferences
# ---------------------------------------------------------------------------
def load_prefs():
    if PREFS_FILE.exists():
        try:
            return json.loads(PREFS_FILE.read_text(encoding="utf-8"))
        except Exception:
            pass
    return {}


def save_prefs(prefs):
    PREFS_FILE.parent.mkdir(parents=True, exist_ok=True)
    PREFS_FILE.write_text(json.dumps(prefs, indent=2), encoding="utf-8")

# ---------------------------------------------------------------------------
# ASCII art banner
# ---------------------------------------------------------------------------
_BANNER = r"""
   ____            _          _ _    ___  ____
  / ___|___   ___| | ___ __ (_) |_ / _ \/ ___|
 | |   / _ \ / __| |/ / '_ \| | __| | | \___ \
 | |__| (_) | (__|   <| |_) | | |_| |_| |___) |
  \____\___/ \___|_|\_\ .__/|_|\__|\___/|____/
                       |_|
        Label Set Creator
"""

# ---------------------------------------------------------------------------
# Step 1: Name the label set
# ---------------------------------------------------------------------------
def step_name_label_set(prefs) -> str | None:
    """Ask user for the new label set name. Returns name or None on cancel."""
    ui.header("Step 1: Name Your Label Set")
    print()
    ui.info("Rules: UPPERCASE letters, digits, underscore only.")
    ui.info("Must start with a letter. Minimum 2 characters.")
    ui.info("The prefix 'LABEL_SET_' is added automatically.")
    print()

    while True:
        raw = ui.text_input("Label set name")
        if raw is None:
            return None     # ESC pressed

        name = raw.strip().upper()
        err = label_set.validate_name(name)
        if err:
            ui.error(err)
            continue

        # Confirm
        full_name = f"LABEL_SET_{name}"
        ok = ui.confirm(f"Create {full_name}?")
        if ok is None:
            return None
        if ok:
            return name
        # User said no — loop again


# ---------------------------------------------------------------------------
# Step 2: Select template
# ---------------------------------------------------------------------------
def step_select_template(prefs) -> Path | None:
    """Let user pick which existing label set to use as template."""
    ui.header("Step 2: Select Template")
    print()
    ui.info("Choose an existing label set to copy as your starting point.")
    ui.info("The template provides reset_data.py, generate_data.py, METADATA, etc.")
    print()

    existing = label_set.list_label_sets()
    if not existing:
        ui.error("No existing label sets found!")
        return None

    # Build options, putting the preferred template first
    default_template = prefs.get("last_template", "LABEL_SET_TEST_ONLY")
    options = [(name, LABELS_DIR / name) for name in existing]

    choice = ui.pick_filterable(
        "Select template:",
        options,
        default=LABELS_DIR / default_template if default_template in existing else None,
    )
    return choice


# ---------------------------------------------------------------------------
# Step 3: Select aircraft
# ---------------------------------------------------------------------------
def step_select_aircraft(prefs) -> tuple[str, Path] | None:
    """Let user pick which aircraft JSON to use."""
    ui.header("Step 3: Select Aircraft")
    print()
    ui.info("Choose the aircraft for this label set.")
    ui.info("The aircraft JSON defines all available panels and controls.")
    print()

    ac_list = aircraft.discover_aircraft()
    if not ac_list:
        ui.error("No aircraft JSONs found in any label set!")
        return None

    default_ac = prefs.get("last_aircraft")
    options = [(name, (name, path)) for name, path in ac_list]

    choice = ui.pick(
        "Select aircraft:",
        options,
        default=None,
    )
    return choice


# ---------------------------------------------------------------------------
# Step 4: Panel selection
# ---------------------------------------------------------------------------
def step_select_panels(aircraft_data: dict) -> list[str] | None:
    """Interactive dual-pane panel selection."""
    all_panels = aircraft.get_categories(aircraft_data)
    return panels_mod.select_panels(all_panels, aircraft_data)


# ---------------------------------------------------------------------------
# Step 5: Confirm and generate
# ---------------------------------------------------------------------------
def step_confirm_and_generate(name: str, new_dir: Path, selected: list[str]) -> bool:
    """Show summary and run generation."""
    ui.header("Step 5: Confirm & Generate")
    print()
    ui.info(f"Label Set:  LABEL_SET_{name}")
    ui.info(f"Location:   {new_dir}")
    ui.info(f"Panels:     {len(selected)} selected")
    print()
    for p in selected:
        ui.info(f"  + {p}")
    print()

    ok = ui.confirm("Proceed with generation?")
    if not ok:
        return False

    # Run reset_data.py
    print()
    ui.info("Running reset_data.py...")
    try:
        rc = label_set.run_reset_data(new_dir)
        if rc == 0:
            ui.success("reset_data.py completed.")
        else:
            ui.warn(f"reset_data.py exited with code {rc}")
    except FileNotFoundError as e:
        ui.error(str(e))
        return False

    # Run generate_data.py
    print()
    ui.info("Running generate_data.py...")
    try:
        rc = label_set.run_generate_data(new_dir)
        if rc == 0:
            ui.success("generate_data.py completed.")
        else:
            ui.warn(f"generate_data.py exited with code {rc}")
    except FileNotFoundError as e:
        ui.error(str(e))
        return False

    return True


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    prefs = load_prefs()

    ui.cls()
    ui.cprint(ui.CYAN + ui.BOLD, _BANNER)
    print()

    # ---- Step 1: Name ----
    name = step_name_label_set(prefs)
    if not name:
        ui.warn("Cancelled.")
        return

    # ---- Step 2: Template ----
    template_dir = step_select_template(prefs)
    if not template_dir:
        ui.warn("Cancelled.")
        return

    prefs["last_template"] = template_dir.name
    save_prefs(prefs)

    # ---- Step 3: Aircraft ----
    ac_choice = step_select_aircraft(prefs)
    if not ac_choice:
        ui.warn("Cancelled.")
        return

    ac_name, ac_path = ac_choice
    prefs["last_aircraft"] = ac_name
    save_prefs(prefs)

    # ---- Copy template and set up aircraft JSON ----
    ui.header("Setting Up Label Set...")
    print()
    ui.info(f"Copying template from {template_dir.name}...")

    new_dir = label_set.copy_template(template_dir, name)
    ui.success(f"Created LABEL_SET_{name}")

    # Remove old aircraft JSON and deploy the selected one
    label_set.remove_existing_aircraft_jsons(new_dir)
    aircraft.deploy_aircraft_json(ac_path, new_dir)
    ui.success(f"Deployed {ac_path.name}")

    # Load the aircraft data for panel selection
    aircraft_data = aircraft.load_aircraft_json(ac_path)

    # ---- Step 4: Panel selection ----
    selected = step_select_panels(aircraft_data)
    if selected is None:
        ui.warn("Panel selection cancelled.")
        ui.info(f"Your label set at LABEL_SET_{name} still exists — you can finish manually.")
        return

    if not selected:
        ui.warn("No panels selected!")
        ok = ui.confirm("Continue with empty selection?", default_yes=False)
        if not ok:
            return

    # Write selected_panels.txt
    all_panels = aircraft.get_categories(aircraft_data)
    label_set.write_selected_panels(new_dir, selected, all_panels)
    ui.success(f"Wrote selected_panels.txt with {len(selected)} panels")

    # ---- Step 5: Confirm & generate ----
    ok = step_confirm_and_generate(name, new_dir, selected)
    if ok:
        print()
        ui.big_success(
            f"LABEL_SET_{name} CREATED!",
            [
                f"Location: {new_dir}",
                f"Aircraft: {ac_name}",
                f"Panels:   {len(selected)}",
                "",
                "Next steps:",
                "  - Edit InputMapping.h to assign hardware (GPIO, PCA, etc.)",
                "  - Edit LEDMapping.h to assign LED devices",
                "  - Edit CustomPins.h for pin definitions",
                "  - Compile with the CockpitOS compiler tool!",
            ]
        )
    else:
        ui.warn("Generation was not completed.")
        ui.info(f"You can manually run reset_data.py and generate_data.py in LABEL_SET_{name}")

    print()
    input("  Press Enter to exit...")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n  Interrupted.")
        sys.exit(0)
