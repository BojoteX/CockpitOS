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
|  DCS-BIOS data stream  -->  ESP32  -->  SPI bus  -->  TFT Display   |
|                                |                                     |
|                                |                                     |
|                      +-------------------+                           |
|                      | Rendering Engine  |                           |
|                      |                   |                           |
|                      | - Background image|                           |
|                      | - Needle rotation |                           |
|                      | - Digit overlays  |                           |
|                      | - Indicator lights|                           |
|                      | - Dirty-rect DMA  |                           |
|                      +-------------------+                           |
|                                                                      |
+----------------------------------------------------------------------+
```

### Existing Gauge Implementations

| Gauge               | Display    | Resolution | Shape |
|---------------------|------------|------------|-------|
| Battery Voltage     | GC9A01     | 240x240    | Round |
| Brake Pressure      | GC9A01     | 240x240    | Round |
| Hydraulic Pressure  | GC9A01     | 240x240    | Round |
| Cabin Pressure      | ST77961    | 360x360    | Round |
| Radar Altimeter     | ST77961    | 360x360    | Round |

---

## Supported Displays

CockpitOS TFT gauges support multiple display controllers through the LovyanGFX library:

| Display Controller | Resolution | Shape      | Bus  | Notes                           |
|--------------------|------------|------------|------|---------------------------------|
| GC9A01             | 240x240    | Round      | SPI  | Common, affordable round displays |
| ST77916            | 360x360    | Round      | QSPI | Waveshare boards                |
| ST77961            | 360x360    | Round      | SPI  | IdeasPark 1.14" and similar     |
| ST7789             | 240x320    | Rectangular| SPI  | Very common rectangular displays |
| ILI9341            | 240x320    | Rectangular| SPI  | CYD (Cheap Yellow Display) boards |

The LovyanGFX library is installed automatically by the Setup Tool.

---

## Wiring

TFT displays connect to the ESP32 over the SPI bus. The exact pin assignments depend on your board and display module.

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

| Metric                  | Typical Value         | Notes                          |
|-------------------------|-----------------------|--------------------------------|
| Frame rate              | 30-60 FPS             | Depends on dirty rect size     |
| Pixel throughput        | 2-8 Mpixels/sec       | SPI clock dependent            |
| Render latency          | 5-15ms per frame      | Dirty rect only                |
| Memory (PSRAM)          | 200-500KB per gauge   | Background + sprites           |
| Memory (internal)       | ~16KB per gauge       | Bounce buffers + state         |
| SPI clock               | 40-80 MHz             | Display dependent              |

---

## Creating Custom Gauges

The easiest way to create a new TFT gauge is to copy an existing one from `src/Panels/`. See [How-To/Wire-TFT-Gauges.md](../How-To/Wire-TFT-Gauges.md) for the complete step-by-step walkthrough, including:

- Setting up the LGFX device class for your display
- Memory allocation and buffer management
- Subscribing to DCS-BIOS data
- Implementing the rendering pipeline
- Dirty-rect calculation
- DMA double-buffer flushing
- Day/NVG mode switching
- BIT test sequences
- PanelKind registration via `REGISTER_PANEL`
- Shared utilities in TFT_GaugeUtils.h

For the panel system architecture and `REGISTER_PANEL` details, see [Advanced/Custom-Panels.md](../Advanced/Custom-Panels.md).

---

*See also: [Hardware Overview](README.md) | [Servo Gauges](Servo-Gauges.md) | [Displays](Displays.md)*
