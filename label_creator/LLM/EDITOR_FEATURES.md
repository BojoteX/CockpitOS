# Label Creator — Editor Features & UX Patterns

> **Audience**: LLMs continuing development of this tool.
> **Last updated**: February 2026 (after MATRIX support, PCA validation,
> field guidance, contextual menus, terminal-responsive columns, and cursor
> restoration were implemented).
>
> This document covers knowledge **not present** in `LLM_GUIDE.md` or
> `ARCHITECTURE.md`. Read those first.

---

## 1. MATRIX Source — Strobe/Data Rotary Scanning

MATRIX is a selector-only input source for multi-position rotary switches
that use GPIO strobe/data matrix scanning. It is **not valid for
momentaries, encoders, or analogs**.

### Firmware Behaviour (InputControl.cpp, lines 700-797)

The firmware has three MATRIX functions built at startup:

1. **`Matrix_buildOnce()`** — scans `inputTable[]` for all `source == MATRIX`
   entries. Groups them by data pin (`port` of the anchor row). Builds a
   per-matrix struct with the data pin, strobe pin array, and bit patterns.

2. **`Matrix_poll()`** — called every loop iteration. For each matrix, calls
   `_scanPattern()` then compares with previous state.

3. **`_scanPattern(dataPin, strobePins[], N)`** — pulls each strobe LOW one
   at a time, reads the data pin, builds a binary pattern word. The pattern
   is then matched against the stored bit values in `inputTable[]`.

### How Positions Are Encoded

Each position in a MATRIX selector has a role:

| Role | port | bit | Meaning |
|---|---|---|---|
| **Anchor** | data GPIO pin | `(1 << N) - 1` (all-ones mask) | Resting state: all strobes connect to data. N = number of strobe pins. |
| **Strobe** | strobe GPIO pin | `1 << strobe_index` | One strobe per wired position. Index is 0-based. |
| **Fallback** | data GPIO pin | `-1` | Fires when scanned pattern matches no position. Optional. |

### Real Example (ALR67 — 5-position rotary, 4 strobes)

From `LABEL_SET_ALR67/InputMapping.h`:

```
Position F (anchor):   source=MATRIX, port=38, bit=15   (0b1111)
Position U (strobe0):  source=MATRIX, port=16, bit=1    (0b0001)
Position A (strobe1):  source=MATRIX, port=17, bit=2    (0b0010)
Position I (strobe2):  source=MATRIX, port=21, bit=4    (0b0100)
Position N (strobe3):  source=MATRIX, port=37, bit=8    (0b1000)
```

Data pin = GPIO 38. Four strobe pins (16, 17, 21, 37). Anchor mask =
`(1 << 4) - 1 = 15`. Each strobe bit is `1 << index`.

### Editor Flow (`_edit_selector_matrix()`)

When a selector group's source is set to MATRIX:

1. **Data pin** — prompt for shared data GPIO pin (auto-detected from
   existing anchor/fallback records).
2. **Fallback** — pick which position (if any) is the fallback (`bit = -1`).
3. **Anchor** — pick which position is the anchor (resting state).
4. **Strobe pins** — prompt a GPIO pin for each remaining wired position.
5. **Auto-compute bits** — anchor gets `(1 << N) - 1`, each strobe gets
   `1 << index`. No manual bit entry.
6. **Summary** — display all positions with computed values.
7. **Optional HIDs** — prompt HID IDs per position.

### Source Lists

MATRIX is in `SELECTOR_SOURCES` only:

```python
SELECTOR_SOURCES = [
    ("NONE (not wired)", "NONE"),
    ("GPIO",             "GPIO"),
    ("HC165",            "HC165"),
    ("PCA9555",          "PCA9555"),
    ("MATRIX",           "MATRIX"),
]
```

It is **not** in `INPUT_SOURCES` or `MOMENTARY_SOURCES`. The
`_edit_selector_group()` dispatch uses:

```python
elif src == "MATRIX":
    return _edit_selector_matrix(group_records, cmd, src)
```

### Source-to-Control-Type Validity Map

| Source | Selectors | Momentaries | Analogs | Encoders |
|---|---|---|---|---|
| GPIO | yes | yes | yes | yes |
| PCA9555 | yes | yes | no | no |
| HC165 | yes | yes | no | no |
| TM1637 | no | yes | no | no |
| MATRIX | yes | no | no | no |

---

## 2. PCA Address Validation — Strict Hex Enforcement

### The Problem

Both editors previously accepted garbage PCA addresses:
- Input editor accepted `0.21`, `0xH1` — the old validation was too loose
- LED editor had **zero validation** — raw `ui.text_input` straight to record
  (accepted `0xy6`, any string at all)

### The Fix: Strict `[0-9A-Fa-f]{2}` Regex

Both editors now enforce: strip known prefixes, then require **exactly two
hex digits** matching `^[0-9A-Fa-f]{2}$`. Address `0x00` is explicitly
rejected (firmware skips it).

**Input editor** — `_prompt_pca_address()`:
- Strips prefixes: `PCA_0X`, `PCA_`, `0X`, or bare
- Returns full source string `"PCA_0xHH"` (e.g. `"PCA_0x20"`)
- Validation loop: keeps prompting until valid or ESC

**LED editor** — `_prompt_pca_hex_address()`:
- Strips prefix: `0X` or bare
- Returns raw hex literal `"0xHH"` (e.g. `"0x20"`) — LED format uses
  raw hex, not `PCA_0xHH`
- Module-level compiled regex: `_HEX2 = re.compile(r'^[0-9A-Fa-f]{2}$')`

### Validation Test Cases (both pass)

| Input | Result |
|---|---|
| `0x20` | valid → `0x20` |
| `20` | valid → `0x20` |
| `0x5B` | valid → `0x5B` |
| `PCA_0x20` | valid (input editor strips PCA_ prefix) |
| `0.21` | **rejected** — dot is not hex |
| `0xH1` | **rejected** — H is not hex |
| `0xy6` | **rejected** — y is not hex |
| `0x0` | **rejected** — only one digit |
| `0x123` | **rejected** — three digits |
| `0x00` | **rejected** — firmware skips 0x00 |
| `ff` | valid → `0xFF` |
| ` 0x20 ` | valid (whitespace stripped) |

---

## 3. LED Editor Field Guidance

Every LED device type now shows contextual explanation text (`ui.info`
blocks) before prompting for its fields. This was added because bare
prompts like "Address:", "Column:", "Row:" gave zero context.

### GPIO

```
GPIO LED — direct pin on the ESP32.
Uses analogWrite (PWM) if dimmable, digitalWrite otherwise.
Must be a PWM-capable pin if dimmable is enabled.
```

### PCA9555

```
PCA9555 LED — I2C GPIO expander, 16 I/O in two 8-bit ports.
Each LED is identified by: chip address + port (bank) + bit.
Port selects which 8-bit bank on the chip:
  0 = Port A (pins IO0_0..IO0_7)    1 = Port B (pins IO1_0..IO1_7)
Bit position within the port (0-7), one per physical pin.
```

### TM1637

```
TM1637 LED — LED/keypad driver chip (bit-banged via CLK+DIO).
Has a 6x8 LED matrix: 6 grid positions (segments) x 8 bits each.
Multiple chips can share the same CLK pin but need unique DIO pins.
Segment = grid position on the chip (0-5).
Bit = which LED within that segment (0-7).
```

### GN1640T

```
GN1640T LED — 8x8 LED matrix driver chip.
Each LED is addressed by column (0-7) and row (0-7).
Single-instance driver, address is reserved for future use.
```

### WS2812

```
WS2812 LED — addressable RGB (NeoPixel) on a data strip.
Each LED has an index on the strip and a default color (R,G,B).
LEDs on the same GPIO pin belong to the same strip.
Default color when LED is ON (0-255 each).
When OFF the LED is black. Brightness scales the color.
```

### GAUGE

```
GAUGE — analog servo/gauge driven by PWM pulse.
Firmware maps DCS value (0-65535) to a pulse width between
min and max microseconds at the given period (typically 50Hz).
Pulse range in microseconds — defines servo sweep.
Typical: min=544, max=2400. Adjust per your servo specs.
Period in microseconds. 20000 = 50Hz (standard RC servo).
```

### Dimmable / Active Low (all device types)

```
Dimmable: if true, LED intensity follows the panel dimmer (PWM).
If false, LED is fully ON or fully OFF.

Active Low: if true, the LED is ON when the signal is LOW.
Most common-cathode LEDs use active HIGH (false).
```

---

## 4. Terminal-Width-Responsive Column Layout

Both editors now compute column widths dynamically based on
`os.get_terminal_size().columns` instead of using hardcoded widths.

### Input Editor

Fixed-width columns: Source(10), Port(10), Bit(4), HID(4), Type(15).
The **Label** column gets all remaining space:

```python
fixed   = 3 + (src_w + port_w + bit_w + hid_w + type_w) + 12 + 2  # = 60
label_w = cols - fixed
label_w = max(label_w, 10)
```

At 120 cols: label_w = 60. At 80 cols: label_w = 20.

### LED Editor

Fixed-width columns: Device(8), Dim(5), ALow(5).
Remaining space is split 40/60 between **Label** and **Info**:

```python
fixed_cols = 3 + (dev_w + dim_w + alow_w) + 8 + 2  # = 31
remaining  = cols - fixed_cols
label_w    = max(remaining * 2 // 5, 10)
info_w     = max(remaining - label_w, 15)
```

At 120 cols: label_w=35, info_w=54. At 80 cols: label_w=19, info_w=30.

### Why This Matters

Previously both editors had hardcoded max label widths (35 for input, 30
for LED) that left 30-40 columns of wasted space on wide terminals. DCS
label names like `COCKKPIT_LIGHT_MODE_SW_DAY` are long and were getting
truncated unnecessarily.

---

## 5. Contextual Modify Screen (`_show_label_set_info`)

The modify screen (reached after create or via browse) adapts its menu
based on the label set's current state.

### Info Header Order

Always shown, top to bottom:

```
Device Name:  CockpitOS Panel v5
Aircraft:     FA-18C_hornet
Panels (4):   Cockpit Altimeter, Comms frequency, ...
```

Device Name is always visible (even before generation) because it comes
from `LabelSetConfig.h` which is created during the initial template copy.

### State Detection

```python
is_generated = (ls_dir / "InputMapping.h").exists() or
               (ls_dir / "LEDMapping.h").exists()
has_panels   = bool(_read_selected_panels(ls_dir))
```

### Menu Variants

**Not generated, no panels:**

```
  Start by selecting which aircraft panels
  you want to wire to this device.

  ── Setup ──────────────
  Select which aircraft panels to include
  ─────────────────────
  RESET LABEL SET
  ─────────────────────
  Back
```

Guidance text appears explaining what to do first. Auto-Generate is hidden.

**Not generated, panels selected:**

```
  ── Setup ──────────────
  Select / deselect panels
  Auto-Generate
  ─────────────────────
  RESET LABEL SET
  ─────────────────────
  Back
```

Auto-Generate appears only when `has_panels` is True.

**Generated (full menu):**

```
  ── Panels ─────────────
  Select / deselect panels        (or "Select which..." if none)
  Auto-Generate                   (only if has_panels)
  ── Wiring ─────────────
  Edit Inputs                     17/41 wired
  Edit LEDs                       38/62 wired
  ── Config ─────────────
  Device Name                     CockpitOS Panel v5
  HID Mode Selector               OFF
  Edit Custom Pins
  ─────────────────────
  RESET LABEL SET
  ─────────────────────
  Back
```

### Panel Label Is Contextual

```python
panels_label = ("Select / deselect panels"
                if has_panels
                else "Select which aircraft panels to include")
```

- **No panels yet**: `"Select which aircraft panels to include"` — tells
  the user what to do.
- **Panels already selected**: `"Select / deselect panels"` — tells the
  user they can modify their existing selection, not that they need to
  start over.

### Auto-Generate From Modify Screen

`_auto_generate(ls_name, ls_dir, prefs)` runs the generation pipeline
inline (same console window) rather than opening a separate console:

1. Validates aircraft is assigned
2. Validates panels are selected
3. Calls `step_generate(bare, ls_dir, same_window=True)`
4. Shows `big_success` on completion
5. Updates prefs with last label set

---

## 6. Cursor Position Restoration (`menu_pick` initial parameter)

### The Problem

When toggling HID Mode Selector or editing Device Name, the screen redraws
(via `continue` in the while loop) and the cursor jumped back to the first
menu item ("Select / deselect panels"). This was disorienting — the user
expected to stay on the item they just interacted with.

### Solution: `initial` Parameter on `menu_pick()`

```python
def menu_pick(items, initial=None):
```

The `initial` parameter accepts a value string. When the menu renders, it
searches selectable items for one whose value matches `initial` and starts
the cursor there. Falls back to the first item if not found.

Resolution code:

```python
sel = 0
if initial is not None:
    for si, ri in enumerate(selectable):
        entry = rows[ri]
        if isinstance(entry, tuple) and len(entry) == 3 and entry[2] == initial:
            sel = si
            break
```

### Usage in label_creator.py

```python
last_choice = None
while True:
    # ... build menu ...
    choice = ui.menu_pick(menu_items, initial=last_choice)
    last_choice = choice    # remember for next iteration
    # ... handle choice ...
```

Now: toggle HID → screen redraws → cursor stays on "HID Mode Selector".

---

## 7. Navigation Flow — Create to Modify

### The Problem

After creating a new label set, `do_create()` returned a boolean and the
main loop dumped the user back to the main menu. Users expected to land
in the modify screen for the label set they just created.

### The Fix

`do_create()` now returns `str | None`:
- Returns the full `LABEL_SET_xxx` name on success
- Returns `None` only for true abort (no name entered, no aircraft selected)

**Critical**: once the label set directory and aircraft are established,
`do_create()` always returns the name — even if the user cancels panel
selection. The directory exists, the aircraft is set, so the modify screen
is the right place to be.

In `main()`:

```python
result = do_create(prefs)
if result:
    # Enter modify screen for the newly created label set
    while True:
        action = _show_label_set_info(result, prefs)
        if action == "regenerate":
            # ... handle panel re-selection + regeneration ...
            continue
        elif action == "reset":
            # ... handle reset ...
            continue
        else:
            break  # back to main menu
```

---

## 8. Help System Content (Input Editor)

The input editor has two help screens accessible via `H` key:

### Main List Help (`_HELP_MAIN_LIST`)

Explains each column in the scrollable list:

- **Label**: DCS-BIOS identifier (read-only, set by generator)
- **Source**: Input driver — GPIO, HC165, PCA9555, TM1637, MATRIX, or NONE
- **Port**: Pin or address (meaning depends on source):
  - GPIO = physical pin number (or `PIN(n)` macro)
  - PCA = port bank (0 or 1)
  - HC165 = always 0
  - TM1637 = DIO pin
  - MATRIX = data or strobe GPIO pin
- **Bit**: Bit position or mode flag (meaning depends on source):
  - GPIO = 0 (unused)
  - PCA = bit within port (0-7)
  - HC165 = position in chain (0-63)
  - TM1637 = key index
  - MATRIX = strobe pattern (auto-computed), -1 = fallback
- **HID**: USB HID report ID (-1 = disabled)
- **Type**: Control type (read-only)

Also lists all source types with brief descriptions, including:

```
MATRIX   Strobe/data rotary scanning. For multi-position selectors
         using a GPIO matrix (shared data pin + strobe pins).
         Selectors only — not valid for buttons or encoders.
```

### Selector Group Help (`_HELP_SELECTOR_GROUP`)

Explains how selector groups work, then has per-source sections:

- **GPIO**: one pin per position, firmware reads which pin is active
- **HC165**: one bit per position in shift register chain
- **PCA9555**: one bit per position on I2C expander port
- **MATRIX**: full explanation of strobe/data scanning with the ALR67
  example showing anchor, strobe, and fallback positions

---

## 9. Design Principles (Learned Through Iteration)

These patterns emerged from user feedback during development:

### Every prompt needs context

Never show bare field labels like "Address:" or "Column:". Always explain
what the value means, why it's needed, and what valid ranges are. Use
`ui.info()` with `DIM` color for guidance text before the prompt.

### Validate everything strictly

If a field should be hex, enforce hex. If it should be two digits, enforce
two digits. Don't try to "be helpful" by accepting loose formats — it
causes downstream bugs and confuses users when garbage values appear in
generated files.

### Menus should be contextual

Don't show "Edit Inputs" when there's no InputMapping.h. Don't show
"Auto-Generate" when there are no panels. The menu should reflect what's
actually possible right now.

### Cursor should stay put

When an action redraws the screen (like toggling a boolean), the cursor
must return to the same menu item. Use `menu_pick(initial=last_choice)`.

### Navigation should keep context

After creating a label set, don't dump back to the main menu. Stay in the
modify screen for that label set. After cancelling a sub-step that doesn't
invalidate previous steps, stay where you are.

### Labels should describe the action, not the noun

"Select Panels" is vague. "Select / deselect panels" (when panels exist)
or "Select which aircraft panels to include" (when none exist) tells the
user exactly what will happen.

### Terminal width should be used

Tables should expand to fill the terminal. Don't waste 40 columns of space
on wide terminals by hardcoding column widths. Use `os.get_terminal_size()`
and give the label column all remaining space.
