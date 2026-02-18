# CockpitOS Label Creator — Architecture & Implementation Guide

> **Audience**: LLMs continuing development of this tool.
> **Last updated**: February 2026 (after InputMapping/LEDMapping editors were created).

---

## 1. What Is CockpitOS?

CockpitOS is ESP32 firmware for DCS World flight simulator cockpit panels. Each physical cockpit is defined by a **Label Set** — a folder of auto-generated C headers that map DCS-BIOS controls to physical hardware (GPIO pins, I2C expanders, shift registers, LED drivers, etc.).

The **Label Creator** is a Windows-only Python TUI that replaces manual text editing of these header files with an interactive workflow.

---

## 2. Project Layout

```
CockpitOS/                          ← Arduino sketch root (SKETCH_DIR)
├── compiler/                       ← Compile Tool (separate TUI app)
│   ├── cockpitos.py                ← Compiler main (has own ASCII banner + "Compile Tool" subtitle)
│   ├── ui.py                       ← Shared UI module (identical to label_creator/ui.py)
│   ├── build.py, boards.py, config.py, labels.py
│   └── compiler_prefs.json
│
├── label_creator/                  ← Label Creator Tool (this tool)
│   ├── label_creator.py            ← Main orchestrator (main menu, flows, action menu)
│   ├── ui.py                       ← Terminal UI toolkit (pure presentation, zero domain logic)
│   ├── panels.py                   ← 3-level drill-down panel browser (Level 1/2/3)
│   ├── aircraft.py                 ← Aircraft JSON discovery & loading
│   ├── label_set.py                ← Label set CRUD, subprocess management, config helpers
│   ├── input_editor.py             ← InputMapping.h TUI editor (NEW)
│   ├── led_editor.py               ← LEDMapping.h TUI editor (NEW)
│   ├── creator_prefs.json          ← Workflow preferences (not used for "active" tracking)
│   └── ARCHITECTURE.md             ← This file
│
├── src/LABELS/
│   ├── _core/
│   │   ├── generator_core.py       ← Source of truth for regex patterns & format strings
│   │   ├── aircraft/               ← Centralized aircraft JSON library
│   │   │   ├── FA-18C_hornet.json
│   │   │   └── ...
│   │   └── template/               ← Boilerplate scripts copied to new label sets
│   │       ├── reset_data.py
│   │       ├── generate_data.py
│   │       └── display_gen.py
│   │
│   ├── active_set.h                ← Single source of truth: #define LABEL_SET <NAME>
│   │
│   ├── LABEL_SET_IFEI/             ← Example label set (best test file — has GPIO, HC165, TM1637, selectors)
│   │   ├── InputMapping.h
│   │   ├── LEDMapping.h
│   │   ├── DCSBIOSBridgeData.h
│   │   ├── LabelSetConfig.h
│   │   ├── CustomPins.h
│   │   ├── aircraft.txt
│   │   ├── selected_panels.txt
│   │   ├── reset_data.py
│   │   ├── generate_data.py
│   │   └── display_gen.py
│   │
│   └── LABEL_SET_MAIN/             ← Larger test file (PCA, TM1637, many selector groups)
│       └── ...
```

---

## 3. Runtime Environment

- **Platform**: Windows 11 only (uses `msvcrt`, `ctypes.windll`, `os.startfile`)
- **Python**: 3.12+ (uses `str | None` union syntax)
- **Input**: `msvcrt.getwch()` — single-character blocking read, no readline
- **Special key prefixes**: `"\xe0"` or `"\x00"` followed by arrow codes: `H`=Up, `P`=Down, `M`=Right, `K`=Left
- **Terminal**: ANSI escape sequences enabled via `ctypes.windll.kernel32.SetConsoleMode` with `ENABLE_VIRTUAL_TERMINAL_PROCESSING`
- **Encoding**: All file I/O uses `encoding="utf-8"`. Subprocess env includes `PYTHONIOENCODING=utf-8` to handle Unicode (e.g., `✓`) when stdout is redirected to a log file.
- **Single-instance**: Lock file at `label_creator/.label_creator.lock` with PID. Stale locks are reclaimed.

---

## 4. ui.py — The TUI Toolkit

`ui.py` is a **pure presentation layer** with zero CockpitOS domain knowledge. It provides:

### ANSI Constants
```python
CYAN, GREEN, YELLOW, RED, BOLD, DIM, RESET, REV, HIDE_CUR, SHOW_CUR, ERASE_LN
```

### Output Helpers
- `cls()` — clear screen
- `cprint(color, text)` — colored print with Unicode fallback
- `header(text)` — clear + banner header
- `success(text)`, `warn(text)`, `error(text)`, `info(text)` — status prefixed messages
- `big_success(title, details)`, `big_fail(title, details)` — large banners

### Interactive Inputs
| Function | Purpose | Returns |
|---|---|---|
| `pick(prompt, options, default)` | Arrow-key single selector | Selected value or `None` (ESC) |
| `menu_pick(items)` | Full-screen styled menu with separators | Selected value or `None` (ESC) |
| `pick_filterable(prompt, options)` | Type-to-filter picker for long lists | Selected value or `None` |
| `pick_live(prompt, initial, scan_fn)` | Auto-refreshing picker with background scan | Selected value or `None` |
| `text_input(prompt, default, mask)` | Single-line text input | String, `None` (ESC), or `_RESET_SENTINEL` (Ctrl+D) |
| `confirm(prompt)` | Y/N prompt | `True`, `False`, or `None` (ESC) |
| `toggle_pick(prompt, items)` | Checkbox toggle list | `{key: bool}` dict |

### menu_pick Item Format
```python
("label", "value", "style")           # selectable row
("label", "value", "style", "caption") # selectable row with DIM caption suffix
("---",)                               # plain separator line
("---", "Section Name")                # labeled separator (centered with dashes)
```
Styles: `"action"` (green bg), `"danger"` (red bg), `"normal"`, `"dim"`

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
- `🗂️` (`\U0001f5c2\ufe0f`) — label set count
- `🏷️` (`\U0001f3f7\ufe0f`) — last generated label set (from `active_set.h`, NOT `creator_prefs.json`)
- `🛩️` (`\U0001f6e9\ufe0f`) — aircraft of last generated set

**Important**: If `active_set.h` references a label set whose directory no longer exists, the status shows `RED` with `(deleted)` and suppresses the aircraft line.

Menu options (plain text, no emojis on menu items):
1. **Create New Label Set** (action style — green) → `do_create()`
2. **Modify Label Set** (normal) → `do_browse()`
3. **Auto-Generate Label Set** (normal) → `do_generate()`
4. **Delete Label Set** (normal) → `do_delete()`
5. **Exit** (dim)

### Active Label Set Tracking
```python
ACTIVE_SET = LABELS_DIR / "active_set.h"

def _read_active_label_set() -> str | None:
    # Parses: #define LABEL_SET <NAME>
    # Returns the bare name (e.g., "IFEI"), not the full dir name
```
The main menu loop checks `ls_exists = (LABELS_DIR / f"LABEL_SET_{active_ls}").exists()` on every iteration.

### do_browse() → _show_label_set_info()
The browse flow picks a label set via `pick_filterable`, then shows `_show_label_set_info()` which **loops internally** — config edits (device name, HID toggle, custom pins, mapping editors) redraw the screen and loop back. Only "Select Panels", "RESET", and "Back" exit the loop.

### Info Header (inside _show_label_set_info)
Three compact lines — no file sizes, no file paths:
```
       Aircraft:     FA-18C_hornet
       Device Name:  CockpitOS Super Panel
       Panels (2):   Master Arm Panel, Master Caution Light
```

### Action Menu (inside _show_label_set_info)
Ordered by workflow priority: panels → wiring → config → nuclear.
```
─────── Panels ─────────
  Select Panels                  (action style — green)
──────── Wiring ────────
  Edit Inputs                   12/44 wired    ← caption from count_wired()
  Edit LEDs                      8/27 wired
──────── Config ────────
  Device Name                    CockpitOS Super Panel  ← caption shows current value
  HID Mode Selector              OFF                    ← caption shows ON/OFF
  Edit Custom Pins
────────────────────────
  RESET LABEL SET                (danger style)
────────────────────────
  Back                           (dim style)
```

### LabelSetConfig.h Editing
Two editable `#define` fields in `LabelSetConfig.h`:
- `LABEL_SET_FULLNAME "..."` — max 32 chars, edited via `text_input`
- `HAS_HID_MODE_SELECTOR 0|1` — toggled with single press (flip + redraw)

Helpers in `label_set.py`: `read_label_set_config()`, `write_label_set_fullname()`, `write_hid_mode_selector()`

### CustomPins.h
Opened in the user's default editor via `os.startfile()`. No in-app editing.

---

## 6. panels.py — 3-Level Drill-Down Browser

Provides the interactive panel browser used during label set creation/modification.

### TUI Patterns (reused by input_editor.py and led_editor.py)
These patterns are **duplicated locally** in each scrollable-list module:

- **Full-redraw**: `os.system("cls")` + sequential `_w()` calls (no incremental updates)
- **Scroll bar**: `█` thumb / `░` track, proportional sizing
- **Boundary flash**: `_flash_active[0]` flag + `threading.Timer(0.15, restore)` — briefly renders highlighted row in yellow when at boundary
- **Key input**: `msvcrt.getwch()`, special prefix `"\xe0"`/`"\x00"`, arrows `H`/`P`/`M`/`K`
- **Header format**: `=` bar, title + counter right-aligned, `=` bar
- **Context string**: `"(LABEL_SET_NAME, aircraft)"` appended to all titles

### Levels
| Level | Function | Shows | Keys |
|---|---|---|---|
| 1 | `select_panels()` | Toggle-list of panels with [X] checkboxes | Up/Down, Enter=toggle, Right=drill, S=save, Esc=cancel |
| 2 | `_show_detail()` | Scrollable control list (Label, Desc, Type) | Up/Down, Right=drill, Left/Esc=back |
| 3 | `_show_control_detail()` | Individual control inputs/outputs | Up/Down, Left/Esc=back |

All three levels receive and display `label_set_name` and `aircraft_name` in their headers.

---

## 7. input_editor.py — InputMapping.h TUI Editor

### File Structure of InputMapping.h
```
// Comments (4 lines)
#pragma once
struct InputMapping { ... };          ← struct definition (NEVER touch)
// column header comment
static const InputMapping InputMappings[] = {
    { "LABEL", "SOURCE", PORT, BIT, HID, "CMD", VALUE, "TYPE", GROUP },
    ...
};
static const size_t InputMappingSize = ...;
// TrackedSelectorLabels[] ...        ← auto-generated (NEVER touch)
// Static hash lookup table ...       ← auto-generated (NEVER touch)
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

### Scrollable List View
- Header: `Input Mapping  (LABEL_SET_NAME, aircraft)     X / Y wired`
- Columns: Label | Source | Port | Bit | HID | Type
- Colors: `source != "NONE"` → GREEN (wired), `source == "NONE"` → DIM
- Keys: Up/Down=navigate, Right/Enter=edit, Left/Esc=back

### Edit Flows
**Standalone record** (momentary, analog, variable_step, fixed_step, or selector with group=0):
1. Pick source from `INPUT_SOURCES` list
2. If NONE → auto-clear port=0, bit=0, hidId=-1
3. Otherwise → prompt: port (text_input) → bit → hidId

**Selector group** (controlType="selector" AND group > 0):
1. Gather ALL records sharing that group ID
2. Display all positions with current assignments
3. Pick source once for the entire group
4. If NONE → auto-clear all positions
5. Otherwise → prompt shared port, then bit per position (-1 = fallback)
6. Optionally prompt HID IDs per position

### Input Sources
```python
INPUT_SOURCES = [
    ("NONE (not wired)", "NONE"),
    ("GPIO", "GPIO"), ("HC165", "HC165"), ("TM1637", "TM1637"),
    ("PCA_0x20", "PCA_0x20"), ... ("PCA_0x27", "PCA_0x27"),
    ("PCA_0x5A", "PCA_0x5A"), ("PCA_0x5B", "PCA_0x5B"),
]
```

### Re-parse After Write
After every successful edit + save, the editor re-parses from disk:
```python
records, raw_lines = parse_input_mapping(filepath)
```
This keeps `line_index` values and all state consistent.

---

## 8. led_editor.py — LEDMapping.h TUI Editor

### File Structure of LEDMapping.h
```
// Comment
#pragma once
enum LEDDeviceType { ... };           ← enum definition (NEVER touch)
struct LEDMapping { ... };            ← struct definition (NEVER touch)
// Auto-generated panelLEDs array
static const LEDMapping panelLEDs[] = {
  { "LABEL", DEVICE_X, {.infoType = {values}}, dimmable, activeLow },
  ...
};
static constexpr uint16_t panelLEDsCount = ...;
// Auto-generated hash table ...      ← NEVER touch
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

### Device-Specific Edit Forms
| Device | info_type | Fields Prompted |
|---|---|---|
| NONE | gpioInfo | (auto-cleared to defaults) |
| GPIO | gpioInfo | pin |
| PCA9555 | pcaInfo | address, port, bit |
| TM1637 | tm1637Info | clkPin, dioPin, segment, bit |
| GN1640T | gn1640Info | address, column, row |
| WS2812 | ws2812Info | index, pin, defR, defG, defB, defBright |
| GAUGE | gaugeInfo | gpio, minPulse, maxPulse, period |

After device-specific fields: dimmable (true/false picker), activeLow (true/false picker).

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
- TM1637: `"JETT_CLK_PIN, JETT_DIO_PIN, 5, 0"` (clk, dio, seg, bit) — can use named constants!
- GN1640T: `"0, 2, 1"` (address, column, row)
- WS2812: `"0, 5, 0, 0, 0, 255"` (index, pin, R, G, B, brightness)
- GAUGE: `"6, 544, 2400, 20000"` (gpio, minPulse, maxPulse, period)

---

## 9. Critical Rules

### NEVER Modify
1. **Hash tables** (`inputHashTable[]`, `ledHashTable[]`) — auto-generated, indexed by label hash
2. **Struct definitions** (`struct InputMapping`, `struct LEDMapping`, `enum LEDDeviceType`)
3. **TrackedSelectorLabels[]** — auto-generated list of selector DCS labels
4. **Size constants** (`InputMappingSize`, `panelLEDsCount`)
5. **Hash functions** (`findInputByLabel`, `findLED`)
6. **File header comments** (the `// THIS FILE IS AUTO-GENERATED...` block)

### ONLY Modify
- **Individual records** within `InputMappings[]` and `panelLEDs[]` arrays
- Specifically the user-editable fields listed above

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

## 10. Selector Groups (InputMapping)

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

### Editor Behavior
When user presses Edit on ANY position in a selector group:
1. All positions in that group are gathered (`[r for r in records if r["group"] == group]`)
2. All positions displayed together
3. Source picked once (applied to all)
4. Port entered once (applied to all)
5. Bit entered per position individually

---

## 11. Integration Points in label_creator.py

### Imports
```python
import input_editor
import led_editor
```

### Caption Helpers
```python
def _input_mapping_caption(ls_dir: Path) -> str:
    # Returns "12/44 wired" or "not generated"
    fpath = ls_dir / "InputMapping.h"
    if not fpath.exists(): return "not generated"
    wired, total = input_editor.count_wired(str(fpath))
    return f"{wired}/{total} wired"

def _led_mapping_caption(ls_dir: Path) -> str:
    # Same pattern for LEDMapping.h
```

### Menu Integration
In `_show_label_set_info()`:
```python
("---", "Mappings"),
("Edit Input Mapping", "edit_input", "normal", input_cap),
("Edit LED Mapping",   "edit_led",   "normal", led_cap),
```

### Handler
```python
elif choice == "edit_input":
    fpath = ls_dir / "InputMapping.h"
    if not fpath.exists():
        ui.warn("InputMapping.h not found. Generate first.")
        input(...)
    else:
        input_editor.edit_input_mapping(str(fpath), ls_name, ac_name)
    continue   # ← loops back to redraw the info screen
```

---

## 12. Test Files

| File | Why It's Good for Testing |
|---|---|
| `src/LABELS/LABEL_SET_IFEI/InputMapping.h` | Has GPIO with PIN() macro, HC165, TM1637, selector groups (3-position with fallback bit=-1), analog + variable_step pairs, NONE records |
| `src/LABELS/LABEL_SET_IFEI/LEDMapping.h` | Has TM1637 with named constants (JETT_CLK_PIN), GPIO with PIN(), NONE records, dimmable entries |
| `src/LABELS/LABEL_SET_MAIN/InputMapping.h` | Larger file with PCA_0x26, PCA_0x5B, TM1637 with PIN(), many selector groups, GPIO analog |

---

## 13. UI Patterns Quick Reference

### Full-Redraw Scrollable List (used by panels.py, input_editor.py, led_editor.py)
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
_SCROLL_BLOCK = "█"  # \u2588
_SCROLL_LIGHT = "░"  # \u2591
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

---

## 14. Emoji Conventions

Emojis appear ONLY on the main menu **status display lines** (never on menu options). All use the same width class (double-width, supplementary plane + variation selector):

| Emoji | Code | Purpose |
|---|---|---|
| 🗂️ | `\U0001f5c2\ufe0f` | Label set count |
| 🏷️ | `\U0001f3f7\ufe0f` | Last generated label set |
| 🛩️ | `\U0001f6e9\ufe0f` | Aircraft |

The compiler tool uses its own set: 🖥️ (board), 🔗 (transport), 🐛 (debug).

**Rule**: Never mix single-width and double-width emojis in the same display group — it breaks margin alignment.

---

## 15. Known Quirks & Gotchas

1. **generator_core.py exit code**: Always exits with code 1, even on success. The reliable success signal is the `.last_run` timestamp file.

2. **Port formatting in generator**: Uses `{port:>2}` which works for both ints and strings in Python's format spec (right-aligns to min width 2).

3. **LED comma pattern**: The generator uses `",\n".join(final_panel)` to join entries, so non-last entries get a trailing `,` after the comment. The last entry has no trailing `,`. Our write-back detects and preserves this.

4. **InputMapping always has trailing comma**: Unlike LED, the generator writes each InputMapping line with `}},\n` (hardcoded trailing comma on every line).

5. **NONE entries in LEDMapping**: Freshly generated NONE entries don't have a comment or `}},`. But once preserved through a regeneration cycle, they get the full `}}, // No Info` format. Our editor always writes the full format.

6. **text_input returns**: Can return `str` (valid input), `None` (ESC=cancel), or `ui._RESET_SENTINEL` (Ctrl+D=reset). Always check for `None` before using the value.

7. **confirm returns**: Can return `True`, `False`, or `None` (ESC). Not just a boolean!

---

## 16. Future Work / Pending Tasks

1. **CustomPins.h validation** — User mentioned wanting pin validation in the future (e.g., checking for conflicts). Currently just opens in default editor.

2. **Testing the editors** — The editors are newly created and need real-world testing:
   - Edit a standalone momentary record → assign GPIO + pin → verify file
   - Edit a selector group → verify all positions → verify fallback
   - Set a record to NONE → verify defaults
   - Edit LED GPIO/TM1637/PCA9555 → verify device-specific fields
   - Run auto-generate after edits → verify preservation
   - Verify hash tables are NOT modified

3. **Potential improvements** identified during development:
   - Search/filter in mapping editors (like `pick_filterable`)
   - Bulk operations (e.g., "wire all TM1637 buttons on this keypad")
   - Undo support (currently no undo — ESC during edit cancels, but completed edits are immediate)
   - Display of which panel each record belongs to (would require cross-referencing with aircraft JSON)
