# Hardware Overview

This section covers everything you need to know about connecting physical components to your ESP32 and configuring them in CockpitOS. Each component type has its own dedicated page with wiring diagrams, configuration examples, and troubleshooting tips.

---

## ESP32 Board Selection Guide

CockpitOS supports a wide range of ESP32 variants. Choose your board based on the features you need.

### Board Capabilities

| Feature               | S2       | S3       | P4       | Classic  | C3 / C5 / C6 | H2       |
|-----------------------|----------|----------|----------|----------|---------------|----------|
| USB HID (native)      | Yes      | Yes      | Yes      | No       | No            | No       |
| USB DCS Transport     | Yes      | Yes      | Yes      | No       | No            | No       |
| Wi-Fi                 | Yes      | Yes      | No       | Yes      | Yes           | No       |
| Bluetooth / BLE       | No       | Yes      | No       | Yes      | Yes           | Yes      |
| GPIO Count            | ~27      | ~36      | ~49      | ~25      | ~15-22        | ~22      |
| ADC Channels          | ~10      | ~10      | ~15      | ~15      | ~6            | ~5       |
| Dual Core             | No       | Yes      | Yes      | Yes      | No            | No       |
| I2C Buses             | 2        | 2        | 2        | 2        | 1             | 1        |
| RMT Channels (WS2812) | 4        | 4        | 8        | 8        | 2             | 2        |
| TFT Gauge Support     | Yes      | Yes      | Yes      | Limited  | Limited       | No       |

### Which Board Should I Use?

**ESP32-S3 (Recommended)** -- Best all-around choice. USB HID support, dual-core for TFT gauges, plenty of GPIO, Wi-Fi and BLE capable.

**ESP32-S2** -- Good budget option with USB HID. Single-core limits TFT gauge performance. No Bluetooth.

**ESP32-P4** -- High-performance application MCU with USB HID. No Wi-Fi or Bluetooth radio. Best for complex panels that rely on USB transport.

**ESP32 Classic** -- Legacy option. No native USB (requires Serial/socat or Wi-Fi transport). Still viable for simple panels.

**ESP32-C3 / C5 / C6** -- Low-cost, low pin count. No USB HID. Use Wi-Fi or Serial transport. Suitable for small auxiliary panels.

**ESP32-H2** -- No Wi-Fi. BLE and 802.15.4 (Thread/Zigbee) only. Use BLE or Serial transport. Niche use cases.

### The PIN() Macro

CockpitOS uses a `PIN()` macro to handle GPIO numbering differences between ESP32 variants. When you develop on an S2 board and later switch to an S3, the PIN() macro automatically translates pin numbers so the same physical header position maps correctly.

```cpp
// Always use PIN() in your configurations:
{ "MY_BUTTON", "GPIO", PIN(12), 0, -1, "MY_BUTTON", 1, "momentary", 0 },
```

---

## GPIO Pin Basics

**GPIO** (General Purpose Input/Output) pins are the connections between your ESP32 and external hardware.

### 3.3V Logic

All ESP32 variants operate at **3.3V logic levels**:

- A HIGH signal is approximately 3.3V
- A LOW signal is approximately 0V (GND)
- Connecting 5V directly to a GPIO input can damage the chip

### Pin Capabilities

```
+----------------------------------------------------------------------+
|  GPIO PIN CAPABILITIES                                               |
+----------------------------------------------------------------------+
|                                                                      |
|  DIGITAL INPUT    Any GPIO can read HIGH or LOW (buttons, switches)  |
|  DIGITAL OUTPUT   Any GPIO can drive HIGH or LOW (LEDs, signals)     |
|  ANALOG INPUT     Only ADC-capable pins (varies by board)            |
|  PWM OUTPUT       Most GPIOs support PWM (dimming LEDs, servos)      |
|  I2C              Configurable SDA/SCL pins for I2C bus              |
|  SPI              Configurable MOSI/MISO/CLK/CS for SPI displays     |
|                                                                      |
+----------------------------------------------------------------------+
```

### Internal Pull-Up Resistors

ESP32 GPIOs have built-in pull-up resistors that CockpitOS enables automatically for button and switch inputs. This means you typically only need two wires per button: one to the GPIO pin and one to GND.

### ADC-Capable Pins

Only certain pins support analog input (12-bit ADC, 0-4095 range). On ESP32-S2/S3, GPIO 1-10 are typically ADC-capable. Always check your specific board's pinout documentation.

---

## Electronics Fundamentals

A quick recap of the essential concepts for cockpit panel building.

### Voltage, Current, and Ground

```
+----------------------------------------------------------------------+
|  THE ESSENTIALS                                                      |
+----------------------------------------------------------------------+
|                                                                      |
|  VOLTAGE (V)  = Electrical pressure                                  |
|    - ESP32 logic level: 3.3V                                         |
|    - Some peripherals (WS2812, servos) need 5V power                 |
|                                                                      |
|  CURRENT (mA) = Electrical flow                                      |
|    - A single LED uses 10-20mA                                       |
|    - A WS2812 RGB LED uses up to 60mA at full white                  |
|    - A servo motor uses 100-500mA during movement                    |
|                                                                      |
|  GROUND (GND) = Reference point (0V)                                 |
|    - All components must share a common ground                       |
|    - ESP32 GND must connect to every peripheral's GND                |
|                                                                      |
+----------------------------------------------------------------------+
```

### Pull-Up Resistors and Active LOW

When a GPIO is configured as an input, it needs a defined resting state. A **pull-up resistor** connects the pin to 3.3V so it reads HIGH when nothing is connected. Pressing a button that connects the pin to GND pulls it LOW.

```
        3.3V -----+-----
                   |
                  [R]  10k (internal pull-up)
                   |
        GPIO ------+-----+
                         |
                      [BUTTON]
                         |
        GND  ------------+

   Button OPEN   --> GPIO reads HIGH (3.3V through resistor)
   Button CLOSED --> GPIO reads LOW  (direct path to GND)
```

This is called **active LOW** because the button is considered active (pressed) when the signal is LOW. Most buttons and switches in CockpitOS use active LOW wiring.

### Active HIGH vs Active LOW

| Term        | ON State       | OFF State      | Typical Use         |
|-------------|----------------|----------------|---------------------|
| Active HIGH | GPIO = HIGH    | GPIO = LOW     | LEDs sourced by GPIO |
| Active LOW  | GPIO = LOW     | GPIO = HIGH    | Buttons to GND, sinking LEDs |

The `activeLow` flag in LEDMapping.h tells CockpitOS how your LED is wired so it drives the signal correctly.

---

## Pin Count Limitations and Expanders

A typical ESP32-S3 dev board exposes around 30 usable GPIO pins. A cockpit panel can easily need 50 to 100+ inputs and outputs. CockpitOS supports several expander strategies:

| Method           | Pins Added    | GPIO Cost | Direction     | Best For              |
|------------------|---------------|-----------|---------------|-----------------------|
| PCA9555 (I2C)    | 16 per chip   | 2 (shared)| Input + Output| Mixed I/O panels      |
| 74HC165          | 8 per chip    | 3 (shared)| Input only    | Button-heavy panels   |
| TM1637           | Up to 48 LEDs | 2 per device | Output (+ keys) | Annunciator panels |
| GN1640T          | Up to 64 LEDs | 2 per device | Output only   | LED matrix grids   |
| WS2812           | Unlimited chain| 1 per strip | Output only  | RGB indicators     |

See the dedicated pages for each expander type for wiring and configuration details.

---

## Power Considerations

### 3.3V Logic, 5V Peripherals

The ESP32 runs at 3.3V internally. Some peripherals need 5V power but can accept 3.3V data signals (WS2812 LEDs, some TM1637 modules). Others strictly require level shifting.

**General rules:**

- Buttons and switches: 3.3V domain, no level shifting needed
- PCA9555, 74HC165: Can run at 3.3V directly
- WS2812 LEDs: Need 5V power; 3.3V data usually works for short runs
- Servos: Need 5V power from an external supply, signal is 3.3V compatible
- TFT displays: Typically 3.3V; check your specific module

### USB Power Limits

A standard USB 2.0 port provides up to 500mA. After the ESP32 itself draws 80-240mA, there is limited headroom for peripherals. For panels with more than a few LEDs, WS2812 strips, or servos, use an **external 5V power supply** with a shared ground connection to the ESP32.

### Current Budget Reference

| Component      | Typical Current  | Notes                      |
|----------------|------------------|----------------------------|
| ESP32          | 80-240mA         | Higher with Wi-Fi active   |
| Single LED     | 10-20mA          | With current-limiting resistor |
| WS2812 (each)  | Up to 60mA       | Full white, full brightness |
| Servo motor    | 100-500mA        | Peaks during movement      |
| PCA9555        | ~1mA             | Negligible                 |
| 74HC165        | ~1mA             | Negligible                 |
| TM1637 module  | ~10-80mA         | Depends on segments lit    |

### External Power Wiring

```
    +-----------------------------------------------------------+
    |                 5V Power Supply (2A+)                      |
    +-------+-----------------------------------+---------------+
            |                                   |
            |                                   |
    +-------+-------+              +------------+----------+
    |    ESP32       |              |   WS2812 / Servos    |
    |   (via USB)    |              |   (5V peripherals)   |
    +-------+-------+              +------------+----------+
            |                                   |
            +-----------------------------------+
                      COMMON GROUND
```

Always connect the GND of external supplies to the ESP32 GND. Without a common ground, signals will be unreliable or components may be damaged.

---

## Safe GPIO Pins by Board Variant

Some GPIO pins are reserved for boot strapping, internal flash, or USB and should not be used for general I/O.

### Pins to Avoid

| ESP32 Variant | Avoid These Pins     | Reason                         |
|---------------|----------------------|--------------------------------|
| All variants  | GPIO 0               | Boot mode selection            |
| Classic       | GPIO 6-11            | Connected to internal flash    |
| Classic       | GPIO 34-39           | Input only (no output)         |
| S2 / S3       | GPIO 19-20           | USB D+ / D- (when using USB)   |
| S2            | GPIO 26              | Flash (SPICS1)                 |
| C3            | GPIO 11              | Flash VDD                      |

When in doubt, start with a single button or LED on a pin to verify it works before committing your panel design.

---

## Component Documentation

Each hardware type has a dedicated guide with wiring diagrams and configuration instructions:

| Document                                      | Description                          |
|-----------------------------------------------|--------------------------------------|
| [Buttons and Switches](Buttons-Switches.md)   | Push buttons, toggles, rotary selectors, cover switches |
| [LEDs](LEDs.md)                               | Single LEDs, dimmable LEDs, WS2812 addressable RGB      |
| [Rotary Encoders](Encoders.md)                | Infinite-rotation encoders, fixed/variable step          |
| [Analog Inputs](Analog-Inputs.md)             | Potentiometers, axes, calibration                        |
| [Shift Registers](Shift-Registers.md)         | 74HC165 input expansion                                 |
| [I2C Expanders](I2C-Expanders.md)             | PCA9555 bidirectional GPIO expansion                     |
| [Displays](Displays.md)                       | TM1637, GN1640T, HT1622 segment LCDs                    |
| [TFT Gauges](TFT-Gauges.md)                   | Graphical TFT instrument displays                        |
| [Servo Gauges](Servo-Gauges.md)               | Physical needle gauges driven by servos                  |

---

## Configuration Workflow

All hardware configuration in CockpitOS is managed through the **Label Creator** tool (`LabelCreator-START.py`). You do not need to manually edit InputMapping.h or LEDMapping.h files.

1. **Wire your hardware** using the guides above
2. **Open Label Creator** and select your Label Set
3. **Edit Inputs** to configure buttons, switches, encoders, and analog inputs
4. **Edit LEDs** to configure indicator lights, displays, and gauges
5. **Edit Custom Pins** to configure shift register pins, I2C pins, and feature enables
6. **Compile and upload** using the Compiler Tool or Arduino IDE

For details on the Label Creator, see [Tools > Label Creator](../Tools/Label-Creator.md).

---

*CockpitOS Hardware Documentation | See [Getting Started](../Getting-Started/README.md) for initial setup*
