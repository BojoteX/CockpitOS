#define GAUGE_DRAW_MIN_INTERVAL_MS 30   // 30Hz
#define RUN_GAUGE_AS_TASK 1

#include "../Globals.h"
#include "../TFT_Gauges.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

// NVG TFT images
#include "Assets/BatteryGauge/batBackgroundNVG.h"
#include "Assets/BatteryGauge/batNeedleNVG.h"

// Day TFT images
#include "Assets/BatteryGauge/batBackground.h"
#include "Assets/BatteryGauge/batNeedle.h"
#include <TFT_eSPI.h>

static TFT_eSPI* tft = nullptr;
static TFT_eSprite* needleU = nullptr;
static TFT_eSprite* needleE = nullptr;
static TFT_eSprite* gaugeBack = nullptr;
static constexpr uint8_t colorDepth = 8;

// --- PSRAM for background only (OPTIONAL) ---
static uint16_t* psramBackground = nullptr;

// --- Battery state ---
static volatile int16_t angleU = 0, angleE = 0;
static volatile int16_t lastDrawnAngleU = INT16_MIN, lastDrawnAngleE = INT16_MIN;
static volatile bool gaugeDirty = false;
static unsigned long lastDrawTime = 0;

static TaskHandle_t tftTaskHandle = nullptr;

// --- Utility ---
static inline void BatteryGauge_CS_ON() { digitalWrite(BATTERY_CS_PIN, LOW); }
static inline void BatteryGauge_CS_OFF() { digitalWrite(BATTERY_CS_PIN, HIGH); }

static void BatteryGauge_draw(bool force = 0);
static void BatteryGauge_task(void*);

// --- DCS-BIOS: Mark gauge dirty on change ---
static void onBatVoltUChange(const char*, uint16_t value, uint16_t) {
    int16_t newAngleU = map(value, 0, 65535, -150, -30);
    if (newAngleU != angleU) {
        angleU = newAngleU;
        gaugeDirty = true;
    }
}
static void onBatVoltEChange(const char*, uint16_t value, uint16_t) {
    int16_t newAngleE = map(value, 0, 65535, 150, 30);
    if (newAngleE != angleE) {
        angleE = newAngleE;
        gaugeDirty = true;
    }
}

// --- Draw background ONCE at startup or mission start ---
static void BatteryGauge_drawBackground() {
    BatteryGauge_CS_ON();
    tft->pushImage(0, 0, 240, 240, psramBackground ? psramBackground : batBackground);
    BatteryGauge_CS_OFF();
}

// --- DMA-only, fast needle updates ---
// (Background is static, drawn by CPU; needles use DMA from sprite IRAM buffer)
static void BatteryGauge_draw(bool force) {
    if (!tft || !needleU || !needleE || !gaugeBack) return;
    if (!isMissionRunning() && !force) return;

    unsigned long now = millis();
    if (!gaugeDirty) return;
    if (now - lastDrawTime < GAUGE_DRAW_MIN_INTERVAL_MS) return;

    int16_t u = angleU;
    int16_t e = angleE;
    if (u == lastDrawnAngleU && e == lastDrawnAngleE) return;

    lastDrawTime = now;
    lastDrawnAngleU = u;
    lastDrawnAngleE = e;
    gaugeDirty = false;

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_DRAW);
#endif

    BatteryGauge_CS_ON();

    // 1. Clear and redraw background on composite sprite
    gaugeBack->fillSprite(TFT_TRANSPARENT);
    gaugeBack->pushImage(0, 0, 240, 240, psramBackground ? psramBackground : batBackground);

    // 2. Draw/rotate needles into composite
    needleU->fillSprite(TFT_TRANSPARENT);
    needleU->pushImage(0, 0, 15, 88, batNeedle);
    needleU->pushRotated(gaugeBack, u, TFT_TRANSPARENT);

    needleE->fillSprite(TFT_TRANSPARENT);
    needleE->pushImage(0, 0, 15, 88, batNeedle);
    needleE->pushRotated(gaugeBack, e, TFT_TRANSPARENT);

    gaugeBack->pushSprite(0, 0, TFT_TRANSPARENT);

    BatteryGauge_CS_OFF();

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_DRAW);
#endif

}

// --- FreeRTOS task (if used) ---
static void BatteryGauge_task(void*) {
    while (1) {
        BatteryGauge_draw();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- INIT: Run ONCE at boot ---
void BatteryGauge_init() {

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_INIT);
#endif

    pinMode(BATTERY_CS_PIN, OUTPUT);
    BatteryGauge_CS_OFF();

    // PSRAM background allocation (optional)
    if (!psramBackground) {
        psramBackground = (uint16_t*)PS_MALLOC(batBackgroundLen * sizeof(uint16_t));
        if (psramBackground) {
            memcpy(psramBackground, batBackground, batBackgroundLen * sizeof(uint16_t));
            debugPrintln("[PSRAM] ✅ batBackground copied to PSRAM.");
        }
        else {
            debugPrintln("[PSRAM] ❌ Failed to allocate batBackground in PSRAM!");
        }
    }

    tft = new TFT_eSPI;
    tft->init();
    tft->fillScreen(TFT_BLACK);

    gaugeBack = new TFT_eSprite(tft);
    gaugeBack->setColorDepth(colorDepth);
    gaugeBack->createSprite(240, 240);
    gaugeBack->setSwapBytes(true);
    gaugeBack->setPivot(120, 120);

    BatteryGauge_drawBackground();

    // --- Needles: IRAM sprites only ---
    needleU = new TFT_eSprite(tft);
    if (!needleU->createSprite(15, 88)) {
        debugPrintln("FATAL: needleU createSprite failed!");
        while (1); // halt!
    }
    needleU->setColorDepth(colorDepth);
    needleU->setSwapBytes(true);
    needleU->setPivot(7, 84);

    needleE = new TFT_eSprite(tft);
    if (!needleE->createSprite(15, 88)) {
        debugPrintln("FATAL: needleE createSprite failed!");
        while (1);
    }
    needleE->setColorDepth(colorDepth);
    needleE->setSwapBytes(true);
    needleE->setPivot(7, 84);

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_INIT);
#endif

    subscribeToLedChange("VOLT_U", onBatVoltUChange);
    subscribeToLedChange("VOLT_E", onBatVoltEChange);

    if (tftTaskHandle == nullptr) {
#if RUN_GAUGE_AS_TASK
    #if defined(IS_S3_PINS)
		xTaskCreatePinnedToCore(BatteryGauge_task, "BatteryGaugeTask", 4096, NULL, 2, &tftTaskHandle, 1); // S3 has 2 cores, use core 1 for TFT tasks
    #else
		xTaskCreatePinnedToCore(BatteryGauge_task, "BatteryGaugeTask", 4096, NULL, 2, &tftTaskHandle, 0); // S2 has 1 core, use core 0
    #endif
#endif
    }

    BatteryGauge_bitTest();

    debugPrintln("✅ BatteryGauge display initialized (DMA needles, static background)");
}

void BatteryGauge_loop() {
#if !RUN_GAUGE_AS_TASK
    BatteryGauge_draw();
#endif
}

void BatteryGauge_notifyMissionStart() {
    BatteryGauge_drawBackground();
    gaugeDirty = true;
}

void BatteryGauge_bitTest() {
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BITTEST);
#endif
    for (int i = 0; i <= 120; i += 5) {
        angleU = map(i, 0, 120, -150, -30);
        angleE = map(i, 0, 120, 150, 30);
        gaugeDirty = true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    for (int i = 120; i >= 0; i -= 5) {
        angleU = map(i, 0, 120, -150, -30);
        angleE = map(i, 0, 120, 150, 30);
        gaugeDirty = true;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    BatteryGauge_draw(true);
#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_BITTEST);
#endif
}

void BatteryGauge_deinit() {
    if (needleU) { needleU->deleteSprite(); delete needleU; needleU = nullptr; }
    if (needleE) { needleE->deleteSprite(); delete needleE; needleE = nullptr; }
    if (gaugeBack) { gaugeBack->deleteSprite(); delete gaugeBack; gaugeBack = nullptr; }
    if (tft) { delete tft; tft = nullptr; }
    if (tftTaskHandle) {
        vTaskDelete(tftTaskHandle);
        tftTaskHandle = nullptr;
    }
    if (psramBackground) { PS_FREE(psramBackground); psramBackground = nullptr; }
}
