// TFT_Gauges_CabPress.cpp - CockpitOS Cabin Pressure Gauge (LovyanGFX, ST77916/61 @ 360×360) - Double-buffered PSRAM, DMA-safe

#define CABIN_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS 13
#define RUN_CABIN_PRESSURE_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "INST_PNL_DIMMER"
#define COLOR_DEPTH_CABIN_PRESS 16

#include "../Globals.h"
#include "../TFT_Gauges_CabPress.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>
#include <cstring>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// --- Pins ---
#define CABIN_PRESSURE_MOSI_PIN   8
#define CABIN_PRESSURE_SCLK_PIN   9
#define CABIN_PRESSURE_DC_PIN    13
#define CABIN_PRESSURE_RST_PIN   -1
#define CABIN_PRESSURE_MISO_PIN  -1

// --- Assets (360x360 bg, 23x238 needle) ---
#include "Assets/CabinPressure/cabinPressBackground.h"
#include "Assets/CabinPressure/cabinPressBackgroundNVG.h"
#include "Assets/CabinPressure/cabinPressNeedle.h"
#include "Assets/CabinPressure/cabinPressNeedleNVG.h"

// --- Misc ---
static constexpr bool shared_bus = true;
static constexpr bool use_lock = true;
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;
static constexpr uint16_t NVG_THRESHOLD = 6553;
static constexpr int16_t SCREEN_W = 360, SCREEN_H = 360;
static constexpr int16_t CENTER_X = 180, CENTER_Y = 180;
static constexpr int16_t NEEDLE_PIVOT_X = 12, NEEDLE_PIVOT_Y = 165;

// --- Panel binding ---
class LGFX_CabPress final : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_ST77961 _panel;
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

// Compose target (once; PSRAM)
static lgfx::LGFX_Sprite frameSpr(&tft);
static lgfx::LGFX_Sprite needleSpr(&tft);

// Live values
static volatile int16_t angleU = -181;
static volatile int16_t lastDrawnAngleU = INT16_MIN;
static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0; // 0=Day, 2=NVG

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

// --- Sprite builder ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, 23, 238, img);
}

// --- DCS-BIOS callbacks ---
static void onPressureAltChange(const char*, uint16_t value, uint16_t) {
    int16_t a = map(value, 0, 65535, -181, 125);
    if (a != angleU) { angleU = a; gaugeDirty = true; }
}
static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode != currentLightingMode) { currentLightingMode = mode; gaugeDirty = true; }
}

// --- Flush helper (blocking or DMA) ---
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

// --- Double-buffered draw ---
static void CabinPressureGauge_draw(bool force = false, bool blocking = false) {
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();
    int16_t u = angleU;
    if (u < -181) u = -181; else if (u > 125) u = 125;

    const bool shouldDraw = force || gaugeDirty || (u != lastDrawnAngleU);
    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < CABIN_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleU = u;
    gaugeDirty = false;

    const uint16_t* bg = (currentLightingMode == 0) ? cabinPressBackground : cabinPressBackgroundNVG;
    const uint16_t* needleImg = (currentLightingMode == 0) ? cabinPressNeedle : cabinPressNeedleNVG;

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_CABIN_PRESSURE_DRAW);
#endif

    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleSpr, needleImg);
        lastNeedleMode = currentLightingMode;
    }

    // Compose full frame into frameSpr
    frameSpr.fillScreen(TFT_BLACK);
    frameSpr.pushImage(0, 0, SCREEN_W, SCREEN_H, bg);
    needleSpr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);

    // Copy to back buffer, then flush
    uint16_t* buf = dmaFrame[dmaIdx ^ 1];
    std::memcpy(buf, frameSpr.getBuffer(), FRAME_BYTES);
    flushFrameToDisplay(buf, blocking);
    dmaIdx ^= 1;

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_CABIN_PRESSURE_DRAW);
#endif
}

// --- Task ---
static void CabinPressureGauge_task(void*) {
    for (;;) {
        CabinPressureGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void CabinPressureGauge_init() {
    if (!initPSRAM()) {
        debugPrintln("❌ No PSRAM detected! Cabin Pressure Gauge disabled.");
        while (1) vTaskDelay(1000);
    }
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
    tft.setColorDepth(COLOR_DEPTH_CABIN_PRESS);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    frameSpr.setColorDepth(COLOR_DEPTH_CABIN_PRESS);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    needleSpr.setColorDepth(COLOR_DEPTH_CABIN_PRESS);
    needleSpr.createSprite(23, 238);
    needleSpr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleSpr, cabinPressNeedle);

    subscribeToLedChange("PRESSURE_ALT", onPressureAltChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    CabinPressureGauge_bitTest();

#if RUN_CABIN_PRESSURE_GAUGE_AS_TASK
#if defined(IS_S3_PINS)
    xTaskCreatePinnedToCore(CabinPressureGauge_task, "CabinPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 1);
#else
    xTaskCreatePinnedToCore(CabinPressureGauge_task, "CabinPressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 0);
#endif
#endif

    debugPrintln("✅ Cabin Pressure Gauge (LovyanGFX, PSRAM double-buffered, DMA-safe) initialized");
}

void CabinPressureGauge_loop() {
#if !RUN_CABIN_PRESSURE_GAUGE_AS_TASK
    CabinPressureGauge_draw(false, false);
#endif
}

void CabinPressureGauge_notifyMissionStart() { gaugeDirty = true; }

// Visual self-test; use blocking flush to avoid DMA overlap during rapid sweeps
void CabinPressureGauge_bitTest() {
    int16_t originalU = angleU;
    const int STEP = 25, DELAY = 2;

    for (int i = 0; i <= 306; i += STEP) {
        angleU = map(i, 0, 306, -181, 125);
        gaugeDirty = true; CabinPressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 306; i >= 0; i -= STEP) {
        angleU = map(i, 0, 306, -181, 125);
        gaugeDirty = true; CabinPressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    angleU = originalU;
    gaugeDirty = true; CabinPressureGauge_draw(true, true);
}

void CabinPressureGauge_deinit() {
    waitDMADone();
    needleSpr.deleteSprite();
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