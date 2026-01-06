// =============================================================================
// TFT_Display_CMWS.cpp — CockpitOS CMWS Threat Ring Display (LOW-MEM, DIRECT-DRAW)
// =============================================================================
// AH-64D Apache Countermeasures Warning System Display
//
// Hardware: IdeasPark ST7789 170×320 TFT (320×170 in landscape after rotation)
// CRITICAL: TFT_RST must be GPIO 4 (not -1) for this display!
//
// DESIGN GOALS (ESP32 Classic: no PSRAM, tight SRAM):
//   1) No full-frame sprites.
//   2) Precompute geometry once (ticks + arrows + AABBs).
//   3) Incremental redraw that is CORRECT: no "erase with black" unless
//      we also restore whatever was under (ticks + other arrows).
//   4) Deterministic: snapshot state under a critical section.
//   5) Moderate complexity: rectangular region restore + redraw layers.
//
// LAYERS:
//   Base layer: black background + tick marks (excluding angles occupied by small arrows).
//   Dynamic layer: 8 small arrows + 4 large arrows + D/R letters + inventory/BIT text.
//
// Incremental update strategy:
//   For each changed element, redraw its AABB region:
//     - clear region to black
//     - redraw ticks that intersect region
//     - redraw ANY arrows (small/large) whose AABB intersects region
//     - redraw D/R if region intersects their fixed rects
//   Inventory/BIT text uses dedicated rect clears.
//
// =============================================================================

#include "../Globals.h"

#if defined(HAS_CMWS_DISPLAY) && defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1)

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "includes/TFT_Display_CMWS.h"

#if !__has_include(<LovyanGFX.hpp>)
#error "❌ Missing LovyanGFX.hpp — Please install LovyanGFX library"
#endif

#include <LovyanGFX.hpp>
#include <cstring>
#include <cmath>

// =============================================================================
// PANEL REGISTRATION
// =============================================================================
#if defined(HAS_CMWS_DISPLAY)
    REGISTER_PANEL(TFTCmws, nullptr, nullptr, CMWSDisplay_init, CMWSDisplay_loop, nullptr, 100);
#endif


// =============================================================================
// FONTS
// =============================================================================
#include "Assets/Fonts/Doto_Regular28pt7b.h"
static const GFXfont* const FONT_DOTO_32 = &Doto_Regular28pt7b;

// =============================================================================
// CONFIG
// =============================================================================
static constexpr uint32_t CMWS_REFRESH_INTERVAL_MS = 33;   // ~30 FPS max

static constexpr bool     RUN_AS_TASK     = true;
static constexpr uint16_t TASK_STACK_SIZE = 4096;
static constexpr uint8_t  TASK_PRIORITY   = 2;
static constexpr uint8_t  CPU_CORE        = 0;

#define RUN_BIT_TEST_ON_INIT 1

// =============================================================================
// PINS (ESP32 Classic)
// =============================================================================
#if defined(HAS_CMWS_DISPLAY) && defined(ESP_FAMILY_CLASSIC)
    static constexpr int8_t PIN_MOSI = PIN(23);
    static constexpr int8_t PIN_SCLK = PIN(18);
    static constexpr int8_t PIN_CS   = PIN(15);
    static constexpr int8_t PIN_DC   = PIN(2);
    static constexpr int8_t PIN_RST  = PIN(4);   // MUST BE GPIO4 for this module
    static constexpr int8_t PIN_BLK  = PIN(32);
#else
    static constexpr int8_t PIN_MOSI = -1;
    static constexpr int8_t PIN_SCLK = -1;
    static constexpr int8_t PIN_CS   = -1;
    static constexpr int8_t PIN_DC   = -1;
    static constexpr int8_t PIN_RST  = -1;
    static constexpr int8_t PIN_BLK  = -1;
#endif

// =============================================================================
// GEOMETRY
// =============================================================================
static constexpr int16_t SCREEN_W = 320;
static constexpr int16_t SCREEN_H = 170;

static constexpr int16_t RING_CX = 235;
static constexpr int16_t RING_CY = 85;

static constexpr int16_t TICK_INNER_R = 66;
static constexpr int16_t TICK_OUTER_R = 76;

static constexpr int     TICK_COUNT = 24;    // 15° increments

static constexpr int16_t TEXT_X       = 5;
static constexpr int16_t TEXT_LINE1   = 40;
static constexpr int16_t TEXT_LINE2   = 95;
static constexpr int16_t TEXT_CLEAR_W = 150;
// static constexpr int16_t TEXT_CLEAR_H = 50;
static constexpr int16_t TEXT_CLEAR_H = 40; 

static constexpr int16_t DR_OFFSET = 25;

// =============================================================================
// ARROW SHAPE
// =============================================================================
static constexpr float LARGE_TIP_Y       = 30.0f;
static constexpr float LARGE_TIP_BASE_Y  = 11.0f;
static constexpr float LARGE_BODY_BASE_Y = 0.0f;
static constexpr float LARGE_TIP_HALF_W  = 16.0f;
static constexpr float LARGE_BODY_HALF_W = 8.5f;

static constexpr float SMALL_ARROW_SCALE = 0.5f;

// =============================================================================
// ARROW POSITIONS
// =============================================================================
static constexpr int LARGE_ARROW_COUNT = 4;
static constexpr int LARGE_ARROW_ANGLES[LARGE_ARROW_COUNT] = { 45, 135, 225, 315 };

// Small arrows: 8 positions (every 45°) INSIDE tick band
static constexpr int SMALL_ARROW_COUNT = 8;
static constexpr int SMALL_ARROW_ANGLES[SMALL_ARROW_COUNT] = { 0, 45, 90, 135, 180, 225, 270, 315 };

// Small arrows sit inside the same boundary band as ticks
// static constexpr int16_t SMALL_ARROW_RADIUS = (TICK_INNER_R + TICK_OUTER_R) / 2;
static constexpr int16_t SMALL_ARROW_RADIUS = TICK_OUTER_R - 10;

// Large radius computed at init
static int16_t g_largeArrowRadius = 0;

// =============================================================================
// COLORS
// =============================================================================
static constexpr uint16_t COL_BLACK     = TFT_Colors::BLACK;
static constexpr uint16_t COL_AMBER_BRT = TFT_Colors::AMBER_BRT;
static constexpr uint16_t COL_AMBER_DIM = TFT_Colors::AMBER_DIM;
static constexpr uint16_t COL_GREEN     = TFT_Colors::GREEN;

// =============================================================================
// STATE
// =============================================================================
enum class ElemState : uint8_t { OFF = 0, DIM = 1, BRT = 2 };

static volatile bool g_deviceOff = false;

static inline ElemState computeStateFromBits(bool brt, bool dim) {
    if (brt) return ElemState::BRT;
    if (dim) return ElemState::DIM;
    return ElemState::OFF;
}

static inline uint16_t colorFor(ElemState s) {
    return (s == ElemState::BRT) ? COL_AMBER_BRT : COL_AMBER_DIM;
}

// =============================================================================
// PRECOMPUTED CACHES
// =============================================================================
struct ArrowCache {
    TFT_Point tip;
    TFT_Point tipBaseL;
    TFT_Point tipBaseR;
    TFT_Point bodyTopL;
    TFT_Point bodyTopR;
    TFT_Point bodyBotL;
    TFT_Point bodyBotR;
};
struct TickCache {
    TFT_Point inner;
    TFT_Point outer;
};
struct RectI16 {
    int16_t x, y, w, h;
};

static ArrowCache g_largeArrows[LARGE_ARROW_COUNT];
static ArrowCache g_smallArrows[SMALL_ARROW_COUNT];
static TickCache  g_ticks[TICK_COUNT];

static RectI16 g_largeAabb[LARGE_ARROW_COUNT];
static RectI16 g_smallAabb[SMALL_ARROW_COUNT];

static RectI16 g_dRect; // bounding rect for "D"
static RectI16 g_rRect; // bounding rect for "R"

// =============================================================================
// SPI / PANEL
// =============================================================================
#if defined(ESP_FAMILY_CLASSIC)
    static constexpr spi_host_device_t CMWS_SPI_HOST = VSPI_HOST;
#else
    static constexpr spi_host_device_t CMWS_SPI_HOST = SPI2_HOST;
#endif

class LGFX_CMWS final : public lgfx::LGFX_Device {
    lgfx::Bus_SPI       _bus;
    lgfx::Panel_ST7789  _panel;
    lgfx::Light_PWM     _light;

public:
    LGFX_CMWS() {
        {
            auto cfg = _bus.config();
            cfg.spi_host    = CMWS_SPI_HOST;
            cfg.spi_mode    = 0;
            cfg.freq_write  = 80000000;     // if corruption: drop to 60000000 or 40000000
            cfg.freq_read   = 16000000;
            cfg.spi_3wire   = false;
            cfg.use_lock    = false;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_mosi    = PIN_MOSI;
            cfg.pin_miso    = -1;
            cfg.pin_sclk    = PIN_SCLK;
            cfg.pin_dc      = PIN_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs          = PIN_CS;
            cfg.pin_rst         = PIN_RST;
            cfg.pin_busy        = -1;

            cfg.memory_width    = 240;
            cfg.memory_height   = 320;
            cfg.panel_width     = 170;
            cfg.panel_height    = 320;
            cfg.offset_x        = 35;
            cfg.offset_y        = 0;
            cfg.offset_rotation = 0;

            cfg.readable        = false;
            cfg.bus_shared      = false;
            cfg.invert          = true;
            cfg.rgb_order       = false;
            cfg.dlen_16bit      = false;
            _panel.config(cfg);
        }
        {
            auto cfg = _light.config();
            cfg.pin_bl      = PIN_BLK;
            cfg.invert      = false;
            cfg.freq        = 12000;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        setPanel(&_panel);
    }
};

static LGFX_CMWS tft;
static TaskHandle_t taskHandle = nullptr;

// =============================================================================
// CONCURRENCY: snapshot state
// =============================================================================
static portMUX_TYPE g_stateMux = portMUX_INITIALIZER_UNLOCKED;

struct CmwsState {
    uint8_t  page;            // 0=NONE/OFF, 1=MAIN, 2=TEST
    bool     showInventory;   // true: F/C inventory, false: BIT lines

    ElemState large[LARGE_ARROW_COUNT];
    ElemState small[SMALL_ARROW_COUNT];
    ElemState dispense;
    ElemState ready;

    uint8_t  backlight;

    char flareLetter[4];
    char chaffLetter[4];
    char flareCount[8];
    char chaffCount[8];
    char bitLine1[8];
    char bitLine2[8];
};

static CmwsState g_pending;     // written by callbacks under mux
static CmwsState g_lastDrawn;   // last frame snapshot (no mux needed inside draw task)

static volatile bool g_dirty = true;
static volatile bool g_forceFull = true;
static uint32_t g_lastDrawMs = 0;

// Cached brt/dim bits for large arrows + D/R (no getMetadataValue() readback)
static volatile bool g_largeBrt[LARGE_ARROW_COUNT] = { false,false,false,false };
static volatile bool g_largeDim[LARGE_ARROW_COUNT] = { true,true,true,true };

static volatile bool g_dispBrt = false;
static volatile bool g_dispDim = true;
static volatile bool g_readyBrt = false;
static volatile bool g_readyDim = true;

// =============================================================================
// SMALL HELPERS
// =============================================================================
static inline int16_t clampI16(int32_t v, int16_t lo, int16_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return static_cast<int16_t>(v);
}

static inline RectI16 rectFromPoints(const TFT_Point* pts, int count, int16_t pad = 1) {
    int16_t minX = 32767, minY = 32767, maxX = -32768, maxY = -32768;
    for (int i = 0; i < count; ++i) {
        if (pts[i].x < minX) minX = pts[i].x;
        if (pts[i].y < minY) minY = pts[i].y;
        if (pts[i].x > maxX) maxX = pts[i].x;
        if (pts[i].y > maxY) maxY = pts[i].y;
    }
    minX = clampI16(minX - pad, 0, SCREEN_W - 1);
    minY = clampI16(minY - pad, 0, SCREEN_H - 1);
    maxX = clampI16(maxX + pad, 0, SCREEN_W - 1);
    maxY = clampI16(maxY + pad, 0, SCREEN_H - 1);
    return RectI16{ minX, minY, static_cast<int16_t>(maxX - minX + 1), static_cast<int16_t>(maxY - minY + 1) };
}

static inline bool rectIntersects(const RectI16& a, const RectI16& b) {
    return !(a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y);
}

// =============================================================================
// TRIG / VECTORS (uses your header LUT for 15° multiples)
// =============================================================================
static void computeForward(int angleDeg, float& fx, float& fy) {
    angleDeg = TFT_Trig::normalizeAngle(angleDeg);
    if ((angleDeg % 15) == 0) {
        fx = TFT_Trig::fastSin15(angleDeg);
        fy = -TFT_Trig::fastCos15(angleDeg);
    } else {
        const float rad = static_cast<float>(angleDeg) * 0.017453292f;
        fx = sinf(rad);
        fy = -cosf(rad);
    }
}

static float maxCenterRadiusForTipOnScreen(int angleDeg, float scale) {
    float fx, fy;
    computeForward(angleDeg, fx, fy);
    const float tipOffset = LARGE_TIP_Y * scale;

    float tMaxX = 1e9f;
    if (fx > 0.0001f) {
        tMaxX = (static_cast<float>(SCREEN_W - 1 - RING_CX) / fx) - tipOffset;
    } else if (fx < -0.0001f) {
        tMaxX = (static_cast<float>(0 - RING_CX) / fx) - tipOffset;
    }

    float tMaxY = 1e9f;
    if (fy > 0.0001f) {
        tMaxY = (static_cast<float>(SCREEN_H - 1 - RING_CY) / fy) - tipOffset;
    } else if (fy < -0.0001f) {
        tMaxY = (static_cast<float>(0 - RING_CY) / fy) - tipOffset;
    }

    const float tMax = (tMaxX < tMaxY) ? tMaxX : tMaxY;
    return (tMax > 0.0f) ? tMax : 0.0f;
}

static int computeLargeArrowRadiusSymmetric() {
    float r = 1e9f;
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        const float ri = maxCenterRadiusForTipOnScreen(LARGE_ARROW_ANGLES[i], 1.0f);
        if (ri < r) r = ri;
    }
    return (r > 0) ? static_cast<int>(r) : 0;
}

static void computeArrowVertices(ArrowCache& cache, int angleDeg, int16_t radius, float scale) {
    float fx, fy;
    computeForward(angleDeg, fx, fy);

    // right vector
    const float rx = fy;
    const float ry = -fx;

    // center
    const float cx = static_cast<float>(RING_CX) + fx * static_cast<float>(radius);
    const float cy = static_cast<float>(RING_CY) + fy * static_cast<float>(radius);

    const float tipY      = LARGE_TIP_Y * scale;
    const float tipBaseY  = LARGE_TIP_BASE_Y * scale;
    const float bodyBaseY = LARGE_BODY_BASE_Y * scale;
    const float tipHalfW  = LARGE_TIP_HALF_W * scale;
    const float bodyHalfW = LARGE_BODY_HALF_W * scale;

    auto toWorld = [&](float lx, float ly, int16_t& wx, int16_t& wy) {
        const float worldX = cx + (lx * rx) + (ly * fx);
        const float worldY = cy + (lx * ry) + (ly * fy);
        wx = static_cast<int16_t>(worldX >= 0 ? (worldX + 0.5f) : (worldX - 0.5f));
        wy = static_cast<int16_t>(worldY >= 0 ? (worldY + 0.5f) : (worldY - 0.5f));
    };

    toWorld(0.0f,        tipY,      cache.tip.x,      cache.tip.y);
    toWorld(-tipHalfW,   tipBaseY,  cache.tipBaseL.x, cache.tipBaseL.y);
    toWorld(+tipHalfW,   tipBaseY,  cache.tipBaseR.x, cache.tipBaseR.y);

    toWorld(-bodyHalfW,  tipBaseY,  cache.bodyTopL.x, cache.bodyTopL.y);
    toWorld(+bodyHalfW,  tipBaseY,  cache.bodyTopR.x, cache.bodyTopR.y);
    toWorld(-bodyHalfW,  bodyBaseY, cache.bodyBotL.x, cache.bodyBotL.y);
    toWorld(+bodyHalfW,  bodyBaseY, cache.bodyBotR.x, cache.bodyBotR.y);
}

static void computeTickVertices(TickCache& cache, int angleDeg) {
    float fx, fy;
    computeForward(angleDeg, fx, fy);

    cache.inner.x = static_cast<int16_t>(RING_CX + fx * TICK_INNER_R + 0.5f);
    cache.inner.y = static_cast<int16_t>(RING_CY + fy * TICK_INNER_R + 0.5f);
    cache.outer.x = static_cast<int16_t>(RING_CX + fx * TICK_OUTER_R + 0.5f);
    cache.outer.y = static_cast<int16_t>(RING_CY + fy * TICK_OUTER_R + 0.5f);
}

// =============================================================================
// DRAW PRIMITIVES (DIRECT)
// =============================================================================
static void drawArrowDirect(const ArrowCache& a, uint16_t color) {
    // Tip triangle
    tft.fillTriangle(a.tip.x, a.tip.y, a.tipBaseL.x, a.tipBaseL.y, a.tipBaseR.x, a.tipBaseR.y, color);
    // Body (2 triangles)
    tft.fillTriangle(a.bodyTopL.x, a.bodyTopL.y, a.bodyTopR.x, a.bodyTopR.y, a.bodyBotR.x, a.bodyBotR.y, color);
    tft.fillTriangle(a.bodyTopL.x, a.bodyTopL.y, a.bodyBotR.x, a.bodyBotR.y, a.bodyBotL.x, a.bodyBotL.y, color);
}

static void drawTickDirect(const TickCache& t, uint16_t color) {
    tft.drawLine(t.inner.x, t.inner.y, t.outer.x, t.outer.y, color);
}

// =============================================================================
// BASE LAYER HELPERS
// =============================================================================
static bool angleIsSmallArrow(int angleDeg) {
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) {
        if (angleDeg == SMALL_ARROW_ANGLES[i]) return true;
    }
    return false;
}

static void drawTicksIntersectingRect(const RectI16& r) {
    // ticks are at 15° increments
    for (int i = 0; i < TICK_COUNT; ++i) {
        const int angle = (i * 360) / TICK_COUNT;

        // ticks replaced by small arrows (including diagonals)
        if (angleIsSmallArrow(angle)) continue;

        // AABB test for the tick line (cheap)
        const TickCache& t = g_ticks[i];
        int16_t minX = (t.inner.x < t.outer.x) ? t.inner.x : t.outer.x;
        int16_t maxX = (t.inner.x > t.outer.x) ? t.inner.x : t.outer.x;
        int16_t minY = (t.inner.y < t.outer.y) ? t.inner.y : t.outer.y;
        int16_t maxY = (t.inner.y > t.outer.y) ? t.inner.y : t.outer.y;

        RectI16 tr { minX, minY, static_cast<int16_t>(maxX - minX + 1), static_cast<int16_t>(maxY - minY + 1) };
        if (rectIntersects(r, tr)) {
            drawTickDirect(t, COL_AMBER_DIM);
        }
    }
}

// Redraw dynamic arrows that intersect the region
static void drawArrowsIntersectingRect(const RectI16& r, const CmwsState& s) {
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) {
        if (!rectIntersects(r, g_smallAabb[i])) continue;
        const ElemState st = s.small[i];
        if (st == ElemState::OFF) continue;
        drawArrowDirect(g_smallArrows[i], colorFor(st));
    }
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        if (!rectIntersects(r, g_largeAabb[i])) continue;
        const ElemState st = s.large[i];
        if (st == ElemState::OFF) continue;
        drawArrowDirect(g_largeArrows[i], colorFor(st));
    }
}

static void drawDRIntersectingRect(const RectI16& r, const CmwsState& s) {
    tft.setFont(&fonts::DejaVu24);
    tft.setTextDatum(textdatum_t::middle_center);

    if (rectIntersects(r, g_dRect)) {
        // Clear already happened; redraw if needed
        if (s.dispense != ElemState::OFF) {
            tft.setTextColor(colorFor(s.dispense));
            tft.drawString("D", RING_CX, RING_CY - DR_OFFSET);
        }
    }
    if (rectIntersects(r, g_rRect)) {
        if (s.ready != ElemState::OFF) {
            tft.setTextColor(colorFor(s.ready));
            tft.drawString("R", RING_CX, RING_CY + DR_OFFSET);
        }
    }
}

// One “correct” region restore + recompose for ring areas.
static void redrawRegion(const RectI16& r, const CmwsState& s) {
    // clamp
    RectI16 rr = r;
    if (rr.x < 0) { rr.w += rr.x; rr.x = 0; }
    if (rr.y < 0) { rr.h += rr.y; rr.y = 0; }
    if (rr.x + rr.w > SCREEN_W) rr.w = SCREEN_W - rr.x;
    if (rr.y + rr.h > SCREEN_H) rr.h = SCREEN_H - rr.y;
    if (rr.w <= 0 || rr.h <= 0) return;

    // 1) clear
    tft.fillRect(rr.x, rr.y, rr.w, rr.h, COL_BLACK);

    // 2) base layer
    drawTicksIntersectingRect(rr);

    // 3) dynamic layers that overlap
    drawArrowsIntersectingRect(rr, s);
    drawDRIntersectingRect(rr, s);
}

// =============================================================================
// STATE SNAPSHOT
// =============================================================================
static inline void snapshotState(CmwsState& out) {
    portENTER_CRITICAL(&g_stateMux);
    out = g_pending;
    portEXIT_CRITICAL(&g_stateMux);
}

// =============================================================================
// FULL REDRAW
// =============================================================================
static void fullRedraw(const CmwsState& s) {
    tft.fillScreen(COL_BLACK);

    // Base layer: all ticks not replaced by small arrows
    // (draw all ticks first; cheap)
    RectI16 full { 0,0, SCREEN_W, SCREEN_H };
    drawTicksIntersectingRect(full);

    // Dynamic: small arrows + large arrows
    drawArrowsIntersectingRect(full, s);

    // D/R
    drawDRIntersectingRect(full, s);

    // Text
    // tft.setFont(&fonts::Orbitron_Light_32);
    tft.setFont(FONT_DOTO_32);

    tft.setTextColor(COL_GREEN);
    tft.setTextDatum(textdatum_t::top_left);

    if (s.showInventory) {
        tft.setCursor(TEXT_X, TEXT_LINE1);
        tft.printf("%s %s", s.flareLetter, s.flareCount);

        tft.setCursor(TEXT_X, TEXT_LINE2);
        tft.printf("%s %s", s.chaffLetter, s.chaffCount);
    } else {
        tft.setCursor(TEXT_X, TEXT_LINE1);
        tft.print(s.bitLine1);

        tft.setCursor(TEXT_X, TEXT_LINE2);
        tft.print(s.bitLine2);
    }
}

// =============================================================================
// INCREMENTAL TEXT REDRAW
// =============================================================================
static void redrawTextLine(int16_t x, int16_t y, const char* text) {
    tft.fillRect(x, y, TEXT_CLEAR_W, TEXT_CLEAR_H, COL_BLACK);
    tft.setCursor(x, y);
    tft.print(text);
}

// =============================================================================
// MAIN DRAW
// =============================================================================
static void CMWSDisplay_draw(bool force = false) {

    if (g_deviceOff) return;

    if (!force && !isMissionRunning()) return;

    // quick check to avoid snapshot work
    if (!force && !g_dirty && !g_forceFull) return;

    const uint32_t now = millis();
    if (!force && !g_forceFull && (now - g_lastDrawMs < CMWS_REFRESH_INTERVAL_MS)) return;
    g_lastDrawMs = now;

    CmwsState s;
    snapshotState(s);

    // Backlight
    // (do outside startWrite, safe)
    tft.setBrightness(s.backlight);

    // OFF page => blank
    if (s.page == 0) {
        tft.fillScreen(COL_BLACK);
        g_forceFull = false;
        g_dirty = false;
        g_lastDrawn = s;
        return;
    }

    tft.startWrite();

    if (g_forceFull || (s.page != g_lastDrawn.page)) {
        fullRedraw(s);
        g_forceFull = false;
        g_dirty = false;
        g_lastDrawn = s;
        tft.endWrite();
        return;
    }

    // Incremental:
    // 1) arrows that changed => redraw their AABB region
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) {
        if (s.small[i] != g_lastDrawn.small[i]) {
            redrawRegion(g_smallAabb[i], s);
            g_lastDrawn.small[i] = s.small[i];
        }
    }
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        if (s.large[i] != g_lastDrawn.large[i]) {
            redrawRegion(g_largeAabb[i], s);
            g_lastDrawn.large[i] = s.large[i];
        }
    }

    // 2) D/R changed => redraw their rect
    if (s.dispense != g_lastDrawn.dispense) {
        redrawRegion(g_dRect, s);
        g_lastDrawn.dispense = s.dispense;
    }
    if (s.ready != g_lastDrawn.ready) {
        redrawRegion(g_rRect, s);
        g_lastDrawn.ready = s.ready;
    }

    // 3) text mode or text content changed => redraw lines
    const bool modeChanged = (s.showInventory != g_lastDrawn.showInventory);

    // tft.setFont(&fonts::Orbitron_Light_32);
    tft.setFont(FONT_DOTO_32);

    tft.setTextColor(COL_GREEN);
    tft.setTextDatum(textdatum_t::top_left);

    char line1[24];
    char line2[24];

    if (s.showInventory) {
        snprintf(line1, sizeof(line1), "%s %s", s.flareLetter, s.flareCount);
        snprintf(line2, sizeof(line2), "%s %s", s.chaffLetter, s.chaffCount);
    } else {
        strncpy(line1, s.bitLine1, sizeof(line1) - 1);
        line1[sizeof(line1) - 1] = '\0';
        strncpy(line2, s.bitLine2, sizeof(line2) - 1);
        line2[sizeof(line2) - 1] = '\0';
    }

    char prev1[24];
    char prev2[24];
    if (g_lastDrawn.showInventory) {
        snprintf(prev1, sizeof(prev1), "%s %s", g_lastDrawn.flareLetter, g_lastDrawn.flareCount);
        snprintf(prev2, sizeof(prev2), "%s %s", g_lastDrawn.chaffLetter, g_lastDrawn.chaffCount);
    } else {
        strncpy(prev1, g_lastDrawn.bitLine1, sizeof(prev1) - 1);
        prev1[sizeof(prev1) - 1] = '\0';
        strncpy(prev2, g_lastDrawn.bitLine2, sizeof(prev2) - 1);
        prev2[sizeof(prev2) - 1] = '\0';
    }

    if (modeChanged || (strcmp(line1, prev1) != 0)) {
        redrawTextLine(TEXT_X, TEXT_LINE1, line1);
    }
    if (modeChanged || (strcmp(line2, prev2) != 0)) {
        redrawTextLine(TEXT_X, TEXT_LINE2, line2);
    }

    // Update last-drawn text fields
    g_lastDrawn.showInventory = s.showInventory;
    strncpy(g_lastDrawn.flareLetter, s.flareLetter, sizeof(g_lastDrawn.flareLetter));
    strncpy(g_lastDrawn.chaffLetter, s.chaffLetter, sizeof(g_lastDrawn.chaffLetter));
    strncpy(g_lastDrawn.flareCount,  s.flareCount,  sizeof(g_lastDrawn.flareCount));
    strncpy(g_lastDrawn.chaffCount,  s.chaffCount,  sizeof(g_lastDrawn.chaffCount));
    strncpy(g_lastDrawn.bitLine1,    s.bitLine1,    sizeof(g_lastDrawn.bitLine1));
    strncpy(g_lastDrawn.bitLine2,    s.bitLine2,    sizeof(g_lastDrawn.bitLine2));

    g_dirty = false;

    tft.endWrite();
}

// =============================================================================
// GEOMETRY PRECOMPUTE
// =============================================================================
static void precomputeGeometry() {
    g_largeArrowRadius = static_cast<int16_t>(computeLargeArrowRadiusSymmetric());

    // Large arrows
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        computeArrowVertices(g_largeArrows[i], LARGE_ARROW_ANGLES[i], g_largeArrowRadius, 1.0f);

        TFT_Point pts[7] = {
            g_largeArrows[i].tip, g_largeArrows[i].tipBaseL, g_largeArrows[i].tipBaseR,
            g_largeArrows[i].bodyTopL, g_largeArrows[i].bodyTopR, g_largeArrows[i].bodyBotL, g_largeArrows[i].bodyBotR
        };
        g_largeAabb[i] = rectFromPoints(pts, 7, 2);
    }

    // Small arrows
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) {
        computeArrowVertices(g_smallArrows[i], SMALL_ARROW_ANGLES[i], SMALL_ARROW_RADIUS, SMALL_ARROW_SCALE);

        TFT_Point pts[7] = {
            g_smallArrows[i].tip, g_smallArrows[i].tipBaseL, g_smallArrows[i].tipBaseR,
            g_smallArrows[i].bodyTopL, g_smallArrows[i].bodyTopR, g_smallArrows[i].bodyBotL, g_smallArrows[i].bodyBotR
        };
        g_smallAabb[i] = rectFromPoints(pts, 7, 2);
    }

    // Ticks
    for (int i = 0; i < TICK_COUNT; ++i) {
        const int angle = (i * 360) / TICK_COUNT;
        computeTickVertices(g_ticks[i], angle);
    }

    // D/R rects (conservative 30x30 like your previous)
    g_dRect = RectI16{ static_cast<int16_t>(RING_CX - 15), static_cast<int16_t>(RING_CY - DR_OFFSET - 15), 30, 30 };
    g_rRect = RectI16{ static_cast<int16_t>(RING_CX - 15), static_cast<int16_t>(RING_CY + DR_OFFSET - 15), 30, 30 };
}

// =============================================================================
// DCS-BIOS CALLBACKS (ALL WRITE UNDER MUX)
// =============================================================================
static void markDirty() { g_dirty = true; }

static void setLargeStateFromBits(int idx) {
    const ElemState st = computeStateFromBits(g_largeBrt[idx], g_largeDim[idx]);

    if (st == ElemState::OFF) {
        if (!g_deviceOff) {  // Only blank once
            g_deviceOff = true;
            tft.startWrite();
            tft.fillScreen(COL_BLACK);
            tft.endWrite();
            g_dirty = false;
            g_forceFull = true;
        }
        return;
    }

    // Device coming back on
    if (g_deviceOff) {
        g_deviceOff = false;
        g_forceFull = true;
    }

    portENTER_CRITICAL(&g_stateMux);
    g_pending.large[idx] = st;
    portEXIT_CRITICAL(&g_stateMux);
    markDirty();
}

static void onFwdRightBrt(const char*, uint16_t v) { g_largeBrt[0] = (v != 0); setLargeStateFromBits(0); }
static void onFwdRightDim(const char*, uint16_t v) { g_largeDim[0] = (v != 0); setLargeStateFromBits(0); }

static void onAftRightBrt(const char*, uint16_t v) { g_largeBrt[1] = (v != 0); setLargeStateFromBits(1); }
static void onAftRightDim(const char*, uint16_t v) { g_largeDim[1] = (v != 0); setLargeStateFromBits(1); }

static void onAftLeftBrt(const char*, uint16_t v)  { g_largeBrt[2] = (v != 0); setLargeStateFromBits(2); }
static void onAftLeftDim(const char*, uint16_t v)  { g_largeDim[2] = (v != 0); setLargeStateFromBits(2); }

static void onFwdLeftBrt(const char*, uint16_t v)  { g_largeBrt[3] = (v != 0); setLargeStateFromBits(3); }
static void onFwdLeftDim(const char*, uint16_t v)  { g_largeDim[3] = (v != 0); setLargeStateFromBits(3); }

static void onDispenseBrt(const char*, uint16_t v) {
    g_dispBrt = (v != 0);
    const ElemState st = computeStateFromBits(g_dispBrt, g_dispDim);
    portENTER_CRITICAL(&g_stateMux);
    g_pending.dispense = st;
    portEXIT_CRITICAL(&g_stateMux);
    markDirty();
}
static void onDispenseDim(const char*, uint16_t v) {
    g_dispDim = (v != 0);
    const ElemState st = computeStateFromBits(g_dispBrt, g_dispDim);
    portENTER_CRITICAL(&g_stateMux);
    g_pending.dispense = st;
    portEXIT_CRITICAL(&g_stateMux);
    markDirty();
}
static void onReadyBrt(const char*, uint16_t v) {
    g_readyBrt = (v != 0);
    const ElemState st = computeStateFromBits(g_readyBrt, g_readyDim);
    portENTER_CRITICAL(&g_stateMux);
    g_pending.ready = st;
    portEXIT_CRITICAL(&g_stateMux);
    markDirty();
}
static void onReadyDim(const char*, uint16_t v) {
    g_readyDim = (v != 0);
    const ElemState st = computeStateFromBits(g_readyBrt, g_readyDim);
    portENTER_CRITICAL(&g_stateMux);
    g_pending.ready = st;
    portEXIT_CRITICAL(&g_stateMux);
    markDirty();
}

// Inventory / BIT strings
static void onFlareCount(const char*, const char* value) {
    if (!value) return;
    portENTER_CRITICAL(&g_stateMux);
    if (strncmp(g_pending.flareCount, value, 3) != 0) {
        strncpy(g_pending.flareCount, value, 3);
        g_pending.flareCount[3] = '\0';
        g_pending.showInventory = true;
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}
static void onChaffCount(const char*, const char* value) {
    if (!value) return;
    portENTER_CRITICAL(&g_stateMux);
    if (strncmp(g_pending.chaffCount, value, 3) != 0) {
        strncpy(g_pending.chaffCount, value, 3);
        g_pending.chaffCount[3] = '\0';
        g_pending.showInventory = true;
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}
static void onFlareLetter(const char*, const char* value) {
    if (!value || value[0] == '\0') return;
    portENTER_CRITICAL(&g_stateMux);
    if (g_pending.flareLetter[0] != value[0]) {
        g_pending.flareLetter[0] = value[0];
        g_pending.flareLetter[1] = '\0';
        g_pending.showInventory = true;
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}
static void onChaffLetter(const char*, const char* value) {
    if (!value || value[0] == '\0') return;
    portENTER_CRITICAL(&g_stateMux);
    if (g_pending.chaffLetter[0] != value[0]) {
        g_pending.chaffLetter[0] = value[0];
        g_pending.chaffLetter[1] = '\0';
        g_pending.showInventory = true;
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}

static void onBitLine1(const char*, const char* value) {
    if (!value) return;
    if (value[0] == '\0') return; // don't auto-switch back; inventory does that
    portENTER_CRITICAL(&g_stateMux);
    if (strncmp(g_pending.bitLine1, value, 7) != 0) {
        strncpy(g_pending.bitLine1, value, 7);
        g_pending.bitLine1[7] = '\0';
        g_pending.showInventory = false;
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}
static void onBitLine2(const char*, const char* value) {
    if (!value) return;
    if (value[0] == '\0') return;
    portENTER_CRITICAL(&g_stateMux);
    if (strncmp(g_pending.bitLine2, value, 7) != 0) {
        strncpy(g_pending.bitLine2, value, 7);
        g_pending.bitLine2[7] = '\0';
        g_pending.showInventory = false;
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}

static void onPage(const char*, const char* value) {
    uint8_t newPage = 1;
    if (value) {
        if (strcmp(value, "NONE") == 0) newPage = 0;
        else if (strcmp(value, "TEST") == 0) newPage = 2;
    }
    
    portENTER_CRITICAL(&g_stateMux);
    if (g_pending.page != newPage) {
        g_pending.page = newPage;
        portEXIT_CRITICAL(&g_stateMux);
        
        if (newPage == 0) {
            g_deviceOff = true;
            tft.startWrite();
            tft.fillScreen(COL_BLACK);
            tft.endWrite();
        } else {
            g_deviceOff = false;
        }
        g_forceFull = true;
        g_dirty = true;
        debugPrintf("[CMWS] Page changed to: %s\n", value);
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}

static void onLampChange(const char*, uint16_t value, uint16_t maxValue) {
    if (maxValue == 0) return;
    const uint8_t newLevel = static_cast<uint8_t>((static_cast<uint32_t>(value) * 255UL) / maxValue);

    // Update stored state
    portENTER_CRITICAL(&g_stateMux);
    g_pending.backlight = newLevel;
    portEXIT_CRITICAL(&g_stateMux);

    // Apply immediately (does NOT require a redraw)
    tft.setBrightness(newLevel);
}


// =============================================================================
// TASK
// =============================================================================
#if RUN_AS_TASK
static void CMWSDisplay_task(void*) {
    for (;;) {
        CMWSDisplay_draw(false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
#endif

// =============================================================================
// API
// =============================================================================
void CMWSDisplay_init() {
    // Precompute geometry before first draw
    precomputeGeometry();

    // Init display
    tft.init();
    tft.setRotation(3);
    tft.setColorDepth(16);
    tft.setSwapBytes(true);
    tft.setBrightness(255);
    tft.fillScreen(COL_BLACK);

    // Initialize pending state deterministically
    CmwsState init{};
    init.page = 1;
    init.showInventory = true;

    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) init.large[i] = ElemState::DIM;
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) init.small[i] = ElemState::DIM; // dynamic in future
    init.dispense = ElemState::DIM;
    init.ready    = ElemState::DIM;

    init.backlight = 255;

    strncpy(init.flareLetter, "F", sizeof(init.flareLetter));
    strncpy(init.chaffLetter, "C", sizeof(init.chaffLetter));
    strncpy(init.flareCount,  "00", sizeof(init.flareCount));
    strncpy(init.chaffCount,  "00", sizeof(init.chaffCount));
    init.bitLine1[0] = '\0';
    init.bitLine2[0] = '\0';

    portENTER_CRITICAL(&g_stateMux);
    g_pending = init;
    portEXIT_CRITICAL(&g_stateMux);

    g_lastDrawn = init;
    g_forceFull = true;
    g_dirty = true;

    // Subscriptions
    subscribeToMetadataChange("PLT_CMWS_FWD_RIGHT_BRT_L", onFwdRightBrt);
    subscribeToMetadataChange("PLT_CMWS_FWD_RIGHT_DIM_L", onFwdRightDim);
    subscribeToMetadataChange("PLT_CMWS_AFT_RIGHT_BRT_L", onAftRightBrt);
    subscribeToMetadataChange("PLT_CMWS_AFT_RIGHT_DIM_L", onAftRightDim);
    subscribeToMetadataChange("PLT_CMWS_AFT_LEFT_BRT_L", onAftLeftBrt);
    subscribeToMetadataChange("PLT_CMWS_AFT_LEFT_DIM_L", onAftLeftDim);
    subscribeToMetadataChange("PLT_CMWS_FWD_LEFT_BRT_L", onFwdLeftBrt);
    subscribeToMetadataChange("PLT_CMWS_FWD_LEFT_DIM_L", onFwdLeftDim);

    subscribeToMetadataChange("PLT_CMWS_D_BRT_L", onDispenseBrt);
    subscribeToMetadataChange("PLT_CMWS_D_DIM_L", onDispenseDim);
    subscribeToMetadataChange("PLT_CMWS_R_BRT_L", onReadyBrt);
    subscribeToMetadataChange("PLT_CMWS_R_DIM_L", onReadyDim);

    subscribeToDisplayChange("PLT_CMWS_FLARE_COUNT", onFlareCount);
    subscribeToDisplayChange("PLT_CMWS_CHAFF_COUNT", onChaffCount);
    subscribeToDisplayChange("PLT_CMWS_FLARE_LETTER", onFlareLetter);
    subscribeToDisplayChange("PLT_CMWS_CHAFF_LETTER", onChaffLetter);
    subscribeToDisplayChange("PLT_CMWS_BIT_LINE_1", onBitLine1);
    subscribeToDisplayChange("PLT_CMWS_BIT_LINE_2", onBitLine2);
    subscribeToDisplayChange("PLT_CMWS_PAGE", onPage);

    subscribeToLedChange("PLT_CMWS_LAMP", onLampChange);

    // Initial draw
    CMWSDisplay_draw(true);

#if RUN_BIT_TEST_ON_INIT
    CMWSDisplay_bitTest();
#endif

#if RUN_AS_TASK
    xTaskCreatePinnedToCore(CMWSDisplay_task, "CMWSTask", TASK_STACK_SIZE, nullptr, TASK_PRIORITY, &taskHandle, CPU_CORE);
#endif

    debugPrintf("✅ CMWS Display initialized: MOSI=%d SCLK=%d CS=%d DC=%d RST=%d BLK=%d\n",
        PIN_MOSI, PIN_SCLK, PIN_CS, PIN_DC, PIN_RST, PIN_BLK);
    debugPrintf("   Large arrow radius=%d, small arrows=%d (dynamic), ticks=%d\n",
        g_largeArrowRadius, SMALL_ARROW_COUNT, TICK_COUNT);
}

void CMWSDisplay_loop() {
#if !RUN_AS_TASK
    CMWSDisplay_draw(false);
#endif
}

void CMWSDisplay_notifyMissionStart() {
    g_forceFull = true;
    g_dirty = true;
}

void CMWSDisplay_deinit() {
#if RUN_AS_TASK
    if (taskHandle) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
#endif
    tft.fillScreen(COL_BLACK);
    tft.setBrightness(0);
}

// =============================================================================
// BIT TEST (blocking) — preserves and restores full state
// =============================================================================
void CMWSDisplay_bitTest() {
    CmwsState saved;
    snapshotState(saved);

    // Force on + bright
    CmwsState tmp = saved;
    tmp.page = 1;
    tmp.backlight = 255;
    tmp.showInventory = true;
    strncpy(tmp.flareCount, "88", sizeof(tmp.flareCount));
    strncpy(tmp.chaffCount, "88", sizeof(tmp.chaffCount));

    // Phase 1: all bright
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) tmp.large[i] = ElemState::BRT;
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) tmp.small[i] = ElemState::BRT;
    tmp.dispense = ElemState::BRT;
    tmp.ready    = ElemState::BRT;

    portENTER_CRITICAL(&g_stateMux);
    g_pending = tmp;
    portEXIT_CRITICAL(&g_stateMux);
    g_forceFull = true; g_dirty = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Phase 2: all dim
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) tmp.large[i] = ElemState::DIM;
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) tmp.small[i] = ElemState::DIM;
    tmp.dispense = ElemState::DIM;
    tmp.ready    = ElemState::DIM;

    portENTER_CRITICAL(&g_stateMux);
    g_pending = tmp;
    portEXIT_CRITICAL(&g_stateMux);
    g_forceFull = true; g_dirty = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Phase 3: all off
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) tmp.large[i] = ElemState::OFF;
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) tmp.small[i] = ElemState::OFF;
    tmp.dispense = ElemState::OFF;
    tmp.ready    = ElemState::OFF;

    portENTER_CRITICAL(&g_stateMux);
    g_pending = tmp;
    portEXIT_CRITICAL(&g_stateMux);
    g_forceFull = true; g_dirty = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Phase 4: rotate large arrows bright
    for (int a = 0; a < LARGE_ARROW_COUNT; ++a) {
        for (int i = 0; i < LARGE_ARROW_COUNT; ++i) tmp.large[i] = ElemState::DIM;
        tmp.large[a] = ElemState::BRT;
        tmp.dispense = ElemState::DIM;
        tmp.ready    = ElemState::DIM;

        portENTER_CRITICAL(&g_stateMux);
        g_pending = tmp;
        portEXIT_CRITICAL(&g_stateMux);
        g_forceFull = true; g_dirty = true;
        CMWSDisplay_draw(true);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    // Restore full state exactly
    portENTER_CRITICAL(&g_stateMux);
    g_pending = saved;
    portEXIT_CRITICAL(&g_stateMux);

    g_forceFull = true;
    g_dirty = true;
    CMWSDisplay_draw(true);
}

#else

void CMWSDisplay_init() {}
void CMWSDisplay_loop() {}
void CMWSDisplay_deinit() {}
void CMWSDisplay_notifyMissionStart() {}
void CMWSDisplay_bitTest() {}

#endif
