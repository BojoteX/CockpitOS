# CockpitOS — Hardware Wiring Guide

> **From theory to physical connections.**  
> This guide teaches you how electronic components work, how to wire them to your ESP32, and how to configure CockpitOS to recognize them. Whether you're a complete beginner or an experienced builder, this document has you covered.

---

## Table of Contents

1. [Electronics Fundamentals](#1-electronics-fundamentals)
2. [GPIO Basics](#2-gpio-basics)
3. [Buttons and Switches](#3-buttons-and-switches)
4. [LEDs — Light Emitting Diodes](#4-leds--light-emitting-diodes)
5. [Potentiometers and Analog Inputs](#5-potentiometers-and-analog-inputs)
6. [Rotary Encoders](#6-rotary-encoders)
7. [Expanding Inputs — 74HC165 Shift Registers](#7-expanding-inputs--74hc165-shift-registers)
8. [Expanding I/O — PCA9555 I²C Expanders](#8-expanding-io--pca9555-i2c-expanders)
9. [TM1637 Displays and Keypads](#9-tm1637-displays-and-keypads)
10. [WS2812 Addressable RGB LEDs](#10-ws2812-addressable-rgb-leds)
11. [GN1640T LED Matrix Drivers](#11-gn1640t-led-matrix-drivers)
12. [Matrix Rotary Switches](#12-matrix-rotary-switches)
13. [Servo Motors and Analog Gauges](#13-servo-motors-and-analog-gauges)
14. [The METADATA Override System](#14-the-metadata-override-system)
15. [Pin Mapping and Board Differences](#15-pin-mapping-and-board-differences)
16. [Power Considerations](#16-power-considerations)
17. [Troubleshooting Hardware](#17-troubleshooting-hardware)
18. [Quick Reference Tables](#18-quick-reference-tables)

---

## 1. Electronics Fundamentals

Before wiring anything, let's understand the basic concepts. If you're experienced, skip to [Section 2](#2-gpio-basics).

### 1.1 Voltage, Current, and Ground

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        THE WATER ANALOGY                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Think of electricity like water in pipes:                                 │
│                                                                             │
│   VOLTAGE (V) = Water pressure                                              │
│   - How hard the electricity "pushes"                                       │
│   - ESP32 operates at 3.3V logic levels                                     │
│   - Some peripherals need 5V                                                │
│                                                                             │
│   CURRENT (A) = Water flow rate                                             │
│   - How much electricity flows                                              │
│   - Measured in milliamps (mA) for our purposes                             │
│   - An LED typically uses 10-20mA                                           │
│                                                                             │
│   GROUND (GND) = The drain / return path                                    │
│   - Electricity must complete a circuit                                     │
│   - GND is the reference point (0V)                                         │
│   - All components share a common ground                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Digital vs Analog Signals

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      DIGITAL vs ANALOG                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   DIGITAL: Only two states — ON or OFF (1 or 0)                             │
│                                                                             │
│        HIGH ─────┐      ┌─────┐      ┌─────                                 │
│             3.3V │      │     │      │                                      │
│                  │      │     │      │                                      │
│         LOW ─────┴──────┘     └──────┘                                      │
│               0V                                                            │
│                                                                             │
│   Examples: Buttons, switches, LEDs (on/off)                                │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   ANALOG: Continuous range of values                                        │
│                                                                             │
│        3.3V ─────────╮                                                      │
│                      ╲                                                      │
│                       ╲    ╱╲                                               │
│                        ╲  ╱  ╲                                              │
│                         ╲╱    ╲                                             │
│          0V ─────────────────────────                                       │
│                                                                             │
│   Examples: Potentiometers, throttle axes, temperature sensors              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.3 Pull-Up and Pull-Down Resistors

When a GPIO pin is configured as an input but nothing is connected, it "floats" — its value is undefined and can randomly read HIGH or LOW due to electrical noise.

**Pull-up resistors** connect the pin to VCC (3.3V) through a resistor, so it reads HIGH by default. When you press a button that connects to GND, the pin reads LOW.

**Pull-down resistors** connect the pin to GND through a resistor, so it reads LOW by default. When you press a button that connects to VCC, the pin reads HIGH.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     PULL-UP RESISTOR (Most Common)                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│         3.3V ─────┬─────                                                    │
│                   │                                                         │
│                  [R] 10kΩ (internal or external)                            │
│                   │                                                         │
│         GPIO ─────┴─────┐                                                   │
│                         │                                                   │
│                        [BUTTON]                                             │
│                         │                                                   │
│         GND ────────────┘                                                   │
│                                                                             │
│   Button OPEN  → GPIO reads HIGH (3.3V through resistor)                    │
│   Button CLOSED → GPIO reads LOW (direct connection to GND)                 │
│                                                                             │
│   ⚠️ This is "ACTIVE LOW" — button press = LOW signal                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

> **✓ Good News**  
> ESP32 has **internal pull-up resistors** that CockpitOS enables automatically for button inputs. You usually don't need external resistors for simple switches.

### 1.4 Active HIGH vs Active LOW

This concept is critical for configuring LEDs correctly in CockpitOS.

| Term | Meaning | When Component is ON |
|------|---------|---------------------|
| **Active HIGH** | ON when signal is HIGH | GPIO outputs 3.3V |
| **Active LOW** | ON when signal is LOW | GPIO outputs 0V (GND) |

Most buttons in CockpitOS are wired **active LOW** (button connects GPIO to GND when pressed).

LEDs can be either, depending on how you wire them. The `activeLow` flag in LEDMapping.h tells CockpitOS which way your LED is wired.

---

## 2. GPIO Basics

**GPIO** stands for **General Purpose Input/Output**. These are the pins on your ESP32 that you use to connect buttons, LEDs, and other components.

### 2.1 ESP32 GPIO Capabilities

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    ESP32 GPIO CAPABILITIES                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   DIGITAL INPUT     Any GPIO can read HIGH/LOW (buttons, switches)          │
│   DIGITAL OUTPUT    Any GPIO can output HIGH/LOW (LEDs, relays)             │
│   ANALOG INPUT      Only specific pins (ADC-capable) can read voltage       │
│   PWM OUTPUT        Most GPIOs support PWM for dimming LEDs                 │
│   I²C               Specific pins for I2C communication (SDA, SCL)          │
│                                                                             │
│   ⚠️ Some pins have restrictions — see Section 15 for details               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 The PIN() Macro

CockpitOS uses a `PIN()` macro to handle GPIO numbering differences between ESP32 variants (S2 vs S3). Always use this macro in your configurations.

```cpp
// Instead of:
{ "MY_BUTTON", "GPIO", 12, 0, -1, "BUTTON", 1, "momentary", 0 },

// Use:
{ "MY_BUTTON", "GPIO", PIN(12), 0, -1, "BUTTON", 1, "momentary", 0 },
```

The macro automatically translates pin numbers when you compile for different boards. For example, PIN(12) on an S2 Mini becomes PIN(10) on an S3 Mini at the same physical location.

### 2.3 GPIO Limitations

Not all GPIOs are equal. Some have special functions or restrictions:

| GPIO | Restriction |
|------|-------------|
| GPIO 0 | Boot mode selection — avoid for inputs |
| GPIO 1, 3 | Often used for Serial TX/RX |
| GPIO 6-11 | Connected to internal flash — **DO NOT USE** |
| GPIO 34-39 | Input only (ESP32 Classic) — cannot output |

> **💡 Best Practice**  
> Stick to the documented pins for your specific board. When in doubt, test a single LED or button on a pin before building your full panel.

---

## 3. Buttons and Switches

Buttons and switches are the most common inputs in a cockpit panel. Let's understand the different types and how to wire them.

### 3.1 Understanding Switch Types

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SWITCH TYPES                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   MOMENTARY (Push Button)                                                   │
│   ─────────────────────────                                                 │
│   - Returns to original position when released                              │
│   - Examples: Fire button, master caution reset, UFC keypad                 │
│                                                                             │
│        ┌───┐                                                                │
│        │ ● │  ← Press and release                                           │
│        └───┘                                                                │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   TOGGLE (Maintained Position)                                              │
│   ─────────────────────────────                                             │
│   - Stays in position until moved again                                     │
│   - 2-position: ON/OFF                                                      │
│   - 3-position: UP/CENTER/DOWN                                              │
│                                                                             │
│       ╱           │           ╲                                             │
│      ○           ○           ○                                              │
│     UP         CENTER       DOWN                                            │
│                                                                             │
│   ─────────────────────────────────────────────────────────────────────     │
│                                                                             │
│   ROTARY SELECTOR (Multi-Position)                                          │
│   ─────────────────────────────────                                         │
│   - Multiple fixed positions                                                │
│   - Examples: Mode selectors, channel knobs                                 │
│                                                                             │
│           ╭───╮                                                             │
│        3 ╱  │  ╲ 1                                                          │
│         ╱ ──┼── ╲                                                           │
│        4 ╲  │  ╱ 0                                                          │
│           ╲ │ ╱                                                             │
│            ╰─╯                                                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 Wiring a Simple Button

The most basic wiring connects a button between a GPIO pin and GND:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    SIMPLE BUTTON WIRING                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                                                │
│                     ┌─────────┐                                             │
│                     │         │                                             │
│                     │  GPIO 5 ├────────┐                                    │
│                     │         │        │                                    │
│                     │         │     [BUTTON]                                │
│                     │         │        │                                    │
│                     │    GND  ├────────┘                                    │
│                     │         │                                             │
│                     └─────────┘                                             │
│                                                                             │
│   That's it! CockpitOS enables the internal pull-up resistor.               │
│                                                                             │
│   Button OPEN  → GPIO reads HIGH (1)                                        │
│   Button PRESSED → GPIO reads LOW (0) — this is "active low"                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.3 Configuring in InputMapping.h

**Momentary Button:**

```cpp
//  label              source   port     bit hidId  DCSCommand         value  Type        group
{ "MASTER_CAUTION_BTN", "GPIO", PIN(5),   0,  -1, "MASTER_CAUTION_BTN",  1, "momentary",   0 },
```

**Explanation of fields:**
- `"MASTER_CAUTION_BTN"` — Unique label for this button
- `"GPIO"` — Hardware source (direct GPIO connection)
- `PIN(5)` — GPIO pin number (use PIN() macro)
- `0` — Bit position (not used for GPIO, keep as 0)
- `-1` — HID button ID (-1 = don't send HID)
- `"MASTER_CAUTION_BTN"` — DCS-BIOS command to send
- `1` — Value to send (1 = pressed)
- `"momentary"` — Control type
- `0` — Group ID (0 = no group)

### 3.4 Two-Position Toggle Switch

A simple ON/OFF toggle needs two entries in InputMapping.h — one for each position:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                 TWO-POSITION TOGGLE WIRING                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   The switch has 3 terminals: Common (C), Normally Open (NO),               │
│   Normally Closed (NC). We typically use C and one other.                   │
│                                                                             │
│                        ESP32                                                │
│                     ┌─────────┐                                             │
│                     │         │                                             │
│                     │  GPIO 5 ├───────┬──── C (common)                      │
│                     │         │       │                                     │
│                     │    GND  ├───────┴──── NO (normally open)              │
│                     │         │                                             │
│                     └─────────┘                                             │
│                                                                             │
│   Switch in "OFF" position → GPIO reads HIGH                                │
│   Switch in "ON" position → GPIO reads LOW                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**InputMapping.h configuration:**

```cpp
//  label               source   port     bit hidId  DCSCommand    value  Type       group
{ "MASTER_ARM_ON",      "GPIO", PIN(5),   0,  -1, "MASTER_ARM_SW",   1, "selector",   1 },
{ "MASTER_ARM_OFF",     "GPIO",    -1,   -1,  -1, "MASTER_ARM_SW",   0, "selector",   1 },
```

**Key points:**
- Both entries share the same **group** (1) — this makes them mutually exclusive
- The "OFF" position has `port = -1` — this is the **fallback** position (when no GPIO is LOW)
- Only ONE position should have a real GPIO; the other is the "neutral" or "default"

### 3.5 Three-Position Toggle Switch

Three-position switches (UP/CENTER/DOWN) need three entries:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│               THREE-POSITION TOGGLE WIRING                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   A 3-position switch typically has one common and two throws.              │
│   We wire it with TWO GPIOs — one for each extreme position.                │
│   The center position is detected when NEITHER GPIO is LOW.                 │
│                                                                             │
│                        ESP32                                                │
│                     ┌─────────┐                                             │
│                     │         │                                             │
│        UP ─────────│ GPIO 5  ├─────────┐                                    │
│                     │         │         │                                   │
│                     │  GPIO 6 ├─────────┼──── C (common to GND)             │
│        DOWN ───────│         │         │                                    │
│                     │         │         │                                   │
│                     │    GND  ├─────────┘                                   │
│                     │         │                                             │
│                     └─────────┘                                             │
│                                                                             │
│   Switch UP     → GPIO 5 LOW, GPIO 6 HIGH                                   │
│   Switch CENTER → GPIO 5 HIGH, GPIO 6 HIGH (neither connected)              │
│   Switch DOWN   → GPIO 5 HIGH, GPIO 6 LOW                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**InputMapping.h configuration:**

```cpp
//  label                   source   port     bit hidId  DCSCommand       value  Type       group
{ "FUEL_DUMP_SW_NORM",      "GPIO", PIN(5),   0,  -1, "FUEL_DUMP_SW",      0, "selector",   5 },
{ "FUEL_DUMP_SW_OFF",       "GPIO",    -1,   -1,  -1, "FUEL_DUMP_SW",      1, "selector",   5 },
{ "FUEL_DUMP_SW_DUMP",      "GPIO", PIN(6),   0,  -1, "FUEL_DUMP_SW",      2, "selector",   5 },
```

**Key points:**
- All three share **group = 5**
- The CENTER position (`OFF`) has `port = -1` — it's the fallback
- CockpitOS automatically detects center when neither GPIO 5 nor GPIO 6 is LOW

### 3.6 Multi-Position Rotary Selector

For rotary selectors with many positions, you have options:

**Option A: One GPIO per position ("one-hot")**

```cpp
//  label               source   port      bit hidId  DCSCommand    value  Type       group
{ "MODE_SW_OFF",        "GPIO", PIN(10),  -1,  -1, "MODE_SELECT",    0, "selector",  10 },
{ "MODE_SW_STBY",       "GPIO", PIN(11),  -1,  -1, "MODE_SELECT",    1, "selector",  10 },
{ "MODE_SW_LOW",        "GPIO", PIN(12),  -1,  -1, "MODE_SELECT",    2, "selector",  10 },
{ "MODE_SW_MED",        "GPIO", PIN(13),  -1,  -1, "MODE_SELECT",    3, "selector",  10 },
{ "MODE_SW_HIGH",       "GPIO", PIN(14),  -1,  -1, "MODE_SELECT",    4, "selector",  10 },
```

Note: `bit = -1` signals "one-hot" mode where each position has its own GPIO.

**Option B: Use INC/DEC with a rotary encoder** (see [Section 6](#6-rotary-encoders))

**Option C: Use a shift register for many positions** (see [Section 7](#7-expanding-inputs--74hc165-shift-registers))

---

## 4. LEDs — Light Emitting Diodes

LEDs provide visual feedback — caution lights, indicator panels, backlit buttons, etc.

### 4.1 LED Basics

LEDs are **polarized** — they only work when connected the right way:
- **Anode (+)** — the longer leg, connects toward positive voltage
- **Cathode (-)** — the shorter leg (flat side of LED body), connects toward GND

LEDs need a **current-limiting resistor** to prevent burnout. The ESP32 outputs 3.3V, and most LEDs need 10-20mA.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       LED RESISTOR CALCULATION                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   R = (Vsource - Vled) / I                                                  │
│                                                                             │
│   Where:                                                                    │
│   - Vsource = 3.3V (ESP32 GPIO output)                                      │
│   - Vled = LED forward voltage (red ≈ 2.0V, green ≈ 2.2V, blue ≈ 3.0V)     │
│   - I = Desired current (10-20mA typical)                                   │
│                                                                             │
│   Example for red LED at 15mA:                                              │
│   R = (3.3V - 2.0V) / 0.015A = 87Ω → Use 100Ω (standard value)             │
│                                                                             │
│   QUICK REFERENCE:                                                          │
│   - Red LED: 100Ω - 220Ω                                                    │
│   - Green LED: 100Ω - 150Ω                                                  │
│   - Blue/White LED: 47Ω - 100Ω (3.3V may be dim)                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 Wiring an LED (Active HIGH)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    LED WIRING — ACTIVE HIGH                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                                                │
│                     ┌─────────┐                                             │
│                     │         │                                             │
│                     │  GPIO 5 ├────[R]────┬──── LED (+) Anode               │
│                     │         │           │                                 │
│                     │         │           └──── LED (-) Cathode             │
│                     │         │                      │                      │
│                     │    GND  ├──────────────────────┘                      │
│                     │         │                                             │
│                     └─────────┘                                             │
│                                                                             │
│   GPIO HIGH (3.3V) → LED ON                                                 │
│   GPIO LOW (0V)    → LED OFF                                                │
│                                                                             │
│   This is "Active HIGH" wiring. Set activeLow = false in LEDMapping.h       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.3 Wiring an LED (Active LOW)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    LED WIRING — ACTIVE LOW                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                                                │
│                     ┌─────────┐                                             │
│                     │         │                                             │
│                     │   3.3V  ├────────────────┬──── LED (+) Anode          │
│                     │         │                │                            │
│                     │         │                └──── LED (-) Cathode        │
│                     │         │                           │                 │
│                     │  GPIO 5 ├────[R]────────────────────┘                 │
│                     │         │                                             │
│                     └─────────┘                                             │
│                                                                             │
│   GPIO LOW (0V)    → LED ON (current flows from 3.3V through LED to GPIO)   │
│   GPIO HIGH (3.3V) → LED OFF (no voltage difference)                        │
│                                                                             │
│   This is "Active LOW" wiring. Set activeLow = true in LEDMapping.h         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.4 Configuring in LEDMapping.h

```cpp
//  label              deviceType    info                    dimmable  activeLow
{ "MASTER_CAUTION_LT", DEVICE_GPIO, { .gpioInfo = { PIN(5) } }, false,    false },
```

**Fields explained:**
- `"MASTER_CAUTION_LT"` — Unique label (must match DCS-BIOS output label)
- `DEVICE_GPIO` — Hardware driver type
- `{ .gpioInfo = { PIN(5) } }` — GPIO pin assignment
- `false` — Dimmable (true = PWM capable, varies brightness)
- `false` — Active LOW (false = active HIGH wiring)

**For a dimmable LED:**

```cpp
{ "INST_PNL_DIMMER", DEVICE_GPIO, { .gpioInfo = { PIN(5) } }, true, false },
```

Setting `dimmable = true` enables PWM, allowing CockpitOS to vary brightness based on DCS values (0-255).

### 4.5 Testing Your LED

Before configuring DCS-BIOS, test your LED with the built-in test mode:

1. In `Config.h`, set:
   ```cpp
   #define TEST_LEDS    1
   ```

2. Compile and upload

3. Open Serial Monitor — you'll see a menu of all configured LEDs

4. Enter the LED number to activate it for 5 seconds

This confirms your wiring before you connect to DCS.

---

## 5. Potentiometers and Analog Inputs

Potentiometers (pots) provide continuous analog input — perfect for throttles, brightness knobs, and trim wheels.

### 5.1 How Potentiometers Work

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    HOW A POTENTIOMETER WORKS                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   A potentiometer is a variable resistor with three terminals:              │
│                                                                             │
│         ┌───────────────────────────────┐                                   │
│         │                               │                                   │
│    Terminal 1                       Terminal 3                              │
│    (one end)                        (other end)                             │
│         │                               │                                   │
│         └───────────┬───────────────────┘                                   │
│                     │                                                       │
│               Terminal 2                                                    │
│                (wiper)                                                      │
│                                                                             │
│   - Terminal 1 to Terminal 3: Fixed total resistance                        │
│   - Terminal 2 (wiper): Moves along the resistance                          │
│   - Rotating the knob changes the wiper position                            │
│                                                                             │
│   Used as a voltage divider:                                                │
│   - Terminal 1 → 3.3V                                                       │
│   - Terminal 3 → GND                                                        │
│   - Terminal 2 → ESP32 ADC pin (reads 0V to 3.3V)                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.2 Wiring a Potentiometer

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   POTENTIOMETER WIRING                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                                                │
│                     ┌─────────┐                                             │
│                     │         │                                             │
│                     │   3.3V  ├─────────────────┐                           │
│                     │         │                 │                           │
│                     │         │              ┌──┴──┐                        │
│                     │ GPIO 4  ├──────────────┤WIPER│ (center terminal)      │
│                     │  (ADC)  │              └──┬──┘                        │
│                     │         │                 │                           │
│                     │    GND  ├─────────────────┘                           │
│                     │         │                                             │
│                     └─────────┘                                             │
│                                                                             │
│   As you rotate the pot:                                                    │
│   - Full CCW → Wiper near GND → ADC reads ~0                                │
│   - Center   → Wiper at middle → ADC reads ~2048                            │
│   - Full CW  → Wiper near 3.3V → ADC reads ~4095                            │
│                                                                             │
│   ESP32 ADC is 12-bit: values range from 0 to 4095                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

> **⚠️ Important**  
> Only certain GPIO pins support analog input (ADC). On ESP32-S2/S3, GPIO 1-10 are typically ADC-capable. Check your specific board's pinout.

### 5.3 Configuring in InputMapping.h

```cpp
//  label          source   port     bit hidId  DCSCommand     value   Type      group
{ "THROTTLE",      "GPIO", PIN(4),   0,  -1, "THROTTLE",    65535, "analog",     0 },
```

**Key points:**
- `controlType = "analog"` — tells CockpitOS this is a continuous input
- `oride_value = 65535` — special value meaning "send raw analog reading"
- CockpitOS automatically maps the 12-bit ADC reading (0-4095) to DCS-BIOS range

### 5.4 Reversing Direction

If your pot works backwards (CCW should be max but reads as min), simply swap the 3.3V and GND connections on the potentiometer terminals.

---

## 6. Rotary Encoders

Rotary encoders are fundamentally different from potentiometers. They don't have fixed positions — they spin infinitely and generate **pulses** indicating direction and speed.

### 6.1 How Encoders Work

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   QUADRATURE ENCODER PRINCIPLE                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   An encoder has two output channels (A and B) that produce square waves    │
│   90° out of phase. The phase relationship indicates direction:             │
│                                                                             │
│   CLOCKWISE ROTATION:                                                       │
│                                                                             │
│   Channel A ─────┐   ┌───┐   ┌───                                           │
│                  │   │   │   │                                              │
│                  └───┘   └───┘                                              │
│                                                                             │
│   Channel B ───┐   ┌───┐   ┌───┐                                            │
│                │   │   │   │   │                                            │
│                └───┘   └───┘   └                                            │
│            ◄─── A leads B                                                   │
│                                                                             │
│   COUNTER-CLOCKWISE ROTATION:                                               │
│                                                                             │
│   Channel A ───┐   ┌───┐   ┌───┐                                            │
│                │   │   │   │   │                                            │
│                └───┘   └───┘   └                                            │
│                                                                             │
│   Channel B ─────┐   ┌───┐   ┌───                                           │
│                  │   │   │   │                                              │
│                  └───┘   └───┘                                              │
│            ◄─── B leads A                                                   │
│                                                                             │
│   By tracking which channel changes first, we determine direction.          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 6.2 Encoder vs Potentiometer — When to Use Which

| Feature | Potentiometer | Rotary Encoder |
|---------|---------------|----------------|
| Rotation | Limited (270° typical) | Infinite |
| Output | Absolute position | Pulses (relative) |
| Use case | Throttle, brightness | Volume, tuning, channel select |
| DCS example | `THROTTLE` | `RADIO_VOLUME`, `UFC_1-0` |

### 6.3 Wiring a Rotary Encoder

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   ROTARY ENCODER WIRING                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Most encoders have 3 pins for rotation (some add 2 more for push button): │
│                                                                             │
│                        ESP32                                                │
│                     ┌─────────┐                                             │
│                     │         │                                             │
│                     │  GPIO 5 ├──────────────── A (Channel A)               │
│                     │         │                                             │
│                     │  GPIO 6 ├──────────────── B (Channel B)               │
│                     │         │                                             │
│                     │    GND  ├──────────────── C (Common/Ground)           │
│                     │         │                                             │
│                     └─────────┘                                             │
│                                                                             │
│   If your encoder has a push button (5-pin type):                           │
│                                                                             │
│                     │  GPIO 7 ├──────────────── SW (Switch)                 │
│                     │    GND  ├──────────────── SW GND                      │
│                                                                             │
│   CockpitOS enables internal pull-ups on A and B pins.                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 6.4 Understanding fixed_step vs variable_step

DCS-BIOS supports two encoder command types:

**fixed_step**: Sends `INC` or `DEC` commands
- Each detent (click) sends one command
- Good for stepped controls (channel selectors, mode switches)
- Example: `TACAN_CHAN INC` or `TACAN_CHAN DEC`

**variable_step**: Sends `+3200` or `-3200` (variable amount)
- Default step is 3200 (out of 65535 total range)
- Multiple rapid rotations accumulate larger values
- Good for smooth controls (volume, brightness)
- Example: `HMD_OFF_BRT +3200` or `HMD_OFF_BRT -3200`

### 6.5 Configuring Encoders in InputMapping.h

Encoders require **two entries** — one for each direction:

**For fixed_step encoder:**

```cpp
//  label             source   port     bit hidId  DCSCommand         value  Type          group
{ "TACAN_CHAN_DEC",   "GPIO", PIN(5),   0,  -1, "TACAN_CHAN",          0, "fixed_step",    0 },
{ "TACAN_CHAN_INC",   "GPIO", PIN(6),   0,  -1, "TACAN_CHAN",          1, "fixed_step",    0 },
```

**For variable_step encoder:**

```cpp
//  label              source   port     bit hidId  DCSCommand         value  Type           group
{ "HMD_OFF_BRT_DEC",   "GPIO", PIN(5),   0,  -1, "HMD_OFF_BRT",         0, "variable_step",  0 },
{ "HMD_OFF_BRT_INC",   "GPIO", PIN(6),   0,  -1, "HMD_OFF_BRT",         1, "variable_step",  0 },
```

**Key points:**
- `oride_value = 0` → Decrement (CCW rotation)
- `oride_value = 1` → Increment (CW rotation)
- Both entries share the same `oride_label` (DCS command)
- The decoder pairs them automatically by matching labels

### 6.6 How CockpitOS Processes Encoder Pulses

CockpitOS uses a **state transition table** to decode the quadrature signals:

```cpp
// From InputControl.cpp
static const int8_t encoder_transition_table[16] = {
     0, -1,  1,  0,  1,  0,  0, -1,
    -1,  0,  0,  1,  0,  1, -1,  0
};
```

The firmware:
1. Reads both channels at high speed (250Hz)
2. Tracks the previous state (A,B) and current state
3. Looks up the transition in the table to get direction (-1, 0, +1)
4. Accumulates counts and sends commands after reaching a threshold (typically 4 transitions per detent)

This provides reliable, bounce-free encoder reading without external hardware debouncing.

### 6.7 Encoder Sensitivity

By default, CockpitOS requires 4 transitions (one full "detent") before sending a command. This is defined by:

```cpp
#define ENCODER_TICKS_PER_NOTCH  4
```

If your encoder has a different PPR (pulses per revolution), you may need to adjust this value in `InputControl.cpp`.

---

## 7. Expanding Inputs — 74HC165 Shift Registers

When you run out of GPIO pins, shift registers let you read many inputs using just 3 wires.

### 7.1 How Shift Registers Work

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    74HC165 SHIFT REGISTER                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   The 74HC165 is a "Parallel-In, Serial-Out" shift register.                │
│   It can read 8 inputs and send them to the ESP32 one bit at a time.        │
│                                                                             │
│   ┌───────────────────────────────────────────┐                             │
│   │            74HC165                        │                             │
│   │                                           │                             │
│   │  D0 ──┤  ├── VCC (3.3V or 5V)             │                             │
│   │  D1 ──┤  ├── GND                          │                             │
│   │  D2 ──┤  ├── QH (Serial Out) ──────► ESP32 Data Pin                     │
│   │  D3 ──┤  ├── QH' (Cascade Out) ────► Next chip's serial in              │
│   │  D4 ──┤  ├── SH/LD (Parallel Load) ◄── ESP32 PL Pin                     │
│   │  D5 ──┤  ├── CLK (Clock) ◄───────────── ESP32 CP Pin                    │
│   │  D6 ──┤  ├── CLK INH (tie to GND)        │                             │
│   │  D7 ──┤  ├── SER (Cascade In) ◄─── Previous chip's QH'                  │
│   │                                           │                             │
│   └───────────────────────────────────────────┘                             │
│                                                                             │
│   D0-D7: Connect your buttons/switches here (active LOW)                    │
│                                                                             │
│   OPERATION:                                                                │
│   1. ESP32 pulses PL LOW → chip captures all 8 inputs                       │
│   2. ESP32 pulses CLK → chip shifts out one bit on QH                       │
│   3. Repeat step 2 for all bits                                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 7.2 Daisy-Chaining Multiple Chips

You can chain multiple 74HC165s to read 16, 24, 32+ inputs:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    DAISY-CHAINED HC165s                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│              ┌─────────┐      ┌─────────┐      ┌─────────┐                  │
│   ESP32      │ HC165   │      │ HC165   │      │ HC165   │                  │
│              │  #1     │      │  #2     │      │  #3     │                  │
│   PL  ──────►│ SH/LD   │─────►│ SH/LD   │─────►│ SH/LD   │  (shared)        │
│   CP  ──────►│ CLK     │─────►│ CLK     │─────►│ CLK     │  (shared)        │
│   QH ◄──────│ QH      │      │ QH      │      │ QH      │                  │
│              │     QH' │─────►│ SER     │      │         │                  │
│              │         │      │     QH' │─────►│ SER     │                  │
│              │  D0-D7  │      │  D0-D7  │      │  D0-D7  │                  │
│              └─────────┘      └─────────┘      └─────────┘                  │
│                 Bits 0-7        Bits 8-15       Bits 16-23                  │
│                                                                             │
│   All chips share PL and CLK. Data cascades through QH' → SER.              │
│   First chip's D0 = Bit 0, Last chip's D7 = highest bit                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 7.3 Wiring Example (16 inputs)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                  HC165 WIRING (16-bit example)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                  HC165 #1         HC165 #2     │
│                     ┌─────────┐             ┌─────────┐      ┌─────────┐    │
│                     │         │             │         │      │         │    │
│                     │GPIO 39  ├─────────────┤ SH/LD   ├──────┤ SH/LD   │    │
│                     │  (PL)   │             │         │      │         │    │
│                     │         │             │         │      │         │    │
│                     │GPIO 38  ├─────────────┤ CLK     ├──────┤ CLK     │    │
│                     │  (CP)   │             │         │      │         │    │
│                     │         │             │         │      │         │    │
│                     │GPIO 40  ├─────────────┤ QH      │      │         │    │
│                     │  (QH)   │             │    QH'  ├──────┤ SER     │    │
│                     │         │             │         │      │         │    │
│                     │   GND   ├─────────────┤ GND     ├──────┤ GND     │    │
│                     │         │             │ CLK INH │      │ CLK INH │    │
│                     │  3.3V   ├─────────────┤ VCC     ├──────┤ VCC     │    │
│                     │         │             │         │      │         │    │
│                     └─────────┘             └─────────┘      └─────────┘    │
│                                                                             │
│   Connect buttons between D0-D7 pins and GND (active LOW)                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 7.4 Configuring in Pins.h

First, configure the shift register pins in `Pins.h`:

```cpp
#define HC165_BITS                16   // Total bits (8 per chip × 2 chips)
#define HC165_CONTROLLER_PL      PIN(39)   // Parallel Load pin
#define HC165_CONTROLLER_CP      PIN(38)   // Clock pin
#define HC165_CONTROLLER_QH      PIN(40)   // Data out pin
```

### 7.5 Configuring in InputMapping.h

```cpp
//  label                 source   port  bit hidId  DCSCommand            value  Type        group
{ "ENGINE_CRANK_LEFT",    "HC165",  0,    0,  -1, "ENGINE_CRANK_SW",        2, "selector",    4 },
{ "ENGINE_CRANK_OFF",     "HC165",  0,   -1,  -1, "ENGINE_CRANK_SW",        1, "selector",    4 },
{ "ENGINE_CRANK_RIGHT",   "HC165",  0,    1,  -1, "ENGINE_CRANK_SW",        0, "selector",    4 },
{ "MASTER_ARM_BTN",       "HC165",  0,    2,  -1, "MASTER_ARM_BTN",         1, "momentary",   0 },
{ "EMERG_JETT_BTN",       "HC165",  0,    3,  -1, "EMERG_JETT_BTN",         1, "momentary",   0 },
```

**Key points:**
- `source = "HC165"` — identifies this as a shift register input
- `port = 0` — always 0 for HC165 (not used, kept for consistency)
- `bit` — which bit position (0-15 for 16 bits, 0-23 for 24 bits, etc.)
- `bit = -1` — fallback position for selectors (when no bit is LOW)

---

## 8. Expanding I/O — PCA9555 I²C Expanders

The PCA9555 adds 16 GPIOs over I²C — usable for both inputs AND outputs.

### 8.1 How I²C Works

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         I²C BUS BASICS                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   I²C (Inter-Integrated Circuit) uses just 2 wires:                         │
│   - SDA (Serial Data) — bidirectional data line                             │
│   - SCL (Serial Clock) — clock signal from master                           │
│                                                                             │
│   Multiple devices share the same bus, each with a unique ADDRESS.          │
│                                                                             │
│                ESP32                                                        │
│              ┌───────┐                                                      │
│              │       │                                                      │
│   SDA ───────┤ SDA   ├──────┬──────────┬──────────┬──────                   │
│              │       │      │          │          │                         │
│   SCL ───────┤ SCL   ├──────┼──────────┼──────────┼──────                   │
│              │       │      │          │          │                         │
│              └───────┘   ┌──┴──┐    ┌──┴──┐    ┌──┴──┐                      │
│                          │ 0x20│    │ 0x21│    │ 0x22│  ← I²C addresses     │
│                          │PCA  │    │PCA  │    │PCA  │                      │
│                          └─────┘    └─────┘    └─────┘                      │
│                                                                             │
│   PCA9555 addresses range from 0x20 to 0x27 (set via A0-A2 pins)           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 8.2 PCA9555 Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         PCA9555 PINOUT                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   The PCA9555 provides 16 I/O pins organized as two 8-bit ports:            │
│                                                                             │
│   ┌─────────────────────────────────────────────────────┐                   │
│   │                    PCA9555                          │                   │
│   │                                                     │                   │
│   │  P0.0 ──┤                              ├── P1.0    │                   │
│   │  P0.1 ──┤                              ├── P1.1    │                   │
│   │  P0.2 ──┤     PORT 0        PORT 1     ├── P1.2    │                   │
│   │  P0.3 ──┤    (8 pins)      (8 pins)    ├── P1.3    │                   │
│   │  P0.4 ──┤                              ├── P1.4    │                   │
│   │  P0.5 ──┤                              ├── P1.5    │                   │
│   │  P0.6 ──┤                              ├── P1.6    │                   │
│   │  P0.7 ──┤                              ├── P1.7    │                   │
│   │                                                     │                   │
│   │  A0, A1, A2 ──── Address select (0x20 + A2A1A0)    │                   │
│   │  SDA, SCL ────── I²C bus connections               │                   │
│   │  INT ─────────── Interrupt output (optional)       │                   │
│   │  VCC, GND ────── Power supply (3.3V or 5V)        │                   │
│   │                                                     │                   │
│   └─────────────────────────────────────────────────────┘                   │
│                                                                             │
│   Each pin can be independently configured as INPUT or OUTPUT.              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 8.3 Setting the I²C Address

The PCA9555 address is determined by the A0, A1, A2 pins:

| A2 | A1 | A0 | Address |
|----|----|----|---------|
| GND | GND | GND | 0x20 |
| GND | GND | VCC | 0x21 |
| GND | VCC | GND | 0x22 |
| GND | VCC | VCC | 0x23 |
| VCC | GND | GND | 0x24 |
| VCC | GND | VCC | 0x25 |
| VCC | VCC | GND | 0x26 |
| VCC | VCC | VCC | 0x27 |

### 8.4 Wiring a PCA9555

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      PCA9555 WIRING                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                       PCA9555                  │
│                     ┌─────────┐                  ┌─────────┐                │
│                     │         │                  │         │                │
│                     │  SDA    ├──────────────────┤ SDA     │                │
│                     │ (GPIO21)│                  │         │                │
│                     │         │                  │         │                │
│                     │  SCL    ├──────────────────┤ SCL     │                │
│                     │ (GPIO22)│                  │         │                │
│                     │         │                  │         │                │
│                     │   GND   ├──────┬───────────┤ GND     │                │
│                     │         │      │           │ A0      │ (for 0x20)     │
│                     │         │      │           │ A1      │                │
│                     │         │      │           │ A2      │                │
│                     │         │      │           │         │                │
│                     │  3.3V   ├──────┴───────────┤ VCC     │                │
│                     │         │                  │         │                │
│                     └─────────┘                  └─────────┘                │
│                                                                             │
│   ⚠️ I²C needs pull-up resistors on SDA and SCL (4.7kΩ typical)            │
│      Some breakout boards include these already.                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 8.5 Enabling PCA9555 in Config.h

```cpp
#define ENABLE_PCA9555    1   // Enable PCA expander support
```

### 8.6 Configuring Inputs in InputMapping.h

```cpp
//  label                  source       port  bit hidId  DCSCommand         value  Type        group
{ "SPIN_RECOV_COVER",      "PCA_0x20",   0,    0,  -1, "SPIN_RECOV_COVER",     1, "momentary",   0 },
{ "SPIN_RECOV_BTN",        "PCA_0x20",   0,    1,  -1, "SPIN_RECOV_BTN",       1, "momentary",   0 },
{ "APU_CONTROL_ON",        "PCA_0x20",   0,    2,  -1, "APU_CONTROL_SW",       1, "selector",    7 },
{ "APU_CONTROL_OFF",       "PCA_0x20",   0,   -1,  -1, "APU_CONTROL_SW",       0, "selector",    7 },
{ "APU_CONTROL_START",     "PCA_0x20",   0,    3,  -1, "APU_CONTROL_SW",       2, "selector",    7 },
```

**Fields:**
- `source = "PCA_0x20"` — PCA9555 at I²C address 0x20
- `port` — 0 for Port 0 (P0.x), 1 for Port 1 (P1.x)
- `bit` — bit position within the port (0-7)

### 8.7 Configuring Outputs in LEDMapping.h

```cpp
//  label               deviceType     info                              dimmable  activeLow
{ "APU_READY_LT",       DEVICE_PCA9555, { .pcaInfo = { 0x20, 1, 4 } },   false,    false },
{ "FIRE_APU_LT",        DEVICE_PCA9555, { .pcaInfo = { 0x20, 1, 5 } },   false,    true  },
```

**Fields:**
- `.pcaInfo = { address, port, bit }`
- `address` — I²C address (0x20)
- `port` — 0 or 1
- `bit` — 0-7

---

## 9. TM1637 Displays and Keypads

The TM1637 is commonly used for 4-digit 7-segment displays and also supports reading button matrices.

### 9.1 TM1637 as LED Output

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      TM1637 LED DISPLAY                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   TM1637 modules typically have 4 connections:                              │
│   - CLK: Clock signal from ESP32                                            │
│   - DIO: Bidirectional data line                                            │
│   - VCC: Power (3.3V or 5V)                                                 │
│   - GND: Ground                                                             │
│                                                                             │
│                        ESP32                       TM1637 Module            │
│                     ┌─────────┐                  ┌─────────────┐            │
│                     │         │                  │ ┌─┬─┬─┬─┐   │            │
│                     │  GPIO 8 ├──────────────────┤ │8│8│8│8│   │            │
│                     │  (CLK)  │                  │ └─┴─┴─┴─┘   │            │
│                     │         │                  │  CLK  DIO   │            │
│                     │  GPIO 9 ├──────────────────┤   │    │    │            │
│                     │  (DIO)  │                  │   │    │    │            │
│                     │         │                  │  VCC  GND   │            │
│                     │  3.3V   ├──────────────────┤   │    │    │            │
│                     │   GND   ├──────────────────┤───┘    │    │            │
│                     │         │                  └────────┴────┘            │
│                     └─────────┘                                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 9.2 Configuring TM1637 LEDs in LEDMapping.h

```cpp
//  label              deviceType    info                                        dimmable  activeLow
{ "WARN_CAUTION_LT",   DEVICE_TM1637, { .tm1637Info = { PIN(8), PIN(9), 2, 3 } }, false,   false },
```

**Fields:**
- `.tm1637Info = { clkPin, dioPin, segment, bit }`
- `clkPin` — GPIO for clock signal
- `dioPin` — GPIO for data signal
- `segment` — which digit/grid position (0-5 typically)
- `bit` — which segment within that digit (0-7)

### 9.3 TM1637 as Button Input

Some TM1637 modules support a key matrix for button input:

```cpp
//  label               source   port      bit  hidId  DCSCommand       value  Type        group
{ "LEFT_FIRE_BTN",      "TM1637", PIN(39),   3,  -1, "LEFT_FIRE_BTN",      1, "momentary",   0 },
```

**Fields:**
- `source = "TM1637"` — identifies TM1637 key input
- `port` — the DIO pin (identifies which TM1637 device)
- `bit` — key scan code returned by the module

---

## 10. WS2812 Addressable RGB LEDs

WS2812 (NeoPixel) LEDs are individually addressable RGB LEDs that can display any color and brightness.

### 10.1 How WS2812 Works

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      WS2812 LED STRIP                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   WS2812 LEDs are daisy-chained. Each LED has:                              │
│   - DIN (Data In) — receives data from previous LED or ESP32                │
│   - DOUT (Data Out) — sends data to next LED in chain                       │
│   - VCC — Power (typically 5V)                                              │
│   - GND — Ground                                                            │
│                                                                             │
│                        ESP32                                                │
│                     ┌─────────┐                                             │
│                     │         │     ┌─────┐   ┌─────┐   ┌─────┐             │
│                     │  GPIO15 ├────►│LED 0│──►│LED 1│──►│LED 2│──► ...      │
│                     │  (Data) │     │DIN  │   │DIN  │   │DIN  │             │
│                     │         │     └──┬──┘   └──┬──┘   └──┬──┘             │
│                     │    5V   ├────────┴─────────┴─────────┴────────        │
│                     │   GND   ├────────────────────────────────────         │
│                     │         │                                             │
│                     └─────────┘                                             │
│                                                                             │
│   Each LED is addressed by its INDEX (position in chain: 0, 1, 2, ...)      │
│   Data protocol sends 24 bits (8R + 8G + 8B) per LED at ~800 kHz            │
│                                                                             │
│   ⚠️ WS2812 needs 5V power but can accept 3.3V data signal.                │
│      For long strips, add a level shifter and adequate power supply.        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.2 Wiring WS2812

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     WS2812 WIRING                                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                     WS2812 Strip               │
│                     ┌─────────┐                ┌─────────────┐              │
│                     │         │                │             │              │
│                     │  GPIO15 ├───────────────►│ DIN         │              │
│                     │         │                │             │              │
│                     │   GND   ├────────────────┤ GND         │              │
│                     │         │                │             │              │
│                     └─────────┘                │ VCC (5V) ◄──┼── 5V Supply  │
│                                                │             │              │
│                                                └─────────────┘              │
│                                                                             │
│   Power Calculation:                                                        │
│   - Each LED draws up to 60mA at full white brightness                      │
│   - 10 LEDs = 600mA max                                                     │
│   - Use adequate power supply; don't power from ESP32's 5V pin              │
│                                                                             │
│   Data Line:                                                                │
│   - Keep data wire short (< 30cm without level shifter)                     │
│   - Add 300-500Ω resistor in series for signal protection                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.3 Configuring in LEDMapping.h

```cpp
//  label              deviceType    info                                              dimmable  activeLow
{ "GEAR_WARN_LT",      DEVICE_WS2812, { .ws2812Info = { 0, PIN(15), 255, 0, 0, 255 } }, false,   false },
{ "GEAR_SAFE_LT",      DEVICE_WS2812, { .ws2812Info = { 1, PIN(15), 0, 255, 0, 255 } }, false,   false },
{ "GEAR_TRANS_LT",     DEVICE_WS2812, { .ws2812Info = { 2, PIN(15), 255, 165, 0, 255 }}, false,   false },
```

**Fields:**
- `.ws2812Info = { index, pin, defR, defG, defB, defBright }`
- `index` — LED position in chain (0 = first LED)
- `pin` — GPIO connected to DIN
- `defR, defG, defB` — default color (Red, Green, Blue: 0-255)
- `defBright` — default brightness (0-255)

> **💡 Color Examples**
> - Red: `{ 255, 0, 0 }`
> - Green: `{ 0, 255, 0 }`
> - Blue: `{ 0, 0, 255 }`
> - White: `{ 255, 255, 255 }`
> - Orange: `{ 255, 165, 0 }`
> - Yellow: `{ 255, 255, 0 }`

---

## 11. GN1640T LED Matrix Drivers

The GN1640T drives LED matrices — useful for annunciator panels with many individual lights.

### 11.1 GN1640T Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      GN1640T LED MATRIX                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   The GN1640T can drive up to 8×8 = 64 individual LEDs.                     │
│   LEDs are addressed by COLUMN (grid) and ROW (segment).                    │
│                                                                             │
│        Col 0   Col 1   Col 2   Col 3   Col 4   Col 5   Col 6   Col 7        │
│       ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐                     │
│ Row 0 │ LED │ LED │ LED │ LED │ LED │ LED │ LED │ LED │                     │
│       ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤                     │
│ Row 1 │ LED │ LED │ LED │ LED │ LED │ LED │ LED │ LED │                     │
│       ├─────┼─────┼─────┼─────┼─────┼─────┼─────┼─────┤                     │
│  ...  │ ... │ ... │ ... │ ... │ ... │ ... │ ... │ ... │                     │
│       └─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘                     │
│                                                                             │
│   Connections: CLK, DIO, VCC, GND (similar to TM1637)                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 11.2 Configuring in LEDMapping.h

```cpp
//  label               deviceType    info                                  dimmable  activeLow
{ "WARN_APU",           DEVICE_GN1640T, { .gn1640Info = { 0, 0, 3 } },      false,    false },
{ "WARN_ENGINE_L",      DEVICE_GN1640T, { .gn1640Info = { 0, 1, 3 } },      false,    false },
{ "WARN_ENGINE_R",      DEVICE_GN1640T, { .gn1640Info = { 0, 2, 3 } },      false,    false },
```

**Fields:**
- `.gn1640Info = { address, column, row }`
- `address` — device address (if multiple GN1640T chips)
- `column` — which column (0-7)
- `row` — which row (0-7)

---

## 12. Matrix Rotary Switches

Matrix rotary switches use strobe/data patterns to read multi-position selectors with fewer wires.

### 12.1 How Matrix Rotaries Work

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    MATRIX ROTARY CONCEPT                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   A matrix rotary uses multiple STROBE lines and one DATA line.             │
│   The rotary switch connects the DATA line to one STROBE at a time.         │
│                                                                             │
│               ┌──────────────────────────────┐                              │
│               │    ROTARY SWITCH             │                              │
│               │                              │                              │
│   Strobe 0 ───┤──○                           │                              │
│   Strobe 1 ───┤──○                           │                              │
│   Strobe 2 ───┤──○───────○ Wiper ────────────┼──── DATA                     │
│   Strobe 3 ───┤──○                           │                              │
│               │                              │                              │
│               └──────────────────────────────┘                              │
│                                                                             │
│   Reading process:                                                          │
│   1. Set all strobes HIGH                                                   │
│   2. Set Strobe 0 LOW, read DATA                                            │
│   3. Set Strobe 0 HIGH, set Strobe 1 LOW, read DATA                         │
│   4. Repeat for all strobes                                                 │
│                                                                             │
│   The DATA line will be LOW when connected to the active (LOW) strobe.      │
│   The bit pattern indicates which position is selected.                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 12.2 Configuring in InputMapping.h

```cpp
//  label                source   port      bit  hidId  DCSCommand        value  Type       group
{ "ALR67_MODE_OFF",      "MATRIX", PIN(10),   1,  -1, "ALR67_POWER",         0, "selector",   20 },
{ "ALR67_MODE_STBY",     "MATRIX", PIN(10),   2,  -1, "ALR67_POWER",         1, "selector",   20 },
{ "ALR67_MODE_ON",       "MATRIX", PIN(10),   4,  -1, "ALR67_POWER",         2, "selector",   20 },
{ "ALR67_MODE_FAIL",     "MATRIX", PIN(10),  -1,  -1, "ALR67_POWER",         3, "selector",   20 },
```

**Fields:**
- `source = "MATRIX"` — identifies matrix rotary input
- `port` — DATA pin GPIO
- `bit` — pattern value (decimal representation of which strobe is active)
- `bit = -1` — fallback position

---

## 13. Servo Motors and Analog Gauges

Servo motors can drive physical gauge needles for immersive cockpit instruments.

### 13.1 Servo Basics

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      SERVO MOTOR BASICS                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Hobby servos have 3 wires:                                                │
│   - Signal (usually white or orange)                                        │
│   - VCC (usually red) — typically 5V                                        │
│   - GND (usually black or brown)                                            │
│                                                                             │
│   Control via PWM:                                                          │
│   - Period: 20ms (50Hz)                                                     │
│   - Pulse width determines position:                                        │
│     • ~1000µs = 0° (full left)                                              │
│     • ~1500µs = 90° (center)                                                │
│     • ~2000µs = 180° (full right)                                           │
│                                                                             │
│       ─────┐                           ┌─────────────────────────           │
│            │                           │                                    │
│            │◄── 1ms pulse ──►│◄── 19ms off ──►│                            │
│            └───────────────────────────┘                                    │
│                                                                             │
│       ─────────┐                       ┌─────────────────────────           │
│                │                       │                                    │
│                │◄── 2ms pulse ──►│◄── 18ms off ──►│                        │
│                └───────────────────────┘                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 13.2 Wiring a Servo

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      SERVO WIRING                                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                         Servo                  │
│                     ┌─────────┐                   ┌─────────┐               │
│                     │         │                   │         │               │
│                     │ GPIO 18 ├───────────────────┤ Signal  │               │
│                     │         │                   │ (PWM)   │               │
│                     │         │                   │         │               │
│                     │   GND   ├───────────────────┤ GND     │               │
│                     │         │                   │         │               │
│                     │         │      5V Supply ───┤ VCC     │               │
│                     │         │                   │         │               │
│                     └─────────┘                   └─────────┘               │
│                                                                             │
│   ⚠️ Don't power servos from ESP32's 5V pin — they draw too much current.  │
│      Use a separate 5V power supply with common ground.                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 13.3 Configuring in LEDMapping.h

```cpp
//  label              deviceType    info                                         dimmable  activeLow
{ "FUEL_QTY_L",        DEVICE_GAUGE, { .gaugeInfo = { PIN(18), 1000, 2000, 20000 }}, true,    false },
```

**Fields:**
- `.gaugeInfo = { gpio, minPulse, maxPulse, period }`
- `gpio` — PWM-capable GPIO pin
- `minPulse` — minimum pulse width in µs (0° position)
- `maxPulse` — maximum pulse width in µs (180° position)
- `period` — PWM period in µs (20000 = 50Hz standard)

**Calibration:**
- `minPulse = 1000` and `maxPulse = 2000` are typical starting values
- Adjust based on your servo's actual range and gauge travel
- Some gauges only need 90° of travel — adjust accordingly

---

## 14. The METADATA Override System

The METADATA system lets you add custom controls that don't exist in the aircraft JSON without modifying the original file.

### 14.1 When to Use METADATA

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    WHEN TO USE METADATA                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   USE METADATA when you need to:                                            │
│                                                                             │
│   ✓ Add custom controls not in the aircraft JSON                            │
│   ✓ Track internal state (NVG mode, day/night lighting)                     │
│   ✓ Create synthetic displays or indicators                                 │
│   ✓ Add controls for hardware testing without DCS                           │
│   ✓ Override or extend existing control definitions                         │
│                                                                             │
│   DON'T USE METADATA for:                                                   │
│                                                                             │
│   ✗ Controls that already exist in the aircraft JSON                        │
│   ✗ Simple hardware configuration (use InputMapping/LEDMapping)             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 14.2 METADATA Directory Structure

Create a `METADATA/` folder inside your Label Set directory:

```
src/LABELS/LABEL_SET_MYPANEL/
├── FA-18C_hornet.json        ← Aircraft JSON (from DCS-BIOS)
├── selected_panels.txt
├── generate_data.py
├── METADATA/                 ← Create this folder
│   ├── custom_controls.json  ← Your custom definitions
│   └── nvg_mode.json         ← Another custom file
└── ...
```

### 14.3 METADATA JSON Format

The format mirrors the aircraft JSON structure:

```json
{
  "Custom Panel": {
    "CUSTOM_BRIGHTNESS": {
      "category": "Custom Panel",
      "control_type": "metadata",
      "description": "Custom brightness control for panel lighting",
      "identifier": "CUSTOM_BRIGHTNESS",
      "outputs": [
        {
          "address": 65534,
          "mask": 65535,
          "shift_by": 0,
          "max_value": 65535
        }
      ]
    },
    "NVG_MODE_ACTIVE": {
      "category": "Custom Panel",
      "control_type": "metadata",
      "description": "Tracks whether NVG mode is active",
      "identifier": "NVG_MODE_ACTIVE",
      "outputs": [
        {
          "address": 65533,
          "mask": 1,
          "shift_by": 0,
          "max_value": 1
        }
      ]
    }
  }
}
```

**Key points:**
- `control_type = "metadata"` — special type for custom tracking
- Choose unique `address` values that don't conflict with aircraft data
- The generator merges METADATA with the aircraft JSON during generation

### 14.4 How METADATA Merging Works

When you run `generate_data.py`:

1. Generator loads the aircraft JSON
2. Generator scans the `METADATA/` folder for `.json` files
3. Each METADATA file is merged into the data
4. **Aircraft JSON takes precedence** — if a label exists in both, aircraft wins
5. Merged data is used to generate all mapping files

This means you can:
- Add new categories/panels
- Add new controls to existing categories
- NOT override existing aircraft controls (by design)

### 14.5 Accessing METADATA Values in Code

The generator creates tracking structures for metadata controls:

```cpp
// Auto-generated in DCSBIOSBridgeData.h
struct MetadataState {
    const char* label;
    uint16_t    value;
};
static MetadataState metadataStates[] = {
    { "CUSTOM_BRIGHTNESS", 0 },
    { "NVG_MODE_ACTIVE", 0 },
};
```

Your panel code can read/write these values to track internal state.

---

## 15. Pin Mapping and Board Differences

Different ESP32 boards have different pin layouts. CockpitOS handles this with the `PIN()` macro.

### 15.1 The PIN() Macro

```cpp
// In your configuration
{ "MY_BUTTON", "GPIO", PIN(12), 0, -1, "BUTTON", 1, "momentary", 0 },
```

The `PIN()` macro translates pin numbers between boards. For example, if you develop on an S2 Mini and later switch to an S3 Mini, your pin numbers automatically adjust.

### 15.2 S2 to S3 Pin Mapping

| S2 Mini GPIO | S3 Mini GPIO (same position) |
|--------------|------------------------------|
| 3 | 2 |
| 5 | 4 |
| 7 | 12 |
| 9 | 13 |
| 12 | 10 |
| 2 | 3 |
| 4 | 5 |
| 8 | 7 |
| 10 | 8 |
| 13 | 9 |
| 40 | 33 |
| 38 | 37 |
| 36 | 38 |
| 39 | 43 |
| 37 | 44 |
| 35 | 36 |
| 33 | 35 |

### 15.3 Pins to Avoid

| ESP32 Variant | Pins to Avoid | Reason |
|---------------|---------------|--------|
| All | GPIO 0 | Boot mode selection |
| All | GPIO 6-11 | Internal flash |
| ESP32 Classic | GPIO 34-39 | Input only |
| S2/S3 | GPIO 19-20 | USB D+/D- |

---

## 16. Power Considerations

### 16.1 Current Budgets

| Component | Typical Current | Notes |
|-----------|-----------------|-------|
| ESP32 | 80-240mA | Depends on WiFi/BLE usage |
| Single LED | 10-20mA | With proper resistor |
| WS2812 LED | Up to 60mA | At full white brightness |
| Servo motor | 100-500mA | Higher during movement |
| PCA9555 | 1mA | Very low |
| 74HC165 | 1mA | Very low |

### 16.2 Power Distribution

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    POWER DISTRIBUTION                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   For small panels (< 10 LEDs, no servos):                                  │
│   - USB power is usually sufficient                                         │
│   - ESP32's 3.3V regulator can power low-current components                 │
│                                                                             │
│   For larger panels:                                                        │
│   - Use a separate 5V power supply                                          │
│   - Connect GND from power supply to ESP32 GND (IMPORTANT!)                 │
│   - Power WS2812 strips and servos from external supply                     │
│   - Add capacitors (100µF-1000µF) near high-current components              │
│                                                                             │
│         ┌─────────────────────────────────────────────────────────┐         │
│         │                     5V Power Supply                     │         │
│         │                    (2A or more)                         │         │
│         └────────┬────────────────────┬───────────────────────────┘         │
│                  │                    │                                     │
│                  │                    │                                     │
│         ┌───────┴───────┐    ┌───────┴───────┐                              │
│         │    ESP32      │    │   WS2812      │                              │
│         │   (via USB)   │    │   Strip       │                              │
│         └───────┬───────┘    └───────────────┘                              │
│                 │                                                           │
│                 │         ← COMMON GROUND                                   │
│         ════════╧══════════════════════════════════                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 16.3 Decoupling Capacitors

Add 0.1µF ceramic capacitors close to IC power pins to filter noise:

- Near each PCA9555
- Near each 74HC165
- Near WS2812 strip start

---

## 17. Troubleshooting Hardware

### 17.1 Button/Switch Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  Button press not detected                                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│  CHECKS                                                                     │
│                                                                             │
│  1. Verify wiring: button should connect GPIO to GND                        │
│  2. Check InputMapping.h configuration:                                     │
│     - Is source = "GPIO"?                                                   │
│     - Is port = PIN(correct_number)?                                        │
│  3. Test with multimeter: does button create continuity?                    │
│  4. Enable DEBUG_ENABLED in Config.h and check Serial output                │
│  5. Try a different GPIO pin                                                │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  Button triggers randomly / multiple times                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│  CHECKS                                                                     │
│                                                                             │
│  1. Switch bounce — CockpitOS debounces, but check selector group config    │
│  2. Floating input — ensure pull-up is enabled (automatic for GPIO)         │
│  3. Electrical noise — add 0.1µF capacitor between GPIO and GND             │
│  4. Long wires — keep button wires short or add shielding                   │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 17.2 LED Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  LED always ON or always OFF                                                │
├─────────────────────────────────────────────────────────────────────────────┤
│  CHECKS                                                                     │
│                                                                             │
│  1. Check activeLow flag in LEDMapping.h — does it match your wiring?       │
│  2. Verify LED polarity — anode (+) toward positive                         │
│  3. Check resistor value — too high = dim, too low = damage                 │
│  4. Test LED directly: connect to 3.3V through 100Ω resistor                │
│  5. Use TEST_LEDS mode to verify firmware sees the LED                      │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 17.3 I²C (PCA9555) Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  PCA9555 not responding                                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│  CHECKS                                                                     │
│                                                                             │
│  1. Verify I²C address matches A0/A1/A2 pin configuration                   │
│  2. Check pull-up resistors on SDA/SCL (4.7kΩ typical)                     │
│  3. Verify ENABLE_PCA9555 = 1 in Config.h                                   │
│  4. Run I²C scanner to detect connected devices                             │
│  5. Check wiring: SDA to SDA, SCL to SCL (don't cross!)                    │
│  6. Try lower I²C speed: set PCA_FAST_MODE = 0                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 17.4 Encoder Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  Encoder skips steps or moves wrong direction                               │
├─────────────────────────────────────────────────────────────────────────────┤
│  CHECKS                                                                     │
│                                                                             │
│  1. Swap A and B pins if direction is reversed                              │
│  2. Check pin assignments — both pins need to match encoder channels        │
│  3. Verify both entries in InputMapping.h share same oride_label            │
│  4. Check controlType is "fixed_step" or "variable_step"                    │
│  5. Encoder may need external pull-ups if internal aren't strong enough     │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 18. Quick Reference Tables

### 18.1 Input Source Types

| Source | Description | Example Usage |
|--------|-------------|---------------|
| `"GPIO"` | Direct ESP32 GPIO | Simple buttons, encoders |
| `"HC165"` | 74HC165 shift register | Expanding inputs (8+ per chip) |
| `"PCA_0xNN"` | PCA9555 at address 0xNN | I²C GPIO expander |
| `"TM1637"` | TM1637 keypad | Button matrices |
| `"MATRIX"` | Matrix rotary switch | Multi-position selectors |
| `"NONE"` | Virtual/disabled | Fallback positions, unused |

### 18.2 Control Types

| Type | Description | Use Case |
|------|-------------|----------|
| `"momentary"` | Press/release | Buttons, guarded switches |
| `"selector"` | Fixed positions | Toggle switches, rotary selectors |
| `"analog"` | Continuous value | Potentiometers, axes |
| `"variable_step"` | Inc/dec with variable amount | Volume knobs |
| `"fixed_step"` | Inc/dec with fixed steps | Channel selectors |

### 18.3 LED Device Types

| Device Type | Description | Info Struct |
|-------------|-------------|-------------|
| `DEVICE_GPIO` | Direct GPIO LED | `{ gpio }` |
| `DEVICE_WS2812` | Addressable RGB | `{ index, pin, R, G, B, bright }` |
| `DEVICE_TM1637` | TM1637 segment | `{ clkPin, dioPin, segment, bit }` |
| `DEVICE_PCA9555` | PCA9555 output | `{ address, port, bit }` |
| `DEVICE_GN1640T` | GN1640T matrix | `{ address, column, row }` |
| `DEVICE_GAUGE` | Servo motor | `{ gpio, minPulse, maxPulse, period }` |
| `DEVICE_NONE` | Disabled/virtual | `{ 0 }` |

### 18.4 Common Resistor Values

| LED Color | Forward Voltage | Resistor @ 15mA |
|-----------|-----------------|-----------------|
| Red | ~2.0V | 100Ω |
| Orange | ~2.1V | 82Ω |
| Yellow | ~2.1V | 82Ω |
| Green | ~2.2V | 68Ω |
| Blue | ~3.0V | 22Ω |
| White | ~3.0V | 22Ω |

---

## What's Next?

With your hardware wired and configured:

1. **Test individual components** using `TEST_LEDS` and debug modes
2. **Configure your Label Set** (see CREATING_LABEL_SETS.md)
3. **Connect to DCS** (see TRANSPORT_MODES.md)
4. **Expand your panel** with more inputs and outputs

---

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│    Need help?                                                               │
│                                                                             │
│    • GitHub Issues: github.com/BojoteX/CockpitOS/issues                     │
│    • Check the README.md in each directory for specific guidance            │
│    • A custom-trained GPT assistant is available — see the                  │
│      GitHub page for the link                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

*Guide version: 1.0 | Last updated: January 2025*  
*Tested with: CockpitOS 1.x, ESP32 Arduino Core 3.3.x*
