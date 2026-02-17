# CockpitOS Auto-Generator Core - LLM Reference Guide

> **Purpose:** Complete technical reference for LLMs working with or reasoning about the CockpitOS auto-generator scripts in `src/LABELS/_core/`. This document maps every function, data flow, preservation mechanism, regex pattern, and hash table strategy so that an LLM can understand, debug, or extend the generators without needing to re-read the full source code.

> **CRITICAL:** The `_core/` scripts are **read-only sacred code**. The README_NOW.txt in that directory says: "DO NOT MODIFY THESE FILES!!" This guide is for understanding only.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [File Inventory](#2-file-inventory)
3. [Invocation Chain](#3-invocation-chain)
4. [generator_core.py - The Main Generator](#4-generator_corepy---the-main-generator)
   - 4.1 [Configuration Constants](#41-configuration-constants)
   - 4.2 [JSON Loading & Panel Filtering](#42-json-loading--panel-filtering)
   - 4.3 [METADATA Merging](#43-metadata-merging)
   - 4.4 [Output Entries (DCS Output Table)](#44-output-entries-dcs-output-table)
   - 4.5 [Selector Entries (Input Command Table)](#45-selector-entries-input-command-table)
   - 4.6 [Writing DCSBIOSBridgeData.h](#46-writing-dcsbiosbridgedatah)
   - 4.7 [Hash Table Strategy](#47-hash-table-strategy)
   - 4.8 [Display Buffers (CT_Display.h/.cpp)](#48-display-buffers-ct_displayhcpp)
   - 4.9 [InputMapping.h - Preservation Logic](#49-inputmappingh---preservation-logic)
   - 4.10 [LEDMapping.h - Preservation Logic](#410-ledmappingh---preservation-logic)
   - 4.11 [LabelSetConfig.h Generation](#411-labelsetconfigh-generation)
   - 4.12 [active_set.h & Post-Generation Steps](#412-active_seth--post-generation-steps)
5. [display_gen_core.py - Display Mapping Generator](#5-display_gen_corepy---display-mapping-generator)
   - 5.1 [Device Prefix System](#51-device-prefix-system)
   - 5.2 [SegmentMap Auto-Detection](#52-segmentmap-auto-detection)
   - 5.3 [fieldDefs[] Preservation](#53-fielddefs-preservation)
   - 5.4 [DisplayMapping.cpp Generation](#54-displaymappingcpp-generation)
   - 5.5 [DisplayMapping.h Generation](#55-displaymappingh-generation)
6. [reset_core.py - The Reset Script](#6-reset_corepy---the-reset-script)
7. [Hash Function Deep Dive](#7-hash-function-deep-dive)
8. [Regex Patterns Reference](#8-regex-patterns-reference)
9. [Preservation Rules Summary](#9-preservation-rules-summary)
10. [Generated File Dependency Graph](#10-generated-file-dependency-graph)
11. [Control Type Taxonomy](#11-control-type-taxonomy)
12. [The .DISABLED Mechanism](#12-the-disabled-mechanism)
13. [Common Pitfalls & Edge Cases](#13-common-pitfalls--edge-cases)
14. [Firmware Integration](#14-firmware-integration--how-generated-data-is-consumed)
15. [Glossary](#15-glossary)

---

## 1. Architecture Overview

The CockpitOS auto-generator is a Python-based code generation system that reads **DCS-BIOS aircraft JSON** files and produces C++ header/source files that the ESP32 firmware compiles. The generators bridge the gap between a flight simulator's data protocol (DCS-BIOS) and physical cockpit panel hardware.

```
_core/aircraft/FA-18C_hornet.json   (centralized aircraft JSONs)
_core/aircraft/AH-64D.json
        |
        v
  aircraft.txt (one-line reference in each LABEL_SET, e.g. "FA-18C_hornet")
        |
        v
+--------------------+     +---------------------+
| selected_panels.txt| --> | generator_core.py   |
+--------------------+     | (main generator)    |
        |                  +---------------------+
        |                          |
        |    +---------+           |  calls
        +--> | METADATA |           v
             | /*.json  |   +---------------------+
             +---------+   | display_gen_core.py |
                           | (display generator) |
                           +---------------------+
                                    |
                                    v
                           +---------------------+
                           | generate_panelkind  |
                           | .py (project root)  |
                           +---------------------+
```

**Outputs from generator_core.py:**
- `DCSBIOSBridgeData.h` - Address tables, control types, hash lookups, command history
- `InputMapping.h` - GPIO/PCA/HC165 input wiring (preserves user edits)
- `LEDMapping.h` - LED/gauge output wiring (preserves user edits)
- `CT_Display.h` / `CT_Display.cpp` - Display character buffers
- `LabelSetConfig.h` - USB PID, names, feature flags
- `active_set.h` - Points firmware to this label set

**Outputs from display_gen_core.py (called by generator_core.py):**
- `DisplayMapping.cpp` - Field definitions with segment maps (preserves user edits)
- `DisplayMapping.h` - Structs, enums, forward declarations

---

## 2. File Inventory

```
src/LABELS/_core/
  generator_core.py      - Main generator (~1450 lines) - THE critical file
  display_gen_core.py    - Display mapping generator (~510 lines)
  reset_core.py          - Data reset/cleanup + aircraft selector (~273 lines)
  aircraft_selector.py   - Interactive arrow-key aircraft picker (~130 lines)
  aircraft/              - Centralized aircraft JSON library
    FA-18C_hornet.json
    AH-64D.json
  README_NOW.txt         - "DO NOT MODIFY THESE FILES!!"
  *.py.bak               - Backup copies
  *.py.OLD               - Legacy versions
  __pycache__/           - Python bytecode cache
```

Each `LABEL_SET_XXX/` directory contains:
- `generate_data.py` -> thin wrapper, calls `generator_core.run()`
- `display_gen.py` -> thin wrapper, calls `display_gen_core.run()`
- `reset_data.py` -> thin wrapper, calls `reset_core.run()`
- `aircraft.txt` -> one-line file referencing the aircraft (e.g. `FA-18C_hornet`)

---

## 3. Invocation Chain

```
User runs:  python generate_data.py  (from inside LABEL_SET_XXX/)
                     |
                     v
    Wrapper sets CWD to LABEL_SET_XXX/ directory
    Wrapper adds ../_core to sys.path
    Wrapper calls generator_core.run()
                     |
                     v
    generator_core.run() does ALL work using CWD as context:
      1. Resolves aircraft JSON via aircraft.txt → _core/aircraft/{name}.json
         (fallback: legacy local JSON → interactive selector)
      2. Merges METADATA/*.json
      3. Reads selected_panels.txt
      4. Generates DCSBIOSBridgeData.h (includes selectorHashTable)
      5. Generates CT_Display.h/.cpp
      6. Generates InputMapping.h (with preservation)
      7. Generates LEDMapping.h (with preservation)
      8. Calls subprocess: python display_gen.py (in CWD)
           -> display_gen_core.run()
              -> Generates DisplayMapping.cpp/.h
      9. Runs .DISABLED cleanup on sibling LABEL_SET dirs
     10. Generates LabelSetConfig.h
     11. Writes ../active_set.h
     12. Calls subprocess: python generate_panelkind.py (from project root)
     13. Writes .last_run timestamp
```

**Batch mode:** When `COCKPITOS_BATCH=1` environment variable is set, all `input()` pauses are skipped. This is used by the compiler tool for automated builds.

---

## 4. generator_core.py - The Main Generator

### 4.1 Configuration Constants

```python
FIXED_STEP_INCDEC_THRESHOLD = 2  # Selectors with more than 3 positions get INC/DEC aliases
PROCESS_ALL = False               # False = use selected_panels.txt; True = ALL panels
OUTPUT_HEADER   = "DCSBIOSBridgeData.h"
INPUT_REFERENCE = "InputMapping.h"
LED_REFERENCE   = "LEDMapping.h"
```

**KNOWN_DEVICES** - The set of valid LED device types:
```python
{"GPIO", "GAUGE", "PCA9555", "TM1637", "GN1640T", "WS2812", "NONE"}
```

**NORMALIZE_TO_SELECTOR** - Control types that should be treated as selectors for input processing:
```python
{'action', 'toggle_switch', 'emergency_parking_brake', 'radio',
 'mission_computer_switch', '3Pos_2Command_Switch_OpenClose',
 'electrically_held_switch'}
```

**control_type_map** - Maps DCS-BIOS control types to C++ ControlType enum values:
```python
{
    'led': 'CT_LED',
    'limited_dial': 'CT_ANALOG',
    'analog_dial': 'CT_ANALOG',
    'analog_gauge': 'CT_GAUGE',
    'selector': 'CT_SELECTOR',
    'fixed_step_dial': 'CT_SELECTOR',
    'toggle_switch': 'CT_SELECTOR',
    'action': 'CT_SELECTOR',
    'emergency_parking_brake': 'CT_SELECTOR',
    'radio': 'CT_SELECTOR',
    'mission_computer_switch': 'CT_SELECTOR',
    'display': 'CT_DISPLAY',
    'metadata': 'CT_METADATA',
    '3Pos_2Command_Switch_OpenClose': 'CT_SELECTOR',
    'electrically_held_switch': 'CT_SELECTOR',
    'variable_step_dial': 'CT_ANALOG',
}
```

### 4.2 JSON Loading & Panel Filtering

**Step 1: Resolve the aircraft JSON via `aircraft.txt`**

Aircraft JSONs are centralized in `_core/aircraft/`. Each LABEL_SET has an `aircraft.txt` file containing one line: the aircraft name (e.g. `FA-18C_hornet`). The resolution chain is:

1. **Primary:** Read `aircraft.txt` from CWD → load `_core/aircraft/{name}.json`
2. **Legacy fallback:** If no `aircraft.txt`, scan CWD for a local `.json` file (backward compat during migration). Prints a warning suggesting `reset_data.py`.
3. **Interactive fallback:** If neither exists, show an arrow-key aircraft picker via `aircraft_selector.py` → write `aircraft.txt` → proceed.

```python
# aircraft.txt resolution (generator_core.py)
if os.path.isfile("aircraft.txt"):
    acft_name = open("aircraft.txt").read().strip()
    JSON_FILE, data = load_aircraft_json(core_dir, acft_name)
```

**Validation** still uses `is_valid_panel_json()` — a dict-of-dicts where top-level keys are panel names and values are dicts of controls:
```python
def is_valid_panel_json(data):
    if not isinstance(data, dict) or not data:
        return False
    for k, v in data.items():
        if not isinstance(v, dict):
            return False
    return True
```

**Step 2: Panel selection**

On first run, `selected_panels.txt` doesn't exist. The script creates a template with all panels commented out and exits. On subsequent runs, it loads both `panels.txt` (auto-regenerated from JSON keys) and `selected_panels.txt` (user-curated). Lines starting with `#` are comments; non-empty, non-comment lines are panel names.

Invalid panel names (in `selected_panels.txt` but not in `panels.txt`) cause an immediate error exit.

### 4.2.1 aircraft_selector.py — Interactive Picker

Self-contained TUI module using `msvcrt` (Windows) + ANSI escape codes (same pattern as `compiler/ui.py`). Provides:

```python
def select_aircraft(core_dir, current=None):
    """Scan _core/aircraft/ for JSONs, show arrow-key picker.
    Pre-highlights 'current' selection if provided. Returns name or None (Esc)."""

def load_aircraft_json(core_dir, aircraft_name):
    """Load JSON from _core/aircraft/{name}.json. Returns (filename, data) or (None, None)."""
```

If only one aircraft JSON exists in `_core/aircraft/`, it auto-selects without showing the picker.

### 4.3 METADATA Merging

```python
def merge_metadata_jsons(data, metadata_dir="METADATA"):
```

Scans `CWD/METADATA/` for `.json` files (sorted alphabetically), loads each, and merges into the main data dict. **Critical rule:** Aircraft JSON takes precedence - existing labels are never overwritten. METADATA can only ADD new controls.

Typical METADATA files:
- `CommonData.json` - Shared telemetry (altitude, heading, speed)
- `MetadataStart.json` - System field at address 0 (`_ACFT_NAME`)
- `MetadataEnd.json` - System counters at high addresses
- `Custom.json` - User-defined custom controls

### 4.4 Output Entries (DCS Output Table)

The script iterates all controls in selected panels and builds `output_entries[]` - a list of dicts representing DCS-BIOS output addresses that the firmware should monitor.

**Standard control path:** For any control whose `control_type` maps to a known `control_type_map` entry, if it has `outputs[]` with `address`, `mask`, `shift_by`:
```python
output_entries.append({
    'label': item.get('identifier', key),
    'addr': out['address'],
    'mask': out['mask'],
    'shift': out['shift_by'],
    'max_value': out.get('max_value', 1),
    'controlType': control_type_map[ctype_raw]
})
```

**Display string patch:** Display controls (control_type == "display") with `address` + `max_length` in outputs are expanded into one entry PER CHARACTER:
```python
for i in range(length):
    output_entries.append({
        'label': label,
        'addr': base_addr + i,
        'mask': 0xFF,
        'shift': 0,
        'max_value': length,
        'controlType': 'CT_DISPLAY'
    })
```
This is because DCS-BIOS transmits display strings one character per address.

### 4.5 Selector Entries (Input Command Table)

This is the most complex logic. It builds `selector_entries[]` - a list of tuples:
```
(full_label, dcs_command, value, control_type, group_id, position_label)
```

**Processing pipeline for each control:**

1. **Type normalization:** Types in `NORMALIZE_TO_SELECTOR` are mapped to `'selector'`.

2. **Skip analog gauges:** `analog_gauge` controls are skipped entirely (they're output-only).

3. **Find max_value:** From `inputs[].interface == 'set_state'` -> `max_value`. For analog knobs (`limited_dial`, `analog_dial`) that only have `variable_step`, `max_value` is forced to 1.

4. **Button detection:** If description contains "button", emit a single `(ident, ident, 1, 'momentary', 0, "PRESS")` entry.

5. **Cover detection:** If identifier contains `_cover`, `cover_`, or `_cover_`, emit as momentary "OPEN".

6. **Momentary last position:** If `api_variant == 'momentary_last_position'`, emit as momentary "PRESS".

7. **Discrete selector splitting** - The label-splitting algorithm:
   - **Slash-split:** If `description` (after first comma) contains `/`, split by `/`, **reverse** the parts. If part count == position count, use those labels and assign a group.
   - **BTN/SW detection:** For 2-position controls whose identifier contains `_btn`, `btn_`, `_sw`, `sw_`, use labels `['PRESS', 'RELEASE']`.
   - **Fallback:** If no label strategy matched, use `POS0, POS1, ... POSN`.

8. **Analog knob path:** `limited_dial`/`analog_dial` get three entries:
   - `(ident, orig_ident, max_val, 'analog', 0, "LEVEL")` - potentiometer
   - `(ident_DEC, orig_ident, 0, 'variable_step', 0, "DEC")` - encoder decrease
   - `(ident_INC, orig_ident, 1, 'variable_step', 0, "INC")` - encoder increase

9. **Fixed-step INC/DEC:** Large selectors (>3 positions) that expose `fixed_step` input get extra entries:
   - `(ident_DEC, orig_ident, 0, 'fixed_step', 0, "DEC")`
   - `(ident_INC, orig_ident, 1, 'fixed_step', 0, "INC")`

**Group ID assignment patch:** After building all selector entries, a second pass ensures all multi-entry selectors with `group == 0` get assigned a unique group ID. This is critical for exclusive-selector logic in the firmware (when one position is active, others in the same group are deactivated).

### 4.6 Writing DCSBIOSBridgeData.h

This file contains everything the firmware needs for DCS-BIOS communication. The generation order is:

1. **Aircraft name defines:**
   ```cpp
   #define DCSBIOS_ACFT_NAME "FA-18C_hornet"
   static constexpr const char* DCSBIOS_AIRCRAFT_NAME = "FA-18C_hornet";
   ```

2. **ControlType enum:** `CT_LED, CT_ANALOG, CT_GAUGE, CT_SELECTOR, CT_DISPLAY, CT_METADATA`

3. **CommandHistoryEntry struct** - For throttling and HID dedup

4. **DcsOutputTable[]** - Flat array of all output entries

5. **dcsAddressTable[]** - Groups outputs by address (multiple outputs can share an address via different masks)

6. **dcsAddressHashTable[]** - Open-addressing hash table: `address % TABLE_SIZE`, linear probing. Sentinel: `addr == 0xFFFF`. Provides O(1) lookup.

7. **SelectorMap[]** - Flat array of all selector entries with group IDs

8. **selectorHashTable[]** - O(1) hash lookup for SelectorMap by composite key `(dcsCommand, value)`. Uses `labelHash(dcsCommand) ^ (value * 7919u)` as hash key. Generated alongside `findSelectorByDcsAndValue()` inline function. Replaces the O(n) linear scan in `onSelectorChange()`.

9. **commandHistory[]** - Unified command tracking table (one entry per unique DCS command)

10. **displayFields[]** - Display field definitions (label, base_addr, length) with hash table

11. **metadataStates[]** - Tracked metadata fields with hash table (if any metadata controls exist)

### 4.7 Hash Table Strategy

All hash tables in the generated code use the **same pattern**:

- **Algorithm:** `label_hash()` - reverse-iteration polynomial hash with uint16_t truncation
- **Table sizing:** `next_prime(entry_count * 2)` - ensures load factor <= 50%
- **Collision resolution:** Open addressing with linear probing
- **Sentinel:** `nullptr` for label pointer (or `0xFFFF` for address tables)
- **Lookup pattern:** Start at `hash % TABLE_SIZE`, probe linearly, return on match or `nullptr`

The Python `label_hash()` function **must produce identical values** to the C++ `constexpr labelHash()` in `CUtils.h`:

```python
def label_hash(s, mod):
    h = 0
    for c in reversed(s):
        h = (ord(c) + 31 * h) & 0xFFFF  # uint16_t truncation at each step
    return h % mod
```

**Raw hash variant** (no modulo, for composite keys):
```python
def label_hash_raw(s):
    """Returns raw uint16_t hash — no modulo applied. Matches C++ labelHash() exactly."""
    h = 0
    for c in reversed(s):
        h = (ord(c) + 31 * h) & 0xFFFF
    return h
```

**Composite key pattern** - Used when lookups need multiple fields:
```
hash = (labelHash(field1) ^ (field2 * 7919u)) % TABLE_SIZE
```
This XOR-with-prime pattern is used in:
- `hidDcsHashTable` - keyed by `(dcsCommand, value)` for HID mapping lookup
- `selectorHashTable` - keyed by `(dcsCommand, value)` for SelectorMap lookup

The `7919` prime multiplier disperses the value component across the hash space, preventing collisions when multiple entries share the same `dcsCommand` but differ in `value`.

There is also a `djb2_hash()` defined but NOT used in current code (legacy).

The `addrHash()` for address tables is simpler: `addr % TABLE_SIZE`.

#### Hash Tables Generated by generator_core.py

| Table | Key | Lookup Function | Used By |
|-------|-----|----------------|---------|
| `dcsAddressHashTable[]` | `address` (uint16_t) | Inline in sniffer loop | DCS-BIOS stream parser |
| `inputHashTable[]` | `label` (string) | `findInputByLabel()` | HIDManager input lookup |
| `cmdHashTable[]` | `label` (string) | `findCmdEntry()` | Command history lookup (HID + DCS) |
| `hidDcsHashTable[]` | `(dcsCommand, value)` | `findHidMappingByDcs()` | HID mapping by DCS command |
| `selectorHashTable[]` | `(dcsCommand, value)` | `findSelectorByDcsAndValue()` | SelectorMap O(1) lookup in `onSelectorChange()` |
| `displayHashTable[]` | `label` (string) | `findDisplayField()` | Display buffer lookup |
| `metadataHashTable[]` | `label` (string) | `findMetadataState()` | Metadata state lookup |

### 4.8 Display Buffers (CT_Display.h/.cpp)

For each display field, the generator creates:
- A character buffer: `char ifei_rpm_l[4] = {};`
- A dirty flag: `bool ifei_rpm_l_dirty = false;`
- A last-value cache: `char last_ifei_rpm_l[4] = {};`

These are grouped into `CT_DisplayBuffers[]` with a hash table for O(1) label-based lookup.

**CT_Display.h** also contains the `renderField()` function declaration and display infrastructure.

**CT_Display.cpp** contains the `renderField()` implementation which:
1. Finds the field definition by label
2. Validates numeric values against min/max bounds
3. Compares new value against `lastValue` cache (memcmp)
4. Only renders if value actually changed
5. Calls `clearFunc` then `renderFunc` on the display driver

### 4.9 InputMapping.h - Preservation Logic

**This is one of the most critical sections.** When the generator runs, users may have already wired their physical buttons by editing `InputMapping.h`. The generator must:
- ADD new entries for new controls
- REMOVE entries for controls no longer in selected panels
- **PRESERVE** user edits (source, port, bit, hidId) for existing entries

**Step 1: Parse existing file with regex**

```python
line_re = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*'        # label (unique key)
    r'"(?P<source>[^"]+)"\s*,\s*'             # source (GPIO, PCA_0x26, etc.)
    r'(?P<port>-?\d+|PIN\(\d+\)|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*'  # port (int, PIN macro, or symbol)
    r'(?P<bit>-?\d+)\s*,\s*'                  # bit position
    r'(?P<hidId>-?\d+)\s*,\s*'                # HID usage ID
    r'"(?P<cmd>[^"]+)"\s*,\s*'                # DCS command (oride_label)
    r'(?P<value>-?\d+)\s*,\s*'                # command value (oride_value)
    r'"(?P<type>[^"]+)"\s*,\s*'               # control type string
    r'(?P<group>\d+)\s*\}\s*,'                # group ID
)
```

The regex captures ALL fields but keying is by `label` (first field). The `port` capture group is flexible - it handles bare integers, `PIN(X)` macros, and symbolic constants.

**Step 2: Build existing_map keyed by label**

```python
existing_map[d["label"]] = {
    "label": d["label"],
    "source": d["source"],
    "port": d["port"],      # stored as string always
    "bit": int(d["bit"]),
    "hidId": int(d["hidId"]),
    "oride_label": d["cmd"],
    "oride_value": int(d["value"]),
    "controlType": d["type"],
    "group": int(d["group"])
}
```

**Step 3: Group preservation/allocation**

Groups must be stable across regenerations. The algorithm:
1. Scan existing entries for max group ID and build `existing_group_by_cmd` map
2. For new entries, `alloc_group()` either reuses the existing group for that DCS command or allocates a fresh unique ID
3. Ungrouped entries (proposed_grp <= 0) stay at group 0

**Step 4: Merge**

For each selector entry from the JSON:
- If `label` exists in `existing_map` -> use the preserved record (user's wiring is kept)
- If new -> create with `source="NONE", port=0, bit=0, hidId=-1` (user must wire later)

**Step 5: Write with alignment**

The output is column-aligned for readability. Also generates:
- `TrackedSelectorLabels[]` - Labels with group > 0 (for panel sync)
- `inputHashTable[]` - O(1) label lookup

### 4.10 LEDMapping.h - Preservation Logic

Similar to InputMapping.h but with a different regex for the union-based struct:

```python
entry_re = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*DEVICE_(?P<device>[A-Z0-9_]+)\s*,'
    r'\s*\{\s*\.(?P<info_type>[a-zA-Z0-9_]+)\s*=\s*\{\s*(?P<info_values>[^}]+)\}\s*\}'
    r'\s*,\s*(?P<dimmable>true|false)\s*,\s*(?P<activeLow>true|false)\s*\}',
    re.MULTILINE
)
```

Captures: `label`, `device` (GPIO/PCA9555/etc), `info_type` (gpioInfo/pcaInfo/etc), `info_values` (comma-separated params), `dimmable`, `activeLow`.

**Merge logic:**
- Labels come from `DcsOutputTable` filtering for `CT_LED`, `CT_GAUGE`, `CT_ANALOG`
- If label exists in previous file -> preserve the entire user-configured record
- If new -> emit with `DEVICE_NONE, {.gpioInfo = {0}}, false, false`
- Unknown device types trigger a warning but are preserved

Also generates the `LEDDeviceType` enum dynamically from `KNOWN_DEVICES` and the hash table.

### 4.11 LabelSetConfig.h Generation

Generates metadata about the label set:

```cpp
#define LABEL_SET_NAME        "CMWS_DISPLAY"
#define HAS_HID_MODE_SELECTOR 0
#define LABEL_SET_FULLNAME    "CockpitOS Panel CMWS_DISPLAY"
#define HAS_CMWS_DISPLAY
#define AUTOGEN_USB_PID       0xC1E5
```

**USB PID generation:** Deterministic from the fullname using SHA-1:
```python
def pid_from_name(name: str) -> int:
    h = hashlib.sha1(name.encode("utf-8")).digest()
    raw16 = int.from_bytes(h[:2], "big") & 0xFFFF
    return PID_MIN + (raw16 % PID_SPACE)  # Range: 0x1000-0xFFFE
```

**Preservation:** If `LabelSetConfig.h` already exists, `LABEL_SET_FULLNAME` and `HAS_HID_MODE_SELECTOR` are preserved from the existing file using `extract_define()`.

### 4.12 active_set.h & Post-Generation Steps

1. **active_set.h** - Written one directory up (`../active_set.h`):
   ```cpp
   // PID:0xC1E5
   #pragma once
   #define LABEL_SET CMWS_DISPLAY
   ```

2. **generate_panelkind.py** - Called via subprocess from project root. Scans `src/Panels/*.cpp` for `PANEL_KIND:` metadata and generates `src/Generated/PanelKind.h` enum.

3. **`.last_run`** - Timestamp file written on completion so the compiler tool can verify generation succeeded.

4. **`.DISABLED` cleanup** - See [Section 12](#12-the-disabled-mechanism).

---

## 5. display_gen_core.py - Display Mapping Generator

Called by `generator_core.py` via subprocess. Generates the display-specific mapping files.

**Configuration:**
```python
PROCESS_ALL = False   # Same as generator_core.py — False = use selected_panels.txt
```

**JSON resolution:** Uses the same `aircraft.txt` → `_core/aircraft/` → legacy fallback → interactive selector chain as `generator_core.py` (Section 4.2).

### 5.1 Device Prefix System

Each display label has a prefix (text before first `_`). For example:
- `IFEI_RPM_L` -> prefix `IFEI`
- `UFC_OPTION_CUEING_1` -> prefix `UFC`

**REAL_DEVICE_PREFIXES** are extracted from the aircraft JSON ONLY (before METADATA merge). This prevents metadata-only display labels from creating phantom device types.

**all_device_prefixes** is the intersection of all display label prefixes with `REAL_DEVICE_PREFIXES`.

### 5.2 SegmentMap Auto-Detection

For each prefix in `prefixes_seen`, the script looks for `{PREFIX}_SegmentMap.h` and parses it for variable declarations:

```python
var_decl_re = re.compile(
    r'const\s+SegmentMap\s+([A-Z0-9_]+)\s*((\[[^\]]+\])*)\s*=', re.MULTILINE
)
```

This captures variable names and their array dimensions. For a label like `IFEI_RPM_L`:
- Looks for `IFEI_RPM_L_MAP` (standard) or `IFEI_RPM_L_LABEL` (for TEXTURE/LABEL types)
- Determines pointer style from dimensions:
  - 2D `[3][7]` -> `&IFEI_RPM_L_MAP[0][0]`, numDigits=3, segsPerDigit=7
  - 1D `[14]` -> `IFEI_RPM_L_MAP`, numDigits=14, segsPerDigit=0
  - Scalar -> `&IFEI_RPM_L_MAP`, 0, 0

### 5.3 fieldDefs[] Preservation

**Reads existing DisplayMapping.cpp** (or .DISABLED version) and extracts preserved entries:

```python
if "const DisplayFieldDefLabel fieldDefs[]" in line:
    inside = True
```

Within the array, extracts each line keyed by the label (first quoted string). On regeneration:
- If label exists in preserved AND has a non-nullptr map -> keep the preserved line verbatim
- If label exists but has `nullptr` map -> re-generate (allows auto-fixing)
- If new label -> generate fresh entry with auto-detected SegmentMap
- If no SegmentMap found -> **skip entirely** (don't emit entry)

### 5.4 DisplayMapping.cpp Generation

Writes `fieldDefs[]` array with entries like:
```cpp
{ "IFEI_RPM_L", &IFEI_RPM_L_MAP[0][0], 3, 7, 0, 0, FIELD_STRING, 0,
  &ifei, DISPLAY_IFEI, renderIFEIDispatcher, nullptr, FIELD_RENDER_7SEG }
```

Also generates:
- `numFieldDefs` and `fieldStates[]` sizing
- `fieldDefHashTable[]` with label-based O(1) lookup
- `findFieldDefByLabel()` function
- `findFieldByLabel()` function (linear scan fallback)

### 5.5 DisplayMapping.h Generation

Writes:
1. `#include "CUtils.h"` (for shared hash)
2. `HAS_{PREFIX}_SEGMENT_MAP` feature defines
3. `#include "{PREFIX}_SegmentMap.h"` for each present prefix
4. `FieldType` enum: `FIELD_LABEL, FIELD_STRING, FIELD_NUMERIC, FIELD_MIXED, FIELD_BARGRAPH`
5. `DisplayDeviceType` enum: `DISPLAY_IFEI, DISPLAY_UFC, ...`
6. `FieldRenderType` enum: 9 render types (7SEG, LABEL, BINGO, BARGRAPH, FUEL, RPM, etc.)
7. `DisplayFieldDefLabel` struct (13 fields including function pointers)
8. `FieldState` struct with 8-byte lastValue cache
9. Extern declarations for device instances and dispatcher functions

---

## 6. reset_core.py - The Reset Script

Interactive cleanup script with integrated aircraft selector. Full flow:

1. **Lists files to delete** (with confirmation required — user must type "yes"):
   ```
   DCSBIOSBridgeData.h, InputMapping.h, LEDMapping.h,
   DisplayMapping.cpp, DisplayMapping.cpp.DISABLED,
   DisplayMapping.h, CT_Display.cpp, CT_Display.cpp.DISABLED,
   CT_Display.h, LabelSetConfig.h, panels.txt, selected_panels.txt
   ```
   Also detects and deletes any legacy local aircraft JSON files (migration cleanup).

2. **Blanks** `CustomPins.h` - Resets to a comprehensive template with all known pin categories commented out (I2C, HC165, WS2812, TM1637, TFT, IFEI, ALR-67, RS485, etc.)

3. **Preserves:**
   - `METADATA/` directory (explicitly noted and preserved)
   - `aircraft.txt` (read before wipe for re-pick default, overwritten after selection)

4. **Clears screen** and presents the interactive aircraft selector:
   - Shows the label set name and explanation text
   - Calls `select_aircraft()` with the previous aircraft pre-highlighted
   - Writes the selection to `aircraft.txt`
   - Displays success message with next-step instructions
   - Waits for `<ENTER>` before closing (so the user sees the result)

**New LABEL_SET creation flow:**
```
1. Copy an existing LABEL_SET_XXX directory
2. Run reset_data.py → wipes all generated files → picks aircraft → writes aircraft.txt
3. Run generate_data.py → creates selected_panels.txt template → exit
4. Edit selected_panels.txt (uncomment desired panels)
5. Run generate_data.py again → generates all files
```

---

## 7. Hash Function Deep Dive

The hash function is the most critical shared algorithm between Python generation and C++ runtime.

**Python implementation (generator-side):**
```python
def label_hash(s, mod):
    h = 0
    for c in reversed(s):
        h = (ord(c) + 31 * h) & 0xFFFF  # uint16_t truncation at each step
    return h % mod
```

**C++ implementation (runtime-side, in CUtils.h):**
```cpp
constexpr uint16_t labelHash(const char* s) {
    // Reverse-iteration polynomial hash
    // Must match Python label_hash() exactly
}
```

**Key properties:**
- Iterates the string in **reverse** (last char first)
- Polynomial with multiplier 31
- Truncates to uint16_t (`& 0xFFFF`) at EACH step (not just at the end)
- Final modulo by table size

**Why reverse iteration?** Reduces collisions for labels that share common prefixes (e.g., `FIRE_EXT_BTN_PRESS` vs `FIRE_EXT_BTN_RELEASE`).

**Table sizing formula:**
```python
TABLE_SIZE = next_prime(entry_count * 2)  # minimum 53
```
This ensures load factor <= 50%, which keeps average probe length close to 1 with linear probing.

---

## 8. Regex Patterns Reference

### InputMapping.h Parser
```python
r'\{\s*"(?P<label>[^"]+)"\s*,\s*'
r'"(?P<source>[^"]+)"\s*,\s*'
r'(?P<port>-?\d+|PIN\(\d+\)|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*'
r'(?P<bit>-?\d+)\s*,\s*'
r'(?P<hidId>-?\d+)\s*,\s*'
r'"(?P<cmd>[^"]+)"\s*,\s*'
r'(?P<value>-?\d+)\s*,\s*'
r'"(?P<type>[^"]+)"\s*,\s*'
r'(?P<group>\d+)\s*\}\s*,'
```
**Port field flexibility:** Matches `-1`, `42`, `PIN(4)`, `SDA_PIN`, etc.

### LEDMapping.h Parser
```python
r'\{\s*"(?P<label>[^"]+)"\s*,\s*DEVICE_(?P<device>[A-Z0-9_]+)\s*,'
r'\s*\{\s*\.(?P<info_type>[a-zA-Z0-9_]+)\s*=\s*\{\s*(?P<info_values>[^}]+)\}\s*\}'
r'\s*,\s*(?P<dimmable>true|false)\s*,\s*(?P<activeLow>true|false)\s*\}'
```
**Union field detection:** Captures designated initializer (`.gpioInfo`, `.pcaInfo`, `.tm1637Info`, etc.)

### DcsOutputTable Parser (for LED label extraction)
```python
r'\{\s*0x[0-9A-Fa-f]+\s*,\s*0x[0-9A-Fa-f]+\s*,\s*\d+\s*,\s*\d+\s*,\s*"([^"]+)"\s*,\s*(CT_\w+)\s*\}'
```
Extracts label and ControlType from previously-generated `DCSBIOSBridgeData.h`.

### DisplayMapping.cpp fieldDefs[] Parser
```python
r'\s*\{\s*"([^"]+)"'  # Extracts label from first quoted string in line
```

### SegmentMap Variable Declaration Parser
```python
r'const\s+SegmentMap\s+([A-Z0-9_]+)\s*((\[[^\]]+\])*)\s*='
```
Captures variable name and array dimension brackets.

### LabelSetConfig.h #define Parser
```python
r'#define\s+{key}\s+"([^"]+)"'        # Quoted value
r'#define\s+{key}\s+([^\s/]+)'        # Unquoted value (fallback)
```

---

## 9. Preservation Rules Summary

| File | Preservation Strategy | Key |
|------|----------------------|-----|
| **InputMapping.h** | Regex parse existing -> merge by label -> preserve source/port/bit/hidId | `label` (first field) |
| **LEDMapping.h** | Regex parse existing -> merge by label -> preserve device/info/dimmable/activeLow | `label` (first field) |
| **DisplayMapping.cpp** | Line-by-line preserve of `fieldDefs[]` entries -> keep if non-nullptr map | `label` (first quoted string) |
| **LabelSetConfig.h** | Extract `LABEL_SET_FULLNAME` and `HAS_HID_MODE_SELECTOR` from existing | `#define` key names |
| **Group IDs** | `existing_group_by_cmd` map -> reuse for same DCS command | DCS command string |
| **USB PID** | Deterministic from FULLNAME via SHA-1 (stable if name unchanged) | FULLNAME string |

**What is NOT preserved (by reset_core.py):**
- `DCSBIOSBridgeData.h` - Deleted on reset, fully regenerated
- `CT_Display.h/.cpp` - Deleted on reset, fully regenerated
- `DisplayMapping.h` - Deleted on reset, fully regenerated
- `panels.txt` - Deleted on reset, regenerated from JSON keys by generator
- `selected_panels.txt` - Deleted on reset (user must re-select panels)
- `active_set.h` - Always overwritten by generator
- Local aircraft JSON files - Deleted on reset (legacy cleanup, now centralized in `_core/aircraft/`)

---

## 10. Generated File Dependency Graph

```
Aircraft JSON + METADATA/*.json
        |
        v
  [generator_core.py]
        |
        +---> DCSBIOSBridgeData.h  (standalone, no includes needed)
        |
        +---> InputMapping.h       (standalone, declares labelHash extern)
        |
        +---> LEDMapping.h         (standalone, declares labelHash extern)
        |
        +---> CT_Display.h         (includes CUtils.h)
        |         |
        |         v
        +---> CT_Display.cpp       (includes Globals.h, CT_Display.h, DisplayMapping.h)
        |
        +---> LabelSetConfig.h     (optionally includes CustomPins.h)
        |
        +---> active_set.h         (standalone, one #define)
        |
        +---> [calls display_gen.py]
                    |
                    +---> DisplayMapping.h  (includes CUtils.h, *_SegmentMap.h)
                    |
                    +---> DisplayMapping.cpp (includes Globals.h, DisplayMapping.h)

  [calls generate_panelkind.py]
        |
        +---> src/Generated/PanelKind.h  (standalone enum)
```

---

## 11. Control Type Taxonomy

### Input Control Types (from selector_entries)

| Type | Description | How Generated |
|------|-------------|---------------|
| `selector` | Multi-position switch | Discrete positions POS0..POSN or slash-split labels |
| `momentary` | Pushbutton (press only) | Buttons, covers, momentary_last_position |
| `analog` | Potentiometer | limited_dial / analog_dial -> LEVEL entry |
| `variable_step` | Encoder step | INC/DEC pair for analog knobs |
| `fixed_step` | Detented rotary | INC/DEC pair for large selectors |

### Output Control Types (from DcsOutputTable)

| Type | C++ Enum | Description |
|------|----------|-------------|
| `led` | `CT_LED` | Single LED on/off |
| `limited_dial` | `CT_ANALOG` | Analog dial (0-65535) |
| `analog_dial` | `CT_ANALOG` | Analog dial (0-65535) |
| `analog_gauge` | `CT_GAUGE` | Servo/stepper gauge |
| `selector` | `CT_SELECTOR` | Multi-position indicator |
| `display` | `CT_DISPLAY` | Character display string |
| `metadata` | `CT_METADATA` | System telemetry |

---

## 12. The .DISABLED Mechanism

CockpitOS can only compile ONE label set at a time. Since Arduino compiles all `.cpp` files in `src/`, the generator uses a rename trick:

**On generation of LABEL_SET_A:**
1. In LABEL_SET_A: If `CT_Display.cpp.DISABLED` exists alongside `CT_Display.cpp`, delete the `.DISABLED` file
2. In ALL other LABEL_SET_B, C, D...: Rename `CT_Display.cpp` -> `CT_Display.cpp.DISABLED` and `DisplayMapping.cpp` -> `DisplayMapping.cpp.DISABLED`

**Only these files are affected:** `CT_Display.cpp` and `DisplayMapping.cpp` (the WHITELIST).

**Safety check:** The script verifies it's running inside a `LABELS/` parent directory before touching sibling directories.

**Preservation awareness:** `display_gen_core.py` checks for both `DisplayMapping.cpp` AND `DisplayMapping.cpp.DISABLED` when loading preserved entries.

---

## 13. Common Pitfalls & Edge Cases

1. **Duplicate labels:** The generator warns but does not deduplicate. Duplicate labels in `DcsOutputTable` will cause hash collisions.

2. **Slash-split reversal:** Labels split by `/` are REVERSED: `"OFF/ON"` becomes `[ON, OFF]` (index 0 = ON, index 1 = OFF). This matches DCS-BIOS value ordering.

3. **Port field as string:** `InputMapping.h` stores port as a string to support `PIN(X)` macros, not just integers.

4. **METADATA precedence:** Aircraft JSON always wins. If a METADATA file defines a label that already exists in the aircraft JSON, the METADATA version is silently ignored.

5. **First-run behavior:** The script exits after creating `selected_panels.txt` template on first run. This is intentional - the user must edit it before generation proceeds.

6. **Display fields without SegmentMap:** If a display label has no matching `*_SegmentMap.h` variable, it is silently skipped in `DisplayMapping.cpp`. This means not all display fields in `DCSBIOSBridgeData.h` will have entries in `DisplayMapping.cpp`.

7. **Hash table MUST match runtime:** If the Python `label_hash()` and C++ `labelHash()` ever diverge, ALL hash-based lookups will fail silently at runtime.

8. **sys.exit(1) at end:** `generator_core.py` ends with `sys.exit(1)` even on success. This is intentional (historical) and does not indicate an error.

9. **PROCESS_ALL mode:** Setting `PROCESS_ALL = True` processes all panels in the JSON regardless of `selected_panels.txt`. This is for dev/testing only and generates very large output files.

10. **Group ID allocation is not idempotent across runs if entries are removed:** If a selector is removed from `selected_panels.txt` and later re-added, it may get a different group ID than before. However, existing groups are always preserved through the `existing_group_by_cmd` mechanism.

---

## 14. Firmware Integration — How Generated Data is Consumed

The auto-generated files are consumed by two core firmware files:

### 14.1 DCSBIOSBridge.cpp — DCS-BIOS Stream Parser

- **`onSelectorChange(label, value)`** — Called when a selector value changes in the DCS-BIOS stream. Uses `findSelectorByDcsAndValue(label, value)` for O(1) hash lookup into `SelectorMap[]` to resolve human-readable position labels (e.g., "NORM", "TEST", "OFF"). Falls back to `"POS %u"` format if no match found.

- **`onLedChange(label, value)`** — Called for LED state changes. Dispatches to LED subscriptions.

- **`onDisplayChange(label, value)`** — Called for display character updates. Routes to display buffer system.

- **`syncCommandHistoryFromInputMapping()`** — Called at mission start. Overwrites auto-generated `commandHistory[]` group IDs with values from `InputMappings[]`. This means the groups in `commandHistory[]` are dead-on-arrival defaults — the runtime values come from `InputMapping.h`.

- **`sendDCSBIOSCommand(label, value)`** — Uses `findCmdEntry(label)` for O(1) command history lookup.

### 14.2 HIDManager.cpp — HID Input Processing

- **`HIDManager_sendReport(label, rawValue)`** — Called on every physical input event (button press, switch toggle, axis move). Uses `findCmdEntry(dcsLabel)` for O(1) lookup into shared `commandHistory[]`. Previously used a linear scan — replaced with the hash lookup.

- **`flushBufferedHidCommands()`** — Processes buffered selector dwell completions. Uses `findInputByLabel()` and `findHidMappingByDcs()` for O(1) lookups.

### 14.3 All Hash Lookups in Hot Paths

Every frequently-called lookup in the firmware is now O(1):

| Function | Lookup | Hot Path? |
|----------|--------|-----------|
| `findCmdEntry()` | commandHistory by label | Yes — every HID input + DCS command |
| `findSelectorByDcsAndValue()` | SelectorMap by (command, value) | Yes — every selector state change from DCS |
| `findInputByLabel()` | InputMappings by label | Yes — every input event |
| `findHidMappingByDcs()` | InputMappings by (dcsCommand, value) | Yes — dwell flush |
| `findFieldDefByLabel()` | Display fieldDefs by label | Yes — every display update |
| `findDisplayField()` | Display buffers by label | Yes — every display character |

---

## 15. Glossary

| Term | Definition |
|------|-----------|
| **Label Set** | A self-contained panel configuration directory (`LABEL_SET_XXX/`) containing all config and generated files for one physical panel |
| **Aircraft JSON** | DCS-BIOS protocol definition for an aircraft, exported as JSON. Contains all panels, controls, addresses |
| **Panel** | A logical grouping of controls in the aircraft JSON (e.g., "Master Arm Panel", "Fire Systems") |
| **Control** | A single cockpit element (switch, LED, display, gauge) with inputs, outputs, and metadata |
| **DCS-BIOS** | Protocol for communicating between DCS World flight simulator and external hardware |
| **Selector Entry** | A tuple representing one position/action of a physical input device |
| **Output Entry** | A dict representing one DCS-BIOS address/mask that the firmware monitors |
| **Preservation** | The mechanism by which user-edited wiring configurations survive regeneration |
| **Group ID** | Identifies mutually-exclusive selector positions (only one active at a time in a group) |
| **SegmentMap** | Hardware mapping struct that tells the firmware which physical segments to drive for a display character |
| **Device Prefix** | First part of a display label before `_` (e.g., `IFEI` from `IFEI_RPM_L`), identifies the display hardware class |
| **METADATA** | Optional JSON overlay directory that adds controls not in the aircraft JSON |
| **PID** | USB Product ID, deterministically derived from the label set fullname |
| **CWD** | Current Working Directory - the `_core` scripts rely on CWD being set to the `LABEL_SET_XXX/` directory |
| **BATCH mode** | Non-interactive mode (`COCKPITOS_BATCH=1`) used by the compiler tool |
| **.DISABLED** | File extension suffix used to prevent Arduino IDE from compiling `.cpp` files in inactive label sets |
| **aircraft.txt** | One-line text file in each LABEL_SET referencing the aircraft name (e.g. `FA-18C_hornet`). Points to `_core/aircraft/{name}.json` |
| **aircraft_selector.py** | Standalone TUI module with arrow-key picker for selecting aircraft. Uses `msvcrt` + ANSI escapes |
| **Composite hash key** | Hash key formed by XOR-ing `labelHash(field1)` with `(field2 * 7919u)`. Used for multi-field lookups (SelectorMap, HID mapping) |
| **label_hash_raw()** | Python function returning raw uint16_t hash with no modulo. Used for composite keys where modulo is applied after XOR |
