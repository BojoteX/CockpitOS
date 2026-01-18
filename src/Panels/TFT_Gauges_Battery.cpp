// CockpitOS — Battery Gauge (LovyanGFX, GC9A01 @ 240×240)
// Dirty-rect compose + region DMA flush (PSRAM sprites, DMA-safe)

// PANEL_KIND: TFTBatt

#include "../Globals.h"

#if defined(HAS_RIGHT_PANEL_CONTROLLER) && (defined(ESP_FAMILY_S3) || defined(ESP_FAMILY_S2)) && (defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1))

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "includes/TFT_Gauges_Batt.h"

// Load Library
#if !__has_include(<LovyanGFX.hpp>)
#error "❌ Missing LovyanGFX.hpp — Please install LovyanGFX library: https://github.com/lovyan03/LovyanGFX"
#endif

#include <LovyanGFX.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>

#if defined(HAS_RIGHT_PANEL_CONTROLLER)
    REGISTER_PANEL(TFTBatt, nullptr, nullptr, BatteryGauge_init, BatteryGauge_loop, nullptr, 100);
#endif

#define MAX_MEMORY_TFT 4
#define GAUGE_DRAW_MIN_INTERVAL_MS 13
#define RUN_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "CONSOLES_DIMMER"
#define COLOR_DEPTH_BATT 16

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// Core
#if defined(ESP_FAMILY_S3)
#define BATT_CPU_CORE 0
#else
#define BATT_CPU_CORE 0
#endif

// TFT Gauges PINs set to -1 if they were never defined
#ifndef BATTERY_MOSI_PIN
  #define BATTERY_MOSI_PIN   -1  // SDA (Yellow)
#endif
#ifndef BATTERY_SCLK_PIN
  #define BATTERY_SCLK_PIN   -1  // SCL (Orange)
#endif
#ifndef BATTERY_DC_PIN
  #define BATTERY_DC_PIN     -1  // Data/Command (Green)
#endif
#ifndef BATTERY_CS_PIN
  #define BATTERY_CS_PIN     -1  // Chip Select (Blue)
#endif
#ifndef BATTERY_RST_PIN
  #define BATTERY_RST_PIN    -1  // Used by the Right Panel Controller (UNKNOWN COLOR)
#endif

// --- Assets (240x240 bg, 15x88 needle) ---
#include "Assets/BatteryGauge/batBackground.h"
#include "Assets/BatteryGauge/batBackgroundNVG.h"
#include "Assets/BatteryGauge/batNeedle.h"
#include "Assets/BatteryGauge/batNeedleNVG.h"

// --- Misc ---
static constexpr bool     shared_bus = false;
static constexpr bool     use_lock = false;
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;
static constexpr uint16_t NVG_THRESHOLD = 6553;

static constexpr int16_t  SCREEN_W = 240, SCREEN_H = 240;
static constexpr int16_t  CENTER_X = 120, CENTER_Y = 120;
static constexpr int16_t  NEEDLE_W = 15, NEEDLE_H = 88;
static constexpr int16_t  NEEDLE_PIVOT_X = 7, NEEDLE_PIVOT_Y = 84;

// Angles
static constexpr int16_t U_MIN = -150, U_MAX = -30;
static constexpr int16_t E_MIN = 30, E_MAX = 150;

// DMA bounce stripes (internal RAM)
static constexpr int    STRIPE_H = MAX_MEMORY_TFT;
static constexpr size_t STRIPE_BYTES = size_t(SCREEN_W) * STRIPE_H * sizeof(uint16_t);

static_assert(SCREEN_W > 0 && SCREEN_H > 0, "bad dims");
static_assert(STRIPE_H > 0 && STRIPE_H <= SCREEN_H, "bad STRIPE_H");

// --- Panel binding ---
class LGFX_Battery : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_GC9A01 _panel;
public:
    LGFX_Battery() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI3_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 0;
            cfg.spi_3wire = false;
            cfg.use_lock = use_lock;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_mosi = BATTERY_MOSI_PIN;
            cfg.pin_miso = -1;
            cfg.pin_sclk = BATTERY_SCLK_PIN;
            cfg.pin_dc = BATTERY_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.readable = false;
            pcfg.pin_cs = BATTERY_CS_PIN;
            pcfg.pin_rst = BATTERY_RST_PIN;
            pcfg.pin_busy = -1;
            pcfg.memory_width = SCREEN_W;
            pcfg.memory_height = SCREEN_H;
            pcfg.panel_width = SCREEN_W;
            pcfg.panel_height = SCREEN_H;
            pcfg.offset_x = 0;
            pcfg.offset_y = 0;
            pcfg.offset_rotation = 0;
            pcfg.bus_shared = shared_bus;
            pcfg.invert = true;
            _panel.config(pcfg);
        }
        setPanel(&_panel);
    }
};

// --- State ---
static LGFX_Battery tft;
static lgfx::LGFX_Sprite frameSpr(&tft);
static lgfx::LGFX_Sprite needleU_spr(&tft), needleE_spr(&tft);

// BG caches (PSRAM)
static uint16_t* bgCache[2] = { nullptr, nullptr }; // 0=day,1=NVG

// DMA bounce (internal RAM)
static uint16_t* dmaBounce[2] = { nullptr, nullptr };

// Live values
static volatile int16_t angleU = U_MIN, angleE = E_MAX;
static volatile int16_t lastDrawnAngleU = INT16_MIN, lastDrawnAngleE = INT16_MIN;
static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0; // 0=Day, 2=NVG
static uint8_t          lastNeedleMode = 0xFF;
static volatile bool    needsFullFlush = true;

static unsigned long    lastDrawTime = 0;
static TaskHandle_t     tftTaskHandle = nullptr;

// --- DMA fence ---
static bool dmaBusy = false;
static inline void waitDMADone() {
    if (dmaBusy) { tft.waitDMA(); dmaBusy = false; }
}

// --- Dirty-rect utils ---
/*
struct Rect {
    int16_t x = 0, y = 0, w = 0, h = 0;
};
*/

struct Rect { int16_t x, y, w, h; };
static inline bool rectEmpty(const Rect& r) { return r.w <= 0 || r.h <= 0; }
static inline Rect rectClamp(const Rect& r) {
    int16_t x = r.x, y = r.y, w = r.w, h = r.h;
    if (x < 0) { w += x; x = 0; } if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w < 0) w = 0; if (h < 0) h = 0;
    return { x,y,w,h };
}
static inline Rect rectUnion(const Rect& a, const Rect& b) {
    if (rectEmpty(a)) return b;
    if (rectEmpty(b)) return a;
    int16_t x1 = std::min(a.x, b.x), y1 = std::min(a.y, b.y);
    int16_t x2 = std::max<int16_t>(a.x + a.w, b.x + b.w);
    int16_t y2 = std::max<int16_t>(a.y + a.h, b.y + b.h);
    return rectClamp({ x1,y1,(int16_t)(x2 - x1),(int16_t)(y2 - y1) });
}
static inline Rect rectPad(const Rect& r, int16_t px) {
    return rectClamp({ (int16_t)(r.x - px),(int16_t)(r.y - px),(int16_t)(r.w + 2 * px),(int16_t)(r.h + 2 * px) });
}
static Rect rotatedAABB(int cx, int cy, int w, int h, int px, int py, float deg) {
    const float rad = deg * (float)M_PI / 180.0f;
    float s = sinf(rad), c = cosf(rad);
    const float xs[4] = { -px, (float)w - px, (float)w - px, -px };
    const float ys[4] = { -py, -py, (float)h - py, (float)h - py };
    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = 1e9f * -1.0f;
    for (int i = 0; i < 4; ++i) {
        float xr = xs[i] * c - ys[i] * s;
        float yr = xs[i] * s + ys[i] * c;
        float X = (float)cx + xr;
        float Y = (float)cy + yr;
        if (X < minx) minx = X; if (X > maxx) maxx = X;
        if (Y < miny) miny = Y; if (Y > maxy) maxy = Y;
    }
    Rect r; r.x = (int16_t)floorf(minx); r.y = (int16_t)floorf(miny);
    r.w = (int16_t)ceilf(maxx - minx); r.h = (int16_t)ceilf(maxy - miny);
    return rectClamp(rectPad(r, 2));
}
static inline void blitBGRectToFrame(const uint16_t* bg, int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    uint16_t* dst = (uint16_t*)frameSpr.getBuffer();
    const int pitch = SCREEN_W;
    for (int row = 0; row < h; ++row) {
        std::memcpy(&dst[(y + row) * pitch + x],
            &bg[(y + row) * pitch + x],
            size_t(w) * sizeof(uint16_t));
    }
}

// ----------------- Region DMA flush -----------------
static inline void flushRectToDisplay(const uint16_t* src, const Rect& rr, bool blocking)
{
    Rect r = rectClamp(rr);
    if (rectEmpty(r)) return;

    waitDMADone();

    const int pitch = SCREEN_W;
    const int lines_per = STRIPE_H;
    int y = r.y;

    tft.startWrite();

    int h0 = (lines_per <= r.h) ? lines_per : r.h;
    for (int row = 0; row < h0; ++row) {
        std::memcpy(dmaBounce[0] + row * r.w,
            src + (y + row) * pitch + r.x,
            size_t(r.w) * sizeof(uint16_t));
    }
    tft.setAddrWindow(r.x, y, r.w, h0);
    tft.pushPixelsDMA(dmaBounce[0], uint32_t(r.w) * h0);
    if (!blocking) dmaBusy = true;

    int bb = 1;
    for (y += h0; y < r.y + r.h; y += lines_per, bb ^= 1) {
        const int h = (y + lines_per <= r.y + r.h) ? lines_per : (r.y + r.h - y);

        for (int row = 0; row < h; ++row) {
            std::memcpy(dmaBounce[bb] + row * r.w,
                src + (y + row) * pitch + r.x,
                size_t(r.w) * sizeof(uint16_t));
        }

        tft.waitDMA();
        tft.setAddrWindow(r.x, y, r.w, h);
        tft.pushPixelsDMA(dmaBounce[bb], uint32_t(r.w) * h);

        if (blocking) { tft.waitDMA(); }
    }

    if (blocking) { tft.waitDMA(); dmaBusy = false; }
    tft.endWrite();
}

static inline void flushFrameToDisplay(const uint16_t* src, bool blocking) {
    flushRectToDisplay(src, { 0,0,SCREEN_W,SCREEN_H }, blocking);
}

// --- Sprite builders ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, NEEDLE_W, NEEDLE_H, img);
}

// --- DCS-BIOS ---
static void onBatVoltUChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, U_MIN, U_MAX);
    if (a != angleU) { angleU = a; gaugeDirty = true; }
}
static void onBatVoltEChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, E_MAX, E_MIN);
    if (a != angleE) { angleE = a; gaugeDirty = true; }
}
static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode != currentLightingMode) {
        currentLightingMode = mode;
        needsFullFlush = true; // full repaint on lighting change
        gaugeDirty = true;
    }
}

// --- Draw ---
static void BatteryGauge_draw(bool force = false, bool blocking = false)
{
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();

    int16_t u = angleU; if (u < U_MIN) u = U_MIN; else if (u > U_MAX) u = U_MAX;
    int16_t e = angleE; if (e < E_MIN) e = E_MIN; else if (e > E_MAX) e = E_MAX;

    const bool stateChanged = gaugeDirty
        || (u != lastDrawnAngleU)
        || (e != lastDrawnAngleE)
        || needsFullFlush;
    if (!stateChanged) return;

    // Change 9/2/25
    // if (!force && (now - lastDrawTime < GAUGE_DRAW_MIN_INTERVAL_MS)) return;
    if (!force && !needsFullFlush && (now - lastDrawTime < GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    gaugeDirty = false;

    // Added 9/2/25
    if (needsFullFlush) waitDMADone();

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BATTERY_DRAW);
#endif

    // Assets
    const uint16_t* bg = (currentLightingMode == 0) ? bgCache[0] : bgCache[1];
    const uint16_t* needleImg = (currentLightingMode == 0) ? batNeedle : batNeedleNVG;

    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleU_spr, needleImg);
        buildNeedle(needleE_spr, needleImg);
        lastNeedleMode = currentLightingMode;
    }

    // Dirty rect: union of old/new for both needles
    Rect dirty = needsFullFlush ? Rect{ 0,0,SCREEN_W,SCREEN_H } : Rect{};
    if (!needsFullFlush) {
        if (lastDrawnAngleU == INT16_MIN || lastDrawnAngleE == INT16_MIN) {
            dirty = { 0,0,SCREEN_W,SCREEN_H };
        }
        else {
            Rect uOld = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)lastDrawnAngleU);
            Rect uNew = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)u);
            Rect eOld = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)lastDrawnAngleE);
            Rect eNew = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)e);
            dirty = rectUnion(rectUnion(uOld, uNew), rectUnion(eOld, eNew));
        }
    }

    // Restore BG only in dirty
    blitBGRectToFrame(bg, dirty.x, dirty.y, dirty.w, dirty.h);

    // Compose needles
    frameSpr.setClipRect(dirty.x, dirty.y, dirty.w, dirty.h);
    needleU_spr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);
    needleE_spr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)e, 1.0f, 1.0f, TRANSPARENT_KEY);
    frameSpr.clearClipRect();

    // Flush
    const uint16_t* buf = (const uint16_t*)frameSpr.getBuffer();

    // flushRectToDisplay(buf, dirty, blocking);
    flushRectToDisplay(buf, dirty, needsFullFlush || blocking);

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_BATTERY_DRAW);
#endif

    lastDrawnAngleU = u;
    lastDrawnAngleE = e;
    needsFullFlush = false;
}

// --- Task ---
static void BatteryGauge_task(void*) {
    for (;;) {
        BatteryGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void BatteryGauge_init()
{
    // DMA bounce (internal RAM)
    dmaBounce[0] = (uint16_t*)heap_caps_aligned_alloc(32, STRIPE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    dmaBounce[1] = (uint16_t*)heap_caps_aligned_alloc(32, STRIPE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!dmaBounce[0] || !dmaBounce[1]) {
        debugPrintf("❌ dmaBounce alloc failed (%u bytes each)\n", (unsigned)STRIPE_BYTES);
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // BG caches (PSRAM)
    static constexpr size_t FRAME_PIXELS = size_t(SCREEN_W) * size_t(SCREEN_H);
    static constexpr size_t FRAME_BYTES = FRAME_PIXELS * sizeof(uint16_t);
    static_assert((FRAME_BYTES % 16u) == 0u, "FRAME_BYTES must be 16-byte aligned");

    bgCache[0] = (uint16_t*)heap_caps_aligned_alloc(16, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    bgCache[1] = (uint16_t*)heap_caps_aligned_alloc(16, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bgCache[0] || !bgCache[1]) {
        debugPrintln("❌ bgCache alloc failed");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    std::memcpy(bgCache[0], batBackground, FRAME_BYTES);
    std::memcpy(bgCache[1], batBackgroundNVG, FRAME_BYTES);

    tft.init();
    tft.setColorDepth(COLOR_DEPTH_BATT);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // Compose sprite (PSRAM)
    frameSpr.setColorDepth(COLOR_DEPTH_BATT);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    // Needles
    needleU_spr.setColorDepth(COLOR_DEPTH_BATT);
    needleU_spr.createSprite(NEEDLE_W, NEEDLE_H);
    needleU_spr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleU_spr, batNeedle);

    needleE_spr.setColorDepth(COLOR_DEPTH_BATT);
    needleE_spr.createSprite(NEEDLE_W, NEEDLE_H);
    needleE_spr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleE_spr, batNeedle);

    // DCS-BIOS
    subscribeToLedChange("VOLT_U", onBatVoltUChange);
    subscribeToLedChange("VOLT_E", onBatVoltEChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    // First paint
    needsFullFlush = true; gaugeDirty = true;
    BatteryGauge_draw(true, true);

    // Optional BIT
    BatteryGauge_bitTest();

#if RUN_GAUGE_AS_TASK
    xTaskCreatePinnedToCore(BatteryGauge_task, "BatteryGaugeTask", 4096, nullptr, 2, &tftTaskHandle, BATT_CPU_CORE);
#endif

    debugPrintln("✅ Battery Gauge (dirty-rect DMA) initialized");
}

void BatteryGauge_loop() {
#if !RUN_GAUGE_AS_TASK
    BatteryGauge_draw(false, false);
#endif
}

void BatteryGauge_notifyMissionStart() { needsFullFlush = true; gaugeDirty = true; }

// Visual self-test; blocking flush
void BatteryGauge_bitTest() {
    int16_t origU = angleU, origE = angleE;
    const int STEP = 1, DELAY = 2;
    for (int i = 0; i <= 120; i += STEP) {
        angleU = map(i, 0, 120, U_MIN, U_MAX);
        angleE = map(i, 0, 120, E_MAX, E_MIN);
        gaugeDirty = true; BatteryGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 120; i >= 0; i -= STEP) {
        angleU = map(i, 0, 120, U_MIN, U_MAX);
        angleE = map(i, 0, 120, E_MAX, E_MIN);
        gaugeDirty = true; BatteryGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    angleU = origU; angleE = origE;
    needsFullFlush = true; gaugeDirty = true; BatteryGauge_draw(true, true);
}

void BatteryGauge_deinit() {
    waitDMADone();
    if (tftTaskHandle) { vTaskDelete(tftTaskHandle); tftTaskHandle = nullptr; }

    needleU_spr.deleteSprite();
    needleE_spr.deleteSprite();
    frameSpr.deleteSprite();

    for (int i = 0; i < 2; ++i) {
        if (dmaBounce[i]) { heap_caps_free(dmaBounce[i]); dmaBounce[i] = nullptr; }
        if (bgCache[i]) { heap_caps_free(bgCache[i]);   bgCache[i] = nullptr; }
    }
}

#else
#warning "Battery Gauge requires ESP32-S2 or ESP32-S3"
#endif