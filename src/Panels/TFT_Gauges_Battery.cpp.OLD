// CockpitOS — Battery Gauge (LovyanGFX)

#define GAUGE_DRAW_MIN_INTERVAL_MS 13   // ~77 Hz
#define RUN_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "CONSOLES_DIMMER" // Other option is INST_PANEL_DIMMER depending on the gauge type

#include "../Globals.h"
#include "../TFT_Gauges_Batt.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>

// Pins (ESP32-S2)
// #define BATTERY_CS_PIN    36  // CS pin for the battery gauge
#define BATTERY_MOSI_PIN   8  // MOSI
#define BATTERY_SCLK_PIN   9  // SCLK
#define BATTERY_DC_PIN    13  // D/C
#define BATTERY_RST_PIN   12  // RST
#define BATTERY_MISO_PIN  -1  // not used

// Assets (240x240 bg, 15x88 needle)
#include "Assets/BatteryGauge/batBackground.h"
#include "Assets/BatteryGauge/batBackgroundNVG.h"
#include "Assets/BatteryGauge/batNeedle.h"
#include "Assets/BatteryGauge/batNeedleNVG.h"

// Misc
static constexpr bool shared_bus = true;
static constexpr bool use_lock = true;

// ---------------------- Panel binding (LovyanGFX) ----------------------
class LGFX_Battery : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_GC9A01 _panel;
public:
    LGFX_Battery() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 0;
            cfg.spi_3wire = false;
            cfg.use_lock = use_lock;
            cfg.dma_channel = 1;
            cfg.pin_mosi = BATTERY_MOSI_PIN;
            cfg.pin_miso = BATTERY_MISO_PIN;
            cfg.pin_sclk = BATTERY_SCLK_PIN;
            cfg.pin_dc = BATTERY_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.pin_cs = BATTERY_CS_PIN;
            pcfg.pin_rst = BATTERY_RST_PIN;
            pcfg.pin_busy = -1;
            pcfg.memory_width = 240;
            pcfg.memory_height = 240;
            pcfg.panel_width = 240;
            pcfg.panel_height = 240;
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
static lgfx::LGFX_Sprite needleU(&tft), needleE(&tft);

static constexpr uint8_t  COLOR_DEPTH = 16;
static constexpr int16_t  CENTER_X = 120;
static constexpr int16_t  CENTER_Y = 120;
static constexpr uint16_t TRANSPARENT_KEY = 0x0120;

static volatile int16_t angleU = -150, angleE = 150;
static volatile int16_t lastDrawnAngleU = INT16_MIN, lastDrawnAngleE = INT16_MIN;
static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0;   // 0=Day, 2=NVG
static uint8_t          lastNeedleMode = 0xFF;
static unsigned long    lastDrawTime = 0;
static TaskHandle_t     tftTaskHandle = nullptr;
static constexpr uint16_t NVG_THRESHOLD = 6553;

// --- Needle sprite helper ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(false);
    spr.pushImage(0, 0, 15, 88, img);
}

// --- DCS-BIOS callbacks ---
static void onBatVoltUChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, -150, -30);
    if (a != angleU) { angleU = a; gaugeDirty = true; }
}
static void onBatVoltEChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, 150, 30);
    if (a != angleE) { angleE = a; gaugeDirty = true; }
}

// --- Draw ---
static void BatteryGauge_draw(bool force = false) {
    if (!force && !isMissionRunning()) return;

    const unsigned long now = millis();
    int16_t u = angleU; if (u < -150) u = -150; else if (u > -30) u = -30;
    int16_t e = angleE; if (e < 30)  e = 30;  else if (e > 150) e = 150;

    const bool shouldDraw = force || gaugeDirty || (u != lastDrawnAngleU) || (e != lastDrawnAngleE);
    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleU = u; lastDrawnAngleE = e;
    gaugeDirty = false;

    // Assets
    const uint16_t* bg = (currentLightingMode == 0) ? batBackground : batBackgroundNVG;
    const uint16_t* needleImg = (currentLightingMode == 0) ? batNeedle : batNeedleNVG;

    // Rebuild needles only on mode change
    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleU, needleImg);
        buildNeedle(needleE, needleImg);
        lastNeedleMode = currentLightingMode;
    }

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BATTERY_DRAW);
#endif

    tft.startWrite();
    tft.pushImageDMA(0, 0, 240, 240, bg);
    needleU.pushRotateZoom(&tft, CENTER_X, CENTER_Y, (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);
    needleE.pushRotateZoom(&tft, CENTER_X, CENTER_Y, (float)e, 1.0f, 1.0f, TRANSPARENT_KEY);
    tft.endWrite();

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_BATTERY_DRAW);
#endif
}

// --- Task ---
static void BatteryGauge_task(void*) {
    for (;;) { BatteryGauge_draw(false); vTaskDelay(pdMS_TO_TICKS(5)); }
}

// --- Lighting ---
static void onBatteryGaugeLightingChange(const char*, uint16_t value) {
    const uint8_t normalized = (value == 0) ? 0 : 2;
    if (normalized == currentLightingMode) return;
    currentLightingMode = normalized;
#if DEBUG_ENABLED
    debugPrintf("⚙️ LGFX lighting=%s\n", currentLightingMode == 0 ? "DAY" : "NVG");
#endif
    gaugeDirty = true;
}

static void onInstPanelDimmerChange(const char*, uint16_t v, uint16_t) {
    const uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode == currentLightingMode) return;
    currentLightingMode = mode;
    gaugeDirty = true;
}

// --- API ---
void BatteryGauge_setLightingMode(uint8_t mode) { onBatteryGaugeLightingChange(nullptr, mode); }

void BatteryGauge_init() {
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BATTERY_INIT);
#endif
    tft.init();
    tft.setColorDepth(16);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    needleU.setColorDepth(COLOR_DEPTH);
    needleU.createSprite(15, 88);
    needleU.setPivot(7, 84);

    needleE.setColorDepth(COLOR_DEPTH);
    needleE.createSprite(15, 88);
    needleE.setPivot(7, 84);

    subscribeToLedChange("VOLT_U", onBatVoltUChange);
    subscribeToLedChange("VOLT_E", onBatVoltEChange);

    subscribeToLedChange(BACKLIGHT_LABEL, onInstPanelDimmerChange);
    // subscribeToSelectorChange("COCKKPIT_LIGHT_MODE_SW", onBatteryGaugeLightingChange);

    // BIT before starting task
    BatteryGauge_bitTest();

#if RUN_GAUGE_AS_TASK
#if defined(IS_S3_PINS)
    xTaskCreatePinnedToCore(BatteryGauge_task, "BatteryGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 1);
#else
    xTaskCreatePinnedToCore(BatteryGauge_task, "BatteryGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 0);
#endif
#endif

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_BATTERY_INIT);
#endif
    debugPrintln("✅ Battery Gauge (LovyanGFX) initialized");
}

void BatteryGauge_loop() {
#if !RUN_GAUGE_AS_TASK
    BatteryGauge_draw(false);
#endif
}

void BatteryGauge_notifyMissionStart() { gaugeDirty = true; }

void BatteryGauge_bitTest() {
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BATTERY_BITTEST);
#endif
    const int STEP = 5, DELAY = 10;
    for (int i = 0; i <= 120; i += STEP) {
        angleU = map(i, 0, 120, -150, -30);
        angleE = map(i, 0, 120, 150, 30);
        gaugeDirty = true; BatteryGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 120; i >= 0; i -= STEP) {
        angleU = map(i, 0, 120, -150, -30);
        angleE = map(i, 0, 120, 150, 30);
        gaugeDirty = true; BatteryGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    angleU = -150; angleE = 150;
    gaugeDirty = true; BatteryGauge_draw(true);
#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_BATTERY_BITTEST);
#endif
}

void BatteryGauge_deinit() {
    needleU.deleteSprite();
    needleE.deleteSprite();
    if (tftTaskHandle) { vTaskDelete(tftTaskHandle); tftTaskHandle = nullptr; }
}