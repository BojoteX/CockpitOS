# Buttons and Switches

Buttons and switches are the most common inputs in a cockpit panel. This guide covers all switch types supported by CockpitOS, how to wire them, and how to configure them using the Label Creator tool.

---

## Switch Types at a Glance

```
+----------------------------------------------------------------------+
|  SWITCH TYPES                                                        |
+----------------------------------------------------------------------+
|                                                                      |
|  MOMENTARY (Push Button)                                             |
|  - Returns to original position when released                        |
|  - Examples: fire button, master caution reset, UFC keypad           |
|                                                                      |
|  TOGGLE (2-Position, Maintained)                                     |
|  - Stays in position until moved again                               |
|  - Examples: master arm ON/OFF, generator switch                     |
|                                                                      |
|  TOGGLE (3-Position)                                                 |
|  - UP / CENTER / DOWN                                                |
|  - Examples: fuel dump, exterior lights, engine crank                |
|                                                                      |
|  ROTARY SELECTOR (Multi-Position)                                    |
|  - Multiple fixed positions on a dial                                |
|  - Examples: mode selectors, channel knobs                           |
|                                                                      |
|  KEY SWITCH                                                          |
|  - Momentary, 2-position, or 3-position, operated by a key          |
|  - Wired identically to the equivalent toggle/momentary type         |
|                                                                      |
+----------------------------------------------------------------------+
```

---

## Push Buttons (Momentary)

### Wiring

Connect one terminal of the button to a GPIO pin and the other to GND. CockpitOS enables the internal pull-up resistor automatically, so no external resistor is needed.

```
                    ESP32
                 +---------+
                 |         |
                 |  GPIO 5 +--------+
                 |         |        |
                 |         |     [BUTTON]
                 |         |        |
                 |    GND  +--------+
                 |         |
                 +---------+

   Button OPEN   --> GPIO reads HIGH (pulled up to 3.3V)
   Button PRESSED --> GPIO reads LOW  (shorted to GND)
```

### InputMapping.h Fields

A momentary button requires a single row in InputMapping.h:

```cpp
//  label                source   port     bit  hidId  DCSCommand             value  Type        group
{ "MASTER_CAUTION_BTN",  "GPIO",  PIN(5),   0,   -1,  "MASTER_CAUTION_BTN",     1,  "momentary",   0 },
```

**Field reference:**

| Field         | Value               | Meaning                                       |
|---------------|---------------------|-----------------------------------------------|
| label         | `"MASTER_CAUTION_BTN"` | Unique identifier for this input           |
| source        | `"GPIO"`            | Direct GPIO connection                        |
| port          | `PIN(5)`            | GPIO pin number (use PIN() macro)             |
| bit           | `0`                 | Not used for simple GPIO buttons, keep as 0   |
| hidId         | `-1`                | HID button ID (-1 = no HID output)            |
| oride_label   | `"MASTER_CAUTION_BTN"` | DCS-BIOS command name to send              |
| oride_value   | `1`                 | Value sent when pressed                       |
| controlType   | `"momentary"`       | Press-and-release behavior                    |
| group         | `0`                 | No group (standalone button)                  |

### Label Creator Configuration

1. Open Label Creator and select your Label Set
2. Click **Edit Inputs**
3. Find the control (e.g., MASTER_CAUTION_BTN)
4. Set **Source** = `GPIO`
5. Set **Port** = your GPIO pin number
6. Save

---

## Two-Position Toggle Switches

A two-position toggle (ON/OFF) requires two rows in InputMapping.h -- one for each position. These rows are linked by a shared **group** number.

### Wiring

Same as a momentary button: common terminal to GPIO, one throw to GND.

```
                    ESP32
                 +---------+
                 |         |
                 |  GPIO 5 +-------+---- C (common)
                 |         |       |
                 |    GND  +-------+---- NO (normally open)
                 |         |
                 +---------+

   Switch in OFF position --> GPIO reads HIGH
   Switch in ON  position --> GPIO reads LOW
```

### InputMapping.h Configuration

```cpp
//  label              source   port     bit  hidId  DCSCommand       value  Type       group
{ "MASTER_ARM_ON",     "GPIO",  PIN(5),   0,   -1,  "MASTER_ARM_SW",    1,  "selector",   1 },
{ "MASTER_ARM_OFF",    "GPIO",     -1,   -1,   -1,  "MASTER_ARM_SW",    0,  "selector",   1 },
```

**Key points:**

- Both rows share the same **group** number (1) -- this links them as a selector
- Both rows share the same **DCS command** (`MASTER_ARM_SW`) but send different **values**
- The OFF position has `port = -1` and `bit = -1` -- this marks it as the **fallback** position
- The fallback is automatically active when no GPIO in the group reads LOW

---

## Three-Position Toggle Switches

Three-position switches (UP / CENTER / DOWN) use two GPIOs. The center position is detected automatically when neither GPIO is LOW.

### Wiring

```
                    ESP32
                 +---------+
                 |         |
     UP  --------| GPIO 5  +----------+
                 |         |          |
                 |  GPIO 6 +----------+---- C (common to GND)
     DOWN -------|         |          |
                 |         |          |
                 |    GND  +----------+
                 |         |
                 +---------+

   Switch UP     --> GPIO 5 LOW, GPIO 6 HIGH
   Switch CENTER --> GPIO 5 HIGH, GPIO 6 HIGH (neither active)
   Switch DOWN   --> GPIO 5 HIGH, GPIO 6 LOW
```

### InputMapping.h Configuration

```cpp
//  label                  source   port     bit  hidId  DCSCommand        value  Type       group
{ "FUEL_DUMP_SW_NORM",     "GPIO",  PIN(5),   0,   -1,  "FUEL_DUMP_SW",      0,  "selector",   5 },
{ "FUEL_DUMP_SW_OFF",      "GPIO",     -1,   -1,   -1,  "FUEL_DUMP_SW",      1,  "selector",   5 },
{ "FUEL_DUMP_SW_DUMP",     "GPIO",  PIN(6),   0,   -1,  "FUEL_DUMP_SW",      2,  "selector",   5 },
```

**Key points:**

- All three rows share **group = 5**
- The CENTER position has `port = -1` -- it is the fallback
- CockpitOS automatically reports CENTER when neither GPIO 5 nor GPIO 6 reads LOW

---

## Multi-Position Rotary Selectors

For rotary selectors with many positions, use the "one-hot" approach where each position has its own GPIO pin.

### Wiring

Each position connects the common pin to a different GPIO through the rotary switch mechanism. The common is wired to GND.

### InputMapping.h Configuration (One-Hot)

```cpp
//  label              source   port      bit  hidId  DCSCommand     value  Type       group
{ "MODE_SW_OFF",       "GPIO",  PIN(10),  -1,   -1,  "MODE_SELECT",    0,  "selector",  10 },
{ "MODE_SW_STBY",      "GPIO",  PIN(11),  -1,   -1,  "MODE_SELECT",    1,  "selector",  10 },
{ "MODE_SW_LOW",       "GPIO",  PIN(12),  -1,   -1,  "MODE_SELECT",    2,  "selector",  10 },
{ "MODE_SW_MED",       "GPIO",  PIN(13),  -1,   -1,  "MODE_SELECT",    3,  "selector",  10 },
{ "MODE_SW_HIGH",      "GPIO",  PIN(14),  -1,   -1,  "MODE_SELECT",    4,  "selector",  10 },
```

**Key points:**

- All positions share the same **group** and the same **DCS command**
- `bit = -1` signals one-hot mode (each position has its own GPIO)
- Each position sends a different **value** corresponding to the DCS-BIOS selector position
- For high-position-count selectors, consider using [Shift Registers](Shift-Registers.md) or [I2C Expanders](I2C-Expanders.md) to conserve GPIO pins

---

## Selector Groups -- How They Work

Selector groups are central to how CockpitOS handles multi-position switches. Understanding them is essential.

### Rules

1. Every row in a selector group shares the **same group number** (any non-zero value)
2. Every row shares the **same DCS-BIOS command name** (oride_label)
3. Each row has a **different value** (oride_value) matching the DCS-BIOS position
4. Exactly **one** row should be the fallback (port = -1, bit = -1) -- the position reported when no GPIO is active
5. CockpitOS scans all rows in a group and reports the one whose GPIO is LOW, or the fallback if none is LOW

### Group numbering

Group numbers must be unique within a Label Set. Group 0 means "not grouped" (used for standalone momentary buttons and analog inputs). Any positive integer (1-65535) can be used as a group ID.

---

## Key Switches

Key switches (operated by inserting and turning a physical key) are electrically identical to their toggle or momentary equivalents:

- **Momentary key switch**: Wire and configure as a push button
- **2-position key switch**: Wire and configure as a 2-position toggle
- **3-position key switch**: Wire and configure as a 3-position toggle

---

## Cover Switches (CoverGate System)

Some cockpit switches have protective covers (e.g., the fire button cover, spin recovery cover). CockpitOS has a CoverGate system that tracks cover state and can prevent the protected switch from activating unless the cover is open.

Cover switches are wired as standard momentary or toggle inputs. The relationship between the cover and the protected switch is managed in the firmware.

For details on configuring CoverGate behavior, see the Advanced documentation.

---

## Using Expander Sources

Buttons and switches are not limited to direct GPIO. You can connect them through any supported input source:

| Source       | Example                                                              |
|--------------|----------------------------------------------------------------------|
| `GPIO`       | `{ "MY_BTN", "GPIO", PIN(5), 0, -1, "MY_BTN", 1, "momentary", 0 }` |
| `PCA_0x20`   | `{ "MY_BTN", "PCA_0x20", 0, 3, -1, "MY_BTN", 1, "momentary", 0 }`  |
| `HC165`      | `{ "MY_BTN", "HC165", 0, 5, -1, "MY_BTN", 1, "momentary", 0 }`     |
| `TM1637`     | `{ "MY_BTN", "TM1637", PIN(39), 3, -1, "MY_BTN", 1, "momentary", 0 }` |
| `MATRIX`     | `{ "MY_BTN", "MATRIX", PIN(10), 4, -1, "MY_BTN", 1, "selector", 20 }` |

See [Shift Registers](Shift-Registers.md) and [I2C Expanders](I2C-Expanders.md) for wiring details on those sources.

---

## Debouncing

CockpitOS handles switch debouncing automatically in firmware. Key parameters in Config.h:

| Parameter              | Default | Purpose                                         |
|------------------------|---------|-------------------------------------------------|
| `VALUE_THROTTLE_MS`    | 50      | Minimum ms between sending the same value again |
| `ANY_VALUE_THROTTLE_MS`| 33      | Minimum ms between sending any value            |
| `SELECTOR_DWELL_MS`    | 100     | Wait time for stable selector value             |

You should not need to change these for standard switches. If you experience false triggers, add a 0.1uF capacitor between the GPIO pin and GND.

---

## HID Button Output

If you want a button to also register as a USB HID gamepad button (for use outside DCS), set the `hidId` field to a button number (1-128). Set it to `-1` to disable HID output for that button.

```cpp
//                                                hidId
{ "MASTER_MODE_AA", "PCA_0x5B", 0, 1, 19, "MASTER_MODE_AA", 1, "momentary", 0 },
//                                     ^^ HID button 19
```

---

## Quick Reference

### Control Types for Switches

| Switch Type          | controlType    | Rows Needed | Group Required |
|----------------------|----------------|-------------|----------------|
| Push button          | `"momentary"`  | 1           | No (group = 0) |
| 2-position toggle    | `"selector"`   | 2           | Yes            |
| 3-position toggle    | `"selector"`   | 3           | Yes            |
| Multi-position rotary| `"selector"`   | N (one per position) | Yes   |
| Key switch (momentary)| `"momentary"` | 1           | No             |
| Key switch (toggle)  | `"selector"`   | 2 or 3      | Yes            |

### Input Source Types

| Source     | Hardware              | Details Page                          |
|------------|-----------------------|---------------------------------------|
| `GPIO`     | Direct ESP32 pin      | This page                             |
| `PCA_0xNN` | PCA9555 I2C expander  | [I2C Expanders](I2C-Expanders.md)     |
| `HC165`    | 74HC165 shift register| [Shift Registers](Shift-Registers.md) |
| `TM1637`   | TM1637 keypad input   | [Displays](Displays.md)               |
| `MATRIX`   | Matrix rotary switch  | [Displays](Displays.md)               |
| `NONE`     | Disabled / fallback   | Used for virtual entries              |

---

*See also: [Hardware Overview](README.md) | [LEDs](LEDs.md) | [Encoders](Encoders.md)*
