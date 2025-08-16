// CockpitOS — Hydraulic Pressure Gauge (LovyanGFX, GC9A01) - Double-buffered PSRAM, DMA-safe

#define HYD_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS 13
#define RUN_HYD_PRESSURE_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "CONSOLES_DIMMER"
#define COLOR_DEPTH_HYD_PRESS 16

#include "../Globals.h"
#include "../TFT_Gauges_HydPress.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>
#include <cstring>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// --- Pins ---
#define HYD_PRESSURE_MOSI_PIN   8
#define HYD_PRESSURE_SCLK_PIN   9
#define HYD_PRESSURE_DC_PIN    13
#define HYD_PRESSURE_RST_PIN   12
#define HYD_PRESSURE_MISO_PIN  -1

// --- Assets (240x240 bg, 33x120 needles) ---
#include "Assets/HydPressure/hydPressBackground.h"
#include "Assets/HydPressure/hydPressBackgroundNVG.h"
#include "Assets/HydPressure/hydPressNeedle1.h"
#include "Assets/HydPressure/hydPressNeedle1NVG.h"
#include "Assets/HydPressure/hydPressNeedle2.h"
#include "Assets/HydPressure/hydPressNeedle2NVG.h"

// --- Misc ---
static constexpr bool shared_bus = true;
static constexpr bool use_lock = true;
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;
static constexpr uint16_t NVG_THRESHOLD = 6553;
static constexpr int16_t SCREEN_W = 240, SCREEN_H = 240;
static constexpr int16_t CENTER_X = 120, CENTER_Y = 120;
static constexpr int16_t NEEDLE_PIVOT_X = 17, NEEDLE_PIVOT_Y = 103;

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
            cfg.dma_channel = 1;
            cfg.pin_mosi = HYD_PRESSURE_MOSI_PIN;
            cfg.pin_miso = HYD_PRESSURE_MISO_PIN;
            cfg.pin_sclk = HYD_PRESSURE_SCLK_PIN;
            cfg.pin_dc = HYD_PRESSURE_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
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

// Compose target (once; PSRAM)
static lgfx::LGFX_Sprite frameSpr(&tft);
static lgfx::LGFX_Sprite needleL(&tft), needleR(&tft);

// Live values
static volatile int16_t angleL = -280;   // left needle
static volatile int16_t angleR = -280;   // right needle
static volatile int16_t lastDrawnAngleL = INT16_MIN;
static volatile int16_t lastDrawnAngleR = INT16_MIN;

static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0;   // 0=Day, 2=NVG

static uint8_t          lastNeedleMode = 0xFF;
static unsigned long    lastDrawTime = 0;
static TaskHandle_t     tftTaskHandle = nullptr;

// --- DMA-safe double buffer ---
static constexpr size_t FRAME_PIXELS = size_t(SCREEN_W) * size_t(SCREEN_H);
static constexpr size_t FRAME_BYTES = FRAME_PIXELS * sizeof(uint16_t);

static uint16_t* dmaFrame[2] = { nullptr, nullptr };
static uint8_t   dmaIdx = 0;
static bool      dmaBusy = false;

static inline void waitDMADone() {
    if (dmaBusy) {
        tft.waitDMA();
        dmaBusy = false;
    }
}

// --- Needle sprite builder ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
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
static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode != currentLightingMode) { currentLightingMode = mode; gaugeDirty = true; }
}

// --- Flush helper ---
static inline void flushFrameToDisplay(uint16_t* buf, bool blocking) {
    tft.startWrite();
    if (blocking) {
        tft.pushImage(0, 0, SCREEN_W, SCREEN_H, buf);
        dmaBusy = false;
    }
    else {
        waitDMADone();
        tft.pushImageDMA(0, 0, SCREEN_W, SCREEN_H, buf);
        dmaBusy = true;
    }
    tft.endWrite();
}

// --- Double-buffered draw (no tearing) ---
static void HydPressureGauge_draw(bool force = false, bool blocking = false) {
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();
    int16_t l = angleL; if (l < -280) l = -280; else if (l > 40) l = 40;
    int16_t r = angleR; if (r < -280) r = -280; else if (r > 40) r = 40;

    const bool shouldDraw = force || gaugeDirty || (l != lastDrawnAngleL) || (r != lastDrawnAngleR);
    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < HYD_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleL = l;
    lastDrawnAngleR = r;
    gaugeDirty = false;

    // Assets
    const uint16_t* bg = (currentLightingMode == 0) ? hydPressBackground : hydPressBackgroundNVG;
    const uint16_t* n1 = (currentLightingMode == 0) ? hydPressNeedle1 : hydPressNeedle1NVG;
    const uint16_t* n2 = (currentLightingMode == 0) ? hydPressNeedle2 : hydPressNeedle2NVG;

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_HYDPRESS_DRAW);
#endif

    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleL, n1);
        buildNeedle(needleR, n2);
        lastNeedleMode = currentLightingMode;
    }

    // Compose into frameSpr (PSRAM)
    frameSpr.fillScreen(TFT_BLACK);
    frameSpr.pushImage(0, 0, SCREEN_W, SCREEN_H, bg);
    needleL.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)l, 1.0f, 1.0f, TRANSPARENT_KEY);
    needleR.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)r, 1.0f, 1.0f, TRANSPARENT_KEY);

    uint16_t* buf = dmaFrame[dmaIdx ^ 1];
    std::memcpy(buf, frameSpr.getBuffer(), FRAME_BYTES);
    flushFrameToDisplay(buf, blocking);
    dmaIdx ^= 1;

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_HYDPRESS_DRAW);
#endif
}

// --- Task ---
static void HydPressureGauge_task(void*) {
    for (;;) {
        HydPressureGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void HydPressureGauge_init() {
#if defined(ARDUINO_ARCH_ESP32)
    dmaFrame[0] = (uint16_t*)heap_caps_malloc(FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    dmaFrame[1] = (uint16_t*)heap_caps_malloc(FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
#else
    dmaFrame[0] = (uint16_t*)malloc(FRAME_BYTES);
    dmaFrame[1] = (uint16_t*)malloc(FRAME_BYTES);
#endif
    if (!dmaFrame[0] || !dmaFrame[1]) {
        debugPrintln("❌ PSRAM DMA framebuffer alloc failed!");
        while (1) vTaskDelay(1000);
    }

    tft.init();
    tft.setColorDepth(COLOR_DEPTH_HYD_PRESS);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    frameSpr.setColorDepth(COLOR_DEPTH_HYD_PRESS);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    needleL.setColorDepth(COLOR_DEPTH_HYD_PRESS);
    needleL.createSprite(33, 120);
    needleL.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleL, hydPressNeedle1);

    needleR.setColorDepth(COLOR_DEPTH_HYD_PRESS);
    needleR.createSprite(33, 120);
    needleR.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleR, hydPressNeedle2);

    subscribeToLedChange("HYD_IND_LEFT", onHydLeftChange);
    subscribeToLedChange("HYD_IND_RIGHT", onHydRightChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    HydPressureGauge_bitTest();

#if RUN_HYD_PRESSURE_GAUGE_AS_TASK
#if defined(IS_S3_PINS)
    xTaskCreatePinnedToCore(HydPressureGauge_task, "HydPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 1);
#else
    xTaskCreatePinnedToCore(HydPressureGauge_task, "HydPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 0);
#endif
#endif

    debugPrintln("✅ Hydraulic Pressure Gauge (LovyanGFX, PSRAM double-buffered, DMA-safe) initialized");
}

void HydPressureGauge_loop() {
#if !RUN_HYD_PRESSURE_GAUGE_AS_TASK
    HydPressureGauge_draw(false, false);
#endif
}

void HydPressureGauge_notifyMissionStart() { gaugeDirty = true; }

// Visual self-test; use blocking flush to avoid DMA overlap during rapid sweeps
void HydPressureGauge_bitTest() {
    int16_t originalL = angleL, originalR = angleR;
    const int STEP = 20, DELAY = 2;
    // Forward: L -280→40, R 40→-280
    for (int i = 0; i <= 320; i += STEP) {
        angleL = map(i, 0, 320, -280, 40);
        angleR = map(i, 0, 320, 40, -280);
        gaugeDirty = true; HydPressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    // Reverse: L 40→-280, R -280→40
    for (int i = 320; i >= 0; i -= STEP) {
        angleL = map(i, 0, 320, -280, 40);
        angleR = map(i, 0, 320, 40, -280);
        gaugeDirty = true; HydPressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    angleL = originalL; angleR = originalR;
    gaugeDirty = true; HydPressureGauge_draw(true, true);
}

void HydPressureGauge_deinit() {
    waitDMADone();
    needleL.deleteSprite();
    needleR.deleteSprite();
    frameSpr.deleteSprite();
    if (tftTaskHandle) { vTaskDelete(tftTaskHandle); tftTaskHandle = nullptr; }
#if defined(ARDUINO_ARCH_ESP32)
    if (dmaFrame[0]) { heap_caps_free(dmaFrame[0]); dmaFrame[0] = nullptr; }
    if (dmaFrame[1]) { heap_caps_free(dmaFrame[1]); dmaFrame[1] = nullptr; }
#else
    if (dmaFrame[0]) { free(dmaFrame[0]); dmaFrame[0] = nullptr; }
    if (dmaFrame[1]) { free(dmaFrame[1]); dmaFrame[1] = nullptr; }
#endif
}