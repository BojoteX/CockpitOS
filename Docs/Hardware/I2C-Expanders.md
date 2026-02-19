# PCA9555 I2C GPIO Expanders

The PCA9555 adds 16 general-purpose I/O pins over the I2C bus. Unlike the 74HC165 (input only), PCA9555 pins can be used for both inputs (buttons, switches) and outputs (LEDs). Up to 8 chips can share the same I2C bus, giving you up to 128 additional I/O pins using just 2 GPIO wires.

---

## What PCA9555 Does

The PCA9555 is a 16-bit I2C GPIO expander organized as two 8-bit ports (Port 0 and Port 1). Each pin can be independently configured as input or output by the firmware.

```
+----------------------------------------------------------------------+
|  PCA9555 PINOUT                                                      |
+----------------------------------------------------------------------+
|                                                                      |
|  +-----------------------------------------------------+            |
|  |                    PCA9555                          |            |
|  |                                                     |            |
|  |  P0.0 --|                              |-- P1.0    |            |
|  |  P0.1 --|                              |-- P1.1    |            |
|  |  P0.2 --|     PORT 0        PORT 1     |-- P1.2    |            |
|  |  P0.3 --|    (8 pins)      (8 pins)    |-- P1.3    |            |
|  |  P0.4 --|                              |-- P1.4    |            |
|  |  P0.5 --|                              |-- P1.5    |            |
|  |  P0.6 --|                              |-- P1.6    |            |
|  |  P0.7 --|                              |-- P1.7    |            |
|  |                                                     |            |
|  |  A0, A1, A2 ---- Address select (0x20 + A2:A1:A0) |            |
|  |  SDA, SCL   ---- I2C bus connections               |            |
|  |  INT        ---- Interrupt output (optional)       |            |
|  |  VCC, GND   ---- Power supply (3.3V)               |            |
|  |                                                     |            |
|  +-----------------------------------------------------+            |
|                                                                      |
|  Port 0 pins = bits 0-7    |    Port 1 pins = bits 8-15            |
|                                                                      |
+----------------------------------------------------------------------+
```

---

## I2C Addressing

The PCA9555 address is determined by the A0, A1, and A2 pin connections:

| A2  | A1  | A0  | Address |
|-----|-----|-----|---------|
| GND | GND | GND | 0x20    |
| GND | GND | VCC | 0x21    |
| GND | VCC | GND | 0x22    |
| GND | VCC | VCC | 0x23    |
| VCC | GND | GND | 0x24    |
| VCC | GND | VCC | 0x25    |
| VCC | VCC | GND | 0x26    |
| VCC | VCC | VCC | 0x27    |

Each chip on the same I2C bus must have a unique address. With 8 addresses available (0x20-0x27), you can have up to 8 PCA9555 chips on one bus.

CockpitOS also supports chips at extended addresses (e.g., 0x5B) found on some modules. The driver handles any valid PCA9555 address.

---

## Wiring

### Basic Connection

```
                    ESP32                       PCA9555
                 +---------+                  +---------+
                 |         |                  |         |
                 |   SDA   +------------------| SDA     |
                 | (GPIO)  |                  |         |
                 |         |                  |         |
                 |   SCL   +------------------| SCL     |
                 | (GPIO)  |                  |         |
                 |         |                  |         |
                 |   GND   +------+-----------| GND     |
                 |         |      |           | A0      | (for address 0x20)
                 |         |      |           | A1      |
                 |         |      |           | A2      |
                 |         |      |           |         |
                 |  3.3V   +------+-----------| VCC     |
                 |         |                  |         |
                 +---------+                  +---------+
```

### I2C Pull-Up Resistors

The I2C bus requires pull-up resistors on both SDA and SCL lines. Use 4.7k ohm resistors connected to 3.3V. Many breakout boards include these already -- check before adding external ones (too many pull-ups will cause problems).

```
   3.3V ----+------+
             |      |
           [4.7k] [4.7k]
             |      |
   SDA ------+      |
   SCL -------------+
```

### Multiple Chips on One Bus

All PCA9555 chips share the same SDA and SCL lines. Only the address select pins (A0-A2) differ.

```
                ESP32
              +-------+
              |       |
   SDA -------| SDA   +------+----------+----------+
              |       |      |          |          |
   SCL -------| SCL   +------+----------+----------+
              |       |      |          |          |
              +-------+   +--+--+    +--+--+    +--+--+
                          | 0x20|    | 0x21|    | 0x22|
                          | PCA |    | PCA |    | PCA |
                          +-----+    +-----+    +-----+
```

---

## Enabling PCA9555 in CustomPins.h

Enable PCA9555 support and define the I2C pins in your Label Set's `CustomPins.h`:

```cpp
// In src/LABELS/LABEL_SET_YOURPANEL/CustomPins.h

#define ENABLE_PCA9555    1          // Enable PCA9555 support
#define SDA_PIN           PIN(8)     // I2C data pin
#define SCL_PIN           PIN(9)     // I2C clock pin
```

These settings are configured through the **Custom Pins Editor** in the Label Creator tool.

### Fast Mode

CockpitOS supports I2C Fast Mode (400kHz) for faster communication, enabled by default:

```cpp
// In Config.h
#define PCA_FAST_MODE     1    // 1 = 400kHz (default), 0 = 100kHz standard mode
```

Use standard mode (set to 0) if you experience communication errors with long I2C bus wires (over 50cm) or marginal pull-up resistors.

---

## Configuring Inputs in InputMapping.h

### Source Format

PCA9555 inputs use the source format `"PCA_0xNN"` where `0xNN` is the hexadecimal I2C address.

### Momentary Button

```cpp
//  label               source       port  bit  hidId  DCSCommand            value  Type        group
{ "SPIN_RECOV_COVER",  "PCA_0x20",    0,    0,   -1,  "SPIN_RECOV_COVER",      1,  "momentary",   0 },
```

### Selector Group

```cpp
{ "IR_COOL_SW_OFF",   "PCA_0x26", 1,  1, 23, "IR_COOL_SW", 0, "selector", 6 },
{ "IR_COOL_SW_NORM",  "PCA_0x26", 1, -1, 24, "IR_COOL_SW", 1, "selector", 6 },
{ "IR_COOL_SW_ORIDE", "PCA_0x26", 1,  0, 25, "IR_COOL_SW", 2, "selector", 6 },
```

**Field reference:**

| Field  | Value         | Meaning                                            |
|--------|---------------|----------------------------------------------------|
| source | `"PCA_0x26"`  | PCA9555 at I2C address 0x26                        |
| port   | `0` or `1`    | Port 0 (P0.x pins) or Port 1 (P1.x pins)          |
| bit    | `0-7`         | Pin within the port (P0.0-P0.7 or P1.0-P1.7)      |
| bit    | `-1`          | Fallback position for selectors                    |

### Port and Bit Mapping

```
Port 0:  P0.0 = bit 0, P0.1 = bit 1, ... P0.7 = bit 7
Port 1:  P1.0 = bit 0, P1.1 = bit 1, ... P1.7 = bit 7
```

When thinking in terms of the full 16-bit range:
- Port 0 = bits 0-7 of the chip
- Port 1 = bits 8-15 of the chip

But in InputMapping.h, `port` selects the port and `bit` is the position within that port (0-7).

---

## Configuring Outputs (LEDs) in LEDMapping.h

```cpp
//  label               deviceType     info                              dimmable  activeLow
{ "MASTER_MODE_AA_LT", DEVICE_PCA9555, { .pcaInfo = { 0x5B, 1, 3 } },   false,    true },
{ "MASTER_MODE_AG_LT", DEVICE_PCA9555, { .pcaInfo = { 0x5B, 1, 4 } },   false,    true },
```

**Field reference for pcaInfo:**

| Field   | Value  | Meaning                         |
|---------|--------|---------------------------------|
| address | `0x5B` | I2C address of the PCA9555      |
| port    | `1`    | Port 0 or Port 1                |
| bit     | `3`    | Pin within the port (0-7)       |

The `activeLow` flag works the same as for GPIO LEDs -- set it to `true` if the LED turns on when the output goes LOW.

---

## Label Creator Configuration

### Address Validation

The Label Creator validates PCA9555 addresses. Valid standard addresses are 0x20 through 0x27. Extended addresses (like 0x5B) are also accepted if your module uses them.

### Configuring Inputs

1. Open Label Creator and select your Label Set
2. Click **Edit Custom Pins** to enable PCA9555 and set I2C pins
3. Click **Edit Inputs**
4. For each PCA-connected button, set:
   - **Source** = `PCA_0xNN` (with the correct address)
   - **Port** = `0` or `1`
   - **Bit** = pin number within the port
5. Save

### Configuring Outputs

1. Click **Edit LEDs**
2. For each PCA-connected LED, set:
   - **Device** = `PCA9555`
   - Fill in the address, port, and bit fields
   - Set **Active Low** as appropriate for your wiring
3. Save

---

## Selector Groups with PCA Inputs

Multi-position selectors using PCA9555 inputs work exactly the same as GPIO selectors. Use the `group` field to link positions:

```cpp
{ "ENGINE_CRANK_LEFT",  "PCA_0x20", 0,  0, -1, "ENGINE_CRANK_SW", 2, "selector", 4 },
{ "ENGINE_CRANK_OFF",   "PCA_0x20", 0, -1, -1, "ENGINE_CRANK_SW", 1, "selector", 4 },
{ "ENGINE_CRANK_RIGHT", "PCA_0x20", 0,  1, -1, "ENGINE_CRANK_SW", 0, "selector", 4 },
```

All entries with the same group number form a selector. Only one position can be active at a time. The entry with `bit = -1` is the fallback reported when no pin is LOW.

---

## When to Use PCA9555 vs HC165

| Feature        | PCA9555              | 74HC165              |
|----------------|----------------------|----------------------|
| Direction      | Input AND Output     | Input only           |
| Interface      | I2C (2-wire shared)  | 3-wire (SPI-like)    |
| Pins per chip  | 16                   | 8                    |
| Max chips      | 8 (128 I/O)          | 8 (64 inputs)        |
| GPIO cost      | 2 (shared bus)       | 3 (dedicated)        |
| LED output     | Yes                  | No                   |
| Speed          | Moderate             | Fast                 |
| Best for       | Mixed I/O panels     | Button-heavy panels  |

**Choose PCA9555 when:**
- You need both inputs and LED outputs on the same expander
- You are already using I2C for other devices
- You want to minimize dedicated GPIO usage
- You need more than 64 inputs

**Choose HC165 when:**
- You only need inputs
- You want the fastest possible scan rate
- You have 3 GPIO pins to spare

---

## Debugging

Enable targeted debug output during initial wiring:

```cpp
// In Config.h
#define DEBUG_ENABLED_FOR_PCA_ONLY    1
```

This outputs port and bit information when PCA inputs change, helping you identify which physical switch corresponds to which address/port/bit combination.

---

## Limits

| Parameter                | Value  |
|--------------------------|--------|
| `MAX_PCAS`               | 8      |
| `MAX_PCA9555_INPUTS`     | 64     |
| `MAX_PCA_GROUPS`         | 128    |
| Standard address range   | 0x20 - 0x27 |
| Pins per chip            | 16 (2 ports of 8) |
| I2C speed (fast mode)    | 400 kHz |
| I2C speed (standard)     | 100 kHz |

---

*See also: [Hardware Overview](README.md) | [Shift Registers](Shift-Registers.md) | [LEDs](LEDs.md)*
