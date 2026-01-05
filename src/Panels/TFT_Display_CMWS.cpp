// TFT_Display_CMWS.cpp — CockpitOS CMWS Threat Ring Display (LovyanGFX, ST7789 @ 320×170)
// AH-64D Apache Countermeasures Warning System Display
//
// Hardware: IdeasPark ST7789 170×320 TFT
// CRITICAL: TFT_RST must be GPIO 4 (not -1) for this display!

#include "../Globals.h"

#if defined(HAS_CMWS_DISPLAY) && defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1)

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "includes/TFT_Display_CMWS.h"

// =============================================================================
// LIBRARY CHECK
// =============================================================================
#if !__has_include(<LovyanGFX.hpp>)
#error "❌ Missing LovyanGFX.hpp — Please install LovyanGFX library"
#endif

#include <LovyanGFX.hpp>
#include <lgfx/v1/LGFX_Sprite.hpp>
#include <cstring>
#include <cmath>

// =============================================================================
// PANEL REGISTRATION
// =============================================================================
#if defined(HAS_CMWS_DISPLAY)
    REGISTER_PANEL(TFTCmws, nullptr, nullptr, CMWSDisplay_init, CMWSDisplay_loop, nullptr, 100);
#endif

// =============================================================================
// CONFIGURATION
// =============================================================================
#define CMWS_DISPLAY_REFRESH_RATE_MS    33      // ~30 FPS max
#define RUN_CMWS_DISPLAY_AS_TASK        1
#define CMWS_TASK_STACK_SIZE            4096
#define CMWS_TASK_PRIORITY              2
#define CMWS_CPU_CORE                   0

// =============================================================================
// PIN DEFINITIONS — ADJUST FOR YOUR WIRING
// =============================================================================
#if defined(HAS_CMWS_DISPLAY) && defined(ESP_FAMILY_CLASSIC)
    #define CMWS_TFT_MOSI   PIN(23)
    #define CMWS_TFT_SCLK   PIN(18)
    #define CMWS_TFT_CS     PIN(15)
    #define CMWS_TFT_DC     PIN(2)
    #define CMWS_TFT_RST    PIN(4)      // MUST BE 4 for IdeasPark ST7789!
    #define CMWS_TFT_BLK    PIN(32)
#else
    #define CMWS_TFT_MOSI   -1
    #define CMWS_TFT_SCLK   -1
    #define CMWS_TFT_CS     -1
    #define CMWS_TFT_DC     -1
    #define CMWS_TFT_RST    -1
    #define CMWS_TFT_BLK    -1
#endif

// =============================================================================
// DISPLAY GEOMETRY
// =============================================================================
static constexpr int16_t SCREEN_WIDTH   = 320;
static constexpr int16_t SCREEN_HEIGHT  = 170;

static constexpr int16_t RING_CENTER_X      = 235;
static constexpr int16_t RING_CENTER_Y      = 85;
static constexpr int16_t TICK_INNER_RADIUS  = 66;
static constexpr int16_t TICK_OUTER_RADIUS  = 76;

static constexpr int     TICK_COUNT     = 24;
static constexpr float   TICK_STEP_DEG  = 360.0f / static_cast<float>(TICK_COUNT);

static constexpr int16_t INVENTORY_MARGIN_X     = 5;
static constexpr int16_t INVENTORY_MARGIN_LINE1 = 40;
static constexpr int16_t INVENTORY_MARGIN_LINE2 = 95;

static constexpr int16_t DR_OFFSET = 25;

// =============================================================================
// ARROW GEOMETRY
// =============================================================================
static constexpr float LARGE_TIP_Y       = 30.0f;
static constexpr float LARGE_TIP_BASE_Y  = 11.0f;
static constexpr float LARGE_BODY_BASE_Y = 0.0f;
static constexpr float LARGE_TIP_HALF_W  = 16.0f;
static constexpr float LARGE_BODY_HALF_W = 8.5f;
static constexpr float ARROW_GLOBAL_SCALE = 1.0f;

// =============================================================================
// COLORS (RGB565)
// =============================================================================
static constexpr uint16_t COLOR_BLACK       = 0x0000;
static constexpr uint16_t COLOR_AMBER_BRT   = 0xFDE0;
static constexpr uint16_t COLOR_AMBER_DIM   = 0x8400;
static constexpr uint16_t COLOR_GREEN       = 0x07E0;

// =============================================================================
// ELEMENT STATE
// =============================================================================
enum class ElementState : uint8_t {
    OFF = 0,
    DIM = 1,
    BRT = 2
};

// =============================================================================
// SPI CONFIGURATION
// =============================================================================
#if defined(ESP_FAMILY_CLASSIC)
static constexpr spi_host_device_t spi_host = VSPI_HOST;
#else
static constexpr spi_host_device_t spi_host = SPI2_HOST;
#endif

// =============================================================================
// LOVYANGFX PANEL CONFIG
// =============================================================================
class LGFX_CMWS final : public lgfx::LGFX_Device {
    lgfx::Bus_SPI       _bus;
    lgfx::Panel_ST7789  _panel;
    lgfx::Light_PWM     _light;

public:
    LGFX_CMWS() {
        {
            auto cfg = _bus.config();
            cfg.spi_host    = spi_host;
            cfg.spi_mode    = 0;
            cfg.freq_write  = 80000000;
            cfg.freq_read   = 16000000;
            cfg.spi_3wire   = false;
            cfg.use_lock    = false;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_mosi    = CMWS_TFT_MOSI;
            cfg.pin_miso    = -1;
            cfg.pin_sclk    = CMWS_TFT_SCLK;
            cfg.pin_dc      = CMWS_TFT_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg = _panel.config();
            cfg.pin_cs          = CMWS_TFT_CS;
            cfg.pin_rst         = CMWS_TFT_RST;
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
            cfg.pin_bl      = CMWS_TFT_BLK;
            cfg.invert      = false;
            cfg.freq        = 12000;
            cfg.pwm_channel = 7;
            _light.config(cfg);
            _panel.setLight(&_light);
        }
        setPanel(&_panel);
    }
};

// =============================================================================
// STATIC STATE
// =============================================================================
static LGFX_CMWS tft;

static volatile uint8_t displayPage = 1;    // 0=NONE, 1=MAIN, 2=TEST

// Arrow states: 0=FWD_RIGHT(45°), 1=AFT_RIGHT(135°), 2=AFT_LEFT(225°), 3=FWD_LEFT(315°)
static volatile ElementState arrowState[4] = {
    ElementState::DIM, ElementState::DIM, ElementState::DIM, ElementState::DIM
};
static constexpr float ARROW_ANGLES[4] = { 45.0f, 135.0f, 225.0f, 315.0f };

static volatile ElementState dispenseState = ElementState::DIM;
static volatile ElementState readyState    = ElementState::DIM;

static char flareCount[4]  = "00";
static char chaffCount[4]  = "00";
static char flareLetter[2] = "F";
static char chaffLetter[2] = "C";

static volatile uint8_t backlightLevel = 255;
static volatile bool displayDirty      = true;
static volatile bool needsFullRedraw   = true;

static uint32_t lastDrawTime = 0;
static TaskHandle_t cmwsTaskHandle = nullptr;

// =============================================================================
// MATH HELPERS
// =============================================================================
static inline void unitForward(float angleDeg, float& fx, float& fy) {
    const float rad = angleDeg * DEG_TO_RAD;
    fx = sinf(rad);
    fy = -cosf(rad);
}

static inline void pointFromCenter(float angleDeg, float radius, int16_t& x, int16_t& y) {
    float fx, fy;
    unitForward(angleDeg, fx, fy);
    x = static_cast<int16_t>(RING_CENTER_X + fx * radius);
    y = static_cast<int16_t>(RING_CENTER_Y + fy * radius);
}

// =============================================================================
// ARROW RADIUS CALCULATION
// =============================================================================
static float maxCenterRadiusForTipOnScreen(float angleDeg) {
    float fx, fy;
    unitForward(angleDeg, fx, fy);
    const float tipOffset = LARGE_TIP_Y * ARROW_GLOBAL_SCALE;

    float tMaxX = 1e9f;
    if (fx > 0.0001f) {
        tMaxX = (static_cast<float>(SCREEN_WIDTH - 1 - RING_CENTER_X) / fx) - tipOffset;
    } else if (fx < -0.0001f) {
        tMaxX = (static_cast<float>(0 - RING_CENTER_X) / fx) - tipOffset;
    }

    float tMaxY = 1e9f;
    if (fy > 0.0001f) {
        tMaxY = (static_cast<float>(SCREEN_HEIGHT - 1 - RING_CENTER_Y) / fy) - tipOffset;
    } else if (fy < -0.0001f) {
        tMaxY = (static_cast<float>(0 - RING_CENTER_Y) / fy) - tipOffset;
    }

    const float tMax = (tMaxX < tMaxY) ? tMaxX : tMaxY;
    return (tMax > 0.0f) ? tMax : 0.0f;
}

static int computeLargeArrowRadiusSymmetric() {
    float r = 1e9f;
    for (int i = 0; i < 4; ++i) {
        const float ri = maxCenterRadiusForTipOnScreen(ARROW_ANGLES[i]);
        if (ri < r) r = ri;
    }
    return (r > 0) ? static_cast<int>(r) : 0;
}

// =============================================================================
// DRAWING: PENTAGON ARROW
// =============================================================================
static void drawPentagonArrow(int16_t cx, int16_t cy, float angleDeg, float scale, uint16_t color) {
    float fwdX, fwdY;
    unitForward(angleDeg, fwdX, fwdY);

    const float rightX = fwdY;
    const float rightY = -fwdX;

    const float tipY      = LARGE_TIP_Y * scale;
    const float tipBaseY  = LARGE_TIP_BASE_Y * scale;
    const float bodyBaseY = LARGE_BODY_BASE_Y * scale;
    const float tipHalfW  = LARGE_TIP_HALF_W * scale;
    const float bodyHalfW = LARGE_BODY_HALF_W * scale;

    const float lx[6] = { 0.0f, -tipHalfW, +tipHalfW, -bodyHalfW, +bodyHalfW, -bodyHalfW };
    const float ly[6] = { tipY, tipBaseY, tipBaseY, bodyBaseY, bodyBaseY, tipBaseY };

    int16_t sx[6], sy[6];
    for (int i = 0; i < 6; ++i) {
        const float wx = (lx[i] * rightX) + (ly[i] * fwdX);
        const float wy = (lx[i] * rightY) + (ly[i] * fwdY);
        sx[i] = static_cast<int16_t>(cx + (wx >= 0 ? (wx + 0.5f) : (wx - 0.5f)));
        sy[i] = static_cast<int16_t>(cy + (wy >= 0 ? (wy + 0.5f) : (wy - 0.5f)));
    }

    const float wx6 = (bodyHalfW * rightX) + (tipBaseY * fwdX);
    const float wy6 = (bodyHalfW * rightY) + (tipBaseY * fwdY);
    const int16_t sx6 = static_cast<int16_t>(cx + (wx6 >= 0 ? (wx6 + 0.5f) : (wx6 - 0.5f)));
    const int16_t sy6 = static_cast<int16_t>(cy + (wy6 >= 0 ? (wy6 + 0.5f) : (wy6 - 0.5f)));

    tft.fillTriangle(sx[0], sy[0], sx[1], sy[1], sx[2], sy[2], color);

    if (bodyBaseY < tipBaseY) {
        tft.fillTriangle(sx[5], sy[5], sx6, sy6, sx[4], sy[4], color);
        tft.fillTriangle(sx[5], sy[5], sx[4], sy[4], sx[3], sy[3], color);
    }
}

// =============================================================================
// DRAWING: TICK MARK
// =============================================================================
static void drawTickMark(float angleDeg, uint16_t color) {
    float fx, fy;
    unitForward(angleDeg, fx, fy);

    const int16_t x1 = static_cast<int16_t>(RING_CENTER_X + fx * TICK_INNER_RADIUS);
    const int16_t y1 = static_cast<int16_t>(RING_CENTER_Y + fy * TICK_INNER_RADIUS);
    const int16_t x2 = static_cast<int16_t>(RING_CENTER_X + fx * TICK_OUTER_RADIUS);
    const int16_t y2 = static_cast<int16_t>(RING_CENTER_Y + fy * TICK_OUTER_RADIUS);

    tft.drawLine(x1, y1, x2, y2, color);
}

// =============================================================================
// DRAWING: FULL DISPLAY
// =============================================================================
static void CMWSDisplay_draw(bool force = false) {
    if (!force && !isMissionRunning()) return;
    if (!force && !displayDirty && !needsFullRedraw) return;

    const uint32_t now = millis();
    if (!force && !needsFullRedraw && (now - lastDrawTime < CMWS_DISPLAY_REFRESH_RATE_MS)) return;

    lastDrawTime = now;
    displayDirty = false;

    // Display OFF
    if (displayPage == 0) {
        tft.fillScreen(COLOR_BLACK);
        needsFullRedraw = false;
        return;
    }

    tft.startWrite();
    tft.fillScreen(COLOR_BLACK);

    // Inventory (left side)
    tft.setFont(&fonts::Orbitron_Light_32);
    tft.setTextColor(COLOR_GREEN);
    tft.setTextDatum(textdatum_t::top_left);

    tft.setCursor(INVENTORY_MARGIN_X, INVENTORY_MARGIN_LINE1);
    tft.printf("%s    %s", flareLetter, flareCount);

    tft.setCursor(INVENTORY_MARGIN_X, INVENTORY_MARGIN_LINE2);
    tft.printf("%s    %s", chaffLetter, chaffCount);

    // Tick marks
    for (int i = 0; i < TICK_COUNT; ++i) {
        const float angle = static_cast<float>(i) * TICK_STEP_DEG;
        const int angleInt = static_cast<int>(angle + 0.5f);
        if (angleInt == 45 || angleInt == 135 || angleInt == 225 || angleInt == 315) continue;
        drawTickMark(angle, COLOR_AMBER_DIM);
    }

    // Large arrows
    const int largeRadius = computeLargeArrowRadiusSymmetric();
    for (int i = 0; i < 4; ++i) {
        const ElementState state = arrowState[i];
        if (state == ElementState::OFF) continue;
        const uint16_t color = (state == ElementState::BRT) ? COLOR_AMBER_BRT : COLOR_AMBER_DIM;
        int16_t ax, ay;
        pointFromCenter(ARROW_ANGLES[i], static_cast<float>(largeRadius), ax, ay);
        drawPentagonArrow(ax, ay, ARROW_ANGLES[i], ARROW_GLOBAL_SCALE, color);
    }

    // D/R indicators
    tft.setFont(&fonts::DejaVu24);
    tft.setTextDatum(textdatum_t::middle_center);

    if (dispenseState != ElementState::OFF) {
        tft.setTextColor((dispenseState == ElementState::BRT) ? COLOR_AMBER_BRT : COLOR_AMBER_DIM);
        tft.drawString("D", RING_CENTER_X, RING_CENTER_Y - DR_OFFSET);
    }
    if (readyState != ElementState::OFF) {
        tft.setTextColor((readyState == ElementState::BRT) ? COLOR_AMBER_BRT : COLOR_AMBER_DIM);
        tft.drawString("R", RING_CENTER_X, RING_CENTER_Y + DR_OFFSET);
    }

    tft.endWrite();
    needsFullRedraw = false;
}

// =============================================================================
// HELPER: DECODE BRT/DIM TO STATE
// =============================================================================
static inline ElementState decodeState(bool brt, bool dim) {
    if (brt) return ElementState::BRT;
    if (dim) return ElementState::DIM;
    return ElementState::OFF;
}

// =============================================================================
// DCS-BIOS CALLBACKS — METADATA (BRT/DIM bits)
// =============================================================================
static void onFwdRightBrt(const char*, uint16_t v) {
    bool brt = (v != 0);
    bool dim = (getMetadataValue("PLT_CMWS_FWD_RIGHT_DIM_L") != 0);
    arrowState[0] = decodeState(brt, dim);
    displayDirty = true;
}
static void onFwdRightDim(const char*, uint16_t v) {
    bool brt = (getMetadataValue("PLT_CMWS_FWD_RIGHT_BRT_L") != 0);
    arrowState[0] = decodeState(brt, v != 0);
    displayDirty = true;
}
static void onAftRightBrt(const char*, uint16_t v) {
    bool brt = (v != 0);
    bool dim = (getMetadataValue("PLT_CMWS_AFT_RIGHT_DIM_L") != 0);
    arrowState[1] = decodeState(brt, dim);
    displayDirty = true;
}
static void onAftRightDim(const char*, uint16_t v) {
    bool brt = (getMetadataValue("PLT_CMWS_AFT_RIGHT_BRT_L") != 0);
    arrowState[1] = decodeState(brt, v != 0);
    displayDirty = true;
}
static void onAftLeftBrt(const char*, uint16_t v) {
    bool brt = (v != 0);
    bool dim = (getMetadataValue("PLT_CMWS_AFT_LEFT_DIM_L") != 0);
    arrowState[2] = decodeState(brt, dim);
    displayDirty = true;
}
static void onAftLeftDim(const char*, uint16_t v) {
    bool brt = (getMetadataValue("PLT_CMWS_AFT_LEFT_BRT_L") != 0);
    arrowState[2] = decodeState(brt, v != 0);
    displayDirty = true;
}
static void onFwdLeftBrt(const char*, uint16_t v) {
    bool brt = (v != 0);
    bool dim = (getMetadataValue("PLT_CMWS_FWD_LEFT_DIM_L") != 0);
    arrowState[3] = decodeState(brt, dim);
    displayDirty = true;
}
static void onFwdLeftDim(const char*, uint16_t v) {
    bool brt = (getMetadataValue("PLT_CMWS_FWD_LEFT_BRT_L") != 0);
    arrowState[3] = decodeState(brt, v != 0);
    displayDirty = true;
}

static void onDispenseBrt(const char*, uint16_t v) {
    bool brt = (v != 0);
    bool dim = (getMetadataValue("PLT_CMWS_D_DIM_L") != 0);
    dispenseState = decodeState(brt, dim);
    displayDirty = true;
}
static void onDispenseDim(const char*, uint16_t v) {
    bool brt = (getMetadataValue("PLT_CMWS_D_BRT_L") != 0);
    dispenseState = decodeState(brt, v != 0);
    displayDirty = true;
}
static void onReadyBrt(const char*, uint16_t v) {
    bool brt = (v != 0);
    bool dim = (getMetadataValue("PLT_CMWS_R_DIM_L") != 0);
    readyState = decodeState(brt, dim);
    displayDirty = true;
}
static void onReadyDim(const char*, uint16_t v) {
    bool brt = (getMetadataValue("PLT_CMWS_R_BRT_L") != 0);
    readyState = decodeState(brt, v != 0);
    displayDirty = true;
}

// =============================================================================
// DCS-BIOS CALLBACKS — DISPLAY STRINGS
// =============================================================================
static void onFlareCount(const char*, const char* value) {
    if (value && strncmp(flareCount, value, 3) != 0) {
        strncpy(flareCount, value, 3);
        flareCount[3] = '\0';
        displayDirty = true;
    }
}
static void onChaffCount(const char*, const char* value) {
    if (value && strncmp(chaffCount, value, 3) != 0) {
        strncpy(chaffCount, value, 3);
        chaffCount[3] = '\0';
        displayDirty = true;
    }
}
static void onFlareLetter(const char*, const char* value) {
    if (value && value[0] != flareLetter[0]) {
        flareLetter[0] = value[0];
        displayDirty = true;
    }
}
static void onChaffLetter(const char*, const char* value) {
    if (value && value[0] != chaffLetter[0]) {
        chaffLetter[0] = value[0];
        displayDirty = true;
    }
}
static void onPage(const char*, const char* value) {
    uint8_t newPage = 1;
    if (value) {
        if (strcmp(value, "NONE") == 0) newPage = 0;
        else if (strcmp(value, "TEST") == 0) newPage = 2;
    }
    if (newPage != displayPage) {
        displayPage = newPage;
        needsFullRedraw = true;
        displayDirty = true;
    }
}

// =============================================================================
// DCS-BIOS CALLBACK — LAMP BRIGHTNESS
// =============================================================================
static void onLampChange(const char*, uint16_t value, uint16_t maxValue) {
    uint8_t newLevel = static_cast<uint8_t>((static_cast<uint32_t>(value) * 255UL) / maxValue);
    if (newLevel != backlightLevel) {
        backlightLevel = newLevel;
        tft.setBrightness(backlightLevel);
    }
}

// =============================================================================
// TASK
// =============================================================================
#if RUN_CMWS_DISPLAY_AS_TASK
static void CMWSDisplay_task(void*) {
    for (;;) {
        CMWSDisplay_draw(false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
#endif

// =============================================================================
// API: INIT
// =============================================================================
void CMWSDisplay_init() {
    tft.init();
    tft.setRotation(3);
    tft.setColorDepth(16);
    tft.setSwapBytes(true);
    tft.setBrightness(255);
    tft.fillScreen(COLOR_BLACK);

    // Subscribe to metadata (BRT/DIM bits)
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

    // Subscribe to display strings
    subscribeToDisplayChange("PLT_CMWS_FLARE_COUNT", onFlareCount);
    subscribeToDisplayChange("PLT_CMWS_CHAFF_COUNT", onChaffCount);
    subscribeToDisplayChange("PLT_CMWS_FLARE_LETTER", onFlareLetter);
    subscribeToDisplayChange("PLT_CMWS_CHAFF_LETTER", onChaffLetter);
    subscribeToDisplayChange("PLT_CMWS_PAGE", onPage);

    // Subscribe to lamp brightness
    subscribeToLedChange("PLT_CMWS_LAMP", onLampChange);

    // Initial draw
    needsFullRedraw = true;
    displayDirty = true;
    CMWSDisplay_draw(true);

    // BIT test
    CMWSDisplay_bitTest();

#if RUN_CMWS_DISPLAY_AS_TASK
    xTaskCreatePinnedToCore(CMWSDisplay_task, "CMWSDisplayTask", CMWS_TASK_STACK_SIZE, nullptr, CMWS_TASK_PRIORITY, &cmwsTaskHandle, CMWS_CPU_CORE);
#endif

    debugPrintf("✅ CMWS Display initialized: MOSI=%d, SCLK=%d, CS=%d, DC=%d, RST=%d, BLK=%d\n",
        CMWS_TFT_MOSI, CMWS_TFT_SCLK, CMWS_TFT_CS, CMWS_TFT_DC, CMWS_TFT_RST, CMWS_TFT_BLK);
}

// =============================================================================
// API: LOOP
// =============================================================================
void CMWSDisplay_loop() {
#if !RUN_CMWS_DISPLAY_AS_TASK
    CMWSDisplay_draw(false);
#endif
}

// =============================================================================
// API: MISSION START
// =============================================================================
void CMWSDisplay_notifyMissionStart() {
    needsFullRedraw = true;
    displayDirty = true;
}

// =============================================================================
// API: DEINIT
// =============================================================================
void CMWSDisplay_deinit() {
#if RUN_CMWS_DISPLAY_AS_TASK
    if (cmwsTaskHandle) {
        vTaskDelete(cmwsTaskHandle);
        cmwsTaskHandle = nullptr;
    }
#endif
    tft.fillScreen(COLOR_BLACK);
    tft.setBrightness(0);
}

// =============================================================================
// API: BIT TEST
// =============================================================================
void CMWSDisplay_bitTest() {
    const uint8_t savedPage = displayPage;
    const uint8_t savedBacklight = backlightLevel;

    displayPage = 1;
    tft.setBrightness(255);

    // All bright
    for (int i = 0; i < 4; ++i) arrowState[i] = ElementState::BRT;
    dispenseState = ElementState::BRT;
    readyState = ElementState::BRT;
    strncpy(flareCount, "88", 3);
    strncpy(chaffCount, "88", 3);
    needsFullRedraw = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // All dim
    for (int i = 0; i < 4; ++i) arrowState[i] = ElementState::DIM;
    dispenseState = ElementState::DIM;
    readyState = ElementState::DIM;
    needsFullRedraw = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // All off
    for (int i = 0; i < 4; ++i) arrowState[i] = ElementState::OFF;
    dispenseState = ElementState::OFF;
    readyState = ElementState::OFF;
    needsFullRedraw = true;
    CMWSDisplay_draw(true);
    vTaskDelay(pdMS_TO_TICKS(500));

    // Rotate arrows
    for (int a = 0; a < 4; ++a) {
        for (int i = 0; i < 4; ++i) arrowState[i] = ElementState::DIM;
        arrowState[a] = ElementState::BRT;
        needsFullRedraw = true;
        CMWSDisplay_draw(true);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    // Restore
    displayPage = savedPage;
    tft.setBrightness(savedBacklight);
    strncpy(flareCount, "00", 3);
    strncpy(chaffCount, "00", 3);
    for (int i = 0; i < 4; ++i) arrowState[i] = ElementState::DIM;
    dispenseState = ElementState::DIM;
    readyState = ElementState::DIM;
    needsFullRedraw = true;
    CMWSDisplay_draw(true);
}

#else
void CMWSDisplay_init() {}
void CMWSDisplay_loop() {}
void CMWSDisplay_deinit() {}
void CMWSDisplay_notifyMissionStart() {}
void CMWSDisplay_bitTest() {}
#endif
