# CockpitOS Label Creator — Architecture & Implementation Guide

> **Audience**: LLMs continuing development of this tool.
> **Last updated**: February 2026 (Ctrl+D delete for display entries, auto-type detection for segment maps, conditional min/max prompts, auto-regenerate after segment map changes, "Generate & Set as Default" button, warning style).

---

## 1. What Is CockpitOS?

CockpitOS is ESP32 firmware for DCS World flight simulator cockpit panels. Each physical cockpit is defined by a **Label Set** — a folder of auto-generated C headers that map DCS-BIOS controls to physical hardware (GPIO pins, I2C expanders, shift registers, LED drivers, etc.).

The **Label Creator** is a Windows-only Python TUI that replaces manual text editing of these header files with an interactive workflow.

---

## 2. Project Layout

```
CockpitOS/                          <- Arduino sketch root (SKETCH_DIR)
|-- compiler/                       <- Compile Tool (separate TUI app)
|   |-- cockpitos.py                <- Compiler main (has own ASCII banner + "Compile Tool" subtitle)
|   |-- ui.py                       <- Shared UI module (identical to label_creator/ui.py)
|   |-- build.py, boards.py, config.py, labels.py
|   `-- compiler_prefs.json
|
|-- label_creator/                  <- Label Creator Tool (this tool)
|   |-- label_creator.py            <- Main orchestrator (main menu, flows, action menu)
|   |-- ui.py                       <- Terminal UI toolkit (pure presentation, zero domain logic)
|   |-- panels.py                   <- 3-level drill-down panel browser (Level 1/2/3)
|   |-- aircraft.py                 <- Aircraft JSON discovery & loading
|   |-- label_set.py                <- Label set CRUD, subprocess management, config helpers
|   |-- input_editor.py             <- InputMapping.h TUI editor (~1620 lines)
|   |-- led_editor.py               <- LEDMapping.h TUI editor (~700 lines)
|   |-- display_editor.py           <- DisplayMapping.cpp TUI editor (~750 lines)
|   |-- segment_map_editor.py       <- {PREFIX}_SegmentMap.h manager (~1430 lines)
|   |-- creator_prefs.json          <- Workflow preferences (not used for "active" tracking)
|   `-- ARCHITECTURE.md             <- This file
|
|-- src/LABELS/
|   |-- _core/
|   |   |-- generator_core.py       <- Source of truth for regex patterns & format strings
|   |   |-- aircraft/               <- Centralized aircraft JSON library
|   |   |   |-- FA-18C_hornet.json
|   |   |   `-- ...
|   |   `-- template/               <- Boilerplate scripts copied to new label sets
|   |       |-- reset_data.py
|   |       |-- generate_data.py
|   |       `-- display_gen.py
|   |
|   |-- active_set.h                <- Single source of truth: #define LABEL_SET <NAME>
|   |
|   |-- LABEL_SET_IFEI/             <- Example label set (best test file)
|   |   |-- InputMapping.h
|   |   |-- LEDMapping.h
|   |   |-- DCSBIOSBridgeData.h
|   |   |-- LabelSetConfig.h
|   |   |-- CustomPins.h
|   |   |-- aircraft.txt
|   |   |-- selected_panels.txt
|   |   |-- reset_data.py
|   |   |-- generate_data.py
|   |   `-- display_gen.py
|   |
|   `-- LABEL_SET_MAIN/             <- Larger test file (PCA, TM1637, many selector groups)
|       `-- ...
```

---

## 3. Runtime Environment

- **Platform**: Windows 11 only (uses `msvcrt`, `ctypes.windll`, `os.startfile`)
- **Python**: 3.12+ (uses `str | None` union syntax)
- **Input**: `msvcrt.getwch()` -- single-character blocking read, no readline
- **Special key prefixes**: `"\xe0"` or `"\x00"` followed by arrow codes: `H`=Up, `P`=Down, `M`=Right, `K`=Left
- **Terminal**: ANSI escape sequences enabled via `ctypes.windll.kernel32.SetConsoleMode` with `ENABLE_VIRTUAL_TERMINAL_PROCESSING`
- **Encoding**: All file I/O uses `encoding="utf-8"`. Subprocess env includes `PYTHONIOENCODING=utf-8` to handle Unicode when stdout is redirected.
- **Single-instance**: Lock file at `label_creator/.label_creator.lock` with PID. Stale locks are reclaimed. Existing window brought to front via `FindWindowW` + `SetForegroundWindow`.

---

## 4. ui.py — The TUI Toolkit

`ui.py` is a **pure presentation layer** with zero CockpitOS domain knowledge. It provides:

### ANSI Constants
```python
CYAN, GREEN, YELLOW, RED, BOLD, DIM, RESET, REV, HIDE_CUR, SHOW_CUR, ERASE_LN
```

### Output Helpers
- `cls()` -- clear screen
- `cprint(color, text)` -- colored print with Unicode fallback
- `header(text)` -- clear + banner header
- `success(text)`, `warn(text)`, `error(text)`, `info(text)` -- status prefixed messages
- `big_success(title, details)`, `big_fail(title, details)` -- large banners

### Interactive Inputs
| Function | Purpose | Returns |
|---|---|---|
| `pick(prompt, options, default, on_help, on_delete, descriptions)` | Arrow-key selector with optional help, Ctrl+D delete, live descriptions | Selected value, `None` (ESC), or `_DELETE_SENTINEL` (Ctrl+D) |
| `menu_pick(items, initial)` | Full-screen styled menu with separators/spacers | Selected value or `None` (ESC) |
| `pick_filterable(prompt, options)` | Type-to-filter picker for long lists | Selected value or `None` |
| `pick_live(prompt, initial, scan_fn)` | Auto-refreshing picker with background scan | Selected value or `None` |
| `text_input(prompt, default, mask)` | Single-line text input | String, `None` (ESC), or `_RESET_SENTINEL` (Ctrl+D) |
| `confirm(prompt)` | Y/N prompt | `True`, `False`, or `None` (ESC) |
| `toggle_pick(prompt, items)` | Checkbox toggle list | `{key: bool}` dict |

### menu_pick Item Format
```python
("label", "value", "style")           # selectable row
("label", "value", "style", "caption") # selectable row with DIM caption suffix
("---",)                               # plain separator line (unicode dash)
("---", "Section Name")                # labeled separator (centered with dashes)
("",)                                  # blank spacer line (empty row, not selectable)
```
Styles: `"action"` (green bg), `"danger"` (red bg), `"warning"` (yellow bg + black text), `"normal"`, `"dim"`

**Caption alignment**: When items have captions, `menu_pick` computes `max_label_w` across all captioned items and pads labels to align captions in a column.

**Cursor memory**: The `initial` parameter lets the menu start with the cursor on the item whose value matches -- used by `_show_label_set_info()` to remember cursor position across config-edit redraws.

### pick() on_help callback
The `pick()` function accepts an optional `on_help` callable. When provided, pressing `H` calls `on_help()` (typically a scrollable help screen) and then redraws the picker. Used extensively by input_editor.py for contextual help.

### pick() descriptions (live contextual help)
The `pick()` function accepts an optional `descriptions` dict mapping option values to lists of pre-formatted display strings. When provided, a fixed-height description area appears below the option list (separated by a thin `---` line) and **updates dynamically** as the user arrows through options. This is used by:
- **input_editor.py**: `_SOURCE_DESCRIPTIONS` dict explains each input source (GPIO, HC165, TM1637, PCA9555, MATRIX) in beginner-friendly language
- **led_editor.py**: `_DEVICE_DESCRIPTIONS` dict explains each LED device type (GPIO, PCA9555, TM1637, GN1640T, WS2812, GAUGE)

Each description follows a consistent format: bold colored device name + one-line summary, then 2-4 lines of detail, ending with "Good for: ..." examples. The area height is fixed (max across all descriptions) to prevent layout jumping.

**Redundancy rule**: Because the picker descriptions already educate the user about each source/device, the device-specific edit forms that follow do NOT re-describe the hardware. They jump straight into the field prompts (with field-specific hints only). Selector sub-editors in input_editor.py show only a short mode label (e.g., "One-Hot mode") without repeating the source description.

**Section separator (`_SECTION_SEP`)**: Both editors define a `_SECTION_SEP` constant — a thin `── Configure ──` dash line that prints between the picker (education) section and the first field prompt (questions). This gives the user a clear visual break: "you just chose something, now fill in the details." GPIO selector sub-editors use custom variants (e.g., `── One-Hot mode ── ...`).

### Spinner
```python
spinner = ui.Spinner("Generating")
spinner.start()
# ... long operation ...
spinner.stop()
```

---

## 5. label_creator.py — Main Orchestrator

### Main Menu
ASCII banner with "Label Set Creator" subtitle, then 3 emoji status lines:
- `\U0001f5c2\ufe0f` (file box) -- label set count
- `\U0001f3f7\ufe0f` (label) -- last generated label set (from `active_set.h`, NOT `creator_prefs.json`)
- `\U0001f6e9\ufe0f` (airplane) -- aircraft of last generated set

**Important**: If `active_set.h` references a label set whose directory no longer exists, the status shows `RED` with `(deleted)` and suppresses the aircraft line.

Menu options (plain text, no emojis on menu items):
1. **Create New Label Set** (action style -- green)
2. **Modify Label Set** (normal)
3. **Auto-Generate Label Set** (normal)
4. **Delete Label Set** (normal)
5. *(blank spacer)*
6. **--- Switch Tool ---** (labeled separator)
7. **Compile Tool** (normal) -- `os.execl()` to `CockpitOS-START.py`
8. **Environment Setup** (normal) -- `os.execl()` to `Setup-START.py`
9. *(blank spacer)*
10. **Exit** (dim)

### Tool Switcher
`_launch_tool(script_name)` replaces the current process with a sibling CockpitOS tool via `os.execl()`. Cleans up the lock file before exec. No process nesting -- the old tool is gone, the new one takes over the same console window.

### Active Label Set Tracking
```python
ACTIVE_SET = LABELS_DIR / "active_set.h"

def _read_active_label_set() -> str | None:
    # Parses: #define LABEL_SET <NAME>
    # Returns the bare name (e.g., "IFEI"), not the full dir name
```
The main menu loop checks `ls_exists = (LABELS_DIR / f"LABEL_SET_{active_ls}").exists()` on every iteration.

### do_browse() -> _show_label_set_info()
The browse flow picks a label set via `pick_filterable`, then shows `_show_label_set_info()` which **loops internally** -- config edits (device name, HID toggle, custom pins, mapping editors) redraw the screen and loop back. Only "Select panels", "RESET", and "Back" exit the loop.

### Action Menu (inside _show_label_set_info)

The menu uses blank spacer rows `("",)` for visual grouping (not labeled separators). Two variants exist:

**Full menu** (shown when generation has happened AND panels are selected):
```
     Select / deselect panels          (3 selected)

     Edit Inputs                       (12/44 wired)
     Edit Outputs                      (8/27 wired)

     Device Name                       My Panel Name
     HID Mode Selector                 ON
     Edit Custom Pins

     RESET LABEL SET                   (danger style)
     Back                              (dim style)
```

**Minimal menu** (shown when no generation yet or no panels selected):
```
     Select which aircraft panels...   (0 selected)

     RESET LABEL SET                   (danger style)
     Back                              (dim style)
```

The info screen header shows two emoji status lines:
- `\U0001f5a5\ufe0f` (desktop computer) -- device fullname
- `\U0001f6e9\ufe0f` (airplane) -- aircraft name

**Cursor memory**: `last_choice` variable remembers the selected menu value across redraws, passed to `menu_pick(items, initial=last_choice)`.

### Panel Selection -> Auto-Generate Flow
When user selects "Select / deselect panels", the flow returns `"regenerate"` to the outer loop. `_regenerate()` opens the panel selector (pre-populated with existing selection), writes the updated selection, and then **automatically runs `step_generate()`** if panels were selected. No separate "Auto-Generate" option in the action menu.

### LabelSetConfig.h Editing
Two editable `#define` fields in `LabelSetConfig.h`:
- `LABEL_SET_FULLNAME "..."` -- max 32 chars, edited via `text_input`
- `HAS_HID_MODE_SELECTOR 0|1` -- toggled with single press (flip + redraw)

Helpers in `label_set.py`: `read_label_set_config()`, `write_label_set_fullname()`, `write_hid_mode_selector()`

### CustomPins.h
Opened in the user's default editor via `os.startfile()`. No in-app editing.

---

## 6. panels.py — 3-Level Drill-Down Browser

Provides the interactive panel browser used during label set creation/modification.

### TUI Patterns (reused by input_editor.py and led_editor.py)
These patterns are **duplicated locally** in each scrollable-list module:

- **Full-redraw**: `os.system("cls")` + sequential `_w()` calls (no incremental updates)
- **Scroll bar**: `\u2588` thumb / `\u2591` track, proportional sizing
- **Boundary flash**: `_flash_active[0]` flag + `threading.Timer(0.15, restore)` -- briefly renders highlighted row in yellow when at boundary
- **Key input**: `msvcrt.getwch()`, special prefix `"\xe0"`/`"\x00"`, arrows `H`/`P`/`M`/`K`
- **Header format**: `=` bar, title + counter right-aligned, `=` bar
- **Context string**: `"(LABEL_SET_NAME, aircraft)"` appended to all titles
- **`_SEL_BG` row highlight**: `"\033[48;5;236m"` (dark gray background) for the selected row -- gives a subtle, modern look instead of full reverse video

### Levels
| Level | Function | Shows | Keys |
|---|---|---|---|
| 1 | `select_panels()` | Toggle-list of panels with [X] checkboxes | Up/Down, Enter=toggle, Right=drill, S=save, Esc=cancel |
| 2 | `_show_detail()` | Scrollable control list (Label, Desc, Type) | Up/Down, Right=drill, Left/Esc=back |
| 3 | `_show_control_detail()` | Individual control inputs/outputs | Up/Down, Left/Esc=back |

All three levels receive and display `label_set_name` and `aircraft_name` in their headers.

---

## 7. input_editor.py — InputMapping.h TUI Editor (~1620 lines)

### File Structure of InputMapping.h
```
// Comments (4 lines)
#pragma once
struct InputMapping { ... };          <- struct definition (NEVER touch)
// column header comment
static const InputMapping InputMappings[] = {
    { "LABEL", "SOURCE", PORT, BIT, HID, "CMD", VALUE, "TYPE", GROUP },
    ...
};
static const size_t InputMappingSize = ...;
// TrackedSelectorLabels[] ...        <- auto-generated (NEVER touch)
// Static hash lookup table ...       <- auto-generated (NEVER touch)
```

### Parser
Uses the **exact same regex** as `generator_core.py` (lines 960-970):
```python
_LINE_RE = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*'
    r'"(?P<source>[^"]+)"\s*,\s*'
    r'(?P<port>-?\d+|PIN\(\d+\)|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*'
    r'(?P<bit>-?\d+)\s*,\s*'
    r'(?P<hidId>-?\d+)\s*,\s*'
    r'"(?P<cmd>[^"]+)"\s*,\s*'
    r'(?P<value>-?\d+)\s*,\s*'
    r'"(?P<type>[^"]+)"\s*,\s*'
    r'(?P<group>\d+)\s*\}\s*,'
)
```

Each record dict:
```python
{
    "label": str,           # e.g., "MODE_SELECTOR_SW_AUTO"
    "source": str,          # "GPIO", "HC165", "TM1637", "PCA_0x26", "NONE", etc.
    "port": str,            # ALWAYS string: "0", "-1", "PIN(3)", "JETT_CLK_PIN"
    "bit": int,             # Bit position, -1 = fallback position in selector group
    "hidId": int,           # HID usage ID, -1 = none
    "cmd": str,             # DCS-BIOS command label (read-only in editor)
    "value": int,           # Override value (read-only in editor)
    "controlType": str,     # "selector", "momentary", "analog", "variable_step", etc.
    "group": int,           # Selector group ID, 0 = standalone
    "line_index": int,      # Position in raw_lines for surgical write-back
}
```

### User-Editable Fields (ONLY these 4)
`source`, `port`, `bit`, `hidId`

### Write-Back Format
Matches `generator_core.py` lines 1071-1079 exactly:
```python
f'    {{ {lblf}, "{src}" , {port_fmt} , {bit:>2} , {hid:>3} , {cmdf}, {val_str} , {ctf}, {gp:>2} }},\n'
```
- `_fmt_port()` tries `int(port):>2` for alignment, falls back to raw string for `PIN()`/constants
- Only lines at tracked `line_index` positions are replaced in `raw_lines`
- Everything else written back unchanged

### Source-Specific Picker Lists
```python
INPUT_SOURCES = [("NONE", "NONE"), ("GPIO", "GPIO"), ("HC165", "HC165"),
                 ("TM1637", "TM1637"), ("PCA9555", "PCA9555")]

SELECTOR_SOURCES  = [NONE, GPIO, HC165, PCA9555, MATRIX]  # MATRIX for selectors only
ENCODER_SOURCES   = [NONE, GPIO]                           # encoders are GPIO-only
ANALOG_SOURCES    = [NONE, GPIO]                           # analog is GPIO-only
MOMENTARY_SOURCES = INPUT_SOURCES                          # all sources
```

When user picks **PCA9555**, `_prompt_pca_address()` prompts for hex address (strict 2-digit hex, rejects 0x00) and returns `"PCA_0xHH"`.

All source pickers pass `descriptions=_SOURCE_DESCRIPTIONS` to `ui.pick()`, which shows a beginner-friendly description area below the option list that updates as the user scrolls.

### Edit Dispatch Logic (`_do_edit`)
```
record at idx
  |
  +-- controlType == "selector" AND group > 0
  |     -> _edit_selector_group()  (all positions together)
  |
  +-- controlType in ("fixed_step", "variable_step")
  |     -> find encoder partner by matching cmd + controlType
  |     -> _edit_encoder_pair(rec_a=DEC, rec_b=INC)
  |
  +-- controlType == "analog"
  |     -> _edit_analog()  (GPIO pin + HID axis)
  |
  +-- everything else
        -> _edit_standalone()  (source -> source-specific fields -> hidId)
```

### Scrollable List View
- Header: `Input Mapping  (LABEL_SET_NAME, aircraft)     X / Y wired`
- Columns: Label | Source | Port | Bit | HID | Type
- Colors: `source != "NONE"` -> GREEN (wired), `source == "NONE"` -> DIM
- Keys: Up/Down=navigate, Right/Enter=edit, Left/Esc=back, **H=help**

### Help System
Pressing `H` in the main list shows a scrollable help screen. All edit flows also have contextual help via the `on_help` parameter passed to `ui.pick()`:

| Context | Help Content |
|---|---|
| Main list | Columns, navigation, source driver descriptions |
| Selector group | One-hot vs regular modes, PCA/HC165/MATRIX behavior |
| Encoder pair | Pin A/B, fixed_step vs variable_step |
| Analog | ADC pin, HID axis assignment |
| Momentary | Source-specific port/bit/HID details |

### Selector Group Editing

When editing a selector record (group > 0), all positions in the group are gathered and the user picks a source. Dispatch by source type:

| Source | Sub-editor | Key behavior |
|---|---|---|
| GPIO | `_edit_selector_gpio()` | Asks one-hot vs regular mode first |
| GPIO one-hot | `_edit_selector_gpio_onehot()` | All bit=-1, fallback picker (port=-1), per-position GPIO pin |
| GPIO regular | `_edit_selector_gpio_regular()` | bit=0 (active LOW), fallback picker (port=-1), per-position GPIO pin |
| PCA9555 | `_edit_selector_pca()` | Hex address prompt, per-position port(0/1) + bit(0-7), fallback bit=-1 |
| HC165 | `_edit_selector_hc165()` | port=0 (fixed), per-position chain bit(0-63), fallback bit=-1 |
| MATRIX | `_edit_selector_matrix()` | Data pin, anchor/fallback/strobe assignment, auto-computed bit masks |
| Other | `_edit_selector_generic()` | Per-position port + bit (raw) |

All selector editors end with optional HID ID assignment per position.

### MATRIX Selector Detail

MATRIX uses strobe/data rotary scanning:
- **Data pin**: shared GPIO that reads all strobe signals
- **Anchor**: resting position where all strobes connect to data. `port=dataPin, bit=(2^N - 1)` (all-ones mask)
- **Strobe positions**: each gets a unique strobe GPIO pin. `port=strobePin, bit=(1 << strobe_index)`
- **Fallback**: fires when no pattern matches. `port=dataPin, bit=-1`

The editor auto-computes bit masks based on strobe count and position order.

### Encoder Pair Editing

Encoders (`fixed_step`, `variable_step`) are pairs sharing the same DCS-BIOS command:
- `_find_encoder_partner()` locates the partner record by matching `cmd` + `controlType`, opposite `value` (0 vs 1)
- Pin A = DEC/CCW (value=0), Pin B = INC/CW (value=1)
- Source is GPIO-only. bit is always 0 (auto-set).

### Re-parse After Write
After every successful edit + save, the editor re-parses from disk:
```python
records, raw_lines = parse_input_mapping(filepath)
```
This keeps `line_index` values and all state consistent.

---

## 8. led_editor.py — LEDMapping.h TUI Editor (~700 lines)

### File Structure of LEDMapping.h
```
// Comment
#pragma once
enum LEDDeviceType { ... };           <- enum definition (NEVER touch)
struct LEDMapping { ... };            <- struct definition (NEVER touch)
// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "LABEL", DEVICE_X, {.infoType = {values}}, dimmable, activeLow },
  ...
};
static constexpr uint16_t panelLEDsCount = ...;
// Auto-generated hash table ...      <- NEVER touch
```

### Parser
Uses the **exact same regex** as `generator_core.py` (lines 1174-1178):
```python
_ENTRY_RE = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*DEVICE_(?P<device>[A-Z0-9_]+)\s*,'
    r'\s*\{\s*\.(?P<info_type>[a-zA-Z0-9_]+)\s*=\s*\{\s*(?P<info_values>[^}]+)\}\s*\}'
    r'\s*,\s*(?P<dimmable>true|false)\s*,\s*(?P<activeLow>true|false)\s*\}',
    re.MULTILINE
)
```

Each record dict:
```python
{
    "label": str,           # e.g., "FLP_LG_FLAPS_LT"
    "device": str,          # "NONE", "GPIO", "PCA9555", "TM1637", "GN1640T", "WS2812", "GAUGE"
    "info_type": str,       # "gpioInfo", "pcaInfo", "tm1637Info", "gn1640Info", "ws2812Info", "gaugeInfo"
    "info_values": str,     # Comma-separated values, e.g., "JETT_CLK_PIN, JETT_DIO_PIN, 5, 0"
    "dimmable": str,        # "true" or "false" (stored as string)
    "activeLow": str,       # "true" or "false" (stored as string)
    "line_index": int,
}
```

### User-Editable Fields (ALL of them)
`device`, `info_type`, `info_values`, `dimmable`, `activeLow`

### Write-Back Format
Matches `generator_core.py` lines 1234-1237 (preserved-entry path):
```python
# Non-NONE:
f'  {{ {padded_label}, DEVICE_{dev.ljust(8)}, {{.{info_type} = {{{info_values}}}}}, {dimmable}, {activeLow} }}, {comment},\n'
# NONE:
f'  {{ {padded_label}, DEVICE_NONE    , {{.gpioInfo = {{0}}}}, false, false }}, {comment},\n'
```

**Trailing comma handling**: The generator uses `",\n".join(final_panel)` so:
- Non-last entries have a trailing `,` at the end of the line
- Last entry has NO trailing `,`
- We detect this from the original line (`orig_line.rstrip().endswith(",")`) and preserve the pattern

### Comment Generator
Ported from `generator_core.py`'s `generate_comment()`:
```python
def _generate_comment(device, info_type, info_values):
    # Returns "// TM1637 CLK X DIO Y Seg Z Bit W" etc.
```

### PCA9555 Address Prompt
`_prompt_pca_hex_address()` -- strict 2-digit hex validation, normalizes to `0xHH`, rejects `0x00`. Returns the address string (e.g., `"0x20"`), not the full `PCA_0x20` source name (different from input_editor which returns the full source).

### Device Type Picker with Descriptions
The device picker passes `descriptions=_DEVICE_DESCRIPTIONS` to `ui.pick()`, showing a beginner-friendly description area below the option list that updates as the user scrolls through NONE, GPIO, PCA9555, TM1637, GN1640T, WS2812, and GAUGE.

### Device-Specific Edit Forms

Each device type jumps straight into field prompts (no redundant hardware explanation — that was already shown in the picker description):

| Device | info_type | Fields Prompted | Field-level hints |
|---|---|---|---|
| NONE | gpioInfo | (auto-cleared to defaults) | -- |
| GPIO | gpioInfo | pin | -- |
| PCA9555 | pcaInfo | address, port, bit | "Port 0 = bank A, Port 1 = bank B" |
| TM1637 | tm1637Info | clkPin, dioPin, segment, bit | Segment/bit range hints |
| GN1640T | gn1640Info | address, column, row | -- |
| WS2812 | ws2812Info | index, pin, defR, defG, defB, defBright | "Default color when LED is ON" |
| GAUGE | gaugeInfo | gpio, minPulse, maxPulse, period | "Typical 544-2400µs at 50Hz" |

After device-specific fields:
- **Dimmable** (true/false picker): "LED intensity follows panel dimmer (PWM)"
- **Reversed?** (No/Yes picker): Beginner-friendly alias for Active Low. "No" = normal (most common, maps to `activeLow=false`), "Yes" = reversed/active-low (maps to `activeLow=true`). The underlying record field is still `activeLow` with `"true"`/`"false"` string values.

### _DEVICE_INFO_MAP
Maps device name to its union member name:
```python
_DEVICE_INFO_MAP = {
    "NONE": "gpioInfo", "GPIO": "gpioInfo", "PCA9555": "pcaInfo",
    "TM1637": "tm1637Info", "GN1640T": "gn1640Info",
    "WS2812": "ws2812Info", "GAUGE": "gaugeInfo",
}
```

### info_values Format by Device
- GPIO: `"PIN(6)"` or `"0"` (single value)
- PCA9555: `"0x20, 0, 3"` (address, port, bit)
- TM1637: `"JETT_CLK_PIN, JETT_DIO_PIN, 5, 0"` (clk, dio, seg, bit) -- can use named constants!
- GN1640T: `"0, 2, 1"` (address, column, row)
- WS2812: `"0, 5, 0, 0, 0, 255"` (index, pin, R, G, B, brightness)
- GAUGE: `"6, 544, 2400, 20000"` (gpio, minPulse, maxPulse, period)

### Scrollable List View
- Header: `LED Mapping  (LABEL_SET_NAME, aircraft)     X / Y wired`
- Columns: Label | Device | Info (summary) | Dim | ALow
- Colors: `device != "NONE"` -> GREEN (wired), `device == "NONE"` -> DIM
- Keys: Up/Down=navigate, Right/Enter=edit, Left/Esc=back

### _info_summary()
Builds a compact summary from device info for the list column:
- GPIO: `pin=PIN(6)`
- PCA9555: `addr=0x20 p=0 b=3`
- TM1637: `clk=JETT_CLK_PIN dio=JETT_DIO_PIN s=5 b=0`
- GN1640T: `a=0 c=2 r=1`
- WS2812: `idx=0`
- GAUGE: `gpio=6 544-2400`

---

## 9. display_editor.py — DisplayMapping.cpp TUI Editor (~750 lines)

### Purpose
Edits user-configurable fields in the `fieldDefs[]` array within `DisplayMapping.cpp`. Display fields define how DCS-BIOS display data (CT_DISPLAY) is rendered on physical segmented displays (7-segment, 14-segment, bargraphs) through driver chips like HT1622.

### File Structure of DisplayMapping.cpp
```
// DisplayMapping.cpp - Defines the display fields
#include "../../Globals.h"
#include "DisplayMapping.h"

const DisplayFieldDefLabel fieldDefs[] = {
    { "LABEL", &MAP_PTR, numDigits, segsPerDigit, min, max, FIELD_TYPE, barCount, &driver, DEVICE_TYPE, renderFunc, clearFunc, RENDER_TYPE },
    ...
};
size_t numFieldDefs = sizeof(fieldDefs) / sizeof(fieldDefs[0]);
FieldState fieldStates[...];
// Hash table ...         <- NEVER touch
// findFieldDefByLabel    <- NEVER touch
// findFieldByLabel       <- NEVER touch
```

### Parser
Regex matches each `fieldDefs[]` entry and extracts all 13 fields. Records store all values as strings for faithful round-tripping. Parser only operates within the `fieldDefs[]` array (between `const DisplayFieldDefLabel fieldDefs[]` and `};`).

### User-Editable Fields
- `renderType` — which render algorithm to use (picker with descriptions)
- `fieldType` — data type (FIELD_LABEL, FIELD_STRING, FIELD_NUMERIC, FIELD_MIXED, FIELD_BARGRAPH)
- `minValue` / `maxValue` — value range (only prompted for FIELD_NUMERIC; firmware range-checks `value >= min && value <= max`)
- `barCount` — number of bar segments (only prompted for BARGRAPH render type)
- `clearFunc` — `nullptr` or a clear dispatcher function name

### Read-Only Fields (auto-generated)
`label`, `segMap`, `numDigits`, `segsPerDigit`, `driver`, `deviceType`, `renderFunc`

### FieldRenderType Options
| Render Type | Description |
|---|---|
| FIELD_RENDER_7SEG | Standard 7-segment numeric display |
| FIELD_RENDER_7SEG_SHARED | 7-seg sharing physical space with another field (overlay) |
| FIELD_RENDER_LABEL | Single on/off indicator (texture) |
| FIELD_RENDER_BINGO | Bingo fuel display (special digit handling) |
| FIELD_RENDER_BARGRAPH | Pointer bar / bargraph (maps value to segments) |
| FIELD_RENDER_FUEL | Fuel display with overlay support |
| FIELD_RENDER_RPM | RPM display (special hundreds-digit handling) |
| FIELD_RENDER_ALPHA_NUM_FUEL | 14-segment fuel with overlay support |
| FIELD_RENDER_CUSTOM | Custom render function |

### "Configured" Definition
A record is "configured" if its `segMap` pointer is non-nullptr — meaning it has a real segment map backing it. Entries with `nullptr` segMap are generator skeletons with no hardware. This simple check replaced the previous heuristic that compared field values against generator defaults (which broke when user-confirmed defaults matched generator defaults exactly).

### Ctrl+D Delete
The display editor supports Ctrl+D to delete individual entries from `fieldDefs[]`. This removes the preserved line from DisplayMapping.cpp. On next regeneration, the entry will either be regenerated with defaults (if a matching segment map exists) or omitted entirely (if no segment map matches). Single confirmation required.

### Conditional Field Prompts
The edit form only shows prompts relevant to the selected field type:
- **FIELD_NUMERIC** — prompts for minValue/maxValue (firmware does `value >= min && value <= max`). Auto-fixes dangerous `0,0` default to `0,99999`. Warns if max ≤ min.
- **FIELD_RENDER_BARGRAPH** — prompts for barCount
- **All other types** — skips min/max and barCount (firmware ignores them)

### Write-Back
Only replaces lines at tracked `line_index` positions. Hash tables, lookup functions, and includes are untouched. Trailing comma handling follows the same pattern as led_editor.py.

### Scrollable List View
- Header: `Display Mapping  (LABEL_SET_NAME, aircraft)     X / Y configured`
- Columns: Label | Dims | Render | Type
- Colors: configured (non-nullptr segMap) -> GREEN, unconfigured -> DIM
- Keys: Up/Down=navigate, Right/Enter=edit, Left/Esc=back, **Ctrl+D=delete entry**

---

## 10. segment_map_editor.py — Segment Map Manager (~1430 lines)

### Purpose
Creates and manages `{PREFIX}_SegmentMap.h` files. These files define the physical mapping between logical display fields and hardware RAM addresses, bits, and chip IDs for display driver chips. They are a **prerequisite** for display generation — without a segment map, the display generator skips all fields for that device prefix.

### Entry Point
```python
segment_map_editor.manage_segment_maps(ls_dir, ls_name, ac_name)
# Returns True if any segment map was created/modified
```

### Flow
1. **Device Picker** — scans aircraft JSON for display device prefixes (e.g., IFEI, HUD, UFC). Shows which have existing segment maps and which are missing. Supports **Ctrl+D delete** with double confirmation.
2. **Existing Map** — scrollable list of declared variables with dimensions and chip IDs. Supports editing individual entries.
3. **New Map** — guides user through creating entries field by field: pick field, **auto-detect type** (MAP or LABEL), enter dimensions, enter addresses.

### Entry Type Auto-Detection
The entry type (MAP vs LABEL) is **automatically determined** from the field name convention, matching the generator's `get_map_pointer_and_dims()` logic in `display_gen_core.py`:
- Field ending in `_TEXTURE` or `_LABEL` → **LABEL** type (single on/off indicator)
- Everything else → **MAP** type (multi-digit display)

Users cannot manually choose the wrong type — the old picker that let users freely select MAP/LABEL was removed because mismatches caused the generator to silently skip entries (e.g., creating `HUD_LTDR_LABEL` when the generator expected `HUD_LTDR_MAP`).

### Segment Map File Format
```c
#pragma once

// ================== LEFT ENGINE RPM ==================
// [0]=TOP, [1]=TOP-RIGHT, [2]=BOTTOM-RIGHT, [3]=BOTTOM, [4]=BOTTOM-LEFT, [5]=TOP-LEFT, [6]=MIDDLE
const SegmentMap IFEI_RPM_L_MAP[3][7] = {
    { {2,3,0}, {2,2,0}, {2,1,0}, {2,0,0}, {0,1,0}, {0,3,0}, {0,2,0} },
    ...
};
const SegmentMap IFEI_RPM_TEXTURE_LABEL = {5, 0, 0};
```

Each entry: `{ RAM_ADDR, BIT, CHIP_ID }` where:
- RAM_ADDR: 0-63 (HT1622 RAM range) or 0xFF (unused)
- BIT: 0-7 or 0xFF (unused)
- CHIP_ID: which physical display chip (0, 1, etc.)

### Dimension Patterns Found in Real Data
| Pattern | Example | Usage |
|---|---|---|
| [3][7] | RPM_L, FF_L, OIL_PRESS | 3-digit 7-segment |
| [2][7] | CLOCK_H, TIMER_M | 2-digit 7-segment |
| [5][7] | BINGO | 5-digit 7-segment |
| [6][14] | FUEL_UP, T | 6-position 14-segment |
| [1][11] | NOZZLE_L | Single bargraph |
| single | RPM_TEXTURE_LABEL | On/off indicator |

### Parser
```python
parse_segment_map(filepath) -> list[dict]
```
Each entry dict: `name`, `kind` (MAP/LABEL), `dims`, `entries` (nested tuples), `line_start`, `line_end`, `comment_above`.

### Validation Rules
- No duplicate `{addr, bit, chipID}` combinations within a file
- All chip IDs within a single MAP entry should be consistent
- addr ≤ 63 (or 0xFF), bit ≤ 7 (or 0xFF)
- **Segment order MUST match font table order** (critical — documented in SegmentMap.txt)

### Device Prefix Scanner
```python
scan_display_prefixes(ls_dir, ac_name) -> list[dict]
```
Loads aircraft JSON, extracts all `control_type: "display"` entries, groups by prefix. Returns prefix info including field count and whether a segment map exists.

### Regeneration Integration
When `manage_segment_maps()` returns `True` (changed), `label_creator.py` **automatically** runs `_auto_generate()` so the new maps are picked up by `display_gen_core.py`. The user stays on the Modify Label Set menu after regeneration completes (no panel selector detour).

---

## 11. Display System Pipeline

### Full Chain
```
DCS-BIOS data
  → DCSBIOSBridgeData.h (CT_DISPLAY entries in DcsOutputTable[])
  → CT_Display.cpp (char buffers + dirty flags per field)
  → DCSBIOSBridge.cpp (buffer accumulation + renderField() call)
  → DisplayMapping.cpp (fieldDefs[] with segment maps, render types)
  → Panel.cpp (renderDispatcher switch → shadow buffer → hardware)
```

### Generation Chain
```
aircraft JSON (control_type: "display")
  → display_gen_core.py (scans for {PREFIX}_SegmentMap.h)
  → DisplayMapping.cpp + DisplayMapping.h
```

Key: The generator SKIPS fields with no matching segment map. Creating a segment map and regenerating is the way to "activate" display fields.

### Preservation
The generator preserves existing hand-edited `fieldDefs[]` entries (by label) if they have a non-nullptr map pointer. This means user edits via the display editor survive regeneration.

---

## 12. Critical Rules (applies to ALL editors)

### NEVER Modify
1. **Hash tables** (`inputHashTable[]`, `ledHashTable[]`, `fieldDefHashTable[]`) -- auto-generated, indexed by label hash
2. **Struct definitions** (`struct InputMapping`, `struct LEDMapping`, `struct DisplayFieldDefLabel`, `enum LEDDeviceType`)
3. **TrackedSelectorLabels[]** -- auto-generated list of selector DCS labels
4. **Size constants** (`InputMappingSize`, `panelLEDsCount`, `numFieldDefs`)
5. **Hash/lookup functions** (`findInputByLabel`, `findLED`, `findFieldDefByLabel`, `findFieldByLabel`)
6. **File header comments** (the `// THIS FILE IS AUTO-GENERATED...` block)
7. **FieldState arrays** (`fieldStates[]`)

### ONLY Modify
- **Individual records** within `InputMappings[]`, `panelLEDs[]`, and `fieldDefs[]` arrays
- Specifically the user-editable fields listed in each editor's section above

### Port Is Always a String
In InputMapping, port can be: `0`, `-1`, `PIN(3)`, `PIN(40)`, `JETT_CLK_PIN`, or any `[A-Za-z_][A-Za-z0-9_]*` identifier. Always store and compare as string. The `_fmt_port()` helper tries `int()` for right-aligned numeric formatting, falls back to raw string.

### LED info_values Can Contain Named Constants
Values like `JETT_CLK_PIN`, `PIN(9)` are valid inside info_values. These are C constants defined in `CustomPins.h`. The editor stores them as-is (string passthrough).

### Re-parse After Every Write
After any record is saved to disk, both editors re-parse the entire file:
```python
write_input_mapping(filepath, records, raw_lines)
records, raw_lines = parse_input_mapping(filepath)  # re-parse from disk
```
This ensures `line_index` values remain correct and all state is consistent.

### Generator Preservation
After user edits via our TUI, running `generate_data.py` will **preserve** those edits. The generator reads existing records by label, merges with the new control list, and keeps user-edited fields (source, port, bit, hidId for inputs; device, info, dimmable, activeLow for LEDs). This is the fundamental design: user edits are durable across regeneration.

---

## 13. Selector Groups (InputMapping)

Selectors are multi-position switches (e.g., a 3-position switch: OFF/AUTO/MAN). They share the same `oride_label` (DCS command) and `group` ID.

### Example: MODE_SELECTOR_SW (3-position, group=5)
```c
{ "MODE_SELECTOR_SW_AUTO", "HC165", 0,  6, -1, "MODE_SELECTOR_SW", 0, "selector", 5 },
{ "MODE_SELECTOR_SW_OFF",  "HC165", 0, -1, -1, "MODE_SELECTOR_SW", 1, "selector", 5 },
{ "MODE_SELECTOR_SW_MAN",  "HC165", 0,  0, -1, "MODE_SELECTOR_SW", 2, "selector", 5 },
```

Key points:
- All positions share: same source, same port, same group ID
- Each position has a unique bit (one position is `-1` = fallback/default)
- The `bit = -1` position is the "rest" position (what the switch reads when neither extreme is active)
- `value` is the DCS-BIOS command value for that position (0, 1, 2...)

### GPIO Selector Modes (firmware detail)

**ONE-HOT** (all bits == -1):
- Firmware CASE 1: `oneHot == total`
- Each wired position: unique GPIO pin, bit=-1
- First LOW pin wins
- Fallback: port=-1, bit=-1 (fires when no pin is LOW)
- All positions CAN be wired (no fallback needed)

**REGULAR** (NOT all bits == -1):
- Firmware CASE 2: `oneHot != total`
- Each wired position: GPIO pin, bit=0 (active LOW)
- Fallback: port=-1, bit=0 (bit MUST NOT be -1 or it pollutes oneHot count)

### Editor Behavior
When user presses Edit on ANY position in a selector group:
1. All positions in that group are gathered (`[r for r in records if r["group"] == group]`)
2. All positions displayed together with current assignments
3. Source picker (dispatches to source-specific sub-editor)
4. Source-specific sub-editor handles fallback, pin/bit assignment per position
5. Optional HID IDs per position

---

## 14. Integration Points in label_creator.py

### Imports
```python
import input_editor
import led_editor
import display_editor
import segment_map_editor
```

### Caption Helpers
```python
def _input_mapping_caption(ls_dir) -> str:
    # Returns "12/44 wired" or "not generated"

def _led_mapping_caption(ls_dir) -> str:
    # Returns "8/27 wired" or "not generated"

def _display_mapping_caption(ls_dir) -> str:
    # Returns "5/32 configured" or "not generated" or "no fields"

def _segment_map_caption(ls_dir, ac_name) -> str:
    # Returns "1 of 3 devices" or "no displays"
```

### Menu Structure (when `is_generated and has_panels`)
```
    Select / deselect panels          (3 selected)         <- action style (green)
    Generate & Set as Default         (active) / (not active)  <- warning style (yellow)

    ——— LEDs & Controls ———
    Edit Inputs                       (12/44 wired)
    Edit Outputs (LEDs)               (8/27 wired)

    ——— Displays ———
    Segment Maps                      (1 of 3 devices)
    Edit Displays                     (5/32 configured)    <- only if DisplayMapping.cpp has entries

    ——— Misc Options ———
    Device Name                       My Panel Name
    HID Mode Selector                 ON
    Edit Custom Pins

    RESET LABEL SET                   (danger style)
    Back                              (dim style)
```

The **"Generate & Set as Default"** button (warning/yellow style) runs `_auto_generate()` which makes this label set the active one in `active_set.h`. Its caption shows `(active)` in green if this label set is already the default, or `(not active)` in dim if not.

### Handlers
```python
elif choice == "edit_input":
    input_editor.edit_input_mapping(...)
    continue

elif choice == "edit_led":
    led_editor.edit_led_mapping(...)
    continue

elif choice == "edit_display":
    display_editor.edit_display_mapping(...)
    continue

elif choice == "segment_maps":
    changed = segment_map_editor.manage_segment_maps(...)
    if changed:
        _auto_generate(ls_name, ls_dir, prefs)  # auto-regenerate in place
    continue
```

### Regeneration Flow
When a segment map is created or modified, the user is prompted to regenerate. This re-runs `generate_data.py` which includes `display_gen.py`, causing `display_gen_core.py` to pick up the new segment maps and populate `DisplayMapping.cpp` with field entries. Previously hand-edited entries are preserved by label.

---

## 15. Test Files

| File | Why It's Good for Testing |
|---|---|
| `src/LABELS/LABEL_SET_IFEI/InputMapping.h` | Has GPIO with PIN() macro, HC165, TM1637, selector groups (3-position with fallback bit=-1), analog + variable_step pairs, NONE records |
| `src/LABELS/LABEL_SET_IFEI/LEDMapping.h` | Has TM1637 with named constants (JETT_CLK_PIN), GPIO with PIN(), NONE records, dimmable entries |
| `src/LABELS/LABEL_SET_MAIN/InputMapping.h` | Larger file with PCA_0x26, PCA_0x5B, TM1637 with PIN(), many selector groups, GPIO analog |
| `src/LABELS/LABEL_SET_NEWIFEI/DisplayMapping.cpp` | 32 fieldDefs[] entries with various render types, hash table, lookup functions |
| `src/LABELS/LABEL_SET_NEWIFEI/IFEI_SegmentMap.h` | 35 entries covering all dimension patterns: [3][7], [2][7], [5][7], [6][14], [1][11], single struct |
| `src/LABELS/LABEL_SET_NEWIFEI/SegmentMap.txt` | Documentation/guide for understanding segment map format and construction |

---

## 16. UI Patterns Quick Reference

### Shared Row Highlight: _SEL_BG
All scrollable lists (panels.py, input_editor.py, led_editor.py) use the same highlight style:
```python
_SEL_BG = "\033[48;5;236m"    # subtle dark gray background
```
This replaces full reverse video (`REV`) for content rows. The cursor arrow `>` is rendered in CYAN, content preserves its GREEN/DIM coloring on the dark gray background.

### Full-Redraw Scrollable List Pattern
```python
def _draw():
    os.system("cls")
    _w(HIDE_CUR)
    # Header (3 lines: = bar, title + counter, = bar)
    # Column headers (1 line)
    # Rows (list_height lines, with scroll bar character at right edge)
    # Footer (hint line)

# Key loop
while True:
    ch = msvcrt.getwch()
    if ch in ("\xe0", "\x00"):
        ch2 = msvcrt.getwch()
        # H=Up, P=Down, M=Right, K=Left
    elif ch == "\r":  # Enter
    elif ch == "\x1b":  # Esc
```

### Scroll Bar
```python
_SCROLL_BLOCK = "\u2588"  # full block
_SCROLL_LIGHT = "\u2591"  # light shade
thumb = max(1, round(list_height * list_height / total))
top = round(scroll * (list_height - thumb) / max_scroll)
```

### Boundary Flash
```python
_flash_timer = [None]    # mutable container (closures can't reassign nonlocal easily)
_flash_active = [False]  # when True, highlighted row renders in YELLOW

def _flash_row():
    _flash_active[0] = True
    _draw()
    def _restore():
        _flash_active[0] = False
        _draw()
    _flash_timer[0] = threading.Timer(0.15, _restore)
    _flash_timer[0].daemon = True
    _flash_timer[0].start()
```

### Scrollable Help Screen (input_editor.py)
```python
def _show_help(title, lines):
    # Full-screen display of help text
    # Up/Down = scroll, any other key = exit back
    # Used standalone (H key in list) and as on_help callback in ui.pick()
```

---

## 17. Emoji Conventions

Emojis appear ONLY on **status display lines** (never on menu options). All use double-width supplementary plane characters with variation selector:

### Main Menu Status
| Emoji | Code | Purpose |
|---|---|---|
| file box | `\U0001f5c2\ufe0f` | Label set count |
| label | `\U0001f3f7\ufe0f` | Last generated label set |
| airplane | `\U0001f6e9\ufe0f` | Aircraft |

### Label Set Info Screen Status
| Emoji | Code | Purpose |
|---|---|---|
| desktop computer | `\U0001f5a5\ufe0f` | Device fullname |
| airplane | `\U0001f6e9\ufe0f` | Aircraft |

The compiler tool uses its own set: desktop (board), link (transport), bug (debug).

**Rule**: Never mix single-width and double-width emojis in the same display group -- it breaks margin alignment.

---

## 18. Known Quirks & Gotchas

1. **generator_core.py exit code**: Always exits with code 1, even on success. The reliable success signal is the `.last_run` timestamp file.

2. **Port formatting in generator**: Uses `{port:>2}` which works for both ints and strings in Python's format spec (right-aligns to min width 2).

3. **LED comma pattern**: The generator uses `",\n".join(final_panel)` to join entries, so non-last entries get a trailing `,` after the comment. The last entry has no trailing `,`. Our write-back detects and preserves this.

4. **InputMapping always has trailing comma**: Unlike LED, the generator writes each InputMapping line with `}},\n` (hardcoded trailing comma on every line).

5. **NONE entries in LEDMapping**: Freshly generated NONE entries don't have a comment or `}},`. But once preserved through a regeneration cycle, they get the full `}}, // No Info` format. Our editor always writes the full format.

6. **text_input returns**: Can return `str` (valid input), `None` (ESC=cancel), or `ui._RESET_SENTINEL` (Ctrl+D=reset). Always check for `None` before using the value.

7. **confirm returns**: Can return `True`, `False`, or `None` (ESC). Not just a boolean!

8. **Tool switcher uses os.execl()**: The process is **replaced** -- all Python cleanup (atexit handlers, finally blocks) runs, but the old tool's code is gone after exec. Lock file is cleaned up manually before exec.

9. **menu_pick blank spacers vs separators**: `("",)` creates a blank line (not selectable, no dash). `("---",)` creates a unicode dash separator. `("---", "Label")` creates a labeled separator. All three are different visual elements.

10. **PCA address handling differs between editors**: `input_editor._prompt_pca_address()` returns the full source string `"PCA_0x20"`. `led_editor._prompt_pca_hex_address()` returns just the address `"0x20"`. This is because InputMapping stores the source as `"PCA_0x20"` while LEDMapping stores the address as a value inside info_values.

11. **UTF-8 encoding in generators**: When generators run in a piped/redirected context (not a real console), Windows defaults to cp1252 encoding which can't handle Unicode characters like `✓`. Fixed by wrapping `sys.stdout`/`sys.stderr` with `io.TextIOWrapper(encoding="utf-8", errors="replace")` at the top of `generator_core.py`, `display_gen_core.py`, and `generate_panelkind.py`.

12. **`.DISABLED` convention**: The generator renames `.cpp` files in inactive label sets to `.cpp.DISABLED` to avoid linker conflicts. Only the active label set has live `.cpp` files. This means DisplayMapping.cpp won't be editable until the label set is made active via generation.

13. **Multiple segment maps**: The generator supports multiple segment maps simultaneously (e.g., IFEI + HUD + UFC) via prefix-namespaced variables. No single-active restriction needed.

14. **Display minValue/maxValue with 0,0**: For `FIELD_NUMERIC` type, the firmware does `value >= minValue && value <= maxValue`. If both are 0, only the literal value `0` passes — everything else is rejected. For non-NUMERIC types (STRING, LABEL, MIXED, BARGRAPH), the range check is skipped entirely, so 0,0 is harmless.

15. **ANSI color inside DIM wrappers**: Embedding `{ui.GREEN}text{ui.RESET}` inside a DIM-wrapped string kills the DIM for everything after the RESET. Fix: put parentheses/punctuation inside the color span, not outside (e.g., `f"{ui.GREEN}(active){ui.RESET}"` not `f"({ui.GREEN}active{ui.RESET})"`).

16. **BOLD + black text on colored background**: `\033[1m\033[30m` renders as bright black (gray) on many terminals. For yellow background + black text, omit BOLD: `\033[43m\033[30m`.

---

## 19. Future Work / Pending Tasks

1. **CustomPins.h validation** -- User mentioned wanting pin validation in the future (e.g., checking for conflicts). Currently just opens in default editor.

2. **Testing the editors** -- The editors need real-world testing:
   - Edit a standalone momentary record -> assign GPIO + pin -> verify file
   - Edit a selector group -> verify all positions -> verify fallback
   - Edit an encoder pair -> verify both pins written correctly
   - Edit an analog record -> verify ADC pin + HID axis
   - Set a record to NONE -> verify defaults
   - Edit LED GPIO/TM1637/PCA9555/WS2812/GAUGE -> verify device-specific fields
   - Run auto-generate after edits -> verify preservation
   - Verify hash tables are NOT modified
   - Test MATRIX selector (data pin, anchor, strobe pins, bit masks)

3. **Potential improvements** identified during development:
   - Search/filter in mapping editors (like `pick_filterable`)
   - Bulk operations (e.g., "wire all TM1637 buttons on this keypad")
   - Undo support (currently no undo -- ESC during edit cancels, but completed edits are immediate)
   - Display of which panel each record belongs to (would require cross-referencing with aircraft JSON)
