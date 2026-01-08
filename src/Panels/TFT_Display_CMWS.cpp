// =============================================================================
// TFT_Display_CMWS.cpp — CockpitOS CMWS Threat Ring Display (LOW-MEM, DIRECT-DRAW)
// =============================================================================
// AH-64D Apache Countermeasures Warning System Display
//
// Hardware Support:
//   - ESP32 Classic + IdeasPark ST7789 170×320 TFT (4-wire SPI)
//   - ESP32-S3 + LilyGo T-Display S3 ST7789 170×320 TFT (8-bit Parallel)
//
// DESIGN GOALS (ESP32 Classic: no PSRAM, tight SRAM):
//   1) No full-frame sprites — direct draw only.
//   2) Precompute geometry once (ticks + arrows + AABBs).
//   3) Incremental redraw with correct layering restoration.
//   4) Deterministic: snapshot state under critical section.
//   5) Deferred state recomputation for burst DCS-BIOS updates.
//
// OPTIMIZATION NOTES:
//   - Zero heap allocation
//   - memcpy/memcmp for fixed-width fields (no null scanning)
//   - Deferred device state recomputation (once per frame, not per callback)
//   - Compile-time geometry validation
//   - Bounded iteration with explicit caps
//
// LAYERS:
//   Base layer: black background + tick marks (excluding small arrow positions).
//   Dynamic layer: 8 small arrows + 4 large arrows + D/R letters + inventory/BIT text.
//
// DEVICE ON/OFF RULES:
//   - Device is OFF only when ALL 4 large arrows have BOTH BRT=0 AND DIM=0
//   - When OFF: LAMP=0%, small arrows=OFF, ticks=OFF, nothing drawn
//   - When ON: small arrows and ticks are always DIM
//
// PAGE MODE RULES:
//   - Only accept exact full strings: "MAIN" or "TEST"
//   - MAIN: show FLARE_LETTER/FLARE_COUNT and CHAFF_LETTER/CHAFF_COUNT
//   - TEST: show BIT_LINE_1 and BIT_LINE_2
//
// =============================================================================

#include "../Globals.h"

#if defined(HAS_CMWS_DISPLAY) && defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1)

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "includes/TFT_Display_CMWS.h"

// =============================================================================
// PANEL REGISTRATION
// =============================================================================
#if defined(HAS_CMWS_DISPLAY)
    REGISTER_PANEL(TFTCmws, nullptr, nullptr, CMWSDisplay_init, CMWSDisplay_loop, nullptr, 100);
#endif

#if !__has_include(<LovyanGFX.hpp>)
#error "❌ Missing LovyanGFX.hpp — Please install LovyanGFX library"
#endif

#include <LovyanGFX.hpp>
#include <cstring>
#include <cmath>
#include <cstdlib>

// =============================================================================
// COMPILE-TIME CONFIGURATION
// =============================================================================
static constexpr uint32_t CMWS_REFRESH_INTERVAL_MS = 33;   // ~30 FPS max
static constexpr uint8_t  CPU_CORE                 = 0;
static constexpr uint16_t TASK_STACK_SIZE          = 4096;
static constexpr uint8_t  TASK_PRIORITY            = 2;

// IMPORTANT: These must be macros (not constexpr) because they're used with #if preprocessor
#define RUN_AS_TASK              1   // 1 = FreeRTOS task, 0 = loop() polling
#define USE_REFACTORED_VERSION   1   // Dirty-rect batching (recommended)
#define RUN_BIT_TEST_ON_INIT     1   // Self-test on boot
#define ENABLE_PROFILING         0   // Set to 1 to enable timing instrumentation

// =============================================================================
// FIELD SIZE CONSTANTS (eliminates magic numbers)
// =============================================================================
static constexpr size_t LETTER_FIELD_SIZE   = 4;   // "F" + padding + null
static constexpr size_t COUNT_FIELD_SIZE    = 8;   // " 00" + padding + null
static constexpr size_t BITLINE_FIELD_SIZE  = 8;   // "XXXX" + padding + null
static constexpr size_t COUNT_DISPLAY_LEN   = 3;   // " 00" visible chars
static constexpr size_t BITLINE_DISPLAY_LEN = 4;   // "XXXX" visible chars

// =============================================================================
// GEOMETRY CONSTANTS
// =============================================================================
static constexpr int16_t SCREEN_W = 320;
static constexpr int16_t SCREEN_H = 170;

static constexpr int16_t RING_CX = 235;
static constexpr int16_t RING_CY = 85;

static constexpr int16_t TICK_INNER_R = 66;
static constexpr int16_t TICK_OUTER_R = 76;

static constexpr int TICK_COUNT        = 24;   // 15° increments
static constexpr int LARGE_ARROW_COUNT = 4;
static constexpr int SMALL_ARROW_COUNT = 8;

// Text layout
static constexpr int16_t TEXT_LINE1    = 28;
static constexpr int16_t TEXT_LINE2    = 100;
static constexpr int16_t TEXT_CLEAR_H  = 42;
static constexpr int16_t TEXT_CLEAR_W  = 135;
static constexpr int16_t TEXT_X        = 10;

// D/R letter positioning
static constexpr int16_t DR_OFFSET     = 40;
static constexpr int16_t DR_X_OFFSET   = 3;

// Arrow shape constants
static constexpr float LARGE_TIP_Y       = 30.0f;
static constexpr float LARGE_TIP_BASE_Y  = 11.0f;
static constexpr float LARGE_BODY_BASE_Y = 0.0f;
static constexpr float LARGE_TIP_HALF_W  = 16.0f;
static constexpr float LARGE_BODY_HALF_W = 8.5f;
static constexpr float SMALL_ARROW_SCALE = 0.5f;

static constexpr int16_t SMALL_ARROW_RADIUS = TICK_OUTER_R - 10;

// Arrow angles (constexpr arrays)
static constexpr int LARGE_ARROW_ANGLES[LARGE_ARROW_COUNT] = { 45, 135, 225, 315 };
static constexpr int SMALL_ARROW_ANGLES[SMALL_ARROW_COUNT] = { 0, 45, 90, 135, 180, 225, 270, 315 };

// =============================================================================
// COMPILE-TIME VALIDATION
// =============================================================================
static_assert(LARGE_ARROW_COUNT == 4, "Large arrow count must be 4");
static_assert(SMALL_ARROW_COUNT == 8, "Small arrow count must be 8");
static_assert(TICK_COUNT == 24, "Tick count must be 24 (15° increments)");
static_assert(SCREEN_W == 320 && SCREEN_H == 170, "Screen dimensions mismatch");
static_assert(COUNT_FIELD_SIZE >= COUNT_DISPLAY_LEN + 1, "Count field too small");
static_assert(BITLINE_FIELD_SIZE >= BITLINE_DISPLAY_LEN + 1, "Bitline field too small");

// =============================================================================
// COLORS (pixel-matched to reference display)
// =============================================================================
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

static constexpr uint16_t COL_BLACK     = 0x0000;
static constexpr uint16_t COL_GREEN     = RGB565(115, 190, 100);
static constexpr uint16_t COL_AMBER_BRT = RGB565(255, 200, 0);
static constexpr uint16_t COL_AMBER_DIM = RGB565(8, 4, 0);

// =============================================================================
// FONTS
// =============================================================================
#include "Assets/Fonts/Doto_Rounded_Black26pt7b.h"
static const GFXfont* const FONT_DOTO = &Doto_Rounded_Black26pt7b;

#include "Assets/Fonts/MilSpec3355810pt7b.h"
static const GFXfont* const FONT_MILSPEC = &MilSpec3355810pt7b;

// =============================================================================
// DISPLAY INTERFACE SELECTION (auto-detect if not specified)
// =============================================================================
#if !defined(CMWS_USE_SPI_INTERFACE) && !defined(CMWS_USE_PARALLEL_INTERFACE)
    #if defined(CONFIG_IDF_TARGET_ESP32S3)
        #define CMWS_USE_PARALLEL_INTERFACE
    #elif defined(ESP_FAMILY_CLASSIC) || defined(CONFIG_IDF_TARGET_ESP32)
        #define CMWS_USE_SPI_INTERFACE
    #else
        #define CMWS_USE_SPI_INTERFACE
    #endif
#endif

// =============================================================================
// PIN DEFINITIONS - SPI INTERFACE
// =============================================================================
#if defined(CMWS_USE_SPI_INTERFACE)

    #if defined(HAS_CMWS_DISPLAY)
        static constexpr int8_t PIN_MOSI = PIN(23);
        static constexpr int8_t PIN_SCLK = PIN(18);
        static constexpr int8_t PIN_CS   = PIN(15);
        static constexpr int8_t PIN_DC   = PIN(2);
        static constexpr int8_t PIN_RST  = PIN(4);
        static constexpr int8_t PIN_BLK  = PIN(32);
    #else
        static constexpr int8_t PIN_MOSI = -1;
        static constexpr int8_t PIN_SCLK = -1;
        static constexpr int8_t PIN_CS   = -1;
        static constexpr int8_t PIN_DC   = -1;
        static constexpr int8_t PIN_RST  = -1;
        static constexpr int8_t PIN_BLK  = -1;
    #endif

    #if defined(ESP_FAMILY_CLASSIC)
        static constexpr spi_host_device_t CMWS_SPI_HOST = VSPI_HOST;
    #else
        static constexpr spi_host_device_t CMWS_SPI_HOST = SPI2_HOST;
    #endif

#endif

// =============================================================================
// PIN DEFINITIONS - 8-BIT PARALLEL INTERFACE (LilyGo T-Display S3)
// =============================================================================
#if defined(CMWS_USE_PARALLEL_INTERFACE)

    static constexpr int8_t PIN_D0  = 39;
    static constexpr int8_t PIN_D1  = 40;
    static constexpr int8_t PIN_D2  = 41;
    static constexpr int8_t PIN_D3  = 42;
    static constexpr int8_t PIN_D4  = 45;
    static constexpr int8_t PIN_D5  = 46;
    static constexpr int8_t PIN_D6  = 47;
    static constexpr int8_t PIN_D7  = 48;

    static constexpr int8_t PIN_WR    = 8;
    static constexpr int8_t PIN_RD    = 9;
    static constexpr int8_t PIN_DC    = 7;
    static constexpr int8_t PIN_CS    = 6;
    static constexpr int8_t PIN_RST   = 5;
    static constexpr int8_t PIN_BLK   = 38;
    static constexpr int8_t PIN_POWER = 15;

#endif

// =============================================================================
// STATE ENUMS
// =============================================================================
enum class ElemState : uint8_t { OFF = 0, DIM = 1, BRT = 2 };
enum class PageMode  : uint8_t { MAIN = 0, TEST = 1 };

// =============================================================================
// SMALL HELPERS (no IRAM_ATTR - real bottleneck is LovyanGFX driver calls)
// =============================================================================

static inline ElemState computeStateFromBits(bool brt, bool dim) {
    if (brt) return ElemState::BRT;
    if (dim) return ElemState::DIM;
    return ElemState::OFF;
}

static inline uint16_t colorFor(ElemState s) {
    return (s == ElemState::BRT) ? COL_AMBER_BRT : COL_AMBER_DIM;
}

static inline int16_t clampI16(int32_t v, int16_t lo, int16_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return static_cast<int16_t>(v);
}

// =============================================================================
// FIXED-WIDTH FIELD HELPERS (NO HEAP, NO NULL SCANNING)
// =============================================================================

// Format BIT line as exactly 4 chars (space padded)
static inline void formatField4(char out[5], const char* in) {
    out[0] = ' '; out[1] = ' '; out[2] = ' '; out[3] = ' ';
    out[4] = '\0';

    if (!in) return;

    for (int i = 0; i < 4; ++i) {
        const char c = in[i];
        if (c == '\0') break;
        out[i] = c;
    }
}

// =============================================================================
// GEOMETRY CACHES (static, computed once at init)
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
static RectI16 g_dRect;
static RectI16 g_rRect;

static int16_t g_largeArrowRadius = 0;

// Compile-time size validation
static_assert(sizeof(ArrowCache) <= 32, "ArrowCache exceeds expected size");
static_assert(sizeof(RectI16) == 8, "RectI16 should be 8 bytes");

// =============================================================================
// RECT UTILITIES
// =============================================================================

static RectI16 rectFromPoints(const TFT_Point* pts, int count, int16_t pad = 1) {
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
// LOVYANGFX DEVICE CLASS - SPI INTERFACE
// =============================================================================
#if defined(CMWS_USE_SPI_INTERFACE)

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
            cfg.freq_write  = 80000000;
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

#endif

// =============================================================================
// LOVYANGFX DEVICE CLASS - 8-BIT PARALLEL INTERFACE
// =============================================================================
#if defined(CMWS_USE_PARALLEL_INTERFACE)

class LGFX_CMWS final : public lgfx::LGFX_Device {
    lgfx::Bus_Parallel8 _bus;
    lgfx::Panel_ST7789  _panel;
    lgfx::Light_PWM     _light;

public:
    LGFX_CMWS() {
        {
            auto cfg = _bus.config();
            cfg.freq_write = 20000000;  // 20MHz max stable for this hardware
            cfg.freq_read  = 0;
            cfg.pin_wr     = PIN_WR;
            cfg.pin_rd     = PIN_RD;
            cfg.pin_rs     = PIN_DC;
            cfg.pin_d0     = PIN_D0;
            cfg.pin_d1     = PIN_D1;
            cfg.pin_d2     = PIN_D2;
            cfg.pin_d3     = PIN_D3;
            cfg.pin_d4     = PIN_D4;
            cfg.pin_d5     = PIN_D5;
            cfg.pin_d6     = PIN_D6;
            cfg.pin_d7     = PIN_D7;
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

#endif

static LGFX_CMWS tft;
static volatile TaskHandle_t taskHandle = nullptr;  // volatile: read in deinit polling loop, written by task
static volatile bool g_displayInitialized = false;

// =============================================================================
// PLATFORM-SPECIFIC INITIALIZATION
// =============================================================================
static void platformInit() {
#if defined(CMWS_USE_PARALLEL_INTERFACE)
    pinMode(PIN_POWER, OUTPUT);
    digitalWrite(PIN_POWER, HIGH);
    
    // Force RD high to prevent floating noise (reads disabled anyway)
    pinMode(PIN_RD, OUTPUT);
    digitalWrite(PIN_RD, HIGH);
    
    delay(10);
#endif
}

// =============================================================================
// STATE MANAGEMENT
// =============================================================================
static portMUX_TYPE g_stateMux = portMUX_INITIALIZER_UNLOCKED;

struct CmwsState {
    PageMode  pageMode;
    bool      deviceOn;
    ElemState large[LARGE_ARROW_COUNT];
    ElemState small[SMALL_ARROW_COUNT];
    ElemState dispense;
    ElemState ready;
    uint8_t   backlight;

    char flareLetter[LETTER_FIELD_SIZE];
    char chaffLetter[LETTER_FIELD_SIZE];
    char flareCount[COUNT_FIELD_SIZE];
    char chaffCount[COUNT_FIELD_SIZE];
    char bitLine1[BITLINE_FIELD_SIZE];
    char bitLine2[BITLINE_FIELD_SIZE];
};

static_assert(sizeof(CmwsState) <= 96, "CmwsState exceeds expected size budget");

static CmwsState g_pending;
static CmwsState g_lastDrawn;

static volatile bool g_dirty              = true;
static volatile bool g_forceFull          = true;
static volatile bool g_deviceStateStale   = false;  // Deferred recomputation flag
static volatile bool g_taskStopRequested  = false;  // Cooperative shutdown flag
static uint32_t      g_lastDrawMs         = 0;
static uint8_t       g_lastBrightness     = 0xFF;   // Cached to avoid redundant updates

// Raw BRT/DIM bits (written by callbacks, consumed by deferred recomputation)
static volatile bool g_largeBrt[LARGE_ARROW_COUNT] = { false, false, false, false };
static volatile bool g_largeDim[LARGE_ARROW_COUNT] = { false, false, false, false };
static volatile bool g_dispBrt  = false;
static volatile bool g_dispDim  = false;
static volatile bool g_readyBrt = false;
static volatile bool g_readyDim = false;

// =============================================================================
// DEVICE ON/OFF LOGIC (CORRECTED FOR THREAD SAFETY)
// =============================================================================

// Compute device ON from a snapshot of bits (deterministic, no races)
static inline bool computeDeviceOnFromSnapshot(const bool brt[LARGE_ARROW_COUNT], 
                                                const bool dim[LARGE_ARROW_COUNT]) {
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        if (brt[i] || dim[i]) {
            return true;
        }
    }
    return false;
}

// Recompute all derived state from raw bits (called once per frame, not per callback)
// Note: Volatile bit snapshot is best-effort (not under mux); writes to g_pending are atomic under mux.
// Worst case: one frame sees mixed snapshot, next frame corrects. Acceptable for display use.
static void recomputeDeviceState() {
    // Snapshot volatile bits FIRST (single atomic-ish read of each)
    bool localBrt[LARGE_ARROW_COUNT];
    bool localDim[LARGE_ARROW_COUNT];
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        localBrt[i] = g_largeBrt[i];
        localDim[i] = g_largeDim[i];
    }
    const bool localDispBrt  = g_dispBrt;
    const bool localDispDim  = g_dispDim;
    const bool localReadyBrt = g_readyBrt;
    const bool localReadyDim = g_readyDim;

    // Compute derived state from snapshot
    const bool nowOn = computeDeviceOnFromSnapshot(localBrt, localDim);

    // Apply under critical section (wasOn read + all writes atomic)
    portENTER_CRITICAL(&g_stateMux);

    const bool wasOn = g_pending.deviceOn;
    g_pending.deviceOn = nowOn;

    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        g_pending.large[i] = computeStateFromBits(localBrt[i], localDim[i]);
    }

    g_pending.dispense = computeStateFromBits(localDispBrt, localDispDim);
    g_pending.ready    = computeStateFromBits(localReadyBrt, localReadyDim);

    portEXIT_CRITICAL(&g_stateMux);

    // Force full redraw on ON/OFF transition (outside critical section is safe now)
    if (wasOn != nowOn) {
        g_forceFull = true;
    }
}

// =============================================================================
// TRIG / VECTOR COMPUTATION
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

    const float rx = fy;
    const float ry = -fx;

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
// GEOMETRY PRECOMPUTATION (called once at init)
// =============================================================================
static void precomputeGeometry() {
    g_largeArrowRadius = static_cast<int16_t>(computeLargeArrowRadiusSymmetric());

    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        computeArrowVertices(g_largeArrows[i], LARGE_ARROW_ANGLES[i], g_largeArrowRadius, 1.0f);

        TFT_Point pts[7] = {
            g_largeArrows[i].tip, g_largeArrows[i].tipBaseL, g_largeArrows[i].tipBaseR,
            g_largeArrows[i].bodyTopL, g_largeArrows[i].bodyTopR,
            g_largeArrows[i].bodyBotL, g_largeArrows[i].bodyBotR
        };
        g_largeAabb[i] = rectFromPoints(pts, 7, 2);
    }

    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) {
        computeArrowVertices(g_smallArrows[i], SMALL_ARROW_ANGLES[i], SMALL_ARROW_RADIUS, SMALL_ARROW_SCALE);

        TFT_Point pts[7] = {
            g_smallArrows[i].tip, g_smallArrows[i].tipBaseL, g_smallArrows[i].tipBaseR,
            g_smallArrows[i].bodyTopL, g_smallArrows[i].bodyTopR,
            g_smallArrows[i].bodyBotL, g_smallArrows[i].bodyBotR
        };
        g_smallAabb[i] = rectFromPoints(pts, 7, 2);
    }

    for (int i = 0; i < TICK_COUNT; ++i) {
        const int angle = (i * 360) / TICK_COUNT;
        computeTickVertices(g_ticks[i], angle);
    }

    g_dRect = RectI16{ static_cast<int16_t>(RING_CX + DR_X_OFFSET - 15),
                       static_cast<int16_t>(RING_CY - DR_OFFSET - 15), 30, 30 };
    g_rRect = RectI16{ static_cast<int16_t>(RING_CX + DR_X_OFFSET - 15),
                       static_cast<int16_t>(RING_CY + DR_OFFSET - 15), 30, 30 };
}

// =============================================================================
// DRAW PRIMITIVES (DIRECT TO DISPLAY)
// =============================================================================

static void drawArrowDirect(const ArrowCache& a, uint16_t color) {
    tft.fillTriangle(a.tip.x, a.tip.y, a.tipBaseL.x, a.tipBaseL.y, a.tipBaseR.x, a.tipBaseR.y, color);
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

static void drawTicksIntersectingRect(const RectI16& r, bool deviceOn) {
    if (!deviceOn) return;

    for (int i = 0; i < TICK_COUNT; ++i) {
        const int angle = (i * 360) / TICK_COUNT;

        if (angleIsSmallArrow(angle)) continue;

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

static void drawArrowsIntersectingRect(const RectI16& r, const CmwsState& s) {
    if (s.deviceOn) {
        for (int i = 0; i < SMALL_ARROW_COUNT; ++i) {
            if (!rectIntersects(r, g_smallAabb[i])) continue;
            drawArrowDirect(g_smallArrows[i], COL_AMBER_DIM);
        }
    }

    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        if (!rectIntersects(r, g_largeAabb[i])) continue;
        const ElemState st = s.large[i];
        if (st == ElemState::OFF) continue;
        drawArrowDirect(g_largeArrows[i], colorFor(st));
    }
}

static void drawDRIntersectingRect(const RectI16& r, const CmwsState& s) {
    if (!s.deviceOn) return;

    tft.setFont(FONT_MILSPEC);
    tft.setTextDatum(textdatum_t::middle_center);

    if (rectIntersects(r, g_dRect)) {
        if (s.dispense != ElemState::OFF) {
            tft.setTextColor(colorFor(s.dispense));
            tft.drawString("D", RING_CX + DR_X_OFFSET, RING_CY - DR_OFFSET);
        }
    }
    if (rectIntersects(r, g_rRect)) {
        if (s.ready != ElemState::OFF) {
            tft.setTextColor(colorFor(s.ready));
            tft.drawString("R", RING_CX + DR_X_OFFSET, RING_CY + DR_OFFSET);
        }
    }
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

    if (!s.deviceOn) {
        return;
    }

    RectI16 full { 0, 0, SCREEN_W, SCREEN_H };
    drawTicksIntersectingRect(full, s.deviceOn);
    drawArrowsIntersectingRect(full, s);
    drawDRIntersectingRect(full, s);

    tft.setFont(FONT_DOTO);
    tft.setTextColor(COL_GREEN);
    tft.setTextDatum(textdatum_t::top_left);

    if (s.pageMode == PageMode::MAIN) {
        tft.setCursor(TEXT_X, TEXT_LINE1);
        tft.printf("%s%s", s.flareLetter, s.flareCount);

        tft.setCursor(TEXT_X, TEXT_LINE2);
        tft.printf("%s%s", s.chaffLetter, s.chaffCount);
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
// DIRTY-RECT LIST (bounded, merge-on-overlap)
// =============================================================================

#if USE_REFACTORED_VERSION == 1

struct DirtyRectList {
    static constexpr int kMax = 8;

    RectI16 r[kMax];
    uint8_t n = 0;

    static inline RectI16 clampToScreen(RectI16 rr) {
        if (rr.x < 0) { rr.w += rr.x; rr.x = 0; }
        if (rr.y < 0) { rr.h += rr.y; rr.y = 0; }
        if (rr.x + rr.w > SCREEN_W) rr.w = SCREEN_W - rr.x;
        if (rr.y + rr.h > SCREEN_H) rr.h = SCREEN_H - rr.y;
        if (rr.w < 0) rr.w = 0;
        if (rr.h < 0) rr.h = 0;
        return rr;
    }

    static inline bool intersectsOrTouches(const RectI16& a, const RectI16& b) {
        return !(a.x + a.w < b.x || b.x + b.w < a.x || a.y + a.h < b.y || b.y + b.h < a.y);
    }

    static inline RectI16 unite(const RectI16& a, const RectI16& b) {
        const int16_t x1 = (a.x < b.x) ? a.x : b.x;
        const int16_t y1 = (a.y < b.y) ? a.y : b.y;
        const int16_t x2 = ((int32_t)a.x + a.w > (int32_t)b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
        const int16_t y2 = ((int32_t)a.y + a.h > (int32_t)b.y + b.h) ? (a.y + a.h) : (b.y + b.h);
        return RectI16{ x1, y1, static_cast<int16_t>(x2 - x1), static_cast<int16_t>(y2 - y1) };
    }

    // Cascade-merge rect at index i with any overlapping rects
    // Bounded by kMax merges (can't merge more rects than exist)
    inline void cascadeMerge(uint8_t i) {
        int mergeCount = 0;
        bool merged;
        do {
            merged = false;
            for (uint8_t j = 0; j < n; ++j) {
                if (j != i && intersectsOrTouches(r[i], r[j])) {
                    r[i] = clampToScreen(unite(r[i], r[j]));
                    // Remove j by replacing with last element
                    r[j] = r[--n];
                    // FIX: If i was the last element, our merged rect was copied to j
                    if (i == n) {
                        i = j;
                    }
                    ++mergeCount;
                    merged = true;
                    break;  // Restart scan since indices shifted
                }
            }
        } while (merged && mergeCount < kMax);  // Can't merge more than kMax rects
    }

    inline void add(RectI16 rr) {
        rr = clampToScreen(rr);
        if (rr.w <= 0 || rr.h <= 0) return;

        // Try to merge with existing rects
        for (uint8_t i = 0; i < n; ++i) {
            if (intersectsOrTouches(r[i], rr)) {
                r[i] = clampToScreen(unite(r[i], rr));
                cascadeMerge(i);
                return;
            }
        }

        // No overlap found - add new rect
        if (n < kMax) {
            r[n++] = rr;
        } else {
            // Overflow: merge into first rect (safe fallback)
            r[0] = clampToScreen(unite(r[0], rr));
            cascadeMerge(0);
        }
    }
};

static void redrawRegion(const RectI16& r, const CmwsState& s) {
    RectI16 rr = DirtyRectList::clampToScreen(r);
    if (rr.w <= 0 || rr.h <= 0) return;

    tft.fillRect(rr.x, rr.y, rr.w, rr.h, COL_BLACK);
    drawTicksIntersectingRect(rr, s.deviceOn);
    drawArrowsIntersectingRect(rr, s);
    drawDRIntersectingRect(rr, s);
}

// =============================================================================
// MAIN DRAW FUNCTION (REFACTORED)
// =============================================================================

static void CMWSDisplay_draw(bool force = false) {
    if (!g_displayInitialized) return;
    if (!force && !isMissionRunning()) return;
    
    // Capture and clear dirty flags at START of frame to prevent lost updates.
    // If a callback fires during render, it re-sets g_dirty for the NEXT frame.
    const bool wasDirty = g_dirty;
    const bool wasForceFull = g_forceFull;
    g_dirty = false;
    g_forceFull = false;
    
    if (!force && !wasDirty && !wasForceFull) return;

    const uint32_t now = millis();
    if (!force && !wasForceFull && (now - g_lastDrawMs < CMWS_REFRESH_INTERVAL_MS)) {
        // Restore flags if we're rate-limited (so next frame processes them)
        if (wasDirty) g_dirty = true;
        if (wasForceFull) g_forceFull = true;
        return;
    }
    g_lastDrawMs = now;

#if ENABLE_PROFILING
    const uint32_t profStart = micros();
#endif

    // Deferred device state recomputation (once per frame, not per callback)
    // Clear flag BEFORE processing to avoid race: if callback sets flag during
    // recompute, we'll just recompute again next frame (safe)
    if (g_deviceStateStale) {
        g_deviceStateStale = false;
        recomputeDeviceState();
    }

    CmwsState s;
    snapshotState(s);

    // Backlight: 0% when device OFF, user-requested when ON
    const uint8_t effectiveBacklight = s.deviceOn ? s.backlight : 0;
    if (effectiveBacklight != g_lastBrightness) {
        tft.setBrightness(effectiveBacklight);
        g_lastBrightness = effectiveBacklight;
    }

    // Device OFF => blank screen
    if (!s.deviceOn) {
        if (wasForceFull || g_lastDrawn.deviceOn) {
            tft.startWrite();
            tft.fillScreen(COL_BLACK);
            tft.endWrite();
        }
        g_lastDrawn = s;
        return;
    }

    tft.startWrite();

    // Full redraw if forced, device just came ON, or page mode changed
    if (wasForceFull || !g_lastDrawn.deviceOn || (s.pageMode != g_lastDrawn.pageMode)) {
        fullRedraw(s);
        g_lastDrawn = s;
        tft.endWrite();

#if ENABLE_PROFILING
        debugPrintf("[CMWS] Full redraw: %luµs, pixels=%lu\n", 
                    micros() - profStart, (unsigned long)(SCREEN_W * SCREEN_H));
#endif
        return;
    }

    // --- INCREMENTAL UPDATE ---
    DirtyRectList dirty;

    // 1) Large arrows that changed
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        if (s.large[i] != g_lastDrawn.large[i]) {
            dirty.add(g_largeAabb[i]);
        }
    }

    // 2) D/R changes
    if (s.dispense != g_lastDrawn.dispense) dirty.add(g_dRect);
    if (s.ready != g_lastDrawn.ready)       dirty.add(g_rRect);

    // 3) Redraw all merged dirty rects
#if ENABLE_PROFILING
    uint32_t totalPixels = 0;
#endif
    for (uint8_t i = 0; i < dirty.n; ++i) {
        redrawRegion(dirty.r[i], s);
#if ENABLE_PROFILING
        totalPixels += static_cast<uint32_t>(dirty.r[i].w) * dirty.r[i].h;
#endif
    }

    // --- TEXT UPDATE ---
    tft.setFont(FONT_DOTO);
    tft.setTextColor(COL_GREEN);
    tft.setTextDatum(textdatum_t::top_left);

    if (s.pageMode == PageMode::MAIN) {
        // Compare fixed-width fields directly (no strcmp overhead)
        const bool line1Changed =
            (g_lastDrawn.flareLetter[0] != s.flareLetter[0]) ||
            (memcmp(g_lastDrawn.flareCount, s.flareCount, COUNT_DISPLAY_LEN + 1) != 0);

        const bool line2Changed =
            (g_lastDrawn.chaffLetter[0] != s.chaffLetter[0]) ||
            (memcmp(g_lastDrawn.chaffCount, s.chaffCount, COUNT_DISPLAY_LEN + 1) != 0);

        if (line1Changed) {
            char line1[8];
            line1[0] = s.flareLetter[0] ? s.flareLetter[0] : ' ';
            memcpy(&line1[1], s.flareCount, COUNT_DISPLAY_LEN);
            line1[COUNT_DISPLAY_LEN + 1] = '\0';
            redrawTextLine(TEXT_X, TEXT_LINE1, line1);
        }
        if (line2Changed) {
            char line2[8];
            line2[0] = s.chaffLetter[0] ? s.chaffLetter[0] : ' ';
            memcpy(&line2[1], s.chaffCount, COUNT_DISPLAY_LEN);
            line2[COUNT_DISPLAY_LEN + 1] = '\0';
            redrawTextLine(TEXT_X, TEXT_LINE2, line2);
        }
    } else {
        // TEST mode: compare BIT lines
        const bool line1Changed = (memcmp(g_lastDrawn.bitLine1, s.bitLine1, BITLINE_DISPLAY_LEN + 1) != 0);
        const bool line2Changed = (memcmp(g_lastDrawn.bitLine2, s.bitLine2, BITLINE_DISPLAY_LEN + 1) != 0);

        if (line1Changed) redrawTextLine(TEXT_X, TEXT_LINE1, s.bitLine1);
        if (line2Changed) redrawTextLine(TEXT_X, TEXT_LINE2, s.bitLine2);
    }

    // --- UPDATE g_lastDrawn (all at end, after all comparisons) ---
    // Note: deviceOn and backlight intentionally not updated here:
    // - deviceOn: This path only runs when deviceOn=true (would have returned early otherwise)
    // - backlight: Handled separately via g_lastBrightness, not used in comparisons
    memcpy(g_lastDrawn.large, s.large, sizeof(g_lastDrawn.large));
    g_lastDrawn.dispense = s.dispense;
    g_lastDrawn.ready = s.ready;
    g_lastDrawn.pageMode = s.pageMode;
    memcpy(g_lastDrawn.flareLetter, s.flareLetter, LETTER_FIELD_SIZE);
    memcpy(g_lastDrawn.chaffLetter, s.chaffLetter, LETTER_FIELD_SIZE);
    memcpy(g_lastDrawn.flareCount, s.flareCount, COUNT_FIELD_SIZE);
    memcpy(g_lastDrawn.chaffCount, s.chaffCount, COUNT_FIELD_SIZE);
    memcpy(g_lastDrawn.bitLine1, s.bitLine1, BITLINE_FIELD_SIZE);
    memcpy(g_lastDrawn.bitLine2, s.bitLine2, BITLINE_FIELD_SIZE);

    tft.endWrite();

#if ENABLE_PROFILING
    debugPrintf("[CMWS] Incremental: %luµs, rects=%d, pixels=%lu\n", 
                micros() - profStart, dirty.n, totalPixels);
#endif
}

#else // !USE_REFACTORED_VERSION

// Original non-refactored version (kept for reference/comparison)
static void redrawRegion(const RectI16& r, const CmwsState& s) {
    RectI16 rr = r;
    if (rr.x < 0) { rr.w += rr.x; rr.x = 0; }
    if (rr.y < 0) { rr.h += rr.y; rr.y = 0; }
    if (rr.x + rr.w > SCREEN_W) rr.w = SCREEN_W - rr.x;
    if (rr.y + rr.h > SCREEN_H) rr.h = SCREEN_H - rr.y;
    if (rr.w <= 0 || rr.h <= 0) return;

    tft.fillRect(rr.x, rr.y, rr.w, rr.h, COL_BLACK);
    drawTicksIntersectingRect(rr, s.deviceOn);
    drawArrowsIntersectingRect(rr, s);
    drawDRIntersectingRect(rr, s);
}

static void CMWSDisplay_draw(bool force = false) {
    if (!g_displayInitialized) return;
    if (!force && !isMissionRunning()) return;
    
    // Capture and clear dirty flags at START of frame to prevent lost updates.
    // If a callback fires during render, it re-sets g_dirty for the NEXT frame.
    const bool wasDirty = g_dirty;
    const bool wasForceFull = g_forceFull;
    g_dirty = false;
    g_forceFull = false;
    
    if (!force && !wasDirty && !wasForceFull) return;

    const uint32_t now = millis();
    if (!force && !wasForceFull && (now - g_lastDrawMs < CMWS_REFRESH_INTERVAL_MS)) {
        // Restore flags if we're rate-limited (so next frame processes them)
        if (wasDirty) g_dirty = true;
        if (wasForceFull) g_forceFull = true;
        return;
    }
    g_lastDrawMs = now;

    if (g_deviceStateStale) {
        g_deviceStateStale = false;
        recomputeDeviceState();
    }

    CmwsState s;
    snapshotState(s);

    const uint8_t effectiveBacklight = s.deviceOn ? s.backlight : 0;
    if (effectiveBacklight != g_lastBrightness) {
        tft.setBrightness(effectiveBacklight);
        g_lastBrightness = effectiveBacklight;
    }

    if (!s.deviceOn) {
        if (wasForceFull || g_lastDrawn.deviceOn) {
            tft.startWrite();
            tft.fillScreen(COL_BLACK);
            tft.endWrite();
        }
        g_lastDrawn = s;
        return;
    }

    tft.startWrite();

    if (wasForceFull || !g_lastDrawn.deviceOn || (s.pageMode != g_lastDrawn.pageMode)) {
        fullRedraw(s);
        g_lastDrawn = s;
        tft.endWrite();
        return;
    }

    // Incremental update
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) {
        if (s.large[i] != g_lastDrawn.large[i]) {
            redrawRegion(g_largeAabb[i], s);
            g_lastDrawn.large[i] = s.large[i];
        }
    }

    if (s.dispense != g_lastDrawn.dispense) {
        redrawRegion(g_dRect, s);
        g_lastDrawn.dispense = s.dispense;
    }
    if (s.ready != g_lastDrawn.ready) {
        redrawRegion(g_rRect, s);
        g_lastDrawn.ready = s.ready;
    }

    // Text
    tft.setFont(FONT_DOTO);
    tft.setTextColor(COL_GREEN);
    tft.setTextDatum(textdatum_t::top_left);

    char line1[24], line2[24], prev1[24], prev2[24];

    if (s.pageMode == PageMode::MAIN) {
        snprintf(line1, sizeof(line1), "%s%s", s.flareLetter, s.flareCount);
        snprintf(line2, sizeof(line2), "%s%s", s.chaffLetter, s.chaffCount);
    } else {
        memcpy(line1, s.bitLine1, sizeof(s.bitLine1));
        memcpy(line2, s.bitLine2, sizeof(s.bitLine2));
    }

    if (g_lastDrawn.pageMode == PageMode::MAIN) {
        snprintf(prev1, sizeof(prev1), "%s%s", g_lastDrawn.flareLetter, g_lastDrawn.flareCount);
        snprintf(prev2, sizeof(prev2), "%s%s", g_lastDrawn.chaffLetter, g_lastDrawn.chaffCount);
    } else {
        memcpy(prev1, g_lastDrawn.bitLine1, sizeof(g_lastDrawn.bitLine1));
        memcpy(prev2, g_lastDrawn.bitLine2, sizeof(g_lastDrawn.bitLine2));
    }

    if (strcmp(line1, prev1) != 0) redrawTextLine(TEXT_X, TEXT_LINE1, line1);
    if (strcmp(line2, prev2) != 0) redrawTextLine(TEXT_X, TEXT_LINE2, line2);

    g_lastDrawn.pageMode = s.pageMode;
    memcpy(g_lastDrawn.flareLetter, s.flareLetter, LETTER_FIELD_SIZE);
    memcpy(g_lastDrawn.chaffLetter, s.chaffLetter, LETTER_FIELD_SIZE);
    memcpy(g_lastDrawn.flareCount, s.flareCount, COUNT_FIELD_SIZE);
    memcpy(g_lastDrawn.chaffCount, s.chaffCount, COUNT_FIELD_SIZE);
    memcpy(g_lastDrawn.bitLine1, s.bitLine1, BITLINE_FIELD_SIZE);
    memcpy(g_lastDrawn.bitLine2, s.bitLine2, BITLINE_FIELD_SIZE);

    tft.endWrite();
}

#endif // USE_REFACTORED_VERSION

// =============================================================================
// DCS-BIOS CALLBACKS (DEFERRED STATE RECOMPUTATION)
// =============================================================================

static inline void markDirty() { g_dirty = true; }

static inline void markDeviceStateStale() {
    g_deviceStateStale = true;
    g_dirty = true;
}

// Large arrow callbacks (deferred pattern)
static void onFwdRightBrt(const char*, uint16_t v) { g_largeBrt[0] = (v != 0); markDeviceStateStale(); }
static void onFwdRightDim(const char*, uint16_t v) { g_largeDim[0] = (v != 0); markDeviceStateStale(); }

static void onAftRightBrt(const char*, uint16_t v) { g_largeBrt[1] = (v != 0); markDeviceStateStale(); }
static void onAftRightDim(const char*, uint16_t v) { g_largeDim[1] = (v != 0); markDeviceStateStale(); }

static void onAftLeftBrt(const char*, uint16_t v)  { g_largeBrt[2] = (v != 0); markDeviceStateStale(); }
static void onAftLeftDim(const char*, uint16_t v)  { g_largeDim[2] = (v != 0); markDeviceStateStale(); }

static void onFwdLeftBrt(const char*, uint16_t v)  { g_largeBrt[3] = (v != 0); markDeviceStateStale(); }
static void onFwdLeftDim(const char*, uint16_t v)  { g_largeDim[3] = (v != 0); markDeviceStateStale(); }

// D/R callbacks (deferred pattern)
static void onDispenseBrt(const char*, uint16_t v) { g_dispBrt = (v != 0); markDeviceStateStale(); }
static void onDispenseDim(const char*, uint16_t v) { g_dispDim = (v != 0); markDeviceStateStale(); }
static void onReadyBrt(const char*, uint16_t v)    { g_readyBrt = (v != 0); markDeviceStateStale(); }
static void onReadyDim(const char*, uint16_t v)    { g_readyDim = (v != 0); markDeviceStateStale(); }

// Inventory / BIT strings
static void onFlareCount(const char*, const char* value) {
    if (!value) return;
    const size_t len = strlen(value);
    if (len < 2) return;

    portENTER_CRITICAL(&g_stateMux);
    if (memcmp(g_pending.flareCount, value, COUNT_DISPLAY_LEN) != 0) {
        memcpy(g_pending.flareCount, value, COUNT_DISPLAY_LEN);
        g_pending.flareCount[COUNT_DISPLAY_LEN] = '\0';
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}

static void onChaffCount(const char*, const char* value) {
    if (!value) return;
    const size_t len = strlen(value);
    if (len < 2) return;

    portENTER_CRITICAL(&g_stateMux);
    if (memcmp(g_pending.chaffCount, value, COUNT_DISPLAY_LEN) != 0) {
        memcpy(g_pending.chaffCount, value, COUNT_DISPLAY_LEN);
        g_pending.chaffCount[COUNT_DISPLAY_LEN] = '\0';
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
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}

static void onBitLine1(const char*, const char* value) {
    if (!value) return;
    const size_t len = strlen(value);
    if (len < 3) return;

    char norm[5];
    formatField4(norm, value);

    portENTER_CRITICAL(&g_stateMux);
    if (memcmp(g_pending.bitLine1, norm, BITLINE_DISPLAY_LEN) != 0) {
        memcpy(g_pending.bitLine1, norm, BITLINE_DISPLAY_LEN + 1);
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}

static void onBitLine2(const char*, const char* value) {
    if (!value) return;
    const size_t len = strlen(value);
    if (len < 3) return;

    char norm[5];
    formatField4(norm, value);

    portENTER_CRITICAL(&g_stateMux);
    if (memcmp(g_pending.bitLine2, norm, BITLINE_DISPLAY_LEN) != 0) {
        memcpy(g_pending.bitLine2, norm, BITLINE_DISPLAY_LEN + 1);
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}

static void onPage(const char*, const char* value) {
    if (!value) return;

    PageMode newMode;
    if (strcmp(value, "MAIN") == 0) {
        newMode = PageMode::MAIN;
    } else if (strcmp(value, "TEST") == 0) {
        newMode = PageMode::TEST;
    } else {
        return;  // Unknown page, ignore
    }

    portENTER_CRITICAL(&g_stateMux);
    if (g_pending.pageMode != newMode) {
        g_pending.pageMode = newMode;
        portEXIT_CRITICAL(&g_stateMux);
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

    portENTER_CRITICAL(&g_stateMux);
    if (g_pending.backlight != newLevel) {
        g_pending.backlight = newLevel;
        portEXIT_CRITICAL(&g_stateMux);
        markDirty();
        return;
    }
    portEXIT_CRITICAL(&g_stateMux);
}

// =============================================================================
// TASK (cooperative shutdown, deterministic frame pacing)
// =============================================================================

#if RUN_AS_TASK
static void CMWSDisplay_task(void*) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    
    while (!g_taskStopRequested) {
        CMWSDisplay_draw(false);
        
        // Sleep until next frame boundary (deterministic timing)
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(CMWS_REFRESH_INTERVAL_MS));
    }
    
    // Task exits cleanly - deinit is waiting for us
    taskHandle = nullptr;
    vTaskDelete(nullptr);
}
#endif

// =============================================================================
// PUBLIC API
// =============================================================================

void CMWSDisplay_init() {
    platformInit();
    precomputeGeometry();

    // Initialize display (LovyanGFX init() is typically void, handle gracefully)
    tft.init();
    
    // Verify display is responding by checking if we can set rotation
    // (This is a lightweight sanity check since init() doesn't return status)
    tft.setRotation(3);
    tft.setColorDepth(16);
    tft.setSwapBytes(true);
    tft.setBrightness(255);
    tft.fillScreen(COL_BLACK);

    g_displayInitialized = true;

    // Initialize state
    CmwsState init{};
    init.pageMode = PageMode::MAIN;
    init.deviceOn = false;
    init.backlight = 255;

    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) init.large[i] = ElemState::OFF;
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) init.small[i] = ElemState::DIM;
    init.dispense = ElemState::OFF;
    init.ready    = ElemState::OFF;

    // Fixed-width field initialization (explicit null termination)
    init.flareLetter[0] = 'F'; init.flareLetter[1] = '\0';
    init.chaffLetter[0] = 'C'; init.chaffLetter[1] = '\0';
    memcpy(init.flareCount, " 00", 4);
    memcpy(init.chaffCount, " 00", 4);
    init.bitLine1[0] = '\0';
    init.bitLine2[0] = '\0';

    portENTER_CRITICAL(&g_stateMux);
    g_pending = init;
    portEXIT_CRITICAL(&g_stateMux);

    g_lastDrawn = init;
    g_forceFull = true;
    g_dirty = true;
    g_deviceStateStale = false;
    g_taskStopRequested = false;
    g_lastBrightness = 0xFF;

    // DCS-BIOS subscriptions
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
    xTaskCreatePinnedToCore(CMWSDisplay_task, "CMWSTask", TASK_STACK_SIZE, nullptr, TASK_PRIORITY, 
                            const_cast<TaskHandle_t*>(&taskHandle), CPU_CORE);
#endif

#if defined(CMWS_USE_SPI_INTERFACE)
    debugPrintf("✅ CMWS Display (SPI): MOSI=%d SCLK=%d CS=%d DC=%d RST=%d BLK=%d\n",
        PIN_MOSI, PIN_SCLK, PIN_CS, PIN_DC, PIN_RST, PIN_BLK);
#elif defined(CMWS_USE_PARALLEL_INTERFACE)
    debugPrintf("✅ CMWS Display (8-bit Parallel): WR=%d DC=%d CS=%d RST=%d BLK=%d PWR=%d\n",
        PIN_WR, PIN_DC, PIN_CS, PIN_RST, PIN_BLK, PIN_POWER);
#endif
    debugPrintf("   Radius=%d, SmallArrows=%d, Ticks=%d, StateSize=%u bytes\n",
        g_largeArrowRadius, SMALL_ARROW_COUNT, TICK_COUNT, (unsigned)sizeof(CmwsState));
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
        // Request cooperative shutdown and wait for task to exit cleanly
        // This prevents killing the task mid-transaction (would wedge SPI bus)
        g_taskStopRequested = true;
        
        // Wait for task to acknowledge and exit (max ~3 frame times)
        // Use subtraction pattern for wrap-safe timing (handles millis() overflow after ~49 days)
        const uint32_t start = millis();
        while (taskHandle != nullptr && (millis() - start) < (CMWS_REFRESH_INTERVAL_MS * 3)) {
            vTaskDelay(pdMS_TO_TICKS(5));
        }
        
        // If task didn't exit cleanly, force delete (shouldn't happen)
        if (taskHandle) {
            vTaskDelete(taskHandle);
            taskHandle = nullptr;
        }
        
        g_taskStopRequested = false;  // Reset for potential re-init
    }
#endif

    if (g_displayInitialized) {
        tft.fillScreen(COL_BLACK);
        tft.setBrightness(0);
    }

#if defined(CMWS_USE_PARALLEL_INTERFACE)
    digitalWrite(PIN_POWER, LOW);
#endif

    g_displayInitialized = false;
}

// =============================================================================
// BIT TEST (blocking self-test)
// WARNING: Only call during init (before task creation) or after deinit.
//          Calling while render task is running will cause display corruption.
// =============================================================================

void CMWSDisplay_bitTest() {
    if (!g_displayInitialized) return;

    CmwsState saved;
    snapshotState(saved);

    CmwsState tmp = saved;
    tmp.pageMode = PageMode::MAIN;
    tmp.deviceOn = true;
    tmp.backlight = 255;
    memcpy(tmp.flareCount, " 88", 4);
    memcpy(tmp.chaffCount, " 88", 4);

    // Phase 1: All bright
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) tmp.large[i] = ElemState::BRT;
    for (int i = 0; i < SMALL_ARROW_COUNT; ++i) tmp.small[i] = ElemState::DIM;
    tmp.dispense = ElemState::BRT;
    tmp.ready    = ElemState::BRT;

    portENTER_CRITICAL(&g_stateMux);
    g_pending = tmp;
    portEXIT_CRITICAL(&g_stateMux);
    g_forceFull = true; g_dirty = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Phase 2: All dim
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) tmp.large[i] = ElemState::DIM;
    tmp.dispense = ElemState::DIM;
    tmp.ready    = ElemState::DIM;

    portENTER_CRITICAL(&g_stateMux);
    g_pending = tmp;
    portEXIT_CRITICAL(&g_stateMux);
    g_forceFull = true; g_dirty = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Phase 3: Device OFF
    tmp.deviceOn = false;
    for (int i = 0; i < LARGE_ARROW_COUNT; ++i) tmp.large[i] = ElemState::OFF;
    tmp.dispense = ElemState::OFF;
    tmp.ready    = ElemState::OFF;

    portENTER_CRITICAL(&g_stateMux);
    g_pending = tmp;
    portEXIT_CRITICAL(&g_stateMux);
    g_forceFull = true; g_dirty = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Phase 4: Rotate large arrows
    tmp.deviceOn = true;
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

    // Restore
    portENTER_CRITICAL(&g_stateMux);
    g_pending = saved;
    portEXIT_CRITICAL(&g_stateMux);

    g_forceFull = true;
    g_dirty = true;
    CMWSDisplay_draw(true);
}

#else // !HAS_CMWS_DISPLAY || !ENABLE_TFT_GAUGES

void CMWSDisplay_init() {}
void CMWSDisplay_loop() {}
void CMWSDisplay_deinit() {}
void CMWSDisplay_notifyMissionStart() {}
void CMWSDisplay_bitTest() {}

#endif
