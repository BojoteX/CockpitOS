// CockpitOS — Hydraulic Pressure Gauge (LovyanGFX, GC9A01 @ 240×240)
// Dirty-rect compose + region DMA flush (PSRAM sprites, DMA-safe)

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "includes/TFT_Gauges_HydPress.h"
#include <LovyanGFX.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>

#if defined(HAS_CUSTOM_RIGHT)
    REGISTER_PANEL(TFTHyd, nullptr, nullptr, HydPressureGauge_init, HydPressureGauge_loop, nullptr, 100);
#endif

#define MAX_MEMORY_TFT 16
#define HYD_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS 13
#define RUN_HYD_PRESSURE_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "INST_PNL_DIMMER"
#define COLOR_DEPTH_HYD_PRESS 16

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// Core
#if defined(ARDUINO_LOLIN_S3_MINI)
#define HYD_CPU_CORE 1
#else
#define HYD_CPU_CORE 0
#endif

// --- Pins ---
#define HYD_PRESSURE_CS_PIN     PIN(17)  // Chip Select (Blue) *** CANT USE GPIO 8 *** Conflict with i2c BUS for PCA9555
#define HYD_PRESSURE_DC_PIN     PIN(3)   // Data/Command (Green)
#define HYD_PRESSURE_MOSI_PIN   PIN(4)   // SDA (Yellow)
#define HYD_PRESSURE_SCLK_PIN  PIN(12)   // SCL (Orange)
#define HYD_PRESSURE_RST_PIN   -1
#define HYD_PRESSURE_MISO_PIN  -1


// --- Assets (240x240 bg, 33x120 needles) ---
#include "Assets/HydPressure/hydPressBackground.h"
#include "Assets/HydPressure/hydPressBackgroundNVG.h"
#include "Assets/HydPressure/hydPressNeedle1.h"
#include "Assets/HydPressure/hydPressNeedle1NVG.h"
#include "Assets/HydPressure/hydPressNeedle2.h"
#include "Assets/HydPressure/hydPressNeedle2NVG.h"

// --- Misc ---
static constexpr bool     shared_bus = false;
static constexpr bool     use_lock = false;
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;
static constexpr uint16_t NVG_THRESHOLD = 6553;

static constexpr int16_t  SCREEN_W = 240, SCREEN_H = 240;
static constexpr int16_t  CENTER_X = 120, CENTER_Y = 120;
static constexpr int16_t  NEEDLE_W = 33, NEEDLE_H = 120;
static constexpr int16_t  NEEDLE_PIVOT_X = 17, NEEDLE_PIVOT_Y = 103;

// Angle range
static constexpr int16_t ANG_MIN = -280;
static constexpr int16_t ANG_MAX = 40;

// DMA bounce stripes (internal RAM)
static constexpr int    STRIPE_H = MAX_MEMORY_TFT;
static constexpr size_t STRIPE_BYTES = size_t(SCREEN_W) * STRIPE_H * sizeof(uint16_t);

static_assert(SCREEN_W > 0 && SCREEN_H > 0, "bad dims");
static_assert(STRIPE_H > 0 && STRIPE_H <= SCREEN_H, "bad STRIPE_H");

// --- Panel binding ---
class LGFX_HydPress : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_GC9A01 _panel;
public:
    LGFX_HydPress() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 0;
            cfg.spi_3wire = false;
            cfg.use_lock = use_lock;
            cfg.dma_channel = SPI_DMA_CH_AUTO; // We can also use SPI_DMA_CH_AUTO
            cfg.pin_mosi = HYD_PRESSURE_MOSI_PIN;
            cfg.pin_miso = HYD_PRESSURE_MISO_PIN;
            cfg.pin_sclk = HYD_PRESSURE_SCLK_PIN;
            cfg.pin_dc = HYD_PRESSURE_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.readable = false;
            pcfg.pin_cs = HYD_PRESSURE_CS_PIN;
            pcfg.pin_rst = HYD_PRESSURE_RST_PIN;
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
static LGFX_HydPress tft;
static lgfx::LGFX_Sprite frameSpr(&tft);
static lgfx::LGFX_Sprite needleL_spr(&tft), needleR_spr(&tft);

// BG caches (PSRAM)
static uint16_t* bgCache[2] = { nullptr, nullptr }; // 0=day,1=NVG

// DMA bounce (internal RAM)
static uint16_t* dmaBounce[2] = { nullptr, nullptr };

// Live values
static volatile int16_t angleL = ANG_MIN;
static volatile int16_t angleR = ANG_MIN;
static volatile int16_t lastDrawnAngleL = INT16_MIN;
static volatile int16_t lastDrawnAngleR = INT16_MIN;
static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0; // 0=Day, 2=NVG
static uint8_t          lastNeedleMode = 0xFF;
static volatile bool    needsFullFlush = true;

static unsigned long    lastDrawTime = 0;
static TaskHandle_t     tftTaskHandle = nullptr;

// --- DMA fence ---
static bool dmaBusy = false;
static inline void waitDMADone() { if (dmaBusy) { tft.waitDMA(); dmaBusy = false; } }

// --- Dirty-rect utils ---
struct Rect { int16_t x = 0, y = 0, w = 0, h = 0; };
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
static void onHydLeftChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, ANG_MIN, ANG_MAX);
    if (a != angleL) { angleL = a; gaugeDirty = true; }
}
static void onHydRightChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, ANG_MIN, ANG_MAX);
    if (a != angleR) { angleR = a; gaugeDirty = true; }
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
static void HydPressureGauge_draw(bool force = false, bool blocking = false)
{
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();

    int16_t l = angleL; if (l < ANG_MIN) l = ANG_MIN; else if (l > ANG_MAX) l = ANG_MAX;
    int16_t r = angleR; if (r < ANG_MIN) r = ANG_MIN; else if (r > ANG_MAX) r = ANG_MAX;

    const bool stateChanged = gaugeDirty
        || (l != lastDrawnAngleL)
        || (r != lastDrawnAngleR)
        || needsFullFlush;
    if (!stateChanged) return;

    // Change 9/2/25
    // if (!force && (now - lastDrawTime < HYD_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;
    if (!force && !needsFullFlush && (now - lastDrawTime < HYD_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    gaugeDirty = false;

    // Added 9/2/25
    if (needsFullFlush) waitDMADone();

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_HYDPRESS_DRAW);
#endif

    // Assets
    const uint16_t* bg = (currentLightingMode == 0) ? bgCache[0] : bgCache[1];
    const uint16_t* n1 = (currentLightingMode == 0) ? hydPressNeedle1 : hydPressNeedle1NVG;
    const uint16_t* n2 = (currentLightingMode == 0) ? hydPressNeedle2 : hydPressNeedle2NVG;

    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleL_spr, n1);
        buildNeedle(needleR_spr, n2);
        lastNeedleMode = currentLightingMode;
    }

    // Dirty rect: union of old/new for both needles
    Rect dirty = needsFullFlush ? Rect{ 0,0,SCREEN_W,SCREEN_H } : Rect{};
    if (!needsFullFlush) {
        if (lastDrawnAngleL == INT16_MIN || lastDrawnAngleR == INT16_MIN) {
            dirty = { 0,0,SCREEN_W,SCREEN_H };
        }
        else {
            Rect lOld = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)lastDrawnAngleL);
            Rect lNew = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)l);
            Rect rOld = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)lastDrawnAngleR);
            Rect rNew = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H, NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)r);
            dirty = rectUnion(rectUnion(lOld, lNew), rectUnion(rOld, rNew));
        }
    }

    // Restore BG only in dirty
    blitBGRectToFrame(bg, dirty.x, dirty.y, dirty.w, dirty.h);

    // Compose needles
    frameSpr.setClipRect(dirty.x, dirty.y, dirty.w, dirty.h);
    needleL_spr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)l, 1.0f, 1.0f, TRANSPARENT_KEY);
    needleR_spr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)r, 1.0f, 1.0f, TRANSPARENT_KEY);
    frameSpr.clearClipRect();

    // Flush
    const uint16_t* buf = (const uint16_t*)frameSpr.getBuffer();
    
    // Change 9/2/25
    // flushRectToDisplay(buf, dirty, blocking);
    flushRectToDisplay(buf, dirty, needsFullFlush || blocking);

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_HYDPRESS_DRAW);
#endif

    lastDrawnAngleL = l;
    lastDrawnAngleR = r;
    needsFullFlush = false;
}

// --- Task ---
static void HydPressureGauge_task(void*) {
    for (;;) {
        HydPressureGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void HydPressureGauge_init()
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
    std::memcpy(bgCache[0], hydPressBackground, FRAME_BYTES);
    std::memcpy(bgCache[1], hydPressBackgroundNVG, FRAME_BYTES);

    tft.init();
    tft.setColorDepth(COLOR_DEPTH_HYD_PRESS);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // Compose sprite (PSRAM)
    frameSpr.setColorDepth(COLOR_DEPTH_HYD_PRESS);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    // Needles
    needleL_spr.setColorDepth(COLOR_DEPTH_HYD_PRESS);
    needleL_spr.createSprite(NEEDLE_W, NEEDLE_H);
    needleL_spr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleL_spr, hydPressNeedle1);

    needleR_spr.setColorDepth(COLOR_DEPTH_HYD_PRESS);
    needleR_spr.createSprite(NEEDLE_W, NEEDLE_H);
    needleR_spr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleR_spr, hydPressNeedle2);

    // DCS-BIOS
    subscribeToLedChange("HYD_IND_LEFT", onHydLeftChange);
    subscribeToLedChange("HYD_IND_RIGHT", onHydRightChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    // First paint
    needsFullFlush = true; gaugeDirty = true;
    HydPressureGauge_draw(true, true);

    // Optional BIT
    HydPressureGauge_bitTest();

#if RUN_HYD_PRESSURE_GAUGE_AS_TASK
    xTaskCreatePinnedToCore(HydPressureGauge_task, "HydPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, HYD_CPU_CORE);
#endif

    debugPrintln("✅ Hydraulic Pressure Gauge (dirty-rect DMA) initialized");
}

void HydPressureGauge_loop() {
#if !RUN_HYD_PRESSURE_GAUGE_AS_TASK
    HydPressureGauge_draw(false, false);
#endif
}

void HydPressureGauge_notifyMissionStart() { needsFullFlush = true; gaugeDirty = true; }

// Visual self-test; blocking flush
void HydPressureGauge_bitTest() {
    int16_t originalL = angleL, originalR = angleR;
    const int STEP = 5, DELAY = 2;
    for (int i = 0; i <= 320; i += STEP) {
        angleL = map(i, 0, 320, ANG_MIN, ANG_MAX);
        angleR = map(i, 0, 320, ANG_MAX, ANG_MIN);
        gaugeDirty = true; HydPressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 320; i >= 0; i -= STEP) {
        angleL = map(i, 0, 320, ANG_MIN, ANG_MAX);
        angleR = map(i, 0, 320, ANG_MAX, ANG_MIN);
        gaugeDirty = true; HydPressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    angleL = originalL; angleR = originalR;
    needsFullFlush = true; gaugeDirty = true; HydPressureGauge_draw(true, true);
}

void HydPressureGauge_deinit() {
    waitDMADone();
    if (tftTaskHandle) { vTaskDelete(tftTaskHandle); tftTaskHandle = nullptr; }

    needleL_spr.deleteSprite();
    needleR_spr.deleteSprite();
    frameSpr.deleteSprite();

    for (int i = 0; i < 2; ++i) {
        if (dmaBounce[i]) { heap_caps_free(dmaBounce[i]); dmaBounce[i] = nullptr; }
        if (bgCache[i]) { heap_caps_free(bgCache[i]);   bgCache[i] = nullptr; }
    }
}