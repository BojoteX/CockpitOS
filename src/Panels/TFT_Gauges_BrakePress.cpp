// TFT_Gauges_BrakePress.cpp - CockpitOS Brake Pressure Gauge (LovyanGFX, GC9A01) - Double-buffered PSRAM, DMA-safe

#define BRAKE_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS 13   // 77 Hz
#define RUN_BRAKE_PRESSURE_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "INST_PNL_DIMMER"
#define COLOR_DEPTH_BRAKE_PRESS 16

#include "../Globals.h"
#include "../TFT_Gauges_BrakePress.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>
#include <cstring>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// Select core (For this gauge, we always use core 0 for all devices)
#if defined(ARDUINO_LOLIN_S3_MINI)
#define BRAKEPRESS_CPU_CORE 0
#else 
#define BRAKEPRESS_CPU_CORE 0
#endif

// --- Pins ---
#if defined(ARDUINO_LOLIN_S3_MINI)
#define BRAKE_PRESSURE_CS_PIN    38  
#define BRAKE_PRESSURE_MOSI_PIN   8
#define BRAKE_PRESSURE_SCLK_PIN   9
#define BRAKE_PRESSURE_DC_PIN    14
#define BRAKE_PRESSURE_RST_PIN   -1
#define BRAKE_PRESSURE_MISO_PIN  -1
#else
#define BRAKE_PRESSURE_CS_PIN    38  
#define BRAKE_PRESSURE_MOSI_PIN   8
#define BRAKE_PRESSURE_SCLK_PIN   9
#define BRAKE_PRESSURE_DC_PIN    14 // 13 for Right Panel Controller
#define BRAKE_PRESSURE_RST_PIN   -1 // 12 for Right Panel Controller
#define BRAKE_PRESSURE_MISO_PIN  -1
#endif

// --- Assets (240x240 bg, 15x150 needle) ---
#include "Assets/BrakePressure/brakePressBackground.h"
#include "Assets/BrakePressure/brakePressBackgroundNVG.h"
#include "Assets/BrakePressure/brakePressNeedle.h"
#include "Assets/BrakePressure/brakePressNeedleNVG.h"

// --- Misc ---
static constexpr bool shared_bus = true;
static constexpr bool use_lock = true;
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;
static constexpr uint16_t NVG_THRESHOLD = 6553;
static constexpr int16_t SCREEN_W = 240, SCREEN_H = 240;
static constexpr int16_t CENTER_X = 120, CENTER_Y = 239;
static constexpr int16_t NEEDLE_PIVOT_X = 7, NEEDLE_PIVOT_Y = 150;

// --- Panel binding ---
class LGFX_BrakePress : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;
    lgfx::Panel_GC9A01 _panel;
public:
    LGFX_BrakePress() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 0;
            cfg.spi_3wire = false;
            cfg.use_lock = use_lock;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_mosi = BRAKE_PRESSURE_MOSI_PIN;
            cfg.pin_miso = BRAKE_PRESSURE_MISO_PIN;
            cfg.pin_sclk = BRAKE_PRESSURE_SCLK_PIN;
            cfg.pin_dc = BRAKE_PRESSURE_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.readable = false;
            pcfg.pin_cs = BRAKE_PRESSURE_CS_PIN;
            pcfg.pin_rst = BRAKE_PRESSURE_RST_PIN;
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
static LGFX_BrakePress tft;
static lgfx::LGFX_Sprite frameSpr(&tft);
static lgfx::LGFX_Sprite needleSpr(&tft);

// Live values
static volatile int16_t angleU = -25;
static volatile int16_t lastDrawnAngleU = INT16_MIN;
static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0; // 0 = DAY, 2 = NVG
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

// --- Build needle sprite ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, 15, 150, img);
}

// --- DCS-BIOS callback for HYD_IND_BRAKE ---
static void onBrakePressureChange(const char*, uint16_t value, uint16_t) {
    int16_t newAngle = map(value, 0, 65535, -25, 25);
    if (newAngle != angleU) { angleU = newAngle; gaugeDirty = true; }
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

// --- Double-buffered draw ---
static void BrakePressureGauge_draw(bool force = false, bool blocking = false) {
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();
    int16_t u = angleU; if (u < -25) u = -25; else if (u > 25) u = 25;

    const bool shouldDraw = force || gaugeDirty || (u != lastDrawnAngleU);
    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < BRAKE_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleU = u;
    gaugeDirty = false;

    const uint16_t* bg = (currentLightingMode == 0)
        ? brakePressBackground
        : brakePressBackgroundNVG;
    const uint16_t* needleImg = (currentLightingMode == 0)
        ? brakePressNeedle
        : brakePressNeedleNVG;

    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleSpr, needleImg);
        lastNeedleMode = currentLightingMode;
    }

    frameSpr.fillScreen(TFT_BLACK);
    frameSpr.pushImage(0, 0, SCREEN_W, SCREEN_H, bg);
    needleSpr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);

    uint16_t* buf = dmaFrame[dmaIdx ^ 1];
    std::memcpy(buf, frameSpr.getBuffer(), FRAME_BYTES);
    flushFrameToDisplay(buf, blocking);
    dmaIdx ^= 1;
}

// --- Task ---
static void BrakePressureGauge_task(void*) {
    for (;;) {
        BrakePressureGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void BrakePressureGauge_init() {
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
    tft.setColorDepth(COLOR_DEPTH_BRAKE_PRESS);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    frameSpr.setColorDepth(COLOR_DEPTH_BRAKE_PRESS);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    needleSpr.setColorDepth(COLOR_DEPTH_BRAKE_PRESS);
    needleSpr.createSprite(15, 150);
    needleSpr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleSpr, brakePressNeedle);

    subscribeToLedChange("HYD_IND_BRAKE", onBrakePressureChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    BrakePressureGauge_bitTest();

#if RUN_BRAKE_PRESSURE_GAUGE_AS_TASK
    xTaskCreatePinnedToCore(BrakePressureGauge_task, "BrakePressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, BRAKEPRESS_CPU_CORE);
#endif

    debugPrintln("✅ Brake Pressure Gauge (LovyanGFX, PSRAM double-buffered, DMA-safe) initialized");
}

void BrakePressureGauge_loop() {
#if !RUN_BRAKE_PRESSURE_GAUGE_AS_TASK
    BrakePressureGauge_draw(false, false);
#endif
}

void BrakePressureGauge_notifyMissionStart() { gaugeDirty = true; }

// Visual self-test; use blocking flush to avoid DMA overlap during rapid sweeps
void BrakePressureGauge_bitTest() {
    int16_t originalU = angleU;
    const int STEP = 5, DELAY = 2;
    for (int i = 0; i <= 50; i += STEP) {
        angleU = map(i, 0, 50, -25, 25);
        gaugeDirty = true; BrakePressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 50; i >= 0; i -= STEP) {
        angleU = map(i, 0, 50, -25, 25);
        gaugeDirty = true; BrakePressureGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    angleU = originalU;
    gaugeDirty = true; BrakePressureGauge_draw(true, true);
}

void BrakePressureGauge_deinit() {
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