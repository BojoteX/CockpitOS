# Control Types Reference

Every input mapping in CockpitOS has a **control type** that tells the firmware how to read the hardware and what kind of DCS-BIOS command to send. This document covers each control type, how it works, and how to configure it in `InputMapping.h`.

---

## Overview

| Control Type | Hardware | DCS-BIOS Interface | Description |
|-------------|----------|-------------------|-------------|
| `"momentary"` | Push button, toggle | `set_state` | Sends a value on press, resets on release |
| `"selector"` | Multi-position switch | `set_state` | Each position is a separate mapping row, grouped together |
| `"analog"` | Potentiometer, slider | `set_state` | Continuous value from 0 to 65535 |
| `"variable_step"` | Rotary encoder | `variable_step` | Sends increment or decrement per detent |
| `"fixed_step"` | Rotary encoder | `fixed_step` | Sends a fixed step value per click |

---

## InputMapping.h Structure

Every input is a row in the `InputMappings[]` array. Here is the structure:

```cpp
struct InputMapping {
    const char* label;        // Unique label (auto-generated)
    const char* source;       // Hardware source: "GPIO", "PCA_0x26", "HC165", "TM1637", "MATRIX", "NONE"
    int8_t      port;         // Pin number, PCA port (0 or 1), or -1
    int8_t      bit;          // Bit position (PCA/HC165) or -1 for GPIO selectors
    int8_t      hidId;        // HID gamepad button number (-1 = none)
    const char* oride_label;  // DCS-BIOS command name
    uint16_t    oride_value;  // Override command value
    const char* controlType;  // "momentary", "selector", "analog", "variable_step", "fixed_step"
    uint16_t    group;        // Selector group number (0 = not grouped)
};
```

The Label Creator generates and manages these entries. You can also edit them with a text editor, but the Label Creator is recommended.

---

## Momentary

A **momentary** control sends a command when pressed and (optionally) a different command when released. This is the most common control type for push buttons.

### Behavior

1. Button pressed (GPIO goes LOW) -- CockpitOS sends the DCS-BIOS command with the configured `oride_value`.
2. Button released (GPIO goes HIGH) -- CockpitOS sends the same command with value `0`.

### Wiring

- One leg of the button to the GPIO pin
- Other leg to GND
- CockpitOS enables the internal pull-up resistor automatically

### Example

```cpp
//  label                  source  port bit hidId  DCSCommand                value   Type          group
{ "MASTER_CAUTION_RESET_SW", "GPIO", 5,  0,  -1, "MASTER_CAUTION_RESET_SW",   1, "momentary",     0 },
```

This maps a push button on GPIO 5 to the Master Caution Reset. When pressed, it sends `MASTER_CAUTION_RESET_SW 1\n` to DCS-BIOS. When released, it sends `MASTER_CAUTION_RESET_SW 0\n`.

### Notes

- The `group` field is `0` for momentary controls (no grouping needed).
- Toggle switches (on/off) can also use `"momentary"` when they only have two states and you want CockpitOS to send the current state.

---

## Selector

A **selector** represents a multi-position switch (2-position toggle, 3-position rocker, rotary selector, etc.). Each physical position of the switch is a separate row in `InputMapping.h`, and all positions share the same **group number** and the same **DCS-BIOS command name**.

### Behavior

1. CockpitOS reads all positions in the same group.
2. The active position (the one where the GPIO/PCA/HC165 pin reads LOW) determines which `oride_value` to send.
3. CockpitOS sends the DCS-BIOS command with the active position's value.
4. Dwell-time filtering (`SELECTOR_DWELL_MS`) prevents bouncing between positions during transitions.

### How Selector Groups Work

```
Physical 3-position switch
+-------+-------+-------+
| POS 0 | POS 1 | POS 2 |
+-------+-------+-------+
    |       |       |
  GPIO 6  GPIO 7  GPIO 8     (or PCA bits, HC165 bits, etc.)
    |       |       |
    +-------+-------+
            |
        Common --> GND
```

All three positions share:
- The same DCS-BIOS command name (`oride_label`)
- The same non-zero `group` number
- Different `oride_value` values (0, 1, 2)

### Example: 3-Position Switch

```cpp
//  label                         source  port bit hidId  DCSCommand                value  Type       group
{ "COCKKPIT_LIGHT_MODE_SW_DAY",  "NONE",   0,  0,  -1, "COCKKPIT_LIGHT_MODE_SW",    0, "selector",   4 },
{ "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE",   0,  0,  -1, "COCKKPIT_LIGHT_MODE_SW",    1, "selector",   4 },
{ "COCKKPIT_LIGHT_MODE_SW_NVG",  "NONE",   0,  0,  -1, "COCKKPIT_LIGHT_MODE_SW",    2, "selector",   4 },
```

All three rows have `group=4` and share the command `COCKKPIT_LIGHT_MODE_SW`. The `oride_value` differs: 0 for DAY, 1 for NITE, 2 for NVG.

In this example, the Source is `"NONE"` (not yet wired). To wire it, you would set each row's Source to `GPIO`, `PCA_0xNN`, or `HC165` and assign the appropriate port/bit.

### Example: 2-Position Toggle (PCA9555)

```cpp
//  label                  source      port bit hidId  DCSCommand       value  Type       group
{ "MASTER_ARM_SW_SAFE",  "PCA_0x5B",   0,   3,   18, "MASTER_ARM_SW",   0, "selector",   8 },
{ "MASTER_ARM_SW_ARM",   "PCA_0x5B",   0,  -1,   17, "MASTER_ARM_SW",   1, "selector",   8 },
```

Both rows share `group=8` and command `MASTER_ARM_SW`. The SAFE position reads PCA port 0, bit 3. The ARM position uses bit `-1`, which means it is the default/inverse position (active when no other position in the group is active).

### Key Rules for Selectors

1. **Same group number.** All positions of one physical switch must share the same non-zero `group` value.
2. **Same command name.** All positions must have the same `oride_label` (DCS-BIOS command).
3. **Different values.** Each position must have a unique `oride_value` (0, 1, 2, ...).
4. **Group 0 is reserved.** A `group` of `0` means "not part of a selector group" (used by momentary, analog, and encoder controls).
5. **Default position.** A `bit` value of `-1` indicates the default position -- the one that is active when no other position in the group is reading LOW.

---

## Analog

An **analog** control reads a continuous value from a potentiometer, slider, or other analog input and maps it to the DCS-BIOS value range (0-65535).

### Behavior

1. CockpitOS reads the raw ADC value (0-4095 on ESP32).
2. The value is filtered, deadzone-corrected, and scaled to 0-65535.
3. CockpitOS sends the DCS-BIOS command with the scaled value.
4. Throttling (`VALUE_THROTTLE_MS`) prevents flooding DCS-BIOS with rapid updates.

### Wiring

- Potentiometer outer pins: one to 3.3V, one to GND
- Center wiper pin to an ADC-capable GPIO
- **Never connect 5V** to an ESP32 GPIO

### Example

```cpp
//  label              source  port     bit hidId  DCSCommand       value   Type      group
{ "HMD_OFF_BRT",      "GPIO", PIN(18),  0,  -1, "HMD_OFF_BRT",   65535, "analog",    0 },
```

The `oride_value` of `65535` for analog controls indicates the maximum range. CockpitOS automatically scales the ADC reading.

### Filtering

Analog inputs use multiple filtering stages controlled by `Config.h` settings:

| Setting | Default | Effect |
|---------|---------|--------|
| `MIDDLE_AXIS_THRESHOLD` | `64` | Center snap deadzone |
| `UPPER_AXIS_THRESHOLD` | `128` | Top snap zone |
| `LOWER_AXIS_THRESHOLD` | `256` | Bottom snap zone |
| `CENTER_DEADZONE_INNER` | `256` | Center entry threshold |
| `CENTER_DEADZONE_OUTER` | `384` | Center exit threshold (hysteresis) |
| `AX_DEFAULT_MIN` | `768` | Starting min assumption (auto-learns) |
| `AX_DEFAULT_MAX` | `3327` | Starting max assumption (auto-learns) |
| `SKIP_ANALOG_FILTERING` | `0` | Bypass all filtering (HID mode only) |

The min/max values auto-learn as you move the potentiometer and are saved to NVS (non-volatile storage).

---

## Variable Step

A **variable_step** control is used for rotary encoders that send relative increment/decrement signals per detent.

### Behavior

1. CockpitOS detects the encoder rotation direction.
2. For clockwise rotation: sends the DCS-BIOS command with argument `+3200` (increment).
3. For counter-clockwise rotation: sends the DCS-BIOS command with argument `-3200` (decrement).

The `oride_value` field (0 for the decrement row, 1 for the increment row) identifies the rotation direction internally; CockpitOS translates this to the actual step argument.

### Example

Encoders typically have two rows in `InputMapping.h` -- one for decrement and one for increment:

```cpp
//  label                  source  port bit hidId  DCSCommand       value   Type              group
{ "CHART_DIMMER_DEC",     "NONE",   0,  0,  -1, "CHART_DIMMER",     0, "variable_step",      0 },
{ "CHART_DIMMER_INC",     "NONE",   0,  0,  -1, "CHART_DIMMER",     1, "variable_step",      0 },
```

Both rows share the same `oride_label` (`CHART_DIMMER`). The `_DEC` row sends value `0` (decrement) and the `_INC` row sends value `1` (increment).

### Notes

- The Source for encoder rows is set to the GPIO/PCA/HC165 pins connected to the encoder's A and B outputs.
- The `group` field is `0` for variable_step controls.
- The DCS-BIOS `variable_step` interface handles the actual value change inside DCS World.

---

## Fixed Step

A **fixed_step** control is similar to `variable_step` but sends a fixed step command instead of an increment/decrement. This is used by DCS-BIOS controls that use the `fixed_step` interface.

### Behavior

1. CockpitOS detects the encoder rotation direction.
2. Sends the DCS-BIOS `fixed_step` command with the appropriate direction.

### Example

```cpp
//  label                  source  port bit hidId  DCSCommand         value   Type          group
{ "AMPCD_CONT_SW_DEC",   "GPIO",   10,  0,  -1, "AMPCD_CONT_SW",     0, "fixed_step",     0 },
{ "AMPCD_CONT_SW_INC",   "GPIO",   11,  0,  -1, "AMPCD_CONT_SW",     1, "fixed_step",     0 },
```

### When to Use fixed_step vs variable_step

The choice depends on the DCS-BIOS control definition for the specific aircraft:

- If the aircraft JSON defines the control with an `"interface": "variable_step"` input, use `"variable_step"`.
- If it defines `"interface": "fixed_step"`, use `"fixed_step"`.

The Label Creator automatically sets the correct type based on the aircraft JSON definitions.

---

## HID Button Mapping

Every input mapping has a `hidId` field that assigns a USB HID gamepad button number. This is used in two scenarios:

1. **HID Mode:** When `MODE_DEFAULT_IS_HID=1`, the device acts as a USB gamepad. Each button press sends the corresponding HID button event to the operating system.
2. **Dual Mode:** When `SEND_HID_AXES_IN_DCS_MODE=1`, analog axes are sent as HID gamepad axes even while DCS mode is active.

| hidId Value | Meaning |
|------------|---------|
| `-1` | No HID button assigned (DCS-BIOS only) |
| `0` to `127` | HID gamepad button number |

### Example

```cpp
//  label             source      port bit hidId  DCSCommand      value  Type         group
{ "FIRE_EXT_BTN",   "PCA_0x5B",   0,   2,   16, "FIRE_EXT_BTN",    1, "momentary",    0 },
```

This button has `hidId=16`, meaning it appears as gamepad button 16 in HID mode.

---

## Hardware Source Types

The `source` field determines where CockpitOS reads the input from:

| Source | Description | Port Field | Bit Field |
|--------|-------------|-----------|-----------|
| `"GPIO"` | Direct ESP32 GPIO pin | Pin number (e.g., `5`, `PIN(18)`) | `0` for normal, `-1` for selector inverse |
| `"PCA_0xNN"` | PCA9555 I2C expander at address 0xNN | Port index (`0` or `1`) | Bit position (0-7), `-1` for inverse |
| `"HC165"` | 74HC165 shift register | Not used (set to `0`) | Bit position in the chain |
| `"TM1637"` | TM1637 display key input | DIO pin number | Key index |
| `"MATRIX"` | Button matrix | Row index | Column index |
| `"NONE"` | Not wired (placeholder) | Ignored | Ignored |

Controls with Source `"NONE"` are ignored at runtime. They exist in the mapping file as placeholders for controls you might wire later.

---

## Complete Example: Mixed Panel

Here is a realistic excerpt showing multiple control types on one panel:

```cpp
static const InputMapping InputMappings[] = {
    // Momentary push buttons
    { "APU_FIRE_BTN",           "TM1637", PIN(40),  0,  -1, "APU_FIRE_BTN",            1, "momentary",     0 },
    { "MASTER_CAUTION_RESET_SW","TM1637", PIN(39),  9,  -1, "MASTER_CAUTION_RESET_SW",  1, "momentary",     0 },

    // Analog potentiometer
    { "HMD_OFF_BRT",            "GPIO",  PIN(18),   0,  -1, "HMD_OFF_BRT",          65535, "analog",        0 },

    // Variable step encoder (two rows per encoder)
    { "CHART_DIMMER_DEC",       "NONE",       0,    0,  -1, "CHART_DIMMER",             0, "variable_step", 0 },
    { "CHART_DIMMER_INC",       "NONE",       0,    0,  -1, "CHART_DIMMER",             1, "variable_step", 0 },

    // 3-position selector switch (all share group 4)
    { "COCKKPIT_LIGHT_MODE_SW_DAY",  "NONE", 0,    0,  -1, "COCKKPIT_LIGHT_MODE_SW",   0, "selector",      4 },
    { "COCKKPIT_LIGHT_MODE_SW_NITE", "NONE", 0,    0,  -1, "COCKKPIT_LIGHT_MODE_SW",   1, "selector",      4 },
    { "COCKKPIT_LIGHT_MODE_SW_NVG",  "NONE", 0,    0,  -1, "COCKKPIT_LIGHT_MODE_SW",   2, "selector",      4 },

    // 2-position selector with PCA9555 (group 8)
    { "MASTER_ARM_SW_SAFE",    "PCA_0x5B",   0,    3,  18, "MASTER_ARM_SW",            0, "selector",      8 },
    { "MASTER_ARM_SW_ARM",     "PCA_0x5B",   0,   -1,  17, "MASTER_ARM_SW",            1, "selector",      8 },
};
```

---

## See Also

- [Config.h Reference](Config.md) -- Axis thresholds and timing settings
- [DCS-BIOS Integration](DCS-BIOS.md) -- How commands and values are exchanged
- [Label Creator](../Tools/Label-Creator.md) -- Editing inputs through the GUI
- [Troubleshooting](Troubleshooting.md) -- Input not working?

---

*CockpitOS Control Types Reference | Last updated: February 2026*
