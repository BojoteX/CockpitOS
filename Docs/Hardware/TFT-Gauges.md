# TFT Gauges

TFT gauges are graphical LCD panels that render instrument displays -- gauge faces with moving needles, digital readouts, and indicator lights. CockpitOS drives these displays using the LovyanGFX library with DMA-accelerated rendering and dirty-rect optimization for smooth, flicker-free updates.

This page provides an overview of TFT gauge capabilities and setup. For a step-by-step guide to wiring and creating TFT gauges, see [How-To/Wire-TFT-Gauges.md](../How-To/Wire-TFT-Gauges.md). For the panel registration system and lifecycle, see [Advanced/Custom-Panels.md](../Advanced/Custom-Panels.md).

---

## What TFT Gauges Are

Each TFT gauge is a small round or rectangular LCD display mounted behind a physical gauge bezel. The ESP32 renders a gauge face image and overlays animated elements (needles, digits, warning lights) that respond to live DCS-BIOS data from the simulator.

```
+----------------------------------------------------------------------+
|  TFT GAUGE ARCHITECTURE                                              |
+----------------------------------------------------------------------+
|                                                                      |
|  DCS World (PC)                                                      |
|       |                                                              |
|       v                                                              |
|  DCS-BIOS data stream  -->  ESP32  -->  Bus  -->  TFT Display       |
|                                |                                     |
|                      +-------------------+                           |
|                      | Rendering Engine  |                           |
|                      |                   |                           |
|                      | - Background image|                           |
|                      | - Needle rotation |                           |
|                      | - Digit overlays  |                           |
|                      | - Indicator lights|                           |
|                      | - Dirty-rect DMA  |  (SPI gauges)            |
|                      | - Framebuffer     |  (RGB parallel)          |
|                      +-------------------+                           |
|                                                                      |
|  SPI gauges:        ESP32 --> SPI --> Display (push dirty rects)     |
|  RGB parallel:      ESP32 --> PSRAM framebuffer (auto-scanned)       |
|                                                                      |
+----------------------------------------------------------------------+
```

### Existing Gauge Implementations

| Gauge               | Display    | Resolution | Shape       | Bus         |
|---------------------|------------|------------|-------------|-------------|
| Battery Voltage     | GC9A01     | 240x240    | Round       | SPI         |
| Brake Pressure      | GC9A01     | 240x240    | Round       | SPI         |
| Hydraulic Pressure  | GC9A01     | 240x240    | Round       | SPI         |
| Cabin Pressure      | ST77916    | 360x360    | Round       | SPI         |
| Radar Altimeter     | ST77916    | 360x360    | Round       | SPI         |
| IFEI (POC)          | ST7262     | 800x480    | Rectangular | RGB Parallel|

---

## Supported Displays

CockpitOS TFT gauges support multiple display controllers through the LovyanGFX library:

### SPI Displays

| Display Controller | Resolution | Shape      | Bus  | Notes                           |
|--------------------|------------|------------|------|---------------------------------|
| GC9A01             | 240x240    | Round      | SPI  | Common, affordable round displays |
| ST77916            | 360x360    | Round      | SPI / QSPI | Waveshare, IdeasPark boards. LovyanGFX uses class `Panel_ST77961` for SPI mode. |
| ST7789             | 240x320    | Rectangular| SPI  | Very common rectangular displays |
| ILI9341            | 240x320    | Rectangular| SPI  | CYD (Cheap Yellow Display) boards |

### RGB Parallel Displays

| Board                           | LCD Controller | Resolution | Touch | IO Expander | Notes |
|---------------------------------|---------------|------------|-------|-------------|-------|
| Waveshare ESP32-S3-Touch-LCD-7  | ST7262        | 800x480    | GT911 (I2C) | CH422G (I2C) | 16-bit RGB565 parallel bus. All 16 data + 4 control GPIOs are hard-wired on the PCB. |

RGB parallel displays use the ESP32-S3's built-in LCD peripheral to continuously scan a framebuffer from PSRAM. There is **no SPI bus** and **no DMA flush** -- the LCD peripheral reads the framebuffer automatically at the pixel clock rate. LovyanGFX classes `Bus_RGB` and `Panel_RGB` handle this.

> **Key difference from SPI:** SPI gauges push pixel data on demand (dirty-rect DMA). RGB parallel displays maintain a persistent framebuffer -- you draw into it and the hardware scans it out continuously. This means any `pushSprite()` or draw call is immediately visible on the next scan.

The LovyanGFX library is installed automatically by the Setup Tool.

---

## Wiring

### SPI Displays

TFT displays using SPI connect to the ESP32 over the SPI bus. The exact pin assignments depend on your board and display module.

### Typical SPI Connections

| Signal | Description              | Notes                            |
|--------|--------------------------|----------------------------------|
| MOSI   | Master Out Slave In      | Data from ESP32 to display       |
| CLK    | SPI Clock                | Clock signal                     |
| CS     | Chip Select              | Selects this display (active LOW)|
| DC     | Data/Command             | Distinguishes data from commands |
| RST    | Reset                    | Display reset (active LOW)       |
| BL     | Backlight                | PWM for brightness control       |
| VCC    | Power                    | 3.3V (check your module)         |
| GND    | Ground                   | Common ground                    |

```
                    ESP32                         TFT Display
                 +---------+                     +----------+
                 |         |                     |          |
                 | MOSI    +---------------------| MOSI/SDA |
                 | CLK     +---------------------| SCK/SCL  |
                 | CS pin  +---------------------| CS       |
                 | DC pin  +---------------------| DC       |
                 | RST pin +---------------------| RST      |
                 | BL pin  +---------------------| BL       |
                 |         |                     |          |
                 |  3.3V   +---------------------| VCC      |
                 |   GND   +---------------------| GND      |
                 |         |                     |          |
                 +---------+                     +----------+
```

Pin assignments are defined in each gauge's source file and can be customized for your board layout.

### RGB Parallel Displays (Waveshare ESP32-S3-Touch-LCD-7)

RGB parallel displays have all pins hard-wired on the PCB -- there is no user wiring. The 16 data lines, 4 control signals, I2C bus, and touch controller are fixed by the board design.

**LovyanGFX requires explicit includes for RGB parallel:**

```cpp
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>   // NOT auto-included
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>      // NOT auto-included
```

These headers are **not** pulled in by `<LovyanGFX.hpp>` automatically. Forgetting them causes "Bus_RGB not found" compile errors.

**CH422G IO Expander (I2C, address-based):**

The Waveshare 7" board uses a CH422G to control backlight, LCD reset, and touch reset. It is **not** register-based like PCA9555 -- it uses different I2C addresses for mode vs output:

| I2C Address | Function | Value |
|-------------|----------|-------|
| `0x24` | Mode register | Write `0x01` = push-pull output mode |
| `0x38` | Output register | Bit 0 (EXIO1) = Touch RST, Bit 1 (EXIO2) = Backlight, Bit 2 (EXIO3) = LCD RST |

Initialize CH422G **before** calling `tft.init()`:

```cpp
Wire.begin(SDA_PIN, SCL_PIN);  // GPIO8, GPIO9

// Set push-pull output mode
Wire.beginTransmission(0x24);
Wire.write(0x01);
Wire.endTransmission();

// Backlight ON, LCD reset HIGH (inactive), touch reset HIGH (inactive)
Wire.beginTransmission(0x38);
Wire.write(0x07);  // bits: EXIO1 | EXIO2 | EXIO3
Wire.endTransmission();
```

**GT911 Touch Controller:**

The GT911 shares the same I2C pins (GPIO8/9) as the CH422G but **must use a different I2C port**:

| Peripheral | I2C Port | Why |
|------------|----------|-----|
| CH422G     | `I2C_NUM_0` (Wire) | `Wire.begin()` claims port 0 |
| GT911 Touch| `I2C_NUM_1` (LovyanGFX) | Must use port 1 to avoid bus conflict |

Set `tcfg.i2c_port = 1` in the touch config. Using port 0 will cause touch initialization to fail or corrupt CH422G communication.

**Waveshare 7" Pin Map (fixed on PCB):**

| Signal Group | Pins |
|-------------|------|
| Red (R0-R4) | GPIO 1, 2, 42, 41, 40 |
| Green (G0-G5) | GPIO 39, 0, 45, 48, 47, 21 |
| Blue (B0-B4) | GPIO 14, 38, 18, 17, 10 |
| HSYNC, VSYNC, PCLK, DE | GPIO 46, 3, 7, 5 |
| Touch SDA, SCL, INT | GPIO 8, 9, 4 |

**LovyanGFX Bus_RGB pin mapping order** (critical -- this is NOT R/G/B order):

```
pin_d0..d4   = B0..B4   (Blue first)
pin_d5..d10  = G0..G5   (Green second)
pin_d11..d15 = R0..R4   (Red last)
```

**Correct timing parameters** (from [paulhamsh/Waveshare-ESP32-S3-LCD-7-LVGL](https://github.com/paulhamsh/Waveshare-ESP32-S3-LCD-7-LVGL)):

```cpp
cfg.freq_write = 16000000;     // 16 MHz pixel clock

cfg.hsync_polarity    = 0;
cfg.hsync_front_porch = 8;
cfg.hsync_pulse_width = 4;
cfg.hsync_back_porch  = 8;

cfg.vsync_polarity    = 0;
cfg.vsync_front_porch = 8;
cfg.vsync_pulse_width = 4;
cfg.vsync_back_porch  = 8;

cfg.pclk_active_neg   = 1;
cfg.pclk_idle_high    = 0;
```

> **WARNING:** Random forum timing values WILL cause white screens or color cycling. Always use the values above for the Waveshare 7". These are confirmed working. See [Troubleshooting](#troubleshooting) for common RGB parallel issues.

**Authoritative References:**
- [Waveshare Official Wiki](https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-7) -- pinouts, schematics, demo code ZIP
- [paulhamsh/Waveshare-ESP32-S3-LCD-7-LVGL](https://github.com/paulhamsh/Waveshare-ESP32-S3-LCD-7-LVGL) -- confirmed working LovyanGFX config
- [SoreGus/esp32s3-whaveshare-lcd](https://github.com/SoreGus/esp32s3-whaveshare-lcd) -- ESP-IDF reference (different API, useful for timing cross-check)

---

## Enabling TFT Gauges

Enable TFT gauge support in your Label Set's CustomPins.h:

```cpp
// In src/LABELS/LABEL_SET_YOURPANEL/CustomPins.h
#define ENABLE_TFT_GAUGES    1
```

This is configured through the **Custom Pins Editor** in the Label Creator tool.

**Requirements:**
- ESP32-S3 or ESP32-S2 recommended (PSRAM required for frame buffers)
- Dual-core boards (S3) perform significantly better than single-core (S2)
- The LovyanGFX library must be installed (handled by the Setup Tool)

---

## How TFT Gauges Work

### FreeRTOS Tasks

Each TFT gauge runs in its own dedicated FreeRTOS task, independent of the main loop. This ensures smooth rendering even when the main loop is busy processing DCS-BIOS data, scanning inputs, or updating other peripherals.

On dual-core ESP32-S3 boards, gauge rendering tasks can run on the second core, leaving the first core entirely for DCS-BIOS communication and input scanning.

For details on the FreeRTOS task architecture, see the Advanced documentation.

### DCS-BIOS Data Flow

1. The gauge subscribes to specific DCS-BIOS export addresses during initialization
2. When new data arrives from the simulator, the firmware updates the subscribed values
3. The gauge's render loop detects value changes and recalculates needle positions, digits, etc.
4. Only the changed portions of the display are redrawn and flushed

### Dirty-Rect Optimization

Rather than redrawing the entire display on every frame, CockpitOS calculates the smallest rectangle that encompasses the changed pixels (the "dirty rect"). Only this region is:

1. Restored from the background image
2. Composited with the new needle/digit position
3. Flushed to the display via DMA

This dramatically reduces the number of pixels transferred per frame, enabling smooth updates even at high frame rates.

### DMA Double-Buffer Flushing

The rendering engine uses two alternating bounce buffers in internal RAM. While one buffer is being transferred to the display via DMA, the CPU prepares the next frame in the other buffer. This pipelining eliminates CPU stalls and maximizes throughput.

---

## Day/NVG Lighting Modes

TFT gauges support two lighting modes that respond to the cockpit's lighting control:

| Mode | Visual Style                      | Trigger                            |
|------|-----------------------------------|------------------------------------|
| Day  | Full-brightness gauge face        | Dimmer value above NVG threshold   |
| NVG  | Green-tinted, reduced brightness  | Dimmer value below NVG threshold   |

The mode switches automatically based on DCS-BIOS dimmer values. Each gauge implementation defines its own Day and NVG color palettes and background images.

---

## BIT (Built-In Test)

At startup, each TFT gauge runs a Built-In Test sequence:

1. Display initializes and shows the gauge face background
2. Needles sweep from minimum to maximum position and back
3. Digital readouts cycle through test values
4. Indicator lights flash briefly
5. Gauge settles to the zero/default position and waits for DCS-BIOS data

This confirms the display is working, the SPI bus is healthy, and the rendering pipeline is functional.

---

## Asset Pipeline

TFT gauge visuals (background images, needle sprites) are stored as C header files containing raw pixel data. The asset pipeline converts source images into optimized arrays:

1. **Source images** -- Created in an image editor (e.g., Photoshop, GIMP) as PNG files
2. **Conversion** -- Python scripts convert PNGs to RGB565 pixel arrays in `.h` files
3. **Storage** -- Assets are stored in PSRAM at runtime; headers are compiled into flash
4. **Rendering** -- The gauge engine composites background + needle sprites per frame

Each gauge defines its own asset files. See [How-To/Wire-TFT-Gauges.md](../How-To/Wire-TFT-Gauges.md) for the asset creation workflow.

---

## Performance Characteristics

### SPI Gauges

| Metric                  | Typical Value         | Notes                          |
|-------------------------|-----------------------|--------------------------------|
| Frame rate              | 30-60 FPS             | Depends on dirty rect size     |
| Pixel throughput        | 2-8 Mpixels/sec       | SPI clock dependent            |
| Render latency          | 5-15ms per frame      | Dirty rect only                |
| Memory (PSRAM)          | 200-500KB per gauge   | Background + sprites           |
| Memory (internal)       | ~16KB per gauge       | Bounce buffers + state         |
| SPI clock               | 40-80 MHz             | Display dependent              |

### RGB Parallel Displays

| Metric                  | Typical Value         | Notes                          |
|-------------------------|-----------------------|--------------------------------|
| Frame rate              | 30+ FPS               | Draw calls appear immediately  |
| Pixel clock             | 16 MHz                | Waveshare 7"                   |
| Framebuffer             | 768KB                 | 800x480x2 bytes (RGB565)      |
| Memory (PSRAM)          | ~1MB total            | Framebuffer + sprites          |
| Render latency          | <5ms per region       | Direct framebuffer write       |

---

## Creating Custom Gauges

The easiest way to create a new TFT gauge is to copy an existing one from `src/Panels/`. See [How-To/Wire-TFT-Gauges.md](../How-To/Wire-TFT-Gauges.md) for the complete step-by-step walkthrough, including:

- Setting up the LGFX device class for your display (SPI or RGB parallel)
- Memory allocation and buffer management
- Subscribing to DCS-BIOS data
- Implementing the rendering pipeline
- Dirty-rect calculation (SPI) or direct framebuffer drawing (RGB parallel)
- DMA double-buffer flushing (SPI) or continuous scan (RGB parallel)
- Day/NVG mode switching
- BIT test sequences
- PanelKind registration via `REGISTER_PANEL`
- Shared utilities in TFT_GaugeUtils.h

**For RGB parallel displays**, use `TFT_Display_IFEI.cpp` as the reference template. It demonstrates CH422G init, LGFX_WS7 device class setup, Bus_RGB/Panel_RGB configuration, and GT911 touch with correct I2C port separation.

For the panel system architecture and `REGISTER_PANEL` details, see [Advanced/Custom-Panels.md](../Advanced/Custom-Panels.md).

---

*See also: [Hardware Overview](README.md) | [Servo Gauges](Servo-Gauges.md) | [Displays](Displays.md)*
