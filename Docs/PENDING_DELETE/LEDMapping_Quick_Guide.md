# LEDMapping Quick Guide

This maps every cockpit “light-like” item to:

1. **Who** drives it (hardware driver)  
2. **What** info that driver needs (pins, addresses, indexes, etc.)  
3. **How** it behaves (dimmable? active-low?)

> Edit **only** the individual records in `panelLEDs[]`.  
> Do **not** add or remove records; the generator depends on the list length.

---

## Struct Overview

```cpp
// LEDMapping.h (excerpt)

enum LEDDeviceType {
  DEVICE_GPIO,
  DEVICE_TM1637,
  DEVICE_PCA9555,
  DEVICE_WS2812,
  DEVICE_GN1640T,
  DEVICE_NONE,
  DEVICE_GAUGE,
};

struct LEDMapping {
  const char* label;          // human-readable name used by the firmware
  LEDDeviceType deviceType;   // which driver controls this LED

  union { // driver-specific info (fill exactly one)
    struct { uint8_t gpio; }                                                          gpioInfo;    // GPIO LED
    struct { uint8_t gpio; uint16_t minPulse; uint16_t maxPulse; uint16_t period; }   gaugeInfo;   // Servo/gauge
    struct { uint8_t address; uint8_t port; uint8_t bit; }                            pcaInfo;     // PCA9555 expander
    struct { uint8_t clkPin; uint8_t dioPin; uint8_t segment; uint8_t bit; }          tm1637Info;  // TM1637
    struct { uint8_t address; uint8_t column; uint8_t row; }                          gn1640Info;  // GN1640T matrix

    // WS2812: 'index' must remain first to preserve older one-field initializers { .ws2812Info = {3} }
    struct { uint8_t index; uint8_t pin; uint8_t defR; uint8_t defG; uint8_t defB; uint8_t defBright; } ws2812Info;
  } info;

  bool dimmable;   // true if brightness can vary (PWM/WS2812/etc.)
  bool activeLow;  // true if “0” turns LED ON (wired inversion); ignored for WS2812/TM1637
};
```

---

## Device Types and Required `info`

- **DEVICE_TM1637** — 7‑segment/indicator driver (Left/Right Annunciators)  
  `tm1637Info = { clkPin, dioPin, segment, bit }`  
  - `clkPin` / `dioPin`: `PIN(x)` macro or raw GPIO  
  - `segment` / `bit`: position of the indicator inside the module

- **DEVICE_GPIO** — plain ESP32 GPIO LED  
  `gpioInfo = { gpio }`  
  - `gpio`: the ESP32 GPIO that drives this LED  
  - `dimmable = true` enables PWM via `analogWrite()`  
  - `activeLow = true` if ON = logic LOW (inverted wiring/transistor)

- **DEVICE_WS2812** — addressable RGB LED (NeoPixel)  
  `ws2812Info = { index, pin, defR, defG, defB, defBright }`  
  - `index`: LED position on the strip (0‑based). **Must remain first**  
  - `pin`: data pin for this strip  
  - `defR/defG/defB` (0..255): default color used at init/test  
  - `defBright` (0..255): default brightness used at init/test  
  - `dimmable` typically `true` for “_F” entries that use brightness  
  - `activeLow` ignored

- **DEVICE_GAUGE** — analog gauge/servo  
  `gaugeInfo = { gpio, minPulse, maxPulse, period }`  
  - `gpio`: PWM‑capable pin  
  - `minPulse`/`maxPulse` in microseconds (e.g., 1000..2000)  
  - `period` in microseconds (e.g., 20000 = 50 Hz)

- **DEVICE_GN1640T** — matrix LED driver (Caution Advisory)  
  `gn1640Info = { address, column, row }`  
  - `address`: I²C address (e.g., 0x24)  
  - `column`, `row`: matrix position

- **DEVICE_PCA9555** — I/O expander outputs used as LEDs  
  `pcaInfo = { address, port, bit }`  
  - `address`: I²C address (e.g., 0x5B)  
  - `port`: 0 or 1  
  - `bit`: 0..7  
  - `activeLow = true` if hardware inverts the output

- **DEVICE_NONE** — placeholder (not used yet)

---

## How to Fill a `panelLEDs[]` Line

1. **Pick the driver** in `deviceType`.  
2. **Fill the matching `info`** block for that driver (and only that one).  
3. **Set `dimmable`:**  
   - `true` if brightness varies (WS2812 “_F”, PWM GPIO, gauges)  
   - `false` for simple on/off  
4. **Set `activeLow`:**  
   - `true` if ON = LOW (GPIO/PCA inversion)  
   - `false` otherwise; ignored for WS2812/TM1637

---

## Examples

### WS2812 (Lock/Shoot, AOA)

```cpp
{ "LS_LOCK", DEVICE_WS2812,
  { .ws2812Info = { /*index*/0, /*pin*/PIN(12), /*R*/0,   /*G*/255, /*B*/0,   /*bright*/255 } },
  false, false },

{ "AOA_INDEXER_NORMAL_F", DEVICE_WS2812,
  { .ws2812Info = { /*index*/4, /*pin*/PIN(15), /*R*/255, /*G*/165, /*B*/0,   /*bright*/200 } },
  true,  false },
```

### GPIO

```cpp
{ "SPIN_LT", DEVICE_GPIO,
  { .gpioInfo = { PIN(34) } },
  false, false },

{ "BACKLIGHT", DEVICE_GPIO,
  { .gpioInfo = { PIN(6) } },
  true,  false }, // dimmable via PWM
```

### TM1637

```cpp
{ "LH_ADV_GO", DEVICE_TM1637,
  { .tm1637Info = { PIN(37), PIN(39), /*segment*/0, /*bit*/0 } },
  false, false },
```

### PCA9555

```cpp
{ "MASTER_MODE_AA_LT", DEVICE_PCA9555,
  { .pcaInfo = { 0x5B, /*port*/1, /*bit*/3 } },
  false, true }, // activeLow since expander output is inverted
```

### GN1640T

```cpp
{ "CA_LIGHT", DEVICE_GN1640T,
  { .gn1640Info = { 0x24, /*col*/1, /*row*/3 } },
  false, false },
```

### GAUGE

```cpp
{ "FUEL_QTY", DEVICE_GAUGE,
  { .gaugeInfo = { PIN(10), /*min*/1000, /*max*/2000, /*period*/20000 } },
  true, false },
```

### DEVICE_NONE

```cpp
{ "AOA_INDEXER_LOW", DEVICE_NONE,
  { .gpioInfo = { 0 } },
  false, false },
```

---

## Notes

- For WS2812, **`index` must remain the first field** to keep older one‑field initializers like `{ .ws2812Info = {3} }` working.  
- Use your `PIN(x)` macro for clarity and portability.  
- If unsure about `activeLow`, leave it `false`. It only affects GPIO/PCA.  
- The auto‑generator reads the whole initializer, so adding the new WS2812 fields is safe.
