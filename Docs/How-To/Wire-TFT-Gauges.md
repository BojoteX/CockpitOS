# How To: Wire TFT Gauges

TFT gauges are graphical LCD displays that render instrument faces with animated needles, digital readouts, and indicator lights. CockpitOS drives these displays using the LovyanGFX library with DMA-accelerated rendering and dirty-rect optimization for smooth, flicker-free updates.

---

## What You Need

### For SPI Displays (Individual Gauges)

| Item | Notes |
|---|---|
| TFT display module | GC9A01 (240x240 round), ST77961 (360x360 round), ST7789 (240x320 rectangular), or ILI9341 (240x320 rectangular) |
| ESP32-S3 board (recommended) | Dual-core with PSRAM required for frame buffers. S2 works but with reduced performance. |
| Hookup wire | SPI bus connections (6-8 wires) |

### For RGB Parallel Displays (Full Panel Displays)

| Item | Notes |
|---|---|
| Waveshare ESP32-S3-Touch-LCD-7 | 800x480, ST7262, all pins hard-wired on PCB. No external wiring needed. |

> RGB parallel boards are self-contained -- the ESP32-S3, display, touch controller, and IO expander are all on one PCB. You only need USB-C for power/programming and a network connection for DCS-BIOS UDP.

### Supported Displays

| Display Controller | Resolution | Shape | Bus | Typical Use |
|---|---|---|---|---|
| GC9A01 | 240x240 | Round | SPI | Needle gauges (battery, brake, hydraulic) |
| ST77916 | 360x360 | Round | QSPI | Needle gauges (Waveshare boards) |
| ST77961 | 360x360 | Round | SPI | Needle gauges (cabin pressure, radar altimeter) |
| ST7789 | 240x320 | Rectangular | SPI | Text displays (CMWS) |
| ILI9341 | 240x320 | Rectangular | SPI | Text displays (CYD boards) |
| ST7262 (Waveshare 7") | 800x480 | Rectangular | RGB Parallel | Full instrument panels (IFEI) |

> **Note:** ST7789 and ILI9341 are currently used only for text-based displays (e.g., CMWS), not for animated needle gauges. Use GC9A01, ST77916, or ST77961 for needle gauges. Use ST7262 (RGB parallel) for full instrument panel displays like the IFEI.

---

## Step 1: Connect the Display

CockpitOS supports two display bus types. Choose the section that matches your hardware.

### Option A: SPI Displays (GC9A01, ST77916, ST7789, ILI9341)

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

### Option B: RGB Parallel Displays (Waveshare ESP32-S3-Touch-LCD-7)

No wiring needed -- all 16 data lines, 4 control signals, I2C, and touch are hard-wired on the PCB. However, the LGFX device class setup is significantly different from SPI.

**Required includes (NOT auto-included by LovyanGFX):**

```cpp
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <Wire.h>
```

**LGFX device class pattern:**

```cpp
class LGFX_WS7 : public lgfx::LGFX_Device {
    lgfx::Bus_RGB     _bus_instance;    // MUST be _bus_instance (not _bus)
    lgfx::Panel_RGB   _panel_instance;  // MUST be _panel_instance (not _panel)
    lgfx::Touch_GT911 _touch_instance;  // MUST be _touch_instance (not _touch)

public:
    LGFX_WS7() {
        // 1. Panel FIRST (bus references panel)
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = 800;
            cfg.memory_height = 480;
            cfg.panel_width   = 800;
            cfg.panel_height  = 480;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            _panel_instance.config(cfg);
        }

        // 2. Bus SECOND (references panel)
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;

            // Blue (pin_d0..d4)
            cfg.pin_d0  = 14;  cfg.pin_d1  = 38;  cfg.pin_d2  = 18;
            cfg.pin_d3  = 17;  cfg.pin_d4  = 10;
            // Green (pin_d5..d10)
            cfg.pin_d5  = 39;  cfg.pin_d6  = 0;   cfg.pin_d7  = 45;
            cfg.pin_d8  = 48;  cfg.pin_d9  = 47;  cfg.pin_d10 = 21;
            // Red (pin_d11..d15)
            cfg.pin_d11 = 1;   cfg.pin_d12 = 2;   cfg.pin_d13 = 42;
            cfg.pin_d14 = 41;  cfg.pin_d15 = 40;

            // Control signals
            cfg.pin_henable = 5;    // DE
            cfg.pin_vsync   = 3;
            cfg.pin_hsync   = 46;
            cfg.pin_pclk    = 7;

            // Timing (Waveshare 7" confirmed working values)
            cfg.freq_write        = 16000000;  // 16 MHz
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

            _bus_instance.config(cfg);
        }
        _panel_instance.setBus(&_bus_instance);

        // 3. Touch (GT911 on I2C port 1)
        {
            auto tcfg = _touch_instance.config();
            tcfg.i2c_port = 1;    // MUST be 1 (CH422G uses port 0)
            tcfg.i2c_addr = 0x14;
            tcfg.pin_sda  = 8;
            tcfg.pin_scl  = 9;
            tcfg.pin_int  = 4;
            tcfg.pin_rst  = -1;   // RST via CH422G
            tcfg.freq     = 400000;
            tcfg.x_min    = 0;  tcfg.x_max = 799;
            tcfg.y_min    = 0;  tcfg.y_max = 479;
            _touch_instance.config(tcfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }
};
```

**Critical rules for RGB parallel LGFX classes:**

1. **Member names:** Use `_bus_instance`, `_panel_instance`, `_touch_instance`. Names like `_bus`, `_panel`, `_touch` collide with LGFX_Device base class members and cause compile errors.
2. **Config order:** Panel config MUST come before bus config. The bus config references the panel: `cfg.panel = &_panel_instance`.
3. **Pin order:** Bus_RGB maps `pin_d0..d4` = Blue, `pin_d5..d10` = Green, `pin_d11..d15` = Red. This is NOT the same as the physical RGB order.
4. **I2C port separation:** CH422G uses `Wire` (I2C_NUM_0). Touch GT911 MUST use `i2c_port = 1` (I2C_NUM_1).
5. **CH422G before tft.init():** Initialize the CH422G IO expander to release reset lines and enable backlight BEFORE calling `tft.init()`.

**CH422G initialization (call before tft.init()):**

```cpp
Wire.begin(8, 9);  // SDA=GPIO8, SCL=GPIO9

Wire.beginTransmission(0x24);  // Mode register
Wire.write(0x01);              // Push-pull output
Wire.endTransmission();

Wire.beginTransmission(0x38);  // Output register
Wire.write(0x07);              // EXIO1+EXIO2+EXIO3 HIGH (resets inactive, backlight ON)
Wire.endTransmission();
```

**Init sequence:**

```cpp
ch422g_init();                  // 1. IO expander first
tft.init();                     // 2. LGFX init (configures LCD peripheral)
tft.fillScreen(TFT_BLACK);     // 3. Clear framebuffer
```

> **Full working example:** See `src/Panels/TFT_Display_IFEI.cpp` (LGFX_WS7 class, ch422g_init function, TFTIFEIDisplay_init).

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

| File | Gauge | Display | Resolution | Bus |
|---|---|---|---|---|
| `TFT_Gauges_Battery.cpp` | Battery Voltage | GC9A01 | 240x240 | SPI |
| `TFT_Gauges_BrakePress.cpp` | Brake Pressure | GC9A01 | 240x240 | SPI |
| `TFT_Gauges_HydPress.cpp` | Hydraulic Pressure | GC9A01 | 240x240 | SPI |
| `TFT_Gauges_CabinPressure.cpp` | Cabin Pressure | ST77961 | 360x360 | SPI |
| `TFT_Gauges_RadarAlt.cpp` | Radar Altimeter | ST77961 | 360x360 | SPI |
| `TFT_Display_CMWS.cpp` | CMWS Display | -- | Text-based | SPI |
| `TFT_Display_IFEI.cpp` | IFEI Display | ST7262 | 800x480 | RGB Parallel |

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

### SPI Display Issues

| Symptom | Cause | Fix |
|---|---|---|
| White screen, nothing displayed | Wrong SPI pin assignments | Verify MOSI, CLK, CS, DC, RST pins in CustomPins.h match your wiring |
| Screen flickers | DMA configuration issue, or rendering taking too long | Check that DMA bounce buffers allocated successfully; reduce dirty rect size |
| Gauge face displays but needle does not move | DCS-BIOS subscription label is wrong, or no data arriving | Verify the subscribed label matches a real DCS-BIOS export; check DCS connection |
| Colors look wrong | Byte order mismatch | Check `setSwapBytes(true/false)` in your LGFX setup |
| Compilation error about missing LovyanGFX | Library not installed | Run the Setup Tool (`Setup-START.py`) to install all required libraries |
| Out of memory at startup | Not enough PSRAM | TFT gauges require PSRAM. Use an ESP32-S3 board with PSRAM. Reduce the number of simultaneous gauges if needed. |

### RGB Parallel Display Issues

| Symptom | Cause | Fix |
|---|---|---|
| `Bus_RGB` / `Panel_RGB` not found (compile error) | Missing explicit includes | Add `#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>` and `Bus_RGB.hpp`. These are NOT auto-included by `<LovyanGFX.hpp>`. |
| `_panel` member collision (compile error) | Member name conflicts with LGFX_Device base class | Rename members to `_bus_instance`, `_panel_instance`, `_touch_instance`. |
| White screen (display powers on but no content) | Wrong timing parameters or freq_write | Use the exact Waveshare 7" values: `freq_write=16000000`, hsync/vsync `8/4/8`. See Step 1 Option B. |
| Cycling solid colors (red/green/blue/white) | Severely wrong timing (front/back porch, pulse width) | Do NOT use random forum values. Use the confirmed values from paulhamsh's repo. |
| Display subscription overflow in log | Too many `subscribeToDisplayChange()` calls | Increase `MAX_DISPLAY_SUBSCRIPTIONS` in DCSBIOSBridge.h (default 32, IFEI needs 48). |
| Touch not responding | GT911 on wrong I2C port | Set `tcfg.i2c_port = 1` (CH422G occupies port 0). |
| Board target set to ESP32-S2 | Bus_RGB/Panel_RGB are ESP32-S3 only | In Arduino IDE / PlatformIO, select the correct ESP32-S3 board target. |
| CH422G not initializing | Wire.begin() not called before CH422G writes | Call `Wire.begin(8, 9)` first, then CH422G mode + output writes, then `tft.init()`. |

> **Golden rule for RGB parallel:** Always use manufacturer documentation and known-working configs. The Waveshare 7" timing values from [paulhamsh/Waveshare-ESP32-S3-LCD-7-LVGL](https://github.com/paulhamsh/Waveshare-ESP32-S3-LCD-7-LVGL) are confirmed working. Random forum timing values will produce white screens or color cycling.

---

## Further Reading

- [TFT Gauges Reference](../Hardware/TFT-Gauges.md) -- Hardware overview and supported displays
- [Servo Gauges](Wire-Analog-Gauges.md) -- Physical needle alternative using servo motors
- [Custom Panels](../Advanced/Custom-Panels.md) -- Panel system architecture, REGISTER_PANEL, lifecycle, and DCS-BIOS subscriptions

---

*See also: [Hardware Overview](../Hardware/README.md) | [Servo Gauges](../Hardware/Servo-Gauges.md) | [Displays Reference](../Hardware/Displays.md)*
