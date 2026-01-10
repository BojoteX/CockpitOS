# InputMapping Quick Guide

This file tells the firmware **how each physical control is read** and **which simulator command it triggers**.

Edit **only** the individual records in `InputMappings[]`.  
Do **not** add or remove records; the generator depends on the list length.

---

## Struct Overview

```cpp
struct InputMapping {
  const char* label;       // Unique name for this control position/action
  const char* source;      // Hardware source (e.g., "PCA_0x5B", "HC165", "GPIO", "TM1637", "NONE")
  int8_t     port;         // PCA: 0/1. HC165: usually 0 (ignored). GPIO: use PIN(x); -1 when sharing a GPIO
  int8_t     bit;          // Bit index for PCA & HC165. For GPIO one-hot selectors set to -1
  int8_t     hidId;        // HID usage ID to send (or -1 to disable HID)
  const char* oride_label; // DCS command name (string sent to DCS-BIOS)
  uint16_t    oride_value; // DCS command value (0/1/2… or 65535 for analog passthrough)
  const char* controlType; // "momentary", "selector", "analog", "variable_step", "fixed_step"
  uint16_t    group;       // Exclusive group ID for selectors (0 = no exclusivity)
};
```

> **PIN(x) macro:** you may write `PIN(4)` and the build system auto-maps it to the correct S2/S3 GPIO for your backplane.  
> Example: `PIN(4)` remains 4 on S2, becomes 5 on S3 if that board’s mapping requires it.

---

## Fields Explained (plain language)

- **label**: A human-readable, unique key. Also used in logs and debugging.
- **source**: Where the signal comes from. Common values:
  - `"PCA_0xNN"` — I²C expander PCA9555 at address `0xNN`
  - `"HC165"` — 74HC165 shift-register chain
  - `"GPIO"` — direct ESP32 GPIO
  - `"TM1637"` — TM1637 module (some panels use its keys)
  - `"NONE"` — no physical input; 
- **port**:
  - For **PCA9555**: `0` or `1` (which side of the expander)
  - For **HC165**: typically `0` (kept for consistency; not used by the reader)
  - For **GPIO**: put `PIN(x)`. If a single GPIO is *shared* (e.g., reads HIGH/LOW for two meanings), set `port = PIN(x)` and use `bit = -1` to disambiguate in your module logic.
- **bit**:
  - For **PCA9555/HC165**: the bit index in that port/byte (0..7 or 0..n on chained 165)
  - For **GPIO one-hot selectors**: set to `-1` (each position has its own GPIO)
  - For *center/neutral* positions with **no physical bit**, set to `-1` (see recipes)
- **hidId**: If not `-1`, a HID usage is emitted for PC/game use if enabled
- **oride_label/value**: The DCS-BIOS command name and value to send when this input becomes active.  
  Use `65535` for analog controls to indicate “send raw value”.
- **controlType**:
  - `"momentary"` — press/release (buttons, guarded actions)
  - `"selector"` — fixed position(s). Use the **group** to bind all positions of one switch. Uses exclusivity and time dwelling logic.
  - `"analog"` — continuous potentiometer/axis
  - `"variable_step"` — send 0/1 for decrement/increment (ENC-like behavior)
  - `"fixed_step"` — same as above but fixed increments (one-shot pulses)
- **group**: All entries with the same non-zero group are **mutually exclusive**. Exactly one should be “selected” at a time for that switch. Use `0` for independent buttons.

---

## Common Patterns (recipes)

### 1) Momentary button on PCA9555
```cpp
{ "FIRE_EXT_BTN", "PCA_0x5B", 0, 2, 16, "FIRE_EXT_BTN", 1, "momentary", 0 },
```
- Source is PCA at 0x5B, port 0, bit 2. Sends DCS `FIRE_EXT_BTN 1` and HID usage 16.

### 2) Three‑position selector on HC165 with a **neutral** that has no bit
```cpp
{ "ENGINE_CRANK_SW_LEFT",  "HC165", 0, 11, -1, "ENGINE_CRANK_SW", 2, "selector", 4 },
{ "ENGINE_CRANK_SW_OFF",   "HC165", 0, -1, -1, "ENGINE_CRANK_SW", 1, "selector", 4 }, // no bit: neutral
{ "ENGINE_CRANK_SW_RIGHT", "HC165", 0, 12, -1, "ENGINE_CRANK_SW", 0, "selector", 4 },
```
- Both outer positions have real bits (11 and 12). The **center** has `bit = -1`, so your read logic treats “neither left nor right” as OFF. All three share `group = 4`.

### 3) One‑hot selector on GPIO (each position has its own pin)
```cpp
{ "MODE_SW_POS0", "GPIO", PIN(33), -1, -1, "MODE_SW", 0, "selector", 21 },
{ "MODE_SW_POS1", "GPIO", PIN(21), -1, -1, "MODE_SW", 1, "selector", 21 },
```
- `bit = -1` signals **one‑hot**. The group `21` binds the positions.

### 4) Analog knob on GPIO. Alternatively, you could use inc/dec if you don't want to use a potentiometer but a regular pulse rotary
```cpp
{ "COM_AUX",     "GPIO", PIN(4),  0, -1, "COM_AUX", 65535, "analog",        0 },
{ "COM_AUX_DEC", "NONE", 0,       0, -1, "COM_AUX",     0, "variable_step", 0 },
{ "COM_AUX_INC", "NONE", 0,       0, -1, "COM_AUX",     1, "variable_step", 0 },
```
- The analog row sends raw values (`65535`). Only ONE of the controls should be enabled, either the analog or the variable step, but not both. So its either like the above or:
```cpp
{ "COM_AUX",     "NONE", 0,            0, -1, "COM_AUX", 65535, "analog",        0 },
{ "COM_AUX_DEC", "GPIO", PIN(4),       0, -1, "COM_AUX",     0, "variable_step", 0 },
{ "COM_AUX_INC", "GPIO", PIN(5),       0, -1, "COM_AUX",     1, "variable_step", 0 },
```

### 5) Guarded action (cover + button)
```cpp
{ "LEFT_FIRE_BTN_COVER", "NONE",  0, 0, -1, "LEFT_FIRE_BTN_COVER", 1, "momentary", 0 },
{ "LEFT_FIRE_BTN",       "TM1637", PIN(39), 3, -1, "LEFT_FIRE_BTN", 1, "momentary", 0 },
```
- The cover is a virtual momentary; the button is a real key read from a TM1637 device. Cover logic is handled in Mappings.cpp

---

## Selector Groups (how `group` works)

- Give all positions of the same selector the **same `group` ID** (non-zero).  
- The firmware enforces **mutual exclusivity**: only one can be active at a time.  
- Use `0` for buttons or controls that don’t belong to any selector.

---

## Special Notes

- **Center / Neutral without a bit:** Use `bit = -1` for the neutral item and ensure your reader logic derives “neutral” when no other position in the group is active.
- **One‑hot GPIO selectors:** Each position uses its own GPIO; set `bit = -1`. The group ties them together.
- **Shared GPIO edge cases:** If you intentionally reuse a GPIO for multiple meanings, set `port = PIN(x)` and `bit = -1`, then implement the disambiguation in that panel’s module.
- **HID events:** Leave `hidId = -1` if you don’t want a HID packet for this control. (even if hidId is specified, you still need to compile with HID mode or hybrid enabled) as this is DISABLED by default by the firmware.
- **DCS commands:** `oride_label` is the DCS-BIOS command name; `oride_value` is the payload. Use `65535` for analog passthrough.

---

## Minimal Checklist (when adding a line)

1. Pick a **source** (`"PCA_0xNN"`, `"HC165"`, `"GPIO"`, `"TM1637"`, or `"NONE"`).  
2. Fill **port/bit** according to the source rules above.  
3. Set **controlType**: `"momentary"`, `"selector"`, `"analog"`, `"variable_step"`, or `"fixed_step"`.  
4. Provide **oride_label/value** for the DCS action.  
5. Choose a **group** for selectors (non-zero) or `0` for stand-alone controls.  
6. Optional: set **hidId** if this control should emit a HID event.

Most of the fields will be pre-populated for you by the auto-generator
