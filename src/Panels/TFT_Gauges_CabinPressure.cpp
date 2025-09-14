// TFT_Gauges_CabPress.cpp — CockpitOS Cabin Pressure Gauge (LovyanGFX, ST77916/61 @ 360×360)
// Dirty-rect compose + region DMA flush (PSRAM sprites, DMA-safe)

#include "../Globals.h"

#if (defined(ESP_FAMILY_S3) || defined(ESP_FAMILY_S2)) && (defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1))

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "includes/TFT_Gauges_CabPress.h"

// Load Library
#if !__has_include(<LovyanGFX.hpp>)
#error "❌ Missing LovyanGFX.hpp — Please install LovyanGFX library: https://github.com/lovyan03/LovyanGFX"
#endif

#include <LovyanGFX.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>

#if defined(HAS_ALR67) || defined(HAS_CABIN_PRESSURE_GAUGE) 
    REGISTER_PANEL(TFTCabPress, nullptr, nullptr, CabinPressureGauge_init, CabinPressureGauge_loop, nullptr, 100);
#endif

#define MAX_MEMORY_TFT 8
#define CABIN_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS 13
#define RUN_CABIN_PRESSURE_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "INST_PNL_DIMMER"
#define COLOR_DEPTH_CABIN_PRESS 16

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// Select core
#if defined(ESP_FAMILY_S3)
#define CABPRESS_CPU_CORE 0
#else
#define CABPRESS_CPU_CORE 0
#endif

#if defined(HAS_ALR67) || defined(HAS_CABIN_PRESSURE_GAUGE) 
    #define CABIN_PRESSURE_DC_PIN   PIN(18)  // Data/Command (Green)
    #define CABIN_PRESSURE_CS_PIN   PIN(36)  // Chip Select (Blue)
    #define CABIN_PRESSURE_MOSI_PIN PIN(39)  // SDA (Yellow)
    #define CABIN_PRESSURE_SCLK_PIN PIN(40)  // SCL (Orange)
    #define CABIN_PRESSURE_RST_PIN  -1  // Reset (White)
    #define CABIN_PRESSURE_MISO_PIN -1  // Unused
#else
    #define CABIN_PRESSURE_DC_PIN   -1  // Data/Command (Green)
    #define CABIN_PRESSURE_CS_PIN   -1  // Chip Select (Blue)
    #define CABIN_PRESSURE_MOSI_PIN -1  // SDA (Yellow)
    #define CABIN_PRESSURE_SCLK_PIN -1  // SCL (Orange)
    #define CABIN_PRESSURE_RST_PIN  -1  // Reset (White)
    #define CABIN_PRESSURE_MISO_PIN -1  // Unused
#endif

// --- Assets (360x360 bg, 23x238 needle) ---
#include "Assets/CabinPressure/cabinPressBackground.h"
#include "Assets/CabinPressure/cabinPressBackgroundNVG.h"
#include "Assets/CabinPressure/cabinPressNeedle.h"
#include "Assets/CabinPressure/cabinPressNeedleNVG.h"

static constexpr uint16_t TRANSPARENT_KEY = 0x2001;
static constexpr uint16_t NVG_THRESHOLD = 6553;

static constexpr int16_t  SCREEN_W = 360, SCREEN_H = 360;
static constexpr int16_t  CENTER_X = 180, CENTER_Y = 180;
static constexpr int16_t  NEEDLE_W = 23, NEEDLE_H = 238;
static constexpr int16_t  NEEDLE_PIVOT_X = 12, NEEDLE_PIVOT_Y = 165;

// Angle range (units)
static constexpr int16_t ANG_MIN = -181;
static constexpr int16_t ANG_MAX = 125;

// Bounce stripes (internal RAM, DMA-capable)
static constexpr int    STRIPE_H = MAX_MEMORY_TFT;
static constexpr size_t STRIPE_BYTES = size_t(SCREEN_W) * STRIPE_H * sizeof(uint16_t);

static_assert(SCREEN_W > 0 && SCREEN_H > 0, "bad dims");
static_assert(STRIPE_H > 0 && STRIPE_H <= SCREEN_H, "bad STRIPE_H");

// add near other state vars
static volatile bool needsFullFlush = true;

// --- Misc: Global TFT SPI/Bus Config ---
// Edit these to test combinations
static constexpr bool                   shared_bus    = false;              // false = exclusive bus (recommended)
static constexpr bool                   use_lock      = false;              // false = no mutex/lock (recommended)
static constexpr spi_host_device_t      spi_host      = SPI2_HOST;          // SPI2_HOST or SPI3_HOST
static constexpr uint8_t                dma_channel   = SPI_DMA_CH_AUTO;    // SPI_DMA_CH_AUTO, 1, or 2
static constexpr uint32_t               freq_write    = 80000000;           // Write clock (Hz), try lower if unstable

// --- Panel binding ---
class LGFX_CabPress final : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_ST77961 _panel;
public:
    LGFX_CabPress() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = spi_host;
            cfg.spi_mode = 0;
            cfg.freq_write = freq_write;
            cfg.freq_read = 0;
            cfg.spi_3wire = false;
            cfg.use_lock = use_lock;
            cfg.dma_channel = dma_channel;
            cfg.pin_mosi = CABIN_PRESSURE_MOSI_PIN;
            cfg.pin_miso = CABIN_PRESSURE_MISO_PIN;
            cfg.pin_sclk = CABIN_PRESSURE_SCLK_PIN;
            cfg.pin_dc = CABIN_PRESSURE_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.readable = false;
            pcfg.pin_cs = CABIN_PRESSURE_CS_PIN;
            pcfg.pin_rst = CABIN_PRESSURE_RST_PIN;
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
            pcfg.rgb_order = true;
            _panel.config(pcfg);
        }
        setPanel(&_panel);
    }
};

// --- State ---
static LGFX_CabPress tft;

// Compose target (PSRAM)
static lgfx::LGFX_Sprite frameSpr(&tft);
static lgfx::LGFX_Sprite needleSpr(&tft);

// Background caches (PSRAM)
static uint16_t* bgCache[2] = { nullptr, nullptr }; // [0]=day, [1]=NVG

// DMA bounce buffers (internal RAM)
static uint16_t* dmaBounce[2] = { nullptr, nullptr };

// Live values
static volatile int16_t angleU = ANG_MIN;
static volatile int16_t lastDrawnAngleU = INT16_MIN;
static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0; // 0=Day, 2=NVG
static uint8_t          lastNeedleMode = 0xFF;

static unsigned long    lastDrawTime = 0;
static TaskHandle_t     tftTaskHandle = nullptr;

// DMA fence
static bool dmaBusy = false;
static inline void waitDMADone() {
    if (dmaBusy) { tft.waitDMA(); dmaBusy = false; }
}

// --- Dirty-rect utilities ---
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
// AABB for rotated needle
static Rect rotatedAABB(int cx, int cy, int w, int h, int px, int py, float deg) {
    const float rad = deg * (float)M_PI / 180.0f;
    float s = sinf(rad), c = cosf(rad);
    const float xs[4] = { -px, (float)w - px, (float)w - px, -px };
    const float ys[4] = { -py, -py, (float)h - py, (float)h - py };
    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
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
// Copy BG subsection into frame
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

    // Prime first chunk
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

        // overlap pack with previous DMA
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

// --- Sprite builder ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, NEEDLE_W, NEEDLE_H, img);
}

// --- DCS-BIOS callbacks ---
static void onPressureAltChange(const char*, uint16_t value, uint16_t) {
    int16_t a = map(value, 0, 65535, ANG_MIN, ANG_MAX);
    if (a != angleU) { angleU = a; gaugeDirty = true; }
}

static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode != currentLightingMode) {
        currentLightingMode = mode;
        needsFullFlush = true;   // force full repaint like Radar Alt
        gaugeDirty = true;
    }
}

// --- Draw ---
static void CabinPressureGauge_draw(bool force = false, bool blocking = false)
{
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();

    int16_t u = angleU; if (u < ANG_MIN) u = ANG_MIN; else if (u > ANG_MAX) u = ANG_MAX;

    const bool stateChanged = gaugeDirty || (u != lastDrawnAngleU);
    if (!stateChanged) return;

    // Change 9/2/25
    // if (!force && (now - lastDrawTime < CABIN_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;
    if (!force && !needsFullFlush && (now - lastDrawTime < CABIN_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    gaugeDirty = false;

    // Added 9/2/25
    if (needsFullFlush) waitDMADone();

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_CABIN_PRESSURE_DRAW);
#endif

    // Select assets
    const uint16_t* bg = bgCache[(currentLightingMode == 0) ? 0u : 1u];
    const uint16_t* needleImg = (currentLightingMode == 0) ? cabinPressNeedle : cabinPressNeedleNVG;

    // Rebuild needle sprite on mode change
    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleSpr, needleImg);
        lastNeedleMode = currentLightingMode;
    }

    // Dirty rectangle = union of old/new needle AABBs
    Rect dirty = needsFullFlush ? Rect{ 0,0,SCREEN_W,SCREEN_H } : Rect{};
    if (!needsFullFlush) {
        if (lastDrawnAngleU == INT16_MIN) {
            dirty = { 0,0,SCREEN_W,SCREEN_H };
        }
        else {
            Rect nOld = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)lastDrawnAngleU);
            Rect nNew = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)u);
            dirty = rectUnion(nOld, nNew);
        }
    }

    // Restore BG only within dirty rect
    blitBGRectToFrame(bg, dirty.x, dirty.y, dirty.w, dirty.h);

    // Clip and compose
    frameSpr.setClipRect(dirty.x, dirty.y, dirty.w, dirty.h);
    needleSpr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);
    frameSpr.clearClipRect();

    // Flush region
    const uint16_t* buf = (const uint16_t*)frameSpr.getBuffer();

    // Change 9/2/25
    // flushRectToDisplay(buf, dirty, blocking);
    flushRectToDisplay(buf, dirty, needsFullFlush || blocking);

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_CABIN_PRESSURE_DRAW);
#endif

    needsFullFlush = false;
    lastDrawnAngleU = u;
}

// --- Task ---
static void CabinPressureGauge_task(void*) {
    for (;;) {
        CabinPressureGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void CabinPressureGauge_init()
{
    if (!initPSRAM()) {
        debugPrintln("❌ No PSRAM detected! Cabin Pressure Gauge disabled.");
        while (1) vTaskDelay(1000);
    }

    // DMA bounce buffers (internal RAM)
    dmaBounce[0] = (uint16_t*)heap_caps_aligned_alloc(32, STRIPE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    dmaBounce[1] = (uint16_t*)heap_caps_aligned_alloc(32, STRIPE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!dmaBounce[0] || !dmaBounce[1]) {
        debugPrintf("❌ dmaBounce alloc failed (%u bytes each)\n", (unsigned)STRIPE_BYTES);
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Background caches (PSRAM)
    static constexpr size_t FRAME_PIXELS = size_t(SCREEN_W) * size_t(SCREEN_H);
    static constexpr size_t FRAME_BYTES = FRAME_PIXELS * sizeof(uint16_t);
    static_assert((FRAME_BYTES % 16u) == 0u, "FRAME_BYTES must be 16-byte aligned");

    bgCache[0] = (uint16_t*)heap_caps_aligned_alloc(16, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    bgCache[1] = (uint16_t*)heap_caps_aligned_alloc(16, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bgCache[0] || !bgCache[1]) {
        debugPrintln("❌ bgCache alloc failed");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    // populate once from flash
    std::memcpy(bgCache[0], cabinPressBackground, FRAME_BYTES);
    std::memcpy(bgCache[1], cabinPressBackgroundNVG, FRAME_BYTES);

    tft.init();
    tft.setColorDepth(COLOR_DEPTH_CABIN_PRESS);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // Compose sprite (PSRAM)
    frameSpr.setColorDepth(COLOR_DEPTH_CABIN_PRESS);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    // Needle sprite
    needleSpr.setColorDepth(COLOR_DEPTH_CABIN_PRESS);
    needleSpr.createSprite(NEEDLE_W, NEEDLE_H);
    needleSpr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleSpr, cabinPressNeedle);

    // DCS-BIOS
    subscribeToLedChange("PRESSURE_ALT", onPressureAltChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    // First paint (full)
    lastDrawnAngleU = INT16_MIN;
    gaugeDirty = true;
    CabinPressureGauge_draw(true, true);

    // Optional BIT
    CabinPressureGauge_bitTest();

#if RUN_CABIN_PRESSURE_GAUGE_AS_TASK
    xTaskCreatePinnedToCore(CabinPressureGauge_task, "CabinPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, CABPRESS_CPU_CORE);
#endif

    debugPrintf(
        "✅ Cabin Pressure Gauge initialized "
        "DC=%d (Green), CS=%d (Blue), MOSI=%d (Yellow), SCLK=%d (Orange)\n",
        CABIN_PRESSURE_DC_PIN,
        CABIN_PRESSURE_CS_PIN,
        CABIN_PRESSURE_MOSI_PIN,
        CABIN_PRESSURE_SCLK_PIN
    );

}

void CabinPressureGauge_loop() {
#if !RUN_CABIN_PRESSURE_GAUGE_AS_TASK
    CabinPressureGauge_draw(false, false);
#endif
}

void CabinPressureGauge_notifyMissionStart() { needsFullFlush = true; gaugeDirty = true; }

// Visual self-test; blocking flush to avoid overlap during rapid sweep
void CabinPressureGauge_bitTest() {
    int16_t originalU = angleU;
    const int STEP = 10, DELAY = 2;

    for (int i = 0; i <= 306; i += STEP) {
        angleU = map(i, 0, 306, ANG_MIN, ANG_MAX);
        gaugeDirty = true; CabinPressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 306; i >= 0; i -= STEP) {
        angleU = map(i, 0, 306, ANG_MIN, ANG_MAX);
        gaugeDirty = true; CabinPressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    angleU = originalU;
    gaugeDirty = true; CabinPressureGauge_draw(true, true);
}

void CabinPressureGauge_deinit() {
    waitDMADone();
    if (tftTaskHandle) { vTaskDelete(tftTaskHandle); tftTaskHandle = nullptr; }

    needleSpr.deleteSprite();
    frameSpr.deleteSprite();

    for (int i = 0; i < 2; ++i) {
        if (dmaBounce[i]) { heap_caps_free(dmaBounce[i]); dmaBounce[i] = nullptr; }
        if (bgCache[i]) { heap_caps_free(bgCache[i]);   bgCache[i] = nullptr; }
    }
}

#else
#warning "Cabin Pressure Gauge requires ESP32-S2 or ESP32-S3"
#endif