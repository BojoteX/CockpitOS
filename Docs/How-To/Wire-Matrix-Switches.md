# How To: Wire Matrix Switches

Matrix scanning reads multi-position rotary switches using fewer GPIO pins than direct wiring. By scanning rows (strobes) and reading columns (data), you can decode many switch positions from a handful of pins -- essential for radio channel selectors, mode knobs, and other multi-position rotary switches.

---

## What Matrix Scanning Is

In a matrix configuration, one set of GPIO pins (strobes) are driven as outputs, and one pin (data) is read as an input. The rotary switch wiper connects to the data pin, and each switch position connects to a different strobe pin. By activating one strobe at a time and reading the data pin, CockpitOS determines which position the switch is in.

```
+----------------------------------------------------------------------+
|  MATRIX SCANNING CONCEPT                                              |
+----------------------------------------------------------------------+
|                                                                      |
|   Strobe pins (outputs)          Data pin (input)                    |
|                                                                      |
|   STROBE_1 ---+                                                      |
|               |                                                      |
|   STROBE_2 ---+--- Rotary switch positions                          |
|               |         |                                            |
|   STROBE_3 ---+         +--- Common wiper ---> DATA pin              |
|               |                                                      |
|   STROBE_4 ---+                                                      |
|                                                                      |
|   Scan sequence:                                                     |
|   1. Set STROBE_1 LOW, all others HIGH. Read DATA. If LOW -> pos 1  |
|   2. Set STROBE_2 LOW, all others HIGH. Read DATA. If LOW -> pos 2  |
|   3. Set STROBE_3 LOW, all others HIGH. Read DATA. If LOW -> pos 3  |
|   4. Set STROBE_4 LOW, all others HIGH. Read DATA. If LOW -> pos 4  |
|                                                                      |
+----------------------------------------------------------------------+
```

### Why Use Matrix Scanning

A 12-position rotary switch wired directly needs 12 GPIO pins. With matrix scanning using 4 strobe pins and 1 data pin, you need only 5 GPIO pins to read all 12 positions (4 strobes x 3 data lines = 12 positions). For CockpitOS, the typical configuration uses one data pin per rotary group.

---

## What You Need

| Item | Notes |
|---|---|
| Multi-position rotary switch | Common wiper + one contact per position |
| ESP32 GPIO pins | One per strobe line + one data pin per rotary group |
| Hookup wire | Connect switch positions to strobe pins, wiper to data pin |

---

## Step 1: Wire the Rotary Switch

### Example: 5-Position Rotary Switch with 4 Strobes

```
+----------------------------------------------------------------------+
|  ROTARY SWITCH WIRING (5-position example)                            |
+----------------------------------------------------------------------+
|                                                                      |
|                Rotary Switch                                         |
|              +-----+-----+                                           |
|              |  1  |  2  |  3  |  4  |  5  |                         |
|              +--+--+--+--+--+--+--+--+--+--+                         |
|                 |     |     |     |     |                             |
|   ESP32         |     |     |     |     |                             |
|  +--------+     |     |     |     |     |                             |
|  | STB_1  +-----+     |     |     |     |  Position 1 -> strobe 1    |
|  | STB_2  +-----------+     |     |     |  Position 2 -> strobe 2    |
|  | STB_3  +-----------------+     |     |  Position 3 -> strobe 3    |
|  | STB_4  +-----------------------+     |  Position 4 -> strobe 4    |
|  |        |                       |     |  Position 5 -> fallback    |
|  |        |                       |     |  (no strobe active)        |
|  |  DATA  +-------- wiper --------+-----+                            |
|  |        |   (common terminal)                                      |
|  +--------+                                                          |
|                                                                      |
|  DATA pin uses INPUT_PULLUP internally.                              |
|  When a strobe goes LOW and the wiper is on that position,           |
|  DATA reads LOW.                                                     |
|                                                                      |
+----------------------------------------------------------------------+
```

Each switch position connects one of the fixed contacts to the common wiper. The wiper connects to the DATA pin. Each fixed contact connects to a strobe GPIO pin.

---

## Step 2: Define Pins in CustomPins.h

Define the strobe and data pin assignments in your label set's `CustomPins.h`:

```cpp
// Matrix switch pins for ALR-67 display type selector
#define ALR67_STROBE_1     PIN(16)
#define ALR67_STROBE_2     PIN(17)
#define ALR67_STROBE_3     PIN(21)
#define ALR67_STROBE_4     PIN(37)
#define ALR67_DataPin      PIN(38)
```

---

## Step 3: Configure in InputMapping.h

Matrix rotary switches use `source="MATRIX"` in InputMapping.h. Each rotary group needs:

1. **One anchor row** -- identifies the data pin and the full strobe mask
2. **One row per position** -- identifies which strobe pin maps to which DCS command value

### InputMapping.h Example

```cpp
//  label                   source    port              bit  hidId  DCSCommand           value  Type        group
// Anchor row: port=DataPin, bit=all-ones mask (15 = 0b1111 for 4 strobes)
{ "RWR_DIS_TYPE_SW_F",    "MATRIX", ALR67_DataPin,     15,   -1,  "RWR_DIS_TYPE_SW",     4,  "selector",    6 },

// Position rows: port=strobe GPIO, bit=one-hot strobe index
{ "RWR_DIS_TYPE_SW_U",    "MATRIX", ALR67_STROBE_1,     1,   -1,  "RWR_DIS_TYPE_SW",     3,  "selector",    6 },
{ "RWR_DIS_TYPE_SW_A",    "MATRIX", ALR67_STROBE_2,     2,   -1,  "RWR_DIS_TYPE_SW",     2,  "selector",    6 },
{ "RWR_DIS_TYPE_SW_I",    "MATRIX", ALR67_STROBE_3,     4,   -1,  "RWR_DIS_TYPE_SW",     1,  "selector",    6 },
{ "RWR_DIS_TYPE_SW_N",    "MATRIX", ALR67_STROBE_4,     8,   -1,  "RWR_DIS_TYPE_SW",     0,  "selector",    6 },
```

### How the Fields Map

| Row Type | `source` | `port` | `bit` | Meaning |
|---|---|---|---|---|
| Anchor | `"MATRIX"` | Data pin GPIO | All-ones mask (e.g., 15 for 4 strobes) | Identifies the data pin and strobe count. This row is also the fallback position (when no strobe is active). |
| Position | `"MATRIX"` | Strobe pin GPIO | One-hot strobe index (1, 2, 4, 8, ...) | Maps a specific strobe to a DCS command value |

The `bit` field on position rows is a one-hot encoding of the strobe index:
- Strobe 0 = bit 1 (2^0)
- Strobe 1 = bit 2 (2^1)
- Strobe 2 = bit 4 (2^2)
- Strobe 3 = bit 8 (2^3)

All rows in the same rotary group share the same `group` number and the same `oride_label` (DCS command).

---

## Step 4: Configure via Label Creator

1. Open Label Creator and select your label set
2. Click **Edit Inputs**
3. For each rotary switch position:
   - Set **Source** = `MATRIX`
   - Set **Port** = the appropriate GPIO (data pin for anchor, strobe pin for positions)
   - Set **Bit** = the strobe mask (all-ones for anchor, one-hot for positions)
   - Set **DCS Command** = the DCS-BIOS command name
   - Set **Value** = the DCS command value for that position
   - Set **Type** = `selector`
   - Set **Group** = a unique group number shared by all positions of this rotary
4. Save

---

## Step 5: How CockpitOS Scans

CockpitOS auto-discovers matrix rotaries from InputMapping.h at startup. The scanning algorithm:

1. Identifies all entries with `source="MATRIX"` and groups them by label family (common prefix)
2. Configures strobe pins as outputs (HIGH by default) and the data pin as input with pull-up
3. On each scan cycle (~250Hz):
   - Drives each strobe LOW one at a time
   - Reads the data pin
   - If data reads LOW, that strobe position is active
   - If no strobe reads LOW, the fallback position is active
4. Debounces the result before sending the DCS command

The scanning is fully automatic -- you only need to define the mappings in InputMapping.h.

---

## Example: 12-Position Rotary with 4 Strobes + 3 Data Lines

For a 12-position switch, you can use 4 strobe pins and 3 data pins:

```
        Strobe 0   Strobe 1   Strobe 2   Strobe 3
Data 0:  Pos 0      Pos 1      Pos 2      Pos 3
Data 1:  Pos 4      Pos 5      Pos 6      Pos 7
Data 2:  Pos 8      Pos 9      Pos 10     Pos 11
```

This gives 12 positions from 7 GPIO pins (4 strobe + 3 data). In CockpitOS, each data line would be a separate rotary group in InputMapping.h, all sharing the same DCS command but with different group IDs.

**Note:** CockpitOS supports up to 8 matrix rotary groups and up to 8 strobe pins per group, with up to 16 positions per group.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Wrong position detected | Matrix wiring is shifted by one | Check that each switch position connects to the correct strobe pin; verify the one-hot bit values in InputMapping.h |
| Ghost readings (multiple positions active) | Missing pull-up, wiring short | Ensure the data pin has `INPUT_PULLUP` (handled automatically by CockpitOS); check for shorts between strobe lines |
| Position never detected | Strobe pin not configured, wrong data pin | Verify the GPIO pins in CustomPins.h match your physical wiring |
| Fallback position always active | Data pin not connected to switch wiper | Verify the common wiper terminal connects to the data pin GPIO |
| Switch bounces between positions | Mechanical contact bounce | CockpitOS applies debouncing automatically; if still an issue, clean the switch contacts |

---

*See also: [Buttons and Switches Reference](../Hardware/Buttons-Switches.md) | [Control Types Reference](../Reference/Control-Types.md) | [Config Reference](../Reference/Config.md)*
