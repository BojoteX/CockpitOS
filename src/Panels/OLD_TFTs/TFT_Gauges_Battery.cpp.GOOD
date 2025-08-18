// CockpitOS — Battery Gauge (LovyanGFX, GC9A01) - Double-buffered PSRAM, DMA-safe

#define GAUGE_DRAW_MIN_INTERVAL_MS 13
#define RUN_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "CONSOLES_DIMMER"
#define COLOR_DEPTH_BATT 16

#include "../Globals.h"
#include "../TFT_Gauges_Batt.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>
#include <cstring>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// Select core (For this gauge, we always use core 0 for all devices)
#if defined(ARDUINO_LOLIN_S3_MINI)
#define BATT_CPU_CORE 0
#else 
#define BATT_CPU_CORE 0
#endif

// --- Pins ---
#if defined(ARDUINO_LOLIN_S3_MINI)
#define BATTERY_CS_PIN    38  
#define BATTERY_MOSI_PIN   8
#define BATTERY_SCLK_PIN   9
#define BATTERY_DC_PIN    14
#define BATTERY_RST_PIN   -1
#define BATTERY_MISO_PIN  -1
#else
#define BATTERY_CS_PIN    38  
#define BATTERY_MOSI_PIN   8
#define BATTERY_SCLK_PIN   9
#define BATTERY_DC_PIN    14 // 13 for Right Panel Controller
#define BATTERY_RST_PIN   -1 // 12 for Right Panel Controller
#define BATTERY_MISO_PIN  -1
#endif

// Pin overrides for Right Panel Controller
#if defined(LABEL_SET_RIGHT_PANEL_CONTROLLER)
#define BATTERY_DC_PIN    13
#define BATTERY_RST_PIN   12 
#endif

// --- Assets (240x240 bg, 15x88 needle) ---
#include "Assets/BatteryGauge/batBackground.h"
#include "Assets/BatteryGauge/batBackgroundNVG.h"
#include "Assets/BatteryGauge/batNeedle.h"
#include "Assets/BatteryGauge/batNeedleNVG.h"

// --- Misc ---
static constexpr bool shared_bus = true;
static constexpr bool use_lock = true;
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;
static constexpr uint16_t NVG_THRESHOLD = 6553;
static constexpr int16_t SCREEN_W = 240, SCREEN_H = 240;
static constexpr int16_t CENTER_X = 120, CENTER_Y = 120;
static constexpr int16_t NEEDLE_PIVOT_X = 7, NEEDLE_PIVOT_Y = 84;

// --- Panel binding ---
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
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_mosi = BATTERY_MOSI_PIN;
            cfg.pin_miso = BATTERY_MISO_PIN;
            cfg.pin_sclk = BATTERY_SCLK_PIN;
            cfg.pin_dc = BATTERY_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.readable = false;
            pcfg.pin_cs = BATTERY_CS_PIN;
            pcfg.pin_rst = BATTERY_RST_PIN;
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
static LGFX_Battery tft;
static lgfx::LGFX_Sprite frameSpr(&tft);
static lgfx::LGFX_Sprite needleU(&tft), needleE(&tft);

static volatile int16_t angleU = -150, angleE = 150;
static volatile int16_t lastDrawnAngleU = INT16_MIN, lastDrawnAngleE = INT16_MIN;
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

// --- Needle sprite builder ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
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
static void BatteryGauge_draw(bool force = false, bool blocking = false) {
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();
    int16_t u = angleU; if (u < -150) u = -150; else if (u > -30) u = -30;
    int16_t e = angleE; if (e < 30) e = 30; else if (e > 150) e = 150;

    const bool shouldDraw = force || gaugeDirty || (u != lastDrawnAngleU) || (e != lastDrawnAngleE);
    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleU = u; lastDrawnAngleE = e;
    gaugeDirty = false;

    const uint16_t* bg = (currentLightingMode == 0) ? batBackground : batBackgroundNVG;
    const uint16_t* needleImg = (currentLightingMode == 0) ? batNeedle : batNeedleNVG;

    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleU, needleImg);
        buildNeedle(needleE, needleImg);
        lastNeedleMode = currentLightingMode;
    }

    frameSpr.fillScreen(TFT_BLACK);
    frameSpr.pushImage(0, 0, SCREEN_W, SCREEN_H, bg);
    needleU.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);
    needleE.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)e, 1.0f, 1.0f, TRANSPARENT_KEY);

    uint16_t* buf = dmaFrame[dmaIdx ^ 1];
    std::memcpy(buf, frameSpr.getBuffer(), FRAME_BYTES);
    flushFrameToDisplay(buf, blocking);
    dmaIdx ^= 1;
}

// --- Task ---
static void BatteryGauge_task(void*) {
    for (;;) {
        BatteryGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void BatteryGauge_init() {
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
    tft.setColorDepth(COLOR_DEPTH_BATT);
    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    frameSpr.setColorDepth(COLOR_DEPTH_BATT);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    needleU.setColorDepth(COLOR_DEPTH_BATT);
    needleU.createSprite(15, 88);
    needleU.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleU, batNeedle);

    needleE.setColorDepth(COLOR_DEPTH_BATT);
    needleE.createSprite(15, 88);
    needleE.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    buildNeedle(needleE, batNeedle);

    subscribeToLedChange("VOLT_U", onBatVoltUChange);
    subscribeToLedChange("VOLT_E", onBatVoltEChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    BatteryGauge_bitTest();

#if RUN_GAUGE_AS_TASK
    xTaskCreatePinnedToCore(BatteryGauge_task, "BatteryGaugeTask", 4096, nullptr, 2, &tftTaskHandle, BATT_CPU_CORE);
#endif

    debugPrintln("✅ Battery Gauge (LovyanGFX, PSRAM double-buffered, DMA-safe) initialized");
}

void BatteryGauge_loop() {
#if !RUN_GAUGE_AS_TASK
    BatteryGauge_draw(false, false);
#endif
}

void BatteryGauge_notifyMissionStart() { gaugeDirty = true; }

// Visual self-test; use blocking flush to avoid DMA overlap during rapid sweeps
void BatteryGauge_bitTest() {
    int16_t origU = angleU, origE = angleE;
    const int STEP = 10, DELAY = 10;
    for (int i = 0; i <= 120; i += STEP) {
        angleU = map(i, 0, 120, -150, -30);
        angleE = map(i, 0, 120, 150, 30);
        gaugeDirty = true; BatteryGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    for (int i = 120; i >= 0; i -= STEP) {
        angleU = map(i, 0, 120, -150, -30);
        angleE = map(i, 0, 120, 150, 30);
        gaugeDirty = true; BatteryGauge_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    angleU = origU; angleE = origE;
    gaugeDirty = true; BatteryGauge_draw(true, true);
}

void BatteryGauge_deinit() {
    waitDMADone();
    needleU.deleteSprite();
    needleE.deleteSprite();
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