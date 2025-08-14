# Cockpit Utils (CUtils): Hardware Abstraction Layer

**Cockpit Utils (CUtils)** is the core hardware abstraction layer for CockpitOS.  
It implements all low-level device drivers and exposes a unified, safe API for panel logic to access all supported hardware types: GPIO, I2C expanders, matrix LEDs, segment displays, shift registers, and more.

> **Key Point:**  
> Users never interact with hardware directly in panel code. All interaction goes through the shared functions exposed in `CUtils.h`.

---

## Features

- **Zero dynamic memory:**  
  All buffers and device structures are statically allocated for mission-critical, deterministic performance.

- **All drivers always compiled:**  
  Every hardware module (GPIO, PCA9555, GN1640, TM1637, HC165, HT1622, WS2812, MatrixRotary) is included and ready, regardless of whether a specific build uses them.

- **Robust device abstraction:**  
  - Centralizes all hardware access.
  - Handles pin assignments, I2C discovery, LED sweep/test, display commit, shift register polling, and more.
  - Strict, runtime-safe function signatures: no hidden heap, no exceptions, no ambiguity.

- **Expansion ready:**  
  New hardware types are added by creating a new file, exposing its functions via `CUtils.h`, and (optionally) including it in `CUtils.cpp` using the established include pattern.

---

## Directory Structure

```
/lib/
  CUtils
    src
      CUtils.h
      CUtils.cpp
      internal/
        GPIO.cpp
        PCA9555.cpp
        GN1640.cpp
        TM1637.cpp
        WS2812.cpp
        MatrixRotary.cpp
        HC165.cpp
        HT1622.cpp
        ... (future hardware drivers)
```

---

## How CUtils Works

- **CUtils.cpp** includes all hardware modules via its `internal/` directory.
- Each driver is strictly panel-agnostic: it exposes routines for setup, reading/writing state, sweeping, or committing updates for its hardware type.
- Panel logic interacts with the hardware **only via CUtils.h**—never directly or by creating duplicate implementations.
- Global device arrays, runtime booleans, and static mappings are maintained in CUtils for efficient panel discovery and polling.

---

## Example API Surface (Partial)

- **GPIO & Analog Gauges:**
  ```cpp
  void GPIO_setAllLEDs(bool state);
  void GPIO_setDigital(uint8_t pin, bool activeHigh, bool state);
  void GPIO_setAnalog(uint8_t pin, bool activeLow, uint8_t intensity);
  void resetAllGauges();
  void preconfigureGPIO();
  ```

- **WS2812 RGB LEDs:**
  ```cpp
  void WS2812_init();
  void WS2812_setLEDColor(uint8_t idx, CRGB color);
  void WS2812_allOn(CRGB color);
  void WS2812_allOff();
  void WS2812_tick();
  ```

- **TM1637 4-digit displays/buttons:**
  ```cpp
  void tm1637_init(TM1637Device&, uint8_t clk, uint8_t dio);
  uint8_t tm1637_readKeys(TM1637Device&);
  void tm1637_displaySingleLED(TM1637Device&, uint8_t grid, uint8_t seg, bool on);
  void tm1637_tick();
  ```

- **GN1640 LED matrix:**
  ```cpp
  void GN1640_init(uint8_t clkPin, uint8_t dioPin);
  void GN1640_setLED(uint8_t row, uint8_t col, bool state);
  void GN1640_allOn();
  void GN1640_allOff();
  void GN1640_tick();
  ```

- **PCA9555 I2C expanders:**
  ```cpp
  void PCA9555_scanConnectedPanels();
  bool readPCA9555(uint8_t addr, byte &p0, byte &p1);
  void PCA9555_write(uint8_t addr, uint8_t port, uint8_t bit, bool state);
  void PCA9555_autoInitFromLEDMap(uint8_t addr);
  ```

- **Matrix Rotary Selectors:**
  ```cpp
  uint8_t MatrixRotary_readPattern(const int* strobes, int count, int dataPin);
  ```

- **74HC165 Shift Register:**
  ```cpp
  void HC165_init(uint8_t pinPL, uint8_t pinCP, uint8_t pinQH);
  uint8_t HC165_read();
  ```

- **HT1622 Segment Display:**
  ```cpp
  HT1622(uint8_t cs, uint8_t wr, uint8_t data);
  void HT1622::init();
  void HT1622::commit(const uint8_t* shadow, uint8_t* lastShadow, int shadowLen);
  ```

---

## Usage Pattern

1. **Expand hardware support:**  
   Add a new driver .cpp file in `internal/`, expose its API in `CUtils.h`, and include it in `CUtils.cpp`.

2. **Panel logic:**  
   Only call CUtils API functions—never access hardware directly or duplicate logic.

3. **Panel detection and runtime flags:**  
   Device presence and logic are managed with runtime booleans (`hasIFEI`, `hasCA`, etc.), which are set by CUtils functions and referenced throughout the firmware.

---

## Tips for Contributors

- **Study the included driver modules for reference.**
- Use the established naming and API conventions—don't invent per-panel routines for shared hardware.
- **Never** use dynamic allocation, exceptions, or blocking calls.
- **Always** keep panel code minimal: all device-specific logic belongs in CUtils or HIDManager.

---

## Adding Support for New Hardware

1. Write a minimal, static driver (no heap) for your device in `/internal/`.
2. Expose your new routines in `CUtils.h`.
3. `#include` your new driver file in `CUtils.cpp`.
4. Use only these routines from your panel .cpp/.h file.

---

For examples of actual panel integration, see the included panel source files (`ECMPanel.cpp`, `IFEIPanel.cpp`, etc.).  
For HID and logical action integration, see `HIDManager.cpp`.

