# 74HC165 Shift Registers

When you run out of GPIO pins, 74HC165 shift registers let you read many inputs using just 3 GPIO wires. Each chip adds 8 inputs, and chips can be daisy-chained for up to 64 inputs total.

---

## What Shift Registers Do

The 74HC165 is a "Parallel-In, Serial-Out" shift register. It captures the state of 8 input pins simultaneously and shifts them out to the ESP32 one bit at a time over a serial interface.

```
+----------------------------------------------------------------------+
|  74HC165 SHIFT REGISTER                                              |
+----------------------------------------------------------------------+
|                                                                      |
|  +-------------------------------------------+                      |
|  |            74HC165                        |                      |
|  |                                           |                      |
|  |  D0 --|                        |-- VCC (3.3V)                    |
|  |  D1 --|                        |-- GND                           |
|  |  D2 --|                        |-- QH (Serial Out) --> ESP32     |
|  |  D3 --|                        |-- QH' (Cascade) --> Next chip   |
|  |  D4 --|                        |-- SH/LD (Load) <-- ESP32       |
|  |  D5 --|                        |-- CLK (Clock) <-- ESP32        |
|  |  D6 --|                        |-- CLK INH (tie to GND)         |
|  |  D7 --|                        |-- SER (Cascade In)             |
|  |                                           |                      |
|  +-------------------------------------------+                      |
|                                                                      |
|  D0-D7: Connect your buttons/switches here (active LOW with GND)    |
|                                                                      |
|  OPERATION:                                                          |
|  1. ESP32 pulses SH/LD LOW --> chip captures all 8 inputs           |
|  2. ESP32 pulses CLK --> chip shifts out one bit on QH              |
|  3. Repeat step 2 for all 8 bits                                    |
|                                                                      |
+----------------------------------------------------------------------+
```

---

## Daisy-Chaining Multiple Chips

Multiple 74HC165s share the same LOAD and CLK lines. Data cascades from one chip to the next through the QH' (cascade out) to SER (cascade in) connection.

```
              +---------+      +---------+      +---------+
   ESP32      | HC165   |      | HC165   |      | HC165   |
              |  #1     |      |  #2     |      |  #3     |
   PL  ------>| SH/LD   |----->| SH/LD   |----->| SH/LD   |  (shared)
   CLK ------>| CLK     |----->| CLK     |----->| CLK     |  (shared)
   QH <-------| QH      |      | QH      |      | QH      |
              |     QH' |----->| SER     |      |         |
              |         |      |     QH' |----->| SER     |
              |  D0-D7  |      |  D0-D7  |      |  D0-D7  |
              +---------+      +---------+      +---------+
                Bits 0-7        Bits 8-15       Bits 16-23
```

**Bit numbering:**

| Chip | Input Pins | Bit Range  |
|------|------------|------------|
| #1   | D0 - D7    | 0 - 7      |
| #2   | D0 - D7    | 8 - 15     |
| #3   | D0 - D7    | 16 - 23    |
| #4   | D0 - D7    | 24 - 31    |
| ...  | ...        | ...        |
| #8   | D0 - D7    | 56 - 63    |

CockpitOS supports up to 8 chips (64 bits) in a single chain using a 64-bit internal register.

---

## Wiring

### GPIO Connections

The shift register chain uses 3 GPIO pins from the ESP32:

| Signal  | ESP32 Pin         | HC165 Pin    | Description                    |
|---------|-------------------|-------------|--------------------------------|
| LOAD    | Any GPIO          | SH/LD (pin 1) | Parallel load (active LOW)   |
| CLK     | Any GPIO          | CLK (pin 2)   | Clock signal                 |
| DATA    | Any GPIO          | QH (pin 9)    | Serial data output           |

### Wiring Diagram (16 inputs, 2 chips)

```
                    ESP32                  HC165 #1         HC165 #2
                 +---------+             +---------+      +---------+
                 |         |             |         |      |         |
                 | GPIO 39 +------------>| SH/LD   +----->| SH/LD   |
                 |  (LOAD) |             |         |      |         |
                 |         |             |         |      |         |
                 | GPIO 38 +------------>| CLK     +----->| CLK     |
                 |  (CLK)  |             |         |      |         |
                 |         |             |         |      |         |
                 | GPIO 40 +<------------| QH      |      |         |
                 |  (DATA) |             |    QH'  +----->| SER     |
                 |         |             |         |      |         |
                 |   GND   +-------------| GND     +------| GND     |
                 |         |             | CLK INH |      | CLK INH |
                 |  3.3V   +-------------| VCC     +------| VCC     |
                 |         |             |         |      |         |
                 +---------+             +---------+      +---------+

   Connect buttons between D0-D7 pins and GND (active LOW).
   Tie CLK INH to GND on all chips.
   Tie unused SER input (first chip) to GND.
```

### Input Wiring

Each input pin (D0-D7) on the 74HC165 connects to one terminal of a button or switch. The other terminal connects to GND. The 74HC165 has internal pull-up capability, but for reliability you may add external 10k ohm pull-up resistors from each D pin to VCC.

---

## CustomPins.h Configuration

Enable the shift register and define the GPIO pins in your Label Set's `CustomPins.h`:

```cpp
// In src/LABELS/LABEL_SET_YOURPANEL/CustomPins.h

#define HC165_BITS              16       // Total bits (8 per chip x number of chips)
#define HC165_CONTROLLER_PL     PIN(39)  // Parallel Load pin
#define HC165_CONTROLLER_CP     PIN(38)  // Clock pin
#define HC165_CONTROLLER_QH     PIN(40)  // Data output pin
```

These definitions are set through the **Custom Pins Editor** in the Label Creator tool.

---

## InputMapping.h Configuration

### Momentary Button on HC165

```cpp
//  label              source   port  bit  hidId  DCSCommand         value  Type        group
{ "MASTER_ARM_BTN",   "HC165",   0,    2,   -1,  "MASTER_ARM_BTN",     1,  "momentary",   0 },
```

### Selector Group on HC165

```cpp
{ "ENGINE_CRANK_LEFT",  "HC165", 0,  0, -1, "ENGINE_CRANK_SW", 2, "selector", 4 },
{ "ENGINE_CRANK_OFF",   "HC165", 0, -1, -1, "ENGINE_CRANK_SW", 1, "selector", 4 },
{ "ENGINE_CRANK_RIGHT", "HC165", 0,  1, -1, "ENGINE_CRANK_SW", 0, "selector", 4 },
```

**Field reference:**

| Field  | Value    | Meaning                                              |
|--------|----------|------------------------------------------------------|
| source | `"HC165"` | Identifies this as a shift register input            |
| port   | `0`      | Always 0 for HC165 (reserved for future use)         |
| bit    | `0-63`   | Bit position in the chain (see bit numbering table)  |
| bit    | `-1`     | Fallback position for selectors (no bit is LOW)      |

### Bit Position Reference

Within each chip, D0 is the lowest bit and D7 is the highest:

```
Chip #1:  D0=bit 0, D1=bit 1, D2=bit 2, ... D7=bit 7
Chip #2:  D0=bit 8, D1=bit 9, D2=bit 10, ... D7=bit 15
Chip #3:  D0=bit 16, D1=bit 17, ...
```

---

## Label Creator Configuration

1. Open Label Creator and select your Label Set
2. Click **Edit Custom Pins** to define the HC165 GPIO assignments (LOAD, CLK, DATA) and bit count
3. Click **Edit Inputs**
4. For each button on the shift register chain, set:
   - **Source** = `HC165`
   - **Port** = `0`
   - **Bit** = the bit position of that button in the chain
5. Save

---

## When to Use HC165 vs PCA9555

| Feature        | 74HC165              | PCA9555              |
|----------------|----------------------|----------------------|
| Direction      | Input only           | Input AND Output     |
| Interface      | 3-wire (SPI-like)    | I2C (2-wire shared)  |
| Pins per chip  | 8                    | 16                   |
| Max chips      | 8 (64 inputs)        | 8 (128 I/O)          |
| GPIO cost      | 3 (dedicated)        | 2 (shared I2C bus)   |
| Speed          | Fast (direct clocking)| Moderate (I2C protocol overhead) |
| LED output     | No                   | Yes                  |
| Best for       | Button-heavy panels  | Mixed I/O panels     |

**Choose HC165 when:**
- You only need inputs (no LED outputs on the expander)
- You want the fastest possible scan rate
- You have 3 spare GPIO pins to dedicate

**Choose PCA9555 when:**
- You need both inputs and outputs on the same expander
- You are already using I2C for other devices
- You want to minimize dedicated GPIO usage

---

## Debugging

Enable targeted debug output to map physical buttons to bit positions:

```cpp
// In Config.h
#define DEBUG_ENABLED_FOR_HC165_ONLY    1
```

This outputs the raw 64-bit value whenever any HC165 input changes:

```
[HC165] Raw: 0x0000000000000004 (bit 2 active)
[HC165] Raw: 0x0000000000000000 (all released)
```

Press each button one at a time and note the bit number that activates. This is the value you use for the `bit` field in InputMapping.h.

---

## Limits

| Parameter             | Value  |
|-----------------------|--------|
| Max chips in chain    | 8      |
| Max total inputs      | 64     |
| GPIO pins required    | 3      |
| Bits per chip         | 8      |
| Supply voltage        | 3.3V   |

---

*See also: [Hardware Overview](README.md) | [I2C Expanders](I2C-Expanders.md) | [Buttons and Switches](Buttons-Switches.md)*
