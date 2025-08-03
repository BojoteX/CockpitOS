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

// Track light modes
static uint8_t currentLightingMode = 0; // 0 = Day, 1 = Nite, 2 = NVG

static TFT_eSPI* tft = nullptr;
static TFT_eSprite* needleU = nullptr;
static TFT_eSprite* needleE = nullptr;
static TFT_eSprite* gaugeBack = nullptr;
static constexpr uint8_t colorDepth = 16;

// --- PSRAM for background only (OPTIONAL) ---
static uint16_t* psramBackgroundDay = nullptr;
static uint16_t* psramBackgroundNVG = nullptr;

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

// --- DMA-only, fast needle updates ---
// (Background is static, drawn by CPU; needles use DMA from sprite IRAM buffer)
static void BatteryGauge_draw(bool force) {
    if (!tft || !needleU || !needleE || !gaugeBack) return;
    if (!force && !isMissionRunning()) return;

    unsigned long now = millis();

    // Snapshot angles ONCE — for consistency
    int16_t u = angleU;
    int16_t e = angleE;

    bool shouldDraw = force ||
        gaugeDirty ||
        (u != lastDrawnAngleU || e != lastDrawnAngleE);

    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleU = u;
    lastDrawnAngleE = e;
    gaugeDirty = false;

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_DRAW);
#endif

#if DEBUG_ENABLED
    debugPrintf("🔋 Drawing Battery Gauge: U=%d, E=%d using lightMode=%d\n", u, e, currentLightingMode);
#endif

    BatteryGauge_CS_ON();

    // Background selection
    const uint16_t* bg = (currentLightingMode == 0) ? psramBackgroundDay : psramBackgroundNVG;
    gaugeBack->pushImage(0, 0, 240, 240, bg);

    // Needle image selection
    const uint16_t* activeNeedle = (currentLightingMode == 0) ? batNeedle : batNeedleNVG;

    needleU->fillSprite(TFT_TRANSPARENT);
    needleU->pushImage(0, 0, 15, 88, activeNeedle);
    needleU->pushRotated(gaugeBack, u, TFT_TRANSPARENT);  // ✅ use snapshotted value

    needleE->fillSprite(TFT_TRANSPARENT);
    needleE->pushImage(0, 0, 15, 88, activeNeedle);
    needleE->pushRotated(gaugeBack, e, TFT_TRANSPARENT);  // ✅ use snapshotted value

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

// --- Lighting Mode Setter ---
void BatteryGauge_setLightingMode(uint8_t mode) {
    // Normalize lighting mode:
    // 0 = Day
    // 1 or 2 = NVG
    // any other = NVG fallback
    uint8_t normalized = (mode == 0) ? 0 : 2;

    if (normalized == currentLightingMode) return;
    currentLightingMode = normalized;

#if DEBUG_ENABLED
    // Optional logging for debug
    debugPrintf("⚙️ Lighting mode set to %s\n",
        currentLightingMode == 0 ? "DAY" : "NVG");
#endif

	// Optional: force immediate redraw
    gaugeDirty = true;
    BatteryGauge_draw(true);
}

static void onBatteryGaugeLightingChange(const char* label, uint16_t value) {
    BatteryGauge_setLightingMode(value);
}

// --- INIT: Run ONCE at boot ---
void BatteryGauge_init() {
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_INIT);
#endif

    pinMode(BATTERY_CS_PIN, OUTPUT);
    BatteryGauge_CS_OFF();

    // Allocate both backgrounds in PSRAM (fully static)
    psramBackgroundDay = (uint16_t*)PS_MALLOC(batBackgroundLen * sizeof(uint16_t));
    psramBackgroundNVG = (uint16_t*)PS_MALLOC(batBackgroundLen * sizeof(uint16_t));

    if (psramBackgroundDay && psramBackgroundNVG) {
        memcpy(psramBackgroundDay, batBackground, batBackgroundLen * sizeof(uint16_t));
        memcpy(psramBackgroundNVG, batBackgroundNVG, batBackgroundLen * sizeof(uint16_t));
        debugPrintln("[PSRAM] ✅ Both backgrounds loaded into PSRAM.");
    }
    else {
        debugPrintln("[PSRAM] ❌ Failed to allocate both backgrounds!");
        // Optional: fallback logic here if needed
    }

    tft = new TFT_eSPI;
    tft->init();
    tft->setSwapBytes(true);
    tft->fillScreen(TFT_BLACK);

    gaugeBack = new TFT_eSprite(tft);
    gaugeBack->setColorDepth(colorDepth);
    gaugeBack->setSwapBytes(true);
    gaugeBack->createSprite(240, 240);
    gaugeBack->setPivot(120, 120);

    // --- Needles: IRAM sprites only ---
    needleU = new TFT_eSprite(tft);
    needleU->setColorDepth(colorDepth);
    needleU->setSwapBytes(true);
    if (!needleU->createSprite(15, 88)) {
        debugPrintln("FATAL: needleU createSprite failed!");
        while (1);
    }
    needleU->setPivot(7, 84);

    needleE = new TFT_eSprite(tft);
    needleE->setColorDepth(colorDepth);
    needleE->setSwapBytes(true);
    if (!needleE->createSprite(15, 88)) {
        debugPrintln("FATAL: needleE createSprite failed!");
        while (1);
    }
    needleE->setPivot(7, 84);

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_INIT);
#endif

    subscribeToLedChange("VOLT_U", onBatVoltUChange);
    subscribeToLedChange("VOLT_E", onBatVoltEChange);
    subscribeToSelectorChange("COCKKPIT_LIGHT_MODE_SW", onBatteryGaugeLightingChange);

    if (tftTaskHandle == nullptr) {
#if RUN_GAUGE_AS_TASK
#if defined(IS_S3_PINS)
        xTaskCreatePinnedToCore(BatteryGauge_task, "BatteryGaugeTask", 4096, NULL, 2, &tftTaskHandle, 1);
#else
        xTaskCreatePinnedToCore(BatteryGauge_task, "BatteryGaugeTask", 4096, NULL, 2, &tftTaskHandle, 0);
#endif
#endif
    }

    BatteryGauge_bitTest();
    debugPrintln("✅ BatteryGauge display initialized (DMA needles, dual PSRAM backgrounds)");
}

void BatteryGauge_loop() {
#if !RUN_GAUGE_AS_TASK
    BatteryGauge_draw();
#endif
}

void BatteryGauge_notifyMissionStart() {
    gaugeDirty = true;
    BatteryGauge_draw(true); // Force redraw on mission start
}

void BatteryGauge_bitTest() {
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BITTEST);
#endif

    int16_t originalU = angleU;
    int16_t originalE = angleE;

    // Sweep inward
    for (int i = 0; i <= 120; i += 5) {
        angleU = map(i, 0, 120, -150, -30);
        angleE = map(i, 0, 120, 150, 30);
        gaugeDirty = true;
        BatteryGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Sweep outward
    for (int i = 120; i >= 0; i -= 5) {
        angleU = map(i, 0, 120, -150, -30);
        angleE = map(i, 0, 120, 150, 30);
        gaugeDirty = true;
        BatteryGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Final: cold & dark parked state (symmetrical)
    angleU = -150;   // left needle at 7 o'clock
    angleE = 150;    // right needle at 5 o'clock
    gaugeDirty = true;
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

    if (psramBackgroundDay) {
        PS_FREE(psramBackgroundDay);
        psramBackgroundDay = nullptr;
    }
    if (psramBackgroundNVG) {
        PS_FREE(psramBackgroundNVG);
        psramBackgroundNVG = nullptr;
    }
}
