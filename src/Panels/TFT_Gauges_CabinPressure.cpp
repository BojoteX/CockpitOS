// CockpitOS — Cabin Pressure Gauge (LovyanGFX, ST77916/61)

#define CABIN_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS 30   // ~33 Hz
#define RUN_CABIN_PRESSURE_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "INST_PANEL_DIMMER" // Other option is CONSOLES_DIMMER depending on the gauge type

#include "../Globals.h"
#include "../TFT_Gauges_CabPress.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>

// Pin Definitions (S3 device)
// #define CABIN_PRESSURE_CS_PIN   10  // [BLUE] MOSI pin for the cabin pressure gauge
// #define CABIN_PRESSURE_MOSI_PIN 11  // [YELLOW] MOSI pin for the cabin pressure gauge
// #define CABIN_PRESSURE_SCLK_PIN 12  // [ORANGE] SCLK pin for the cabin pressure gauge
// #define CABIN_PRESSURE_DC_PIN   13  // [GREEN] DC pin for the cabin pressure gauge

// Pin Definitions (S2 device)
// #define CABIN_PRESSURE_CS_PIN   36  // [BLUE] MOSI pin for the cabin pressure gauge
#define CABIN_PRESSURE_MOSI_PIN  8  // [YELLOW] MOSI pin for the cabin pressure gauge
#define CABIN_PRESSURE_SCLK_PIN  9  // [ORANGE] SCLK pin for the cabin pressure gauge
#define CABIN_PRESSURE_DC_PIN   13  // [GREEN] DC pin for the cabin pressure gauge

// Not used in this implementation
#define CABIN_PRESSURE_RST_PIN  -1  // RESET pin for the cabin pressure gauge
#define CABIN_PRESSURE_MISO_PIN -1  // MISO pin for the cabin pressure gauge

// Assets (360x360 bg, 23x238 needle)
#include "Assets/CabinPressure/cabinPressBackground.h"
#include "Assets/CabinPressure/cabinPressBackgroundNVG.h"
#include "Assets/CabinPressure/cabinPressNeedle.h"
#include "Assets/CabinPressure/cabinPressNeedleNVG.h"

// Bus flags
static constexpr bool shared_bus = true;
static constexpr bool use_lock = true;

// ---------------------- Panel binding ----------------------
class LGFX_CabPress final : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_ST77961 _panel;  // class name as provided by LGFX build
public:
    LGFX_CabPress() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 0;
            cfg.spi_3wire = false;
            cfg.use_lock = use_lock;
            cfg.dma_channel = 1;
            cfg.pin_mosi = CABIN_PRESSURE_MOSI_PIN;
            cfg.pin_miso = CABIN_PRESSURE_MISO_PIN;
            cfg.pin_sclk = CABIN_PRESSURE_SCLK_PIN;
            cfg.pin_dc = CABIN_PRESSURE_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.pin_cs = CABIN_PRESSURE_CS_PIN;
            pcfg.pin_rst = CABIN_PRESSURE_RST_PIN;
            pcfg.pin_busy = -1;
            pcfg.memory_width = 360;
            pcfg.memory_height = 360;
            pcfg.panel_width = 360;
            pcfg.panel_height = 360;
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
static lgfx::LGFX_Sprite needleU(&tft);

static constexpr uint8_t  COLOR_DEPTH = 16;
static constexpr int16_t  CENTER_X = 180;
static constexpr int16_t  CENTER_Y = 180;
static constexpr uint16_t TRANSPARENT_KEY = 0x0120;

static volatile int16_t   angleU = -181;
static volatile int16_t   lastDrawnAngleU = INT16_MIN;
static volatile bool      gaugeDirty = false;
static volatile uint8_t   currentLightingMode = 0;   // 0=Day, 2=NVG
static uint8_t            lastNeedleMode = 0xFF;
static unsigned long      lastDrawTime = 0;
static TaskHandle_t       tftTaskHandle = nullptr;
static constexpr uint16_t NVG_THRESHOLD = 6553;

// --- Build needle sprite ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(false);
    spr.pushImage(0, 0, 23, 238, img);
}

// --- DCS-BIOS callback ---
static void onPressureAltChange(const char*, uint16_t value, uint16_t) {
    int16_t a = map(value, 0, 65535, -181, 125);
    if (a != angleU) { angleU = a; gaugeDirty = true; }
}

// --- Draw ---
static void CabinPressureGauge_draw(bool force = false)
{
    if (!force && !isMissionRunning()) return;

    const unsigned long now = millis();
    int16_t u = angleU; if (u < -181) u = -181; else if (u > 125) u = 125;

    const bool shouldDraw = force || gaugeDirty || (u != lastDrawnAngleU);
    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < CABIN_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleU = u;
    gaugeDirty = false;

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_CABIN_PRESSURE_DRAW);
#endif

    const uint16_t* bg = (currentLightingMode == 0) ? cabinPressBackground : cabinPressBackgroundNVG;
    const uint16_t* needleImg = (currentLightingMode == 0) ? cabinPressNeedle : cabinPressNeedleNVG;

    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleU, needleImg);
        lastNeedleMode = currentLightingMode;
    }

    tft.startWrite();
    tft.pushImageDMA(0, 0, 360, 360, bg);
    needleU.pushRotateZoom(&tft, CENTER_X, CENTER_Y, (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);
    tft.endWrite();

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_CABIN_PRESSURE_DRAW);
#endif
}

// --- Task ---
static void CabinPressureGauge_task(void*) {
    for (;;) { CabinPressureGauge_draw(false); vTaskDelay(pdMS_TO_TICKS(5)); }
}

// --- Lighting control --- (Disabled for now; using dimmer instead)
static void onCabinPressureGaugeLightingChange(const char*, uint16_t value) {
    const uint8_t norm = (value == 0) ? 0 : 2;
    if (norm == currentLightingMode) return;
    currentLightingMode = norm;
#if DEBUG_ENABLED
    debugPrintf("⚙️ LGFX lighting=%s\n", currentLightingMode == 0 ? "DAY" : "NVG");
#endif
    gaugeDirty = true;   // no immediate draw here
}

static void onInstPanelDimmerChange(const char*, uint16_t v, uint16_t) {
    const uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;   // 0=DAY, 2=NVG
    if (mode == currentLightingMode) return;
    currentLightingMode = mode;
    gaugeDirty = true;     // no immediate draw; task/loop will pick it up
}

// --- API ---
void CabinPressureGauge_setLightingMode(uint8_t mode) { onCabinPressureGaugeLightingChange(nullptr, mode); }

void CabinPressureGauge_init()
{
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_CABIN_PRESSURE_INIT);
#endif
    tft.init();
    tft.setColorDepth(16);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    needleU.setColorDepth(COLOR_DEPTH);
    needleU.createSprite(23, 238);
    needleU.setPivot(12, 165);

    subscribeToLedChange("PRESSURE_ALT", onPressureAltChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onInstPanelDimmerChange);
    // subscribeToSelectorChange("COCKKPIT_LIGHT_MODE_SW", onCabinPressureGaugeLightingChange);

    // BIT before task start
    CabinPressureGauge_bitTest();

#if RUN_CABIN_PRESSURE_GAUGE_AS_TASK
#if defined(IS_S3_PINS)
    xTaskCreatePinnedToCore(CabinPressureGauge_task, "CabinPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 1);
#else
    xTaskCreatePinnedToCore(CabinPressureGauge_task, "CabinPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 0);
#endif
#endif

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_CABIN_PRESSURE_INIT);
#endif
    debugPrintln("✅ Cabin Pressure Gauge (LovyanGFX) initialized");
}

void CabinPressureGauge_loop() {
#if !RUN_CABIN_PRESSURE_GAUGE_AS_TASK
    CabinPressureGauge_draw(false);
#endif
}

void CabinPressureGauge_notifyMissionStart() { gaugeDirty = true; }

void CabinPressureGauge_bitTest()
{
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_CABIN_PRESSURE_BITTEST);
#endif
    int16_t originalU = angleU;
    const int STEP = 10, DELAY = 2;

    for (int i = 0; i <= 306; i += STEP) {
        angleU = map(i, 0, 306, -181, 125);
        gaugeDirty = true; CabinPressureGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 306; i >= 0; i -= STEP) {
        angleU = map(i, 0, 306, -181, 125);
        gaugeDirty = true; CabinPressureGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    angleU = originalU;
    gaugeDirty = true; CabinPressureGauge_draw(true);

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_CABIN_PRESSURE_BITTEST);
#endif
}

void CabinPressureGauge_deinit()
{
    needleU.deleteSprite();
    if (tftTaskHandle) { vTaskDelete(tftTaskHandle); tftTaskHandle = nullptr; }
}
