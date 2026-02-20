# Displays

CockpitOS supports several display and LED driver ICs for numeric readouts, annunciator panels, and segment LCDs. This page covers TM1637, GN1640T, and HT1622 devices. For graphical TFT displays, see [TFT Gauges](TFT-Gauges.md).

---

## TM1637 7-Segment Displays

The TM1637 is a versatile chip commonly found on 4-digit and 6-digit 7-segment display modules. In CockpitOS, TM1637 is used for two purposes: driving individual LED segments (annunciator panels) and reading button inputs through its built-in key scanning capability.

### Wiring

TM1637 modules use a 2-wire interface (CLK + DIO) plus power:

```
                    ESP32                       TM1637 Module
                 +---------+                  +---------------+
                 |         |                  |  +-+-+-+-+    |
                 | GPIO 37 +------------------| CLK         |
                 |  (CLK)  |                  |  |8|8|8|8|    |
                 |         |                  |  +-+-+-+-+    |
                 | GPIO 39 +------------------| DIO         |
                 |  (DIO)  |                  |               |
                 |         |                  |               |
                 |  3.3V   +------------------| VCC           |
                 |   GND   +------------------| GND           |
                 |         |                  +---------------+
                 +---------+
```

Each TM1637 device is identified by its DIO pin in the configuration. The CLK pin can optionally be shared across multiple TM1637 devices (each with a different DIO pin), or each device can have its own CLK.

### TM1637 as LED Output

In CockpitOS annunciator panels, each individual LED is mapped to a specific segment and bit within the TM1637's display RAM. This is how the left and right advisory panels on the F/A-18C are driven, for example.

**LEDMapping.h configuration:**

```cpp
//  label           deviceType    info                                         dimmable  activeLow
{ "LH_ADV_GO",     DEVICE_TM1637, { .tm1637Info = { PIN(37), PIN(39), 0, 0 } }, false,   false },
{ "LH_ADV_NO_GO",  DEVICE_TM1637, { .tm1637Info = { PIN(37), PIN(39), 0, 1 } }, false,   false },
{ "LH_ADV_L_BLEED",DEVICE_TM1637, { .tm1637Info = { PIN(37), PIN(39), 1, 0 } }, false,   false },
```

**Field reference for tm1637Info:**

| Field   | Example    | Meaning                                     |
|---------|------------|---------------------------------------------|
| clkPin  | `PIN(37)`  | GPIO connected to CLK                       |
| dioPin  | `PIN(39)`  | GPIO connected to DIO (identifies the device)|
| segment | `0`        | Grid/digit position (0-5 typically)          |
| bit     | `0`        | Segment within that digit (0-7)              |

### TM1637 as Button Input

The TM1637 chip includes a key-scanning matrix that CockpitOS can read. This is useful for buttons that are physically mounted on TM1637-connected panels (like the fire buttons on the F/A-18C advisory panels).

**InputMapping.h configuration:**

```cpp
//  label            source    port      bit  hidId  DCSCommand         value  Type        group
{ "LEFT_FIRE_BTN",  "TM1637", PIN(39),   0,   -1,  "LEFT_FIRE_BTN",      1,  "momentary",   0 },
{ "APU_FIRE_BTN",   "TM1637", PIN(40),   0,   -1,  "APU_FIRE_BTN",       1,  "momentary",   0 },
```

**Field reference:**

| Field  | Value      | Meaning                                          |
|--------|------------|--------------------------------------------------|
| source | `"TM1637"` | Identifies TM1637 key input                     |
| port   | `PIN(39)`  | DIO pin (identifies which TM1637 device)         |
| bit    | `0`        | Key scan code returned by the module             |

### Brightness Control

TM1637 supports 8 brightness levels (0-7). CockpitOS maps DCS-BIOS dimmer values (0-65535) to these levels automatically:

| Level | Duty Cycle | Appearance     |
|-------|------------|----------------|
| 0     | 1/16       | Very dim       |
| 1     | 2/16       | Dim            |
| 2     | 4/16       | Low            |
| 3     | 6/16       | Medium-low     |
| 4     | 8/16       | Medium         |
| 5     | 10/16      | Medium-high    |
| 6     | 12/16      | Bright         |
| 7     | 14/16      | Maximum        |

### Multi-Device Support

CockpitOS supports up to **8 TM1637 devices** for LED output and up to **4 devices** for key input scanning. Each device is automatically registered when first referenced in LEDMapping.h or InputMapping.h. Devices are identified by their DIO pin.

```cpp
// Device 1: CLK=PIN(37), DIO=PIN(39) -- Left Advisory Panel
// Device 2: CLK=PIN(37), DIO=PIN(40) -- Right Advisory Panel
```

Each device can control up to 6 digits x 8 segments = 48 individual LEDs.

### Debugging TM1637 Inputs

For key scanning issues (ghosting, false triggers):

```cpp
// In Config.h
#define DEBUG_ENABLED_FOR_TM1637_ONLY    1   // Show raw key scan data
#define ADVANCED_TM1637_INPUT_FILTERING  1   // Extra debouncing for noisy modules
```

---

## GN1640T LED Matrix Drivers

The GN1640T drives LED matrices -- useful for annunciator panels with many individual indicator lights arranged in a grid pattern.

### How It Works

The GN1640T can drive up to 8 columns x 8 rows = 64 individual LEDs. Each LED is addressed by its column and row position.

```
        Col 0   Col 1   Col 2   Col 3   Col 4   Col 5   Col 6   Col 7
       +-----+-----+-----+-----+-----+-----+-----+-----+
 Row 0 | LED | LED | LED | LED | LED | LED | LED | LED |
       +-----+-----+-----+-----+-----+-----+-----+-----+
 Row 1 | LED | LED | LED | LED | LED | LED | LED | LED |
       +-----+-----+-----+-----+-----+-----+-----+-----+
  ...  | ... | ... | ... | ... | ... | ... | ... | ... |
       +-----+-----+-----+-----+-----+-----+-----+-----+
 Row 7 | LED | LED | LED | LED | LED | LED | LED | LED |
       +-----+-----+-----+-----+-----+-----+-----+-----+
```

### Wiring

The GN1640T uses a 2-wire interface (CLK + DIO) similar to TM1637:

```
                    ESP32                       GN1640T
                 +---------+                  +---------+
                 |         |                  |         |
                 | GPIO 10 +------------------| CLK     |
                 |         |                  |         |
                 | GPIO 11 +------------------| DIO     |
                 |         |                  |         |
                 |  3.3V   +------------------| VCC     |
                 |   GND   +------------------| GND     |
                 |         |                  |         |
                 +---------+                  +---------+
```

### LEDMapping.h Configuration

```cpp
//  label            deviceType     info                                dimmable  activeLow
{ "WARN_APU",       DEVICE_GN1640T, { .gn1640Info = { 0, 0, 3 } },     false,    false },
{ "WARN_ENGINE_L",  DEVICE_GN1640T, { .gn1640Info = { 0, 1, 3 } },     false,    false },
{ "WARN_ENGINE_R",  DEVICE_GN1640T, { .gn1640Info = { 0, 2, 3 } },     false,    false },
```

**Field reference for gn1640Info:**

| Field   | Example | Meaning                                    |
|---------|---------|--------------------------------------------|
| address | `0`     | Device address (for multiple GN1640T chips) |
| column  | `0`     | Column position (0-7)                       |
| row     | `3`     | Row position (0-7)                          |

### Shadow Buffer and Refresh

CockpitOS maintains a shadow buffer for each GN1640T device. Only columns that have changed are updated on the bus, minimizing traffic and preventing flicker. A minimum 2ms delay between updates prevents overwhelming the chip during fast DCS-BIOS data streams.

---

## HT1622 Segment LCDs

HT1622-based segment LCDs are large multi-digit LCD panels used for displays like the IFEI (Integrated Fuel/Engine Indicator) and UFC (Up-Front Controller) on the F/A-18C. These displays show numeric and limited alphanumeric characters by controlling individual LCD segments.

### Overview

Unlike TM1637 (which drives individual LEDs), HT1622 drives LCD segments. Each character on the display is formed by combining multiple segments, much like a traditional 7-segment display but often with more complex segment patterns for alphanumeric capability.

### Segment Mapping

Each HT1622 display requires a **SegmentMap** file that maps the HT1622's display RAM addresses to visual character positions and segment patterns. This mapping is specific to the physical LCD panel being used.

- SegmentMap files are stored as `{PREFIX}_SegmentMap.h` in the Label Set directory (e.g., `IFEI_SegmentMap.h`)
- The mapping defines how RAM bits correspond to visible LCD segments
- Different LCD panels have different segment layouts, even with the same HT1622 driver

### Configuration via Label Creator

HT1622 segment LCDs are configured through two specialized editors in the Label Creator:

1. **Display Editor** -- Defines the display layout, character positions, and DCS-BIOS data sources
2. **Segment Map Editor** -- Maps HT1622 RAM addresses to physical LCD segments

For a detailed step-by-step guide to setting up HT1622 segment LCDs, see the How-To documentation on segment LCD setup.

### Wiring

HT1622 modules typically use a 3-wire SPI-like interface:

| Signal | Description          |
|--------|----------------------|
| CS     | Chip Select          |
| WR     | Write/Clock          |
| DATA   | Serial data          |
| VCC    | Power (3.3V or 5V)   |
| GND    | Ground               |

Pin assignments are defined in CustomPins.h for each Label Set.

---

## Label Creator Configuration (All Display Types)

The Label Creator provides a unified interface for configuring all display types:

1. Open Label Creator and select your Label Set
2. **For TM1637 / GN1640T LEDs**: Click **Edit LEDs** and set the device type, pin assignments, and segment/bit positions
3. **For TM1637 key inputs**: Click **Edit Inputs** and set Source = TM1637 with the DIO pin and key code
4. **For HT1622 segment LCDs**: Use the **Display Editor** and **Segment Map Editor** tools
5. **For Custom Pins**: Click **Edit Custom Pins** to define CLK/DIO/CS pins used by display modules

---

## Display Device Summary

| Device         | Type            | Interface | Max per Panel | Best For                    |
|----------------|-----------------|-----------|---------------|-----------------------------|
| TM1637         | LED driver      | 2-wire    | 8             | Annunciator LEDs, key input |
| GN1640T        | LED matrix      | 2-wire    | Multiple      | Dense LED grids             |
| HT1622         | Segment LCD     | 3-wire    | Multiple      | IFEI, UFC numeric displays  |
| TFT (various)  | Graphical LCD   | SPI       | Multiple      | Gauge faces, complex graphics |
| Servo (GAUGE)  | Analog needle   | PWM       | Multiple      | Physical gauge instruments  |

For TFT graphical displays, see [TFT Gauges](TFT-Gauges.md).
For servo-driven analog gauges, see [Servo Gauges](Servo-Gauges.md).

---

*See also: [Hardware Overview](README.md) | [LEDs](LEDs.md) | [TFT Gauges](TFT-Gauges.md)*
