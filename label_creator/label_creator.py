"""
CockpitOS Label Creator — Main orchestrator.

Interactive TUI tool for creating new Label Sets.
Create flow:
  1. Name the new label set (sanitized automatically)
  2. Create directory + run reset_data.py (user picks aircraft in its window)
  3. Interactive panel selection (dual-pane)
  4. Run generate_data.py (batch mode, no pauses)

Also supports Browse and Delete operations via the main menu.
Follows the same architectural pattern as the compiler tool.
"""

import os
import re
import sys
import json
import time
import shutil
import ctypes
import subprocess
from pathlib import Path

# ---------------------------------------------------------------------------
# Module imports
# ---------------------------------------------------------------------------
import ui
import aircraft
import label_set
import panels as panels_mod
import input_editor
import led_editor
import display_editor
import segment_map_editor
import latched_editor
import covergate_editor
import custompins_editor
import custom_editor

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SKETCH_DIR = Path(__file__).resolve().parent.parent
LABELS_DIR = SKETCH_DIR / "src" / "LABELS"
ACTIVE_SET = LABELS_DIR / "active_set.h"
PREFS_FILE = Path(__file__).resolve().parent / "creator_prefs.json"


# ---------------------------------------------------------------------------
# Tool switcher — launch sibling CockpitOS tools
# ---------------------------------------------------------------------------
_lock_path = None          # set in main(), cleaned up before exec

def _launch_tool(script_name):
    """Replace the current process with a sibling CockpitOS tool.

    Uses os.execl so there is no nesting — the old tool is gone,
    the new one takes over the same console window.
    """
    script = SKETCH_DIR / script_name
    if not script.exists():
        ui.error(f"{script_name} not found.")
        return
    # Clean up our lock file before we disappear
    if _lock_path:
        _lock_path.unlink(missing_ok=True)
    ui.cls()
    os.execl(sys.executable, sys.executable, str(script))


def _read_active_label_set() -> str | None:
    """Read the last successfully generated label set from active_set.h."""
    if ACTIVE_SET.exists():
        text = ACTIVE_SET.read_text(encoding="utf-8")
        m = re.search(r'#define\s+LABEL_SET\s+(\S+)', text)
        if m:
            return m.group(1)
    return None


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
       / ___|___   ___ | | ___ __ (_) |_ / _ \/ ___|
      | |   / _ \ / __|| |/ / '_ \| | __| | | \___ \
      | |__| (_) | (__ |   <| |_) | | |_| |_| |___) |
       \____\___/ \___|_|\_\ .__/|_|\__|\___/|____/
                            |_|
              Label Set Creator
"""

_WINDOW_TITLE = "CockpitOS Label Creator"


# ---------------------------------------------------------------------------
# Single-instance guard helpers
# ---------------------------------------------------------------------------
def _bring_existing_to_front():
    """Find an existing Label Creator window and bring it to the foreground."""
    user32 = ctypes.windll.user32
    hwnd = user32.FindWindowW(None, _WINDOW_TITLE)
    if hwnd:
        SW_RESTORE = 9
        user32.ShowWindow(hwnd, SW_RESTORE)
        user32.SetForegroundWindow(hwnd)
        return True
    return False


# ---------------------------------------------------------------------------
# Step 1: Name the label set
# ---------------------------------------------------------------------------
def step_name_label_set(prefs) -> str | None:
    """Ask user for the new label set name. Returns name or None on cancel."""
    ui.header("Step 1: Name Your Label Set")
    print()
    ui.info("The prefix 'LABEL_SET_' is added automatically.")
    ui.info("Minimum 2 characters. Spaces and special characters are")
    ui.info("converted to underscores automatically.")
    print()

    while True:
        raw = ui.text_input("Label set name")
        if raw is None:
            return None     # ESC pressed

        name, was_modified = label_set.sanitize_name(raw)

        if not name:
            ui.error("Name cannot be empty.")
            continue

        if was_modified:
            ui.warn(f"Adjusted to: {name}")
            ui.info(f"(Your input was modified to meet naming rules)")
            print()

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
# Step 2: Panel selection
# ---------------------------------------------------------------------------
def step_select_panels(aircraft_data: dict,
                       pre_selected: list[str] | None = None,
                       aircraft_name: str = "",
                       label_set_name: str = "",
                       label_set_dir: Path | None = None) -> list[str] | None:
    """Interactive toggle-list panel selection."""
    if label_set_dir is not None:
        aircraft.merge_metadata(aircraft_data, label_set_dir)
    all_panels = aircraft.get_categories(aircraft_data)
    return panels_mod.select_panels(all_panels, aircraft_data, pre_selected,
                                    aircraft_name=aircraft_name,
                                    label_set_name=label_set_name)


# ---------------------------------------------------------------------------
# Step 3: Generate
# ---------------------------------------------------------------------------
def step_generate(name: str, new_dir: Path, same_window: bool = False) -> bool:
    """Run generate_data.py and validate output.

    No confirmation — the user already confirmed by completing prior steps.
    Generator runs in batch mode (COCKPITOS_BATCH=1) so no pauses.

    When same_window is True, output is redirected to generate.log and the
    screen is cleared afterwards to show our own clean result.

    Args:
        same_window: if True, run generator in the current process (output
                     logged to generate.log); if False, open a separate window.
    """
    # Clean screen so generation results are the only thing visible
    ui.header(f"Generating — LABEL_SET_{name}")

    # Run generate_data.py
    # Note: generator_core.py always exits with code 1, even on success.
    # The reliable success signal is the .last_run timestamp file.
    print()
    ui.info("Generating ...")
    gen_start = time.time()
    spinner = ui.Spinner("Generating")
    spinner.start()
    try:
        label_set.run_generate_data(new_dir, same_window=same_window)
    except FileNotFoundError as e:
        spinner.stop()
        ui.error(str(e))
        return False
    spinner.stop()

    # Clear screen and show our own clean results
    ui.header(f"LABEL_SET_{name}")
    print()

    # Check .last_run to verify the generator completed successfully
    last_run = new_dir / ".last_run"
    gen_ok = last_run.exists() and last_run.stat().st_mtime >= gen_start

    # Post-generation validation — verify expected output files
    expected_files = ["InputMapping.h", "LEDMapping.h", "DCSBIOSBridgeData.h",
                      "LabelSetConfig.h"]
    all_found = True
    for fname in expected_files:
        fpath = new_dir / fname
        if fpath.exists():
            size = fpath.stat().st_size
            ui.success(f"{fname} ({size:,} bytes)")
        else:
            ui.error(f"{fname} — NOT generated!")
            all_found = False

    if not gen_ok or not all_found:
        print()
        log_path = LABELS_DIR / "_core" / "generate.log"
        if log_path.exists():
            ui.warn(f"See log: {log_path}")
        ui.big_fail(
            "GENERATION FAILED",
            [
                "One or more expected files were not created.",
            ]
        )
        return False

    return True


# ---------------------------------------------------------------------------
# Create new label set (full flow)
# ---------------------------------------------------------------------------
def do_create(prefs) -> str | None:
    """Run the full create-new-label-set flow.

    Returns the LABEL_SET_xxx name on success, or None on cancel/failure.
    """

    # ---- Step 1: Name ----
    name = step_name_label_set(prefs)
    if not name:
        return None

    # ---- Create directory + run reset (includes aircraft selection) ----
    ui.header("Setting Up Label Set...")
    print()

    new_dir = label_set.create_label_set(name)
    ui.success(f"Created LABEL_SET_{name}")

    # Run reset_data.py — creates CustomPins.h, then presents the
    # interactive aircraft selector in the same window.
    # Runs in batch mode (skips "Are you sure?" and final pause) but the
    # aircraft selector is still fully interactive.
    print()
    try:
        rc = label_set.run_reset_data(new_dir, same_window=True)
    except FileNotFoundError as e:
        ui.error(str(e))
        return None

    # Read the aircraft that the user selected in reset_data.py
    ac_name = aircraft.read_aircraft_txt(new_dir)
    if not ac_name:
        ui.error("No aircraft was selected. Cannot continue.")
        ui.info(f"You can re-run reset_data.py in LABEL_SET_{name}")
        return None

    prefs["last_aircraft"] = ac_name
    save_prefs(prefs)
    ui.success(f"Aircraft: {ac_name}")

    # Load aircraft data for panel selection
    aircraft_data = aircraft.load_aircraft_json(ac_name)

    # ---- Step 2: Panel selection ----
    ls_name = f"LABEL_SET_{name}"

    selected = step_select_panels(aircraft_data, aircraft_name=ac_name,
                                   label_set_name=ls_name,
                                   label_set_dir=new_dir)

    # Write selected panels if any were chosen
    if selected:
        all_panels = aircraft.get_categories(aircraft_data)
        label_set.write_selected_panels(new_dir, selected, all_panels)

    prefs["last_label_set"] = ls_name
    save_prefs(prefs)

    # Auto-generate immediately so the label set is ready when the user
    # lands in the modify screen (InputMapping.h / LEDMapping.h must exist
    # for the full menu to appear).
    if selected:
        step_generate(name, new_dir, same_window=True)

    # Always return the label set name — the directory and aircraft are
    # already set up, so the user should land in the modify screen even
    # if they skipped or cancelled panel selection.
    return ls_name


# ---------------------------------------------------------------------------
# Browse existing label sets
# ---------------------------------------------------------------------------
def _read_selected_panels(ls_dir: Path) -> list[str]:
    """Read uncommented panel names from selected_panels.txt."""
    sp_path = ls_dir / "selected_panels.txt"
    if not sp_path.exists():
        return []
    panels = []
    for line in sp_path.read_text(encoding="utf-8").splitlines():
        stripped = line.strip()
        if stripped and not stripped.startswith("#"):
            panels.append(stripped)
    return panels


def _run_reset(ls_name: str, ls_dir: Path) -> str | None:
    """Run reset_data.py for an existing label set (same window).

    Returns the aircraft name from aircraft.txt after reset completes,
    or None if no aircraft was selected.
    """
    bare = ls_name[len("LABEL_SET_"):] if ls_name.startswith("LABEL_SET_") else ls_name

    print()
    try:
        label_set.run_reset_data(ls_dir, same_window=True)
    except FileNotFoundError as e:
        ui.error(str(e))
        return None

    ac_name = aircraft.read_aircraft_txt(ls_dir)
    if ac_name:
        ui.success(f"Aircraft: {ac_name}")
    return ac_name


def _regenerate(ls_name: str, prefs):
    """Regenerate files for an existing label set (uninterrupted).

    If no aircraft is set, runs reset_data.py first.
    If no panels are selected, routes the user to the panel selector first.
    """
    ls_dir = LABELS_DIR / ls_name

    # Extract the bare name (without LABEL_SET_ prefix)
    bare = ls_name[len("LABEL_SET_"):] if ls_name.startswith("LABEL_SET_") else ls_name

    ui.header(f"Regenerating {ls_name}")
    print()

    # Verify aircraft.txt exists — if not, run reset to let user pick one
    ac_name = aircraft.read_aircraft_txt(ls_dir)
    if not ac_name:
        ui.error("No aircraft assigned to this label set.")
        ui.info("You'll select one in the next screen.")
        # Clear screen so user returns to clean output after external window
        ui.header(f"Resetting — {ls_name}")
        ac_name = _run_reset(ls_name, ls_dir)
        if not ac_name:
            ui.error("No aircraft was selected — cannot regenerate.")
            input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
            return
    else:
        ui.info(f"Aircraft: {ac_name}")

    # Check if panels have been selected — let user review/edit
    existing_panels = _read_selected_panels(ls_dir)
    if not existing_panels:
        print()
        ui.warn("No panels selected yet — opening panel selector...")

    aircraft_data = aircraft.load_aircraft_json(ac_name)
    selected = step_select_panels(aircraft_data, pre_selected=existing_panels,
                                   aircraft_name=ac_name,
                                   label_set_name=ls_name,
                                   label_set_dir=ls_dir)
    if selected is None:
        ui.warn("Panel selection cancelled.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    # Write the (possibly updated) selection — even if empty
    all_panels = aircraft.get_categories(aircraft_data)
    label_set.write_selected_panels(ls_dir, selected, all_panels)

    prefs["last_label_set"] = ls_name
    save_prefs(prefs)

    # Auto-generate after saving panel selection (skip if nothing selected)
    if selected:
        bare = ls_name[len("LABEL_SET_"):] if ls_name.startswith("LABEL_SET_") else ls_name
        step_generate(bare, ls_dir, same_window=True)


def do_browse(prefs):
    """Modify an existing label set (add/delete panels or reset)."""
    while True:
        ui.header("Modify Label Set")
        print()

        existing = label_set.list_label_sets()
        if not existing:
            ui.warn("No label sets found.")
            input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
            return

        ui.info(f"{len(existing)} label sets found")
        print()

        options = [(name, name) for name in existing]
        choice = ui.pick_filterable("Select a label set to inspect:", options)
        if choice is None:
            return

        # Show details — may return an action; loop back to info after
        while True:
            action = _show_label_set_info(choice, prefs)
            if action == "regenerate":
                _regenerate(choice, prefs)
                continue          # back to info page for same label set
            elif action == "reset":
                _reset_existing(choice, prefs)
                continue          # back to info page for same label set
            else:
                break             # "Back" or ESC — return to label set picker


def _reset_existing(ls_name: str, prefs):
    """Reset data for an existing label set (wipe + pick new aircraft).

    Requires explicit confirmation since this wipes generated files.
    """
    ls_dir = LABELS_DIR / ls_name
    bare = ls_name[len("LABEL_SET_"):] if ls_name.startswith("LABEL_SET_") else ls_name

    ui.header(f"Reset Data — {ls_name}")
    print()
    ui.warn("This will wipe all auto-generated files and let you")
    ui.warn("select a new aircraft for this label set.")
    print()

    ok = ui.confirm(f"Reset {bare}?")
    if not ok:
        ui.info("Cancelled.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    # Clear screen so user returns to clean output after external window
    ui.header(f"Resetting — {ls_name}")

    ac_name = _run_reset(ls_name, ls_dir)
    if ac_name:
        prefs["last_aircraft"] = ac_name
        save_prefs(prefs)
    else:
        ui.error("No aircraft was selected.")

    input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")


def _input_mapping_caption(ls_dir: Path) -> str:
    """Return 'X/Y wired' caption for InputMapping.h, or 'not generated'."""
    fpath = ls_dir / "InputMapping.h"
    if not fpath.exists():
        return "not generated"
    wired, total = input_editor.count_wired(str(fpath))
    return f"{wired}/{total} wired"


def _led_mapping_caption(ls_dir: Path) -> str:
    """Return 'X/Y wired' caption for LEDMapping.h, or 'not generated'."""
    fpath = ls_dir / "LEDMapping.h"
    if not fpath.exists():
        return "not generated"
    wired, total = led_editor.count_wired(str(fpath))
    return f"{wired}/{total} wired"


def _display_mapping_caption(ls_dir: Path) -> str:
    """Return 'X/Y configured' caption for DisplayMapping.cpp, or 'not generated'."""
    fpath = ls_dir / "DisplayMapping.cpp"
    if not fpath.exists():
        return "not generated"
    configured, total = display_editor.count_configured(str(fpath))
    if total == 0:
        return "no fields"
    return f"{configured}/{total} configured"


def _segment_map_caption(ls_dir: Path, ac_name: str) -> str:
    """Return 'X of Y devices' caption for the segment maps menu item."""
    return segment_map_editor.segment_map_caption(ls_dir, ac_name)


def _latched_buttons_caption(ls_dir: Path) -> str:
    """Return 'X latched' caption for LatchedButtons.h, or 'not configured'."""
    fpath = ls_dir / "LatchedButtons.h"
    if not fpath.exists():
        return "not configured"
    n = latched_editor.count_latched(str(fpath), str(ls_dir / "InputMapping.h"))
    return f"{n} latched"


def _covergates_caption(ls_dir: Path) -> str:
    """Return 'X gates' caption for CoverGates.h, or 'not configured'."""
    fpath = ls_dir / "CoverGates.h"
    if not fpath.exists():
        return "not configured"
    n = covergate_editor.count_covergates(str(fpath), str(ls_dir / "InputMapping.h"))
    return f"{n} gate{'s' if n != 1 else ''}"


def _custom_pins_caption(ls_dir: Path) -> str:
    """Return 'X pins' caption for CustomPins.h, or 'not configured'."""
    fpath = ls_dir / "CustomPins.h"
    if not fpath.exists():
        return "not configured"
    n = custompins_editor.count_pins(str(fpath))
    return f"{n} pin{'s' if n != 1 else ''}"


def _custom_controls_caption(ls_dir: Path) -> str:
    """Return 'X custom' caption for Custom.json, or 'none'."""
    fpath = ls_dir / "METADATA" / "Custom.json"
    if not fpath.exists():
        return "none"
    n = custom_editor.count_custom(str(fpath))
    return f"{n} custom" if n > 0 else "none"


def _show_label_set_info(ls_name: str, prefs) -> str | None:
    """Display detailed info about a label set.

    If no aircraft is assigned, warns the user and auto-runs reset_data.py
    so they can pick one before returning to the info screen.

    Returns an action string ("regenerate", "reset") or None.
    Loops internally for config edits (device name, HID toggle, custom pins)
    so the info screen refreshes after each change.
    """
    ls_dir = LABELS_DIR / ls_name

    # ── Ensure aircraft is assigned (one-time check) ─────────────────────
    ac_name = aircraft.read_aircraft_txt(ls_dir)
    if not ac_name:
        ui.header(ls_name)
        print()
        ui.error("No aircraft assigned to this label set!")
        ui.info("You'll select one in the next screen.")
        ui.header(f"Resetting — {ls_name}")
        ac_name = _run_reset(ls_name, ls_dir)
        if not ac_name:
            ui.error("No aircraft was selected.")
            input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
            return None
        prefs["last_aircraft"] = ac_name
        save_prefs(prefs)

    # ── Info + action loop (re-draws after config edits) ─────────────────
    last_choice = None      # remembers cursor position across redraws
    while True:
        ac_name = aircraft.read_aircraft_txt(ls_dir) or "(unknown)"
        cfg = label_set.read_label_set_config(ls_dir)
        fullname = cfg["fullname"] or "(not set)"
        hid_on = cfg["hid_selector"]
        hid_label = "Yes, PCB has a switch" if hid_on else "No switch (most users)"
        hid_color = ui.GREEN if hid_on else ui.DIM

        # Detect whether generation has happened
        has_input = (ls_dir / "InputMapping.h").exists()
        has_led   = (ls_dir / "LEDMapping.h").exists()
        is_generated = has_input or has_led

        panels = _read_selected_panels(ls_dir)
        has_panels = bool(panels)

        # ── Header — consistent with other sub-screens ─────────────────
        ui.header(ls_name)
        print()

        # Status lines with emoji — consistent spacing: 5sp + emoji + space
        print(f"     \U0001f5a5\ufe0f {ui.CYAN}{fullname}{ui.RESET}")
        print(f"     \U0001f6e9\ufe0f {ui.CYAN}{ac_name}{ui.RESET}")
        print()

        # ── Menu ───────────────────────────────────────────────────────
        panels_cap = f"({len(panels)} selected)" if panels else ""
        panels_label = ("Select / deselect panels"
                        if has_panels
                        else "Select which aircraft panels to include")

        if is_generated and has_panels:
            input_cap = _input_mapping_caption(ls_dir)
            led_cap   = _led_mapping_caption(ls_dir)
            fullname_cap = fullname if fullname != "(not set)" else "not set"
            hid_cap = f"{hid_color}{hid_label}{ui.RESET}"

            # Display system: check if DisplayMapping.cpp exists with entries
            has_display_entries = False
            display_cap = ""
            if (ls_dir / "DisplayMapping.cpp").exists():
                _dc, _dt = display_editor.count_configured(
                    str(ls_dir / "DisplayMapping.cpp"))
                has_display_entries = _dt > 0
                if has_display_entries:
                    display_cap = _display_mapping_caption(ls_dir)

            segmap_cap = _segment_map_caption(ls_dir, ac_name)
            latched_cap = _latched_buttons_caption(ls_dir)
            covergate_cap = _covergates_caption(ls_dir)
            custom_cap = _custom_controls_caption(ls_dir)
            pins_cap = _custom_pins_caption(ls_dir)

            # Check if this label set is the active (default) one
            active_ls = _read_active_label_set()
            bare = ls_name[len("LABEL_SET_"):] if ls_name.startswith("LABEL_SET_") else ls_name
            is_default = (active_ls == bare)
            default_cap = (f"{ui.GREEN}(active){ui.RESET}"
                           if is_default
                           else f"{ui.DIM}(not active){ui.RESET}")

            menu_items = [
                (panels_label,               "select_panels", "action", panels_cap),
                ("Auto-Generate Label Set",  "set_default",  "warning", default_cap),
                ("",),
                ("---", "LEDs & Controls"),
                ("Edit Inputs",              "edit_input",  "normal", f"({input_cap})"),
                ("Edit Outputs",             "edit_led",    "normal", f"({led_cap})"),
                ("Latched Buttons",          "edit_latched","normal", f"({latched_cap})"),
                ("Cover Gates",              "edit_covergate","normal", f"({covergate_cap})"),
                ("Custom Controls",          "edit_custom",  "normal", f"({custom_cap})"),
                ("",),
                ("---", "Displays"),
                ("Display Configuration",    "segment_maps","normal", f"({segmap_cap})"),
            ]

            if has_display_entries:
                menu_items.append(
                    ("Edit Displays",        "edit_display","normal", f"({display_cap})"),
                )

            menu_items.extend([
                ("",),
                ("---", "Misc Options"),
                ("Device Name",              "devname",     "normal", fullname_cap),
                ("HID Toggle Switch",        "hid",         "normal", hid_cap),
                ("Edit Custom Pins",         "pins",        "normal", f"({pins_cap})"),
                ("",),
                ("RESET LABEL SET",          "reset",       "danger"),
                ("Back",                     "back",        "dim"),
            ])
            choice = ui.menu_pick(menu_items, initial=last_choice)
        else:
            if not has_panels:
                print(f"  {ui.DIM}Start by selecting which aircraft panels")
                print(f"  you want to wire to this device.{ui.RESET}")
                print()

            menu_items = [
                (panels_label,               "select_panels", "action", panels_cap),
                ("",),
                ("RESET LABEL SET",          "reset",       "danger"),
                ("Back",                     "back",        "dim"),
            ]
            choice = ui.menu_pick(menu_items, initial=last_choice)

        last_choice = choice    # remember for cursor restoration

        if choice == "set_default":
            _auto_generate(ls_name, ls_dir, prefs)
            continue
        elif choice == "devname":
            _edit_device_name(ls_dir, fullname)
            continue
        elif choice == "hid":
            label_set.write_hid_mode_selector(ls_dir, not hid_on)
            continue
        elif choice == "pins":
            _edit_custom_pins(ls_dir, ls_name, ac_name)
            continue
        elif choice == "edit_input":
            input_editor.edit_input_mapping(str(ls_dir / "InputMapping.h"),
                                            ls_name, ac_name)
            continue
        elif choice == "edit_led":
            led_editor.edit_led_mapping(str(ls_dir / "LEDMapping.h"),
                                        ls_name, ac_name)
            continue
        elif choice == "edit_latched":
            latched_editor.edit_latched_buttons(
                str(ls_dir / "LatchedButtons.h"),
                str(ls_dir / "InputMapping.h"),
                ls_name, ac_name)
            continue
        elif choice == "edit_covergate":
            covergate_editor.edit_covergates(
                str(ls_dir / "CoverGates.h"),
                str(ls_dir / "InputMapping.h"),
                ls_name, ac_name)
            continue
        elif choice == "edit_custom":
            changed = _edit_custom_controls(ls_dir, ls_name, ac_name)
            if changed:
                _auto_generate(ls_name, ls_dir, prefs)
            continue
        elif choice == "edit_display":
            display_editor.edit_display_mapping(
                str(ls_dir / "DisplayMapping.cpp"), ls_name, ac_name)
            continue
        elif choice == "segment_maps":
            changed = segment_map_editor.manage_segment_maps(
                str(ls_dir), ls_name, ac_name)
            if changed:
                # Auto-regenerate so new maps are picked up immediately
                _auto_generate(ls_name, ls_dir, prefs)
            continue
        elif choice == "select_panels":
            return "regenerate"
        elif choice == "reset":
            return "reset"
        else:
            return None


def _auto_generate(ls_name: str, ls_dir: Path, prefs):
    """Run auto-generate for a label set from the modify screen.

    Validates that aircraft and panels are set, then runs step_generate.
    """
    bare = ls_name[len("LABEL_SET_"):] if ls_name.startswith("LABEL_SET_") else ls_name

    ac_name = aircraft.read_aircraft_txt(ls_dir)
    if not ac_name:
        ui.error("No aircraft assigned. Use RESET LABEL SET first.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    panels = _read_selected_panels(ls_dir)
    if not panels:
        ui.error("No panels selected. Select panels first.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    ok = step_generate(bare, ls_dir, same_window=True)
    if ok:
        prefs["last_label_set"] = ls_name
        save_prefs(prefs)
        print()
        ui.big_success(
            f"{ls_name} GENERATED!",
            [f"Aircraft: {ac_name}"],
        )

    input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")


def _edit_device_name(ls_dir: Path, current: str):
    """Prompt user for a new LABEL_SET_FULLNAME (max 32 chars)."""
    print()
    default = current if current != "(not set)" else None
    new_name = ui.text_input("Device name (max 32 chars)", default=default)
    if new_name is None or new_name == ui._RESET_SENTINEL:
        return
    new_name = new_name.strip()
    if not new_name:
        return
    if len(new_name) > 32:
        new_name = new_name[:32]
        ui.warn(f"Truncated to 32 chars: {new_name}")
    label_set.write_label_set_fullname(ls_dir, new_name)
    ui.success(f"Device name set to: {new_name}")
    time.sleep(0.6)


def _edit_custom_controls(ls_dir: Path, ls_name: str = "", ac_name: str = ""):
    """Launch the Custom.json TUI editor."""
    if not ac_name:
        ac_name = aircraft.read_aircraft_txt(ls_dir) or ""
    if not ac_name:
        ui.warn("No aircraft assigned. Cannot edit custom controls.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return
    aircraft_data = aircraft.load_aircraft_json(ac_name)
    aircraft.merge_metadata(aircraft_data, ls_dir)
    sel_panels = _read_selected_panels(ls_dir)
    custom_json_path = ls_dir / "METADATA" / "Custom.json"
    return custom_editor.edit_custom_controls(
        str(custom_json_path), aircraft_data,
        ls_name, ac_name,
        selected_panels=sel_panels or None)


def _edit_custom_pins(ls_dir: Path, ls_name: str = "", ac_name: str = ""):
    """Launch the intelligent CustomPins.h TUI editor."""
    pins_file = ls_dir / "CustomPins.h"
    if not pins_file.exists():
        ui.warn("CustomPins.h not found in this label set.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return
    custompins_editor.edit_custom_pins(
        str(pins_file),
        str(ls_dir / "InputMapping.h"),
        str(ls_dir / "LEDMapping.h"),
        str(ls_dir / "DisplayMapping.cpp"),
        ls_name, ac_name,
        config_filepath=str(SKETCH_DIR / "Config.h"))


# ---------------------------------------------------------------------------
# Auto-generate a label set
# ---------------------------------------------------------------------------
def do_generate(prefs):
    """Pick a label set and run generate_data.py (same window)."""
    ui.header("Auto-Generate Label Set")
    print()

    existing = label_set.list_label_sets()
    if not existing:
        ui.warn("No label sets found.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    ui.info(f"{len(existing)} label sets found")
    print()

    options = [(name, name) for name in existing]
    choice = ui.pick_filterable("Select a label set to generate:", options)
    if choice is None:
        return

    ls_dir = LABELS_DIR / choice
    bare = choice[len("LABEL_SET_"):] if choice.startswith("LABEL_SET_") else choice

    # Validate: must have aircraft
    ac_name = aircraft.read_aircraft_txt(ls_dir)
    if not ac_name:
        ui.error("No aircraft assigned to this label set.")
        ui.info("Use 'Modify Label Set' to set an aircraft first.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    # Validate: must have selected panels
    panels = _read_selected_panels(ls_dir)
    if not panels:
        ui.error("No panels selected for this label set.")
        ui.info("Use 'Modify Label Set' to select panels first.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    # Show what we're about to generate
    print()
    ui.info(f"Aircraft: {ac_name}")
    panel_list = ", ".join(panels)
    ui.info(f"Panels ({len(panels)}): {ui.YELLOW}{panel_list}{ui.RESET}")
    print()

    ok = step_generate(bare, ls_dir, same_window=True)
    if ok:
        prefs["last_label_set"] = choice
        save_prefs(prefs)
        print()
        ui.big_success(
            f"{choice} GENERATED!",
            [f"Aircraft: {ac_name}"],
        )

    input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")


# ---------------------------------------------------------------------------
# Delete a label set
# ---------------------------------------------------------------------------
def do_delete(prefs):
    """Let user pick and delete an existing label set."""
    ui.header("Delete Label Set")
    print()

    existing = label_set.list_label_sets()
    if not existing:
        ui.warn("No label sets found.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    options = [(name, name) for name in existing]
    choice = ui.pick_filterable("Select label set to delete:", options)
    if choice is None:
        return

    ls_dir = LABELS_DIR / choice
    print()
    ui.warn(f"This will permanently delete {choice}")
    ui.warn(f"Location: {ls_dir}")
    print()

    ok = ui.confirm(f"Delete {choice}?", default_yes=False)
    if not ok:
        ui.info("Cancelled.")
        return

    # Double-confirm for safety
    ok2 = ui.confirm("Are you absolutely sure?", default_yes=False)
    if not ok2:
        ui.info("Cancelled.")
        return

    shutil.rmtree(ls_dir)
    ui.success(f"Deleted {choice}")

    # Clear last_label_set if it was the deleted one
    if prefs.get("last_label_set") == choice:
        prefs.pop("last_label_set", None)
        save_prefs(prefs)

    input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
def main():
    import atexit

    os.system("")  # Enable ANSI on Windows

    # --- Single-instance guard (lock file) ---
    global _lock_path
    lock_path = Path(__file__).parent / ".label_creator.lock"
    _lock_path = lock_path
    try:
        lock_fd = os.open(str(lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, str(os.getpid()).encode())
        os.close(lock_fd)
    except FileExistsError:
        try:
            old_pid = int(lock_path.read_text().strip())
            kernel32 = ctypes.windll.kernel32
            handle = kernel32.OpenProcess(0x1000, False, old_pid)  # PROCESS_QUERY_LIMITED_INFORMATION
            if handle:
                kernel32.CloseHandle(handle)
                _bring_existing_to_front()
                print("\n  Label Creator is already running. Switching to existing window.")
                sys.exit(0)
        except (ValueError, OSError):
            pass
        # Stale lock — reclaim it
        lock_path.unlink(missing_ok=True)
        lock_fd = os.open(str(lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, str(os.getpid()).encode())
        os.close(lock_fd)

    atexit.register(lambda: lock_path.unlink(missing_ok=True))

    # Set window title so other instances can find us
    ctypes.windll.kernel32.SetConsoleTitleW(_WINDOW_TITLE)

    prefs = load_prefs()

    while True:
        # Reload prefs and live state every iteration to avoid stale data
        prefs = load_prefs()
        existing = label_set.list_label_sets()
        active_ls = _read_active_label_set()
        active_ac = None
        ls_exists = False
        if active_ls:
            ls_dir = LABELS_DIR / f"LABEL_SET_{active_ls}"
            ls_exists = ls_dir.exists()
            if ls_exists:
                active_ac = aircraft.read_aircraft_txt(ls_dir)

        ui.cls()
        print()
        for line in _BANNER.strip("\n").splitlines():
            ui.cprint(ui.CYAN + ui.BOLD, line)
        print()

        # Status display — emoji lines like the compiler tool
        print()
        print(f"     \U0001f5c2\ufe0f {ui.CYAN}{len(existing)}{ui.RESET} label sets")
        if active_ls and ls_exists:
            print(f"     \U0001f3f7\ufe0f {ui.CYAN}{active_ls}{ui.RESET}")
            ac_display = active_ac or "(unknown)"
            print(f"     \U0001f6e9\ufe0f {ui.CYAN}{ac_display}{ui.RESET}")
        elif active_ls:
            print(f"     \U0001f3f7\ufe0f {ui.RED}{active_ls} (deleted){ui.RESET}")
        else:
            print(f"     \U0001f3f7\ufe0f {ui.DIM}No label set generated yet{ui.RESET}")
        print()

        choice = ui.menu_pick([
            ("Create New Label Set",      "create",    "action"),
            ("Modify Label Set",          "browse",    "normal"),
            ("Auto-Generate Label Set",   "generate",  "normal"),
            ("Delete Label Set",          "delete",    "normal"),
            ("",),
            ("---", "Switch Tool"),
            ("Compile Tool",              "compiler",  "normal"),
            ("Environment Setup",         "setup",     "normal"),
            ("",),
            ("Exit",                      "exit",      "dim"),
        ])

        if choice is None:         # ESC on main menu — just redraw
            continue

        if choice == "exit":
            ui.cprint(ui.DIM, "  Bye!")
            break

        elif choice == "create":
            created_ls = do_create(prefs)
            prefs = load_prefs()    # Reload in case prefs changed during flow
            if created_ls:
                # Go straight into the modify/detail screen for the new label set
                while True:
                    action = _show_label_set_info(created_ls, prefs)
                    if action == "regenerate":
                        _regenerate(created_ls, prefs)
                        continue
                    elif action == "reset":
                        _reset_existing(created_ls, prefs)
                        continue
                    else:
                        break
            else:
                input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")

        elif choice == "browse":
            do_browse(prefs)

        elif choice == "generate":
            do_generate(prefs)
            prefs = load_prefs()

        elif choice == "compiler":
            _launch_tool("CockpitOS-START.py")

        elif choice == "setup":
            _launch_tool("Setup-START.py")

        elif choice == "delete":
            do_delete(prefs)
            prefs = load_prefs()


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n  Interrupted.")
        sys.exit(0)
