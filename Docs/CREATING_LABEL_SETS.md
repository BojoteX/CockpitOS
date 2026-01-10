# CockpitOS — Creating Label Sets

A complete guide for creating custom Label Sets in CockpitOS. This document assumes you have completed the initial setup covered in **GETTING_STARTED.md** and have a working Arduino IDE environment with the ESP32 board package installed.

---

## Table of Contents

1. [What Is a Label Set?](#1-what-is-a-label-set)
2. [Prerequisites](#2-prerequisites)
3. [Understanding the Aircraft JSON](#3-understanding-the-aircraft-json)
4. [The Workflow: Copy → Reset → Configure → Generate](#4-the-workflow-copy--reset--configure--generate)
5. [Configuring selected_panels.txt](#5-configuring-selected_panelstxt)
6. [Configuring InputMapping.h](#6-configuring-inputmappingh)
7. [Configuring LEDMapping.h](#7-configuring-ledmappingh)
8. [Running the Generator](#8-running-the-generator)
9. [Common Patterns and Recipes](#9-common-patterns-and-recipes)
10. [Troubleshooting](#10-troubleshooting)
11. [Next Steps](#11-next-steps)

---

## 1. What Is a Label Set?

A **Label Set** is a self-contained configuration folder that tells CockpitOS:

- **Which aircraft** you're building a panel for (F/A-18C, AH-64D, A-10C, etc.)
- **Which panels** from that aircraft you want to include (IFEI, Caution Advisory, UFC, etc.)
- **How your physical hardware is wired** (which GPIO pins, I²C addresses, shift register bits)

Each Label Set lives in its own directory under `src/LABELS/` and contains:

```
src/LABELS/LABEL_SET_MYPANEL/
├── FA-18C_hornet.json      ← Aircraft definition (from DCS-BIOS)
├── selected_panels.txt     ← Which panels YOU want
├── panels.txt              ← All available panels (auto-generated reference)
├── generate_data.py        ← Main generator script
├── display_gen.py          ← Display-specific generator
├── reset_data.py           ← Cleanup script
├── METADATA/               ← Optional custom extensions
│
│   ─── AUTO-GENERATED (created by running generate_data.py) ───
├── DCSBIOSBridgeData.h     ← Address tables, control types
├── InputMapping.h          ← Buttons, switches, encoders → YOU EDIT THIS
├── LEDMapping.h            ← LEDs, gauges, indicators → YOU EDIT THIS
├── DisplayMapping.cpp/h    ← Display field definitions
├── CT_Display.cpp/h        ← Character buffers for displays
└── LabelSetConfig.h        ← Label set metadata
```

**Why Label Sets?**

- **Modularity**: Build one panel at a time, or combine multiple panels
- **Scalability**: Support cockpits from 1 button to 200+ devices
- **Flexibility**: Switch between aircraft by switching Label Sets
- **Preservation**: Your hardware assignments survive when you regenerate

---

## 2. Prerequisites

Before creating a Label Set, ensure you have:

### 2.1 DCS-BIOS Installed

CockpitOS requires **DCS-BIOS** (Skunkworks version) to be installed in DCS World. DCS-BIOS is the bridge that exports cockpit data from the simulator.

**Installation Steps:**

1. Download the latest release from: https://github.com/DCS-Skunkworks/dcs-bios/releases
2. Extract the ZIP file
3. Copy the `Scripts/DCS-BIOS` folder to your DCS Saved Games folder:
   ```
   %USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS
   ```
4. If you don't have an `Export.lua` file in your Scripts folder, copy it from the DCS-BIOS download
5. If you already have an `Export.lua`, add this line to it:
   ```lua
   dofile(lfs.writedir()..[[Scripts\DCS-BIOS\BIOS.lua]])
   ```

For detailed instructions, see the DCS-BIOS documentation at:
https://github.com/DCS-Skunkworks/dcs-bios/wiki

### 2.2 Arduino IDE Configured

You should have:

- Arduino IDE 2.x installed
- ESP32 board package installed (version 3.3.0 or later)
- CockpitOS source code downloaded

See **GETTING_STARTED.md** for detailed setup instructions.

### 2.3 Python 3 Installed

The generator scripts require Python 3. Most systems have this pre-installed. To verify:

```bash
python --version
# or
python3 --version
```

If not installed, download from: https://www.python.org/downloads/

---

## 3. Understanding the Aircraft JSON

The **Aircraft JSON** is the source of truth for all cockpit controls. It comes from DCS-BIOS and contains every button, switch, LED, display, and gauge for a specific aircraft.

### 3.1 Where to Find Aircraft JSON Files

**Option A: From Your DCS-BIOS Installation in DCS World**

If DCS-BIOS is installed, JSON files are located at:
```
%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json\
```

You'll find files like:
- `A-10C.json`
- `AH-64D.json`
- `F-16C_50.json`
- `FA-18C_hornet.json`
- `UH-1H.json`
- ... and many more

**Option B: From DCS-BIOS GitHub Release**

1. Go to: https://github.com/DCS-Skunkworks/dcs-bios/releases
2. Download the latest release ZIP
3. Extract and navigate to: `Scripts/DCS-BIOS/doc/json/`
4. Copy your aircraft's JSON file

### 3.2 What's Inside the JSON?

The JSON organizes controls by **panel** (category):

```json
{
  "IFEI": {
    "IFEI_BTN_1": {
      "category": "IFEI",
      "control_type": "selector",
      "description": "IFEI Button 1",
      "identifier": "IFEI_BTN_1",
      "inputs": [
        { "interface": "set_state", "max_value": 1 }
      ],
      "outputs": [
        {
          "address": 29798,
          "mask": 64,
          "shift_by": 6,
          "max_value": 1
        }
      ]
    },
    "IFEI_CLOCK_H": {
      "control_type": "display",
      "outputs": [
        { "address": 29800, "max_length": 2 }
      ]
    }
  },
  "Flaps & Landing Gear Panel": {
    "FLP_LG_FLAPS_LT": {
      "control_type": "led",
      "outputs": [
        { "address": 29798, "mask": 1, "shift_by": 0, "max_value": 1 }
      ]
    }
  }
}
```

**Key Fields:**

| Field | Meaning |
|-------|---------|
| `category` | Panel name (used in selected_panels.txt) |
| `control_type` | What kind: `led`, `selector`, `display`, `analog_gauge`, etc. |
| `identifier` | Unique label (used everywhere in CockpitOS) |
| `address` | DCS-BIOS memory address |
| `mask` / `shift_by` | How to extract the value from the data stream |
| `max_value` | Maximum possible value |
| `max_length` | For displays: number of characters |

The generator reads this JSON and creates all the mapping tables automatically.

---

## 4. The Workflow: Copy → Reset → Configure → Generate

Creating a new Label Set follows a simple four-step process:

### Step 1: Copy an Existing Label Set

The easiest way to start is by copying an existing Label Set. This gives you all the scripts and structure you need.

```bash
# Navigate to the LABELS directory
cd src/LABELS/

# Copy an existing label set
cp -r LABEL_SET_IFEI LABEL_SET_MYPANEL

# On Windows:
xcopy /E /I LABEL_SET_IFEI LABEL_SET_MYPANEL
```

**Why copy?** You get:
- All generator scripts (generate_data.py, display_gen.py, reset_data.py)
- Correct directory structure
- Example configuration files

### Step 2: Run reset_data.py (Clean Slate)

Navigate into your new Label Set and run the reset script:

```bash
cd LABEL_SET_MYPANEL
python reset_data.py
```

This **deletes all auto-generated files** from the copied Label Set:

```
The following files will be DELETED if present:
  - DCSBIOSBridgeData.h
  - InputMapping.h
  - LEDMapping.h
  - DisplayMapping.cpp
  - DisplayMapping.h
  - CT_Display.cpp
  - CT_Display.h
  - LabelSetConfig.h

Are you absolutely sure you want to DELETE these files? (yes/NO): yes
```

**What remains after reset:**
- `generate_data.py` (main generator)
- `display_gen.py` (display generator)
- `reset_data.py` (this cleanup script)
- `selected_panels.txt` (panel selection — you'll edit this)
- `panels.txt` (reference list — auto-regenerated)
- Any `*_SegmentMap.h` files (hardware mappings for displays)
- The aircraft JSON file

### Step 3: Add Your Aircraft JSON

If you're building for a different aircraft than what was in the copied Label Set:

```bash
# Remove the old aircraft JSON
del FA-18C_hornet.json        # Windows
rm FA-18C_hornet.json         # Linux/Mac

# Copy your aircraft's JSON
copy "path\to\AH-64D.json" .  # Windows
cp /path/to/AH-64D.json .     # Linux/Mac
```

The generator auto-detects any valid JSON file in the directory.

**Important:** Only ONE aircraft JSON should be present in each Label Set folder.

### Step 4: Run the Generator (First Time)

Run the generator to create `panels.txt` and the template `selected_panels.txt`:

```bash
python generate_data.py
```

**First run output:**
```
[✓] Detected JSON: AH-64D.json
[✓] Generated panels.txt with 47 panels
[!] selected_panels.txt not found — creating template...
[!] Please edit selected_panels.txt and re-run this script.
```

Now you need to configure which panels you want before running the generator again.

---

## 5. Configuring selected_panels.txt

The `selected_panels.txt` file controls which panels are included in your firmware.

### 5.1 Understanding the File

After the first generator run, you'll have:

**panels.txt** (auto-generated reference — DO NOT EDIT):
```
# Available panels from AH-64D.json
Armament Panel
CMWS Panel
Collective
Communications Panel
Cyclic
Engine Panel
External Aircraft Model
Flight Instruments
...
```

**selected_panels.txt** (template — EDIT THIS):
```
# Uncomment the panels you want to include in your firmware
# Panel names must EXACTLY match those in panels.txt

# Armament Panel
# CMWS Panel
# Collective
# Communications Panel
# Cyclic
...
```

### 5.2 Selecting Your Panels

Open `selected_panels.txt` in a text editor and **uncomment** (remove the `#`) the panels you want:

```
# Uncomment the panels you want to include

CMWS Panel
External Aircraft Model
# Armament Panel
# Collective
...
```

**Tips:**

- **Start small**: Begin with just 1-2 panels to keep things simple
- **Case-sensitive**: Panel names must match EXACTLY (copy/paste from panels.txt)
- **External Aircraft Model**: Include this if you want external lighting data
- **Typos cause errors**: The generator will tell you if a panel name doesn't exist

### 5.3 Run the Generator Again

After editing `selected_panels.txt`, run the generator:

```bash
python generate_data.py
```

**Successful output:**
```
[✓] Detected JSON: AH-64D.json
[✓] Generated panels.txt with 47 panels
[✓] Loaded 2 panels from selected_panels.txt
[✓] Generated DCSBIOSBridgeData.h with 156 entries
[✓] Generated InputMapping.h with 42 entries
[✓] Generated LEDMapping.h with 28 entries
[✓] Generated CT_Display.h/cpp with 12 buffers
[✓] Generated LabelSetConfig.h
[INFO] Generating DisplayMapping.cpp/h using display_gen.py ...
[✓] Set active label set: LABEL_SET_MYPANEL
```

Now you have auto-generated mapping files ready to configure!

---

## 6. Configuring InputMapping.h

The `InputMapping.h` file tells CockpitOS **how to read your physical inputs** (buttons, switches, encoders, potentiometers) and **what DCS-BIOS commands to send**.

### 6.1 Understanding the Structure

Each entry in `InputMappings[]` represents one input control:

```cpp
struct InputMapping {
    const char* label;        // Unique identifier (from JSON)
    const char* source;       // Hardware type: "GPIO", "HC165", "PCA_0x5B", "TM1637", "MATRIX", "NONE"
    int8_t     port;          // Pin or port number (depends on source)
    int8_t     bit;           // Bit position or special flag
    int8_t     hidId;         // USB HID button ID (-1 to disable)
    const char* oride_label;  // DCS-BIOS command name
    uint16_t    oride_value;  // DCS-BIOS command value
    const char* controlType;  // "selector", "momentary", "analog", "variable_step", "fixed_step"
    uint16_t    group;        // Selector group ID (0 = none)
};
```

**Auto-generated entries look like this:**

```cpp
static const InputMapping InputMappings[] = {
    { "CMWS_ARM",     "NONE", 0, 0, -1, "CMWS_ARM",     0, "selector", 1 },
    { "CMWS_BYPASS",  "NONE", 0, 0, -1, "CMWS_BYPASS",  1, "selector", 1 },
    { "CMWS_TEST",    "NONE", 0, 0, -1, "CMWS_TEST",    1, "momentary", 0 },
};
```

**Your job:** Change `"NONE"` to the actual hardware source and fill in the correct `port` and `bit` values.

### 6.2 Source Types

CockpitOS supports six input source types:

| Source | Description | Example |
|--------|-------------|---------|
| `"GPIO"` | Direct ESP32 GPIO pin | Buttons, switches wired directly to ESP32 |
| `"HC165"` | 74HC165 shift register chain | Many buttons on few pins |
| `"PCA_0xNN"` | PCA9555 I²C expander (16 I/O) | I²C-connected buttons/switches |
| `"TM1637"` | TM1637 LED driver key scan | Buttons on annunciator panels |
| `"MATRIX"` | Matrix-wired rotary switch | Multi-position rotary with strobe/data |
| `"NONE"` | No physical hardware | Fallback positions, virtual controls |

### 6.3 GPIO Inputs

GPIO is the most common source for simple builds.

#### Simple Momentary Button

```cpp
// Button on GPIO 8, active LOW (pressed = connects to GND)
{ "CMWS_TEST", "GPIO", PIN(8), 0, -1, "CMWS_TEST", 1, "momentary", 0 },
//                     ^^^^^^  ^
//                     pin     bit=0 means active LOW
```

- **port**: Use `PIN(x)` macro for portability between ESP32-S2/S3
- **bit**: `0` = active LOW (button pulls pin to GND), `1` = active HIGH
- **group**: `0` for standalone buttons

#### Two-Position Toggle Switch (ON/OFF)

```cpp
// Toggle switch: ON position on GPIO 5
{ "MASTER_ARM_ON",  "GPIO", PIN(5),  0, -1, "MASTER_ARM", 1, "selector", 3 },
{ "MASTER_ARM_OFF", "GPIO",    -1, -1, -1, "MASTER_ARM", 0, "selector", 3 },
//                         ^^^^^  ^^
//                         port=-1, bit=-1 = FALLBACK (no physical wire)
```

- When GPIO 5 is LOW → ON position active → sends `MASTER_ARM 1`
- When GPIO 5 is HIGH → no winner → FALLBACK fires → sends `MASTER_ARM 0`
- Both positions share the same **group** (3)

#### Three-Position Selector (UP/CENTER/DOWN)

```cpp
// Three-position toggle: UP and DOWN have wires, CENTER is fallback
{ "SW_UP",     "GPIO", PIN(5),  0, -1, "MY_SWITCH", 0, "selector", 7 },
{ "SW_CENTER", "GPIO",    -1, -1, -1, "MY_SWITCH", 1, "selector", 7 },  // FALLBACK
{ "SW_DOWN",   "GPIO", PIN(6),  0, -1, "MY_SWITCH", 2, "selector", 7 },
```

- **CENTER** has `port=-1` and `bit=-1` — it's the fallback
- When neither UP nor DOWN is active, CENTER wins automatically

#### One-Hot Selector (Each Position Has Own GPIO)

For rotary switches where each position has its own GPIO:

```cpp
// 4-position rotary: each position on separate GPIO
{ "MODE_OFF",  "GPIO", PIN(10), -1, -1, "MODE_SW", 0, "selector", 12 },
{ "MODE_STBY", "GPIO", PIN(11), -1, -1, "MODE_SW", 1, "selector", 12 },
{ "MODE_ON",   "GPIO", PIN(12), -1, -1, "MODE_SW", 2, "selector", 12 },
{ "MODE_TEST", "GPIO", PIN(13), -1, -1, "MODE_SW", 3, "selector", 12 },
//                              ^^
//                              ALL bit=-1 signals ONE-HOT mode
```

- **All entries have `bit=-1`** — this tells the firmware it's one-hot mode
- First pin that reads LOW wins
- All positions share the same **group**

#### Rotary Encoder (Quadrature)

```cpp
// Rotary encoder: two GPIO pins for A/B phases
{ "VOLUME_DEC", "GPIO", PIN(6), 0, -1, "VOLUME", 0, "variable_step", 0 },
{ "VOLUME_INC", "GPIO", PIN(7), 0, -1, "VOLUME", 1, "variable_step", 0 },
//                                               ^
//                                               0=CCW, 1=CW
```

- **Same `oride_label`** for both entries
- **`oride_value`**: `0` = counter-clockwise, `1` = clockwise
- Firmware auto-pairs them as encoder A/B phases
- Use `"variable_step"` or `"fixed_step"` for controlType

#### Analog Input (Potentiometer)

```cpp
// Potentiometer on GPIO 3
{ "BRIGHTNESS", "GPIO", PIN(3), 0, -1, "BRIGHTNESS", 65535, "analog", 0 },
//                                                   ^^^^^
//                                                   65535 = raw value passthrough
```

- **`oride_value = 65535`** means send the raw analog value
- The firmware reads the ADC and sends the value directly to DCS-BIOS

### 6.4 HC165 Shift Register Inputs

The 74HC165 shift register lets you read many buttons with only 3 GPIO pins.

```cpp
// 3-position selector on HC165 chain
{ "MODE_AUTO", "HC165", 0,  6, -1, "MODE_SW", 0, "selector", 5 },
{ "MODE_OFF",  "HC165", 0, -1, -1, "MODE_SW", 1, "selector", 5 },  // FALLBACK
{ "MODE_MAN",  "HC165", 0,  0, -1, "MODE_SW", 2, "selector", 5 },
//                      ^  ^^
//                      │  └── bit position in chain (0-63)
//                      └───── port (usually 0, ignored by HC165)
```

- **bit**: Position in the shift register chain (bit 0 = first chip's D0, bit 8 = second chip's D0, etc.)
- **bit=-1**: Fallback/neutral position
- HC165 is **active LOW**: bit reads 0 when switch is closed

### 6.5 PCA9555 I²C Expander Inputs

The PCA9555 provides 16 I/O pins over I²C.

```cpp
// Button on PCA9555 at address 0x5B, port 0, bit 2
{ "FIRE_BTN", "PCA_0x5B", 0, 2, -1, "FIRE_BTN", 1, "momentary", 0 },
//            ^^^^^^^^^   ^  ^
//            I²C addr    │  └── bit (0-7)
//                        └───── port (0 or 1)
```

- **source**: `"PCA_0xNN"` where NN is the I²C address in hex
- **port**: `0` or `1` (each PCA9555 has two 8-bit ports)
- **bit**: `0-7` within that port, or `-1` for fallback

### 6.6 TM1637 Key Scan Inputs

TM1637 LED drivers can also scan for button presses.

```cpp
// Button on TM1637, DIO on GPIO 39, key index 3
{ "LA_FIRE", "TM1637", PIN(39), 3, -1, "LA_FIRE", 1, "momentary", 0 },
//                     ^^^^^^^  ^
//                     DIO pin  key index (0-15)
```

- **port**: The DIO GPIO pin connected to the TM1637
- **bit**: Key index (0-15) from the TM1637 scan code

**Important:** If you have TM1637 inputs, you MUST have at least one TM1637 LED entry in `LEDMapping.h` so the firmware initializes the device!

### 6.7 MATRIX Rotary Inputs

For matrix-wired rotary switches with strobe/data lines:

```cpp
// Matrix rotary: 4 strobes, 1 data line
// "Anchor" row defines data pin
{ "DIAL_X",    "MATRIX", PIN(20), 15, -1, "DIAL", 0, "selector", 0 },
//                       ^^^^^^^  ^^
//                       dataPin  0b1111 = 4 strobes

// Position rows (bit = strobe pattern as decimal)
{ "DIAL_POS0", "MATRIX", PIN(21),  1, -1, "DIAL", 0, "selector", 0 },
{ "DIAL_POS1", "MATRIX", PIN(22),  2, -1, "DIAL", 1, "selector", 0 },
{ "DIAL_POS2", "MATRIX", PIN(23),  4, -1, "DIAL", 2, "selector", 0 },
{ "DIAL_POS3", "MATRIX", PIN(24),  8, -1, "DIAL", 3, "selector", 0 },
//                       ^^^^^^^   ^
//                       strobeGPIO  pattern (1<<strobe_index)

// Optional fallback
{ "DIAL_OFF",  "MATRIX", PIN(20), -1, -1, "DIAL", 4, "selector", 0 },
```

This is an advanced configuration for specialized hardware.

### 6.8 The Group System

**Selector groups** enforce mutual exclusivity — only one position can be active at a time.

**Rules:**
1. All positions of the same switch **must share the same non-zero group ID**
2. Use `group = 0` for standalone buttons (no exclusivity)
3. Group IDs are arbitrary numbers — just keep them unique per switch
4. The generator auto-assigns group IDs, but you can change them

**How it works:**
1. Firmware scans all entries in a group
2. First entry with an active physical signal wins
3. If no physical signal found, the fallback entry (bit=-1) wins
4. Only ONE command sent per group per poll cycle

---

## 7. Configuring LEDMapping.h

The `LEDMapping.h` file tells CockpitOS **how to control your physical outputs** (LEDs, indicators, gauges) when DCS-BIOS sends state updates.

### 7.1 Understanding the Structure

Each entry in `panelLEDs[]` represents one output:

```cpp
struct LEDMapping {
    const char* label;           // Unique identifier (from JSON)
    LEDDeviceType deviceType;    // Hardware driver type
    union { ... } info;          // Driver-specific configuration
    bool dimmable;               // Supports brightness control?
    bool activeLow;              // ON = LOW voltage?
};
```

**Auto-generated entries look like this:**

```cpp
static const LEDMapping panelLEDs[] = {
    { "CMWS_ARMED_LT", DEVICE_NONE, {.gpioInfo = {0}}, false, false },
    { "CMWS_JAMMED_LT", DEVICE_NONE, {.gpioInfo = {0}}, false, false },
};
```

**Your job:** Change `DEVICE_NONE` to the actual device type and fill in the correct info struct.

### 7.2 Device Types

CockpitOS supports seven LED device types:

| Device Type | Description | Use Case |
|-------------|-------------|----------|
| `DEVICE_GPIO` | Direct ESP32 GPIO | Simple LEDs |
| `DEVICE_WS2812` | Addressable RGB LED | NeoPixel strips |
| `DEVICE_TM1637` | 7-segment LED driver | Annunciator panels |
| `DEVICE_PCA9555` | I²C expander output | Many LEDs on I²C |
| `DEVICE_GN1640T` | LED matrix driver | Caution Advisory panel |
| `DEVICE_GAUGE` | Servo/PWM gauge | Analog instruments |
| `DEVICE_NONE` | Not configured | Placeholder |

### 7.3 DEVICE_GPIO — Direct GPIO LEDs

#### Simple On/Off LED

```cpp
{ "SPIN_LT", DEVICE_GPIO,
  { .gpioInfo = { PIN(34) } },
  false,    // dimmable = false → digitalWrite()
  false },  // activeLow = false → HIGH = ON
```

#### Dimmable LED (PWM)

```cpp
{ "BACKLIGHT", DEVICE_GPIO,
  { .gpioInfo = { PIN(6) } },
  true,     // dimmable = true → analogWrite() with intensity
  false },  // activeLow
```

#### Inverted Logic (Transistor Driver)

```cpp
{ "MASTER_CAUTION", DEVICE_GPIO,
  { .gpioInfo = { PIN(18) } },
  false,    // dimmable
  true },   // activeLow = true → LOW = ON (transistor sink)
```

**Flags explained:**
- `dimmable = true`: Uses PWM, intensity controlled by DCS-BIOS value
- `dimmable = false`: Simple HIGH/LOW digital output
- `activeLow = true`: ON sends LOW, OFF sends HIGH (inverted driver)

### 7.4 DEVICE_WS2812 — Addressable RGB LEDs

WS2812 (NeoPixel) LEDs are RGB and individually addressable.

```cpp
{ "LS_LOCK", DEVICE_WS2812,
  { .ws2812Info = {
      0,        // index: position on strip (0-based)
      PIN(12),  // pin: data GPIO for this strip
      0,        // defR: default RED (0-255)
      255,      // defG: default GREEN (0-255)
      0,        // defB: default BLUE (0-255)
      255       // defBright: default brightness (0-255)
  }},
  false,    // dimmable = false → uses defBright as fixed
  false },  // activeLow IGNORED for WS2812
```

**Important notes:**

1. **`index` must be the first field** in ws2812Info (for backward compatibility)
2. **Color is defined in the mapping**, not from DCS-BIOS (DCS only sends ON/OFF)
3. **Same pin = same strip**: All entries with the same pin are on the same physical strip
4. `activeLow` is ignored for WS2812

**Multiple strips example:**

```cpp
// Strip 1 on PIN 12 (Lock/Shoot lights)
{ "LS_LOCK",  ..., { .ws2812Info = { 0, PIN(12), 0, 255, 0, 255 } }, ... },
{ "LS_SHOOT", ..., { .ws2812Info = { 1, PIN(12), 0, 255, 0, 255 } }, ... },
{ "LS_RANGE", ..., { .ws2812Info = { 2, PIN(12), 0, 255, 0, 255 } }, ... },

// Strip 2 on PIN 15 (AOA indexer)
{ "AOA_HIGH", ..., { .ws2812Info = { 0, PIN(15), 0, 255, 0, 200 } }, ... },
{ "AOA_MID",  ..., { .ws2812Info = { 1, PIN(15), 255, 165, 0, 200 } }, ... },
{ "AOA_LOW",  ..., { .ws2812Info = { 2, PIN(15), 255, 0, 0, 200 } }, ... },
```

### 7.5 DEVICE_TM1637 — 7-Segment LED Driver

TM1637 chips drive 7-segment displays and individual indicator LEDs.

```cpp
{ "FLP_LG_FLAPS_LT", DEVICE_TM1637,
  { .tm1637Info = {
      PIN(9),   // clkPin: clock GPIO
      PIN(8),   // dioPin: data I/O GPIO
      5,        // segment: digit position (0-5)
      0         // bit: segment within digit (0-7)
  }},
  false,    // dimmable
  false },  // activeLow IGNORED for TM1637
```

**How TM1637 works:**
- Each TM1637 chip has 6 "digit" positions
- Each digit has 8 segments (bits)
- The firmware identifies the device by the clkPin+dioPin pair

**Multiple LEDs on same TM1637:**

```cpp
// Same TM1637 (CLK=9, DIO=8), different segment/bit positions
{ "GEAR_LEFT",  ..., { .tm1637Info = { PIN(9), PIN(8), 3, 0 } }, ... },
{ "GEAR_NOSE",  ..., { .tm1637Info = { PIN(9), PIN(8), 5, 1 } }, ... },
{ "GEAR_RIGHT", ..., { .tm1637Info = { PIN(9), PIN(8), 3, 1 } }, ... },
```

### 7.6 DEVICE_PCA9555 — I²C Expander Output

PCA9555 can be used for both inputs AND outputs.

```cpp
{ "MASTER_MODE_AA_LT", DEVICE_PCA9555,
  { .pcaInfo = {
      0x5B,     // address: I²C address
      1,        // port: 0 or 1
      3         // bit: position within port (0-7)
  }},
  false,    // dimmable (PCA doesn't support PWM)
  true },   // activeLow = true → common with transistor drivers
```

**Note:** `activeLow` is often `true` for PCA9555 because many LED driver circuits invert the signal.

### 7.7 DEVICE_GN1640T — LED Matrix Driver

GN1640T drives LED matrices (commonly used for Caution Advisory panels).

```cpp
{ "CA_BATT_SW", DEVICE_GN1640T,
  { .gn1640Info = {
      0x24,     // address: I²C address (usually fixed)
      1,        // column: X position in matrix
      3         // row: Y position in matrix
  }},
  false,    // dimmable
  false },  // activeLow
```

### 7.8 DEVICE_GAUGE — Servo Motor / Analog Gauge

For physical analog gauges driven by servo motors.

```cpp
{ "FUEL_QTY_L", DEVICE_GAUGE,
  { .gaugeInfo = {
      PIN(10),  // gpio: PWM-capable pin
      1000,     // minPulse: minimum pulse width in µs
      2000,     // maxPulse: maximum pulse width in µs
      20000     // period: PWM period in µs (20000 = 50Hz)
  }},
  true,     // dimmable = true (position varies with DCS value)
  false },  // activeLow
```

**Standard servo values:**
- `minPulse = 1000` → 0° position
- `maxPulse = 2000` → 180° position
- `period = 20000` → 50Hz (standard for most servos)

**Calibration:** Adjust `minPulse` and `maxPulse` to match your gauge's travel range.

### 7.9 Quick Reference Table

| Device Type | Info Struct | Fields |
|-------------|-------------|--------|
| `DEVICE_GPIO` | `.gpioInfo` | `{ gpio }` |
| `DEVICE_WS2812` | `.ws2812Info` | `{ index, pin, defR, defG, defB, defBright }` |
| `DEVICE_TM1637` | `.tm1637Info` | `{ clkPin, dioPin, segment, bit }` |
| `DEVICE_PCA9555` | `.pcaInfo` | `{ address, port, bit }` |
| `DEVICE_GN1640T` | `.gn1640Info` | `{ address, column, row }` |
| `DEVICE_GAUGE` | `.gaugeInfo` | `{ gpio, minPulse, maxPulse, period }` |
| `DEVICE_NONE` | `.gpioInfo` | `{ 0 }` (dummy, required for union) |

---

## 8. Running the Generator

After configuring your mapping files, run the generator to apply your changes:

```bash
python generate_data.py
```

### 8.1 What the Generator Does

1. **Reads the aircraft JSON** and your `selected_panels.txt`
2. **Preserves your edits** in InputMapping.h and LEDMapping.h
3. **Regenerates address tables** and hash lookups
4. **Sets this Label Set as active** (updates `active_set.h`)
5. **Disables other Label Sets** (renames their .cpp files)
6. **Calls display_gen.py** for display-specific generation

### 8.2 Preservation Mechanism

The generator **preserves your hardware configurations** when you re-run it:

```cpp
// Your edit (before regeneration):
{ "CMWS_ARM", "GPIO", PIN(5), 0, -1, "CMWS_ARM", 0, "selector", 1 },

// After regeneration — YOUR EDIT IS PRESERVED:
{ "CMWS_ARM", "GPIO", PIN(5), 0, -1, "CMWS_ARM", 0, "selector", 1 },
```

**What gets preserved:**
- `source` (hardware type)
- `port` and `bit` (pin assignments)
- `hidId` (HID button mapping)
- `group` (selector groups)
- Device-specific info in LEDMapping

**What gets regenerated:**
- `label` (from JSON)
- `oride_label` and `oride_value` (from JSON)
- `controlType` (from JSON)
- Hash tables and lookup functions

### 8.3 Switching Between Label Sets

To switch to a different Label Set, simply run its generator:

```bash
cd src/LABELS/LABEL_SET_OTHER
python generate_data.py
```

This automatically:
- Sets the new Label Set as active
- Disables .cpp files in other Label Sets
- Updates `active_set.h`

---

## 9. Common Patterns and Recipes

### 9.1 Three-Position Toggle Switch

**Hardware:** Toggle switch with UP/CENTER/DOWN positions, UP and DOWN wired to GPIOs

```cpp
// InputMapping.h
{ "FUEL_DUMP_UP",     "GPIO", PIN(10),  0, -1, "FUEL_DUMP", 2, "selector", 8 },
{ "FUEL_DUMP_CENTER", "GPIO",     -1, -1, -1, "FUEL_DUMP", 1, "selector", 8 },
{ "FUEL_DUMP_DOWN",   "GPIO", PIN(11),  0, -1, "FUEL_DUMP", 0, "selector", 8 },
```

### 9.2 Guarded Momentary Button

**Hardware:** Button protected by a flip-up cover

```cpp
// InputMapping.h
{ "EMERG_JETT_COVER", "GPIO", PIN(5), 0, -1, "EMERG_JETT_COVER", 1, "momentary", 0 },
{ "EMERG_JETT_BTN",   "GPIO", PIN(6), 0, -1, "EMERG_JETT_BTN",   1, "momentary", 0 },
```

### 9.3 Rotary Encoder for Volume/Brightness

**Hardware:** Quadrature encoder with A/B outputs

```cpp
// InputMapping.h
{ "RADIO_VOL_DEC", "GPIO", PIN(12), 0, -1, "RADIO_VOL", 0, "variable_step", 0 },
{ "RADIO_VOL_INC", "GPIO", PIN(13), 0, -1, "RADIO_VOL", 1, "variable_step", 0 },
```

### 9.4 Multi-Position Rotary Switch on HC165

**Hardware:** 5-position rotary connected to shift register

```cpp
// InputMapping.h
{ "MODE_OFF",   "HC165", 0,  0, -1, "MODE_SW", 0, "selector", 15 },
{ "MODE_STBY",  "HC165", 0,  1, -1, "MODE_SW", 1, "selector", 15 },
{ "MODE_LOW",   "HC165", 0,  2, -1, "MODE_SW", 2, "selector", 15 },
{ "MODE_MED",   "HC165", 0,  3, -1, "MODE_SW", 3, "selector", 15 },
{ "MODE_HIGH",  "HC165", 0,  4, -1, "MODE_SW", 4, "selector", 15 },
```

### 9.5 RGB Status Light

**Hardware:** WS2812 LED for multi-color status

```cpp
// LEDMapping.h — Three entries for different states
{ "GEAR_UNSAFE", DEVICE_WS2812,
  { .ws2812Info = { 0, PIN(15), 255, 0, 0, 255 } },  // RED
  false, false },

{ "GEAR_TRANSIT", DEVICE_WS2812,
  { .ws2812Info = { 0, PIN(15), 255, 165, 0, 255 } },  // ORANGE
  false, false },

{ "GEAR_SAFE", DEVICE_WS2812,
  { .ws2812Info = { 0, PIN(15), 0, 255, 0, 255 } },  // GREEN
  false, false },
```

### 9.6 Analog Fuel Gauge with Servo

**Hardware:** Servo motor driving a fuel gauge needle

```cpp
// LEDMapping.h
{ "FUEL_QTY_L", DEVICE_GAUGE,
  { .gaugeInfo = { PIN(18), 544, 2400, 20000 } },  // Calibrated for this gauge
  true, false },
```

---

## 10. Troubleshooting

### 10.1 Generator Errors

**"Invalid panel name"**
```
[ERROR] Panel 'CMWS panel' not found in panels.txt
```
- Panel names are **case-sensitive**
- Copy/paste from `panels.txt` to avoid typos

**"No JSON file found"**
```
[ERROR] No valid JSON file found in directory
```
- Ensure exactly ONE `.json` file is in the Label Set folder
- Check the JSON is valid (no syntax errors)

**"Multiple JSON files found"**
```
[ERROR] Multiple JSON files found. Please keep only one.
```
- Remove extra JSON files, keep only your target aircraft

### 10.2 Compilation Errors

**"PIN not declared"**
- Ensure `#include "Config.h"` is present
- The `PIN()` macro is defined in the config

**"Duplicate label"**
- Check for duplicate entries in InputMapping or LEDMapping
- Each label must be unique

### 10.3 Runtime Issues

**"LED label not found"**
- The label in your code doesn't match LEDMapping.h
- Labels are case-sensitive

**"TM1637: no device for CLK/DIO"**
- The TM1637 device wasn't initialized
- Ensure at least one TM1637 LED entry exists in LEDMapping.h

**Selector not responding**
- Check all positions share the same **group** ID
- Verify the fallback position has `bit=-1`
- Ensure the correct `source` is set

**LED always on/off**
- Check `activeLow` flag matches your hardware
- Verify the GPIO is not used elsewhere

### 10.4 Debugging Tips

1. **Enable DEBUG mode** in Config.h:
   ```cpp
   #define DEBUG 1
   ```

2. **Check serial output** for initialization messages:
   ```
   [INIT] GPIO LED SPIN_LT (GPIO 34, Digital) -> OUTPUT
   [INIT] TM1637 device initialized: CLK=9 DIO=8
   ```

3. **Use the test Label Set** (`LABEL_SET_TEST_ONLY`) to verify basic functionality before building your custom set.

---

## 11. Next Steps

After creating your Label Set:

1. **Compile and upload** the firmware in Arduino IDE
2. **Test each input** using DCS-BIOS debug tools (Bort or BIOSBuddy)
3. **Verify LED responses** by manipulating cockpit controls in DCS
4. **Fine-tune** your InputMapping and LEDMapping as needed
5. **Add more panels** by editing `selected_panels.txt` and regenerating

### Advanced Topics

- **Display panels** (IFEI, UFC): See the Display documentation
- **Custom panel code**: Check `src/Panels/` for examples
- **Hardware debugging**: Use the I²C scanner and GPIO test functions

### Resources

- **DCS-BIOS Documentation**: https://github.com/DCS-Skunkworks/dcs-bios/wiki
- **CockpitOS GitHub**: https://github.com/BojoteX/CockpitOS
- **Bort Reference Tool**: https://github.com/DCS-Skunkworks/Bort
- **BIOSBuddy**: https://github.com/DCS-Skunkworks/BIOSBuddy

---

**Congratulations!** You now have the knowledge to create custom Label Sets for any DCS aircraft. Start simple, test often, and build your cockpit one panel at a time.
