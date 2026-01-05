// TFT_Gauges_RadarAlt.cpp - CockpitOS Radar Altimeter (LovyanGFX, ST77916/61 @ 360×360)
// Dirty-rect compose + region DMA flush (PSRAM sprites, DMA-safe)

#include "../Globals.h"

#if defined(HAS_CUSTOM_RIGHT) && (defined(ESP_FAMILY_S3) || defined(ESP_FAMILY_S2)) && (defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1))

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "includes/TFT_Gauges_RadarAlt.h"

// Load Library
#if !__has_include(<LovyanGFX.hpp>)
#error "❌ Missing LovyanGFX.hpp — Please install LovyanGFX library: https://github.com/lovyan03/LovyanGFX"
#endif

#include <LovyanGFX.hpp>
#include <cstring>
#include <cmath>
#include <algorithm>

#if defined(HAS_CUSTOM_RIGHT)
    REGISTER_PANEL(TFTRadarAlt, nullptr, nullptr, RadarAlt_init, RadarAlt_loop, nullptr, 100);
#endif

#define MAX_MEMORY_TFT 32
#define RADARALT_DRAW_MIN_INTERVAL_MS 13
#define RUN_RADARALT_AS_TASK 1
#define BACKLIGHT_LABEL "INST_PNL_DIMMER"
#define COLOR_DEPTH_RADARALT 16

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// Select core (For this gauge, we always use core 0 for all devices)
#if defined(ESP_FAMILY_S3)
#define RA_CPU_CORE 0
#else
#define RA_CPU_CORE 0
#endif

// --- Pins ---


#define RADARALT_CS_PIN   PIN(10)    // Chip Select (Blue)
#define RADARALT_DC_PIN   PIN(11)    // Data/Command (Green)
#define RADARALT_MOSI_PIN PIN(13)    // SDA (Yellow)
#define RADARALT_SCLK_PIN PIN(14)    // SCL (Orange)
#define RADARALT_RST_PIN  -1
#define RADARALT_MISO_PIN -1


// --- Assets ---
#include "Assets/RadarAltimeter/radarAltBackground.h"
#include "Assets/RadarAltimeter/radarAltNeedle.h"
#include "Assets/RadarAltimeter/radarAltMinHeightPointer.h"
#include "Assets/RadarAltimeter/radarAltMinHeightPointerNVG.h"
#include "Assets/RadarAltimeter/radarAltGreenLamp.h"
#include "Assets/RadarAltimeter/radarAltLowAltWarning.h"
#include "Assets/RadarAltimeter/radarAltOffFlag.h"
#include "Assets/RadarAltimeter/radarAltOffFlagNVG.h"
#include "Assets/RadarAltimeter/radarAltBackgroundNVG.h"
#include "Assets/RadarAltimeter/radarAltNeedleNVG.h"

// --- Misc ---
static constexpr bool shared_bus = false;
static constexpr bool use_lock = false;
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;  // color not present in assets
static constexpr uint16_t NVG_THRESHOLD = 6553;
static constexpr int16_t SCREEN_W = 360, SCREEN_H = 360;
static constexpr int16_t LOWALT_X = 95, LOWALT_Y = 158;
static constexpr int16_t GREEN_X = 229, GREEN_Y = 158;
static constexpr int16_t OFF_X = 152, OFF_Y = 254;
static constexpr int16_t CENTER_X = 180, CENTER_Y = 180;
static uint16_t* bgCache[2] = { nullptr, nullptr };  // PSRAM day/NVG

// Bounce stripes (internal RAM, DMA-capable)
static constexpr int    STRIPE_H = MAX_MEMORY_TFT;
static constexpr size_t STRIPE_BYTES = size_t(SCREEN_W) * STRIPE_H * sizeof(uint16_t);
static uint16_t* dmaBounce[2] = { nullptr, nullptr };

// Angles
static constexpr int16_t RA_ANGLE_MIN = -17;
static constexpr int16_t RA_ANGLE_MAX = 325;
static constexpr int16_t MHP_ANGLE_MIN = -10;
static constexpr int16_t MHP_ANGLE_MAX = 325;

// --- Sanity checks ---
static_assert(SCREEN_W > 0 && SCREEN_H > 0, "bad dims");
static_assert(STRIPE_H > 0 && STRIPE_H <= SCREEN_H, "bad STRIPE_H");

// --- Panel binding ---
class LGFX_RadarAlt : public lgfx::LGFX_Device {
    lgfx::Bus_SPI       _bus;
    lgfx::Panel_ST77961 _panel;
public:
    LGFX_RadarAlt() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI3_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 0;
            cfg.spi_3wire = false;
            cfg.use_lock = use_lock;
            cfg.dma_channel = SPI_DMA_CH_AUTO; // We can also use SPI_DMA_CH_AUTO
            cfg.pin_mosi = RADARALT_MOSI_PIN;
            cfg.pin_miso = RADARALT_MISO_PIN;
            cfg.pin_sclk = RADARALT_SCLK_PIN;
            cfg.pin_dc = RADARALT_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.readable = false;
            pcfg.pin_cs = RADARALT_CS_PIN;
            pcfg.pin_rst = RADARALT_RST_PIN;
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
static LGFX_RadarAlt tft;

// Compose target (PSRAM sprite)
static lgfx::LGFX_Sprite frameSpr(&tft);

// Static sprites
static lgfx::LGFX_Sprite needle(&tft);
static lgfx::LGFX_Sprite pointerSpr(&tft);
static lgfx::LGFX_Sprite lowAltSpr(&tft);
static lgfx::LGFX_Sprite greenLampSpr(&tft);
static lgfx::LGFX_Sprite offFlagSpr(&tft);

// Live values
static volatile int16_t angleRA = 0;
static volatile int16_t angleMHP = 0;
static volatile bool    lowAltOn = false;
static volatile bool    greenOn = false;
static volatile bool    offFlag = false;

static volatile int16_t lastDrawnRA = INT16_MIN;
static volatile int16_t lastDrawnMHP = INT16_MIN;
static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0;   // 0=Day, 2=NVG

static uint8_t  lastNeedleMode = 0xFF;
static uint8_t  lastPointerMode = 0xFF;
static uint8_t  lastOffFlagMode = 0xFF;

static bool     lastLowAltOn = false;
static bool     lastGreenOn = false;
static bool     lastOffOn = false;

static unsigned long lastDrawTime = 0;
static TaskHandle_t  tftTaskHandle = nullptr;

// --- DMA fence ---
static bool dmaBusy = false;
static inline void waitDMADone() {
    if (dmaBusy) {
        tft.waitDMA();   // ensure last DMA completed
        dmaBusy = false; // bus will be released by the caller that started it
    }
}

// Full-frame force flag (first frame, mode change, BIT start)
static volatile bool needsFullFlush = true;

// --- Sprite helpers ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, 76, 173, img);
}
static void buildPointer(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, 23, 180, img);
}
static void buildLamp(lgfx::LGFX_Sprite& spr, const uint16_t* img, int w, int h) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, w, h, img);
}

// --- DCS-BIOS callbacks ---
static void onRadarAltAngle(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, RA_ANGLE_MIN, RA_ANGLE_MAX);
    if (a != angleRA) { angleRA = a; gaugeDirty = true; }
}
static void onMinHeightAngle(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, MHP_ANGLE_MIN, MHP_ANGLE_MAX);
    if (a != angleMHP) { angleMHP = a; gaugeDirty = true; }
}
static void onLowAltLamp(const char*, uint16_t v, uint16_t) {
    bool on = (v != 0);
    if (on != lowAltOn) { lowAltOn = on; gaugeDirty = true; }
}
static void onGreenLamp(const char*, uint16_t v, uint16_t) {
    bool on = (v != 0);
    if (on != greenOn) { greenOn = on; gaugeDirty = true; }
}
static void onOffFlag(const char*, uint16_t v, uint16_t) {
    bool on = (v != 0);
    if (on != offFlag) { offFlag = on; gaugeDirty = true; }
}
static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode != currentLightingMode) {
        currentLightingMode = mode;
        gaugeDirty = true;
        needsFullFlush = true;
    }
}

// ----------------- Dirty-rect utilities -----------------

/*
struct Rect {
    int16_t x = 0, y = 0, w = 0, h = 0;
};
*/

struct Rect { int16_t x, y, w, h; };

static inline bool rectEmpty(const Rect& r) { return r.w <= 0 || r.h <= 0; }
static inline Rect rectClamp(const Rect& r) {
    int16_t x = r.x, y = r.y, w = r.w, h = r.h;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    return { x,y,w,h };
}
static inline Rect rectUnion(const Rect& a, const Rect& b) {
    if (rectEmpty(a)) return b;
    if (rectEmpty(b)) return a;
    int16_t x1 = std::min(a.x, b.x);
    int16_t y1 = std::min(a.y, b.y);
    int16_t x2 = std::max<int16_t>(a.x + a.w, b.x + b.w);
    int16_t y2 = std::max<int16_t>(a.y + a.h, b.y + b.h);
    return rectClamp({ x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1) });
}
static inline Rect rectPad(const Rect& r, int16_t px) {
    return rectClamp({ (int16_t)(r.x - px), (int16_t)(r.y - px), (int16_t)(r.w + 2 * px), (int16_t)(r.h + 2 * px) });
}

// AABB for rotated sprite (pivot inside sprite coordinates)
static Rect rotatedAABB(int cx, int cy, int w, int h, int pivotX, int pivotY, float deg) {
    const float rad = deg * (float)M_PI / 180.0f;
    float s = sinf(rad), c = cosf(rad);
    // corners relative to pivot
    const float xs[4] = { -pivotX, (float)w - pivotX, (float)w - pivotX, -pivotX };
    const float ys[4] = { -pivotY, -pivotY, (float)h - pivotY, (float)h - pivotY };
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
    return rectClamp(rectPad(r, 2)); // small pad for AA edges
}

// Copy a BG sub-rectangle from cache into frame sprite buffer
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
    // pack rows into bounce[0]
    for (int row = 0; row < h0; ++row) {
        std::memcpy(dmaBounce[0] + row * r.w, src + (y + row) * pitch + r.x, size_t(r.w) * sizeof(uint16_t));
    }
    tft.setAddrWindow(r.x, y, r.w, h0);
    tft.pushPixelsDMA(dmaBounce[0], uint32_t(r.w) * h0);
    if (!blocking) dmaBusy = true;

    int bb = 1;
    for (y += h0; y < r.y + r.h; y += lines_per, bb ^= 1) {
        int h = (y + lines_per <= r.y + r.h) ? lines_per : (r.y + r.h - y);

        // overlap pack with previous DMA
        for (int row = 0; row < h; ++row) {
            std::memcpy(dmaBounce[bb] + row * r.w, src + (y + row) * pitch + r.x, size_t(r.w) * sizeof(uint16_t));
        }

        tft.waitDMA();
        tft.setAddrWindow(r.x, y, r.w, h);
        tft.pushPixelsDMA(dmaBounce[bb], uint32_t(r.w) * h);

        if (blocking) {
            tft.waitDMA();
        }
    }

    if (blocking) {
        tft.waitDMA();
        dmaBusy = false;
    }

    tft.endWrite();
}

static inline void flushFrameToDisplay(const uint16_t* src, bool blocking) {
    flushRectToDisplay(src, { 0,0,SCREEN_W,SCREEN_H }, blocking);
}

// ----------------- Draw -----------------
static void RadarAlt_draw(bool force = false, bool blocking = false)
{
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();

    int16_t ra = angleRA;  if (ra < RA_ANGLE_MIN) ra = RA_ANGLE_MIN; else if (ra > RA_ANGLE_MAX) ra = RA_ANGLE_MAX;
    int16_t mhp = angleMHP; if (mhp < MHP_ANGLE_MIN) mhp = MHP_ANGLE_MIN; else if (mhp > MHP_ANGLE_MAX) mhp = MHP_ANGLE_MAX;

    const bool stateChanged = gaugeDirty || (ra != lastDrawnRA) || (mhp != lastDrawnMHP)
        || (lowAltOn != lastLowAltOn) || (greenOn != lastGreenOn) || (offFlag != lastOffOn)
        || needsFullFlush;

    if (!stateChanged) return;

    // Change 9/2/25
    // if (!force && (now - lastDrawTime < RADARALT_DRAW_MIN_INTERVAL_MS)) return;
    if (!force && !needsFullFlush && (now - lastDrawTime < RADARALT_DRAW_MIN_INTERVAL_MS)) return;

    const uint32_t t_frame0 = micros();

    lastDrawTime = now;
    gaugeDirty = false;

    // Added 9/2/25
    if (needsFullFlush) waitDMADone();

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_RADARALT_DRAW);
#endif

    // Select assets
    const uint16_t* nImg = (currentLightingMode == 0) ? radarAltNeedle : radarAltNeedleNVG;
    const uint16_t* ptrImg = (currentLightingMode == 0) ? radarAltMinHeightPointer : radarAltMinHeightPointerNVG;
    const uint16_t* flagImg = (currentLightingMode == 0) ? radarAltOffFlag : radarAltOffFlagNVG;
    const uint16_t* bg = bgCache[(currentLightingMode == 0) ? 0u : 1u];

    // Rebuild sprites if lighting mode changed
    if (lastNeedleMode != currentLightingMode) { buildNeedle(needle, nImg);                lastNeedleMode = currentLightingMode; }
    if (lastPointerMode != currentLightingMode) { buildPointer(pointerSpr, ptrImg);         lastPointerMode = currentLightingMode; }
    if (lastOffFlagMode != currentLightingMode) { buildLamp(offFlagSpr, flagImg, 51, 19);  lastOffFlagMode = currentLightingMode; }

    // Compute dirty rect
    Rect dirty = needsFullFlush ? Rect{ 0,0,SCREEN_W,SCREEN_H } : Rect{};

    // needle & pointer: union old/new AABBs
    if (!needsFullFlush) {
        Rect nOld = rotatedAABB(CENTER_X, CENTER_Y, 76, 173, 38, 134, (float)lastDrawnRA);
        Rect nNew = rotatedAABB(CENTER_X, CENTER_Y, 76, 173, 38, 134, (float)ra);
        Rect pOld = rotatedAABB(CENTER_X, CENTER_Y, 23, 180, 12, 180, (float)lastDrawnMHP);
        Rect pNew = rotatedAABB(CENTER_X, CENTER_Y, 23, 180, 12, 180, (float)mhp);
        dirty = rectUnion(dirty, rectUnion(nOld, nNew));
        dirty = rectUnion(dirty, rectUnion(pOld, pNew));
    }

    // Lamps: always include small rects if ON or state changed (cheap and safe)
    auto lampRect = [](int x, int y) { return rectClamp(Rect{ x, y, 34, 34 }); };
    auto offRect = [](int x, int y) { return rectClamp(Rect{ x, y, 51, 19 }); };
    if (needsFullFlush || greenOn || greenOn != lastGreenOn)   dirty = rectUnion(dirty, lampRect(GREEN_X, GREEN_Y));
    if (needsFullFlush || lowAltOn || lowAltOn != lastLowAltOn)dirty = rectUnion(dirty, lampRect(LOWALT_X, LOWALT_Y));
    if (needsFullFlush || offFlag || offFlag != lastOffOn)   dirty = rectUnion(dirty, offRect(OFF_X, OFF_Y));

    // Restore BG only inside dirty rect
    blitBGRectToFrame(bg, dirty.x, dirty.y, dirty.w, dirty.h);

    // Clip composition to dirty rect so we don't paint outside flushed area
    frameSpr.setClipRect(dirty.x, dirty.y, dirty.w, dirty.h);

    // Overlays
    if (greenOn)  greenLampSpr.pushRotateZoom(&frameSpr, GREEN_X, GREEN_Y, 0.0f, 1.0f, 1.0f, TRANSPARENT_KEY);
    if (lowAltOn) lowAltSpr.pushRotateZoom(&frameSpr, LOWALT_X, LOWALT_Y, 0.0f, 1.0f, 1.0f, TRANSPARENT_KEY);
    if (offFlag)  offFlagSpr.pushRotateZoom(&frameSpr, OFF_X, OFF_Y, 0.0f, 1.0f, 1.0f, TRANSPARENT_KEY);

    // Pointers
    pointerSpr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)mhp, 1.0f, 1.0f, TRANSPARENT_KEY);
    needle.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)ra, 1.0f, 1.0f, TRANSPARENT_KEY);

    frameSpr.clearClipRect();

    // Flush region
    const uint16_t* buf = (const uint16_t*)frameSpr.getBuffer();

    // Change 9/2/25
    // flushRectToDisplay(buf, dirty, blocking);
    flushRectToDisplay(buf, dirty, needsFullFlush || blocking);


#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_RADARALT_DRAW);
#endif

    // Update 'last' state
    lastDrawnRA = ra;
    lastDrawnMHP = mhp;
    lastLowAltOn = lowAltOn;
    lastGreenOn = greenOn;
    lastOffOn = offFlag;
    needsFullFlush = false;

    (void)t_frame0; // (keep for optional ad-hoc profiling)
}

// --- Task ---
static void RadarAlt_task(void*)
{
    for (;;) {
        RadarAlt_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void RadarAlt_init()
{
    if (!initPSRAM()) {
        debugPrintln("❌ No PSRAM detected! RadarAlt disabled.");
        while (1) vTaskDelay(1000);
    }

    // Bounce buffers (internal RAM, DMA-capable)
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
    std::memcpy(bgCache[0], radarAltBackground, FRAME_BYTES);
    std::memcpy(bgCache[1], radarAltBackgroundNVG, FRAME_BYTES);

    tft.init();
    tft.setColorDepth(COLOR_DEPTH_RADARALT);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // Compose sprite (PSRAM)
    frameSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    // Element sprites
    needle.setColorDepth(COLOR_DEPTH_RADARALT);
    needle.createSprite(76, 173);
    needle.setPivot(38, 134);
    buildNeedle(needle, radarAltNeedle);

    pointerSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    pointerSpr.createSprite(23, 180);
    pointerSpr.setPivot(12, 180);
    buildPointer(pointerSpr, radarAltMinHeightPointer);

    lowAltSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    lowAltSpr.createSprite(34, 34);
    lowAltSpr.setPivot(0, 0);
    buildLamp(lowAltSpr, radarAltLowAltWarning, 34, 34);

    greenLampSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    greenLampSpr.createSprite(34, 34);
    greenLampSpr.setPivot(0, 0);
    buildLamp(greenLampSpr, radarAltGreenLamp, 34, 34);

    offFlagSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    offFlagSpr.createSprite(51, 19);
    offFlagSpr.setPivot(0, 0);
    buildLamp(offFlagSpr, radarAltOffFlag, 51, 19);

    // DCS-BIOS bindings
    subscribeToLedChange("RADALT_ALT_PTR", onRadarAltAngle);
    subscribeToLedChange("RADALT_MIN_HEIGHT_PTR", onMinHeightAngle);
    subscribeToLedChange("LOW_ALT_WARN_LT", onLowAltLamp);
    subscribeToLedChange("RADALT_GREEN_LAMP", onGreenLamp);
    subscribeToLedChange("RADALT_OFF_FLAG", onOffFlag);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    // First frame: restore full BG and flush once so gauge is visible before sweeps/loop
    needsFullFlush = true;
    gaugeDirty = true;
    RadarAlt_draw(true, true);

    // Optional BIT (will also force full once at start)
    RadarAlt_bitTest();  // <-- call on demand if you prefer

#if RUN_RADARALT_AS_TASK
    xTaskCreatePinnedToCore(RadarAlt_task, "RadarAltTask", 4096, nullptr, 2, &tftTaskHandle, RA_CPU_CORE);
#endif

    debugPrintln("✅ Radar Altimeter (LovyanGFX, dirty-rect DMA) initialized");
}

void RadarAlt_loop()
{
#if !RUN_RADARALT_AS_TASK
    RadarAlt_draw(false, false);
#endif
}

void RadarAlt_notifyMissionStart() {
    needsFullFlush = true;
    gaugeDirty = true;
}

// Visual self-test; do a full-frame show at start
void RadarAlt_bitTest()
{
    int16_t savRA = angleRA, savMHP = angleMHP;
    bool savLow = lowAltOn, savGreen = greenOn, savOff = offFlag;

    const int STEP = 10, DELAY = 2;

    for (int i = 0; i <= 320; i += STEP) {
        angleRA = i; angleMHP = i; gaugeDirty = true;
        RadarAlt_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 320; i >= 0; i -= STEP) {
        angleRA = i; angleMHP = i; gaugeDirty = true;
        RadarAlt_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    lowAltOn = true;  greenOn = false; offFlag = false;
    gaugeDirty = true; RadarAlt_draw(true, true); vTaskDelay(pdMS_TO_TICKS(200));

    lowAltOn = false; greenOn = true;  offFlag = false;
    gaugeDirty = true; RadarAlt_draw(true, true); vTaskDelay(pdMS_TO_TICKS(200));

    lowAltOn = false; greenOn = false; offFlag = true;
    gaugeDirty = true; RadarAlt_draw(true, true); vTaskDelay(pdMS_TO_TICKS(200));

    lowAltOn = true;  greenOn = true;  offFlag = true;
    gaugeDirty = true; RadarAlt_draw(true, true); vTaskDelay(pdMS_TO_TICKS(400));

    // Restore
    angleRA = savRA; angleMHP = savMHP;
    lowAltOn = savLow; greenOn = savGreen; offFlag = savOff;
    needsFullFlush = true;
    gaugeDirty = true; RadarAlt_draw(true, true);
}

void RadarAlt_deinit()
{
    if (tftTaskHandle) { vTaskDelete(tftTaskHandle); tftTaskHandle = nullptr; }

    waitDMADone(); dmaBusy = false;

    needle.deleteSprite();
    pointerSpr.deleteSprite();
    lowAltSpr.deleteSprite();
    greenLampSpr.deleteSprite();
    offFlagSpr.deleteSprite();
    frameSpr.deleteSprite();

    for (int i = 0; i < 2; ++i) {
        if (dmaBounce[i]) { heap_caps_free(dmaBounce[i]); dmaBounce[i] = nullptr; }
    }
    for (int i = 0; i < 2; ++i) {
        if (bgCache[i]) { heap_caps_free(bgCache[i]); bgCache[i] = nullptr; }
    }
}
#else
#warning "Radar Altimeter requires ESP32-S2 or ESP32-S3"
#endif