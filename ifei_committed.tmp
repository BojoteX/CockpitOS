// =============================================================================
// TFT_Display_IFEI.cpp — CockpitOS TFT IFEI (Pixel-Perfect)
// =============================================================================
//
// PANEL_KIND: TFTIFEI
//
// Hardware: Waveshare ESP32-S3-Touch-LCD-7 (800x480, RGB parallel, ST7262)
//           CH422G I2C IO expander (backlight, LCD/touch reset)
//           GT911 capacitive touch (unused for now, but initialized)
//
// Architecture:
//   - LovyanGFX Bus_RGB + Panel_RGB for continuous framebuffer scanning
//   - VLW fonts loaded from PROGMEM (7-segment style, matching real IFEI)
//   - Shared LGFX_Sprite objects per font size — sprite-based rendering
//   - Procedural nozzle gauges with rotation math
//   - Day/NVG color mode switching
//   - SP/CODES overlay TEMP, T/TIME_SET_MODE overlay FUEL
//
// Reference: SCUBA82 OH-IFEI + ashchan hornet-esp32-gauges
//
// =============================================================================

#include "../Globals.h"

#if defined(HAS_TFT_IFEI) && defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1)

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "includes/TFT_Display_IFEI.h"

#if !__has_include(<LovyanGFX.hpp>)
#error "Missing LovyanGFX.hpp -- Please install LovyanGFX library"
#endif

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <Wire.h>
#include <cstring>
#include <cmath>

// VLW fonts (PROGMEM byte arrays)
#include "Assets/Fonts/IFEI_Data_36.h"
#include "Assets/Fonts/IFEI_Data_32.h"
#include "Assets/Fonts/IFEI_Labels_16.h"

// =============================================================================
// PANEL REGISTRATION
// =============================================================================
REGISTER_PANEL(TFTIFEI, TFTIFEIDisplay_notifyMissionStart, nullptr, TFTIFEIDisplay_init, TFTIFEIDisplay_loop, nullptr, 100);

// =============================================================================
// COMPILE-TIME CONFIGURATION
// =============================================================================
static constexpr int16_t  SCREEN_W = 800;
static constexpr int16_t  SCREEN_H = 480;
static constexpr uint32_t REFRESH_INTERVAL_MS = 33;  // ~30 FPS

// Colors (RGB888 for 24-bit LovyanGFX sprites)
static constexpr uint32_t COLOR_DAY   = 0xFFFFFFU;  // White
static constexpr uint32_t COLOR_NIGHT = 0x1CDD2AU;  // Green (NVG)
static constexpr uint32_t COLOR_BG    = 0x000000U;  // Black

// =============================================================================
// CH422G INITIALIZATION
// =============================================================================
static void ch422g_init() {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.beginTransmission(CH422G_MODE_ADDR);
    Wire.write(0x01);
    Wire.endTransmission();
    Wire.beginTransmission(CH422G_OUTPUT_ADDR);
    Wire.write(CH422G_BACKLIGHT_BIT | CH422G_LCD_RST_BIT | CH422G_TOUCH_RST_BIT);
    Wire.endTransmission();
    debugPrintln("  CH422G initialized (backlight ON, resets released)");
}

// =============================================================================
// LGFX DEVICE CLASS — Waveshare ESP32-S3-Touch-LCD-7
// =============================================================================
class LGFX_WS7 : public lgfx::LGFX_Device {
    lgfx::Bus_RGB     _bus_instance;
    lgfx::Panel_RGB   _panel_instance;
    lgfx::Touch_GT911 _touch_instance;

public:
    LGFX_WS7() {
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = SCREEN_W;
            cfg.memory_height = SCREEN_H;
            cfg.panel_width   = SCREEN_W;
            cfg.panel_height  = SCREEN_H;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;
            cfg.pin_d0  = IFEI_TFT_B0;
            cfg.pin_d1  = IFEI_TFT_B1;
            cfg.pin_d2  = IFEI_TFT_B2;
            cfg.pin_d3  = IFEI_TFT_B3;
            cfg.pin_d4  = IFEI_TFT_B4;
            cfg.pin_d5  = IFEI_TFT_G0;
            cfg.pin_d6  = IFEI_TFT_G1;
            cfg.pin_d7  = IFEI_TFT_G2;
            cfg.pin_d8  = IFEI_TFT_G3;
            cfg.pin_d9  = IFEI_TFT_G4;
            cfg.pin_d10 = IFEI_TFT_G5;
            cfg.pin_d11 = IFEI_TFT_R0;
            cfg.pin_d12 = IFEI_TFT_R1;
            cfg.pin_d13 = IFEI_TFT_R2;
            cfg.pin_d14 = IFEI_TFT_R3;
            cfg.pin_d15 = IFEI_TFT_R4;
            cfg.pin_henable = IFEI_TFT_DE;
            cfg.pin_vsync   = IFEI_TFT_VSYNC;
            cfg.pin_hsync   = IFEI_TFT_HSYNC;
            cfg.pin_pclk    = IFEI_TFT_PCLK;
            cfg.freq_write = 16000000;
            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 4;
            cfg.hsync_back_porch  = 8;
            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 4;
            cfg.vsync_back_porch  = 8;
            cfg.pclk_active_neg   = 1;
            cfg.pclk_idle_high    = 0;
            _bus_instance.config(cfg);
        }
        _panel_instance.setBus(&_bus_instance);
        {
            auto tcfg = _touch_instance.config();
            tcfg.i2c_port = 1;
            tcfg.i2c_addr = 0x14;
            tcfg.pin_sda  = IFEI_TOUCH_SDA;
            tcfg.pin_scl  = IFEI_TOUCH_SCL;
            tcfg.pin_int  = IFEI_TOUCH_INT;
            tcfg.pin_rst  = -1;
            tcfg.freq     = 400000;
            tcfg.x_min    = 0;
            tcfg.x_max    = SCREEN_W - 1;
            tcfg.y_min    = 0;
            tcfg.y_max    = SCREEN_H - 1;
            _touch_instance.config(tcfg);
            _panel_instance.setTouch(&_touch_instance);
        }
        setPanel(&_panel_instance);
    }
};

// =============================================================================
// DISPLAY STATE
// =============================================================================
static LGFX_WS7 tft;

// Shared sprites (reused across elements to minimize PSRAM)
static LGFX_Sprite twoDigitSprite(&tft);    // 76x38 — RPM L/R, OIL L/R
static LGFX_Sprite threeDigitSprite(&tft);  // 108x38 — TEMP L/R, FF L/R
static LGFX_Sprite fuelSprite(&tft);        // 176x38 — FUEL UP/DOWN, BINGO
static LGFX_Sprite clockSprite(&tft);       // 176x35 — CLOCK UP/DOWN
static LGFX_Sprite labelSprite(&tft);       // 65x18 — all text labels
static LGFX_Sprite tagSprite(&tft);         // 18x18 — ZULU, L, R

// Nozzle compositing sprites
static LGFX_Sprite nozCompositeL(&tft);     // 150x154 — left nozzle buffer
static LGFX_Sprite nozCompositeR(&tft);     // 150x154 — right nozzle buffer
static LGFX_Sprite nozzleBand(&tft);        // 308x154 — combined nozzle area

// Current text color
static volatile uint32_t ifeiColor = COLOR_DAY;

// Dirty flags
static volatile bool displayDirty = true;
static volatile bool needsFullRedraw = true;

// Timing
static unsigned long lastDrawTime = 0;

// =============================================================================
// DISPLAY ELEMENT STRUCTURE
// =============================================================================
enum TextAlignment : uint8_t { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT };

struct DisplayElement {
    int16_t spriteWidth;
    int16_t spriteHeight;
    int16_t posX;
    int16_t posY;
    TextAlignment align;
    LGFX_Sprite* sprite;
    char value[8];
};

enum DisplayName : uint8_t {
    RPML, RPMR, RPMT,
    TMPL, TMPR, TMPT,
    FFL, FFR, FFTU, FFTL,
    OILL, OILR, OILT,
    NOZL, NOZR, NOZT,
    FUELU, FUELL, BINGO_VAL, BINGOT,
    CLOCKU, CLOCKL,
    ZULU, L_TAG, R_TAG,
    NUM_ELEMENTS
};

// Element layout — pixel-perfect positions from ashchan reference (offsets = 0)
static DisplayElement elems[NUM_ELEMENTS] = {
    // RPM
    { 76,  38,  76,  42,  ALIGN_RIGHT,  &twoDigitSprite,   "" },  // RPML
    { 76,  38,  242, 42,  ALIGN_RIGHT,  &twoDigitSprite,   "" },  // RPMR
    { 58,  18,  170, 54,  ALIGN_CENTER, &labelSprite,      "" },  // RPMT
    // TEMP
    { 108, 38,  48,  112, ALIGN_RIGHT,  &threeDigitSprite, "" },  // TMPL
    { 108, 38,  242, 112, ALIGN_RIGHT,  &threeDigitSprite, "" },  // TMPR
    { 58,  18,  172, 120, ALIGN_CENTER, &labelSprite,      "" },  // TMPT
    // FF
    { 108, 38,  44,  178, ALIGN_RIGHT,  &threeDigitSprite, "" },  // FFL
    { 108, 38,  242, 178, ALIGN_RIGHT,  &threeDigitSprite, "" },  // FFR
    { 58,  18,  172, 186, ALIGN_CENTER, &labelSprite,      "" },  // FFTU
    { 65,  18,  168, 203, ALIGN_CENTER, &labelSprite,      "" },  // FFTL
    // OIL
    { 76,  38,  80,  439, ALIGN_RIGHT,  &twoDigitSprite,   "" },  // OILL
    { 76,  38,  248, 439, ALIGN_RIGHT,  &twoDigitSprite,   "" },  // OILR
    { 58,  18,  176, 450, ALIGN_CENTER, &labelSprite,      "" },  // OILT
    // NOZZLE (gauge positions — sprite not used for standard updateElement)
    { 150, 154, 46,  252, ALIGN_LEFT,   &nozCompositeL,    "" },  // NOZL
    { 150, 154, 204, 252, ALIGN_LEFT,   &nozCompositeR,    "" },  // NOZR
    { 58,  18,  172, 320, ALIGN_CENTER, &labelSprite,      "" },  // NOZT
    // FUEL
    { 176, 38,  566, 56,  ALIGN_RIGHT,  &fuelSprite,       "" },  // FUELU
    { 176, 38,  566, 132, ALIGN_RIGHT,  &fuelSprite,       "" },  // FUELL
    { 176, 38,  566, 246, ALIGN_RIGHT,  &fuelSprite,       "" },  // BINGO_VAL
    { 58,  18,  648, 215, ALIGN_CENTER, &labelSprite,      "" },  // BINGOT
    // CLOCK
    { 176, 35,  575, 378, ALIGN_RIGHT,  &clockSprite,      "" },  // CLOCKU
    { 176, 35,  575, 440, ALIGN_RIGHT,  &clockSprite,      "" },  // CLOCKL
    // TAGS
    { 18,  18,  750, 396, ALIGN_CENTER, &tagSprite,        "" },  // ZULU
    { 18,  18,  740, 70,  ALIGN_CENTER, &tagSprite,        "" },  // L_TAG
    { 18,  18,  740, 146, ALIGN_CENTER, &tagSprite,        "" },  // R_TAG
};

// =============================================================================
// NOZZLE GAUGE STATE
// =============================================================================
static constexpr float NOZL_PIVOT_X      = 6.2f;
static constexpr float NOZL_PIVOT_Y      = 8.4f;
static constexpr float NOZR_PIVOT_X      = 143.1f;
static constexpr float NOZR_PIVOT_Y      = 8.6f;
static constexpr float NOZ_DEG_PER_PCT_L = 0.897f;
static constexpr float NOZ_DEG_PER_PCT_R = 0.892f;
static constexpr int16_t NOZ_BAND_X      = 46;
static constexpr int16_t NOZ_BAND_Y      = 252;
static constexpr int16_t NOZ_BAND_W      = 308;
static constexpr int16_t NOZ_BAND_H      = 154;

// Nozzle needle length and width
static constexpr float NEEDLE_LEN = 130.0f;
static constexpr float NEEDLE_W   = 2.0f;

// Scale arc radius
static constexpr float SCALE_RADIUS = 135.0f;

// Nozzle position values (0-65535 from DCS-BIOS)
static volatile uint16_t nozzlePosL = 0;
static volatile uint16_t nozzlePosR = 0;
static volatile bool nozzleDirty = true;

// Nozzle texture visibility flags
static bool tex_lpointer = false;
static bool tex_rpointer = false;
static bool tex_lscale   = false;
static bool tex_rscale   = false;
static bool tex_l0       = false;
static bool tex_l50      = false;
static bool tex_l100     = false;
static bool tex_r0       = false;
static bool tex_r50      = false;
static bool tex_r100     = false;

// =============================================================================
// FIELD STORAGE AND DIRTY FLAGS
// =============================================================================
// Engine data
static char fld_rpm_l[4]  = "";  static bool dirty_rpm_l  = true;
static char fld_rpm_r[4]  = "";  static bool dirty_rpm_r  = true;
static char fld_temp_l[4] = "";  static bool dirty_temp_l = true;
static char fld_temp_r[4] = "";  static bool dirty_temp_r = true;
static char fld_ff_l[4]   = "";  static bool dirty_ff_l   = true;
static char fld_ff_r[4]   = "";  static bool dirty_ff_r   = true;
static char fld_oil_l[4]  = "";  static bool dirty_oil_l  = true;
static char fld_oil_r[4]  = "";  static bool dirty_oil_r  = true;

// Fuel
static char fld_fuel_up[7]   = "";  static bool dirty_fuel_up   = true;
static char fld_fuel_down[7] = "";  static bool dirty_fuel_down = true;
static char fld_bingo[6]     = "";  static bool dirty_bingo     = true;

// SP/Codes overlay
static char fld_sp[4]    = "";  static bool dirty_sp    = true;
static char fld_codes[4] = "";  static bool dirty_codes = true;

// T mode / Time set overlay
static char fld_t[7]        = "";  static bool dirty_t        = true;
static char fld_time_set[7] = "";  static bool dirty_time_set = true;

// Clock (upper)
static char fld_clock_h[3] = "";  static bool dirty_clock = true;
static char fld_clock_m[3] = "";
static char fld_clock_s[3] = "";
static char fld_dd_1[2]    = "";
static char fld_dd_2[2]    = "";

// Timer (lower)
static char fld_timer_h[3] = "";  static bool dirty_timer = true;
static char fld_timer_m[3] = "";
static char fld_timer_s[3] = "";
static char fld_dd_3[2]    = "";
static char fld_dd_4[2]    = "";

// Texture visibility flags (label text on/off)
static bool tex_rpm   = false;  static bool dirty_tex_rpm   = true;
static bool tex_temp  = false;  static bool dirty_tex_temp  = true;
static bool tex_ff    = false;  static bool dirty_tex_ff    = true;
static bool tex_oil   = false;  static bool dirty_tex_oil   = true;
static bool tex_noz   = false;  static bool dirty_tex_noz   = true;
static bool tex_bingo = false;  static bool dirty_tex_bingo = true;
static bool tex_z     = false;  static bool dirty_tex_z     = true;
static bool tex_l     = false;  static bool dirty_tex_l     = true;
static bool tex_r     = false;  static bool dirty_tex_r     = true;

// =============================================================================
// HELPERS
// =============================================================================
static void copyTrimmed(const char* src, char* dest, size_t destSize) {
    if (!src || !dest || destSize == 0) return;
    while (*src && *src == ' ') src++;
    const char* end = src + strlen(src);
    while (end > src && *(end - 1) == ' ') end--;
    size_t n = (size_t)(end - src);
    if (n >= destSize) n = destSize - 1;
    memcpy(dest, src, n);
    dest[n] = '\0';
}

static bool isBlankString(const char* s, size_t maxLen) {
    if (!s) return true;
    for (size_t i = 0; i < maxLen && s[i] != '\0'; i++) {
        if (s[i] != ' ') return false;
    }
    return true;
}

// Calculate cursor X for text alignment within a sprite
static int16_t alignCursorX(LGFX_Sprite* spr, const char* text, int16_t sprWidth, TextAlignment align) {
    if (align == ALIGN_RIGHT) {
        return sprWidth - spr->textWidth(text);
    } else if (align == ALIGN_CENTER) {
        return (sprWidth - spr->textWidth(text)) / 2;
    }
    return 0;
}

// =============================================================================
// DCS-BIOS CALLBACKS — Display fields
// =============================================================================
static void onRpmL(const char*, const char* v)  { copyTrimmed(v, fld_rpm_l, sizeof(fld_rpm_l));  dirty_rpm_l = true; displayDirty = true; }
static void onRpmR(const char*, const char* v)  { copyTrimmed(v, fld_rpm_r, sizeof(fld_rpm_r));  dirty_rpm_r = true; displayDirty = true; }
static void onTempL(const char*, const char* v) { copyTrimmed(v, fld_temp_l, sizeof(fld_temp_l)); dirty_temp_l = true; displayDirty = true; }
static void onTempR(const char*, const char* v) { copyTrimmed(v, fld_temp_r, sizeof(fld_temp_r)); dirty_temp_r = true; displayDirty = true; }
static void onFfL(const char*, const char* v)   { copyTrimmed(v, fld_ff_l, sizeof(fld_ff_l));   dirty_ff_l = true; displayDirty = true; }
static void onFfR(const char*, const char* v)   { copyTrimmed(v, fld_ff_r, sizeof(fld_ff_r));   dirty_ff_r = true; displayDirty = true; }
static void onOilL(const char*, const char* v)  { copyTrimmed(v, fld_oil_l, sizeof(fld_oil_l)); dirty_oil_l = true; displayDirty = true; }
static void onOilR(const char*, const char* v)  { copyTrimmed(v, fld_oil_r, sizeof(fld_oil_r)); dirty_oil_r = true; displayDirty = true; }

static void onFuelUp(const char*, const char* v)   { copyTrimmed(v, fld_fuel_up, sizeof(fld_fuel_up));     dirty_fuel_up = true; displayDirty = true; }
static void onFuelDown(const char*, const char* v)  { copyTrimmed(v, fld_fuel_down, sizeof(fld_fuel_down)); dirty_fuel_down = true; displayDirty = true; }
static void onBingo(const char*, const char* v)     { copyTrimmed(v, fld_bingo, sizeof(fld_bingo));         dirty_bingo = true; displayDirty = true; }

static void onSp(const char*, const char* v)    { copyTrimmed(v, fld_sp, sizeof(fld_sp));       dirty_sp = true; dirty_temp_l = true; displayDirty = true; }
static void onCodes(const char*, const char* v)  { copyTrimmed(v, fld_codes, sizeof(fld_codes)); dirty_codes = true; dirty_temp_r = true; displayDirty = true; }
static void onT(const char*, const char* v)      { copyTrimmed(v, fld_t, sizeof(fld_t));         dirty_t = true; dirty_fuel_up = true; displayDirty = true; }
static void onTimeSet(const char*, const char* v){ copyTrimmed(v, fld_time_set, sizeof(fld_time_set)); dirty_time_set = true; dirty_fuel_down = true; displayDirty = true; }

// Clock
static void onClockH(const char*, const char* v) { copyTrimmed(v, fld_clock_h, sizeof(fld_clock_h)); dirty_clock = true; displayDirty = true; }
static void onClockM(const char*, const char* v) { copyTrimmed(v, fld_clock_m, sizeof(fld_clock_m)); dirty_clock = true; displayDirty = true; }
static void onClockS(const char*, const char* v) { copyTrimmed(v, fld_clock_s, sizeof(fld_clock_s)); dirty_clock = true; displayDirty = true; }
static void onDd1(const char*, const char* v)    { if (v) { fld_dd_1[0] = v[0]; fld_dd_1[1] = '\0'; } dirty_clock = true; displayDirty = true; }
static void onDd2(const char*, const char* v)    { if (v) { fld_dd_2[0] = v[0]; fld_dd_2[1] = '\0'; } dirty_clock = true; displayDirty = true; }

// Timer
static void onTimerH(const char*, const char* v) { copyTrimmed(v, fld_timer_h, sizeof(fld_timer_h)); dirty_timer = true; displayDirty = true; }
static void onTimerM(const char*, const char* v) { copyTrimmed(v, fld_timer_m, sizeof(fld_timer_m)); dirty_timer = true; displayDirty = true; }
static void onTimerS(const char*, const char* v) { copyTrimmed(v, fld_timer_s, sizeof(fld_timer_s)); dirty_timer = true; displayDirty = true; }
static void onDd3(const char*, const char* v)    { if (v) { fld_dd_3[0] = v[0]; fld_dd_3[1] = '\0'; } dirty_timer = true; displayDirty = true; }
static void onDd4(const char*, const char* v)    { if (v) { fld_dd_4[0] = v[0]; fld_dd_4[1] = '\0'; } dirty_timer = true; displayDirty = true; }

// =============================================================================
// DCS-BIOS CALLBACKS — Texture visibility (labels on/off)
// =============================================================================
static void onTexRpm(const char*, const char* v)   { tex_rpm   = (v && v[0] == '1'); dirty_tex_rpm   = true; displayDirty = true; }
static void onTexTemp(const char*, const char* v)  { tex_temp  = (v && v[0] == '1'); dirty_tex_temp  = true; displayDirty = true; }
static void onTexFf(const char*, const char* v)    { tex_ff    = (v && v[0] == '1'); dirty_tex_ff    = true; displayDirty = true; }
static void onTexOil(const char*, const char* v)   { tex_oil   = (v && v[0] == '1'); dirty_tex_oil   = true; displayDirty = true; }
static void onTexNoz(const char*, const char* v)   { tex_noz   = (v && v[0] == '1'); dirty_tex_noz   = true; nozzleDirty = true; displayDirty = true; }
static void onTexBingo(const char*, const char* v) { tex_bingo = (v && v[0] == '1'); dirty_tex_bingo = true; displayDirty = true; }
static void onTexZ(const char*, const char* v)     { tex_z     = (v && v[0] == '1'); dirty_tex_z     = true; displayDirty = true; }
static void onTexL(const char*, const char* v)     { tex_l     = (v && v[0] == '1'); dirty_tex_l     = true; displayDirty = true; }
static void onTexR(const char*, const char* v)     { tex_r     = (v && v[0] == '1'); dirty_tex_r     = true; displayDirty = true; }

// =============================================================================
// DCS-BIOS CALLBACKS — Nozzle textures
// =============================================================================
static void onTexLPointer(const char*, const char* v) { tex_lpointer = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexRPointer(const char*, const char* v) { tex_rpointer = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexLScale(const char*, const char* v)   { tex_lscale   = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexRScale(const char*, const char* v)   { tex_rscale   = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexL0(const char*, const char* v)       { tex_l0       = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexL50(const char*, const char* v)      { tex_l50      = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexL100(const char*, const char* v)     { tex_l100     = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexR0(const char*, const char* v)       { tex_r0       = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexR50(const char*, const char* v)      { tex_r50      = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }
static void onTexR100(const char*, const char* v)     { tex_r100     = (v && v[0] == '1'); nozzleDirty = true; displayDirty = true; }

// =============================================================================
// DCS-BIOS CALLBACKS — Nozzle position (metadata)
// =============================================================================
static void onNozzlePosL(const char*, uint16_t value) { nozzlePosL = value; nozzleDirty = true; displayDirty = true; }
static void onNozzlePosR(const char*, uint16_t value) { nozzlePosR = value; nozzleDirty = true; displayDirty = true; }

// =============================================================================
// DCS-BIOS CALLBACKS — Color mode and brightness
// =============================================================================
static void onLightModeChange(const char*, uint16_t value) {
    uint32_t newColor = (value == 0) ? COLOR_DAY : COLOR_NIGHT;
    if (newColor != ifeiColor) {
        ifeiColor = newColor;
        needsFullRedraw = true;
        displayDirty = true;
    }
}

static void onBrightnessChange(const char*, uint16_t value, uint16_t) {
    // Brightness via setBrightness() — works if GPIO 16 hardware mod is present
    // Day mode: fixed brightness; Night mode: proportional
    if (ifeiColor == COLOR_DAY) {
        tft.setBrightness(150);
    } else {
        uint8_t brightness = (uint8_t)map(value, 0, 65535, 0, 255);
        tft.setBrightness(brightness);
    }
}

// =============================================================================
// DRAWING: updateElement — generic sprite-based element redraw
// =============================================================================
static void updateElement(DisplayName idx) {
    DisplayElement& el = elems[idx];
    LGFX_Sprite* spr = el.sprite;

    // Resize sprite if needed (shared sprites may have different dimensions)
    if (spr->width() != el.spriteWidth || spr->height() != el.spriteHeight) {
        spr->deleteSprite();
        spr->setPsram(true);
        spr->setColorDepth(24);
        spr->createSprite(el.spriteWidth, el.spriteHeight);
    }

    spr->fillScreen(COLOR_BG);
    spr->setTextColor(ifeiColor);
    int16_t cx = alignCursorX(spr, el.value, el.spriteWidth, el.align);
    spr->setCursor(cx, 0);
    spr->print(el.value);
    spr->pushSprite(el.posX, el.posY);
}

// =============================================================================
// DRAWING: renderClock — clock/timer with special separator formatting
// =============================================================================
static void renderClock(DisplayName idx, const char* h, const char* d1,
                         const char* m, const char* d2, const char* s,
                         bool isTimer) {
    DisplayElement& el = elems[idx];
    LGFX_Sprite* spr = el.sprite;

    if (spr->width() != el.spriteWidth || spr->height() != el.spriteHeight) {
        spr->deleteSprite();
        spr->setPsram(true);
        spr->setColorDepth(24);
        spr->createSprite(el.spriteWidth, el.spriteHeight);
    }

    spr->fillScreen(COLOR_BG);
    spr->setTextColor(ifeiColor);

    // Build formatted clock string: "HH:MM:SS"
    // For clock (isTimer=false): pad single-digit hours with leading '0'
    // For timer (isTimer=true): no leading zero on hours
    char buf[16] = "";
    char hBuf[4] = "";
    char mBuf[4] = "";
    char sBuf[4] = "";

    // Hours
    if (h[0] == '\0' || h[0] == ' ') {
        hBuf[0] = ' ';
        hBuf[1] = '\0';
    } else {
        int hVal = atoi(h);
        if (!isTimer && hVal >= 0 && hVal < 10) {
            snprintf(hBuf, sizeof(hBuf), "0%d", hVal);
        } else {
            strncpy(hBuf, h, sizeof(hBuf) - 1);
            hBuf[sizeof(hBuf) - 1] = '\0';
        }
    }

    // Minutes
    if (m[0] == '\0' || m[0] == ' ') {
        mBuf[0] = ' ';
        mBuf[1] = '\0';
    } else {
        int mVal = atoi(m);
        if (mVal >= 0 && mVal < 10) {
            snprintf(mBuf, sizeof(mBuf), "0%d", mVal);
        } else {
            strncpy(mBuf, m, sizeof(mBuf) - 1);
            mBuf[sizeof(mBuf) - 1] = '\0';
        }
    }

    // Seconds
    if (s[0] == '\0' || s[0] == ' ') {
        sBuf[0] = ' ';
        sBuf[1] = '\0';
    } else {
        int sVal = atoi(s);
        if (sVal >= 0 && sVal < 10) {
            snprintf(sBuf, sizeof(sBuf), "0%d", sVal);
        } else {
            strncpy(sBuf, s, sizeof(sBuf) - 1);
            sBuf[sizeof(sBuf) - 1] = '\0';
        }
    }

    // Render each segment at hardcoded cursor positions (from ashchan reference)
    // These cursor X values give proper fixed-width spacing for the clock font
    if (hBuf[0] != ' ') {
        // If hours is "0" (single digit timer), offset to align with colon
        int16_t hx = (strlen(hBuf) == 1) ? 29 : 1;
        spr->setCursor(hx, 1);
        spr->print(hBuf);
    } else {
        spr->setCursor(57, 1);  // Skip past hours area
    }

    // Separator 1
    if (d1[0] != '\0' && d1[0] != ' ') {
        spr->setCursor(63, 1);
        spr->print(d1);
    } else {
        spr->setCursor(63, 1);  // Advance cursor
    }

    // Minutes
    if (mBuf[0] != ' ') {
        spr->setCursor(spr->getCursorX(), 1);
        spr->print(mBuf);
    } else {
        spr->setCursor(119, 1);
    }

    // Separator 2
    if (d2[0] != '\0' && d2[0] != ' ') {
        spr->setCursor(125, 1);
        spr->print(d2);
    } else {
        spr->setCursor(125, 1);
    }

    // Seconds
    if (sBuf[0] != ' ') {
        spr->setCursor(spr->getCursorX(), 1);
        spr->print(sBuf);
    }

    spr->pushSprite(el.posX, el.posY);
}

// =============================================================================
// DRAWING: Procedural nozzle gauge
// =============================================================================
static void drawNozzleGauge(LGFX_Sprite* composite, float pivotX, float pivotY,
                             float degPerPct, bool invert,
                             uint16_t rawPos,
                             bool showPointer, bool showScale,
                             bool show0, bool show50, bool show100) {
    composite->fillScreen(COLOR_BG);

    float pct = (rawPos / 65535.0f) * 100.0f;
    float angleDeg = pct * degPerPct;
    if (invert) angleDeg = -angleDeg;

    // Convert to radians for trig
    // The gauge sweeps from straight down (0%) rotating CW (left) or CCW (right)
    // Base angle: -90 degrees (pointing down from pivot)
    float baseAngleRad = -M_PI / 2.0f;

    // Scale arc: draw from 0% to 100% as tick marks
    if (showScale) {
        for (int i = 0; i <= 100; i += 2) {
            float a = (float)i * degPerPct;
            if (invert) a = -a;
            float rad = baseAngleRad + (a * M_PI / 180.0f);

            float innerR = SCALE_RADIUS - 5.0f;
            float outerR = SCALE_RADIUS;
            // Longer ticks at 0, 10, 20... positions
            if (i % 10 == 0) innerR = SCALE_RADIUS - 10.0f;

            float x1 = pivotX + innerR * cosf(rad);
            float y1 = pivotY - innerR * sinf(rad);
            float x2 = pivotX + outerR * cosf(rad);
            float y2 = pivotY - outerR * sinf(rad);
            composite->drawLine((int16_t)x1, (int16_t)y1, (int16_t)x2, (int16_t)y2, ifeiColor);
        }
    }

    // Number labels at 0, 50, 100 positions
    composite->setTextColor(ifeiColor);
    composite->setTextSize(1);
    // Use the label font for numbers
    composite->loadFont(IFEI_Labels_16);

    auto drawNumber = [&](int val, bool visible) {
        if (!visible) return;
        float a = (float)val * degPerPct;
        if (invert) a = -a;
        float rad = baseAngleRad + (a * M_PI / 180.0f);
        float labelR = SCALE_RADIUS + 10.0f;
        float x = pivotX + labelR * cosf(rad);
        float y = pivotY - labelR * sinf(rad);
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", val);
        int16_t tw = composite->textWidth(buf);
        int16_t th = composite->fontHeight();
        composite->setCursor((int16_t)(x - tw / 2), (int16_t)(y - th / 2));
        composite->print(buf);
    };

    drawNumber(0, show0);
    drawNumber(50, show50);
    drawNumber(100, show100);

    composite->unloadFont();

    // Needle/pointer
    if (showPointer) {
        float needleRad = baseAngleRad + (angleDeg * M_PI / 180.0f);
        float x2 = pivotX + NEEDLE_LEN * cosf(needleRad);
        float y2 = pivotY - NEEDLE_LEN * sinf(needleRad);

        // Draw thick needle (3px)
        for (int d = -1; d <= 1; d++) {
            composite->drawLine(
                (int16_t)pivotX + d, (int16_t)pivotY,
                (int16_t)x2 + d, (int16_t)y2, ifeiColor
            );
            composite->drawLine(
                (int16_t)pivotX, (int16_t)pivotY + d,
                (int16_t)x2, (int16_t)y2 + d, ifeiColor
            );
        }
    }
}

static void renderNozzles() {
    // Draw left nozzle gauge
    drawNozzleGauge(&nozCompositeL, NOZL_PIVOT_X, NOZL_PIVOT_Y,
                     NOZ_DEG_PER_PCT_L, false, nozzlePosL,
                     tex_lpointer, tex_lscale, tex_l0, tex_l50, tex_l100);

    // Draw right nozzle gauge
    drawNozzleGauge(&nozCompositeR, NOZR_PIVOT_X, NOZR_PIVOT_Y,
                     NOZ_DEG_PER_PCT_R, true, nozzlePosR,
                     tex_rpointer, tex_rscale, tex_r0, tex_r50, tex_r100);

    // Composite into nozzle band
    nozzleBand.fillScreen(COLOR_BG);
    nozCompositeL.pushSprite(&nozzleBand, 0, 0);
    nozCompositeR.pushSprite(&nozzleBand, 158, 0);

    // Draw NOZ label into nozzle band at relative position
    if (tex_noz) {
        int16_t nozRelX = elems[NOZT].posX - NOZ_BAND_X;
        int16_t nozRelY = elems[NOZT].posY - NOZ_BAND_Y;

        labelSprite.fillScreen(COLOR_BG);
        labelSprite.setTextColor(ifeiColor);
        int16_t cx = alignCursorX(&labelSprite, "NOZ", elems[NOZT].spriteWidth, ALIGN_CENTER);
        labelSprite.setCursor(cx, 0);
        labelSprite.print("NOZ");
        labelSprite.pushSprite(&nozzleBand, nozRelX, nozRelY, COLOR_BG);
    }

    // Push entire nozzle band to TFT in one operation
    nozzleBand.pushSprite(NOZ_BAND_X, NOZ_BAND_Y);
}

// =============================================================================
// DRAWING: Full screen redraw
// =============================================================================
static void drawFullScreen() {
    tft.fillScreen(COLOR_BG);

    // RPM
    strcpy(elems[RPML].value, fld_rpm_l);   updateElement(RPML);
    strcpy(elems[RPMR].value, fld_rpm_r);   updateElement(RPMR);
    strcpy(elems[RPMT].value, tex_rpm ? "RPM" : "");  updateElement(RPMT);

    // TEMP / SP+CODES overlay
    {
        const char* left  = (!isBlankString(fld_sp, 3))    ? fld_sp    : fld_temp_l;
        const char* right = (!isBlankString(fld_codes, 3)) ? fld_codes : fld_temp_r;
        strcpy(elems[TMPL].value, left);    updateElement(TMPL);
        strcpy(elems[TMPR].value, right);   updateElement(TMPR);
        strcpy(elems[TMPT].value, tex_temp ? "TEMP" : "");  updateElement(TMPT);
    }

    // FF
    strcpy(elems[FFL].value, fld_ff_l);    updateElement(FFL);
    strcpy(elems[FFR].value, fld_ff_r);    updateElement(FFR);
    strcpy(elems[FFTU].value, tex_ff ? "FF" : "");     updateElement(FFTU);
    strcpy(elems[FFTL].value, tex_ff ? "X100" : "");   updateElement(FFTL);

    // OIL
    strcpy(elems[OILL].value, fld_oil_l);  updateElement(OILL);
    strcpy(elems[OILR].value, fld_oil_r);  updateElement(OILR);
    strcpy(elems[OILT].value, tex_oil ? "OIL" : "");   updateElement(OILT);

    // Nozzle gauges
    renderNozzles();

    // FUEL UP / T-mode overlay
    {
        const char* fuelUpText = (!isBlankString(fld_t, 6)) ? fld_t : fld_fuel_up;
        strcpy(elems[FUELU].value, fuelUpText);  updateElement(FUELU);
    }

    // FUEL DOWN / TIME_SET_MODE overlay
    {
        const char* fuelDnText = (!isBlankString(fld_time_set, 6)) ? fld_time_set : fld_fuel_down;
        strcpy(elems[FUELL].value, fuelDnText);  updateElement(FUELL);
    }

    // BINGO
    strcpy(elems[BINGOT].value, tex_bingo ? "BINGO" : "");  updateElement(BINGOT);
    strcpy(elems[BINGO_VAL].value, fld_bingo);  updateElement(BINGO_VAL);

    // CLOCK
    renderClock(CLOCKU, fld_clock_h, fld_dd_1, fld_clock_m, fld_dd_2, fld_clock_s, false);

    // TIMER
    renderClock(CLOCKL, fld_timer_h, fld_dd_3, fld_timer_m, fld_dd_4, fld_timer_s, true);

    // Tags
    strcpy(elems[ZULU].value,  tex_z ? "Z" : "");  updateElement(ZULU);
    strcpy(elems[L_TAG].value, tex_l ? "L" : "");   updateElement(L_TAG);
    strcpy(elems[R_TAG].value, tex_r ? "R" : "");    updateElement(R_TAG);
}

// =============================================================================
// DRAWING: Incremental redraw (dirty fields only)
// =============================================================================
static void drawDirtyFields() {
    // RPM data
    if (dirty_rpm_l) { strcpy(elems[RPML].value, fld_rpm_l); updateElement(RPML); dirty_rpm_l = false; }
    if (dirty_rpm_r) { strcpy(elems[RPMR].value, fld_rpm_r); updateElement(RPMR); dirty_rpm_r = false; }

    // TEMP / SP+CODES overlay
    if (dirty_temp_l || dirty_sp) {
        const char* left = (!isBlankString(fld_sp, 3)) ? fld_sp : fld_temp_l;
        strcpy(elems[TMPL].value, left); updateElement(TMPL);
        dirty_temp_l = false; dirty_sp = false;
    }
    if (dirty_temp_r || dirty_codes) {
        const char* right = (!isBlankString(fld_codes, 3)) ? fld_codes : fld_temp_r;
        strcpy(elems[TMPR].value, right); updateElement(TMPR);
        dirty_temp_r = false; dirty_codes = false;
    }

    // FF data
    if (dirty_ff_l) { strcpy(elems[FFL].value, fld_ff_l); updateElement(FFL); dirty_ff_l = false; }
    if (dirty_ff_r) { strcpy(elems[FFR].value, fld_ff_r); updateElement(FFR); dirty_ff_r = false; }

    // OIL data
    if (dirty_oil_l) { strcpy(elems[OILL].value, fld_oil_l); updateElement(OILL); dirty_oil_l = false; }
    if (dirty_oil_r) { strcpy(elems[OILR].value, fld_oil_r); updateElement(OILR); dirty_oil_r = false; }

    // FUEL UP / T-mode
    if (dirty_fuel_up || dirty_t) {
        const char* fuelUpText = (!isBlankString(fld_t, 6)) ? fld_t : fld_fuel_up;
        strcpy(elems[FUELU].value, fuelUpText); updateElement(FUELU);
        dirty_fuel_up = false; dirty_t = false;
    }

    // FUEL DOWN / TIME_SET_MODE
    if (dirty_fuel_down || dirty_time_set) {
        const char* fuelDnText = (!isBlankString(fld_time_set, 6)) ? fld_time_set : fld_fuel_down;
        strcpy(elems[FUELL].value, fuelDnText); updateElement(FUELL);
        dirty_fuel_down = false; dirty_time_set = false;
    }

    // BINGO
    if (dirty_bingo) { strcpy(elems[BINGO_VAL].value, fld_bingo); updateElement(BINGO_VAL); dirty_bingo = false; }

    // CLOCK
    if (dirty_clock) {
        renderClock(CLOCKU, fld_clock_h, fld_dd_1, fld_clock_m, fld_dd_2, fld_clock_s, false);
        dirty_clock = false;
    }

    // TIMER
    if (dirty_timer) {
        renderClock(CLOCKL, fld_timer_h, fld_dd_3, fld_timer_m, fld_dd_4, fld_timer_s, true);
        dirty_timer = false;
    }

    // Nozzle gauges
    if (nozzleDirty) {
        renderNozzles();
        nozzleDirty = false;
    }

    // Label textures (only redraw when visibility changed)
    if (dirty_tex_rpm) {
        strcpy(elems[RPMT].value, tex_rpm ? "RPM" : ""); updateElement(RPMT);
        dirty_tex_rpm = false;
    }
    if (dirty_tex_temp) {
        strcpy(elems[TMPT].value, tex_temp ? "TEMP" : ""); updateElement(TMPT);
        dirty_tex_temp = false;
    }
    if (dirty_tex_ff) {
        strcpy(elems[FFTU].value, tex_ff ? "FF" : "");   updateElement(FFTU);
        strcpy(elems[FFTL].value, tex_ff ? "X100" : ""); updateElement(FFTL);
        dirty_tex_ff = false;
    }
    if (dirty_tex_oil) {
        strcpy(elems[OILT].value, tex_oil ? "OIL" : ""); updateElement(OILT);
        dirty_tex_oil = false;
    }
    if (dirty_tex_bingo) {
        strcpy(elems[BINGOT].value, tex_bingo ? "BINGO" : ""); updateElement(BINGOT);
        dirty_tex_bingo = false;
    }
    if (dirty_tex_z) {
        strcpy(elems[ZULU].value, tex_z ? "Z" : ""); updateElement(ZULU);
        dirty_tex_z = false;
    }
    if (dirty_tex_l) {
        strcpy(elems[L_TAG].value, tex_l ? "L" : ""); updateElement(L_TAG);
        dirty_tex_l = false;
    }
    if (dirty_tex_r) {
        strcpy(elems[R_TAG].value, tex_r ? "R" : ""); updateElement(R_TAG);
        dirty_tex_r = false;
    }
}

// =============================================================================
// SPRITE CREATION
// =============================================================================
static void createSprites() {
    // Two-digit data sprite (RPM L/R, OIL L/R)
    twoDigitSprite.setPsram(true);
    twoDigitSprite.setColorDepth(24);
    twoDigitSprite.createSprite(76, 38);
    twoDigitSprite.loadFont(IFEI_Data_36);
    twoDigitSprite.setTextWrap(false);
    twoDigitSprite.setTextColor(ifeiColor);

    // Three-digit data sprite (TEMP L/R, FF L/R)
    threeDigitSprite.setPsram(true);
    threeDigitSprite.setColorDepth(24);
    threeDigitSprite.createSprite(108, 38);
    threeDigitSprite.loadFont(IFEI_Data_36);
    threeDigitSprite.setTextWrap(false);
    threeDigitSprite.setTextColor(ifeiColor);

    // Fuel sprite (FUEL UP/DOWN, BINGO)
    fuelSprite.setPsram(true);
    fuelSprite.setColorDepth(24);
    fuelSprite.createSprite(176, 38);
    fuelSprite.loadFont(IFEI_Data_36);
    fuelSprite.setTextWrap(false);
    fuelSprite.setTextColor(ifeiColor);

    // Clock sprite (CLOCK UP/DOWN)
    clockSprite.setPsram(true);
    clockSprite.setColorDepth(24);
    clockSprite.createSprite(176, 35);
    clockSprite.loadFont(IFEI_Data_32);
    clockSprite.setTextWrap(false);
    clockSprite.setTextColor(ifeiColor);

    // Label sprite (all text labels)
    labelSprite.setPsram(true);
    labelSprite.setColorDepth(24);
    labelSprite.createSprite(65, 18);
    labelSprite.loadFont(IFEI_Labels_16);
    labelSprite.setTextColor(ifeiColor);

    // Tag sprite (ZULU, L, R)
    tagSprite.setPsram(true);
    tagSprite.setColorDepth(24);
    tagSprite.createSprite(18, 18);
    tagSprite.loadFont(IFEI_Labels_16);
    tagSprite.setTextColor(ifeiColor);

    // Nozzle composite sprites
    nozCompositeL.setPsram(true);
    nozCompositeL.setColorDepth(24);
    nozCompositeL.createSprite(150, 154);

    nozCompositeR.setPsram(true);
    nozCompositeR.setColorDepth(24);
    nozCompositeR.createSprite(150, 154);

    // Nozzle band (combined area)
    nozzleBand.setPsram(true);
    nozzleBand.setColorDepth(24);
    nozzleBand.createSprite(NOZ_BAND_W, NOZ_BAND_H);

    debugPrintln("  Sprites created (9 sprites, 24-bit PSRAM)");
}

// =============================================================================
// INIT
// =============================================================================
void TFTIFEIDisplay_init() {
    debugPrintln("TFT IFEI: Initializing...");

    ch422g_init();
    delay(50);

    tft.init();
    tft.fillScreen(TFT_BLACK);
    tft.setBrightness(150);

    debugPrintln("  TFT initialized (800x480 RGB parallel)");

    createSprites();

    // --- Subscribe to all IFEI display fields ---
    // Engine data
    subscribeToDisplayChange("IFEI_RPM_L",       onRpmL);
    subscribeToDisplayChange("IFEI_RPM_R",       onRpmR);
    subscribeToDisplayChange("IFEI_TEMP_L",      onTempL);
    subscribeToDisplayChange("IFEI_TEMP_R",      onTempR);
    subscribeToDisplayChange("IFEI_FF_L",        onFfL);
    subscribeToDisplayChange("IFEI_FF_R",        onFfR);
    subscribeToDisplayChange("IFEI_OIL_PRESS_L", onOilL);
    subscribeToDisplayChange("IFEI_OIL_PRESS_R", onOilR);

    // Fuel
    subscribeToDisplayChange("IFEI_FUEL_UP",     onFuelUp);
    subscribeToDisplayChange("IFEI_FUEL_DOWN",   onFuelDown);
    subscribeToDisplayChange("IFEI_BINGO",       onBingo);

    // SP/Codes overlays
    subscribeToDisplayChange("IFEI_SP",          onSp);
    subscribeToDisplayChange("IFEI_CODES",       onCodes);

    // T mode / Time set overlays
    subscribeToDisplayChange("IFEI_T",           onT);
    subscribeToDisplayChange("IFEI_TIME_SET_MODE", onTimeSet);

    // Clock
    subscribeToDisplayChange("IFEI_CLOCK_H",     onClockH);
    subscribeToDisplayChange("IFEI_CLOCK_M",     onClockM);
    subscribeToDisplayChange("IFEI_CLOCK_S",     onClockS);
    subscribeToDisplayChange("IFEI_DD_1",        onDd1);
    subscribeToDisplayChange("IFEI_DD_2",        onDd2);

    // Timer
    subscribeToDisplayChange("IFEI_TIMER_H",     onTimerH);
    subscribeToDisplayChange("IFEI_TIMER_M",     onTimerM);
    subscribeToDisplayChange("IFEI_TIMER_S",     onTimerS);
    subscribeToDisplayChange("IFEI_DD_3",        onDd3);
    subscribeToDisplayChange("IFEI_DD_4",        onDd4);

    // Texture visibility flags (labels)
    subscribeToDisplayChange("IFEI_RPM_TEXTURE",      onTexRpm);
    subscribeToDisplayChange("IFEI_TEMP_TEXTURE",     onTexTemp);
    subscribeToDisplayChange("IFEI_FF_TEXTURE",       onTexFf);
    subscribeToDisplayChange("IFEI_OIL_TEXTURE",      onTexOil);
    subscribeToDisplayChange("IFEI_NOZ_TEXTURE",      onTexNoz);
    subscribeToDisplayChange("IFEI_BINGO_TEXTURE",    onTexBingo);
    subscribeToDisplayChange("IFEI_Z_TEXTURE",        onTexZ);
    subscribeToDisplayChange("IFEI_L_TEXTURE",        onTexL);
    subscribeToDisplayChange("IFEI_R_TEXTURE",        onTexR);

    // Nozzle texture visibility flags
    subscribeToDisplayChange("IFEI_LPOINTER_TEXTURE", onTexLPointer);
    subscribeToDisplayChange("IFEI_RPOINTER_TEXTURE", onTexRPointer);
    subscribeToDisplayChange("IFEI_LSCALE_TEXTURE",   onTexLScale);
    subscribeToDisplayChange("IFEI_RSCALE_TEXTURE",   onTexRScale);
    subscribeToDisplayChange("IFEI_L0_TEXTURE",       onTexL0);
    subscribeToDisplayChange("IFEI_L50_TEXTURE",      onTexL50);
    subscribeToDisplayChange("IFEI_L100_TEXTURE",     onTexL100);
    subscribeToDisplayChange("IFEI_R0_TEXTURE",       onTexR0);
    subscribeToDisplayChange("IFEI_R50_TEXTURE",      onTexR50);
    subscribeToDisplayChange("IFEI_R100_TEXTURE",     onTexR100);

    // Nozzle position (metadata — integer values)
    subscribeToMetadataChange("EXT_NOZZLE_POS_L",  onNozzlePosL);
    subscribeToMetadataChange("EXT_NOZZLE_POS_R",  onNozzlePosR);

    // Color mode and brightness
    subscribeToSelectorChange("COCKKPIT_LIGHT_MODE_SW", onLightModeChange);
    subscribeToLedChange("IFEI_DISP_INT_LT", onBrightnessChange);

    // Initial full draw
    drawFullScreen();
    needsFullRedraw = false;

    debugPrintln("TFT IFEI: Initialized successfully");
    debugPrintf("  PSRAM free: %u bytes\n", (unsigned)ESP.getFreePsram());
}

// =============================================================================
// LOOP
// =============================================================================
void TFTIFEIDisplay_loop() {
    if (!displayDirty) return;

    const unsigned long now = millis();
    if (now - lastDrawTime < REFRESH_INTERVAL_MS) return;
    lastDrawTime = now;

    if (needsFullRedraw) {
        drawFullScreen();
        needsFullRedraw = false;
    } else {
        drawDirtyFields();
    }

    displayDirty = false;
}

// =============================================================================
// MISSION START NOTIFICATION
// =============================================================================
void TFTIFEIDisplay_notifyMissionStart() {
    needsFullRedraw = true;
    displayDirty = true;
}

#endif // HAS_TFT_IFEI && ENABLE_TFT_GAUGES
