// TFT_Gauges_BrakePress.cpp - CockpitOS Brake Pressure Gauge (LovyanGFX)

#define BRAKE_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS 13   // 77 Hz
#define RUN_BRAKE_PRESSURE_GAUGE_AS_TASK 1
#define BACKLIGHT_LABEL "INST_PANEL_DIMMER" // Other option is CONSOLES_DIMMER depending on the gauge type

// Pin Definitions (S2 device)
// #define BRAKE_PRESSURE_CS_PIN   36  // [BLUE] CS pin for the brake pressure gauge
#define BRAKE_PRESSURE_MOSI_PIN  8  // [YELLOW] MOSI pin for the brake pressure gauge
#define BRAKE_PRESSURE_SCLK_PIN  9  // [ORANGE] SCLK pin for the brake pressure gauge
#define BRAKE_PRESSURE_DC_PIN   13  // [GREEN] DC pin for the brake pressure gauge
#define BRAKE_PRESSURE_RST_PIN  12  // RESET pin for the brake pressure gauge

// Not used in this implementation
#define BRAKE_PRESSURE_MISO_PIN -1  // MISO pin for the brake pressure gauge

#include "../Globals.h"
#include "../TFT_Gauges_BrakePress.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>

// Brake Pressure Gauge Assets (day/NVG, 240x240 bg, 15x150 needle)
#include "Assets/BrakePressure/brakePressBackground.h"
#include "Assets/BrakePressure/brakePressBackgroundNVG.h"
#include "Assets/BrakePressure/brakePressNeedle.h"
#include "Assets/BrakePressure/brakePressNeedleNVG.h"

// --- Misc ---
static constexpr bool shared_bus    = true;
static constexpr bool use_lock      = true;

// ---------------------- Panel binding (LovyanGFX) ----------------------
class LGFX_BrakePress : public lgfx::LGFX_Device {
    lgfx::Bus_SPI _bus;                 // SPI bus
    lgfx::Panel_GC9A01 _panel;          // GC9A01 panel
public:
    LGFX_BrakePress() {
        { // Bus config
            auto cfg                = _bus.config();
            cfg.spi_host            = SPI2_HOST;           // ESP32-S2/S3: SPI2_HOST is correct
            cfg.spi_mode            = 0;
            cfg.freq_write          = 80000000;            // 80 MHz write
            cfg.freq_read           = 0;                   // read not used, but set sane value
            cfg.spi_3wire           = false;               // write-only panel
            cfg.use_lock            = use_lock;
            cfg.dma_channel         = 1;                   // auto/1 are fine
            cfg.pin_mosi            = BRAKE_PRESSURE_MOSI_PIN;
            cfg.pin_miso            = BRAKE_PRESSURE_MISO_PIN;            // -1
            cfg.pin_sclk            = BRAKE_PRESSURE_SCLK_PIN;
            cfg.pin_dc              = BRAKE_PRESSURE_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        { // Panel config
            auto pcfg               = _panel.config();
            pcfg.pin_cs             = BRAKE_PRESSURE_CS_PIN;
            pcfg.pin_rst            = BRAKE_PRESSURE_RST_PIN;      // can be -1
            pcfg.pin_busy           = -1;
            pcfg.memory_width       = 240;
            pcfg.memory_height      = 240;
            pcfg.panel_width        = 240;
            pcfg.panel_height       = 240;
            pcfg.offset_x           = 0;
            pcfg.offset_y           = 0;
            pcfg.offset_rotation    = 0;
            pcfg.bus_shared         = shared_bus;
            pcfg.invert             = true;
            _panel.config(pcfg);
        }
        setPanel(&_panel);
    }
};

// --- State ---
static LGFX_BrakePress tft;
static lgfx::LGFX_Sprite needleU(&tft);

// Pivot point on the needle sprite is at (7, 150), However, we draw it at the bottom center of the panel (120, 240) as needle pivots from the bottom.
static constexpr uint8_t    COLOR_DEPTH         = 16;
static constexpr int16_t    CENTER_X            = 120;
static constexpr int16_t    CENTER_Y            = 239; // Bottom center of the panel (240x240) offset by 1 pixel to account for the pivot point of the needle sprite
static constexpr uint16_t   TRANSPARENT_KEY     = 0x0120;
static volatile int16_t     angleU              = -25; // Initial angle for the needle, matches reference logic (-25 to 25 PSI) 
static volatile int16_t     lastDrawnAngleU     = INT16_MIN;
static volatile bool        gaugeDirty          = false;
static volatile uint8_t     currentLightingMode = 0; // 0 = DAY, 2 = NVG
static unsigned long        lastDrawTime        = 0;
static TaskHandle_t         tftTaskHandle       = nullptr;
static uint8_t              lastNeedleMode      = 0xFF;
static constexpr uint16_t   NVG_THRESHOLD       = 6553;

// --- Build needle sprite helper ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img)
{
    spr.fillScreen(TRANSPARENT_KEY);      // Clear needle sprite to transparent
    spr.setSwapBytes(false);              // Match header byte order (false matches your working code)
    spr.pushImage(0, 0, 15, 150, img);    // 15x150 needle!
}

// --- DCS-BIOS callback for HYD_IND_BRAKE ---
static void onBrakePressureChange(const char*, uint16_t value, uint16_t)
{
    int16_t newAngle = map(value, 0, 65535, -25, 25); // Matches reference logic!
    if (newAngle != angleU) { angleU = newAngle; gaugeDirty = true; }
}

// --- Core drawing ---
static void BrakePressureGauge_draw(bool force = false)
{
    if (!force && !isMissionRunning()) return;

    const unsigned long now = millis();
    int16_t u = angleU; // Current angle in U (0-65535 mapped to -25 to 25 PSI)
    if (u < -25) u = -25; else if (u > 25) u = 25; // Clamp the angle to -25 to 25 range

    const bool shouldDraw = force
        || gaugeDirty
        || (u != lastDrawnAngleU);

    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < BRAKE_PRESSURE_GAUGE_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnAngleU = u;
    gaugeDirty = false;

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BRAKE_PRESSURE_DRAW);
#endif

#if DEBUG_ENABLED
    debugPrintf("🔋 LGFX Brake draw: U=%d, mode=%d\n", u, currentLightingMode);
#endif

    // --- Asset selection ---
    const uint16_t* bg = (currentLightingMode == 0)
        ? brakePressBackground
        : brakePressBackgroundNVG;

    const uint16_t* needleImg = (currentLightingMode == 0)
        ? brakePressNeedle
        : brakePressNeedleNVG;

	// --- Update needle sprite if lighting mode changed ---
    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needleU, needleImg);
        lastNeedleMode = currentLightingMode;
    }

    // --- Draw background directly to panel ---
    tft.startWrite();
    tft.pushImageDMA(0, 0, 240, 240, bg);

    // --- Draw rotated needle sprite over background ---
    needleU.pushRotateZoom(&tft, CENTER_X, CENTER_Y, (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);

    tft.endWrite();

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_BRAKE_PRESSURE_DRAW);
#endif
}

// --- FreeRTOS task ---
static void BrakePressureGauge_task(void*)
{
    for (;;) {
        BrakePressureGauge_draw(false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void onBrakePressureGaugeLightingChange(const char*, uint16_t value)
{
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
void BrakePressureGauge_setLightingMode(uint8_t mode)
{
    onBrakePressureGaugeLightingChange(nullptr, mode);
}

void BrakePressureGauge_init()
{
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BRAKE_PRESSURE_INIT);
#endif

    tft.init();
    tft.setColorDepth(16);

    tft.setRotation(0);
    tft.setSwapBytes(true);
    tft.fillScreen(TFT_BLACK);

    // Needle sprite (15x150, pivot 7,150)
    needleU.setColorDepth(COLOR_DEPTH);
    needleU.createSprite(15, 150);
    needleU.setPivot(7, 150);

    // DCS-BIOS subscription: HYD_IND_BRAKE!
    subscribeToLedChange("HYD_IND_BRAKE", onBrakePressureChange);

	// DCS-BIOS subscription: COCKKPIT_LIGHT_MODE_SW! (Not a typo, this is the correct DCS-BIOS event for lighting mode)
    // subscribeToSelectorChange("COCKKPIT_LIGHT_MODE_SW", onBrakePressureGaugeLightingChange);
    subscribeToLedChange(BACKLIGHT_LABEL, onInstPanelDimmerChange);

    BrakePressureGauge_bitTest();

#if RUN_BRAKE_PRESSURE_GAUGE_AS_TASK
#if defined(IS_S3_PINS)
    xTaskCreatePinnedToCore(BrakePressureGauge_task, "BrakePressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 1);
#else
    xTaskCreatePinnedToCore(BrakePressureGauge_task, "BrakePressureGaugeTask", 4096, nullptr, 2, &tftTaskHandle, 0);
#endif
#endif

    debugPrintln("✅ Brake Pressure Gauge (LovyanGFX) initialized");

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_BRAKE_PRESSURE_INIT);
#endif
}

void BrakePressureGauge_loop()
{
#if !RUN_BRAKE_PRESSURE_GAUGE_AS_TASK
    BrakePressureGauge_draw(false);
#endif
}

void BrakePressureGauge_notifyMissionStart()
{
    gaugeDirty = true;
}

void BrakePressureGauge_bitTest()
{
#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_BRAKE_PRESSURE_BITTEST);
#endif

    int16_t originalU = angleU;
    const int STEP = 1; // <-- Increase this for faster sweep (try 10, 15, 20)
    const int DELAY = 2; // <-- Lower for less time per step

    // Sweep inward
    for (int i = 0; i <= 50; i += STEP) {
        angleU = map(i, 0, 50, -25, 25);
        gaugeDirty = true; BrakePressureGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    // Sweep outward
    for (int i = 50; i >= 0; i -= STEP) {
        angleU = map(i, 0, 50, -25, 25);
        gaugeDirty = true; BrakePressureGauge_draw(true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    angleU = originalU;
    gaugeDirty = true; BrakePressureGauge_draw(true);

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_BRAKE_PRESSURE_BITTEST);
#endif
}

void BrakePressureGauge_deinit()
{
    needleU.deleteSprite();
    if (tftTaskHandle) {
        vTaskDelete(tftTaskHandle);
        tftTaskHandle = nullptr;
    }
}