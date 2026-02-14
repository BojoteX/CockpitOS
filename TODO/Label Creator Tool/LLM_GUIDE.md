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

## What This Tool Does (Current Scope — Phase 1)

The tool automates the first half of Label Set creation:

1. **Name** the new label set (strict regex validation)
2. **Copy** an existing label set as a template
3. **Select** the aircraft (DCS-BIOS JSON)
4. **Interactively pick panels** via a dual-pane TUI
5. **Write** `selected_panels.txt`
6. **Run** `reset_data.py` + `generate_data.py` in separate console windows

After this, the user has a fully generated Label Set with `InputMapping.h` and
`LEDMapping.h` populated — but with all hardware set to `NONE`. Phase 2 (future)
will automate the hardware wiring step.

---

## Project Root Context

```
CockpitOS/                              ← Arduino sketch root
├── LabelCreator-START.py               ← Entry point (thin launcher)
├── CockpitOS-START.py                  ← Compiler tool entry (separate tool)
├── Config.h                            ← Firmware configuration
├── label_creator/                      ← THIS TOOL
│   ├── label_creator.py                ← Main orchestrator (like cockpitos.py)
│   ├── ui.py                           ← Terminal UI toolkit (copied from compiler)
│   ├── aircraft.py                     ← Aircraft JSON discovery & management
│   ├── label_set.py                    ← Label set copy / reset / generate
│   ├── panels.py                       ← Interactive dual-pane panel selector
│   └── creator_prefs.json              ← User preferences (auto-created at runtime)
├── compiler/                           ← Compiler tool (separate, pre-existing)
│   ├── cockpitos.py                    ← Compiler orchestrator
│   ├── ui.py                           ← SAME ui.py (label_creator has a copy)
│   └── ...
├── src/
│   └── LABELS/                         ← All label sets live here
│       ├── active_set.h                ← Currently active label set pointer
│       ├── _core/                      ← Generator scripts (shared by all sets)
│       │   ├── generator_core.py       ← Main generator (creates .h files)
│       │   ├── reset_core.py           ← Reset logic (deletes auto-generated files)
│       │   ├── display_gen_core.py     ← Display generator
│       │   └── ...
│       ├── LABEL_SET_MAIN/             ← Example label set
│       ├── LABEL_SET_TEST_ONLY/        ← Minimal test template
│       └── LABEL_SET_*/                ← 18 total label sets (as of writing)
├── Docs/                               ← User documentation
└── TODO/                               ← Task tracking (you are here)
    └── Label Creator Tool/             ← This guide
```

---

## Dependency Graph (no circular imports)

```
label_creator.py ──> ui.py          (TUI toolkit, zero domain knowledge)
                 ──> aircraft.py    (JSON discovery, loading, categories)
                 ──> label_set.py   (copy, validate, reset, generate)
                 ──> panels.py      (dual-pane interactive selector)
```

**Rule:** `ui.py` imports NOTHING from our modules. `panels.py` is self-contained
(has its own ANSI constants, does not import `ui.py` — it implements its own
full-screen rendering). `aircraft.py` and `label_set.py` have zero imports from
each other. Only `label_creator.py` imports from all modules.

**Rule:** External scripts (`reset_data.py`, `generate_data.py`) are ALWAYS run
via `subprocess.Popen` with `CREATE_NEW_CONSOLE` — they open in their own
terminal window. This is deliberate: these scripts have their own interactive
prompts (e.g., reset_core asks "Are you absolutely sure?").

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

### aircraft.py (121 lines) — Aircraft JSON Management

Discovers and manages aircraft definition JSON files. These JSONs are the
DCS-BIOS control definitions that define every switch, LED, gauge, and display
for a given aircraft.

**Path Constants:**

| Name | Value |
|---|---|
| `LABELS_DIR` | `CockpitOS/src/LABELS/` (resolved from `aircraft.py` location) |

**Functions:**

| Function | Signature | Purpose |
|---|---|---|
| `_is_valid_panel_json()` | `(path: Path) -> bool` | Validates JSON structure: must be dict with nested dicts containing `"outputs"` key. Filters out METADATA JSONs and non-aircraft files. |
| `discover_aircraft()` | `() -> list[tuple[str, Path]]` | Scans all `LABEL_SET_*` dirs for `*.json` (excluding METADATA subdir). Deduplicates by filename stem. Returns `[(display_name, source_path)]` sorted alphabetically. |
| `deploy_aircraft_json()` | `(source_path: Path, target_dir: Path) -> Path` | Copies aircraft JSON into target label set directory. Returns destination path. |
| `load_aircraft_json()` | `(json_path: Path) -> dict` | Loads and returns parsed JSON. |
| `get_categories()` | `(data: dict) -> list[str]` | Returns sorted list of top-level keys (= panel/category names). |
| `get_controls_for_category()` | `(data: dict, category: str) -> list[dict]` | Returns list of control dicts for a category, sorted by identifier. |
| `summarize_category()` | `(data: dict, category: str) -> dict` | Returns `{total, inputs, outputs, types}` summary dict. |

**Aircraft JSON Structure (critical for understanding the tool):**

```json
{
  "Panel Name": {                          // top-level = category/panel name
    "CONTROL_IDENTIFIER": {                // unique control ID
      "category": "Panel Name",           // matches parent key
      "control_type": "selector",          // see list below
      "description": "Human-readable",
      "identifier": "CONTROL_IDENTIFIER",
      "inputs": [                          // how to send commands
        {"interface": "set_state", "max_value": 2},
        {"interface": "fixed_step"},
        {"interface": "action", "argument": "TOGGLE"}
      ],
      "outputs": [                         // how to read state
        {
          "address": 29804,
          "mask": 3072,
          "shift_by": 10,
          "max_value": 2,
          "type": "integer"                // or "string"
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

**Currently Discovered Aircraft (2):**
- `FA-18C_hornet` — 74 panels, 500+ controls
- `AH-64D` — Apache helicopter

**How Discovery Works:**
1. Iterate all `LABEL_SET_*` directories under `src/LABELS/`
2. Find `*.json` files in each root (NOT in METADATA subdirectories)
3. Validate each JSON has the expected DCS-BIOS structure
4. Deduplicate by filename stem (first found wins)
5. Return sorted list

---

### label_set.py (127 lines) — Label Set Creation & Management

Handles the filesystem operations: copying templates, validating names,
writing `selected_panels.txt`, and running external scripts.

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
| `remove_existing_aircraft_jsons()` | `(label_dir: Path) -> None` | Deletes all `*.json` files in the label set root directory (so the template's old JSON is removed before deploying the new one). Does NOT touch METADATA. |
| `write_selected_panels()` | `(label_dir, selected, all_panels)` | Writes `selected_panels.txt` with header comments, selected panels uncommented, unselected panels commented with `#` prefix. |

**Critical Detail — CREATE_NEW_CONSOLE:**
Both `reset_data.py` and `generate_data.py` are thin wrappers that import from
`_core/reset_core.py` and `_core/generator_core.py` respectively. They have
their own interactive prompts (reset asks "Are you absolutely sure?"). Running
them in a separate console window means:
- Their I/O doesn't collide with our TUI
- User can see their output and interact with confirmations
- Our tool blocks (`proc.wait()`) until they finish

**What reset_core.py Does:**
1. Deletes auto-generated files: `DCSBIOSBridgeData.h`, `InputMapping.h`,
   `LEDMapping.h`, `DisplayMapping.cpp`, `DisplayMapping.h`, `CT_Display.cpp`,
   `CT_Display.h`, `LabelSetConfig.h`
2. Blanks `CustomPins.h` to a comprehensive template with all pin categories
   commented out
3. Preserves: `METADATA/` directory, `panels.txt`, `selected_panels.txt`,
   the aircraft JSON, `reset_data.py`, `generate_data.py`

**What generator_core.py Does (1406 lines):**
1. Loads aircraft JSON + merges METADATA JSONs
2. Reads `selected_panels.txt` to filter panels
3. Builds DCS output table (`DCSBIOSBridgeData.h`)
4. Builds input selector entries with auto-detection:
   - Simple buttons (max_value=1 + action TOGGLE)
   - Cover gates (identifiers containing `_COVER` or `_GUARD`)
   - Slash-split selectors (e.g., `OFF/ON/AUTO`)
   - Analog controls (max_value=65535 → generates `_DEC` + `_INC` triplet)
   - Large selectors (max_value>4 → generates INC/DEC pair)
5. Writes `InputMapping.h` with hash table (O(1) lookup, ~50% load factor)
   — all entries default to `source=NONE, port=-1, bit=-1`
6. Writes `LEDMapping.h` with hash table
   — all entries default to `deviceType=DEVICE_NONE`
7. Updates `active_set.h`, generates `LabelSetConfig.h` with SHA1-based USB PID
8. Calls `display_gen.py` and `generate_panelkind.py`

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

**Layout:**

```
  ========================================================
    Panel Selector
  ========================================================
  AVAILABLE (62)                    | SELECTED (12)
  --------------------------------- | ---------------------------------
  > AMPCD                           |   Fire Systems
    APU Fire Warning...             | > Master Arm Panel
    Angle of Attack...              |   Caution Light Panel
    ...                             |   ...
  ========================================================
  Preview: AMPCD  (25 controls: 24SW 1DL)
    [  SW] AMPCD_BRT_CTL                  Brightness Control Knob
    [  SW] AMPCD_CONT_SW                  Contrast Control Switch
    [  SW] AMPCD_GAIN_SW                  Gain Control Switch
    [  SW] AMPCD_NIGHT_DAY                Night/Day Switch
    ... and 21 more

  Tab=switch pane  Enter/Right=add  Del/Left=remove  Space=detail  Esc=done  Q=cancel
```

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

**State Management:**
- `pane`: 0 = left (available), 1 = right (selected)
- `idx_left`, `idx_right`: Cursor position in each pane
- `scroll_left`, `scroll_right`: Viewport scroll offset per pane
- `available`: Sorted list of unselected panels (mutated in place)
- `selected`: Ordered list of selected panels (append order preserved)
- `_clamp()`: Called after every mutation to keep indices and scroll in bounds

**When a panel is added (Left → Right):**
1. Remove from `available` list
2. Append to `selected` list (preserves insertion order)
3. Call `_clamp()` to fix indices
4. Full redraw

**When a panel is removed (Right → Left):**
1. Remove from `selected` list
2. Append back to `available` list
3. Re-sort `available` alphabetically
4. Call `_clamp()` to fix indices
5. Full redraw

**Detail View: `_show_detail(aircraft_data, panel_name, cols, rows)`**

Full-screen tabular view of all controls in a panel:

```
  ========================================================
    Panel: Master Arm Panel
  ========================================================

  3 controls  (2SW 1LED)

  Type   Identifier                               Description                                    Inputs
  ------ ---------------------------------------- --------------------------------------------- ----------
  SW     MASTER_ARM_SW                            Master Arm Switch                              fixed_step,set_state
  SW     MC_DISCH                                 Master Caution Discharge                       set_state,action
  LED    MASTER_CAUTION_LT                        MASTER CAUTION (yellow)                        -

  (press any key to go back)
```

Columns: Type icon (from `_short_type`), Identifier, Description (truncated 45 chars),
Input interfaces (comma-separated).

---

### label_creator.py (310 lines) — Main Orchestrator

The central coordinator. Manages the 5-step flow, preferences, and inter-module
communication. Follows the exact pattern of `compiler/cockpitos.py`.

**Path Constants:**

| Name | Value |
|---|---|
| `SKETCH_DIR` | CockpitOS root (parent of `label_creator/`) |
| `LABELS_DIR` | `CockpitOS/src/LABELS/` |
| `PREFS_FILE` | `label_creator/creator_prefs.json` |

**Preferences (`creator_prefs.json`):**

```json
{
  "last_template": "LABEL_SET_TEST_ONLY",
  "last_aircraft": "FA-18C_hornet"
}
```

Saved after template selection and aircraft selection. Used as defaults on
next run.

**Functions:**

| Function | Purpose |
|---|---|
| `load_prefs()` | Read JSON prefs file, return `{}` on error |
| `save_prefs(prefs)` | Write JSON prefs file |
| `step_name_label_set(prefs)` | Step 1: text input + validation loop |
| `step_select_template(prefs)` | Step 2: `pick_filterable` from existing label sets |
| `step_select_aircraft(prefs)` | Step 3: `pick` from discovered aircraft JSONs |
| `step_select_panels(aircraft_data)` | Step 4: delegates to `panels.select_panels()` |
| `step_confirm_and_generate(name, new_dir, selected)` | Step 5: summary + run reset + generate |
| `main()` | Entry point: banner → Step 1-5 → success/fail |

**Complete Flow:**

```
main()
  │
  ├── Show ASCII banner
  │
  ├── Step 1: step_name_label_set()
  │     ├── ui.header("Step 1: Name Your Label Set")
  │     ├── ui.text_input() → raw name
  │     ├── .upper() + label_set.validate_name()
  │     │     ├── len >= 2
  │     │     ├── regex ^[A-Z][A-Z0-9_]*$
  │     │     └── LABEL_SET_{name} must not exist
  │     ├── ui.confirm("Create LABEL_SET_{name}?")
  │     └── returns name or None
  │
  ├── Step 2: step_select_template()
  │     ├── ui.header("Step 2: Select Template")
  │     ├── label_set.list_label_sets() → 18 directories
  │     ├── ui.pick_filterable() with default from prefs
  │     └── returns Path or None
  │
  ├── Save template pref
  │
  ├── Step 3: step_select_aircraft()
  │     ├── ui.header("Step 3: Select Aircraft")
  │     ├── aircraft.discover_aircraft() → [(name, path)]
  │     ├── ui.pick()
  │     └── returns (name, path) or None
  │
  ├── Save aircraft pref
  │
  ├── Copy template:
  │     ├── label_set.copy_template(template, name) → shutil.copytree
  │     ├── label_set.remove_existing_aircraft_jsons(new_dir)
  │     └── aircraft.deploy_aircraft_json(ac_path, new_dir)
  │
  ├── Step 4: step_select_panels()
  │     ├── aircraft.get_categories(data) → 74 panels (FA-18C)
  │     ├── panels.select_panels() → dual-pane TUI
  │     └── returns list[str] or None
  │
  ├── Write selected_panels.txt:
  │     └── label_set.write_selected_panels(new_dir, selected, all_panels)
  │
  ├── Step 5: step_confirm_and_generate()
  │     ├── Show summary (name, location, panel count, panel list)
  │     ├── ui.confirm("Proceed with generation?")
  │     ├── label_set.run_reset_data(new_dir)    → CREATE_NEW_CONSOLE
  │     └── label_set.run_generate_data(new_dir)  → CREATE_NEW_CONSOLE
  │
  └── big_success() or warn()
```

**ESC / Cancel Handling:**
Every step returns `None` on ESC. The main flow checks and exits gracefully
with `ui.warn("Cancelled.")`. If cancellation happens AFTER template copy
(Steps 4+), the partially-created label set directory is preserved with an
info message telling the user they can finish manually.

---

### LabelCreator-START.py (27 lines) — Entry Point

Thin launcher at project root. Pattern matches `CockpitOS-START.py` (compiler).

1. Checks `platform.system() == "Windows"` (exits if not)
2. Adds `label_creator/` to `sys.path`
3. Imports and calls `label_creator.main()`
4. Catches `KeyboardInterrupt` for clean exit

---

## Data Flow Diagram

```
                    ┌──────────────────────────────┐
                    │    Existing LABEL_SET_*       │
                    │    directories (18 total)     │
                    └──────────┬───────────────────┘
                               │
              ┌────────────────┼────────────────────┐
              │                │                    │
    ┌─────────▼──────┐  ┌─────▼──────┐  ┌──────────▼──────┐
    │ list_label_sets │  │ discover_  │  │ copy_template   │
    │ → template pick │  │ aircraft   │  │ (shutil.copy)   │
    └────────────────┘  │ → JSON pick │  └────────┬────────┘
                        └────────────┘           │
                               │                  │
                    ┌──────────▼──────┐  ┌────────▼────────┐
                    │ load_aircraft_  │  │ New LABEL_SET_X  │
                    │ json → data     │  │ directory        │
                    └──────────┬──────┘  └────────┬────────┘
                               │                  │
                    ┌──────────▼──────┐           │
                    │ get_categories  │           │
                    │ → panel list    │           │
                    └──────────┬──────┘           │
                               │                  │
                    ┌──────────▼──────────────┐   │
                    │ panels.select_panels()  │   │
                    │ (dual-pane TUI)         │   │
                    │ → selected panel names  │   │
                    └──────────┬──────────────┘   │
                               │                  │
                    ┌──────────▼──────┐           │
                    │ write_selected_ │───────────▼
                    │ panels.txt      │  (written to new dir)
                    └─────────────────┘
                               │
                    ┌──────────▼──────┐
                    │ run_reset_data  │ → CREATE_NEW_CONSOLE
                    └──────────┬──────┘
                               │
                    ┌──────────▼──────┐
                    │ run_generate_   │ → CREATE_NEW_CONSOLE
                    │ data            │
                    └──────────┬──────┘
                               │
                    ┌──────────▼──────────────────────────┐
                    │ Generated files in new LABEL_SET_X: │
                    │   InputMapping.h   (all NONE)       │
                    │   LEDMapping.h     (all DEVICE_NONE)│
                    │   DCSBIOSBridgeData.h               │
                    │   LabelSetConfig.h                  │
                    │   panels.txt                        │
                    │   CustomPins.h    (blank template)  │
                    │   DisplayMapping.cpp/h              │
                    └─────────────────────────────────────┘
```

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
| `LABEL_SET_IFEI` | FA-18C_hornet | IFEI fuel display |
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

## Generated File Formats (Post-Generation Output)

### InputMapping.h

```cpp
struct InputMapping {
    const char* label;       // DCS-BIOS identifier (e.g., "MASTER_ARM_SW")
    InputSource source;      // GPIO, HC165, PCA_0xNN, TM1637, MATRIX, NONE
    int8_t port;             // Pin number or I2C port
    int8_t bit;              // Bit position within port
    uint8_t hidId;           // HID report ID (0 = disabled)
    const char* oride_label; // Override label (auto-generated, e.g., slash-split)
    int oride_value;         // Override value (-1 = no override, 65535 = analog)
    const char* controlType; // "set_state", "fixed_step", "variable_step", "action"
    uint8_t group;           // Selector group (0 = none, >0 = exclusive group)
};
```

After generation, ALL entries have `source=NONE, port=-1, bit=-1, hidId=0`.
These must be manually assigned (or automated by Phase 2).

### LEDMapping.h

```cpp
struct LEDMapping {
    const char* label;       // DCS-BIOS identifier (e.g., "MASTER_CAUTION_LT")
    DeviceType deviceType;   // DEVICE_GPIO, DEVICE_WS2812, DEVICE_TM1637,
                             // DEVICE_PCA9555, DEVICE_GN1640T, DEVICE_GAUGE,
                             // DEVICE_NONE
    union {                  // Device-specific configuration
        GPIOInfo gpio;       // { int8_t pin; }
        WS2812Info ws2812;   // { uint16_t index; uint8_t r,g,b; }
        TM1637Info tm1637;   // { int8_t clkPin,dioPin; uint8_t grid,seg; }
        PCA9555Info pca9555; // { uint8_t addr; uint8_t port,bit; }
        GN1640TInfo gn1640t; // { int8_t clkPin,dioPin; uint8_t grid,seg; }
        GaugeInfo gauge;     // { uint8_t index; }
    } info;
    bool dimmable;           // PWM capable (GPIO only)
    bool activeLow;          // Inverted logic
};
```

After generation, ALL entries have `deviceType=DEVICE_NONE`.

### Hash Table Pattern

Both files use a hash table for O(1) lookup by label:

```cpp
constexpr uint16_t INPUT_TABLE_SIZE = 83;   // ~50% load factor
InputMapping inputTable[INPUT_TABLE_SIZE] = { ... };

int findInputByLabel(const char* label) {
    uint16_t h = labelHash(label) % INPUT_TABLE_SIZE;
    for (uint16_t i = 0; i < INPUT_TABLE_SIZE; i++) {
        uint16_t idx = (h + i) % INPUT_TABLE_SIZE;
        if (inputTable[idx].label == nullptr) return -1;
        if (strcmp(inputTable[idx].label, label) == 0) return idx;
    }
    return -1;
}
```

`labelHash()` uses a reversed-string FNV-like hash returning `uint16_t`.

---

## Phase 2: Hardware Wiring Automation (FUTURE — NOT YET IMPLEMENTED)

This is the primary motivation for the entire Label Creator Tool. After Phase 1
generates the label set with all controls set to `NONE`, Phase 2 will provide
an interactive TUI for assigning physical hardware to each control.

### Problem Statement

After `generate_data.py` runs, the user currently must hand-edit every line of
`InputMapping.h` and `LEDMapping.h` to assign GPIO pins, PCA9555 addresses,
TM1637 positions, WS2812 indices, HC165 bits, etc. For a label set with 40+
inputs and 40+ LEDs, this is extremely tedious and error-prone.

### Planned Architecture

**New modules to add to `label_creator/`:**

| Module | Purpose |
|---|---|
| `wiring.py` | Main wiring orchestrator (new step in the flow) |
| `hardware.py` | Hardware inventory: available GPIO pins, PCA addresses, etc. |
| `input_mapper.py` | Parse + modify `InputMapping.h` entries |
| `led_mapper.py` | Parse + modify `LEDMapping.h` entries |

### Planned Flow

```
Step 6: Hardware Wiring (after generation completes)
  │
  ├── Parse InputMapping.h → list of {label, source, port, bit, ...}
  ├── Parse LEDMapping.h → list of {label, deviceType, info, ...}
  │
  ├── Interactive assignment:
  │     ├── Show all unassigned inputs
  │     ├── For each (or batch): pick source type (GPIO/PCA/HC165/TM1637/MATRIX)
  │     ├── Based on source type, pick specific pin/address/bit
  │     ├── Auto-validate: no pin conflicts, valid ranges
  │     └── Show all unassigned LEDs, same flow for device types
  │
  ├── Write back to InputMapping.h (preserving hash table format)
  └── Write back to LEDMapping.h (preserving hash table format)
```

### Hardware Source Types to Support

**For InputMapping.h:**

| Source | Parameters | Use Case |
|---|---|---|
| `GPIO` | pin number via `PIN()` macro | Direct button/switch/pot connection |
| `HC165` | bit position (0-N) | Shift register chain for many switches |
| `PCA_0xNN` | I2C address + port + bit | I2C expander for remote buttons |
| `TM1637` | CLK pin + DIO pin + grid + seg | Button matrix on display modules |
| `MATRIX` | row + col | Key matrix scanning |

**For LEDMapping.h:**

| Device Type | Parameters | Use Case |
|---|---|---|
| `DEVICE_GPIO` | pin, dimmable, activeLow | Direct LED connection |
| `DEVICE_WS2812` | index, R, G, B | Addressable LED strip |
| `DEVICE_TM1637` | CLK pin, DIO pin, grid, seg | LED segments on display |
| `DEVICE_PCA9555` | I2C addr, port, bit, activeLow | I2C expander LED output |
| `DEVICE_GN1640T` | CLK pin, DIO pin, grid, seg | GN1640T LED driver |
| `DEVICE_GAUGE` | index | TFT gauge display |

### Key Technical Challenges for Phase 2

1. **Parsing the hash table format**: `InputMapping.h` and `LEDMapping.h` use
   sparse hash tables with linear probing. Entries are at computed positions,
   not sequential. The parser must handle the hash table structure.

2. **Preserving the generator format**: The generator uses a specific code style
   with exact whitespace, comments, and the hash function. Any modifications
   must produce output that the generator can re-parse on the next regeneration
   (the generator preserves user edits via label-keyed merge).

3. **Pin conflict detection**: Must track all assigned GPIO pins, I2C addresses,
   SPI buses, etc. across both InputMapping and LEDMapping to prevent conflicts.

4. **Group system**: Selectors with the same `group` number form exclusive
   groups. The wiring tool should handle group assignment (which positions
   share a physical multi-position switch).

5. **Encoder pairs**: Rotary encoders use two InputMapping entries with the
   same `oride_label` and values 0/1. The wiring tool should pair these
   automatically.

6. **Analog controls**: Controls with `oride_value=65535` are analog
   potentiometers. They need special GPIO assignment (ADC-capable pins only).

7. **The PIN() macro**: GPIO numbers use `PIN(X)` which auto-converts ESP32-S2
   pin numbers to ESP32-S3 equivalents. The wiring tool needs to understand
   this mapping.

### Other Future Enhancements

- **Template profiles**: Save hardware configurations as reusable profiles
  (e.g., "Standard 16-button PCA setup", "HC165 32-switch chain")
- **Import from existing**: Analyze a working label set and extract its
  hardware mapping as a profile
- **Pin map visualization**: Show a visual pin map of the ESP32 with
  assigned/available pins highlighted
- **Conflict detection**: Real-time validation as pins are assigned
- **Batch assignment**: Select multiple controls and assign them to sequential
  pins/bits on the same device
- **CustomPins.h auto-generation**: Based on the hardware assigned in
  InputMapping/LEDMapping, automatically uncomment and set the corresponding
  `#define` values in `CustomPins.h`
- **Main menu integration**: Add a menu loop (like the compiler tool) so users
  can go back to previous steps, re-edit panel selection, edit an existing
  label set, etc.
- **Edit existing label set**: Open an already-generated label set for
  modification (change panels, reassign hardware) without starting from scratch

---

## How to Extend

### Add a new step to the flow
1. Create a new `step_xxx()` function in `label_creator.py`
2. Add it to the sequence in `main()` after the appropriate step
3. Handle `None` returns (ESC/cancel) with graceful exit

### Add a new module
1. Create `label_creator/new_module.py`
2. Import from `ui` if it needs TUI functions
3. Import in `label_creator.py` to wire into the flow
4. Never create circular imports — `ui.py` imports nothing from our modules

### Modify the panel selector
1. Edit `panels.py` — `select_panels()` for behavior, `_draw()` for layout
2. The draw function does full-screen clear + redraw on every input event
3. To add a new key binding: add an `elif` in the main input loop
4. Preview rendering is in the `# Preview` section of `_draw()`

### Add a new aircraft
1. Place the DCS-BIOS JSON in any `LABEL_SET_*/` directory
2. `aircraft.discover_aircraft()` will find it automatically on next run
3. The JSON must follow the DCS-BIOS structure (top-level categories → controls
   with `outputs` arrays)

### Change name validation rules
1. Edit `_NAME_RE` in `label_set.py` for the regex
2. Edit `validate_name()` for additional checks (length, reserved names, etc.)

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
# Test aircraft discovery
cd label_creator
python -c "import aircraft; print(aircraft.discover_aircraft())"

# Test name validation
python -c "import label_set; print(label_set.validate_name('MY_PANEL'))"

# Test label set listing
python -c "import label_set; print(label_set.list_label_sets())"
```

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
| Menu structure | Menu loop (persistent) | Linear flow (Steps 1-5) |

Both tools share the same `ui.py` toolkit. If `ui.py` is updated in one tool,
the change should be propagated to the other (or a shared import should be set
up in the future).

---

## Known Limitations & Edge Cases

1. **Windows-only**: `msvcrt` keyboard input and `CREATE_NEW_CONSOLE` are
   Windows-specific. Cross-platform would require `curses` or `blessed`.

2. **No menu loop yet**: The tool is a linear 5-step flow. If you cancel at
   Step 4, you must restart. Phase 2 should add a menu loop.

3. **No undo for template copy**: If the user cancels after Step 3 (template
   already copied), the new directory remains. This is intentional (user is
   told they can finish manually), but could offer a cleanup option.

4. **Aircraft JSONs are duplicated**: Every label set has its own copy of the
   aircraft JSON. A central JSON library could save disk space but would break
   the self-contained label set design.

5. **panels.py duplicates ANSI constants**: Rather than importing from `ui.py`,
   it has its own copies. This is intentional for self-containment but means
   color changes must be synced manually.

6. **Full-screen redraw on every keypress**: The panel selector clears and
   redraws the entire screen on every input. This is simple and reliable but
   may flicker on slow terminals. Partial redraws could be added for
   performance.

7. **No validation of generated output**: After `generate_data.py` runs, the
   tool doesn't verify that `InputMapping.h` and `LEDMapping.h` were actually
   created. A post-generation check could be added.
