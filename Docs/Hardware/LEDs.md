# LEDs

LEDs provide visual feedback in your cockpit panel -- caution lights, indicator panels, backlit buttons, and status annunciators. CockpitOS supports single LEDs on GPIO pins, LEDs driven through expanders (PCA9555, TM1637, GN1640T), and addressable WS2812 RGB LEDs.

---

## Single LED Wiring

### Active HIGH (Most Common)

The GPIO pin sources current through a resistor and LED to ground. When the GPIO goes HIGH, the LED turns on.

```
                    ESP32
                 +---------+
                 |         |
                 |  GPIO 5 +----[330R]----+---- LED (+) Anode
                 |         |             |
                 |         |             +---- LED (-) Cathode
                 |         |                        |
                 |    GND  +------------------------+
                 |         |
                 +---------+

   GPIO HIGH (3.3V) --> LED ON
   GPIO LOW  (0V)   --> LED OFF

   In LEDMapping.h: activeLow = false
```

### Active LOW

The LED is wired between 3.3V and the GPIO pin (through a resistor). When the GPIO goes LOW, current flows and the LED turns on.

```
                    ESP32
                 +---------+
                 |         |
                 |   3.3V  +----------------+---- LED (+) Anode
                 |         |                |
                 |         |                +---- LED (-) Cathode
                 |         |                           |
                 |  GPIO 5 +----[330R]-----------------+
                 |         |
                 +---------+

   GPIO LOW  (0V)   --> LED ON  (current flows from 3.3V through LED to GPIO)
   GPIO HIGH (3.3V) --> LED OFF (no voltage difference)

   In LEDMapping.h: activeLow = true
```

### Resistor Selection

LEDs require a current-limiting resistor. For 3.3V GPIO output at approximately 15mA:

| LED Color    | Forward Voltage | Recommended Resistor |
|--------------|-----------------|----------------------|
| Red          | ~2.0V           | 100-220 ohm          |
| Orange       | ~2.1V           | 82-150 ohm           |
| Yellow       | ~2.1V           | 82-150 ohm           |
| Green        | ~2.2V           | 68-150 ohm           |
| Blue / White | ~3.0V           | 22-47 ohm            |

A 330 ohm resistor is a safe universal choice for any LED color at reduced brightness.

---

## Configuring LEDs in LEDMapping.h

### Basic GPIO LED

```cpp
//  label               deviceType   info                       dimmable  activeLow
{ "MASTER_CAUTION_LT",  DEVICE_GPIO, { .gpioInfo = { PIN(5) } }, false,    false },
```

**Field reference:**

| Field      | Value                      | Meaning                                    |
|------------|----------------------------|--------------------------------------------|
| label      | `"MASTER_CAUTION_LT"`      | Must match a DCS-BIOS output label         |
| deviceType | `DEVICE_GPIO`              | Direct GPIO output                         |
| info       | `{ .gpioInfo = { PIN(5) } }` | GPIO pin number                         |
| dimmable   | `false`                    | ON/OFF only (no PWM dimming)               |
| activeLow  | `false`                    | Active HIGH wiring (see diagrams above)    |

### Dimmable LED (PWM)

Setting `dimmable = true` enables PWM output. CockpitOS maps the DCS-BIOS value (0-65535) to a PWM duty cycle (0-255), allowing the LED brightness to track a dimmer control in the cockpit.

```cpp
{ "INST_PNL_DIMMER", DEVICE_GPIO, { .gpioInfo = { PIN(6) } }, true, false },
```

**PWM intensity mapping:**

| DCS-BIOS Value | PWM Value | Brightness |
|----------------|-----------|------------|
| 0              | 0         | OFF        |
| 16383          | 63        | 25%        |
| 32767          | 127       | 50%        |
| 49151          | 191       | 75%        |
| 65535          | 255       | 100%       |

For active-low LEDs, the PWM value is automatically inverted (`255 - value`).

---

## LEDs on Expanders

### PCA9555

LEDs can be driven from PCA9555 output pins for panels that need more outputs than GPIO alone can provide.

```cpp
{ "MASTER_MODE_AA_LT", DEVICE_PCA9555, { .pcaInfo = { 0x5B, 1, 3 } }, false, true },
```

- `.pcaInfo = { address, port, bit }` -- I2C address, port (0 or 1), bit (0-7)
- See [I2C Expanders](I2C-Expanders.md) for wiring details

### TM1637

TM1637 modules can drive individual LED segments in annunciator panels. Each LED is addressed by its segment and bit position within the TM1637's display RAM.

```cpp
{ "LH_ADV_GO", DEVICE_TM1637, { .tm1637Info = { PIN(37), PIN(39), 0, 0 } }, false, false },
```

- `.tm1637Info = { clkPin, dioPin, segment, bit }` -- clock GPIO, data GPIO, grid position, segment bit
- See [Displays](Displays.md) for TM1637 wiring and addressing

### GN1640T

GN1640T LED matrix drivers address LEDs by column and row within an 8x8 grid.

```cpp
{ "WARN_APU", DEVICE_GN1640T, { .gn1640Info = { 0, 0, 3 } }, false, false },
```

- `.gn1640Info = { address, column, row }` -- device address, column (0-7), row (0-7)
- See [Displays](Displays.md) for GN1640T details

---

## WS2812 Addressable RGB LEDs

WS2812 (NeoPixel) LEDs are individually addressable RGB LEDs on a single data line. Each LED can display any color at any brightness, controlled independently.

### How WS2812 Works

```
                    ESP32
                 +---------+
                 |         |     +-----+   +-----+   +-----+
                 | GPIO 35 +---->|LED 0|-->|LED 1|-->|LED 2|--> ...
                 |  (Data) |     |DIN  |   |DIN  |   |DIN  |
                 |         |     +--+--+   +--+--+   +--+--+
                 |    5V   +--------+----------+----------+-------- 5V Supply
                 |   GND   +--------+----------+----------+--------
                 |         |
                 +---------+

   Each LED is addressed by INDEX (position in chain: 0, 1, 2, ...)
   Single data line carries color data at ~800 kHz
```

### Wiring Notes

- **Power**: WS2812 LEDs need 5V. Each LED draws up to 60mA at full white. Use an external 5V power supply for more than a few LEDs.
- **Data**: 3.3V data from the ESP32 usually works for short runs (under 30cm). For longer runs, use a 3.3V-to-5V level shifter.
- **Protection**: Add a 300-500 ohm resistor in series on the data line and a 100-1000uF capacitor across the power supply near the first LED.
- **Ground**: Connect ESP32 GND to the strip GND and the external supply GND (common ground).

### LEDMapping.h Configuration

```cpp
//  label             deviceType    info                                              dimmable  activeLow
{ "AOA_INDEXER_HIGH", DEVICE_WS2812, { .ws2812Info = { 3, WS2812B_PIN, 0, 255, 0, 200 } }, true,  false },
```

**Field reference for ws2812Info:**

| Field     | Example       | Meaning                                    |
|-----------|---------------|--------------------------------------------|
| index     | `3`           | LED position in chain (0 = first)          |
| pin       | `WS2812B_PIN` | GPIO connected to DIN (defined in CustomPins.h) |
| defR      | `0`           | Default red component (0-255)              |
| defG      | `255`         | Default green component (0-255)            |
| defB      | `0`           | Default blue component (0-255)             |
| defBright | `200`         | Default brightness (0-255)                 |

### Common Colors

| Color   | R   | G   | B   |
|---------|-----|-----|-----|
| Red     | 255 | 0   | 0   |
| Green   | 0   | 255 | 0   |
| Blue    | 0   | 0   | 255 |
| White   | 255 | 255 | 255 |
| Orange  | 255 | 165 | 0   |
| Yellow  | 255 | 255 | 0   |
| Amber   | 255 | 100 | 0   |

### Multiple Strips

CockpitOS supports up to **8 independent WS2812 strips**, each on a different GPIO pin. LED indices are per-strip (each strip starts at index 0).

```cpp
// Strip 1 on GPIO 15
{ "CAUTION_1", DEVICE_WS2812, { .ws2812Info = { 0, PIN(15), 255, 200, 0, 255 } }, false, false },
{ "CAUTION_2", DEVICE_WS2812, { .ws2812Info = { 1, PIN(15), 255, 200, 0, 255 } }, false, false },

// Strip 2 on GPIO 16
{ "GEAR_NOSE", DEVICE_WS2812, { .ws2812Info = { 0, PIN(16), 0, 255, 0, 255 } }, false, false },
{ "GEAR_LEFT", DEVICE_WS2812, { .ws2812Info = { 1, PIN(16), 0, 255, 0, 255 } }, false, false },
```

### DCS-BIOS Color Control

When DCS-BIOS sends a value for a WS2812 LED:

- Value = 0: LED turns OFF
- Value > 0: LED turns ON with the configured default color and brightness
- Dimmable WS2812 LEDs scale brightness based on the DCS-BIOS value

---

## Label Creator Configuration

To configure LEDs using the Label Creator:

1. Open Label Creator and select your Label Set
2. Click **Edit LEDs**
3. Find the indicator label (must match a DCS-BIOS output)
4. Set **Device** to the appropriate type (GPIO, PCA9555, TM1637, GN1640T, WS2812)
5. Fill in the device-specific fields (pin, address, index, color, etc.)
6. Set **Dimmable** and **Active Low** as appropriate
7. Save

---

## Testing LEDs

CockpitOS includes a built-in LED test mode for verifying your wiring before connecting to DCS:

1. In `Config.h`, set `TEST_LEDS` to `1`
2. Compile and upload
3. Open Serial Monitor -- you will see a numbered list of all configured LEDs
4. Enter a number to activate that LED for a few seconds
5. Set `TEST_LEDS` back to `0` for normal operation

---

## LED Device Type Summary

| Device Type      | Hardware                | Info Struct Fields                      |
|------------------|-------------------------|-----------------------------------------|
| `DEVICE_GPIO`    | Direct GPIO pin         | `{ gpio }`                              |
| `DEVICE_PCA9555` | PCA9555 I2C expander    | `{ address, port, bit }`               |
| `DEVICE_TM1637`  | TM1637 segment driver   | `{ clkPin, dioPin, segment, bit }`     |
| `DEVICE_GN1640T` | GN1640T matrix driver   | `{ address, column, row }`             |
| `DEVICE_WS2812`  | WS2812 addressable RGB  | `{ index, pin, R, G, B, brightness }`  |
| `DEVICE_GAUGE`   | Servo motor (gauge)     | `{ gpio, minPulse, maxPulse, period }` |
| `DEVICE_NONE`    | Disabled / not assigned | `{ 0 }`                                |

---

*See also: [Hardware Overview](README.md) | [Buttons and Switches](Buttons-Switches.md) | [Displays](Displays.md)*
