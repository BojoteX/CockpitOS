// =============================================================================
// TFT_Display_IFEI.cpp — CockpitOS TFT IFEI (Proof of Concept)
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
//   - RGB parallel = framebuffer in PSRAM, scanned by LCD peripheral (no DMA flush)
//   - Full-screen LGFX_Sprite as render target, direct pushSprite regions to LCD
//   - subscribeToDisplayChange() for all IFEI string fields
//   - subscribeToLedChange() for brightness
//   - Day/NVG color mode (white vs green text on black)
//   - SP/CODES overlay TEMP, T/TIME_SET_MODE overlay FUEL (same as 7-seg IFEI)
//   - Built-in font rendering (no LittleFS font files required for POC)
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
static constexpr uint32_t COLOR_NVG   = 0x1CDD2AU;  // Green (NVG)
static constexpr uint32_t COLOR_BLACK = 0x000000U;

// =============================================================================
// CH422G INITIALIZATION
// =============================================================================
// The CH422G uses I2C address-based operation (NOT register-based like PCA9555).
// Write mode byte to addr 0x24, then set output pins via addr 0x38.
// Only called once at boot — never again.

static void ch422g_init() {
    Wire.begin(SDA_PIN, SCL_PIN);

    // Set CH422G to push-pull output mode
    Wire.beginTransmission(CH422G_MODE_ADDR);
    Wire.write(0x01);  // Push-pull output mode
    Wire.endTransmission();

    // Set outputs: backlight ON, LCD reset HIGH (inactive), touch reset HIGH (inactive)
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
        // --- Panel (configured BEFORE bus — bus references panel) ---
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

        // --- Bus (RGB parallel) ---
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;

            // Data pins (directly mapped to ESP32-S3 LCD peripheral)
            cfg.pin_d0  = IFEI_TFT_B0;   // B0 = GPIO14
            cfg.pin_d1  = IFEI_TFT_B1;   // B1 = GPIO38
            cfg.pin_d2  = IFEI_TFT_B2;   // B2 = GPIO18
            cfg.pin_d3  = IFEI_TFT_B3;   // B3 = GPIO17
            cfg.pin_d4  = IFEI_TFT_B4;   // B4 = GPIO10
            cfg.pin_d5  = IFEI_TFT_G0;   // G0 = GPIO39
            cfg.pin_d6  = IFEI_TFT_G1;   // G1 = GPIO0
            cfg.pin_d7  = IFEI_TFT_G2;   // G2 = GPIO45
            cfg.pin_d8  = IFEI_TFT_G3;   // G3 = GPIO48
            cfg.pin_d9  = IFEI_TFT_G4;   // G4 = GPIO47
            cfg.pin_d10 = IFEI_TFT_G5;   // G5 = GPIO21
            cfg.pin_d11 = IFEI_TFT_R0;   // R0 = GPIO1
            cfg.pin_d12 = IFEI_TFT_R1;   // R1 = GPIO2
            cfg.pin_d13 = IFEI_TFT_R2;   // R2 = GPIO42
            cfg.pin_d14 = IFEI_TFT_R3;   // R3 = GPIO41
            cfg.pin_d15 = IFEI_TFT_R4;   // R4 = GPIO40

            // Control signals
            cfg.pin_henable = IFEI_TFT_DE;
            cfg.pin_vsync   = IFEI_TFT_VSYNC;
            cfg.pin_hsync   = IFEI_TFT_HSYNC;
            cfg.pin_pclk    = IFEI_TFT_PCLK;

            // Pixel clock frequency
            cfg.freq_write = 12000000;  // 12 MHz PCLK (safe for 800x480)

            // Timing parameters (Waveshare ESP32-S3-Touch-LCD-7 / ST7262)
            // Values from LovyanGFX community testing for this specific board
            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 50;
            cfg.hsync_pulse_width = 8;
            cfg.hsync_back_porch  = 8;
            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 3;
            cfg.vsync_back_porch  = 8;

            cfg.pclk_active_neg   = 1;
            cfg.de_idle_high      = 0;
            cfg.pclk_idle_high    = 0;

            _bus_instance.config(cfg);
        }

        _panel_instance.setBus(&_bus_instance);

        // --- Touch (GT911) ---
        {
            auto tcfg = _touch_instance.config();
            tcfg.i2c_port = 0;
            tcfg.i2c_addr = 0x14;  // GT911 default (alt: 0x5D)
            tcfg.pin_sda  = IFEI_TOUCH_SDA;
            tcfg.pin_scl  = IFEI_TOUCH_SCL;
            tcfg.pin_int  = IFEI_TOUCH_INT;
            tcfg.pin_rst  = -1;  // RST controlled by CH422G, already released
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

// Current text color (switches between day/NVG)
static volatile uint32_t ifei_color = COLOR_DAY;

// Dirty flag: set true when any field changes, cleared after draw
static volatile bool displayDirty = true;
static volatile bool needsFullRedraw = true;

// Timing
static unsigned long lastDrawTime = 0;

// =============================================================================
// IFEI FIELD STORAGE
// =============================================================================
// All fields stored as null-terminated char arrays, matching DCS-BIOS string lengths.
// An extra "dirty" flag per field allows selective redraw.

// Engine data (left/right)
static char fld_rpm_l[4]       = "";   static bool dirty_rpm_l       = true;
static char fld_rpm_r[4]       = "";   static bool dirty_rpm_r       = true;
static char fld_temp_l[4]      = "";   static bool dirty_temp_l      = true;
static char fld_temp_r[4]      = "";   static bool dirty_temp_r      = true;
static char fld_ff_l[4]        = "";   static bool dirty_ff_l        = true;
static char fld_ff_r[4]        = "";   static bool dirty_ff_r        = true;
static char fld_oil_l[4]       = "";   static bool dirty_oil_l       = true;
static char fld_oil_r[4]       = "";   static bool dirty_oil_r       = true;

// Fuel
static char fld_fuel_up[7]     = "";   static bool dirty_fuel_up     = true;
static char fld_fuel_down[7]   = "";   static bool dirty_fuel_down   = true;
static char fld_bingo[6]       = "";   static bool dirty_bingo       = true;

// SP/Codes (overlay temp fields)
static char fld_sp[4]          = "";   static bool dirty_sp          = true;
static char fld_codes[4]       = "";   static bool dirty_codes       = true;

// T mode / Time set (overlay fuel fields)
static char fld_t[7]           = "";   static bool dirty_t           = true;
static char fld_time_set[7]    = "";   static bool dirty_time_set    = true;

// Clock (upper)
static char fld_clock_h[3]     = "";   static bool dirty_clock_h     = true;
static char fld_clock_m[3]     = "";   static bool dirty_clock_m     = true;
static char fld_clock_s[3]     = "";   static bool dirty_clock_s     = true;
static char fld_dd_1[2]        = "";   // colon 1
static char fld_dd_2[2]        = "";   // colon 2

// Timer (lower clock)
static char fld_timer_h[3]     = "";   static bool dirty_timer       = true;
static char fld_timer_m[3]     = "";
static char fld_timer_s[3]     = "";
static char fld_dd_3[2]        = "";   // colon 3
static char fld_dd_4[2]        = "";   // colon 4

// Texture visibility flags (1-char strings: "0" or "1")
static bool tex_rpm    = false;
static bool tex_temp   = false;
static bool tex_ff     = false;
static bool tex_oil    = false;
static bool tex_noz    = false;
static bool tex_bingo  = false;
static bool tex_z      = false;
static bool tex_l      = false;
static bool tex_r      = false;

// Dirty flags for labels and textures
static bool dirty_labels = true;

// =============================================================================
// LAYOUT CONSTANTS
// =============================================================================
// Layout loosely based on reference implementation, scaled to 800x480
// Left column: engine data (RPM, TEMP/SP, FF, NOZ area, OIL)
// Right column: FUEL, BINGO, CLOCK/TIMER
//
// The IFEI is roughly split:
//   Left half (0-420):  L-data | labels | R-data
//   Right half (420-800): fuel, bingo, clocks

// Vertical positions (Y)
static constexpr int16_t Y_RPM      = 30;
static constexpr int16_t Y_TEMP     = 110;
static constexpr int16_t Y_FF       = 190;
static constexpr int16_t Y_NOZ_AREA = 280;   // Nozzle gauge area (placeholder for POC)
static constexpr int16_t Y_OIL      = 400;

// Horizontal positions for left-column engine data
static constexpr int16_t X_LEFT_DATA   = 30;    // Left engine data right-edge
static constexpr int16_t X_LABEL       = 175;   // Center label (RPM, TEMP, etc.)
static constexpr int16_t X_RIGHT_DATA  = 320;   // Right engine data right-edge

// Right column
static constexpr int16_t X_FUEL      = 560;
static constexpr int16_t Y_FUEL_UP   = 40;
static constexpr int16_t Y_FUEL_DOWN = 120;
static constexpr int16_t Y_BINGO     = 230;
static constexpr int16_t Y_BINGO_LBL = 200;
static constexpr int16_t Y_CLOCK     = 330;
static constexpr int16_t Y_TIMER     = 410;

// Tag positions
static constexpr int16_t X_TAG_L     = 700;
static constexpr int16_t Y_TAG_L     = 55;
static constexpr int16_t X_TAG_R     = 700;
static constexpr int16_t Y_TAG_R     = 130;
static constexpr int16_t X_TAG_Z     = 730;
static constexpr int16_t Y_TAG_Z     = 350;

// Font sizes
static constexpr float FONT_DATA     = 4.0f;   // setTextSize multiplier for data digits
static constexpr float FONT_LABEL    = 2.0f;   // setTextSize for labels
static constexpr float FONT_FUEL     = 4.0f;
static constexpr float FONT_CLOCK    = 3.5f;
static constexpr float FONT_TAG      = 2.0f;

// =============================================================================
// HELPER: Copy and trim spaces (same as reference)
// =============================================================================
static void copyTrimmed(const char* src, char* dest, size_t destSize) {
    if (!src || !dest || destSize == 0) return;
    // Skip leading spaces
    while (*src && *src == ' ') src++;
    // Find end
    const char* end = src + strlen(src);
    while (end > src && *(end - 1) == ' ') end--;
    size_t n = (size_t)(end - src);
    if (n >= destSize) n = destSize - 1;
    memcpy(dest, src, n);
    dest[n] = '\0';
}

// =============================================================================
// HELPER: Check if string is all spaces/empty
// =============================================================================
static bool isBlankString(const char* s, size_t len) {
    if (!s) return true;
    for (size_t i = 0; i < len; i++) {
        if (s[i] != ' ' && s[i] != '\0') return false;
    }
    return true;
}

// =============================================================================
// DCS-BIOS DISPLAY CHANGE CALLBACKS
// =============================================================================
// Signature: void callback(const char* label, const char* value)

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

static void onSp(const char*, const char* v)    { copyTrimmed(v, fld_sp, sizeof(fld_sp));       dirty_sp = true; displayDirty = true; }
static void onCodes(const char*, const char* v)  { copyTrimmed(v, fld_codes, sizeof(fld_codes)); dirty_codes = true; displayDirty = true; }
static void onT(const char*, const char* v)      { copyTrimmed(v, fld_t, sizeof(fld_t));         dirty_t = true; displayDirty = true; }
static void onTimeSet(const char*, const char* v){ copyTrimmed(v, fld_time_set, sizeof(fld_time_set)); dirty_time_set = true; displayDirty = true; }

// Clock
static void onClockH(const char*, const char* v) { copyTrimmed(v, fld_clock_h, sizeof(fld_clock_h)); dirty_clock_h = true; displayDirty = true; }
static void onClockM(const char*, const char* v) { copyTrimmed(v, fld_clock_m, sizeof(fld_clock_m)); dirty_clock_m = true; displayDirty = true; }
static void onClockS(const char*, const char* v) { copyTrimmed(v, fld_clock_s, sizeof(fld_clock_s)); dirty_clock_s = true; displayDirty = true; }
static void onDd1(const char*, const char* v)    { if (v) { fld_dd_1[0] = v[0]; fld_dd_1[1] = '\0'; } dirty_clock_h = true; displayDirty = true; }
static void onDd2(const char*, const char* v)    { if (v) { fld_dd_2[0] = v[0]; fld_dd_2[1] = '\0'; } dirty_clock_m = true; displayDirty = true; }

// Timer
static void onTimerH(const char*, const char* v) { copyTrimmed(v, fld_timer_h, sizeof(fld_timer_h)); dirty_timer = true; displayDirty = true; }
static void onTimerM(const char*, const char* v) { copyTrimmed(v, fld_timer_m, sizeof(fld_timer_m)); dirty_timer = true; displayDirty = true; }
static void onTimerS(const char*, const char* v) { copyTrimmed(v, fld_timer_s, sizeof(fld_timer_s)); dirty_timer = true; displayDirty = true; }
static void onDd3(const char*, const char* v)    { if (v) { fld_dd_3[0] = v[0]; fld_dd_3[1] = '\0'; } dirty_timer = true; displayDirty = true; }
static void onDd4(const char*, const char* v)    { if (v) { fld_dd_4[0] = v[0]; fld_dd_4[1] = '\0'; } dirty_timer = true; displayDirty = true; }

// Texture callbacks (visibility flags)
static void onTexRpm(const char*, const char* v)   { tex_rpm   = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }
static void onTexTemp(const char*, const char* v)  { tex_temp  = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }
static void onTexFf(const char*, const char* v)    { tex_ff    = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }
static void onTexOil(const char*, const char* v)   { tex_oil   = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }
static void onTexNoz(const char*, const char* v)   { tex_noz   = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }
static void onTexBingo(const char*, const char* v) { tex_bingo = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }
static void onTexZ(const char*, const char* v)     { tex_z     = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }
static void onTexL(const char*, const char* v)     { tex_l     = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }
static void onTexR(const char*, const char* v)     { tex_r     = (v && v[0] == '1'); dirty_labels = true; displayDirty = true; }

// Nozzle textures omitted for POC — add back when nozzle gauge is implemented

// =============================================================================
// DCS-BIOS LED CHANGE CALLBACK (brightness)
// =============================================================================
// IFEI_DISP_INT_LT at 0x7564 — display intensity, 0-65535
// In the reference demo this maps to setBrightness. For RGB parallel the backlight
// is controlled by CH422G (on/off only). We use the value to switch Day/NVG mode.
// The real IFEI brightness dimmer only changes brightness in night mode.
// For this POC we simply use it as a day/NVG threshold.

static void onBrightnessChange(const char*, uint16_t value, uint16_t) {
    // Use a threshold to decide day vs NVG mode
    // In real F/A-18C, cockpit_light_mode_sw drives this, but for POC
    // we use the IFEI dimmer value. Low dimmer = NVG.
    uint32_t newColor = (value > 32768) ? COLOR_DAY : COLOR_NVG;
    if (newColor != ifei_color) {
        ifei_color = newColor;
        needsFullRedraw = true;
        displayDirty = true;
    }
}

// =============================================================================
// DRAWING FUNCTIONS
// =============================================================================

// Draw a right-aligned text field in a cleared rectangle
static void drawDataField(int16_t x, int16_t y, int16_t w, int16_t h,
                           const char* text, float fontSize) {
    tft.fillRect(x, y, w, h, COLOR_BLACK);
    if (text[0] == '\0') return;
    tft.setTextSize(fontSize);
    tft.setTextColor(ifei_color, COLOR_BLACK);
    int16_t tw = tft.textWidth(text);
    tft.setCursor(x + w - tw, y);
    tft.print(text);
}

// Draw a center-aligned label
static void drawLabel(int16_t cx, int16_t y, const char* text, float fontSize) {
    tft.setTextSize(fontSize);
    tft.setTextColor(ifei_color, COLOR_BLACK);
    int16_t tw = tft.textWidth(text);
    int16_t th = tft.fontHeight();
    tft.fillRect(cx - 40, y, 80, th, COLOR_BLACK);
    tft.setCursor(cx - tw / 2, y);
    tft.print(text);
}

// Draw clock/timer field: "HH:MM:SS" format
static void drawClockField(int16_t x, int16_t y,
                            const char* h, const char* d1,
                            const char* m, const char* d2,
                            const char* s) {
    static constexpr int16_t CLOCK_W = 220;
    static constexpr int16_t CLOCK_H = 40;
    tft.fillRect(x, y, CLOCK_W, CLOCK_H, COLOR_BLACK);
    tft.setTextSize(FONT_CLOCK);
    tft.setTextColor(ifei_color, COLOR_BLACK);
    tft.setCursor(x, y);

    if (h[0] != '\0') tft.print(h);
    if (d1[0] != '\0' && d1[0] != ' ') tft.print(d1);
    else tft.print(" ");
    if (m[0] != '\0') tft.print(m);
    if (d2[0] != '\0' && d2[0] != ' ') tft.print(d2);
    else tft.print(" ");
    if (s[0] != '\0') tft.print(s);
}

// =============================================================================
// FULL REDRAW
// =============================================================================
static void drawFullScreen() {
    tft.fillScreen(COLOR_BLACK);

    // --- Left column: engine data ---
    // RPM
    drawDataField(X_LEFT_DATA, Y_RPM, 120, 40, fld_rpm_l, FONT_DATA);
    drawDataField(X_RIGHT_DATA, Y_RPM, 120, 40, fld_rpm_r, FONT_DATA);
    if (tex_rpm) drawLabel(X_LABEL, Y_RPM + 10, "RPM", FONT_LABEL);

    // TEMP / SP+CODES overlay
    {
        const char* left  = (!isBlankString(fld_sp, 3))    ? fld_sp    : fld_temp_l;
        const char* right = (!isBlankString(fld_codes, 3)) ? fld_codes : fld_temp_r;
        drawDataField(X_LEFT_DATA, Y_TEMP, 150, 40, left, FONT_DATA);
        drawDataField(X_RIGHT_DATA, Y_TEMP, 150, 40, right, FONT_DATA);
        if (tex_temp && isBlankString(fld_sp, 3)) drawLabel(X_LABEL, Y_TEMP + 10, "TEMP", FONT_LABEL);
    }

    // FF
    drawDataField(X_LEFT_DATA, Y_FF, 150, 40, fld_ff_l, FONT_DATA);
    drawDataField(X_RIGHT_DATA, Y_FF, 150, 40, fld_ff_r, FONT_DATA);
    if (tex_ff) {
        drawLabel(X_LABEL, Y_FF + 5, "FF", FONT_LABEL);
        drawLabel(X_LABEL, Y_FF + 25, "X100", FONT_LABEL * 0.75f);
    }

    // NOZ area (placeholder text for POC — nozzle gauges require BMP assets)
    if (tex_noz) {
        drawLabel(X_LABEL, Y_NOZ_AREA + 30, "NOZ", FONT_LABEL);
        // Future: render nozzle gauges with pushRotateZoom here
    }

    // OIL
    drawDataField(X_LEFT_DATA, Y_OIL, 120, 40, fld_oil_l, FONT_DATA);
    drawDataField(X_RIGHT_DATA, Y_OIL, 120, 40, fld_oil_r, FONT_DATA);
    if (tex_oil) drawLabel(X_LABEL, Y_OIL + 10, "OIL", FONT_LABEL);

    // --- Right column: fuel, bingo, clock ---
    // FUEL UP / T-mode overlay
    {
        const char* fuelUpText = (!isBlankString(fld_t, 6)) ? fld_t : fld_fuel_up;
        drawDataField(X_FUEL, Y_FUEL_UP, 200, 40, fuelUpText, FONT_FUEL);
    }

    // Tag L
    if (tex_l) {
        tft.setTextSize(FONT_TAG);
        tft.setTextColor(ifei_color, COLOR_BLACK);
        tft.setCursor(X_TAG_L, Y_TAG_L);
        tft.print("L");
    }

    // FUEL DOWN / TIME_SET_MODE overlay
    {
        const char* fuelDnText = (!isBlankString(fld_time_set, 6)) ? fld_time_set : fld_fuel_down;
        drawDataField(X_FUEL, Y_FUEL_DOWN, 200, 40, fuelDnText, FONT_FUEL);
    }

    // Tag R
    if (tex_r) {
        tft.setTextSize(FONT_TAG);
        tft.setTextColor(ifei_color, COLOR_BLACK);
        tft.setCursor(X_TAG_R, Y_TAG_R);
        tft.print("R");
    }

    // BINGO
    if (tex_bingo) drawLabel(X_FUEL + 60, Y_BINGO_LBL, "BINGO", FONT_LABEL);
    drawDataField(X_FUEL, Y_BINGO, 200, 40, fld_bingo, FONT_FUEL);

    // CLOCK (upper)
    drawClockField(X_FUEL, Y_CLOCK, fld_clock_h, fld_dd_1, fld_clock_m, fld_dd_2, fld_clock_s);

    // Tag Z
    if (tex_z) {
        tft.setTextSize(FONT_TAG);
        tft.setTextColor(ifei_color, COLOR_BLACK);
        tft.setCursor(X_TAG_Z, Y_TAG_Z);
        tft.print("Z");
    }

    // TIMER (lower)
    drawClockField(X_FUEL, Y_TIMER, fld_timer_h, fld_dd_3, fld_timer_m, fld_dd_4, fld_timer_s);
}

// =============================================================================
// INCREMENTAL REDRAW (only dirty fields)
// =============================================================================
static void drawDirtyFields() {
    // RPM
    if (dirty_rpm_l) { drawDataField(X_LEFT_DATA, Y_RPM, 120, 40, fld_rpm_l, FONT_DATA); dirty_rpm_l = false; }
    if (dirty_rpm_r) { drawDataField(X_RIGHT_DATA, Y_RPM, 120, 40, fld_rpm_r, FONT_DATA); dirty_rpm_r = false; }

    // TEMP / SP+CODES overlay
    if (dirty_temp_l || dirty_sp) {
        const char* left = (!isBlankString(fld_sp, 3)) ? fld_sp : fld_temp_l;
        drawDataField(X_LEFT_DATA, Y_TEMP, 150, 40, left, FONT_DATA);
        dirty_temp_l = false; dirty_sp = false;
    }
    if (dirty_temp_r || dirty_codes) {
        const char* right = (!isBlankString(fld_codes, 3)) ? fld_codes : fld_temp_r;
        drawDataField(X_RIGHT_DATA, Y_TEMP, 150, 40, right, FONT_DATA);
        dirty_temp_r = false; dirty_codes = false;
    }

    // FF
    if (dirty_ff_l) { drawDataField(X_LEFT_DATA, Y_FF, 150, 40, fld_ff_l, FONT_DATA); dirty_ff_l = false; }
    if (dirty_ff_r) { drawDataField(X_RIGHT_DATA, Y_FF, 150, 40, fld_ff_r, FONT_DATA); dirty_ff_r = false; }

    // OIL
    if (dirty_oil_l) { drawDataField(X_LEFT_DATA, Y_OIL, 120, 40, fld_oil_l, FONT_DATA); dirty_oil_l = false; }
    if (dirty_oil_r) { drawDataField(X_RIGHT_DATA, Y_OIL, 120, 40, fld_oil_r, FONT_DATA); dirty_oil_r = false; }

    // FUEL UP / T-mode
    if (dirty_fuel_up || dirty_t) {
        const char* fuelUpText = (!isBlankString(fld_t, 6)) ? fld_t : fld_fuel_up;
        drawDataField(X_FUEL, Y_FUEL_UP, 200, 40, fuelUpText, FONT_FUEL);
        dirty_fuel_up = false; dirty_t = false;
    }

    // FUEL DOWN / TIME_SET_MODE
    if (dirty_fuel_down || dirty_time_set) {
        const char* fuelDnText = (!isBlankString(fld_time_set, 6)) ? fld_time_set : fld_fuel_down;
        drawDataField(X_FUEL, Y_FUEL_DOWN, 200, 40, fuelDnText, FONT_FUEL);
        dirty_fuel_down = false; dirty_time_set = false;
    }

    // BINGO
    if (dirty_bingo) { drawDataField(X_FUEL, Y_BINGO, 200, 40, fld_bingo, FONT_FUEL); dirty_bingo = false; }

    // CLOCK
    if (dirty_clock_h || dirty_clock_m || dirty_clock_s) {
        drawClockField(X_FUEL, Y_CLOCK, fld_clock_h, fld_dd_1, fld_clock_m, fld_dd_2, fld_clock_s);
        dirty_clock_h = false; dirty_clock_m = false; dirty_clock_s = false;
    }

    // TIMER
    if (dirty_timer) {
        drawClockField(X_FUEL, Y_TIMER, fld_timer_h, fld_dd_3, fld_timer_m, fld_dd_4, fld_timer_s);
        dirty_timer = false;
    }

    // Labels (only redraw all labels on texture change or color change)
    if (dirty_labels) {
        if (tex_rpm)  drawLabel(X_LABEL, Y_RPM + 10, "RPM", FONT_LABEL);
        else          tft.fillRect(X_LABEL - 40, Y_RPM + 10, 80, 20, COLOR_BLACK);

        if (tex_temp && isBlankString(fld_sp, 3)) drawLabel(X_LABEL, Y_TEMP + 10, "TEMP", FONT_LABEL);
        else tft.fillRect(X_LABEL - 40, Y_TEMP + 10, 80, 20, COLOR_BLACK);

        if (tex_ff) {
            drawLabel(X_LABEL, Y_FF + 5, "FF", FONT_LABEL);
            drawLabel(X_LABEL, Y_FF + 25, "X100", FONT_LABEL * 0.75f);
        } else {
            tft.fillRect(X_LABEL - 40, Y_FF + 5, 80, 40, COLOR_BLACK);
        }

        if (tex_noz) drawLabel(X_LABEL, Y_NOZ_AREA + 30, "NOZ", FONT_LABEL);
        else         tft.fillRect(X_LABEL - 40, Y_NOZ_AREA + 30, 80, 20, COLOR_BLACK);

        if (tex_oil) drawLabel(X_LABEL, Y_OIL + 10, "OIL", FONT_LABEL);
        else         tft.fillRect(X_LABEL - 40, Y_OIL + 10, 80, 20, COLOR_BLACK);

        if (tex_bingo) drawLabel(X_FUEL + 60, Y_BINGO_LBL, "BINGO", FONT_LABEL);
        else           tft.fillRect(X_FUEL + 20, Y_BINGO_LBL, 120, 20, COLOR_BLACK);

        // Tags
        tft.setTextSize(FONT_TAG);
        tft.setTextColor(ifei_color, COLOR_BLACK);
        tft.fillRect(X_TAG_L, Y_TAG_L, 20, 20, COLOR_BLACK);
        if (tex_l) { tft.setCursor(X_TAG_L, Y_TAG_L); tft.print("L"); }

        tft.fillRect(X_TAG_R, Y_TAG_R, 20, 20, COLOR_BLACK);
        if (tex_r) { tft.setCursor(X_TAG_R, Y_TAG_R); tft.print("R"); }

        tft.fillRect(X_TAG_Z, Y_TAG_Z, 20, 20, COLOR_BLACK);
        if (tex_z) { tft.setCursor(X_TAG_Z, Y_TAG_Z); tft.print("Z"); }

        dirty_labels = false;
    }
}

// =============================================================================
// INIT
// =============================================================================
void TFTIFEIDisplay_init() {
    debugPrintln("TFT IFEI: Initializing...");

    // Init CH422G (backlight, resets)
    ch422g_init();

    // Small delay for LCD controller to come out of reset
    delay(50);

    // Init LovyanGFX (init() auto-sets color depth, rotation, and clears screen)
    tft.init();
    tft.fillScreen(TFT_BLACK);  // Explicit clear — PSRAM may contain 0xFF (white) on first boot
    tft.setFont(&fonts::Font0);  // Built-in 8x6 mono font, scaled via setTextSize

    debugPrintln("  TFT initialized (800x480 RGB parallel)");

    // --- Subscribe to all IFEI display fields ---
    // Engine data
    subscribeToDisplayChange("IFEI_RPM_L",     onRpmL);
    subscribeToDisplayChange("IFEI_RPM_R",     onRpmR);
    subscribeToDisplayChange("IFEI_TEMP_L",    onTempL);
    subscribeToDisplayChange("IFEI_TEMP_R",    onTempR);
    subscribeToDisplayChange("IFEI_FF_L",      onFfL);
    subscribeToDisplayChange("IFEI_FF_R",      onFfR);
    subscribeToDisplayChange("IFEI_OIL_PRESS_L", onOilL);
    subscribeToDisplayChange("IFEI_OIL_PRESS_R", onOilR);

    // Fuel
    subscribeToDisplayChange("IFEI_FUEL_UP",   onFuelUp);
    subscribeToDisplayChange("IFEI_FUEL_DOWN", onFuelDown);
    subscribeToDisplayChange("IFEI_BINGO",     onBingo);

    // SP/Codes overlays
    subscribeToDisplayChange("IFEI_SP",        onSp);
    subscribeToDisplayChange("IFEI_CODES",     onCodes);

    // T mode / Time set overlays
    subscribeToDisplayChange("IFEI_T",         onT);
    subscribeToDisplayChange("IFEI_TIME_SET_MODE", onTimeSet);

    // Clock
    subscribeToDisplayChange("IFEI_CLOCK_H",   onClockH);
    subscribeToDisplayChange("IFEI_CLOCK_M",   onClockM);
    subscribeToDisplayChange("IFEI_CLOCK_S",   onClockS);
    subscribeToDisplayChange("IFEI_DD_1",      onDd1);
    subscribeToDisplayChange("IFEI_DD_2",      onDd2);

    // Timer
    subscribeToDisplayChange("IFEI_TIMER_H",   onTimerH);
    subscribeToDisplayChange("IFEI_TIMER_M",   onTimerM);
    subscribeToDisplayChange("IFEI_TIMER_S",   onTimerS);
    subscribeToDisplayChange("IFEI_DD_3",      onDd3);
    subscribeToDisplayChange("IFEI_DD_4",      onDd4);

    // Texture visibility flags
    subscribeToDisplayChange("IFEI_RPM_TEXTURE",      onTexRpm);
    subscribeToDisplayChange("IFEI_TEMP_TEXTURE",     onTexTemp);
    subscribeToDisplayChange("IFEI_FF_TEXTURE",       onTexFf);
    subscribeToDisplayChange("IFEI_OIL_TEXTURE",      onTexOil);
    subscribeToDisplayChange("IFEI_NOZ_TEXTURE",      onTexNoz);
    subscribeToDisplayChange("IFEI_BINGO_TEXTURE",    onTexBingo);
    subscribeToDisplayChange("IFEI_Z_TEXTURE",        onTexZ);
    subscribeToDisplayChange("IFEI_L_TEXTURE",        onTexL);
    subscribeToDisplayChange("IFEI_R_TEXTURE",        onTexR);

    // Nozzle textures omitted for POC (saves 10 subscription slots)

    // Brightness (LED change — integer value)
    // Note: IFEI_DISP_INT_LT is not in DcsOutputTable for this label set currently.
    // For POC, we start in day mode. If available, this will auto-subscribe.
    // subscribeToLedChange("IFEI_DISP_INT_LT", onBrightnessChange);

    // Initial draw
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
