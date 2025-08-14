// CockpitOS — Hydraulic Pressure Gauge (LovyanGFX, GC9A01)

#define HYD_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS 13   // ~77 Hz
#define RUN_HYD_PRESSURE_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "CONSOLES_DIMMER"          // Or "INST_PANEL_DIMMER" per gauge

#include "../Globals.h"
#include "../TFT_Gauges_HydPress.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>

// Pins (ESP32-S2)
// #define HYD_PRESSURE_CS_PIN  36  // CS (provided by board map if commented)
#define HYD_PRESSURE_MOSI_PIN  8   // MOSI
#define HYD_PRESSURE_SCLK_PIN  9   // SCLK
#define HYD_PRESSURE_DC_PIN   13   // D/C
#define HYD_PRESSURE_RST_PIN  12   // RST
#define HYD_PRESSURE_MISO_PIN -1   // not used

// Assets (240x240 bg, 33x120 needles)
#include "Assets/HydPressure/hydPressBackground.h"
#include "Assets/HydPressure/hydPressBackgroundNVG.h"
#include "Assets/HydPressure/hydPressNeedle1.h"
#include "Assets/HydPressure/hydPressNeedle1NVG.h"
#include "Assets/HydPressure/hydPressNeedle2.h"
#include "Assets/HydPressure/hydPressNeedle2NVG.h"

// Misc
static constexpr bool shared_bus = true;
static constexpr bool use_lock   = true;

// ---------------------- Panel binding (LovyanGFX) ----------------------
class LGFX_HydPress : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_GC9A01 _panel;
public:
    LGFX_HydPress() {
        {
            auto cfg = _bus.config();
            cfg.spi_host   = SPI2_HOST;
            cfg.spi_mode   = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read  = 0;
            cfg.spi_3wire  = false;
            cfg.use_lock   = use_lock;
            cfg.dma_channel= 1;
            cfg.pin_mosi   = HYD_PRESSURE_MOSI_PIN;
            cfg.pin_miso   = HYD_PRESSURE_MISO_PIN;
            cfg.pin_sclk   = HYD_PRESSURE_SCLK_PIN;
            cfg.pin_dc     = HYD_PRESSURE_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.pin_cs          = HYD_PRESSURE_CS_PIN;
            pcfg.pin_rst         = HYD_PRESSURE_RST_PIN;
            pcfg.pin_busy        = -1;
            pcfg.memory_width    = 240;
            pcfg.memory_height   = 240;
            pcfg.panel_width     = 240;
            pcfg.panel_height    = 240;
            pcfg.offset_x        = 0;
            pcfg.offset_y        = 0;
            pcfg.offset_rotation = 0;
            pcfg.bus_shared      = shared_bus;
            pcfg.invert          = true;
            _panel.config(pcfg);
        }
        setPanel(&_panel);
    }
};

// --- State ---
static LGFX_HydPress tft;
static lgfx::LGFX_Sprite needleL(&tft), needleR(&tft);

static constexpr uint8_t  COLOR_DEPTH     = 8;
static constexpr int16_t  CENTER_X        = 120;
static constexpr int16_t  CENTER_Y        = 120;
static constexpr uint16_t TRANSPARENT_KEY = 0x0120;

static volatile int16_t angleL = -280;   // left needle
static volatile int16_t angleR = -280;   // right needle
static volatile int16_t lastDrawnAngleL = INT16_MIN;
static volatile int16_t lastDrawnAngleR = INT16_MIN;

static volatile bool    gaugeDirty          = false;
static volatile uint8_t currentLightingMode = 0;    // 0=Day, 2=NVG
static uint8_t          lastNeedleMode      = 0xFF;
static unsigned long    lastDrawTime        = 0;
static TaskHandle_t     tftTaskHandle       = nullptr;
static constexpr uint16_t NVG_THRESHOLD     = 6553;

// --- Needle sprite helper ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(false);
    spr.pushImage(0, 0, 33, 120, img);
}

// --- DCS-BIOS callbacks ---
static void onHydLeftChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, -280, 40);
    if (a != angleL) { angleL = a; gaugeDirty = true; }
}
static void onHydRightChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, -280, 40);
    if (a != angleR) { angleR = a; gaugeDirty = true; }
}

// --- Draw ---
static void HydPressureGauge_draw(bool force = false) {
    if (!force && !isMissionRunning()) return;

    const unsigned long now = millis();
    int16_t l = angleL; if (l < -280) l = -280; else if (l > 40) l = 40;
    int16_t r = angleR; if (r < -280) r = -280; else if (r > 40) r = 40;

    const bool shouldDraw = force || gaugeDirty || (l != lastDrawnAngleL) || (r != lastDrawnAngleR);
    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < HYD_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleL = l; lastDrawnAngleR = r;
    gaugeDirty = false;

    // Assets
    const uint16_t* bg = (currentLightingMode == 0) ? hydPressBackground : hydPressBackgroundNVG;
    const uint16_t* n1 = (currentLightingMode == 0) ? hydPressNeedle1   : hydPressNeedle1NVG;
    const uint16_t* n2 = (currentLightingMode == 0) ? hydPressNeedle2   : hydPressNeedle2NVG;

    // Rebuild needles only on mode change
    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleL, n1);
        buildNeedle(needleR, n2);
        lastNeedleMode = currentLightingMode;
    }

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_HYDPRESS_DRAW);
#endif

    tft.startWrite();
    tft.pushImage(0, 0, 240, 240, bg);

    needleL.pushRotateZoom(&tft, CENTER_X, CENTER_Y, (float)l, 1.0f, 1.0f, TRANSPARENT_KEY);
    needleR.pushRotateZoom(&tft, CENTER_X, CENTER_Y, (float)r, 1.0f, 1.0f, TRANSPARENT_KEY);

    tft.endWrite();

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_HYDPRESS_DRAW);
#endif
}

// --- Task ---
static void HydPressureGauge_task(void*) {
    for (;;) { HydPressureGauge_draw(false); vTaskDelay(pdMS_TO_TICKS(5)); }
}

// --- Lighting ---
static void onHydPressureGaugeLightingChange(const char*, uint16_t value) {
    const uint8_t normalized = (value == 0) ? 0 : 2;
    if (normalized == currentLightingMode) return;
    currentLightingMode = normalized;
    gaugeDirty = true;
}
static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    const uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode == currentLightingMode) return;
    currentLightingMode = mode;
    gaugeDirty = true;
}

// --- API ---
void HydPressureGauge_setLightingMode(uint8_t mode) { onHydPressureGaugeLightingChange(nullptr, mode); }

void HydPressureGauge_init() {
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_HYDPRESS_INIT);
#endif
    tft.init();
    tft.setColorDepth(16);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    needleL.setColorDepth(COLOR_DEPTH);
    needleL.createSprite(33, 120);
    needleL.setPivot(17, 103);

    needleR.setColorDepth(COLOR_DEPTH);
    needleR.createSprite(33, 120);
    needleR.setPivot(17, 103);

    subscribeToLedChange("HYD_IND_LEFT",  onHydLeftChange);
    subscribeToLedChange("HYD_IND_RIGHT", onHydRightChange);

    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);
    // subscribeToSelectorChange("COCKKPIT_LIGHT_MODE_SW", onHydPressureGaugeLightingChange);

    // BIT before starting task
    HydPressureGauge_bitTest();

#if RUN_HYD_PRESSURE_GAUGE_AS_TASK
#if defined(IS_S3_PINS)
    xTaskCreatePinnedToCore(HydPressureGauge_task, "HydPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 1);
#else
    xTaskCreatePinnedToCore(HydPressureGauge_task, "HydPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 0);
#endif
#endif

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_HYDPRESS_INIT);
#endif
    debugPrintln("✅ Hydraulic Pressure Gauge (LovyanGFX) initialized");
}

void HydPressureGauge_loop() {
#if !RUN_HYD_PRESSURE_GAUGE_AS_TASK
    HydPressureGauge_draw(false);
#endif
}

void HydPressureGauge_notifyMissionStart() { gaugeDirty = true; }

void HydPressureGauge_bitTest() {
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_HYDPRESS_BITTEST);
#endif
    const int STEP = 4, DELAY = 2;
    int16_t originalL = angleL, originalR = angleR;

    // Forward: L from -280→40, R from 40→-280
    for (int i = 0; i <= 320; i += STEP) {
        angleL = map(i, 0, 320, -280, 40);
        angleR = map(i, 0, 320, 40, -280);
        gaugeDirty = true; HydPressureGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    // Reverse: L from 40→-280, R from -280→40
    for (int i = 320; i >= 0; i -= STEP) {
        angleL = map(i, 0, 320, -280, 40);
        angleR = map(i, 0, 320, 40, -280);
        gaugeDirty = true; HydPressureGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    // Restore
    angleL = originalL; angleR = originalR;
    gaugeDirty = true; HydPressureGauge_draw(true);

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_HYDPRESS_BITTEST);
#endif
}

void HydPressureGauge_deinit() {
    needleL.deleteSprite();
    needleR.deleteSprite();
    if (tftTaskHandle) { vTaskDelete(tftTaskHandle); tftTaskHandle = nullptr; }
}
