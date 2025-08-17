// TFT_Gauges_RadarAlt.cpp - CockpitOS Radar Altimeter (LovyanGFX, ST77916/61 @ 360×360) - Double-buffered PSRAM, DMA-safe

#define RADARALT_DRAW_MIN_INTERVAL_MS 100
#define RUN_RADARALT_AS_TASK 1
#define BACKLIGHT_LABEL "INST_PNL_DIMMER"
#define COLOR_DEPTH_RADARALT 16

#include "../Globals.h"
#include "../TFT_Gauges_RadarAlt.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include <LovyanGFX.hpp>
#include <cstring>

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#endif

// Select core (For this gauge, we always use core 0 for all devices)
#if defined(ARDUINO_LOLIN_S3_MINI)
    #define RA_CPU_CORE 0
#else 
    #define RA_CPU_CORE 0
#endif

// --- Pins ---
#if defined(ARDUINO_LOLIN_S3_MINI)
    #define RADARALT_MOSI_PIN 11  // SDA (Yellow)
    #define RADARALT_SCLK_PIN 12  // SCL (Orange)
    #define RADARALT_CS_PIN   10  // Chip Select (Blue)
    #define RADARALT_DC_PIN   13  // Data/Command (Green)
    #define RADARALT_RST_PIN  -1  // Reset (White)
    #define RADARALT_MISO_PIN -1  // Unused
#else
    #define RADARALT_MOSI_PIN 11  // SDA (Yellow)
    #define RADARALT_SCLK_PIN 12  // SCL (Orange)
    #define RADARALT_CS_PIN   10  // Chip Select (Blue)
    #define RADARALT_DC_PIN   13  // Data/Command (Green)
    #define RADARALT_RST_PIN  -1  // Reset (White)
    #define RADARALT_MISO_PIN -1  // Unused
#endif

// Pin overrides for Custom Front Right Console
#if defined(LABEL_SET_CUSTOM_FRONT_RIGHT)
    #define RADARALT_DC_PIN   13  // Data/Command (Green)
    #define RADARALT_CS_PIN   14  // Chip Select (Blue)
    #define RADARALT_MOSI_PIN 16  // SDA (Yellow)
    #define RADARALT_SCLK_PIN 17  // SCL (Orange)
    #define RADARALT_RST_PIN  -1  // Reset (White)
    #define RADARALT_MISO_PIN -1  // Unused
#endif

// --- Assets ---
#include "Assets/RadarAltimeter/radarAltBackground.h"
#include "Assets/RadarAltimeter/radarAltNeedle.h"
#include "Assets/RadarAltimeter/radarAltMinHeightPointer.h"
#include "Assets/RadarAltimeter/radarAltMinHeightPointerNVG.h"
#include "Assets/RadarAltimeter/radarAltGreenLamp.h"
#include "Assets/RadarAltimeter/radarAltLowAltWarning.h"
#include "Assets/RadarAltimeter/radarAltOffFlag.h"
#include "Assets/RadarAltimeter/radarAltOffFlagNVG.h"
#include "Assets/RadarAltimeter/radarAltBackgroundNVG.h"
#include "Assets/RadarAltimeter/radarAltNeedleNVG.h"

// --- Misc ---
static constexpr bool shared_bus = true;
static constexpr bool use_lock = true;
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;  // color not present in assets
static constexpr uint16_t NVG_THRESHOLD = 6553;
static constexpr int16_t SCREEN_W = 360, SCREEN_H = 360;
static constexpr int16_t LOWALT_X = 95, LOWALT_Y = 158;
static constexpr int16_t GREEN_X = 229, GREEN_Y = 158;
static constexpr int16_t OFF_X = 152, OFF_Y = 254;
static constexpr int16_t CENTER_X = 180, CENTER_Y = 180;

// Radar Altimeter angles
static constexpr int16_t RA_ANGLE_MIN = -17;
static constexpr int16_t RA_ANGLE_MAX = 325;

// Min Height Pointer angles
static constexpr int16_t MHP_ANGLE_MIN = -10;
static constexpr int16_t MHP_ANGLE_MAX = 325;

// --- Panel binding ---
class LGFX_RadarAlt : public lgfx::LGFX_Device {
    lgfx::Bus_SPI       _bus;
    lgfx::Panel_ST77961 _panel;
public:
    LGFX_RadarAlt() {
        {
            auto cfg = _bus.config();
            cfg.spi_host = SPI3_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 80000000;
            cfg.freq_read = 0;
            cfg.spi_3wire = false;
            cfg.use_lock = use_lock;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_mosi = RADARALT_MOSI_PIN;
            cfg.pin_miso = RADARALT_MISO_PIN;
            cfg.pin_sclk = RADARALT_SCLK_PIN;
            cfg.pin_dc = RADARALT_DC_PIN;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto pcfg = _panel.config();
            pcfg.readable = false;
            pcfg.pin_cs = RADARALT_CS_PIN;
            pcfg.pin_rst = RADARALT_RST_PIN;
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
static LGFX_RadarAlt tft;

// Compose target (once; PSRAM)
static lgfx::LGFX_Sprite frameSpr(&tft);

// Static sprites
static lgfx::LGFX_Sprite needle(&tft);
static lgfx::LGFX_Sprite pointerSpr(&tft);
static lgfx::LGFX_Sprite lowAltSpr(&tft);
static lgfx::LGFX_Sprite greenLampSpr(&tft);
static lgfx::LGFX_Sprite offFlagSpr(&tft);

// Live values
static volatile int16_t angleRA = 0;
static volatile int16_t angleMHP = 0;
static volatile bool    lowAltOn = false;
static volatile bool    greenOn = false;
static volatile bool    offFlag = false;

static volatile int16_t lastDrawnRA = INT16_MIN;
static volatile int16_t lastDrawnMHP = INT16_MIN;
static volatile bool    gaugeDirty = false;
static volatile uint8_t currentLightingMode = 0;   // 0=Day, 2=NVG

static uint8_t          lastNeedleMode = 0xFF;
static uint8_t          lastPointerMode = 0xFF;
static uint8_t          lastOffFlagMode = 0xFF;
static unsigned long    lastDrawTime = 0;
static TaskHandle_t     tftTaskHandle = nullptr;

// --- DMA-safe double buffer ---
static constexpr size_t FRAME_PIXELS = size_t(SCREEN_W) * size_t(SCREEN_H);
static constexpr size_t FRAME_BYTES = FRAME_PIXELS * sizeof(uint16_t);

static uint16_t* dmaFrame[2] = { nullptr, nullptr };
static uint8_t   dmaIdx = 0;
static bool      dmaBusy = false;

static inline void waitDMADone()
{
    if (dmaBusy) {
        tft.waitDMA();   // fence outstanding DMA
        dmaBusy = false;
    }
}

// --- Sprite builder helpers ---
static void buildNeedle(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, 76, 173, img);
}

static void buildPointer(lgfx::LGFX_Sprite& spr, const uint16_t* img) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, 23, 180, img);
}

static void buildLamp(lgfx::LGFX_Sprite& spr, const uint16_t* img, int w, int h) {
    spr.fillScreen(TRANSPARENT_KEY);
    spr.setSwapBytes(true);
    spr.pushImage(0, 0, w, h, img);
}

// --- DCS-BIOS callbacks ---
static void onRadarAltAngle(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, RA_ANGLE_MIN, RA_ANGLE_MAX);
    if (a != angleRA) { angleRA = a; gaugeDirty = true; }
}
static void onMinHeightAngle(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, MHP_ANGLE_MIN, MHP_ANGLE_MAX);
    if (a != angleMHP) { angleMHP = a; gaugeDirty = true; }
}
static void onLowAltLamp(const char*, uint16_t v, uint16_t) {
    bool on = (v != 0);
    if (on != lowAltOn) { lowAltOn = on; gaugeDirty = true; }
}
static void onGreenLamp(const char*, uint16_t v, uint16_t) {
    bool on = (v != 0);
    if (on != greenOn) { greenOn = on; gaugeDirty = true; }
}
static void onOffFlag(const char*, uint16_t v, uint16_t) {
    bool on = (v != 0);
    if (on != offFlag) { offFlag = on; gaugeDirty = true; }
}
static void onDimmerChange(const char*, uint16_t v, uint16_t) {
    uint8_t mode = (v > NVG_THRESHOLD) ? 2u : 0u;
    if (mode != currentLightingMode) { currentLightingMode = mode; gaugeDirty = true; }
}

// --- Flush helper (blocking or DMA) ---
static inline void flushFrameToDisplay(uint16_t* buf, bool blocking)
{
    tft.startWrite();
    if (blocking) {
        // synchronous path for BIT sweeps or diagnostics
        tft.pushImage(0, 0, SCREEN_W, SCREEN_H, buf);
        dmaBusy = false;
    }
    else {
        // fence any in-flight DMA, then kick a new one
        waitDMADone();
        tft.pushImageDMA(0, 0, SCREEN_W, SCREEN_H, buf);
        dmaBusy = true;
    }
    tft.endWrite();
}

// --- Double-buffered draw (no tearing) ---
static void RadarAlt_draw(bool force = false, bool blocking = false)
{
    if (!force && !isMissionRunning()) return;
    const unsigned long now = millis();

    int16_t ra = angleRA;
    if (ra < RA_ANGLE_MIN) ra = RA_ANGLE_MIN;
    else if (ra > RA_ANGLE_MAX) ra = RA_ANGLE_MAX;

    int16_t mhp = angleMHP;
    if (mhp < MHP_ANGLE_MIN) mhp = MHP_ANGLE_MIN;
    else if (mhp > MHP_ANGLE_MAX) mhp = MHP_ANGLE_MAX;

    const bool shouldDraw = force || gaugeDirty || (ra != lastDrawnRA) || (mhp != lastDrawnMHP);
    if (!shouldDraw) return;
    if (!force && (now - lastDrawTime < RADARALT_DRAW_MIN_INTERVAL_MS)) return;

    lastDrawTime = now;
    lastDrawnRA = ra; lastDrawnMHP = mhp;
    gaugeDirty = false;

    // Assets
    const uint16_t* bg = (currentLightingMode == 0) ? radarAltBackground : radarAltBackgroundNVG;
    const uint16_t* nImg = (currentLightingMode == 0) ? radarAltNeedle : radarAltNeedleNVG;
    const uint16_t* ptrImg = (currentLightingMode == 0) ? radarAltMinHeightPointer : radarAltMinHeightPointerNVG;
    const uint16_t* flagImg = (currentLightingMode == 0) ? radarAltOffFlag : radarAltOffFlagNVG;

#if DEBUG_PERFORMANCE
    beginProfiling(PERF_TFT_RADARALT_DRAW);
#endif

    if (lastNeedleMode != currentLightingMode) {
        buildNeedle(needle, nImg);
        lastNeedleMode = currentLightingMode;
    }
    if (lastPointerMode != currentLightingMode) {
        buildPointer(pointerSpr, ptrImg);
        lastPointerMode = currentLightingMode;
    }
    if (lastOffFlagMode != currentLightingMode) {
        buildLamp(offFlagSpr, flagImg, 51, 19);
        lastOffFlagMode = currentLightingMode;
    }

    // Compose full frame into frameSpr
    frameSpr.fillScreen(TFT_BLACK);
    frameSpr.pushImage(0, 0, SCREEN_W, SCREEN_H, bg);   // bg assets are already in correct byte order for the sprite

    // Overlays
    if (greenOn)  greenLampSpr.pushRotateZoom(&frameSpr, GREEN_X, GREEN_Y, 0.0f, 1.0f, 1.0f, TRANSPARENT_KEY);
    if (lowAltOn) lowAltSpr.pushRotateZoom(&frameSpr, LOWALT_X, LOWALT_Y, 0.0f, 1.0f, 1.0f, TRANSPARENT_KEY);
    if (offFlag)  offFlagSpr.pushRotateZoom(&frameSpr, OFF_X, OFF_Y, 0.0f, 1.0f, 1.0f, TRANSPARENT_KEY);

    // Pointers
    pointerSpr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)mhp, 1.0f, 1.0f, TRANSPARENT_KEY);
    needle.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y, (float)ra, 1.0f, 1.0f, TRANSPARENT_KEY);

    // Copy to back buffer, then flush
    uint16_t* buf = dmaFrame[dmaIdx ^ 1];             // use the buffer not used by the previous DMA
    std::memcpy(buf, frameSpr.getBuffer(), FRAME_BYTES);
    flushFrameToDisplay(buf, blocking);
    dmaIdx ^= 1;

#if DEBUG_PERFORMANCE
    endProfiling(PERF_TFT_RADARALT_DRAW);
#endif
}

// --- Task ---
static void RadarAlt_task(void*)
{
    for (;;) {
        RadarAlt_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// --- API ---
void RadarAlt_init()
{
    if (!initPSRAM()) {
        debugPrintln("❌ No PSRAM detected! RadarAlt disabled.");
        while (1) vTaskDelay(1000);
    }

    // PSRAM DMA buffers
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
    tft.setColorDepth(COLOR_DEPTH_RADARALT);
    tft.setRotation(0);
    tft.setSwapBytes(true);    // keep display-endian handling consistent with existing assets
    tft.fillScreen(TFT_BLACK);

    // Compose sprite (PSRAM)
    frameSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false); // sprite stores native order; background data is prepared accordingly
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        debugPrintln("❌ frameSpr alloc failed!");
        while (1) vTaskDelay(1000);
    }

    // Element sprites
    needle.setColorDepth(COLOR_DEPTH_RADARALT);
    needle.createSprite(76, 173);
    needle.setPivot(38, 134);
    buildNeedle(needle, radarAltNeedle);

    pointerSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    pointerSpr.createSprite(23, 180);
    pointerSpr.setPivot(12, 180);
    buildPointer(pointerSpr, radarAltMinHeightPointer);

    lowAltSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    lowAltSpr.createSprite(34, 34);
    lowAltSpr.setPivot(0, 0);
    buildLamp(lowAltSpr, radarAltLowAltWarning, 34, 34);

    greenLampSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    greenLampSpr.createSprite(34, 34);
    greenLampSpr.setPivot(0, 0);
    buildLamp(greenLampSpr, radarAltGreenLamp, 34, 34);

    offFlagSpr.setColorDepth(COLOR_DEPTH_RADARALT);
    offFlagSpr.createSprite(51, 19);
    offFlagSpr.setPivot(0, 0);
    buildLamp(offFlagSpr, radarAltOffFlag, 51, 19);

    // DCS-BIOS bindings
    subscribeToLedChange("RADALT_ALT_PTR", onRadarAltAngle);
    subscribeToLedChange("RADALT_MIN_HEIGHT_PTR", onMinHeightAngle);
    subscribeToLedChange("LOW_ALT_WARN_LT", onLowAltLamp);
    subscribeToLedChange("RADALT_GREEN_LAMP", onGreenLamp);
    subscribeToLedChange("RADALT_OFF_FLAG", onOffFlag);
    subscribeToLedChange(BACKLIGHT_LABEL, onDimmerChange);

    RadarAlt_bitTest();

#if RUN_RADARALT_AS_TASK
    xTaskCreatePinnedToCore(RadarAlt_task, "RadarAltTask", 4096, nullptr, 2, &tftTaskHandle, RA_CPU_CORE);
#endif

    debugPrintln("✅ Radar Altimeter (LovyanGFX, PSRAM double-buffered, DMA-safe) initialized");
}

void RadarAlt_loop()
{
#if !RUN_RADARALT_AS_TASK
    RadarAlt_draw(false, false);
#endif
}

void RadarAlt_notifyMissionStart() { gaugeDirty = true; }

// Visual self-test; use blocking flush to avoid DMA overlap during rapid sweeps
void RadarAlt_bitTest()
{
    int16_t savRA = angleRA, savMHP = angleMHP;
    bool savLow = lowAltOn, savGreen = greenOn, savOff = offFlag;

    const int STEP = 50, DELAY = 2;

    // Sweep up
    for (int i = 0; i <= 320; i += STEP) {
        angleRA = i; angleMHP = i; gaugeDirty = true;
        RadarAlt_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }
    // Sweep down
    for (int i = 320; i >= 0; i -= STEP) {
        angleRA = i; angleMHP = i; gaugeDirty = true;
        RadarAlt_draw(true, true);
        vTaskDelay(pdMS_TO_TICKS(DELAY));
    }

    lowAltOn = true;  greenOn = false; offFlag = false;
    gaugeDirty = true; RadarAlt_draw(true, true); vTaskDelay(pdMS_TO_TICKS(200));

    lowAltOn = false; greenOn = true;  offFlag = false;
    gaugeDirty = true; RadarAlt_draw(true, true); vTaskDelay(pdMS_TO_TICKS(200));

    lowAltOn = false; greenOn = false; offFlag = true;
    gaugeDirty = true; RadarAlt_draw(true, true); vTaskDelay(pdMS_TO_TICKS(200));

    lowAltOn = true;  greenOn = true;  offFlag = true;
    gaugeDirty = true; RadarAlt_draw(true, true); vTaskDelay(pdMS_TO_TICKS(400));

    // Restore
    angleRA = savRA; angleMHP = savMHP;
    lowAltOn = savLow; greenOn = savGreen; offFlag = savOff;
    gaugeDirty = true; RadarAlt_draw(true, true);
}

void RadarAlt_deinit()
{
    waitDMADone();

    needle.deleteSprite();
    pointerSpr.deleteSprite();
    lowAltSpr.deleteSprite();
    greenLampSpr.deleteSprite();
    offFlagSpr.deleteSprite();
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