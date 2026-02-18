# CockpitOS TFT Gauge Developer Guide

> **Version**: 1.0
> **Last Updated**: February 2026
> **Audience**: Rookies to Advanced Developers
> **Library**: LovyanGFX (https://github.com/lovyan03/LovyanGFX)

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Prerequisites](#2-prerequisites)
3. [Architecture Overview](#3-architecture-overview)
4. [Supported Displays & Buses](#4-supported-displays--buses)
5. [Step-by-Step: Creating a New Gauge](#5-step-by-step-creating-a-new-gauge)
6. [The LGFX Device Class](#6-the-lgfx-device-class)
7. [Memory Management](#7-memory-management)
8. [DCS-BIOS Integration](#8-dcs-bios-integration)
9. [The Rendering Pipeline](#9-the-rendering-pipeline)
10. [Dirty-Rect Optimization](#10-dirty-rect-optimization)
11. [DMA Double-Buffer Flush](#11-dma-double-buffer-flush)
12. [Day/NVG Lighting Modes](#12-daynvg-lighting-modes)
13. [BIT (Built-In Test)](#13-bit-built-in-test)
14. [FreeRTOS Task Mode](#14-freertos-task-mode)
15. [Asset Pipeline](#15-asset-pipeline)
16. [Pin Configuration & Label Sets](#16-pin-configuration--label-sets)
17. [PanelKind Registration](#17-panelkind-registration)
18. [Shared Utilities (TFT_GaugeUtils.h)](#18-shared-utilities-tft_gaugeutils-h)
19. [Reference: Existing Gauges](#19-reference-existing-gauges)
20. [Troubleshooting](#20-troubleshooting)
21. [AI-Assisted Development](#21-ai-assisted-development)

---

## 1. Introduction

CockpitOS drives real cockpit instrument gauges using ESP32 microcontrollers and small TFT displays. Each gauge is a self-contained `.cpp` file that:

- Configures a display via LovyanGFX
- Subscribes to DCS-BIOS data (aircraft simulation values)
- Renders a needle (or multiple needles/indicators) on a background image
- Uses dirty-rect DMA to only update the pixels that changed

This guide covers everything you need to create a new TFT gauge from scratch or adapt an existing one to a new display.

---

## 2. Prerequisites

### Hardware
- **ESP32-S3** or **ESP32-S2** (TFT gauges require PSRAM)
- A round or square TFT display (GC9A01, ST7789, ST77916, ILI9341, etc.)
- SPI or QSPI wiring between ESP32 and display

### Software
- Arduino IDE or PlatformIO
- **LovyanGFX** library installed (`https://github.com/lovyan03/LovyanGFX`)
- CockpitOS source code
- Python 3 (for `generate_panelkind.py`)

### Knowledge
- Basic C/C++ (variables, functions, structs)
- Basic understanding of SPI communication (MOSI, SCLK, CS, DC)
- Familiarity with Arduino `setup()` / `loop()` pattern

---

## 3. Architecture Overview

```
CockpitOS Boot
    |
    v
PanelRegistry_forEachInit()  -->  YourGauge_init()
    |                                   |
    |                              - Allocate memory
    |                              - Configure display
    |                              - Subscribe to DCS-BIOS
    |                              - First paint
    |                              - (Optional) Start FreeRTOS task
    v
PanelRegistry_forEachDisplayLoop()  -->  YourGauge_loop()
    |                                         |
    |                                    - Check if data changed
    |                                    - Calculate dirty rect
    |                                    - Restore background
    |                                    - Compose needle(s)
    |                                    - DMA flush to display
    v
 (repeats every frame)
```

### Key Concepts

| Concept | What It Means |
|---------|---------------|
| **PanelRegistry** | Auto-registration system. Your gauge registers itself at compile time. |
| **PanelKind** | Unique enum identity for each gauge (auto-generated). |
| **DCS-BIOS** | Protocol that streams aircraft instrument values from the simulator. |
| **Dirty Rect** | Only repaint the rectangle where something actually changed. |
| **DMA Flush** | Send pixels to the display using DMA (CPU does other work while data transfers). |
| **Bounce Buffers** | Two small buffers in internal RAM that alternate for DMA transfers. |
| **PSRAM** | External pseudo-static RAM (2-8 MB). Stores large frame buffers and backgrounds. |

---

## 4. Supported Displays & Buses

CockpitOS gauges currently use these display/bus combinations:

| Display | Resolution | Shape | Bus | Panel Class | Used By |
|---------|-----------|-------|-----|-------------|---------|
| GC9A01 | 240x240 | Round | SPI | `Panel_GC9A01` | Battery, Brake Press, Hydraulic |
| ST77961 | 360x360 | Round | SPI | `Panel_ST77961` | Cabin Pressure, Radar Altimeter |
| ST77916 | 360x360 | Round | QSPI | `Panel_ST77916` | Cabin Pressure (Waveshare board) |
| ST7789 | 240x320 | Rect | SPI | `Panel_ST7789` | CMWS Display |
| ILI9341 | 240x320 | Rect | SPI | `Panel_ILI9341` | CMWS Display (CYD variant) |

### Bus Types

**SPI (Serial Peripheral Interface)** — Most common. 4 wires:
- `MOSI` — Data from ESP32 to display
- `SCLK` — Clock
- `CS` — Chip Select (which display to talk to)
- `DC` — Data/Command toggle

**QSPI (Quad SPI)** — 4 data lines instead of 1. Faster throughput:
- `IO0-IO3` — Four data lines
- `SCLK` — Clock
- `CS` — Chip Select
- No DC pin (commands are framed in the protocol)

**8-bit Parallel** — 8 data lines. Used by LilyGo T-Display S3:
- `D0-D7` — Eight data lines
- `WR` — Write strobe
- `DC` — Data/Command

---

## 5. Step-by-Step: Creating a New Gauge

This is the complete checklist. Each step is detailed in later sections.

### Quick Checklist

```
[ ] 1. Create file:  src/Panels/TFT_Gauges_YourGauge.cpp
[ ] 2. Add metadata:  // PANEL_KIND: TFTYourGauge  (first 30 lines)
[ ] 3. Run:           python generate_panelkind.py
[ ] 4. Create assets: src/Panels/Assets/YourGauge/  (backgrounds, needles)
[ ] 5. Write LGFX device class (bus + panel config)
[ ] 6. Define pins with defaults
[ ] 7. Include TFT_GaugeUtils.h (after SCREEN_W/SCREEN_H)
[ ] 8. Allocate DMA bounce buffers + PSRAM caches
[ ] 9. Subscribe to DCS-BIOS labels
[ ] 10. Implement draw function (dirty-rect + DMA flush)
[ ] 11. Register: REGISTER_PANEL(...)
[ ] 12. Add pin definitions to your label set's CustomPins.h
[ ] 13. Compile and test!
```

### Minimal Skeleton

Here is the minimum structure every gauge file follows. Copy this and fill in your specifics:

```cpp
// TFT_Gauges_YourGauge.cpp — CockpitOS Your Gauge (LovyanGFX, PANEL @ RESxRES)
// Dirty-rect compose + region DMA flush (PSRAM sprites, DMA-safe)

// PANEL_KIND: TFTYourGauge

#include "../Globals.h"

#if defined(HAS_YOUR_LABEL_SET) && (defined(ESP_FAMILY_S3) || defined(ESP_FAMILY_S2)) \
    && (defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1))

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

#if !__has_include(<LovyanGFX.hpp>)
#error "Missing LovyanGFX — install from https://github.com/lovyan03/LovyanGFX"
#endif

#include <LovyanGFX.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// ═══════════════════════════════════════════════════════════════
// REGISTRATION
// ═══════════════════════════════════════════════════════════════
#if defined(HAS_YOUR_LABEL_SET)
    REGISTER_PANEL(TFTYourGauge, nullptr, nullptr,
                   YourGauge_init, YourGauge_loop, nullptr, 100);
#endif

// ═══════════════════════════════════════════════════════════════
// CONFIGURATION
// ═══════════════════════════════════════════════════════════════
#define MAX_MEMORY_TFT               4    // Stripe height (4 for 240px, 8 for 360px)
#define GAUGE_DRAW_MIN_INTERVAL_MS  13    // ~76 FPS max
#define RUN_GAUGE_AS_TASK            1    // 1 = dedicated FreeRTOS task
#define BACKLIGHT_LABEL   "CONSOLES_DIMMER"
#define COLOR_DEPTH        16
#define CPU_CORE           0

// ═══════════════════════════════════════════════════════════════
// PINS (defaults to -1, override in CustomPins.h)
// ═══════════════════════════════════════════════════════════════
#ifndef YOURGAUGE_MOSI_PIN
  #define YOURGAUGE_MOSI_PIN  -1
#endif
#ifndef YOURGAUGE_SCLK_PIN
  #define YOURGAUGE_SCLK_PIN  -1
#endif
#ifndef YOURGAUGE_DC_PIN
  #define YOURGAUGE_DC_PIN    -1
#endif
#ifndef YOURGAUGE_CS_PIN
  #define YOURGAUGE_CS_PIN    -1
#endif
#ifndef YOURGAUGE_RST_PIN
  #define YOURGAUGE_RST_PIN   -1
#endif

// ═══════════════════════════════════════════════════════════════
// ASSETS
// ═══════════════════════════════════════════════════════════════
#include "Assets/YourGauge/yourBackground.h"
#include "Assets/YourGauge/yourBackgroundNVG.h"
#include "Assets/YourGauge/yourNeedle.h"
#include "Assets/YourGauge/yourNeedleNVG.h"

// ═══════════════════════════════════════════════════════════════
// DIMENSIONS & SHARED UTILS
// ═══════════════════════════════════════════════════════════════
static constexpr int16_t SCREEN_W = 240, SCREEN_H = 240;
#include "includes/TFT_GaugeUtils.h"
// Provides: Rect, rectEmpty, rectClamp, rectUnion, rectPad,
//           rotatedAABB, TRANSPARENT_KEY, NVG_THRESHOLD

static constexpr int16_t CENTER_X = SCREEN_W / 2;
static constexpr int16_t CENTER_Y = SCREEN_H / 2;
static constexpr int16_t NEEDLE_W = 15,  NEEDLE_H = 88;
static constexpr int16_t NEEDLE_PIVOT_X = 7, NEEDLE_PIVOT_Y = 84;

// Angle range (degrees)
static constexpr int16_t ANG_MIN = -150, ANG_MAX = 150;

// DMA stripe sizing
static constexpr int    STRIPE_H = MAX_MEMORY_TFT;
static constexpr size_t STRIPE_BYTES = size_t(SCREEN_W) * STRIPE_H * sizeof(uint16_t);
static_assert(SCREEN_W > 0 && SCREEN_H > 0, "bad dims");
static_assert(STRIPE_H > 0 && STRIPE_H <= SCREEN_H, "bad STRIPE_H");

// ═══════════════════════════════════════════════════════════════
// LGFX DEVICE CLASS (see Section 6 for details)
// ═══════════════════════════════════════════════════════════════
class LGFX_YourGauge : public lgfx::LGFX_Device {
    lgfx::Bus_SPI      _bus;
    lgfx::Panel_GC9A01 _panel;   // <-- Change to your panel type
public:
    LGFX_YourGauge() {
        {
            auto cfg = _bus.config();
            cfg.spi_host    = SPI3_HOST;
            cfg.spi_mode    = 0;
            cfg.freq_write  = 80000000;
            cfg.freq_read   = 0;
            cfg.spi_3wire   = false;
            cfg.use_lock    = false;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_mosi    = YOURGAUGE_MOSI_PIN;
            cfg.pin_miso    = -1;
            cfg.pin_sclk    = YOURGAUGE_SCLK_PIN;
            cfg.pin_dc      = YOURGAUGE_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.readable      = false;
            pcfg.pin_cs        = YOURGAUGE_CS_PIN;
            pcfg.pin_rst       = YOURGAUGE_RST_PIN;
            pcfg.pin_busy      = -1;
            pcfg.memory_width  = SCREEN_W;
            pcfg.memory_height = SCREEN_H;
            pcfg.panel_width   = SCREEN_W;
            pcfg.panel_height  = SCREEN_H;
            pcfg.offset_x      = 0;
            pcfg.offset_y      = 0;
            pcfg.offset_rotation = 0;
            pcfg.bus_shared    = false;
            pcfg.invert        = true;  // Most displays need this
            _panel.config(pcfg);
        }
        setPanel(&_panel);
    }
};

// ═══════════════════════════════════════════════════════════════
// STATE
// ═══════════════════════════════════════════════════════════════
static LGFX_YourGauge tft;
static lgfx::LGFX_Sprite frameSpr(&tft);
static lgfx::LGFX_Sprite needleSpr(&tft);

static uint16_t* bgCache[2]   = { nullptr, nullptr };  // 0=Day, 1=NVG
static uint16_t* dmaBounce[2] = { nullptr, nullptr };

static volatile int16_t angle         = ANG_MIN;
static volatile int16_t lastDrawnAngle = INT16_MIN;
static volatile bool    gaugeDirty     = false;
static volatile uint8_t currentLightingMode = 0;  // 0=Day, 2=NVG
static uint8_t          lastNeedleMode = 0xFF;
static volatile bool    needsFullFlush = true;
static unsigned long    lastDrawTime   = 0;
static TaskHandle_t     tftTaskHandle  = nullptr;

// DMA fence
static bool dmaBusy = false;
static inline void waitDMADone() {
    if (dmaBusy) { tft.waitDMA(); dmaBusy = false; }
}

// ═══════════════════════════════════════════════════════════════
// HELPER: Blit background sub-rect into frame sprite
// ═══════════════════════════════════════════════════════════════
static inline void blitBGRectToFrame(const uint16_t* bg,
                                     int x, int y, int w, int h)
{
    if (w <= 0 || h <= 0) return;
    uint16_t* dst = (uint16_t*)frameSpr.getBuffer();
    for (int row = 0; row < h; ++row)
        std::memcpy(&dst[(y + row) * SCREEN_W + x],
                    &bg [(y + row) * SCREEN_W + x],
                    size_t(w) * sizeof(uint16_t));
}

// ═══════════════════════════════════════════════════════════════
// DMA FLUSH (see Section 11 for explanation)
// ═══════════════════════════════════════════════════════════════
static inline void flushRectToDisplay(const uint16_t* src,
                                      const Rect& rr, bool blocking)
{
    Rect r = rectClamp(rr);
    if (rectEmpty(r)) return;
    waitDMADone();

    const int pitch = SCREEN_W;
    const int lines_per = STRIPE_H;
    int y = r.y;

    tft.startWrite();

    // Prime first stripe
    int h0 = std::min((int)lines_per, (int)r.h);
    for (int row = 0; row < h0; ++row)
        std::memcpy(dmaBounce[0] + row * r.w,
                    src + (y + row) * pitch + r.x,
                    size_t(r.w) * sizeof(uint16_t));
    tft.setAddrWindow(r.x, y, r.w, h0);
    tft.pushPixelsDMA(dmaBounce[0], uint32_t(r.w) * h0);
    if (!blocking) dmaBusy = true;

    // Overlap: pack next stripe while previous DMA runs
    int bb = 1;
    for (y += h0; y < r.y + r.h; y += lines_per, bb ^= 1) {
        int h = std::min(lines_per, r.y + r.h - y);
        for (int row = 0; row < h; ++row)
            std::memcpy(dmaBounce[bb] + row * r.w,
                        src + (y + row) * pitch + r.x,
                        size_t(r.w) * sizeof(uint16_t));
        tft.waitDMA();
        tft.setAddrWindow(r.x, y, r.w, h);
        tft.pushPixelsDMA(dmaBounce[bb], uint32_t(r.w) * h);
        if (blocking) tft.waitDMA();
    }

    if (blocking) { tft.waitDMA(); dmaBusy = false; }
    tft.endWrite();
}

// ═══════════════════════════════════════════════════════════════
// SPRITE BUILDERS
// ═══════════════════════════════════════════════════════════════
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);   // Fill with magic transparent color
    spr.setSwapBytes(true);            // Assets are big-endian RGB565
    spr.pushImage(0, 0, NEEDLE_W, NEEDLE_H, img);
}

// ═══════════════════════════════════════════════════════════════
// DCS-BIOS CALLBACKS
// ═══════════════════════════════════════════════════════════════
static void onValueChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, ANG_MIN, ANG_MAX);
    if (a != angle) { angle = a; gaugeDirty = true; }
}
static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode != currentLightingMode) {
        currentLightingMode = mode;
        needsFullFlush = true;
        gaugeDirty = true;
    }
}

// ═══════════════════════════════════════════════════════════════
// DRAW (called every frame)
// ═══════════════════════════════════════════════════════════════
static void YourGauge_draw(bool force = false, bool blocking = false)
{
    if (!force && !isMissionRunning()) return;

    int16_t a = angle;
    if (a < ANG_MIN) a = ANG_MIN;
    if (a > ANG_MAX) a = ANG_MAX;

    bool stateChanged = gaugeDirty
        || (a != lastDrawnAngle)
        || needsFullFlush;
    if (!stateChanged) return;

    const unsigned long now = millis();
    if (!force && !needsFullFlush
        && (now - lastDrawTime < GAUGE_DRAW_MIN_INTERVAL_MS))
        return;
    lastDrawTime = now;
    gaugeDirty = false;

    if (needsFullFlush) waitDMADone();

    // Select day/NVG assets
    const uint16_t* bg  = (currentLightingMode == 0)
                          ? bgCache[0] : bgCache[1];
    const uint16_t* img = (currentLightingMode == 0)
                          ? yourNeedle : yourNeedleNVG;

    // Rebuild needle sprite on lighting mode change
    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleSpr, img);
        lastNeedleMode = currentLightingMode;
    }

    // Calculate dirty rect
    Rect dirty = needsFullFlush
        ? Rect{0, 0, SCREEN_W, SCREEN_H}
        : Rect{};
    if (!needsFullFlush) {
        if (lastDrawnAngle == INT16_MIN) {
            dirty = {0, 0, SCREEN_W, SCREEN_H};
        } else {
            Rect oldR = rotatedAABB(CENTER_X, CENTER_Y,
                NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y,
                (float)lastDrawnAngle);
            Rect newR = rotatedAABB(CENTER_X, CENTER_Y,
                NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y,
                (float)a);
            dirty = rectUnion(oldR, newR);
        }
    }

    // 1. Restore background in dirty area
    blitBGRectToFrame(bg, dirty.x, dirty.y, dirty.w, dirty.h);

    // 2. Compose needle with clipping
    frameSpr.setClipRect(dirty.x, dirty.y, dirty.w, dirty.h);
    needleSpr.pushRotateZoom(&frameSpr,
        CENTER_X, CENTER_Y, (float)a,
        1.0f, 1.0f, TRANSPARENT_KEY);
    frameSpr.clearClipRect();

    // 3. Flush dirty region to display via DMA
    const uint16_t* buf = (const uint16_t*)frameSpr.getBuffer();
    flushRectToDisplay(buf, dirty, needsFullFlush || blocking);

    lastDrawnAngle = a;
    needsFullFlush = false;
}

// ═══════════════════════════════════════════════════════════════
// FREERTOS TASK (optional)
// ═══════════════════════════════════════════════════════════════
static void YourGauge_task(void*) {
    for (;;) {
        YourGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ═══════════════════════════════════════════════════════════════
// INIT
// ═══════════════════════════════════════════════════════════════
void YourGauge_init()
{
    // 1. Allocate DMA bounce buffers (internal RAM)
    dmaBounce[0] = (uint16_t*)heap_caps_aligned_alloc(
        32, STRIPE_BYTES,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    dmaBounce[1] = (uint16_t*)heap_caps_aligned_alloc(
        32, STRIPE_BYTES,
        MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!dmaBounce[0] || !dmaBounce[1]) {
        debugPrintf("dmaBounce alloc failed (%u bytes)\n",
                    (unsigned)STRIPE_BYTES);
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // 2. Allocate background caches (PSRAM)
    static constexpr size_t FRAME_BYTES =
        size_t(SCREEN_W) * SCREEN_H * sizeof(uint16_t);
    bgCache[0] = (uint16_t*)heap_caps_aligned_alloc(
        16, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    bgCache[1] = (uint16_t*)heap_caps_aligned_alloc(
        16, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bgCache[0] || !bgCache[1]) {
        debugPrintln("bgCache alloc failed");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    std::memcpy(bgCache[0], yourBackground, FRAME_BYTES);
    std::memcpy(bgCache[1], yourBackgroundNVG, FRAME_BYTES);

    // 3. Initialize display
    tft.init();
    tft.setColorDepth(COLOR_DEPTH);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // 4. Create frame sprite (PSRAM)
    frameSpr.setColorDepth(COLOR_DEPTH);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    // 5. Create needle sprite (internal RAM)
    needleSpr.setColorDepth(COLOR_DEPTH);
    needleSpr.createSprite(NEEDLE_W, NEEDLE_H);
    needleSpr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleSpr, yourNeedle);

    // 6. Subscribe to DCS-BIOS
    subscribeToLedChange("YOUR_DCS_LABEL", onValueChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    // 7. First paint (blocking)
    needsFullFlush = true;
    gaugeDirty = true;
    YourGauge_draw(true, true);

    // 8. Optional: BIT self-test
    // YourGauge_bitTest();

    // 9. Optional: Start dedicated task
#if RUN_GAUGE_AS_TASK
    xTaskCreatePinnedToCore(YourGauge_task, "YourGaugeTask",
        4096, nullptr, 2, &tftTaskHandle, CPU_CORE);
#endif

    debugPrintln("Your Gauge initialized");
}

// ═══════════════════════════════════════════════════════════════
// LOOP
// ═══════════════════════════════════════════════════════════════
void YourGauge_loop() {
#if !RUN_GAUGE_AS_TASK
    YourGauge_draw(false, false);
#endif
}

// ═══════════════════════════════════════════════════════════════
// CLEANUP (called on shutdown)
// ═══════════════════════════════════════════════════════════════
void YourGauge_deinit() {
    waitDMADone();
    if (tftTaskHandle) {
        vTaskDelete(tftTaskHandle);
        tftTaskHandle = nullptr;
    }
    needleSpr.deleteSprite();
    frameSpr.deleteSprite();
    for (int i = 0; i < 2; ++i) {
        if (dmaBounce[i]) { heap_caps_free(dmaBounce[i]); dmaBounce[i] = nullptr; }
        if (bgCache[i])   { heap_caps_free(bgCache[i]);   bgCache[i]   = nullptr; }
    }
}

#elif defined(HAS_YOUR_LABEL_SET) && defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1)
#warning "Your Gauge requires ESP32-S2 or ESP32-S3"
#endif
```

---

## 6. The LGFX Device Class

Every gauge defines a custom `LGFX_Device` subclass that configures the bus and panel. This is where you tell LovyanGFX what display you have and how it is wired.

### SPI Bus Configuration

```cpp
auto cfg = _bus.config();
cfg.spi_host    = SPI3_HOST;       // SPI2_HOST or SPI3_HOST
cfg.spi_mode    = 0;               // Usually 0
cfg.freq_write  = 80000000;        // 80 MHz (max for most SPI displays)
cfg.freq_read   = 0;               // We never read back
cfg.spi_3wire   = false;           // true only for 3-wire SPI (rare)
cfg.use_lock    = false;           // false = no mutex (single-writer)
cfg.dma_channel = SPI_DMA_CH_AUTO; // Let ESP-IDF pick
cfg.pin_mosi    = YOUR_MOSI_PIN;
cfg.pin_miso    = -1;              // Not used (set to actual pin if needed)
cfg.pin_sclk    = YOUR_SCLK_PIN;
cfg.pin_dc      = YOUR_DC_PIN;
```

### QSPI Bus Configuration

For QSPI displays, set four IO pins instead of MOSI/DC:

```cpp
auto cfg = _bus.config();
cfg.spi_host    = SPI2_HOST;
cfg.freq_write  = 32000000;   // 32 MHz x4 = 128 Mbit/s
cfg.pin_dc      = -1;         // No DC pin in QSPI
cfg.pin_mosi    = -1;
cfg.pin_miso    = -1;
cfg.pin_io0     = IO0_PIN;    // DATA0
cfg.pin_io1     = IO1_PIN;    // DATA1
cfg.pin_io2     = IO2_PIN;    // DATA2
cfg.pin_io3     = IO3_PIN;    // DATA3
cfg.pin_sclk    = SCLK_PIN;
```

### Panel Configuration

```cpp
auto pcfg = _panel.config();
pcfg.readable      = false;      // We never read from the display
pcfg.pin_cs        = CS_PIN;
pcfg.pin_rst       = RST_PIN;    // -1 if no reset pin
pcfg.pin_busy      = -1;         // -1 if no busy pin
pcfg.memory_width  = SCREEN_W;   // Display memory width
pcfg.memory_height = SCREEN_H;   // Display memory height
pcfg.panel_width   = SCREEN_W;   // Visible width
pcfg.panel_height  = SCREEN_H;   // Visible height
pcfg.offset_x      = 0;          // Pixel offset (for windowed panels)
pcfg.offset_y      = 0;
pcfg.offset_rotation = 0;        // Rotation offset (0-7)
pcfg.bus_shared    = false;       // false = exclusive bus
pcfg.invert        = true;        // Color inversion (most panels need true)
pcfg.rgb_order     = false;       // true for some panels (ST77961)
```

### Choosing `spi_host`

The ESP32-S3 has two usable SPI buses:
- `SPI2_HOST` — First available bus
- `SPI3_HOST` — Second available bus

**Rule**: If you have two gauges on the same ESP32, put them on different SPI hosts. If only one gauge, either host works.

### Backlight (Light_PWM)

If your display has a PWM-controlled backlight:

```cpp
class LGFX_YourGauge : public lgfx::LGFX_Device {
    lgfx::Bus_SPI      _bus;
    lgfx::Panel_GC9A01 _panel;
    lgfx::Light_PWM    _light;    // <-- Add this
public:
    LGFX_YourGauge() {
        // ... bus config ...
        // ... panel config ...

        // Backlight
        {
            auto lcfg = _light.config();
            lcfg.pin_bl  = BACKLIGHT_PIN;
            lcfg.freq    = 5000;          // PWM frequency
            lcfg.pwm_channel = 0;         // LEDC channel
            lcfg.invert  = false;
            _light.config(lcfg);
            _panel.setLight(&_light);
        }

        setPanel(&_panel);
    }
};
```

Then control brightness: `tft.setBrightness(128);` (0-255)

---

## 7. Memory Management

TFT gauges use three memory pools:

### Internal RAM (DMA-capable)
Used for DMA bounce buffers. These MUST be in internal RAM for DMA to work.

```cpp
dmaBounce[0] = (uint16_t*)heap_caps_aligned_alloc(
    32,                                           // 32-byte alignment for DMA
    STRIPE_BYTES,                                 // Width * STRIPE_H * 2 bytes
    MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
```

### PSRAM (External)
Used for large buffers: frame sprite, background caches.

```cpp
bgCache[0] = (uint16_t*)heap_caps_aligned_alloc(
    16,                                           // 16-byte alignment
    FRAME_BYTES,                                  // Width * Height * 2 bytes
    MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
```

### Memory Budget Reference

| Item | 240x240 gauge | 360x360 gauge |
|------|---------------|---------------|
| DMA bounce x2 | 2 x 1,920 = 3,840 bytes | 2 x 5,760 = 11,520 bytes |
| Frame sprite | 115,200 bytes (PSRAM) | 259,200 bytes (PSRAM) |
| BG cache x2 | 2 x 115,200 = 230,400 bytes (PSRAM) | 2 x 259,200 = 518,400 bytes (PSRAM) |
| Needle sprite(s) | ~2,640 bytes each (internal) | ~6,000+ bytes each (internal) |
| **Total PSRAM** | **~346 KB** | **~778 KB** |
| **Total Internal** | **~6.5 KB** | **~17.5 KB** |

### MAX_MEMORY_TFT

This defines the stripe height for DMA transfers:
- `4` — Good for 240x240 displays (1,920 bytes per bounce buffer)
- `8` — Better for 360x360 displays (5,760 bytes per bounce buffer)

Larger values = fewer DMA transactions but more internal RAM used.

---

## 8. DCS-BIOS Integration

DCS-BIOS sends aircraft instrument values from the flight simulator. Your gauge subscribes to specific labels and receives callbacks when values change.

### Subscription Types

| Function | Callback Signature | Use Case |
|----------|-------------------|----------|
| `subscribeToLedChange(label, cb)` | `void(const char*, uint16_t value, uint16_t max_value)` | Analog gauges (needles), lamps |
| `subscribeToMetadataChange(label, cb)` | `void(const char*, uint16_t value)` | Discrete values |
| `subscribeToDisplayChange(label, cb)` | `void(const char*, const char* value)` | String displays |
| `subscribeToSelectorChange(label, cb)` | `void(const char*, uint16_t value)` | Rotary switch positions |

### Typical Gauge Callback

```cpp
// Needle position callback
static void onValueChange(const char*, uint16_t v, uint16_t) {
    // v ranges from 0 to 65535 (DCS-BIOS normalizes all values)
    int16_t a = map(v, 0, 65535, ANG_MIN, ANG_MAX);
    if (a != angle) {
        angle = a;
        gaugeDirty = true;  // Signal that a repaint is needed
    }
}

// Dimmer/backlight callback (Day/NVG switching)
static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode != currentLightingMode) {
        currentLightingMode = mode;
        needsFullFlush = true;   // Full repaint for lighting change
        gaugeDirty = true;
    }
}

// Boolean lamp callback
static void onLampChange(const char*, uint16_t v, uint16_t) {
    bool on = (v != 0);
    if (on != lampState) {
        lampState = on;
        gaugeDirty = true;
    }
}
```

### Common DCS-BIOS Labels (AH-64D Apache)

| Label | Description | Typical Use |
|-------|------------|-------------|
| `CONSOLES_DIMMER` | Console panel dimmer | Day/NVG switch for console gauges |
| `INST_PNL_DIMMER` | Instrument panel dimmer | Day/NVG switch for instrument gauges |
| `VOLT_U` | Battery voltage (utility) | Needle position |
| `VOLT_E` | Battery voltage (essential) | Needle position |

### Finding Your DCS-BIOS Labels

1. Open DCS-BIOS documentation for your aircraft module
2. Find the instrument you want to replicate
3. Note the label name and value range
4. The label name goes in `subscribeToLedChange("LABEL_NAME", callback)`

---

## 9. The Rendering Pipeline

Every frame follows this exact sequence:

```
1. CHECK: Has anything changed? (gaugeDirty, angle != lastDrawn)
   |-- No  --> return early (skip rendering)
   |-- Yes --> continue
   v
2. THROTTLE: Has enough time passed since last draw?
   |-- No  --> return early
   |-- Yes --> continue
   v
3. SELECT ASSETS: Day or NVG background/needle?
   v
4. REBUILD NEEDLE: If lighting mode changed, rebuild needle sprite
   v
5. CALCULATE DIRTY RECT: Union of old needle AABB + new needle AABB
   v
6. RESTORE BACKGROUND: memcpy background pixels in dirty rect to frame sprite
   v
7. COMPOSE NEEDLE: Rotate needle sprite onto frame sprite (clipped to dirty rect)
   v
8. DMA FLUSH: Send dirty rect pixels from frame sprite to display via DMA
   v
9. UPDATE STATE: lastDrawnAngle = currentAngle, needsFullFlush = false
```

### Why This Order Matters

- **Step 6 before 7**: Background must be restored BEFORE composing the needle, otherwise the old needle image persists
- **Step 7 uses clipping**: `setClipRect()` prevents the rotated needle from drawing outside the dirty rectangle
- **Step 8 is non-blocking by default**: DMA runs in the background while the CPU returns to other tasks

---

## 10. Dirty-Rect Optimization

Instead of redrawing the entire 240x240 or 360x360 screen every frame, we only repaint the region that actually changed.

### How It Works

```
Frame N-1: Needle at 45 degrees    Frame N: Needle at 47 degrees
         ┌───────────┐                     ┌───────────┐
         │           │                     │           │
         │   ╱(old)  │                     │    ╱(new) │
         │  ╱        │                     │   ╱       │
         │ ●─────────│                     │ ●─────────│
         │           │                     │           │
         └───────────┘                     └───────────┘

Dirty rect = Union of old needle AABB + new needle AABB
           = Small rectangle around the needle tip area
```

### The rotatedAABB Function

Calculates the axis-aligned bounding box of a rotated rectangle:

```cpp
Rect rotatedAABB(int cx, int cy,     // Center of rotation
                 int w, int h,       // Needle dimensions
                 int px, int py,     // Pivot point within needle
                 float deg);         // Rotation angle in degrees
```

The dirty rect is the union of:
- Old needle position AABB (where to erase)
- New needle position AABB (where to draw)

### Multiple Needles

For gauges with two needles (like the Battery gauge with U and E needles):

```cpp
Rect uOld = rotatedAABB(..., lastDrawnAngleU);
Rect uNew = rotatedAABB(..., currentAngleU);
Rect eOld = rotatedAABB(..., lastDrawnAngleE);
Rect eNew = rotatedAABB(..., currentAngleE);
dirty = rectUnion(rectUnion(uOld, uNew), rectUnion(eOld, eNew));
```

### Static Indicators (Lamps, Flags)

For non-rotating elements (lamps that turn on/off, flags that appear/disappear), create fixed rects:

```cpp
if (lowAltOn != lastLowAltOn) {
    Rect lampRect = {LAMP_X, LAMP_Y, LAMP_W, LAMP_H};
    dirty = rectUnion(dirty, lampRect);
}
```

---

## 11. DMA Double-Buffer Flush

The `flushRectToDisplay()` function sends pixels to the display using DMA with double-buffering for maximum throughput.

### How Double-Buffering Works

```
Time -->

Buffer A: [PACK stripe 0] [............DMA stripe 0............]
Buffer B:                  [PACK stripe 1] [.....DMA stripe 1.....]
Buffer A:                                  [PACK stripe 2] [..DMA stripe 2..]
                                                                              ...

CPU packs the NEXT stripe into one buffer
while DMA sends the PREVIOUS stripe from the other buffer.
```

### Key Parameters

- **STRIPE_H** (`MAX_MEMORY_TFT`): Number of pixel rows per DMA transfer
  - `4` for 240px wide displays: each stripe = 240 * 4 * 2 = 1,920 bytes
  - `8` for 360px wide displays: each stripe = 360 * 8 * 2 = 5,760 bytes
- **Blocking vs Non-blocking**:
  - `blocking = true`: Waits for all DMA to complete before returning (used for init, BIT test)
  - `blocking = false`: Returns immediately, DMA finishes in background (used during normal operation)

### The DMA Fence

```cpp
static bool dmaBusy = false;
static inline void waitDMADone() {
    if (dmaBusy) { tft.waitDMA(); dmaBusy = false; }
}
```

Always call `waitDMADone()` before:
- Modifying the frame sprite buffer
- Starting a new flush
- Accessing the bounce buffers

---

## 12. Day/NVG Lighting Modes

CockpitOS gauges support two lighting modes that match the real aircraft:

| Mode | `currentLightingMode` | Background | Needle | When |
|------|----------------------|------------|--------|------|
| Day | `0` | `bgCache[0]` | `yourNeedle` | Dimmer > NVG_THRESHOLD |
| NVG | `2` | `bgCache[1]` | `yourNeedleNVG` | Dimmer <= NVG_THRESHOLD |

### How It Works

1. The dimmer DCS-BIOS label sends a value (0-65535)
2. If value > `NVG_THRESHOLD` (6553), we use Day assets
3. If value <= `NVG_THRESHOLD`, we switch to NVG (green-tinted) assets
4. On mode change, set `needsFullFlush = true` to repaint the entire display

### NVG Assets

NVG versions of backgrounds and needles are typically green-tinted versions of the Day assets. You need both Day and NVG versions of:
- Background image
- Needle image(s)
- Any overlay/lamp images that change appearance in NVG

---

## 13. BIT (Built-In Test)

The BIT test sweeps the needle(s) from minimum to maximum and back. It runs during initialization to verify the gauge is working.

```cpp
void YourGauge_bitTest() {
    int16_t origAngle = angle;
    const int STEP = 1, DELAY = 2;

    // Sweep up
    for (int i = 0; i <= 120; i += STEP) {
        angle = map(i, 0, 120, ANG_MIN, ANG_MAX);
        gaugeDirty = true;
        YourGauge_draw(true, true);  // force=true, blocking=true
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    // Sweep down
    for (int i = 120; i >= 0; i -= STEP) {
        angle = map(i, 0, 120, ANG_MIN, ANG_MAX);
        gaugeDirty = true;
        YourGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    // Restore original value
    angle = origAngle;
    needsFullFlush = true;
    gaugeDirty = true;
    YourGauge_draw(true, true);
}
```

---

## 14. FreeRTOS Task Mode

By default, gauges can run as a dedicated FreeRTOS task for smoother rendering:

```cpp
#define RUN_GAUGE_AS_TASK 1

static void YourGauge_task(void*) {
    for (;;) {
        YourGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));   // ~200 Hz polling
    }
}

// In init:
xTaskCreatePinnedToCore(
    YourGauge_task,      // Function
    "YourGaugeTask",     // Name (for debugging)
    4096,                // Stack size (bytes)
    nullptr,             // Parameter
    2,                   // Priority (2 = above idle)
    &tftTaskHandle,      // Handle (for cleanup)
    CPU_CORE             // Core (0 or 1)
);
```

### Task vs Loop Mode

| Mode | Pros | Cons |
|------|------|------|
| Task (`RUN_GAUGE_AS_TASK=1`) | Smooth rendering, independent timing | Uses a FreeRTOS slot + 4KB stack |
| Loop (`RUN_GAUGE_AS_TASK=0`) | Simpler, no extra resources | Rendering tied to main loop speed |

### CPU Core Selection

- **Core 0**: Default. WiFi/BT also runs here, so avoid if you use wireless.
- **Core 1**: Preferred if Core 0 is busy with WiFi or other tasks.

---

## 15. Asset Pipeline

### Asset Format

Assets are C header files containing `const uint16_t` arrays in RGB565 big-endian format.

```cpp
// yourNeedle.h
// 'Needle', 15x88 Pivot point 7x84
// Generated from PNG (RGB, 16bpp RGB565 BE order)
const uint16_t yourNeedle[] = {
    0x0120, 0x0120, 0xFFFF, 0x0120, ...
    // Width * Height uint16_t values
};
```

### Key Details

- **Color format**: RGB565, big-endian byte order
- **Transparent pixels**: Use color `0x2001` (TRANSPARENT_KEY) — this color is unlikely to appear in real artwork
- **Pivot point**: Noted in the comment header, e.g., "Pivot point 7x84" means the rotation center is at pixel (7, 84) within the needle image
- **File organization**: `src/Panels/Assets/YourGauge/` folder

### Required Assets

| File | Description |
|------|-------------|
| `yourBackground.h` | Full-screen Day background (SCREEN_W x SCREEN_H) |
| `yourBackgroundNVG.h` | Full-screen NVG background |
| `yourNeedle.h` | Day needle image |
| `yourNeedleNVG.h` | NVG needle image |

### Creating Assets from PNG

Use an image-to-RGB565 converter. The output should be a flat array of `uint16_t` values in big-endian order. Many tools exist:

1. **LCD Image Converter** (desktop app)
2. **LVGL Image Converter** (web-based)
3. **Custom Python script** using PIL/Pillow

**Critical**: The transparent areas of your needle MUST use the exact color `0x2001`. Replace any other transparent/background pixels with this value.

### Using AI to Generate Asset Conversion Scripts

> **Claude AI Prompt** (copy-paste ready):
>
> ```
> I need a Python script that converts a PNG image to a C header file
> for use with LovyanGFX on ESP32. Requirements:
>
> 1. Read a PNG file (RGBA or RGB)
> 2. Convert each pixel to RGB565 format (big-endian byte order)
> 3. Replace all fully transparent pixels (alpha=0) with 0x2001
> 4. Output a .h file with a const uint16_t array
> 5. Include a comment with image dimensions and pivot point
> 6. The pivot point should be provided as command-line arguments
>
> Example usage: python png_to_rgb565.py needle.png 7 84
> Output: needle.h with const uint16_t needle[] = { ... };
> ```

---

## 16. Pin Configuration & Label Sets

### How Pins Are Defined

Each gauge defines default pins as `-1` (disconnected). Your label set's `CustomPins.h` overrides them with actual GPIO numbers.

**In the gauge .cpp file:**
```cpp
#ifndef YOURGAUGE_MOSI_PIN
  #define YOURGAUGE_MOSI_PIN  -1
#endif
```

**In your label set's CustomPins.h:**
```cpp
// src/LABELS/LABEL_SET_YOUR_SET/CustomPins.h

#define ENABLE_TFT_GAUGES    1   // Must be 1 to enable TFT gauges

// Your Gauge pins
#define YOURGAUGE_MOSI_PIN   PIN(8)
#define YOURGAUGE_SCLK_PIN   PIN(9)
#define YOURGAUGE_DC_PIN     PIN(13)
#define YOURGAUGE_CS_PIN     PIN(36)
#define YOURGAUGE_RST_PIN    PIN(12)
```

### The Compilation Guard

Every gauge file is wrapped in a compilation guard that checks:

```cpp
#if defined(HAS_YOUR_LABEL_SET)           // Label set is selected
    && (defined(ESP_FAMILY_S3) || defined(ESP_FAMILY_S2))  // Chip has PSRAM
    && (defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1))  // TFT enabled
```

- `HAS_YOUR_LABEL_SET` — Defined automatically based on which label set you compile
- `ESP_FAMILY_S3` / `ESP_FAMILY_S2` — Defined based on your board selection
- `ENABLE_TFT_GAUGES` — Set to `1` in your CustomPins.h

---

## 17. PanelKind Registration

### The REGISTER_PANEL Macro

```cpp
REGISTER_PANEL(TFTYourGauge,     // PanelKind enum name
               nullptr,           // init hook (not used, we use disp_init)
               nullptr,           // loop hook (not used, we use disp_loop)
               YourGauge_init,    // disp_init — called during display init phase
               YourGauge_loop,    // disp_loop — called every display loop
               nullptr,           // tick — optional per-frame hook
               100);              // priority (100 = default, lower = runs earlier)
```

### The PANEL_KIND Comment

Every gauge file MUST have this comment in the first 30 lines:

```cpp
// PANEL_KIND: TFTYourGauge
```

This is read by `generate_panelkind.py` to auto-generate the enum.

### Generating PanelKind.h

After creating your gauge file, run from the CockpitOS root:

```bash
python generate_panelkind.py
```

This scans all `src/Panels/*.cpp` files and generates `src/Generated/PanelKind.h`:

```cpp
enum class PanelKind : uint8_t {
    // ... existing entries ...
    TFTYourGauge,    // PANEL_KIND in TFT_Gauges_YourGauge.cpp
    // ...
    COUNT
};
```

**Important**: If you forget to run this script, you will get a compile error:
```
[PanelRegistry] Invalid panel name 'TFTYourGauge'.
--> You probably forgot to add 'TFTYourGauge' to enum class PanelKind
```

---

## 18. Shared Utilities (TFT_GaugeUtils.h)

Located at `src/Panels/includes/TFT_GaugeUtils.h`, this header provides all the pure-math utilities shared across gauges.

### Usage

```cpp
// MUST define SCREEN_W and SCREEN_H BEFORE including
static constexpr int16_t SCREEN_W = 240, SCREEN_H = 240;
#include "includes/TFT_GaugeUtils.h"
```

### What It Provides

| Symbol | Type | Description |
|--------|------|-------------|
| `TRANSPARENT_KEY` | `uint16_t` (0x2001) | Magic color for sprite transparency |
| `NVG_THRESHOLD` | `uint16_t` (6553) | Dimmer value threshold for Day/NVG |
| `struct Rect` | struct | `{ int16_t x, y, w, h }` |
| `rectEmpty(r)` | function | Returns true if rect has zero or negative area |
| `rectClamp(r)` | function | Clamps rect to screen bounds (uses SCREEN_W/H) |
| `rectUnion(a, b)` | function | Returns smallest rect containing both a and b |
| `rectPad(r, px)` | function | Expands rect by px pixels on all sides |
| `rotatedAABB(...)` | function | AABB of a rotated rectangle (for dirty rects) |

### Why SCREEN_W/SCREEN_H Before Include?

The header uses `static inline` functions that reference `SCREEN_W` and `SCREEN_H` for clamping. Since each gauge has its own resolution, each `.cpp` file defines these before including the header. The `static inline` functions compile independently per translation unit with the correct dimensions.

---

## 19. Reference: Existing Gauges

Use these as templates for your own gauge:

### Simple Gauge: Battery (TFT_Gauges_Battery.cpp)
- **Display**: GC9A01, 240x240, round
- **Bus**: SPI3_HOST @ 80 MHz
- **Needles**: 2 (U voltage, E voltage)
- **Indicators**: None
- **Best template for**: Basic dual-needle round gauges

### Complex Gauge: Radar Altimeter (TFT_Gauges_RadarAlt.cpp)
- **Display**: ST77961, 360x360, round
- **Bus**: SPI3_HOST @ 80 MHz
- **Needles**: 2 (altitude, minimum height pointer)
- **Indicators**: Low altitude lamp, green lamp, OFF flag
- **Best template for**: Gauges with needles AND static indicators (lamps, flags)

### QSPI Gauge: Cabin Pressure Test (TFT_Gauges_CabPressTest.cpp)
- **Display**: ST77916, 360x360, round
- **Bus**: SPI2_HOST, QSPI @ 32 MHz (128 Mbit/s effective)
- **Special**: Dual-mode (QSPI and SPI), custom Panel_ST77916 class
- **Best template for**: QSPI displays, Waveshare boards

### Multi-Interface: CMWS Display (TFT_Display_CMWS.cpp)
- **Display**: ST7789/ILI9341, 240x320, rectangular
- **Bus**: SPI/HSPI/Parallel
- **Special**: 4 interface variants selected by `#define`
- **Best template for**: Supporting multiple display boards with one codebase

---

## 20. Troubleshooting

### Blank Screen

| Cause | Fix |
|-------|-----|
| Wrong SPI pins | Double-check MOSI, SCLK, CS, DC in CustomPins.h |
| Wrong `spi_host` | Try `SPI2_HOST` vs `SPI3_HOST` |
| `pin_cs` wrong | Verify with multimeter or board schematic |
| `invert` wrong | Toggle `pcfg.invert` between `true` and `false` |
| `offset_x`/`offset_y` wrong | Set both to 0 first, adjust if display is shifted |
| No PSRAM | TFT gauges require ESP32-S2 or S3 with PSRAM |
| ENABLE_TFT_GAUGES not set | Add `#define ENABLE_TFT_GAUGES 1` to CustomPins.h |

### Garbled/Shifted Display

| Cause | Fix |
|-------|-----|
| Wrong `memory_width`/`memory_height` | Match your display's actual resolution |
| Wrong `panel_width`/`panel_height` | Match visible area (may differ from memory) |
| `offset_rotation` incorrect | Try values 0-7 |
| `rgb_order` wrong | Toggle `pcfg.rgb_order` |
| `setSwapBytes` incorrect | Frame sprite: `false`, needle sprite: `true` |

### Colors Wrong (Red/Blue Swapped)

- Toggle `pcfg.rgb_order` in panel config
- Check if your assets are in the correct byte order

### DMA Crashes / Guru Meditation

| Cause | Fix |
|-------|-----|
| Bounce buffer in PSRAM | Must use `MALLOC_CAP_INTERNAL \| MALLOC_CAP_DMA` |
| Buffer not aligned | Use `heap_caps_aligned_alloc(32, ...)` |
| Writing during DMA | Always call `waitDMADone()` before modifying buffers |
| Stack overflow in task | Increase stack size from 4096 to 8192 |

### Flickering/Tearing

| Cause | Fix |
|-------|-----|
| Full screen repaint every frame | Verify dirty-rect is working (should be small) |
| `needsFullFlush` always true | Check it is set to `false` after draw completes |
| Draw interval too fast | Increase `GAUGE_DRAW_MIN_INTERVAL_MS` |

### Memory Allocation Failed

```
"dmaBounce alloc failed" → Not enough internal RAM
"bgCache alloc failed"   → Not enough PSRAM
"frameSpr alloc failed"  → Not enough PSRAM
```

Reduce `MAX_MEMORY_TFT` or check that your board has PSRAM enabled in the board configuration.

---

## 21. AI-Assisted Development

When implementing a new gauge, AI tools can significantly speed up the process. Below are ready-to-use prompts for Claude AI.

---

### Prompt 1: Create a Complete New Gauge File

> Copy and paste this into Claude AI, replacing the bracketed values:

```
I'm developing a TFT gauge for CockpitOS (ESP32 + LovyanGFX).
I need you to create a complete gauge .cpp file with these specs:

GAUGE NAME: [e.g., "Torque"]
PANEL_KIND: [e.g., "TFTTorque"]
DISPLAY: [e.g., "GC9A01 240x240 round" or "ST7789 320x240 rectangular"]
BUS: [e.g., "SPI" or "QSPI"]
SPI HOST: [e.g., "SPI2_HOST" or "SPI3_HOST"]
NUMBER OF NEEDLES: [e.g., "1" or "2"]
NEEDLE DIMENSIONS: [e.g., "15x88, pivot at 7x84"]
ANGLE RANGE: [e.g., "-150 to 150 degrees"]
DCS-BIOS LABELS: [e.g., "TRQ_PERCENT" for needle, "CONSOLES_DIMMER" for lighting]
LABEL SET GUARD: [e.g., "HAS_RIGHT_PANEL_CONTROLLER"]
INDICATORS (optional): [e.g., "none" or "low warning lamp at 95x158, 36x36"]
PIN PREFIX: [e.g., "TORQUE" → TORQUE_MOSI_PIN, TORQUE_SCLK_PIN, etc.]

Use the CockpitOS pattern:
- Dirty-rect DMA rendering with double-buffered bounce buffers
- PSRAM frame sprite + background caches (day/NVG)
- TFT_GaugeUtils.h for Rect utilities (define SCREEN_W/SCREEN_H before include)
- REGISTER_PANEL macro for auto-registration
- FreeRTOS task mode with configurable RUN_GAUGE_AS_TASK
- BIT self-test sweep on init
- Day/NVG lighting mode switching via CONSOLES_DIMMER

Follow the exact structure of TFT_Gauges_Battery.cpp from CockpitOS.
Include all sections: registration, config, pins, assets, LGFX class,
state, DMA fence, helpers, callbacks, draw, task, init, loop, deinit.
```

---

### Prompt 2: Configure LGFX for a New Display

> Use this when you have a new display module and need the LovyanGFX configuration:

```
I need to configure LovyanGFX for a new TFT display in CockpitOS.

DISPLAY CONTROLLER: [e.g., "ST7789" or "GC9A01" or "ILI9341"]
RESOLUTION: [e.g., "240x240" or "320x240" or "360x360"]
BUS TYPE: [e.g., "SPI" or "QSPI" or "8-bit parallel"]
SPI HOST: [e.g., "SPI2_HOST"]
FREQUENCY: [e.g., "80MHz" or "40MHz"]

PIN CONNECTIONS:
- MOSI/SDA: GPIO [number]
- SCLK/SCL: GPIO [number]
- CS: GPIO [number]
- DC: GPIO [number]  (or "none" for QSPI)
- RST: GPIO [number] (or "none")
- BLK/backlight: GPIO [number] (or "always on")
[For QSPI: IO0-IO3 pins]

DISPLAY CHARACTERISTICS:
- Color inversion needed? [yes/no/unknown]
- RGB order or BGR? [RGB/BGR/unknown]
- Panel offset X/Y? [0/0 or specific values]

Create a LovyanGFX LGFX_Device class with Bus_SPI and the correct
Panel_XXX class. Include Light_PWM if backlight is PWM-controllable.
Follow the CockpitOS pattern used in TFT_Gauges_Battery.cpp.
```

---

### Prompt 3: Convert PNG Assets to CockpitOS Format

```
Write a Python script that converts PNG images to CockpitOS-compatible
C header files for LovyanGFX TFT gauges.

Requirements:
1. Accept PNG input (RGBA or RGB)
2. Output RGB565 big-endian uint16_t array in a .h file
3. Replace fully transparent pixels (alpha < 128) with 0x2001
   (CockpitOS transparent key)
4. Accept optional pivot point arguments for needle images
5. Generate properly formatted C header with:
   - Comment showing dimensions and pivot point
   - const uint16_t arrayName[] = { ... };
   - 15 values per line for readability
6. Auto-generate array name from filename

Usage: python png_to_cockpitos.py image.png [--pivot-x N --pivot-y N]
Output: image.h
```

---

### Prompt 4: Adapt an Existing Gauge to a Different Display

```
I have an existing CockpitOS TFT gauge file that works with [CURRENT DISPLAY]
and I need to adapt it for [NEW DISPLAY].

Current setup:
- Display: [e.g., "GC9A01 240x240"]
- Bus: [e.g., "SPI @ 80MHz"]
- Panel class: [e.g., "Panel_GC9A01"]

New setup:
- Display: [e.g., "ST7789 240x320"]
- Bus: [e.g., "SPI @ 80MHz"]
- Panel class: [e.g., "Panel_ST7789"]

What needs to change:
1. LGFX device class (panel type, resolution, any new config options)
2. SCREEN_W / SCREEN_H constants
3. CENTER_X / CENTER_Y (if the gauge needs recentering)
4. MAX_MEMORY_TFT (if resolution changed significantly)
5. Any offset_x / offset_y values
6. rgb_order, invert, offset_rotation settings
7. Asset dimensions (if resolution changed)

Please list every change needed and provide the updated code sections.
```

---

### Prompt 5: Debug a Display Issue

```
I'm having a display issue with my CockpitOS TFT gauge. Help me debug it.

SYMPTOM: [Describe what you see — blank screen, garbled, shifted,
          wrong colors, flickering, crashes, etc.]

MY SETUP:
- ESP32 variant: [S2 or S3]
- Display: [controller + resolution]
- Bus: [SPI/QSPI + host + frequency]
- Panel class: [which LovyanGFX panel class]

WHAT I'VE TRIED:
[List any troubleshooting steps already attempted]

RELEVANT CONFIG:
[Paste your LGFX class constructor, or at minimum:
 spi_host, freq_write, pin assignments, panel config
 (invert, rgb_order, offset_x, offset_y, memory_width, etc.)]

Please suggest a systematic debugging approach, starting with
the most likely causes for this symptom.
```

---

### Prompt 6: Add Multiple Indicators to a Gauge

```
I have a basic CockpitOS needle gauge and need to add static indicators
(lamps, flags, or overlays that appear/disappear based on DCS-BIOS values).

GAUGE FILE: [filename or paste relevant sections]
SCREEN SIZE: [e.g., 360x360]

INDICATORS TO ADD:
1. [Name]: [type: lamp/flag/overlay], position [x,y], size [w x h],
   DCS-BIOS label: [label], behavior: [on when value > 0 / etc.]
2. [Name]: ...

For each indicator I need:
- A sprite variable and build function
- A DCS-BIOS callback
- Dirty rect tracking (include in the rect union calculation)
- Day/NVG variant support
- Compositing in the draw function (blit after background restore,
  before or after needle compose as appropriate)

Follow the pattern used in TFT_Gauges_RadarAlt.cpp which has
lowAltLamp, greenLamp, and offFlag indicators alongside two needles.
```

---

## Quick Reference Card

```
FILE LOCATION:     src/Panels/TFT_Gauges_YourGauge.cpp
ASSETS LOCATION:   src/Panels/Assets/YourGauge/
PINS LOCATION:     src/LABELS/LABEL_SET_xxx/CustomPins.h
SHARED UTILS:      src/Panels/includes/TFT_GaugeUtils.h
PANELKIND SCRIPT:  python generate_panelkind.py  (run from CockpitOS root)

TRANSPARENT COLOR:  0x2001
NVG THRESHOLD:      6553
COLOR DEPTH:        16 (RGB565)
FRAME SPRITE:       PSRAM, setSwapBytes(false)
NEEDLE SPRITE:      Internal RAM, setSwapBytes(true)
DMA BOUNCE:         Internal RAM, 32-byte aligned, MALLOC_CAP_DMA
BG CACHE:           PSRAM, 16-byte aligned
```

---

*This guide is part of the CockpitOS project. For programming assistance, use Claude AI (https://claude.ai) with the prompts provided in Section 21.*
