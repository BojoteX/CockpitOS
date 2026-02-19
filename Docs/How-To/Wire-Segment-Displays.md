# How To: Wire Segment Displays

HT1622 segment LCD panels are multiplexed display drivers used for large LCD panels like the IFEI (Integrated Fuel/Engine Indicator), UFC scratchpad, and other alphanumeric readouts. Each visible character is formed by controlling individual LCD segments mapped to specific bits in the HT1622's RAM.

---

## What HT1622 Segment LCDs Are

The HT1622 is a RAM-mapped LCD controller. It has 256 bits of display RAM, and each bit controls one LCD segment. By writing the right pattern of bits, you can form numbers, letters, and symbols on the LCD panel.

Unlike a 7-segment LED display (which lights up segments), an HT1622 LCD panel uses voltage to make segments visible or invisible. The controller handles the LCD multiplexing internally -- you just write to RAM.

**Use cases:** IFEI fuel/engine displays, UFC scratchpad, radio frequency readouts, instrument panel readouts.

---

## What You Need

| Item | Notes |
|---|---|
| HT1622-based LCD module | The physical LCD with its driver board |
| ESP32 board | Any variant |
| Hookup wire | 3 signal wires (CS, WR, DATA) + power and ground |

---

## Step 1: Wire the HT1622 Module

HT1622 modules use a 3-wire SPI-like interface:

| Signal | Description | Notes |
|---|---|---|
| CS | Chip Select | Active LOW. Selects this HT1622 chip. |
| WR | Write/Clock | Clock signal for data transfer |
| DATA | Serial data | Data bits shifted in on WR edges |
| VCC | Power | 3.3V or 5V (check your module) |
| GND | Ground | Common ground |

```
                    ESP32                       HT1622 Module
                 +---------+                  +-----------+
                 |         |                  |           |
                 | GPIO 36 +------------------| CS        |
                 | GPIO 35 +------------------| WR        |
                 | GPIO 34 +------------------| DATA      |
                 |         |                  |           |
                 |  3.3V   +------------------| VCC       |
                 |   GND   +------------------| GND       |
                 |         |                  |           |
                 +---------+                  |   LCD     |
                                              |  Panel    |
                                              +-----------+
```

### Multiple HT1622 Chips

Larger displays (like the IFEI) use two HT1622 chips -- one for each half of the LCD. Each chip has its own CS, WR, and DATA pins:

```cpp
// In CustomPins.h (IFEI example)

// IFEI Left LCD (chip 0)
#define DATA0_PIN    PIN(34)
#define WR0_PIN      PIN(35)
#define CS0_PIN      PIN(36)

// IFEI Right LCD (chip 1)
#define DATA1_PIN    PIN(18)
#define WR1_PIN      PIN(21)
#define CS1_PIN      PIN(33)
```

---

## Step 2: Understand the Segment Map

This is the hardest part. Every HT1622 LCD panel has a unique mapping between its RAM addresses and the visible LCD segments. There is no standard -- each LCD panel manufacturer wires the segments differently.

### What a SegmentMap Entry Looks Like

Each segment on the LCD is identified by three values:

```cpp
struct SegmentMap {
    uint8_t addr;   // RAM address (0-63)
    uint8_t bit;    // Bit within that address (0-3)
    uint8_t ledID;  // Which HT1622 chip (0 or 1)
};
```

For example, a 7-segment digit needs 7 entries (segments a through g):

```
     aaaa
    f    b
    f    b
     gggg
    e    c
    e    c
     dddd
```

Each of those 7 segments maps to a specific `{addr, bit, chipID}` triple in the HT1622's RAM.

### Example: One Digit Mapping

```cpp
// Digit: segments a, b, c, d, e, f, g
const SegmentMap digit_map[7] = {
    {2, 3, 0},   // segment a -> chip 0, address 2, bit 3
    {2, 2, 0},   // segment b -> chip 0, address 2, bit 2
    {2, 1, 0},   // segment c -> chip 0, address 2, bit 1
    {2, 0, 0},   // segment d -> chip 0, address 2, bit 0
    {0, 1, 0},   // segment e -> chip 0, address 0, bit 1
    {0, 3, 0},   // segment f -> chip 0, address 0, bit 3
    {0, 2, 0},   // segment g -> chip 0, address 0, bit 2
};
```

---

## Step 3: Discover the Segment Map (RAM Walker)

If you do not already have a segment map for your LCD panel, you need to discover it. The process is called "RAM walking":

1. Initialize the HT1622 chip
2. Write a single bit HIGH in the display RAM (e.g., address 0, bit 0)
3. Observe which LCD segment lights up
4. Record the mapping: `{address, bit, chipID}` = segment X of digit Y
5. Clear that bit, advance to the next bit, and repeat
6. Continue through all 256 bits (64 addresses x 4 bits each)

This is tedious but only needs to be done once per LCD panel model. The RAM Walker tool (if available in the Label Creator) can automate the stepping process while you record which segments activate.

### Tips for RAM Walking

- Work one HT1622 chip at a time
- Start at address 0, bit 0 and work sequentially
- Take notes or photos as you go
- Group segments by digit -- once you find one segment of a digit, the others are usually nearby
- Some bits may not control any visible segment (unused RAM locations) -- mark those as `{0xFF, 0xFF, 0}`

---

## Step 4: Create SegmentMap Files

Once you have the mapping, encode it in a SegmentMap header file. These files live in your label set directory.

For the IFEI, the segment map file is `IFEI_SegmentMap.h`:

```cpp
#pragma once

// LEFT ENGINE RPM (3 digits: 100s single-segment + two 7-segment digits)
const SegmentMap IFEI_RPM_L_MAP[3][7] = {
    // 100s digit (single segment, only position [0] used)
    { {0,0,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, ... },
    // 10s digit (7-segment)
    { {2,3,0}, {2,2,0}, {2,1,0}, {2,0,0}, {0,1,0}, {0,3,0}, {0,2,0} },
    // 1s digit (7-segment)
    { {6,3,0}, {6,2,0}, {6,1,0}, {6,0,0}, {4,1,0}, {4,3,0}, {4,2,0} }
};
```

### Segment Map Conventions

- `{0xFF, 0xFF, chipID}` marks an unused segment position (padding)
- 7-segment digits have 7 entries per digit in order: a, b, c, d, e, f, g
- 14-segment (starburst) digits have 14 entries per digit
- Single-segment indicators (labels, bars) use just one `SegmentMap` entry
- Bar graphs use an array of `SegmentMap` entries (one per bar segment)

---

## Step 5: Register Display Fields in DisplayMapping.cpp

Each display field (e.g., "IFEI_FUEL_LEFT") maps a DCS-BIOS string export to a region of the segment map. These are defined in `DisplayMapping.cpp` for your label set.

The Label Creator's **Display Editor** manages this configuration. Each field definition includes:

| Field | Purpose |
|---|---|
| `label` | DCS-BIOS string label (e.g., "IFEI_FUEL_UP") |
| `segMap` | Pointer to the SegmentMap array for this field |
| `numDigits` | Number of digits in this field |
| `segsPerDigit` | Segments per digit (7 for 7-seg, 14 for 14-seg) |
| `renderType` | How to render: `FIELD_RENDER_7SEG`, `FIELD_RENDER_ALPHA_NUM_FUEL`, `FIELD_RENDER_BARGRAPH`, etc. |
| `renderFunc` | The rendering function (e.g., `renderIFEIDispatcher`) |
| `clearFunc` | The clearing function (e.g., `clearIFEIDispatcher`) |

### Using Label Creator

1. Open Label Creator and select your label set
2. Click **Edit Display** to open the Display Editor
3. Add display fields: set the DCS-BIOS label, digit count, segment type, and render method
4. Click **Edit Segment Map** to open the Segment Map Editor
5. Define or verify the RAM address/bit mappings for each segment
6. Save

---

## Step 6: Font Tables

CockpitOS includes built-in font tables for converting ASCII characters to segment patterns:

### 7-Segment Font

Supports digits 0-9, letters A-Z (with limitations), and common symbols. Each character is an 8-bit pattern where bits 0-6 map to segments a-g.

```
Character '0' -> 0x3F  (segments a,b,c,d,e,f)
Character '1' -> 0x06  (segments b,c)
Character '8' -> 0x7F  (segments a,b,c,d,e,f,g)
Character 'A' -> 0x77  (segments a,b,c,e,f,g)
```

### 14-Segment Font

Full alphanumeric support. Each character is a 16-bit pattern where bits 0-13 map to the 14 segments of a starburst display. Used for the IFEI fuel readout's rightmost character position.

---

## Step 7: Shadow Buffer and Commit

CockpitOS maintains a shadow buffer in RAM that mirrors the HT1622's display RAM. When DCS-BIOS data arrives:

1. The render function writes segment patterns into the shadow buffer
2. On each display loop cycle, the `commit()` function compares the shadow buffer with the last-committed state
3. Only changed addresses are written to the HT1622 hardware

This minimizes SPI bus traffic and prevents flicker.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| LCD is blank | HT1622 not initialized, wrong CS/WR/DATA pins | Verify pin definitions in CustomPins.h match your wiring; check power |
| Wrong segments light up | Segment map is incorrect | Re-run the RAM walker process to verify mappings |
| Characters display garbled | Segment order in map does not match your LCD | The a-g segment order in the map must match the physical LCD wiring; re-map if needed |
| Some digits work, others do not | Wrong chip ID in segment map | Verify the `ledID` field points to the correct HT1622 chip (0 or 1) |
| Display does not update | DCS-BIOS label name mismatch, display field not registered | Check that the label in DisplayMapping.cpp exactly matches the DCS-BIOS export name |
| Display flickers | Commit rate too high, or full-buffer writes every cycle | CockpitOS only commits changed addresses by default; check for bugs in custom render code |

---

## Further Reading

- [Displays Reference](../Hardware/Displays.md) -- Overview of all display types (TM1637, GN1640T, HT1622)
- [Advanced Display Pipeline](../Advanced/Display-Pipeline.md) -- Deep architectural details of the display system
- [TFT Gauges](Wire-TFT-Gauges.md) -- Graphical gauge alternative

---

*See also: [Hardware Overview](../Hardware/README.md) | [LEDs Reference](../Hardware/LEDs.md) | [Label Creator Tool](../Tools/Label-Creator.md)*
