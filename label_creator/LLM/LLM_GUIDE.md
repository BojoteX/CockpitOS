# CockpitOS Label Creator Tool — LLM Reference Guide

> This file is for LLMs working on this codebase. It maps every module,
> function, constant, data structure, and design decision so you can modify
> or extend the tool without reading every source file first.
>
> **Read this ENTIRE document before making changes.** The architecture is
> deliberately modular — understand the dependency graph and data flow first.

---

## Quick Context

CockpitOS is an ESP32 firmware for DCS World flight simulator cockpit panels.
It uses DCS-BIOS to bridge physical cockpit controls (buttons, switches, LEDs,
displays) to the sim. Each physical panel configuration is called a **Label Set**
— a self-contained folder that defines which aircraft controls are mapped and
how they connect to hardware.

The **Label Creator Tool** is an interactive Python TUI that automates creation
of new Label Sets. Previously this was a tedious, error-prone manual process.
The tool follows the exact same architectural pattern as the existing
**Compiler Tool** (`compiler/`).

**Entry point:** `LabelCreator-START.py` (project root) — adds `label_creator/`
to `sys.path` and calls `label_creator.main()`.

---

## What This Tool Does

The tool provides a persistent menu with these operations:

1. **Create New Label Set** — full multi-step flow:
   - Name the new label set (strict regex validation)
   - Copy an existing label set as a template
   - Select aircraft (from centralized `_core/aircraft/` library)
   - Interactively pick panels via a dual-pane TUI
   - Write `aircraft.txt` and `selected_panels.txt`
   - Run `reset_data.py` + `generate_data.py` in separate console windows
   - Post-generation validation of output files

2. **Browse Existing** — inspect any label set: aircraft, generated files,
   selected panels

3. **Delete Label Set** — double-confirmed permanent deletion

4. **Exit**

After creation, the user has a fully generated Label Set with `InputMapping.h`
and `LEDMapping.h` populated — but with all hardware set to `NONE`. Phase 2
(future) will automate the hardware wiring step.

---

## Implemented Features (Pass 1 Complete)

These features are **fully implemented** and working:

- **Menu loop** — persistent `while True` + `menu_pick()` with
  Create/Browse/Delete/Exit. ESC on main menu redraws (never exits).
- **Single-instance guard** — lock file at `.label_creator.lock` with PID,
  stale-lock detection via `OpenProcess`, `atexit` cleanup, and
  `FindWindowW`/`SetForegroundWindow` to bring existing window to front.
- **Console title** — `SetConsoleTitleW("CockpitOS Label Creator")` for
  window discovery.
- **Status display** — shows label set count, last created set, and last
  aircraft on every menu redraw.
- **Spinner** — `ui.Spinner` during `reset_data.py` and `generate_data.py`
  execution.
- **Post-generation validation** — checks `.last_run` timestamp (not exit
  code) and verifies `InputMapping.h`, `LEDMapping.h`, `DCSBIOSBridgeData.h`,
  `LabelSetConfig.h` exist.
- **Centralized aircraft system** — reads from `_core/aircraft/`, writes
  `aircraft.txt` references (not JSON copies).

---

## Project Root Context

```
CockpitOS/                              <-- Arduino sketch root
+-- LabelCreator-START.py               <-- Entry point (thin launcher)
+-- CockpitOS-START.py                  <-- Compiler tool entry (separate tool)
+-- Config.h                            <-- Firmware configuration
+-- label_creator/                      <-- THIS TOOL
|   +-- label_creator.py                <-- Main orchestrator (like cockpitos.py)
|   +-- ui.py                           <-- Terminal UI toolkit (copied from compiler)
|   +-- aircraft.py                     <-- Aircraft JSON discovery & management
|   +-- label_set.py                    <-- Label set copy / reset / generate
|   +-- panels.py                       <-- Interactive dual-pane panel selector
|   +-- input_editor.py                 <-- InputMapping.h TUI editor
|   +-- led_editor.py                   <-- LEDMapping.h TUI editor
|   +-- display_editor.py              <-- DisplayMapping.cpp TUI editor
|   +-- segment_map_editor.py          <-- {PREFIX}_SegmentMap.h manager
|   +-- creator_prefs.json              <-- User preferences (auto-created at runtime)
|   +-- .label_creator.lock             <-- PID lock file (auto-managed, gitignored)
+-- compiler/                           <-- Compiler tool (separate, pre-existing)
|   +-- cockpitos.py                    <-- Compiler orchestrator (gold standard)
|   +-- ui.py                           <-- SAME ui.py (label_creator has a copy)
|   +-- ...
+-- src/
|   +-- LABELS/                         <-- All label sets live here
|       +-- active_set.h                <-- Currently active label set pointer
|       +-- _core/                      <-- Generator scripts + centralized data
|       |   +-- aircraft/               <-- CENTRALIZED aircraft JSON library
|       |   |   +-- FA-18C_hornet.json
|       |   |   +-- AH-64D.json
|       |   +-- generator_core.py       <-- Main generator (creates .h files)
|       |   +-- reset_core.py           <-- Reset logic (deletes auto-generated files)
|       |   +-- display_gen_core.py     <-- Display generator
|       |   +-- aircraft_selector.py    <-- Standalone TUI aircraft picker
|       |   +-- ...
|       +-- LABEL_SET_MAIN/             <-- Example label set
|       +-- LABEL_SET_TEST_ONLY/        <-- Minimal test template
|       +-- LABEL_SET_*/                <-- 18+ total label sets
+-- lib/
|   +-- CUtils/src/                     <-- Hardware drivers (see Hardware Reference)
+-- Docs/
|   +-- AUTOGENERATOR_CORE_LLM_GUIDE.md <-- MUST READ for generator internals
+-- TODO/
    +-- Label Creator Tool/             <-- This guide
```

---

## Dependency Graph (no circular imports)

```
label_creator.py --> ui.py                 (TUI toolkit, zero domain knowledge)
                 --> aircraft.py           (JSON discovery, loading, categories)
                 --> label_set.py          (copy, validate, reset, generate)
                 --> panels.py             (dual-pane interactive selector)
                 --> input_editor.py       (InputMapping.h TUI editor)
                 --> led_editor.py         (LEDMapping.h TUI editor)
                 --> display_editor.py     (DisplayMapping.cpp TUI editor)
                 --> segment_map_editor.py (SegmentMap.h manager)
```

**Rule:** `ui.py` imports NOTHING from our modules. `panels.py` is self-contained
(has its own ANSI constants, does not import `ui.py` — it implements its own
full-screen rendering). `aircraft.py` and `label_set.py` have zero imports from
each other. Only `label_creator.py` imports from all modules. The four editors
import `ui` for TUI functions but nothing from each other.

**Rule:** External scripts (`reset_data.py`, `generate_data.py`) are ALWAYS run
via `subprocess.Popen` with `CREATE_NEW_CONSOLE` — they open in their own
terminal window. This is deliberate: these scripts have their own interactive
prompts (e.g., reset_core asks "Are you absolutely sure?", reset now includes
interactive aircraft selector).

---

## Module Details

### ui.py (715 lines) — Terminal UI Toolkit

**Origin:** Direct copy of `compiler/ui.py`. Pure presentation, zero domain
knowledge, reusable in any Python TUI project. Windows-only (uses `msvcrt`
for keyboard input, `ctypes` for ANSI enable).

**ANSI Constants:**
`CYAN`, `GREEN`, `YELLOW`, `RED`, `BOLD`, `DIM`, `RESET`, `REV`,
`HIDE_CUR`, `SHOW_CUR`, `ERASE_LN`

**Output Functions:**

| Function | Purpose |
|---|---|
| `_w(text)` | Write raw text to stdout, no newline, flush |
| `cls()` | Clear terminal screen (`cls` on Windows, `clear` on Unix) |
| `cprint(color, text)` | Print with ANSI color, handles UnicodeEncodeError |
| `header(text)` | Clear screen + centered cyan banner with `=` borders |
| `success(text)` | Green `[OK]` prefixed message |
| `warn(text)` | Yellow `[!]` prefixed message |
| `error(text)` | Red `[X]` prefixed message |
| `info(text)` | Indented plain text (7-space indent) |
| `big_success(title, details)` | Large green bordered banner with optional detail lines |
| `big_fail(title, details)` | Large red bordered banner with optional detail lines |

**Interactive Pickers:**

| Function | Signature | Purpose |
|---|---|---|
| `pick()` | `pick(prompt, options, default=None)` | Arrow-key single-select. `options` = `[(label, value), ...]`. Returns selected `value` or `None` (ESC). |
| `menu_pick()` | `menu_pick(items)` | Full-screen styled menu with separators. Items: `("label", "value", "style")` or `("---",)` for separator. Styles: `"action"` (green bold), `"normal"`, `"dim"`. |
| `pick_filterable()` | `pick_filterable(prompt, options, default=None)` | Arrow-key picker with type-to-filter. Shows `N/M matches`. Max 20 visible rows with scroll. |
| `pick_live()` | `pick_live(prompt, initial_options, scan_fn, interval)` | Background-refreshing picker. `scan_fn` called every `interval` seconds. |
| `confirm()` | `confirm(prompt, default_yes=True)` | Y/n prompt. Returns `True`/`False`/`None` (ESC). |
| `text_input()` | `text_input(prompt, default=None, mask=False)` | Single-line input. Returns string, `None` (ESC), or `_RESET_SENTINEL` (Ctrl+D). |
| `toggle_pick()` | `toggle_pick(prompt, items)` | Checkbox toggle picker. `items` = `[(label, key, bool), ...]`. Returns `{key: bool}`. |

**Class: `Spinner`**
Background-threaded animated spinner with elapsed time display.
- Constructor: `Spinner(message="Compiling")`
- Methods: `start()`, `stop()`
- Frames: `|`, `/`, `-`, `\`
- Shows `M:SS` elapsed time

**Keyboard Input (all pickers):**
- Uses `msvcrt.getwch()` for raw key reading (Windows-only)
- `\xe0` / `\x00` prefix = special key (arrows: `H`=Up, `P`=Down, `K`=Left, `M`=Right, `S`=Delete)
- `\r` = Enter, `\x1b` = ESC, `\t` = Tab, `\x08` = Backspace
- All pickers hide cursor (`HIDE_CUR`) during interaction, restore on exit (`SHOW_CUR`)

---

### aircraft.py — Aircraft JSON Management (Centralized System)

Discovers and manages aircraft definition JSON files from the **centralized
library** at `_core/aircraft/`. Each Label Set references its aircraft via a
one-line `aircraft.txt` file containing the aircraft name.

**IMPORTANT:** Aircraft JSONs are NO LONGER copied into each label set directory.
They live exclusively in `_core/aircraft/`. This changed from the old system
where each label set had its own local JSON copy.

**Path Constants:**

| Name | Value |
|---|---|
| `LABELS_DIR` | `CockpitOS/src/LABELS/` (resolved from `aircraft.py` location) |
| `AIRCRAFT_DIR` | `CockpitOS/src/LABELS/_core/aircraft/` |

**Functions:**

| Function | Signature | Purpose |
|---|---|---|
| `discover_aircraft()` | `() -> list[tuple[str, Path]]` | Scans `_core/aircraft/` for `*.json` files. Returns `[(name, path)]` sorted alphabetically. Name is the file stem (e.g. `"FA-18C_hornet"`). |
| `load_aircraft_json()` | `(name_or_path) -> dict` | Loads aircraft JSON. Accepts a name string (resolved from `_core/aircraft/`) or a `Path` object (loaded directly). |
| `read_aircraft_txt()` | `(label_set_dir: Path) -> str \| None` | Reads the aircraft name from a label set's `aircraft.txt`. Returns the name string or `None` if not found. |
| `get_categories()` | `(data: dict) -> list[str]` | Returns sorted list of panel/category names from aircraft JSON (top-level keys). |
| `get_controls_for_category()` | `(data: dict, category: str) -> list[dict]` | Returns list of control dicts for a category, sorted by identifier. |
| `summarize_category()` | `(data: dict, category: str) -> dict` | Returns `{total, inputs, outputs, types}` summary dict. |

**Removed Functions (old system):**
- `_is_valid_panel_json()` — no longer needed (centralized JSONs are trusted)
- `deploy_aircraft_json()` — replaced by `label_set.write_aircraft_txt()`

**How Discovery Works (Current):**
1. Scan `_core/aircraft/` directory for `*.json` files
2. Return sorted list of `(stem, path)` tuples
3. That's it — no scanning label sets, no validation, no deduplication

**Aircraft JSON Structure (DCS-BIOS Control Definitions):**

```json
{
  "Panel Name": {
    "CONTROL_IDENTIFIER": {
      "category": "Panel Name",
      "control_type": "selector",
      "description": "Human-readable",
      "identifier": "CONTROL_IDENTIFIER",
      "inputs": [
        {"interface": "set_state", "max_value": 2},
        {"interface": "fixed_step"},
        {"interface": "action", "argument": "TOGGLE"}
      ],
      "outputs": [
        {
          "address": 29804,
          "mask": 3072,
          "shift_by": 10,
          "max_value": 2,
          "type": "integer"
        }
      ]
    }
  }
}
```

**Control Types (13 types found in FA-18C):**

| Type | Count | Description |
|---|---|---|
| `selector` | 241 | Multi-position switches, buttons |
| `led` | 78 | Indicator lights (read-only) |
| `display` | 60 | Text/numeric displays (read-only) |
| `analog_gauge` | 47 | Continuous analog indicators |
| `limited_dial` | 42 | Rotatable knobs with limits |
| `metadata` | 20 | System state data |
| `action` | 5 | Action-only controls |
| `analog_dial` | 3 | Unlimited rotation dials |
| `fixed_step_dial` | 2 | Step-increment dials |
| `radio` | 2 | Radio controls |
| `toggle_switch` | 1 | Toggle switches |
| `emergency_parking_brake` | 1 | Special type |
| `mission_computer_switch` | 1 | Special type |

**Input Interfaces:** `set_state`, `fixed_step`, `variable_step`, `action`

**Currently Available Aircraft (in `_core/aircraft/`):**
- `FA-18C_hornet` — 74 panels, 500+ controls
- `AH-64D` — Apache helicopter

---

### label_set.py — Label Set Creation & Management

Handles the filesystem operations: copying templates, validating names,
writing `selected_panels.txt` and `aircraft.txt`, running external scripts.

**Path Constants:**

| Name | Value |
|---|---|
| `LABELS_DIR` | `CockpitOS/src/LABELS/` |
| `CORE_DIR` | `CockpitOS/src/LABELS/_core/` |

**Name Validation:**

| Constant | Value |
|---|---|
| `_NAME_RE` | `^[A-Z][A-Z0-9_]*$` — compiled regex |

Rules enforced by `validate_name()`:
1. Minimum 2 characters
2. Only uppercase letters (`A-Z`), digits (`0-9`), and underscore (`_`)
3. Must start with a letter
4. `LABEL_SET_{name}` must not already exist as a directory
5. Returns error message string on failure, `None` on success

**Functions:**

| Function | Signature | Purpose |
|---|---|---|
| `validate_name()` | `(name: str) -> str \| None` | Validate label set name. Returns error message or `None`. |
| `list_label_sets()` | `() -> list[str]` | Returns sorted directory names like `["LABEL_SET_ALR67", "LABEL_SET_MAIN", ...]`. |
| `get_label_set_dir()` | `(name: str) -> Path` | Returns full path for a bare name (adds `LABEL_SET_` prefix). |
| `copy_template()` | `(template_dir: Path, new_name: str) -> Path` | `shutil.copytree` from template to new `LABEL_SET_{new_name}`. Returns new path. |
| `run_reset_data()` | `(label_dir: Path) -> int` | Runs `reset_data.py` in **separate console window** (`CREATE_NEW_CONSOLE`). Blocks until completion. Returns exit code. |
| `run_generate_data()` | `(label_dir: Path) -> int` | Runs `generate_data.py` in **separate console window**. Blocks until completion. Returns exit code. |
| `write_aircraft_txt()` | `(label_dir: Path, aircraft_name: str) -> None` | Writes `aircraft.txt` with the aircraft name (one-line file). |
| `remove_legacy_aircraft_jsons()` | `(label_dir: Path) -> None` | Deletes all `*.json` files in the label set root. Cleans up old local JSONs from templates using the legacy system. |
| `write_selected_panels()` | `(label_dir, selected, all_panels)` | Writes `selected_panels.txt` with header comments, selected panels uncommented, unselected panels commented with `#` prefix. |

**Critical Detail — CREATE_NEW_CONSOLE:**
Both `reset_data.py` and `generate_data.py` are thin wrappers that import from
`_core/reset_core.py` and `_core/generator_core.py` respectively. They have
their own interactive prompts. Running them in a separate console window means:
- Their I/O doesn't collide with our TUI
- User can see their output and interact with confirmations
- Our tool blocks (`proc.wait()`) until they finish

**What reset_core.py Does (updated flow):**
1. Deletes auto-generated files: `DCSBIOSBridgeData.h`, `InputMapping.h`,
   `LEDMapping.h`, `DisplayMapping.cpp`, `DisplayMapping.h`, `CT_Display.cpp`,
   `CT_Display.h`, `LabelSetConfig.h`
2. Deletes `selected_panels.txt` (IMPORTANT: we must write it AFTER reset)
3. Removes legacy local aircraft JSONs
4. Blanks `CustomPins.h` to a comprehensive template
5. **Presents interactive aircraft selector** (using `aircraft_selector.py`)
   if `aircraft.txt` exists, the name is pre-highlighted as the default
6. Writes `aircraft.txt` with user's selection (or confirms the existing one)
7. Preserves: `METADATA/` directory, `reset_data.py`, `generate_data.py`

**What generator_core.py Does (1406 lines):**
1. Loads aircraft JSON via `_core/aircraft/{name}.json` (reads `aircraft.txt`)
2. Merges METADATA JSONs from `METADATA/` subdirectory
3. Reads `selected_panels.txt` to filter panels
4. Builds DCS output table (`DCSBIOSBridgeData.h`)
5. Builds input entries with auto-detection (see InputMapping section)
6. Writes `InputMapping.h` with hash table — all entries default to `source=NONE`
7. Writes `LEDMapping.h` with hash table — all entries default to `deviceType=DEVICE_NONE`
8. Updates `active_set.h`, generates `LabelSetConfig.h` with SHA1-based USB PID
9. Calls display generator and panel-kind generator
10. **Always exits with `sys.exit(1)`** — even on success (historical quirk)
11. Writes `.last_run` timestamp file on success — this is the reliable signal

**Preservation Logic (generator_core.py):**
When regenerating an existing label set, the generator preserves user hardware
wiring edits. It parses existing `InputMapping.h` and `LEDMapping.h` via regex,
extracts entries by label, and merges them into the new output. This means:
- Added panels get new `NONE` entries
- Removed panels lose their entries
- Unchanged panels keep their wiring assignments intact

---

### panels.py (363 lines) — Interactive Dual-Pane Panel Selector

The most complex UI in the tool. Implements a full-screen dual-pane selector
from scratch (does NOT use ui.py pickers — the dual-pane layout requires
custom rendering).

**ANSI Constants:**
Mirrors `ui.py` constants locally (CYAN, GREEN, YELLOW, RED, BOLD, DIM,
RESET, REV, HIDE_CUR, SHOW_CUR, ERASE_LN). This is intentional — `panels.py`
is self-contained for rendering.

**Control Type Icons:**

| DCS-BIOS Type | Icon | Meaning |
|---|---|---|
| `selector`, `toggle_switch` | `SW` | Switch |
| `limited_dial`, `analog_dial`, `fixed_step_dial` | `DL` | Dial |
| `led` | `LED` | LED indicator |
| `analog_gauge` | `GA` | Gauge |
| `display` | `DSP` | Display |
| `metadata` | `META` | Metadata |
| `action` | `ACT` | Action |
| `radio` | `RAD` | Radio |

**Helper Functions:**

| Function | Purpose |
|---|---|
| `_w(text)` | Write raw to stdout (local copy) |
| `_get_terminal_size()` | Returns `(cols, rows)`, fallback `(120, 30)` |
| `_short_type(control_type)` | Maps DCS-BIOS control_type to 2-4 char icon |
| `_summarize(controls)` | Builds compact summary like `"24SW 1DL"` from control list |

**Main Function: `select_panels(all_panels, aircraft_data) -> list[str] | None`**

Parameters:
- `all_panels`: Sorted list of all panel/category names from the aircraft JSON
- `aircraft_data`: The full parsed aircraft JSON dict

Returns:
- `list[str]` of selected panel names (on ESC = done)
- `None` if cancelled (Q key)

**Key Bindings:**

| Key | Context | Action |
|---|---|---|
| Up/Down | Either pane | Navigate within current pane |
| Right Arrow | Left pane | Move highlighted panel to selected (right) |
| Enter | Left pane | Move highlighted panel to selected (right) |
| Left Arrow | Right pane | Remove panel, return to available (left) |
| Delete | Right pane | Remove panel, return to available (left) |
| Enter | Right pane | Show full detail view |
| Tab | Either | Switch focus between left and right panes |
| Space | Either | Show full detail view for highlighted panel |
| Esc | Either | Done — return current selection |
| Q | Either | Cancel — return None |

---

### label_creator.py (574 lines) — Main Orchestrator

The central coordinator. Manages the persistent menu loop, preferences,
single-instance guard, and inter-module communication. Follows the exact
pattern of `compiler/cockpitos.py`.

**Path Constants:**

| Name | Value |
|---|---|
| `SKETCH_DIR` | CockpitOS root (parent of `label_creator/`) |
| `LABELS_DIR` | `CockpitOS/src/LABELS/` |
| `PREFS_FILE` | `label_creator/creator_prefs.json` |
| `_WINDOW_TITLE` | `"CockpitOS Label Creator"` |

**Preferences (`creator_prefs.json`):**

```json
{
  "last_template": "LABEL_SET_TEST_ONLY",
  "last_aircraft": "FA-18C_hornet",
  "last_label_set": "LABEL_SET_MY_PANEL"
}
```

Saved after template selection, aircraft selection, and successful creation.
Used as defaults on next run.

**Functions:**

| Function | Purpose |
|---|---|
| `load_prefs()` | Read JSON prefs file, return `{}` on error |
| `save_prefs(prefs)` | Write JSON prefs file |
| `_bring_existing_to_front()` | Win32 `FindWindowW` + `SetForegroundWindow` |
| `step_name_label_set(prefs)` | Step 1: text input + validation loop |
| `step_select_template(prefs)` | Step 2: `pick_filterable` from existing label sets |
| `step_select_aircraft(prefs)` | Step 3: `pick` from discovered aircraft JSONs. Returns aircraft name `str` or `None` |
| `step_select_panels(aircraft_data)` | Step 4: delegates to `panels.select_panels()` |
| `step_generate(name, new_dir, selected)` | Step 5: summary + confirm + run generate + validate output |
| `do_create(prefs)` | Full create flow (steps 1-5 + template copy + reset) |
| `do_browse(prefs)` | Browse + inspect existing label sets |
| `_show_label_set_info(ls_name)` | Detail view for a single label set |
| `do_delete(prefs)` | Pick + double-confirm + delete |
| `main()` | Entry point: lock, title, menu loop |

**Complete Create Flow (`do_create`):**

```
do_create(prefs)
  |
  +-- Step 1: step_name_label_set()
  |     +-- ui.text_input() -> raw name
  |     +-- .upper() + label_set.validate_name()
  |     +-- ui.confirm("Create LABEL_SET_{name}?")
  |     +-- returns name or None
  |
  +-- Step 2: step_select_template()
  |     +-- label_set.list_label_sets() -> existing dirs
  |     +-- ui.pick_filterable() with default from prefs
  |     +-- returns Path or None
  |
  +-- Save template pref
  |
  +-- Step 3: step_select_aircraft()
  |     +-- aircraft.discover_aircraft() -> [(name, path)]
  |     +-- ui.pick() with default from prefs
  |     +-- returns aircraft name str or None
  |
  +-- Save aircraft pref
  +-- Load aircraft JSON data for panel selection
  |
  +-- Step 4: step_select_panels()
  |     +-- aircraft.get_categories(data) -> panel list
  |     +-- panels.select_panels() -> dual-pane TUI
  |     +-- returns list[str] or None
  |
  +-- Copy + setup:
  |     +-- label_set.copy_template(template, name) -> shutil.copytree
  |     +-- label_set.remove_legacy_aircraft_jsons(new_dir)
  |     +-- label_set.write_aircraft_txt(new_dir, ac_name)    <-- BEFORE reset
  |     +-- label_set.run_reset_data(new_dir)                 <-- separate window
  |     +-- label_set.write_selected_panels(new_dir, ...)     <-- AFTER reset
  |
  +-- Step 5: step_generate()
  |     +-- Show summary (name, location, panel count, list)
  |     +-- ui.confirm("Proceed with generation?")
  |     +-- label_set.run_generate_data(new_dir) with Spinner
  |     +-- Check .last_run timestamp (NOT exit code)
  |     +-- Verify InputMapping.h, LEDMapping.h, DCSBIOSBridgeData.h,
  |     |   LabelSetConfig.h exist
  |     +-- big_fail() if any missing
  |
  +-- big_success() with next-steps info
```

**CRITICAL ORDERING in `do_create`:**
1. `aircraft.txt` must be written **BEFORE** reset — reset_data.py reads it
   to pre-highlight the selected aircraft in its interactive selector.
2. `selected_panels.txt` must be written **AFTER** reset — reset_data.py
   deletes it.

**Main Menu Loop:**

```
main()
  |
  +-- Single-instance guard (lock file + PID check + FindWindowW)
  +-- SetConsoleTitleW("CockpitOS Label Creator")
  +-- Load prefs
  |
  +-- while True:
        +-- Gather status (label set count, last set, last aircraft)
        +-- cls() + banner + status display
        +-- menu_pick([Create, Browse, Delete, ---, Exit])
        |
        +-- ESC -> continue (redraw)
        +-- "exit" -> break
        +-- "create" -> do_create(prefs), reload prefs
        +-- "browse" -> do_browse(prefs)
        +-- "delete" -> do_delete(prefs), reload prefs
```

**ESC / Cancel Handling:**
Every step returns `None` on ESC. The create flow checks and returns `False`.
The main loop shows "Press Enter to continue..." and redraws the menu. ESC on
the main menu just redraws (never exits). If cancellation happens AFTER template
copy, the partially-created label set directory is preserved.

---

### LabelCreator-START.py (27 lines) — Entry Point

Thin launcher at project root. Pattern matches `CockpitOS-START.py` (compiler).

1. Checks `platform.system() == "Windows"` (exits if not)
2. Adds `label_creator/` to `sys.path`
3. Imports and calls `label_creator.main()`
4. Catches `KeyboardInterrupt` for clean exit

---

## Centralized Aircraft System

**This is the current system.** Aircraft JSONs live exclusively in
`src/LABELS/_core/aircraft/`. Each label set references its aircraft via
`aircraft.txt` — a one-line file containing just the aircraft name (e.g.
`FA-18C_hornet`).

### How It Works

1. **Discovery:** `aircraft.discover_aircraft()` scans `_core/aircraft/` for
   `*.json` files. Returns `[(stem, path)]` sorted alphabetically.

2. **Reference:** Each label set has an `aircraft.txt` in its root containing
   the aircraft name (e.g. `"AH-64D\n"`).

3. **Loading:** `aircraft.load_aircraft_json("FA-18C_hornet")` resolves to
   `_core/aircraft/FA-18C_hornet.json` and loads it. Also accepts a `Path`
   for direct loading.

4. **Reading references:** `aircraft.read_aircraft_txt(label_set_dir)` reads
   the `aircraft.txt` file and returns the name string (or `None`).

### Why Centralized?

- Single source of truth for each aircraft's DCS-BIOS definitions
- No duplication across 18+ label sets
- Updates to aircraft JSONs apply to all label sets automatically
- `reset_data.py` includes interactive aircraft selector if the user wants to
  switch aircraft

### Legacy Cleanup

Templates copied from old label sets may contain local `.json` files.
`label_set.remove_legacy_aircraft_jsons()` deletes these before writing
`aircraft.txt`.

### The `_core/aircraft_selector.py` Module

Standalone TUI picker used by `reset_core.py`. Has its own `select_aircraft()`
function with arrow-key navigation. If only one aircraft exists, auto-selects.
If `aircraft.txt` exists, pre-highlights the current selection. The label
creator does NOT use this module — it uses `ui.pick()` instead.

---

## Data Flow Diagram

```
                        _core/aircraft/
                     (centralized JSON library)
                              |
                  +-----------+-----------+
                  |                       |
        +---------v-------+     +--------v---------+
        | discover_aircraft|     | load_aircraft_json|
        | -> name list     |     | -> parsed data    |
        +--------+---------+     +--------+---------+
                 |                         |
        +--------v---------+              |
        | ui.pick()        |              |
        | -> ac_name (str) |              |
        +--------+---------+              |
                 |                         |
                 v                         v
        write_aircraft_txt        get_categories -> panel list
                 |                         |
                 v                         v
        +--------+--------+    +----------+-----------+
        | copy_template   |    | panels.select_panels()|
        | (shutil.copy)   |    | (dual-pane TUI)       |
        +--------+--------+    +----------+------------+
                 |                         |
                 v                         v
        New LABEL_SET_X           selected panel names
                 |                         |
                 v                         |
        run_reset_data                     |
        (separate window)                  |
                 |                         |
                 v                         v
        write_selected_panels    (written to new dir)
                 |
                 v
        run_generate_data
        (separate window)
                 |
                 v
        +-------------------------------+
        | Generated files:              |
        |   InputMapping.h   (all NONE) |
        |   LEDMapping.h  (DEVICE_NONE) |
        |   DCSBIOSBridgeData.h         |
        |   LabelSetConfig.h            |
        |   panels.txt                  |
        |   CustomPins.h (blank)        |
        |   DisplayMapping.cpp/h        |
        |   .last_run (timestamp)       |
        +-------------------------------+
```

---

## Generated File Reference — Complete Struct Details

### InputMapping.h

```cpp
struct InputMapping {
    const char* label;        // DCS-BIOS identifier (e.g., "MASTER_ARM_SW")
    InputSource source;       // See InputSource table below
    int8_t port;              // Pin number, I2C port, or -1 for unassigned
    int8_t bit;               // Bit position within port, or -1
    uint8_t hidId;            // HID report ID (0 = disabled)
    const char* oride_label;  // Override label for auto-generated entries
    int oride_value;          // Override value (-1 = no override, 65535 = analog)
    const char* controlType;  // "set_state", "fixed_step", "variable_step", "action"
    uint8_t group;            // Selector group (0 = none, >0 = exclusive group)
};
```

**InputSource Types:**

| Source | Meaning | Port/Bit Usage |
|---|---|---|
| `GPIO` | Direct GPIO pin | port = pin number (via `PIN()` macro) |
| `PCA_0x20` .. `PCA_0x27` | PCA9555 I2C expander at address | port = 0 or 1 (A/B), bit = 0-7 |
| `HC165` | 74HC165 shift register chain | bit = position in chain (0-63) |
| `TM1637` | TM1637 display module key scan | port = CLK pin, bit = key index |
| `MATRIX` | Key matrix scanning | port = row, bit = col |
| `NONE` | Unassigned (default after generation) | port = -1, bit = -1 |

**Auto-Generated Entry Types (by generator_core.py):**

| Pattern | Detection | Entries Created |
|---|---|---|
| Simple button | max_value=1 + action TOGGLE | 1 entry, controlType="action" |
| Cover gate | identifier contains `_COVER` or `_GUARD` | 1 entry, controlType="set_state" |
| Slash-split selector | Description contains `OFF/ON/AUTO` etc. | N entries, one per position with oride_value |
| Analog control | max_value=65535 | 3 entries: base + `_DEC` + `_INC`, oride_value=65535 for analog |
| Large selector | max_value > 4 | 2 entries: `_INC` + `_DEC` pair |
| Encoder pair | fixed_step interface | 2 entries with oride_label pointing to base, values 0 and 1 |

**Hash Table:**
```cpp
constexpr uint16_t INPUT_TABLE_SIZE = next_prime(count * 2);  // ~50% load factor
InputMapping inputTable[INPUT_TABLE_SIZE] = { ... };
int16_t inputHashTable[INPUT_TABLE_SIZE] = { -1, ... };       // label_hash -> index
```

**TrackedSelectorLabels:**
```cpp
const char* TrackedSelectorLabels[] = { "LABEL1", "LABEL2", ... };
```
Array of labels for controls that need state tracking (selectors with
multiple positions, where the firmware needs to poll DCS-BIOS for current state).

---

### LEDMapping.h

```cpp
enum LEDDeviceType {
    DEVICE_GPIO,
    DEVICE_PCA9555,
    DEVICE_TM1637,
    DEVICE_GN1640T,
    DEVICE_WS2812,
    DEVICE_GAUGE,
    DEVICE_NONE
};

struct LEDMapping {
    const char* label;          // DCS-BIOS identifier
    LEDDeviceType deviceType;   // See table below
    union {
        struct { int8_t pin; } gpioInfo;
        struct { uint16_t index; uint8_t r, g, b; } ws2812Info;
        struct { int8_t clkPin, dioPin; uint8_t grid, seg; } tm1637Info;
        struct { uint8_t addr; uint8_t port, bit; } pcaInfo;
        struct { int8_t clkPin, dioPin; uint8_t grid, seg; } gn1640Info;
        struct { uint8_t index; } gaugeInfo;
    } info;
    bool dimmable;              // PWM capable (GPIO only)
    bool activeLow;             // Inverted logic
};
```

**LEDDeviceType Details:**

| Type | Parameters (union) | Use Case |
|---|---|---|
| `DEVICE_GPIO` | `gpioInfo.pin` | Direct LED on GPIO pin. `dimmable`=true for PWM. |
| `DEVICE_PCA9555` | `pcaInfo.{addr, port, bit}` | LED via I2C expander. addr=0x20-0x27, port=0/1, bit=0-7. |
| `DEVICE_TM1637` | `tm1637Info.{clkPin, dioPin, grid, seg}` | LED segment on TM1637 display module. |
| `DEVICE_GN1640T` | `gn1640Info.{clkPin, dioPin, grid, seg}` | LED segment on GN1640T driver chip. |
| `DEVICE_WS2812` | `ws2812Info.{index, r, g, b}` | Addressable RGB LED on WS2812 strip. |
| `DEVICE_GAUGE` | `gaugeInfo.index` | TFT gauge display index. |
| `DEVICE_NONE` | (none) | Unassigned (default after generation). |

**Hash Table:** Same pattern as InputMapping — `label_hash()` polynomial hash,
`next_prime(count*2)` sizing, linear probing, ~50% load factor.

---

### DisplayMapping.cpp / DisplayMapping.h

```cpp
struct DisplayFieldDefLabel {
    const char* label;           // DCS-BIOS identifier (output label)
    const uint8_t* segMap;       // Pointer to segment map array (physical wiring)
    uint8_t numDigits;           // Number of digit positions
    uint8_t segsPerDigit;        // Segments per digit (typically 8 for 7-seg + DP)
    uint16_t min;                // Minimum raw value from DCS-BIOS
    uint16_t max;                // Maximum raw value from DCS-BIOS
    FieldType fieldType;         // FIELD_INTEGER, FIELD_STRING, FIELD_BITMASK
    uint8_t barCount;            // Number of bar segments (for bar-type displays)
    void* driver;                // Pointer to display driver instance
    DisplayDeviceType deviceType;// DT_TM1637, DT_GN1640T, DT_MAX7219, etc.
    RenderFunc renderFunc;       // Function pointer for custom rendering
    ClearFunc clearFunc;         // Function pointer for clearing the display
    FieldRenderType renderType;  // RENDER_NUMERIC, RENDER_BAR, RENDER_CUSTOM, etc.
};
```

**Enums:**
- `FieldType`: `FIELD_INTEGER`, `FIELD_STRING`, `FIELD_BITMASK`
- `DisplayDeviceType`: `DT_TM1637`, `DT_GN1640T`, `DT_MAX7219`, `DT_NONE`
- `FieldRenderType`: `RENDER_NUMERIC`, `RENDER_BAR`, `RENDER_CUSTOM`, `RENDER_NONE`

The `fieldDefs[]` array is the display equivalent of `inputTable[]` and
`ledTable[]`. Each entry maps a DCS-BIOS output label to a physical display
field with rendering configuration.

---

### Hash Table Algorithm (shared by all mapping files)

```cpp
uint16_t label_hash(const char* label) {
    uint16_t h = 0;
    int len = strlen(label);
    for (int i = len - 1; i >= 0; i--) {
        h = h * 31 + label[i];
    }
    return h;
}
```

- **Reverse-iteration polynomial hash** (iterates string backward)
- Multiplier: 31
- Returns `uint16_t`
- Table size: `next_prime(count * 2)` for ~50% load factor
- Collision resolution: linear probing
- Empty slots: `label == nullptr` (InputMapping/LEDMapping) or sentinel

---

## Hardware Driver Reference

All drivers live in `lib/CUtils/src/`. Understanding these is critical for
Phase 2 (hardware wiring automation).

### TM1637 — 7-Segment Display + Key Scan

- **File:** `TM1637.cpp` / `TM1637.h`
- **Interface:** 2-wire (CLK + DIO), bit-banged
- **Capabilities:** Display digits + read key matrix
- **Used for:** Both display output (segments) AND input (key scanning)
- **In InputMapping:** `source=TM1637`, port=CLK pin, bit=key index
- **In LEDMapping:** `deviceType=DEVICE_TM1637`, info={clkPin, dioPin, grid, seg}
- **In DisplayMapping:** `deviceType=DT_TM1637`, driver pointer

### PCA9555 — 16-bit I2C I/O Expander

- **File:** `PCA9555.cpp` / `PCA9555.h`
- **Interface:** I2C (address 0x20-0x27, configurable via A0-A2)
- **Capabilities:** 16 GPIO pins (2 ports x 8 bits), individually configurable
  as input or output
- **Used for:** Both input buttons AND output LEDs
- **In InputMapping:** `source=PCA_0x20` through `PCA_0x27`, port=0/1, bit=0-7
- **In LEDMapping:** `deviceType=DEVICE_PCA9555`, info={addr, port, bit}
- **Auto-init:** The firmware auto-discovers PCA9555 devices from LEDMapping
  entries and configures their direction registers

### HC165 — 74HC165 Shift Register (Input Only)

- **File:** `HC165.cpp` / `HC165.h`
- **Interface:** SPI-like (Load, Clock, Data pins)
- **Capabilities:** Up to 64 bits (8 daisy-chained chips x 8 bits)
- **Used for:** Input only — many switches on a parallel-to-serial chain
- **In InputMapping:** `source=HC165`, bit=position in chain (0-63)
- **In LEDMapping:** N/A (input only)
- **Config.h:** `HC165_BITS` defines chain length

### WS2812 — Addressable RGB LED Strip

- **File:** `WS2812.cpp` / `WS2812.h`
- **Interface:** Single data pin, driven via ESP32 RMT peripheral
- **Capabilities:** Individually addressable RGB LEDs
- **Used for:** Output only — indicator lights with color control
- **In LEDMapping:** `deviceType=DEVICE_WS2812`, info={index, r, g, b}
- **In InputMapping:** N/A (output only)

### GN1640T — LED Driver

- **File:** `GN1640T.cpp` / `GN1640T.h`
- **Interface:** 2-wire (CLK + DIO), similar to TM1637
- **Capabilities:** LED grid driving
- **In LEDMapping:** `deviceType=DEVICE_GN1640T`, info={clkPin, dioPin, grid, seg}

### GPIO — Direct Pin

- **For inputs:** `source=GPIO`, port=pin number via `PIN()` macro
- **For LEDs:** `deviceType=DEVICE_GPIO`, info={pin}, optional dimmable (PWM)
- **The `PIN()` macro** auto-converts ESP32-S2 pin numbers to ESP32-S3
  equivalents. The wiring tool needs to understand this.

### Gauge — TFT Display

- **In LEDMapping:** `deviceType=DEVICE_GAUGE`, info={index}
- **Config.h:** `ENABLE_TFT_GAUGES` must be set

---

## InputControl.cpp — Unified Polling Engine

**File:** `lib/CUtils/src/InputControl.cpp`

The firmware's main input processing module. Reads ALL InputMapping entries
and dispatches them to the correct hardware driver. Understanding this is
essential for knowing how input sources work at runtime.

**Pre-resolved Tables (built at startup):**
The engine scans `inputTable[]` at boot and builds per-hardware-type tables
for efficient polling:

| Table | Source Type | What It Tracks |
|---|---|---|
| GPIO encoders | GPIO pairs with oride_label | Rotary encoder A/B pins |
| GPIO selectors | GPIO with group > 0 | Multi-position switch positions |
| GPIO momentaries | GPIO with group == 0 | Simple push buttons |
| PCA9555 inputs | PCA_0x20..PCA_0x27 | I2C expander button inputs |
| HC165 inputs | HC165 | Shift register chain inputs |
| TM1637 key scan | TM1637 | Display module key matrix |

**Polling Loop:**
1. Read all HC165 bits in one SPI transaction
2. Read all PCA9555 input ports via I2C
3. Read all TM1637 key states
4. Read all GPIO pins
5. Compare with previous state, fire DCS-BIOS commands on change

---

## The .DISABLED Mechanism

Only one label set can compile at a time in the Arduino build system. The
generator handles this by renaming `.cpp` files in all OTHER label sets to
`.cpp.DISABLED`. When a label set is generated/activated:

1. All `.cpp` files in other `LABEL_SET_*` dirs are renamed to `.cpp.DISABLED`
2. Any `.cpp.DISABLED` files in the CURRENT label set are restored to `.cpp`
3. `active_set.h` is updated with the current label set's include path

This means running `generate_data.py` on one label set will disable all others.

---

## Existing Label Sets (18 as of writing)

| Name | Aircraft | Notes |
|---|---|---|
| `LABEL_SET_ALR67` | FA-18C_hornet | ALR-67 countermeasures |
| `LABEL_SET_ANALOG_CABINPRESS` | FA-18C_hornet | Analog cabin pressure gauge |
| `LABEL_SET_CABIN_PRESSURE_GAUGE` | FA-18C_hornet | TFT cabin pressure |
| `LABEL_SET_CMWS_DISPLAY` | AH-64D | Apache CMWS (only Apache set) |
| `LABEL_SET_CUSTOM_LEFT` | FA-18C_hornet | Custom left panel |
| `LABEL_SET_CUSTOM_RIGHT` | FA-18C_hornet | Custom right panel |
| `LABEL_SET_FRONT_LEFT_PANEL` | FA-18C_hornet | Front left panel |
| `LABEL_SET_IFEI` | FA-18C_hornet | IFEI fuel display (rich example) |
| `LABEL_SET_JESUS_PANEL` | FA-18C_hornet | Developer's test panel |
| `LABEL_SET_KY58` | FA-18C_hornet | KY-58 secure voice |
| `LABEL_SET_LEFT_PANEL_CONTROLLER` | FA-18C_hornet | Left panel controller |
| `LABEL_SET_MAIN` | FA-18C_hornet | Primary reference (41 inputs, 46 LEDs) |
| `LABEL_SET_RIGHT_CIRCUIT_BOARD` | FA-18C_hornet | Right circuit board |
| `LABEL_SET_RIGHT_PANEL_CONTROLLER` | FA-18C_hornet | Right panel controller |
| `LABEL_SET_RS485_CENCIO_MANUAL` | FA-18C_hornet | RS485 slave (Cencio) |
| `LABEL_SET_RS485_LOLIN_AUTO` | FA-18C_hornet | RS485 slave (Lolin) |
| `LABEL_SET_RS485_WAVESHARE_MANUAL` | FA-18C_hornet | RS485 slave (Waveshare) |
| `LABEL_SET_TEST_ONLY` | FA-18C_hornet | Minimal test template (default) |

---

## Hardware Wiring Editors (IMPLEMENTED)

All four TUI editors are fully implemented and working:

- **input_editor.py** — InputMapping.h (GPIO, HC165, PCA9555, TM1637, MATRIX sources)
- **led_editor.py** — LEDMapping.h (GPIO, PCA9555, TM1637, GN1640T, WS2812, GAUGE devices)
- **display_editor.py** — DisplayMapping.cpp (render types, field types, min/max, Ctrl+D delete)
- **segment_map_editor.py** — {PREFIX}_SegmentMap.h files (create, edit, Ctrl+D delete, auto-type detection)

All editors preserve user edits across regeneration via label-keyed merge in the generator.

---

## Relationship to the Compiler Tool

| Aspect | Compiler Tool | Label Creator Tool |
|---|---|---|
| Entry point | `CockpitOS-START.py` | `LabelCreator-START.py` |
| Directory | `compiler/` | `label_creator/` |
| Orchestrator | `cockpitos.py` | `label_creator.py` |
| UI toolkit | `ui.py` (original) | `ui.py` (copy) |
| Prefs file | `compiler_prefs.json` | `creator_prefs.json` |
| Domain | Configure + compile firmware | Create + configure label sets |
| External calls | `arduino-cli` | `reset_data.py`, `generate_data.py` |
| Console style | External processes in new windows | Same pattern |
| Menu structure | Menu loop (persistent) | Menu loop (persistent) |
| Single-instance | `.cockpitos.lock` + FindWindowW | `.label_creator.lock` + FindWindowW |
| Status display | Board, transport, role, debug | Set count, last set, aircraft |

Both tools share the same `ui.py` toolkit. If `ui.py` is updated in one tool,
the change should be propagated to the other.

---

## How to Extend

### Add a new step to the create flow
1. Create a new `step_xxx()` function in `label_creator.py`
2. Add it to the sequence in `do_create()` after the appropriate step
3. Handle `None` returns (ESC/cancel) with graceful exit

### Add a new menu action
1. Add an entry to the `menu_pick()` call in `main()`
2. Create a `do_xxx(prefs)` function
3. Wire it into the `elif` chain in the main loop

### Add a new module
1. Create `label_creator/new_module.py`
2. Import from `ui` if it needs TUI functions
3. Import in `label_creator.py` to wire into the flow
4. Never create circular imports — `ui.py` imports nothing from our modules

### Modify the panel selector
1. Edit `panels.py` — `select_panels()` for behavior, `_draw()` for layout
2. The draw function does full-screen clear + redraw on every input event
3. To add a new key binding: add an `elif` in the main input loop

### Add a new aircraft
1. Place the DCS-BIOS JSON in `src/LABELS/_core/aircraft/`
2. `aircraft.discover_aircraft()` will find it automatically on next run

### Change name validation rules
1. Edit `_NAME_RE` in `label_set.py` for the regex
2. Edit `validate_name()` for additional checks

---

## Testing & Running

**To run:**
```
cd CockpitOS
python LabelCreator-START.py
```

**Requirements:**
- Windows (uses `msvcrt` for keyboard input)
- Python 3.10+ (uses `X | Y` union type syntax)
- No external dependencies (stdlib only)

**Module-level testing:**
```python
cd label_creator
python -c "import aircraft; print(aircraft.discover_aircraft())"
python -c "import label_set; print(label_set.validate_name('MY_PANEL'))"
python -c "import label_set; print(label_set.list_label_sets())"
```

---

## Known Limitations & Edge Cases

1. **Windows-only**: `msvcrt` keyboard input and `CREATE_NEW_CONSOLE` are
   Windows-specific. Cross-platform would require `curses` or `blessed`.

2. **No undo for template copy**: If the user cancels after template copy,
   the new directory remains. This is intentional (user is told they can
   finish manually), but could offer a cleanup option.

3. **panels.py duplicates ANSI constants**: Rather than importing from `ui.py`,
   it has its own copies. This is intentional for self-containment but means
   color changes must be synced manually.

4. **Full-screen redraw on every keypress**: The panel selector clears and
   redraws the entire screen on every input. Simple and reliable but may
   flicker on slow terminals.

5. **generator_core.py always exits with code 1**: Even on success. The
   label creator uses `.last_run` timestamp instead of exit code to detect
   success. Any future code must NEVER rely on exit code from generate.

6. **reset_data.py has interactive aircraft selector**: When our tool runs
   reset, it opens in a separate window where the user sees the aircraft
   selector. We pre-write `aircraft.txt` so our selection is pre-highlighted,
   but the user could theoretically change it in the reset window.

---

## Lessons From the Compiler Tool (MUST READ)

### Architecture Principles

1. **Never hard-code values that should come from the source of truth.** The
   compiler had a `COCKPITOS_DEFAULTS` dict that caused real hardware failures.
   Replaced with dynamic resolution from board definitions.

2. **Single-instance enforcement.** Lock file + `FindWindowW` +
   `SetForegroundWindow` + PID stale check + `atexit` cleanup. Already
   implemented in the label creator.

3. **Console window title.** Required for `FindWindowW` to locate the window.
   Already implemented.

4. **`import ctypes` at module top level.** Never inside a try/except block —
   it shadows the top-level import and causes `UnboundLocalError`. This was a
   real bug in the compiler tool.

### Code Quality Standards

1. **Warning-free code** — zero linter warnings
2. **No external dependencies** — stdlib only, no pip installs
3. **Modular design** — one module per concern, no circular imports
4. **Defensive validation** — validate inputs, check file existence, verify
   generation output

---

## Config.h Tracked Defines (Relevant to Label Creator)

The compiler tool tracks these defines in `Config.h` that are relevant to
label set functionality:

- `ENABLE_PCA9555` — I2C expander support
- `ENABLE_TFT_GAUGES` — TFT display support
- `HC165_BITS` — Shift register chain length
- `USE_WIRE_FOR_I2C` — Wire vs IDF I2C driver
- `USE_DCSBIOS_USB` / `USE_DCSBIOS_WIFI` / `USE_DCSBIOS_SERIAL` — Transport
- `RS485_SLAVE_ENABLED` / `RS485_MASTER_ENABLED` — RS485 mode

The label creator should be aware of these when suggesting hardware
configurations in Phase 2, but should NEVER modify Config.h directly.

---

## Firmware Build Pipeline Context

```
Label Creator                    Compiler Tool
--------------                   --------------
aircraft.txt ---------+
                      +---> generate_data.py ---> InputMapping.h
selected_panels.txt --+                     ---> LEDMapping.h
                                            ---> DCSBIOSBridgeData.h
_core/aircraft/*.json                       ---> LabelSetConfig.h
                                            ---> active_set.h
                                            ---> DisplayMapping.cpp/h
                                                       |
                                                       v
                                             Compiler Tool builds
                                             firmware with these
                                             files included
```

---

## Essential External References

- **`Docs/AUTOGENERATOR_CORE_LLM_GUIDE.md`** — MUST READ for deep understanding
  of generator_core.py, reset_core.py, display_gen_core.py, preservation logic,
  hash tables, .DISABLED mechanism, and centralized aircraft system.

- **`src/LABELS/_core/aircraft_selector.py`** — standalone aircraft picker used
  by reset_core.py. Good reference for how the reset flow works interactively.

- **`src/LABELS/LABEL_SET_IFEI/`** — richest example label set with inputs
  (GPIO, TM1637 key scan), LEDs (TM1637 segments, GPIO), and displays
  (TM1637 digit fields). Study this for understanding real-world wiring.

- **`lib/CUtils/src/InputControl.cpp`** — unified polling engine that reads all
  InputMapping entries. Shows how each source type is actually consumed at
  runtime.
