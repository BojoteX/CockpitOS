# How To: Wire TFT Gauges

TFT gauges are graphical LCD displays that render instrument faces with animated needles, digital readouts, and indicator lights. CockpitOS drives these displays using the LovyanGFX library with DMA-accelerated rendering and dirty-rect optimization for smooth, flicker-free updates.

---

## What You Need

| Item | Notes |
|---|---|
| TFT display module | GC9A01 (240x240 round), ST77961 (360x360 round), ST7789 (240x320 rectangular), or ILI9341 (240x320 rectangular) |
| ESP32-S3 board (recommended) | Dual-core with PSRAM required for frame buffers. S2 works but with reduced performance. |
| Hookup wire | SPI bus connections (6-8 wires) |

### Supported Displays

| Display Controller | Resolution | Shape | Bus | Typical Use |
|---|---|---|---|---|
| GC9A01 | 240x240 | Round | SPI | Battery voltage, brake pressure, hydraulic pressure |
| ST77916 | 360x360 | Round | QSPI | Waveshare boards |
| ST77961 | 360x360 | Round | SPI | Cabin pressure, radar altimeter |
| ST7789 | 240x320 | Rectangular | SPI | General purpose |
| ILI9341 | 240x320 | Rectangular | SPI | CYD (Cheap Yellow Display) boards |

---

## Step 1: Wire the SPI Bus

TFT displays connect to the ESP32 over the SPI bus. Each display needs the following connections:

| Signal | Description | Notes |
|---|---|---|
| MOSI | Master Out Slave In | Data from ESP32 to display |
| CLK (SCLK) | SPI Clock | Clock signal |
| CS | Chip Select | Selects this display (active LOW) |
| DC | Data/Command | Distinguishes pixel data from commands |
| RST | Reset | Display reset (active LOW). Can be set to -1 if tied to ESP32 EN. |
| BL | Backlight | PWM brightness control (optional) |
| VCC | Power | 3.3V (check your specific module) |
| GND | Ground | Common ground |

```
                    ESP32                         TFT Display
                 +---------+                     +----------+
                 |         |                     |          |
                 | GPIO 39 +---------------------| MOSI/SDA |
                 | GPIO 40 +---------------------| SCK/SCL  |
                 | GPIO 36 +---------------------| CS       |
                 | GPIO 18 +---------------------| DC       |
                 | RST(-1) |                     | RST      |  (tie to EN or GPIO)
                 |         +---------------------| BL       |  (optional)
                 |         |                     |          |
                 |  3.3V   +---------------------| VCC      |
                 |   GND   +---------------------| GND      |
                 |         |                     |          |
                 +---------+                     +----------+
```

Pin assignments are specific to each gauge and defined in `CustomPins.h`. For example, the cabin pressure gauge uses:

```cpp
// In CustomPins.h
#define CABIN_PRESSURE_MOSI_PIN  PIN(39)
#define CABIN_PRESSURE_SCLK_PIN  PIN(40)
#define CABIN_PRESSURE_CS_PIN    PIN(36)
#define CABIN_PRESSURE_DC_PIN    PIN(18)
#define CABIN_PRESSURE_RST_PIN   -1
```

---

## Step 2: Enable TFT Gauges in CustomPins.h

In your label set's `CustomPins.h`, enable TFT gauge support:

```cpp
#define ENABLE_TFT_GAUGES    1
```

This is configured through the **Custom Pins Editor** in the Label Creator tool.

**Requirements:**
- ESP32-S3 or ESP32-S2 (PSRAM required for frame buffers)
- Dual-core boards (S3) perform significantly better than single-core (S2)
- The LovyanGFX library must be installed (handled automatically by the Setup Tool)

---

## Step 3: Understand the Architecture

Each TFT gauge runs in its own dedicated FreeRTOS task, independent of the main loop. This means gauge rendering does not block input scanning or DCS-BIOS communication.

### Rendering Pipeline

1. **DCS-BIOS data arrives** -- the gauge subscribes to specific addresses via `subscribeToLedChange()`
2. **Value callback fires** -- the callback converts the 16-bit value to a needle angle and sets a `gaugeDirty` flag
3. **Render loop detects change** -- the gauge task checks the dirty flag every ~5ms
4. **Dirty-rect calculation** -- only the region affected by the needle movement is computed
5. **Background restore** -- the background image is blitted into the dirty rect from a PSRAM cache
6. **Needle compositing** -- the rotated needle sprite is drawn on top
7. **DMA flush** -- the dirty rect is transferred to the display using double-buffered DMA

### Day/NVG Lighting Modes

TFT gauges automatically switch between Day and NVG modes based on DCS-BIOS dimmer values. Each gauge stores two sets of assets (background image + needle sprite) and swaps between them when the dimmer crosses a threshold.

| Mode | Visual Style | Trigger |
|---|---|---|
| Day | Full-brightness gauge face | Dimmer value above NVG threshold |
| NVG | Green-tinted, reduced brightness | Dimmer value below NVG threshold |

---

## Step 4: Use an Existing Gauge as a Template

The easiest way to create a new TFT gauge is to copy an existing one. The codebase includes these implementations in `src/Panels/`:

| File | Gauge | Display | Resolution |
|---|---|---|---|
| `TFT_Gauges_Battery.cpp` | Battery Voltage | GC9A01 | 240x240 |
| `TFT_Gauges_BrakePress.cpp` | Brake Pressure | GC9A01 | 240x240 |
| `TFT_Gauges_HydPress.cpp` | Hydraulic Pressure | GC9A01 | 240x240 |
| `TFT_Gauges_CabinPressure.cpp` | Cabin Pressure | ST77961 | 360x360 |
| `TFT_Gauges_RadarAlt.cpp` | Radar Altimeter | ST77961 | 360x360 |
| `TFT_Display_CMWS.cpp` | CMWS Display | -- | Text-based |

Each gauge file follows the same pattern:

1. **LGFX device class** -- configures the SPI bus and panel driver for your display
2. **Asset includes** -- background images and needle sprites as RGB565 arrays
3. **DCS-BIOS callbacks** -- subscribe to the relevant data addresses
4. **Draw function** -- dirty-rect compositing and DMA flush
5. **FreeRTOS task** -- runs the draw function in a loop on a dedicated core
6. **REGISTER_PANEL macro** -- registers the gauge with the panel system

---

## Step 5: Create Gauge Assets

Gauge face images and needle sprites must be converted to RGB565 pixel arrays stored in C header files.

### Asset Pipeline

1. **Create source images** in an image editor (Photoshop, GIMP, etc.)
   - Background: full gauge face at the display's native resolution (e.g., 240x240 for GC9A01)
   - Needle: small sprite with a transparent background (transparent pixels use the key color `0x0120`)
   - Create both Day and NVG variants
2. **Export as PNG** files
3. **Convert to RGB565 headers** using the Python conversion scripts in `src/Panels/Assets/*/Tools/`
4. **Place headers** in an appropriate subdirectory under `src/Panels/Assets/`

Each gauge defines its own asset files. For example, the Battery gauge loads:

```cpp
#include "Assets/BatteryGauge/batBackground.h"      // Day background
#include "Assets/BatteryGauge/batBackgroundNVG.h"    // NVG background
#include "Assets/BatteryGauge/batNeedle.h"           // Day needle sprite
#include "Assets/BatteryGauge/batNeedleNVG.h"        // NVG needle sprite
```

---

## Step 6: BIT (Built-In Test)

At startup, each TFT gauge runs a Built-In Test sequence:

1. Display initializes and shows the background image
2. Needles sweep from minimum to maximum position and back
3. Gauge settles to the default position and waits for DCS-BIOS data

This confirms the display is working, the SPI bus is healthy, and the rendering pipeline is functional.

---

## Step 7: Register the Panel

Every gauge needs a `REGISTER_PANEL` call to integrate with CockpitOS:

```cpp
REGISTER_PANEL(TFTBatt, nullptr, nullptr, BatteryGauge_init, BatteryGauge_loop, nullptr, 100);
```

The arguments are: `(PanelKind, init_fn, loop_fn, disp_init_fn, disp_loop_fn, tick_fn, priority)`.

For TFT gauges, the display init and display loop functions are the primary hooks. The init function is `nullptr` because TFT gauges do their setup in `disp_init`.

The gauge must also be guarded with a compile-time `#if defined(HAS_YOUR_LABEL_SET)` check so it only compiles when the relevant label set is active.

---

## Performance Guidelines

| Metric | Typical Value | Notes |
|---|---|---|
| Frame rate | 30-60 FPS | Depends on dirty rect size |
| Render latency | 5-15ms per frame | Dirty rect only |
| Memory (PSRAM) | 200-500KB per gauge | Background images + sprites |
| Memory (internal) | ~16KB per gauge | DMA bounce buffers + state |
| SPI clock | 40-80 MHz | Display dependent |

**Keep rendering under 16ms per frame** to maintain smooth animation. The dirty-rect optimization helps enormously -- only changed pixels are transferred.

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| White screen, nothing displayed | Wrong SPI pin assignments | Verify MOSI, CLK, CS, DC, RST pins in CustomPins.h match your wiring |
| Screen flickers | DMA configuration issue, or rendering taking too long | Check that DMA bounce buffers allocated successfully; reduce dirty rect size |
| Gauge face displays but needle does not move | DCS-BIOS subscription label is wrong, or no data arriving | Verify the subscribed label matches a real DCS-BIOS export; check DCS connection |
| Colors look wrong | Byte order mismatch | Check `setSwapBytes(true/false)` in your LGFX setup |
| Compilation error about missing LovyanGFX | Library not installed | Run the Setup Tool (`Setup-START.py`) to install all required libraries |
| Out of memory at startup | Not enough PSRAM | TFT gauges require PSRAM. Use an ESP32-S3 board with PSRAM. Reduce the number of simultaneous gauges if needed. |

---

## Further Reading

- [TFT Gauges Reference](../Hardware/TFT-Gauges.md) -- Hardware overview and supported displays
- [Servo Gauges](Wire-Analog-Gauges.md) -- Physical needle alternative using servo motors
- [Custom Panels](../Advanced/Custom-Panels.md) -- Panel system architecture, REGISTER_PANEL, lifecycle, and DCS-BIOS subscriptions

---

*See also: [Hardware Overview](../Hardware/README.md) | [Servo Gauges](../Hardware/Servo-Gauges.md) | [Displays Reference](../Hardware/Displays.md)*
