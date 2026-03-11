// =============================================================================
// TFT_Display_IFEI.cpp — CockpitOS TFT IFEI (Pixel-Perfect)
// =============================================================================
//
// PANEL_KIND: TFTIFEI
//
// Hardware: Waveshare ESP32-S3-Touch-LCD-7 (800x480, RGB parallel, ST7262)
//           CH422G I2C IO expander (backlight, LCD/touch reset)
//           GT911 capacitive touch (unused — I2C polling causes RGB stutter)
//
// Architecture:
//   - LovyanGFX Bus_RGB + Panel_RGB for continuous framebuffer scanning
//   - VLW fonts loaded from PROGMEM (7-segment style, matching real IFEI)
//   - Shared LGFX_Sprite objects per font size — sprite-based rendering
//   - 16-bit sprite depth (RGB565) for minimal PSRAM write volume per pushSprite
//   - Day/NVG/Night color mode switching via COCKKPIT_LIGHT_MODE_SW
//   - Brightness control via IFEI_DISP_INT_LT with deduplication
//
// Nozzle gauges:
//   - 11 discrete bar positions (0%, 10%, 20% ... 100%) — matches real F/A-18C
//   - Static background (scale arc + tick marks + number labels) drawn once
//   - Only the active bar indicator changes per frame — no trig per frame
//   - Pointer visibility gated by IFEI_LPOINTER/RPOINTER_TEXTURE
//
// Overlay state machine (ported from gold-standard IFEIPanel):
//   - SP/CODES overlays TEMP L/R positions
//   - IFEI_T overlays FUEL_UP, IFEI_TIME_SET_MODE overlays FUEL_DOWN
//   - sp_active / fuel_mode_active flags with transition logic
//   - Immediate blank-on-enter, restore-on-exit, cross-suppression
//   - lastTWasBlank gates FUEL_DOWN restore to prevent flash
//
// Mission safety:
//   - isMissionRunning() guard prevents rendering stale/garbage data
//   - blankAllFields() clears all buffers between missions
//   - notifyMissionStart() triggers full redraw
//
// Reference: SCUBA82 OH-IFEI + ashchan hornet-esp32-gauges + IFEIPanel.cpp
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

// Access to the DCS-BIOS display buffer table (for busting dedup cache)
extern DisplayBufferEntry CT_DisplayBuffers[];
extern const size_t numCTDisplayBuffers;

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

// Display refresh: set Hz once, both FreeRTOS task and main-loop paths derive from it
#define RUN_TFT_IFEI_DISPLAY_AS_TASK   1   // 0 = render in main loop, 1 = dedicated FreeRTOS task
#define TFT_IFEI_DISPLAY_REFRESH_RATE_HZ 30 // 30 Hz = 33ms per frame (constant FPS)
static constexpr uint32_t REFRESH_INTERVAL_MS = 1000 / TFT_IFEI_DISPLAY_REFRESH_RATE_HZ;

// --- TROUBLESHOOTING FLAGS (set to 1 to disable for testing) ----------------
// Step 1: Disable nozzle gauges (isolates nozzle rendering from other fields)
#define IFEI_DEBUG_NO_NOZZLES      0
// Step 2: Disable ALL rendering in loop (static screen after init — isolates timing)
#define IFEI_DEBUG_NO_LOOP_RENDER  0

// Colors (RGB888 — LovyanGFX converts to RGB565 internally for 16-bit sprites)
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
            cfg.freq_write = 13900000;   // 13.9 MHz — reduced from 16 MHz to lower PSRAM bus pressure
            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 4;
            cfg.hsync_back_porch  = 43;  // 43 — widens horizontal blanking for safe PSRAM CPU access
            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 4;
            cfg.vsync_back_porch  = 16;  // 16 — extra vertical blanking for uncontested PSRAM CPU access
            cfg.pclk_active_neg   = 1;
            cfg.de_idle_high      = 0;
            cfg.pclk_idle_high    = 0;
            _bus_instance.config(cfg);
        }
        _panel_instance.setBus(&_bus_instance);
        // NOTE: GT911 touch not configured — not used for IFEI, and its periodic
        // I2C polling/interrupts cause occasional display stutter on RGB parallel.
        // Can be re-added later if touch is needed (will require careful I2C scheduling).
        setPanel(&_panel_instance);
    }
};

// =============================================================================
// DISPLAY STATE
// =============================================================================
static LGFX_WS7 tft;

// Shared sprites (reused across elements to minimize PSRAM allocation)
static LGFX_Sprite twoDigitSprite(&tft);    // 76x38 — RPM L/R, OIL L/R
static LGFX_Sprite threeDigitSprite(&tft);  // 108x38 — TEMP L/R, FF L/R
static LGFX_Sprite fuelSprite(&tft);        // 176x38 — FUEL UP/DOWN, BINGO
static LGFX_Sprite clockSprite(&tft);       // 176x35 — CLOCK UP/DOWN
static LGFX_Sprite labelSprite(&tft);       // 65x18 — all text labels
static LGFX_Sprite tagSprite(&tft);         // 18x18 — ZULU, L, R

// Nozzle sprites
static LGFX_Sprite nozzleBand(&tft);        // 308x154 — combined nozzle area

// Current text color (volatile: set in callbacks, read in loop)
static volatile uint32_t ifeiColor = COLOR_DAY;
static uint32_t drawColor = COLOR_DAY;  // non-volatile snapshot for rendering

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
    NOZT,
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
    // OIL (3-digit field: values up to 999)
    { 108, 38,  48,  439, ALIGN_RIGHT,  &threeDigitSprite, "" },  // OILL
    { 108, 38,  242, 439, ALIGN_RIGHT,  &threeDigitSprite, "" },  // OILR
    { 58,  18,  176, 450, ALIGN_CENTER, &labelSprite,      "" },  // OILT
    // NOZZLE label (gauges rendered separately into nozzleBand)
    { 58,  18,  172, 320, ALIGN_CENTER, &labelSprite,      "" },  // NOZT
    // FUEL
    { 220, 38,  522, 56,  ALIGN_RIGHT,  &fuelSprite,       "" },  // FUELU
    { 220, 38,  522, 132, ALIGN_RIGHT,  &fuelSprite,       "" },  // FUELL
    { 220, 38,  522, 246, ALIGN_RIGHT,  &fuelSprite,       "" },  // BINGO_VAL
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
// NOZZLE GAUGE — 11-bar discrete positions (matches real F/A-18C IFEI)
// =============================================================================
// The real IFEI nozzle gauge is NOT a rotating needle. It has 11 discrete
// indicator positions (0%, 10%, 20% ... 100%) arranged along an arc. Only
// one position is lit at a time — a small bar/nibble that turns on or off.
// The scale arc, tick marks, and number labels are static background.
//
// This matches IFEIPanel.cpp: addPointerBarToShadow() with 11-bar map.
// Zero trig per frame — just an index lookup.

static constexpr int16_t NOZ_BAND_X = 46;
static constexpr int16_t NOZ_BAND_Y = 252;
static constexpr int16_t NOZ_BAND_W = 340;
static constexpr int16_t NOZ_BAND_H = 180;
static constexpr int     NOZ_BARS   = 11;  // 0%, 10%, 20% ... 100%

// Gauge geometry (for static background drawing at init)
// Both gauges use identical geometry for perfect mirror symmetry.
// Right pivot = (150 - leftPivotX, leftPivotY) = exact mirror within 150px bg sprite.
static constexpr float NOZL_PIVOT_X      = 6.2f;
static constexpr float NOZL_PIVOT_Y      = 8.4f;
static constexpr float NOZR_PIVOT_X      = 143.8f;  // 150.0 - 6.2 = exact mirror
static constexpr float NOZR_PIVOT_Y      = 8.4f;    // same Y for symmetry
static constexpr float NOZ_DEG_PER_PCT   = 0.895f;  // single value, both gauges identical
static constexpr float SCALE_RADIUS      = 135.0f;
static constexpr float NEEDLE_INNER      = 65.0f;   // inner end of indicator bar
static constexpr float NEEDLE_OUTER      = 120.0f;  // outer end (10px gap before shortest tick at 130)
static constexpr float NEEDLE_HW_INNER   = 1.0f;    // half-width at base (narrow end, 2px total)
static constexpr float NEEDLE_HW_OUTER   = 3.0f;    // half-width at tip  (wide end,   6px total)

// Pre-computed bar endpoint coordinates (filled once at init, zero trig per frame)
struct NozzleBarPos {
    int16_t x1, y1;  // inner end (center)
    int16_t x2, y2;  // outer end (center)
    float perpX, perpY;  // unit perpendicular vector (for tapered needle)
};
static NozzleBarPos nozBarsL[NOZ_BARS];
static NozzleBarPos nozBarsR[NOZ_BARS];

// Cached nozzle static background (ticks + labels + NOZ text, both gauges)
// Built once at init and rebuilt only when textures or color change.
// Per frame: copy cached -> nozzleBand, draw needles, push. Zero trig per frame.
static LGFX_Sprite nozStaticBand(&tft);  // 308x154 — pre-composited background
static volatile bool nozStaticDirty = true;  // rebuild flag (volatile: set in callbacks, read in task)

// Nozzle position values (0-65535 from DCS-BIOS)
static volatile uint16_t nozzlePosL = 0;
static volatile uint16_t nozzlePosR = 0;
static volatile bool nozzleDirty = true;

// Current bar index (0-10, derived from nozzlePosL/R)
static int lastBarIdxL = -1;
static int lastBarIdxR = -1;

// Nozzle pointer visibility (gated by LPOINTER/RPOINTER texture)
// volatile: set in callbacks, read in FreeRTOS render task
static volatile bool showLeftNozPointer  = false;
static volatile bool showRightNozPointer = false;

// Nozzle texture visibility flags (scale, numbers)
// volatile: set in callbacks, read in FreeRTOS render task
static volatile bool tex_lscale = false;
static volatile bool tex_rscale = false;
static volatile bool tex_l0     = false;
static volatile bool tex_l50    = false;
static volatile bool tex_l100   = false;
static volatile bool tex_r0     = false;
static volatile bool tex_r50    = false;
static volatile bool tex_r100   = false;

// =============================================================================
// FIELD STORAGE AND DIRTY FLAGS
// =============================================================================
// All per-field dirty flags are volatile: written by DCS-BIOS callbacks
// (main loop context) and read/cleared by the FreeRTOS render task.
// Without volatile, the compiler may cache stale reads in the task loop.

// Engine data fields
static char fld_rpm_l[4]  = "";  static volatile bool dirty_rpm_l  = true;
static char fld_rpm_r[4]  = "";  static volatile bool dirty_rpm_r  = true;
static char fld_temp_l[4] = "";  static volatile bool dirty_temp_l = true;
static char fld_temp_r[4] = "";  static volatile bool dirty_temp_r = true;
static char fld_ff_l[4]   = "";  static volatile bool dirty_ff_l   = true;
static char fld_ff_r[4]   = "";  static volatile bool dirty_ff_r   = true;
static char fld_oil_l[4]  = "";  static volatile bool dirty_oil_l  = true;
static char fld_oil_r[4]  = "";  static volatile bool dirty_oil_r  = true;

// Fuel fields
static char fld_fuel_up[7]   = "";  static volatile bool dirty_fuel_up   = true;
static char fld_fuel_down[7] = "";  static volatile bool dirty_fuel_down = true;
static char fld_bingo[6]     = "";  static volatile bool dirty_bingo     = true;

// SP/Codes overlay fields
static char fld_sp[4]    = "";  static volatile bool dirty_sp    = true;
static char fld_codes[4] = "";  static volatile bool dirty_codes = true;

// T mode / Time set overlay fields
static char fld_t[7]        = "";  static volatile bool dirty_t        = true;
static char fld_time_set[7] = "";  static volatile bool dirty_time_set = true;

// Clock (upper)
static char fld_clock_h[3] = "";  static volatile bool dirty_clock = true;
static char fld_clock_m[3] = "";
static char fld_clock_s[3] = "";
static char fld_dd_1[2]    = "";
static char fld_dd_2[2]    = "";

// Timer (lower)
static char fld_timer_h[3] = "";  static volatile bool dirty_timer = true;
static char fld_timer_m[3] = "";
static char fld_timer_s[3] = "";
static char fld_dd_3[2]    = "";
static char fld_dd_4[2]    = "";

// Texture visibility flags (label text on/off from DCS-BIOS)
// volatile: set in callbacks, read in FreeRTOS render task
static volatile bool tex_rpm   = false;  static volatile bool dirty_tex_rpm   = true;
static volatile bool tex_temp  = false;  static volatile bool dirty_tex_temp  = true;
static volatile bool tex_ff    = false;  static volatile bool dirty_tex_ff    = true;
static volatile bool tex_oil   = false;  static volatile bool dirty_tex_oil   = true;
static volatile bool tex_noz   = false;  static volatile bool dirty_tex_noz   = true;
static volatile bool tex_bingo = false;  static volatile bool dirty_tex_bingo = true;
static volatile bool tex_z     = false;  static volatile bool dirty_tex_z     = true;
static volatile bool tex_l     = false;  static volatile bool dirty_tex_l     = true;
static volatile bool tex_r     = false;  static volatile bool dirty_tex_r     = true;

// =============================================================================
// OVERLAY STATE MACHINE (ported from gold-standard IFEIPanel.cpp)
// =============================================================================
// The IFEI has two overlay systems that share screen real estate:
//
// 1. SP/CODES: When active, SP replaces TEMP_L, CODES replaces TEMP_R.
//    While SP is active, both FUEL fields are blanked (fuel is meaningless
//    during SP/CODES display). On exit, base TEMP and FUEL values restore.
//
// 2. T/TIME_SET: When IFEI_T is active, it replaces FUEL_UP and blanks
//    TEMP L/R (they show different data in this mode). IFEI_TIME_SET_MODE
//    replaces FUEL_DOWN. On exit, all four fields restore from base cache.
//
// sp_active and fuel_mode_active are the master flags.
// lastTWasBlank gates whether FUEL_DOWN gets restored on SP exit (prevents flash).

static volatile bool sp_active = false;         // SP/CODES overlay active
static volatile bool fuel_mode_active = false;   // T/TIME_SET overlay active
static volatile bool lastTWasBlank = false;      // last IFEI_T was blank (gates FUEL_DOWN restore)

// Base value caches — the "real" values behind overlays
static char tempL_base[4]    = "";
static char tempR_base[4]    = "";
static char fuelUp_base[7]   = "";
static char fuelDn_base[7]   = "";

// Per-field overlay tracking
static volatile bool tempL_overlay = false;  // SP is overriding TEMP_L
static volatile bool tempR_overlay = false;  // CODES is overriding TEMP_R
static volatile bool fuelUp_overlay = false; // T is overriding FUEL_UP

// =============================================================================
// HELPERS
// =============================================================================
// Strip leading/trailing spaces from a DCS-BIOS string value.
// DCS-BIOS pads fixed-width fields with spaces; trimming prevents
// visual artifacts and ensures strcmp-based dedup works correctly.
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
    if (!s || !s[0]) return true;
    for (size_t i = 0; i < maxLen && s[i] != '\0'; i++) {
        if (s[i] != ' ') return false;
    }
    return true;
}

// Trim and compare a new DCS-BIOS value against the cached field buffer.
// Returns true (and updates dest) only if the value actually changed,
// enabling dirty-flag deduplication across all data callbacks.
static bool updateField(const char* v, char* dest, size_t destSize) {
    char tmp[8];
    copyTrimmed(v, tmp, (destSize < sizeof(tmp)) ? destSize : sizeof(tmp));
    if (strcmp(tmp, dest) != 0) {
        memcpy(dest, tmp, destSize);
        return true;
    }
    return false;
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
// Engine data — straightforward: update field, mark dirty
// isMissionRunning() guard: prevents garbage data when no DCS mission is active
// (gold standard: renderIFEIDispatcher line 594 gates ALL field processing)
static void onRpmL(const char*, const char* v)  { if (!isMissionRunning()) return; if (updateField(v, fld_rpm_l, sizeof(fld_rpm_l)))   { dirty_rpm_l  = true; displayDirty = true; } }
static void onRpmR(const char*, const char* v)  { if (!isMissionRunning()) return; if (updateField(v, fld_rpm_r, sizeof(fld_rpm_r)))   { dirty_rpm_r  = true; displayDirty = true; } }
static void onTempL(const char*, const char* v) {
    if (!isMissionRunning()) return;
    if (updateField(v, fld_temp_l, sizeof(fld_temp_l))) {
        // Cache base value for overlay restore (only when non-blank)
        if (!isBlankString(fld_temp_l, 3)) memcpy(tempL_base, fld_temp_l, sizeof(tempL_base));
        dirty_temp_l = true; displayDirty = true;
    }
}
static void onTempR(const char*, const char* v) {
    if (!isMissionRunning()) return;
    if (updateField(v, fld_temp_r, sizeof(fld_temp_r))) {
        if (!isBlankString(fld_temp_r, 3)) memcpy(tempR_base, fld_temp_r, sizeof(tempR_base));
        dirty_temp_r = true; displayDirty = true;
    }
}
static void onFfL(const char*, const char* v)   { if (!isMissionRunning()) return; if (updateField(v, fld_ff_l, sizeof(fld_ff_l)))     { dirty_ff_l   = true; displayDirty = true; } }
static void onFfR(const char*, const char* v)   { if (!isMissionRunning()) return; if (updateField(v, fld_ff_r, sizeof(fld_ff_r)))     { dirty_ff_r   = true; displayDirty = true; } }
static void onOilL(const char*, const char* v)  { if (!isMissionRunning()) return; if (updateField(v, fld_oil_l, sizeof(fld_oil_l)))   { dirty_oil_l  = true; displayDirty = true; } }
static void onOilR(const char*, const char* v)  { if (!isMissionRunning()) return; if (updateField(v, fld_oil_r, sizeof(fld_oil_r)))   { dirty_oil_r  = true; displayDirty = true; } }

// Fuel — cache base values for overlay restore
static void onFuelUp(const char*, const char* v) {
    if (!isMissionRunning()) return;
    if (updateField(v, fld_fuel_up, sizeof(fld_fuel_up))) {
        if (!isBlankString(fld_fuel_up, 6)) memcpy(fuelUp_base, fld_fuel_up, sizeof(fuelUp_base));
        dirty_fuel_up = true; displayDirty = true;
    }
}
static void onFuelDown(const char*, const char* v) {
    if (!isMissionRunning()) return;
    if (updateField(v, fld_fuel_down, sizeof(fld_fuel_down))) {
        if (!isBlankString(fld_fuel_down, 6)) memcpy(fuelDn_base, fld_fuel_down, sizeof(fuelDn_base));
        dirty_fuel_down = true; displayDirty = true;
    }
}
static void onBingo(const char*, const char* v) { if (!isMissionRunning()) return; if (updateField(v, fld_bingo, sizeof(fld_bingo))) { dirty_bingo = true; displayDirty = true; } }

// SP/CODES overlay — these replace TEMP L/R when non-blank
static void onSp(const char*, const char* v) {
    if (!isMissionRunning()) return;
    if (updateField(v, fld_sp, sizeof(fld_sp))) {
        bool wasActive = sp_active;
        bool blank = isBlankString(fld_sp, 3);

        if (!blank) {
            tempL_overlay = true;
        } else {
            tempL_overlay = false;
        }
        sp_active = (tempL_overlay || tempR_overlay);

        // Entering SP: blank both fuel fields immediately
        if (!wasActive && sp_active) {
            dirty_fuel_up = true;
            dirty_fuel_down = true;
        }
        // Exiting SP: restore fuel fields
        if (wasActive && !sp_active) {
            dirty_fuel_up = true;
            if (lastTWasBlank) dirty_fuel_down = true;
        }

        dirty_sp = true; dirty_temp_l = true; displayDirty = true;
    }
}
static void onCodes(const char*, const char* v) {
    if (!isMissionRunning()) return;
    if (updateField(v, fld_codes, sizeof(fld_codes))) {
        bool wasActive = sp_active;
        bool blank = isBlankString(fld_codes, 3);

        if (!blank) {
            tempR_overlay = true;
        } else {
            tempR_overlay = false;
        }
        sp_active = (tempL_overlay || tempR_overlay);

        if (!wasActive && sp_active) {
            dirty_fuel_up = true;
            dirty_fuel_down = true;
        }
        if (wasActive && !sp_active) {
            dirty_fuel_up = true;
            if (lastTWasBlank) dirty_fuel_down = true;
        }

        dirty_codes = true; dirty_temp_r = true; displayDirty = true;
    }
}

// T mode overlay — replaces FUEL_UP, blanks TEMP while active
static void onT(const char*, const char* v) {
    if (!isMissionRunning()) return;
    if (updateField(v, fld_t, sizeof(fld_t))) {
        bool was_t = fuel_mode_active;
        bool blank = isBlankString(fld_t, 6);

        lastTWasBlank = blank;

        if (!blank) {
            fuelUp_overlay = true;
        } else {
            fuelUp_overlay = false;
        }
        fuel_mode_active = fuelUp_overlay;

        // Entering T-mode: blank TEMP L/R and FUEL_DOWN immediately
        if (!was_t && fuel_mode_active) {
            dirty_temp_l = true;
            dirty_temp_r = true;
            dirty_fuel_down = true;
        }
        // Exiting T-mode: restore TEMP and FUEL
        if (was_t && !fuel_mode_active) {
            dirty_temp_l = true;
            dirty_temp_r = true;
            dirty_fuel_up = true;
            dirty_fuel_down = true;
        }

        dirty_t = true; dirty_fuel_up = true; displayDirty = true;
    }
}

// TIME_SET overlay — replaces FUEL_DOWN when in T-mode
static void onTimeSet(const char*, const char* v) {
    if (!isMissionRunning()) return;
    if (updateField(v, fld_time_set, sizeof(fld_time_set))) {
        dirty_time_set = true;
        // Gate: only mark FUEL_DOWN dirty if in T-mode or lastTWasBlank
        // (prevents FUEL_DOWN flash when TIME_SET blanks outside T-mode)
        if (fuel_mode_active || lastTWasBlank) {
            dirty_fuel_down = true;
        }
        displayDirty = true;
    }
}

// Clock
static void onClockH(const char*, const char* v) { if (!isMissionRunning()) return; if (updateField(v, fld_clock_h, sizeof(fld_clock_h))) { dirty_clock = true; displayDirty = true; } }
static void onClockM(const char*, const char* v) { if (!isMissionRunning()) return; if (updateField(v, fld_clock_m, sizeof(fld_clock_m))) { dirty_clock = true; displayDirty = true; } }
static void onClockS(const char*, const char* v) { if (!isMissionRunning()) return; if (updateField(v, fld_clock_s, sizeof(fld_clock_s))) { dirty_clock = true; displayDirty = true; } }
static void onDd1(const char*, const char* v)    { if (!isMissionRunning()) return; if (v && fld_dd_1[0] != v[0]) { fld_dd_1[0] = v[0]; fld_dd_1[1] = '\0'; dirty_clock = true; displayDirty = true; } }
static void onDd2(const char*, const char* v)    { if (!isMissionRunning()) return; if (v && fld_dd_2[0] != v[0]) { fld_dd_2[0] = v[0]; fld_dd_2[1] = '\0'; dirty_clock = true; displayDirty = true; } }

// Timer
static void onTimerH(const char*, const char* v) { if (!isMissionRunning()) return; if (updateField(v, fld_timer_h, sizeof(fld_timer_h))) { dirty_timer = true; displayDirty = true; } }
static void onTimerM(const char*, const char* v) { if (!isMissionRunning()) return; if (updateField(v, fld_timer_m, sizeof(fld_timer_m))) { dirty_timer = true; displayDirty = true; } }
static void onTimerS(const char*, const char* v) { if (!isMissionRunning()) return; if (updateField(v, fld_timer_s, sizeof(fld_timer_s))) { dirty_timer = true; displayDirty = true; } }
static void onDd3(const char*, const char* v)    { if (!isMissionRunning()) return; if (v && fld_dd_3[0] != v[0]) { fld_dd_3[0] = v[0]; fld_dd_3[1] = '\0'; dirty_timer = true; displayDirty = true; } }
static void onDd4(const char*, const char* v)    { if (!isMissionRunning()) return; if (v && fld_dd_4[0] != v[0]) { fld_dd_4[0] = v[0]; fld_dd_4[1] = '\0'; dirty_timer = true; displayDirty = true; } }

// =============================================================================
// DCS-BIOS CALLBACKS — Texture visibility (labels on/off)
// =============================================================================
static void onTexRpm(const char*, const char* v)   { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_rpm)   { tex_rpm   = nv; dirty_tex_rpm   = true; displayDirty = true; } }
static void onTexTemp(const char*, const char* v)  { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_temp)  { tex_temp  = nv; dirty_tex_temp  = true; displayDirty = true; } }
static void onTexFf(const char*, const char* v)    { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_ff)    { tex_ff    = nv; dirty_tex_ff    = true; displayDirty = true; } }
static void onTexOil(const char*, const char* v)   { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_oil)   { tex_oil   = nv; dirty_tex_oil   = true; displayDirty = true; } }
static void onTexNoz(const char*, const char* v)   { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_noz)   { tex_noz   = nv; dirty_tex_noz   = true; nozzleDirty = true; displayDirty = true; } }
static void onTexBingo(const char*, const char* v) { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_bingo) { tex_bingo = nv; dirty_tex_bingo = true; displayDirty = true; } }
static void onTexZ(const char*, const char* v)     { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_z)     { tex_z     = nv; dirty_tex_z     = true; displayDirty = true; } }
static void onTexL(const char*, const char* v)     { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_l)     { tex_l     = nv; dirty_tex_l     = true; displayDirty = true; } }
static void onTexR(const char*, const char* v)     { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_r)     { tex_r     = nv; dirty_tex_r     = true; displayDirty = true; } }

// =============================================================================
// DCS-BIOS CALLBACKS — Nozzle textures (pointer, scale, number visibility)
// =============================================================================
static void onTexLPointer(const char*, const char* v) {
    if (!isMissionRunning()) return;
    bool nv = (v && v[0] == '1');
    if (nv != showLeftNozPointer) { showLeftNozPointer = nv; nozzleDirty = true; displayDirty = true; }
}
static void onTexRPointer(const char*, const char* v) {
    if (!isMissionRunning()) return;
    bool nv = (v && v[0] == '1');
    if (nv != showRightNozPointer) { showRightNozPointer = nv; nozzleDirty = true; displayDirty = true; }
}
static void onTexLScale(const char*, const char* v) { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_lscale) { tex_lscale = nv; nozzleDirty = true; displayDirty = true; } }
static void onTexRScale(const char*, const char* v) { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_rscale) { tex_rscale = nv; nozzleDirty = true; displayDirty = true; } }
static void onTexL0(const char*, const char* v)     { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_l0)     { tex_l0     = nv; nozzleDirty = true; displayDirty = true; } }
static void onTexL50(const char*, const char* v)    { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_l50)    { tex_l50    = nv; nozzleDirty = true; displayDirty = true; } }
static void onTexL100(const char*, const char* v)   { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_l100)   { tex_l100   = nv; nozzleDirty = true; displayDirty = true; } }
static void onTexR0(const char*, const char* v)     { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_r0)     { tex_r0     = nv; nozzleDirty = true; displayDirty = true; } }
static void onTexR50(const char*, const char* v)    { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_r50)    { tex_r50    = nv; nozzleDirty = true; displayDirty = true; } }
static void onTexR100(const char*, const char* v)   { if (!isMissionRunning()) return; bool nv = (v && v[0] == '1'); if (nv != tex_r100)   { tex_r100   = nv; nozzleDirty = true; displayDirty = true; } }

// =============================================================================
// DCS-BIOS CALLBACKS — Nozzle position (metadata — integer values)
// =============================================================================
static void onNozzlePosL(const char*, uint16_t value) { if (!isMissionRunning()) return; if (value != nozzlePosL) { nozzlePosL = value; nozzleDirty = true; displayDirty = true; } }
static void onNozzlePosR(const char*, uint16_t value) { if (!isMissionRunning()) return; if (value != nozzlePosR) { nozzlePosR = value; nozzleDirty = true; displayDirty = true; } }

// =============================================================================
// DCS-BIOS CALLBACKS — Color mode and brightness
// =============================================================================
// COCKKPIT_LIGHT_MODE_SW: 0=Day, 1=Night, 2=NVG
// NVG uses same green as Night on TFT (single backlight, color is pixel-rendered)
static void onLightModeChange(const char*, uint16_t value) {
    uint32_t newColor = (value == 0) ? COLOR_DAY : COLOR_NIGHT;
    if (newColor != ifeiColor) {
        ifeiColor = newColor;
        needsFullRedraw = true;
        displayDirty = true;
    }
}

// Brightness with deduplication — knob controls brightness in ALL modes
// (gold standard: onBackLightIntensityChange scales to 0-255 for every mode)
static uint8_t lastBrightnessVal = 0xFF;

static void onBrightnessChange(const char*, uint16_t value, uint16_t) {
    uint8_t brightness = (uint8_t)map(value, 0, 65535, 0, 255);

    // Deduplication: skip if unchanged
    if (brightness == lastBrightnessVal) return;
    lastBrightnessVal = brightness;

    tft.setBrightness(brightness);
}

// =============================================================================
// DRAWING: updateElement — generic sprite-based element redraw
// =============================================================================
static void updateElement(DisplayName idx) {
    DisplayElement& el = elems[idx];
    LGFX_Sprite* spr = el.sprite;

    spr->fillScreen(COLOR_BG);
    spr->setTextColor(drawColor);
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

    spr->fillScreen(COLOR_BG);
    spr->setTextColor(drawColor);

    // Build formatted clock string: "HH:MM:SS"
    // For clock (isTimer=false): pad single-digit hours with leading '0'
    // For timer (isTimer=true): no leading zero on hours
    char hBuf[4] = "";
    char mBuf[4] = "";
    char sBuf[4] = "";

    // Hours
    if (h[0] == '\0' || h[0] == ' ') {
        hBuf[0] = ' '; hBuf[1] = '\0';
    } else {
        int hVal = atoi(h);
        if (!isTimer && hVal >= 0 && hVal < 10)
            snprintf(hBuf, sizeof(hBuf), "0%d", hVal);
        else {
            strncpy(hBuf, h, sizeof(hBuf) - 1);
            hBuf[sizeof(hBuf) - 1] = '\0';
        }
    }

    // Minutes
    if (m[0] == '\0' || m[0] == ' ') {
        mBuf[0] = ' '; mBuf[1] = '\0';
    } else {
        int mVal = atoi(m);
        if (mVal >= 0 && mVal < 10)
            snprintf(mBuf, sizeof(mBuf), "0%d", mVal);
        else {
            strncpy(mBuf, m, sizeof(mBuf) - 1);
            mBuf[sizeof(mBuf) - 1] = '\0';
        }
    }

    // Seconds
    if (s[0] == '\0' || s[0] == ' ') {
        sBuf[0] = ' '; sBuf[1] = '\0';
    } else {
        int sVal = atoi(s);
        if (sVal >= 0 && sVal < 10)
            snprintf(sBuf, sizeof(sBuf), "0%d", sVal);
        else {
            strncpy(sBuf, s, sizeof(sBuf) - 1);
            sBuf[sizeof(sBuf) - 1] = '\0';
        }
    }

    // Render each segment with natural cursor flow (matches ashchan reference).
    // Hardcoded X positions are ONLY used as skip-to fallbacks when a field is blank.

    // Hours
    if (hBuf[0] != ' ') {
        int16_t hx = (strlen(hBuf) == 1) ? 29 : 1;
        spr->setCursor(hx, 1);
        spr->print(hBuf);
    } else {
        spr->setCursor(57, 1);
    }

    // Separator 1
    if (d1[0] != '\0' && d1[0] != ' ') spr->print(d1);
    else spr->setCursor(63, 1);

    // Minutes
    if (mBuf[0] != ' ') spr->print(mBuf);
    else spr->setCursor(119, 1);

    // Separator 2
    if (d2[0] != '\0' && d2[0] != ' ') spr->print(d2);
    else spr->setCursor(125, 1);

    // Seconds
    if (sBuf[0] != ' ') spr->print(sBuf);

    spr->pushSprite(el.posX, el.posY);
}

// =============================================================================
// NOZZLE: Pre-compute bar positions (called once at init)
// =============================================================================
// Computes the 11 bar indicator positions along the arc for each nozzle gauge.
// Each bar is a short line segment (inner to outer) at the angle for that %.
// This runs ONCE — all trig is front-loaded here, zero trig per frame.
static void precomputeNozzleBars() {
    float baseAngleRad = -M_PI / 2.0f;  // straight down from pivot

    for (int i = 0; i < NOZ_BARS; i++) {
        float pct = (float)(i * 10);

        // Left gauge (normal rotation)
        // 0%=top of arc (closed), 100%=bottom (open) — matches real IFEI
        {
            float angleDeg = (100.0f - pct) * NOZ_DEG_PER_PCT;
            float rad = baseAngleRad + (angleDeg * M_PI / 180.0f);
            float c = cosf(rad), s = sinf(rad);
            nozBarsL[i].x1 = (int16_t)(NOZL_PIVOT_X + NEEDLE_INNER * c);
            nozBarsL[i].y1 = (int16_t)(NOZL_PIVOT_Y - NEEDLE_INNER * s);
            nozBarsL[i].x2 = (int16_t)(NOZL_PIVOT_X + NEEDLE_OUTER * c);
            nozBarsL[i].y2 = (int16_t)(NOZL_PIVOT_Y - NEEDLE_OUTER * s);
            // Perpendicular unit vector for tapered needle (precomputed, zero trig per frame)
            nozBarsL[i].perpX = s;   // perp to (cos,-sin) is (sin,cos)
            nozBarsL[i].perpY = c;
        }

        // Right gauge (inverted rotation — exact mirror of left)
        {
            float angleDeg = -(100.0f - pct) * NOZ_DEG_PER_PCT;
            float rad = baseAngleRad + (angleDeg * M_PI / 180.0f);
            float c = cosf(rad), s = sinf(rad);
            nozBarsR[i].x1 = (int16_t)(NOZR_PIVOT_X + NEEDLE_INNER * c);
            nozBarsR[i].y1 = (int16_t)(NOZR_PIVOT_Y - NEEDLE_INNER * s);
            nozBarsR[i].x2 = (int16_t)(NOZR_PIVOT_X + NEEDLE_OUTER * c);
            nozBarsR[i].y2 = (int16_t)(NOZR_PIVOT_Y - NEEDLE_OUTER * s);
            nozBarsR[i].perpX = s;
            nozBarsR[i].perpY = c;
        }
    }
}

// =============================================================================
// NOZZLE: Draw tick marks onto a target sprite (used by rebuildNozStaticBand)
// =============================================================================
// Draws the scale arc tick marks at the given pivot position.
// Does NOT fillScreen — caller is responsible for clearing first.
static void drawNozzleTicks(LGFX_Sprite* target, float pivotX, float pivotY,
                             bool invert, bool showScale, uint32_t color) {
    if (!showScale) return;
    float baseAngleRad = -M_PI / 2.0f;

    // Scale arc: exactly 11 tick marks (every 10%), longer at 0/50/100
    // 0%=top of arc (closed), 100%=bottom (open) — matches real F/A-18C IFEI
    for (int i = 0; i <= 100; i += 10) {
        float a = (100.0f - (float)i) * NOZ_DEG_PER_PCT;
        if (invert) a = -a;
        float rad = baseAngleRad + (a * M_PI / 180.0f);

        float innerR = SCALE_RADIUS - 5.0f;
        float outerR = SCALE_RADIUS;
        if (i == 0 || i == 50 || i == 100) innerR = SCALE_RADIUS - 10.0f;

        float x1 = pivotX + innerR * cosf(rad);
        float y1 = pivotY - innerR * sinf(rad);
        float x2 = pivotX + outerR * cosf(rad);
        float y2 = pivotY - outerR * sinf(rad);
        target->drawLine((int16_t)x1, (int16_t)y1, (int16_t)x2, (int16_t)y2, color);
    }
}

// =============================================================================
// NOZZLE: Draw all number labels on a target sprite (both gauges)
// =============================================================================
// Labels (0, 50, 100) are drawn on the wide 308px sprite to avoid clipping.
// Font must already be loaded on the target sprite (done once in createSprites).
static void drawAllNozzleLabels(LGFX_Sprite* target, uint32_t color,
                                 bool showL0, bool showL50, bool showL100,
                                 bool showR0, bool showR50, bool showR100) {
    float baseAngleRad = -M_PI / 2.0f;
    // 18px past tick outer edge for 50/100 labels — visible gap from ticks
    // 4px for the 0% label — keeps it close to its own gauge, avoids center overlap
    static constexpr float LABEL_R      = SCALE_RADIUS + 18.0f;
    static constexpr float LABEL_R_ZERO = SCALE_RADIUS + 4.0f;

    target->setTextColor(color);
    target->setTextSize(1);

    auto drawLabel = [&](float pivotX, float pivotY, bool invert, int val, bool visible) {
        if (!visible) return;
        float a = (100.0f - (float)val) * NOZ_DEG_PER_PCT;
        if (invert) a = -a;
        float rad = baseAngleRad + (a * M_PI / 180.0f);
        float r = (val == 0) ? LABEL_R_ZERO : LABEL_R;
        float x = pivotX + r * cosf(rad);
        float y = pivotY - r * sinf(rad);
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", val);
        int16_t tw = target->textWidth(buf);
        int16_t th = target->fontHeight();
        target->setCursor((int16_t)(x - tw / 2), (int16_t)(y - th / 2));
        target->print(buf);
    };

    // Left gauge labels (pivot in band-space = same as bg sprite coords)
    drawLabel(NOZL_PIVOT_X, NOZL_PIVOT_Y, false, 0,   showL0);
    drawLabel(NOZL_PIVOT_X, NOZL_PIVOT_Y, false, 50,  showL50);
    drawLabel(NOZL_PIVOT_X, NOZL_PIVOT_Y, false, 100, showL100);

    // Right gauge labels (pivot offset by 158px — right gauge drawn at x=158)
    float rPivX = 158.0f + NOZR_PIVOT_X;
    float rPivY = NOZR_PIVOT_Y;
    drawLabel(rPivX, rPivY, true, 0,   showR0);
    drawLabel(rPivX, rPivY, true, 50,  showR50);
    drawLabel(rPivX, rPivY, true, 100, showR100);
}

// =============================================================================
// NOZZLE: Tapered needle drawing (matches real F/A-18C IFEI pointer shape)
// =============================================================================
// Draws a wedge: narrow at the base (inner, near pivot), wide at the tip (outer).
// Uses precomputed perpendicular vectors — zero trig per frame.
static void drawTaperedNeedle(LGFX_Sprite* spr, const NozzleBarPos& b,
                               int16_t offsetX, uint32_t color) {
    float px = b.perpX, py = b.perpY;
    int16_t x = offsetX;
    // Inner edge corners (narrow base)
    int16_t i1x = x + b.x1 + (int16_t)(px * NEEDLE_HW_INNER);
    int16_t i1y =     b.y1 + (int16_t)(py * NEEDLE_HW_INNER);
    int16_t i2x = x + b.x1 - (int16_t)(px * NEEDLE_HW_INNER);
    int16_t i2y =     b.y1 - (int16_t)(py * NEEDLE_HW_INNER);
    // Outer edge corners (wide tip)
    int16_t o1x = x + b.x2 + (int16_t)(px * NEEDLE_HW_OUTER);
    int16_t o1y =     b.y2 + (int16_t)(py * NEEDLE_HW_OUTER);
    int16_t o2x = x + b.x2 - (int16_t)(px * NEEDLE_HW_OUTER);
    int16_t o2y =     b.y2 - (int16_t)(py * NEEDLE_HW_OUTER);
    // Two triangles forming the tapered quad
    spr->fillTriangle(i1x, i1y, o1x, o1y, o2x, o2y, color);
    spr->fillTriangle(i1x, i1y, i2x, i2y, o2x, o2y, color);
}

// =============================================================================
// NOZZLE: Rebuild cached static background (ticks + labels + NOZ text)
// =============================================================================
// Called only on init, texture changes, or color changes. NOT per frame.
// Draws directly on nozStaticBand — no intermediate sprites needed.
static void rebuildNozStaticBand(uint32_t color) {
    nozStaticBand.fillScreen(COLOR_BG);

    // Draw tick marks directly on the 308-wide static band
    // Left gauge: pivot at original position (band x=0 corresponds to left gauge)
    drawNozzleTicks(&nozStaticBand, NOZL_PIVOT_X, NOZL_PIVOT_Y,
                     false, tex_lscale, color);
    // Right gauge: pivot offset by 158px (right gauge drawn at x=158 in band)
    drawNozzleTicks(&nozStaticBand, 158.0f + NOZR_PIVOT_X, NOZR_PIVOT_Y,
                     true, tex_rscale, color);

    // Draw number labels directly on static band (font already loaded in createSprites)
    drawAllNozzleLabels(&nozStaticBand, color,
                         tex_l0, tex_l50, tex_l100,
                         tex_r0, tex_r50, tex_r100);

    // NOZ label — draw directly on static band via labelSprite
    if (tex_noz) {
        int16_t nozRelX = elems[NOZT].posX - NOZ_BAND_X;
        int16_t nozRelY = elems[NOZT].posY - NOZ_BAND_Y;
        labelSprite.fillScreen(COLOR_BG);
        labelSprite.setTextColor(color);
        int16_t cx = alignCursorX(&labelSprite, "NOZ", elems[NOZT].spriteWidth, ALIGN_CENTER);
        labelSprite.setCursor(cx, 0);
        labelSprite.print("NOZ");
        labelSprite.pushSprite(&nozStaticBand, nozRelX, nozRelY, COLOR_BG);
    }

    nozStaticDirty = false;
}

// =============================================================================
// NOZZLE: Render nozzles (called per frame when nozzleDirty)
// =============================================================================
// Fast path: copy cached static background, draw needles, push to TFT.
// Eliminates per-frame trig, font loading, and redundant PSRAM copies.
static void renderNozzles() {
    // Convert raw 0-65535 to bar index 0-10 (same formula as IFEIPanel)
    int percentL = ((int)nozzlePosL * 100 + 32767) / 65535;
    if (percentL > 100) percentL = 100;
    if (percentL < 0) percentL = 0;
    int barIdxL = constrain((percentL + 5) / 10, 0, NOZ_BARS - 1);

    int percentR = ((int)nozzlePosR * 100 + 32767) / 65535;
    if (percentR > 100) percentR = 100;
    if (percentR < 0) percentR = 0;
    int barIdxR = constrain((percentR + 5) / 10, 0, NOZ_BARS - 1);

    // Check if static background needs rebuild (texture or color change)
    // Tracked locally — no need to modify callbacks
    static bool cached_lscale = false, cached_rscale = false;
    static bool cached_l0 = false, cached_l50 = false, cached_l100 = false;
    static bool cached_r0 = false, cached_r50 = false, cached_r100 = false;
    static bool cached_noz = false;
    static uint32_t cachedColor = 0;

    if (nozStaticDirty
        || drawColor    != cachedColor
        || tex_lscale   != cached_lscale  || tex_rscale != cached_rscale
        || tex_l0       != cached_l0      || tex_l50    != cached_l50
        || tex_l100     != cached_l100
        || tex_r0       != cached_r0      || tex_r50    != cached_r50
        || tex_r100     != cached_r100
        || tex_noz      != cached_noz) {

        rebuildNozStaticBand(drawColor);

        cachedColor   = drawColor;
        cached_lscale = tex_lscale;  cached_rscale = tex_rscale;
        cached_l0     = tex_l0;      cached_l50    = tex_l50;
        cached_l100   = tex_l100;
        cached_r0     = tex_r0;      cached_r50    = tex_r50;
        cached_r100   = tex_r100;
        cached_noz    = tex_noz;
    }

    // Fast per-frame path: stamp cached background onto working band
    nozStaticBand.pushSprite(&nozzleBand, 0, 0);

    // Draw active bar indicator — tapered needle (narrow base, wide tip)
    if (showLeftNozPointer) {
        drawTaperedNeedle(&nozzleBand, nozBarsL[barIdxL], 0, drawColor);
    }
    if (showRightNozPointer) {
        drawTaperedNeedle(&nozzleBand, nozBarsR[barIdxR], 158, drawColor);
    }

    // Push entire nozzle band to TFT in one operation
    nozzleBand.pushSprite(NOZ_BAND_X, NOZ_BAND_Y);

    lastBarIdxL = barIdxL;
    lastBarIdxR = barIdxR;
}

// =============================================================================
// OVERLAY HELPERS — resolve what to actually display per field
// =============================================================================
// These implement the overlay priority logic from the state machine.
// Called during both full and incremental redraws.

static const char* resolveTempL() {
    // SP overrides TEMP_L when active; fuel_mode blanks TEMP entirely
    if (fuel_mode_active) return "";
    if (tempL_overlay && !isBlankString(fld_sp, 3)) return fld_sp;
    // Blank-hold: show cached base when raw field is blank (prevents flash)
    if (isBlankString(fld_temp_l, 3)) return tempL_base;
    return fld_temp_l;
}

static const char* resolveTempR() {
    if (fuel_mode_active) return "";
    if (tempR_overlay && !isBlankString(fld_codes, 3)) return fld_codes;
    // Blank-hold: show cached base when raw field is blank (prevents flash)
    if (isBlankString(fld_temp_r, 3)) return tempR_base;
    return fld_temp_r;
}

static const char* resolveFuelUp() {
    if (sp_active) return "";  // SP suppresses fuel display
    if (!isBlankString(fld_t, 6)) return fld_t;  // T overlay
    return fld_fuel_up;
}

static const char* resolveFuelDown() {
    if (sp_active) return "";  // SP suppresses fuel display
    if (fuel_mode_active) {
        // In T-mode: TIME_SET replaces FUEL_DOWN, or blank if TIME_SET is blank
        if (!isBlankString(fld_time_set, 6)) return fld_time_set;
        return "";
    }
    return fld_fuel_down;
}

// =============================================================================
// DRAWING: Full screen redraw (color change, mission start, init)
// =============================================================================
static void drawFullScreen() {
    // No tft.fillScreen() here — each element's sprite already fills its own
    // rectangle with COLOR_BG before drawing text. The gaps between elements are
    // already black from init or previous frames. Skipping the 768KB full-framebuffer
    // wipe eliminates the single heaviest PSRAM burst per full redraw.

    // RPM
    strcpy(elems[RPML].value, fld_rpm_l);   updateElement(RPML);
    strcpy(elems[RPMR].value, fld_rpm_r);   updateElement(RPMR);
    strcpy(elems[RPMT].value, tex_rpm ? "RPM" : "");  updateElement(RPMT);

    // TEMP (with overlay resolution)
    strcpy(elems[TMPL].value, resolveTempL());  updateElement(TMPL);
    strcpy(elems[TMPR].value, resolveTempR());  updateElement(TMPR);
    strcpy(elems[TMPT].value, tex_temp ? "TEMP" : "");  updateElement(TMPT);

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
#if !IFEI_DEBUG_NO_NOZZLES
    renderNozzles();
#endif

    // FUEL (with overlay resolution)
    strcpy(elems[FUELU].value, resolveFuelUp());   updateElement(FUELU);
    strcpy(elems[FUELL].value, resolveFuelDown());  updateElement(FUELL);

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

    // Clear all dirty flags — prevents redundant redraws on next incremental pass
    dirty_rpm_l = false;  dirty_rpm_r = false;
    dirty_temp_l = false; dirty_temp_r = false;
    dirty_ff_l = false;   dirty_ff_r = false;
    dirty_oil_l = false;  dirty_oil_r = false;
    dirty_fuel_up = false; dirty_fuel_down = false;
    dirty_bingo = false;
    dirty_sp = false;     dirty_codes = false;
    dirty_t = false;      dirty_time_set = false;
    dirty_clock = false;  dirty_timer = false;
    nozzleDirty = false;  dirty_tex_noz = false;
    dirty_tex_rpm = false;  dirty_tex_temp = false;
    dirty_tex_ff = false;   dirty_tex_oil = false;
    dirty_tex_bingo = false;
    dirty_tex_z = false;  dirty_tex_l = false;  dirty_tex_r = false;
}

// =============================================================================
// DRAWING: Incremental redraw (dirty fields only)
// =============================================================================
static void drawDirtyFields() {
    // RPM data
    if (dirty_rpm_l) { strcpy(elems[RPML].value, fld_rpm_l); updateElement(RPML); dirty_rpm_l = false; }
    if (dirty_rpm_r) { strcpy(elems[RPMR].value, fld_rpm_r); updateElement(RPMR); dirty_rpm_r = false; }

    // TEMP with overlay resolution
    if (dirty_temp_l || dirty_sp) {
        strcpy(elems[TMPL].value, resolveTempL()); updateElement(TMPL);
        dirty_temp_l = false; dirty_sp = false;
    }
    if (dirty_temp_r || dirty_codes) {
        strcpy(elems[TMPR].value, resolveTempR()); updateElement(TMPR);
        dirty_temp_r = false; dirty_codes = false;
    }

    // FF data
    if (dirty_ff_l) { strcpy(elems[FFL].value, fld_ff_l); updateElement(FFL); dirty_ff_l = false; }
    if (dirty_ff_r) { strcpy(elems[FFR].value, fld_ff_r); updateElement(FFR); dirty_ff_r = false; }

    // OIL data
    if (dirty_oil_l) { strcpy(elems[OILL].value, fld_oil_l); updateElement(OILL); dirty_oil_l = false; }
    if (dirty_oil_r) { strcpy(elems[OILR].value, fld_oil_r); updateElement(OILR); dirty_oil_r = false; }

    // FUEL with overlay resolution
    if (dirty_fuel_up || dirty_t) {
        strcpy(elems[FUELU].value, resolveFuelUp()); updateElement(FUELU);
        dirty_fuel_up = false; dirty_t = false;
    }
    if (dirty_fuel_down || dirty_time_set) {
        strcpy(elems[FUELL].value, resolveFuelDown()); updateElement(FUELL);
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
#if !IFEI_DEBUG_NO_NOZZLES
    if (nozzleDirty) {
        renderNozzles();
        nozzleDirty = false;
        dirty_tex_noz = false;
    }
#else
    nozzleDirty = false;
    dirty_tex_noz = false;
#endif

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
// BLANK ALL FIELDS — clean reset between missions (no stale data survives)
// =============================================================================
// Ported from IFEIPanel blankBuffersAndDirty(): clears every field buffer to
// empty, resets all state machine flags, and marks everything dirty so the
// next render cycle starts from a known-clean state.
static void blankAllFields() {
    // Engine data
    fld_rpm_l[0] = '\0';  fld_rpm_r[0] = '\0';
    fld_temp_l[0] = '\0'; fld_temp_r[0] = '\0';
    fld_ff_l[0] = '\0';   fld_ff_r[0] = '\0';
    fld_oil_l[0] = '\0';  fld_oil_r[0] = '\0';

    // Fuel
    fld_fuel_up[0] = '\0'; fld_fuel_down[0] = '\0'; fld_bingo[0] = '\0';

    // Overlays
    fld_sp[0] = '\0'; fld_codes[0] = '\0';
    fld_t[0] = '\0'; fld_time_set[0] = '\0';

    // Base caches
    tempL_base[0] = '\0'; tempR_base[0] = '\0';
    fuelUp_base[0] = '\0'; fuelDn_base[0] = '\0';

    // Clock/timer
    fld_clock_h[0] = '\0'; fld_clock_m[0] = '\0'; fld_clock_s[0] = '\0';
    fld_dd_1[0] = '\0'; fld_dd_2[0] = '\0';
    fld_timer_h[0] = '\0'; fld_timer_m[0] = '\0'; fld_timer_s[0] = '\0';
    fld_dd_3[0] = '\0'; fld_dd_4[0] = '\0';

    // Nozzle state
    nozzlePosL = 0; nozzlePosR = 0;
    lastBarIdxL = -1; lastBarIdxR = -1;
    showLeftNozPointer = false; showRightNozPointer = false;
    tex_lscale = false; tex_rscale = false;
    tex_l0 = false; tex_l50 = false; tex_l100 = false;
    tex_r0 = false; tex_r50 = false; tex_r100 = false;

    // Overlay state machine
    sp_active = false; fuel_mode_active = false; lastTWasBlank = false;
    tempL_overlay = false; tempR_overlay = false; fuelUp_overlay = false;

    // Texture visibility
    tex_rpm = false; tex_temp = false; tex_ff = false;
    tex_oil = false; tex_noz = false; tex_bingo = false;
    tex_z = false; tex_l = false; tex_r = false;

    // Mark everything dirty for next render
    dirty_rpm_l = true; dirty_rpm_r = true;
    dirty_temp_l = true; dirty_temp_r = true;
    dirty_ff_l = true; dirty_ff_r = true;
    dirty_oil_l = true; dirty_oil_r = true;
    dirty_fuel_up = true; dirty_fuel_down = true;
    dirty_bingo = true;
    dirty_sp = true; dirty_codes = true;
    dirty_t = true; dirty_time_set = true;
    dirty_clock = true; dirty_timer = true;
    nozzleDirty = true;
    nozStaticDirty = true;  // force nozzle background rebuild on next render
    dirty_tex_rpm = true; dirty_tex_temp = true; dirty_tex_ff = true;
    dirty_tex_oil = true; dirty_tex_noz = true; dirty_tex_bingo = true;
    dirty_tex_z = true; dirty_tex_l = true; dirty_tex_r = true;

    // Brightness dedup reset
    lastBrightnessVal = 0xFF;

    // Bust the DCS-BIOS bridge's change-detection cache (Layer 1 dedup).
    // RegisteredDisplayBuffer.last points to the SAME memory as
    // CT_DisplayBuffers[i].last — setting it to 0xFF (impossible ASCII)
    // guarantees strncmp(buffer, last) will see a difference on the next
    // frame, forcing onDisplayChange() to fire for ALL IFEI fields.
    // Without this, values that haven't changed (like colons) would never
    // trigger a callback after blanking, because buffer == last == ":".
    for (size_t i = 0; i < numCTDisplayBuffers; ++i) {
        DisplayBufferEntry& b = CT_DisplayBuffers[i];
        if (!b.label || strncmp(b.label, "IFEI_", 5) != 0) continue;
        memset(b.last, 0xFF, b.length);
        b.last[b.length] = '\0';
    }

    debugPrintln("  TFT IFEI: All fields blanked, bridge dedup cache busted");
}

// =============================================================================
// SPRITE CREATION
// =============================================================================
static void createSprites() {
    // --- Text sprites: internal SRAM + 16-bit (RGB565) ---
    // 16-bit depth minimizes PSRAM write volume per pushSprite (2 bytes/pixel vs 3).
    // LovyanGFX converts RGB888 color constants to RGB565 internally.

    // Two-digit data sprite (RPM L/R, OIL L/R)
    twoDigitSprite.setPsram(false);
    twoDigitSprite.setColorDepth(16);
    twoDigitSprite.createSprite(76, 38);
    twoDigitSprite.loadFont(IFEI_Data_36);
    twoDigitSprite.setTextWrap(false);
    twoDigitSprite.setTextColor(ifeiColor);

    // Three-digit data sprite (TEMP L/R, FF L/R, OIL L/R)
    threeDigitSprite.setPsram(false);
    threeDigitSprite.setColorDepth(16);
    threeDigitSprite.createSprite(108, 38);
    threeDigitSprite.loadFont(IFEI_Data_36);
    threeDigitSprite.setTextWrap(false);
    threeDigitSprite.setTextColor(ifeiColor);

    // Fuel sprite (FUEL UP/DOWN, BINGO)
    fuelSprite.setPsram(false);
    fuelSprite.setColorDepth(16);
    fuelSprite.createSprite(220, 38);
    fuelSprite.loadFont(IFEI_Data_36);
    fuelSprite.setTextWrap(false);
    fuelSprite.setTextColor(ifeiColor);

    // Clock sprite (CLOCK UP/DOWN)
    clockSprite.setPsram(false);
    clockSprite.setColorDepth(16);
    clockSprite.createSprite(176, 35);
    clockSprite.loadFont(IFEI_Data_32);
    clockSprite.setTextWrap(false);
    clockSprite.setTextColor(ifeiColor);

    // Label sprite (all text labels)
    labelSprite.setPsram(false);
    labelSprite.setColorDepth(16);
    labelSprite.createSprite(65, 18);
    labelSprite.loadFont(IFEI_Labels_16);
    labelSprite.setTextColor(ifeiColor);

    // Tag sprite (ZULU, L, R)
    tagSprite.setPsram(false);
    tagSprite.setColorDepth(16);
    tagSprite.createSprite(18, 18);
    tagSprite.loadFont(IFEI_Labels_16);
    tagSprite.setTextColor(ifeiColor);

#if !IFEI_DEBUG_NO_NOZZLES
    // --- Nozzle sprites: PSRAM + 16-bit ---
    // nozStaticBand: cached background (ticks + labels + NOZ text) — rebuilt only on texture/color change
    // nozzleBand: working buffer — stamped from cache + needles drawn per frame
    nozStaticBand.setPsram(true);
    nozStaticBand.setColorDepth(16);
    nozStaticBand.createSprite(NOZ_BAND_W, NOZ_BAND_H);
    nozStaticBand.loadFont(IFEI_Labels_16);  // keep loaded permanently — no per-frame flash access

    nozzleBand.setPsram(true);
    nozzleBand.setColorDepth(16);
    nozzleBand.createSprite(NOZ_BAND_W, NOZ_BAND_H);
    nozzleBand.loadFont(IFEI_Labels_16);  // needed for lampTest label rendering

    debugPrintln("  Sprites created (6 text SRAM/16-bit, 2 nozzle PSRAM/16-bit)");
#else
    debugPrintln("  Sprites created (6 text SRAM/16-bit, nozzles DISABLED)");
#endif

    debugPrintf("  Internal SRAM free: %u bytes\n", (unsigned)ESP.getFreeHeap());
}

// =============================================================================
// FREERTOS DISPLAY TASK (optional — set RUN_TFT_IFEI_DISPLAY_AS_TASK to 1)
// =============================================================================
// Runs rendering on a dedicated task at constant TFT_IFEI_DISPLAY_REFRESH_RATE_HZ.
// Frees the main loop entirely — display gets its own core on the dual-core S3.
// When disabled (0), rendering falls through to TFTIFEIDisplay_loop() instead.
#if RUN_TFT_IFEI_DISPLAY_AS_TASK
static void TFTIFEIDisplayTask(void* pv) {
    const TickType_t tickDelay = pdMS_TO_TICKS(1000 / TFT_IFEI_DISPLAY_REFRESH_RATE_HZ);
    while (true) {

        if (displayDirty) {
            drawColor = ifeiColor;  // snapshot volatile color once per frame

            #if DEBUG_PERFORMANCE
            beginProfiling(PERF_DISPLAY_RENDER);
            #endif

            if (needsFullRedraw) {
                drawFullScreen();
                needsFullRedraw = false;
            } else {
                drawDirtyFields();
            }

            #if DEBUG_PERFORMANCE
            endProfiling(PERF_DISPLAY_RENDER);
            #endif

            displayDirty = false;
        }

        vTaskDelay(tickDelay);
    }
}
#endif

// =============================================================================
// LAMP TEST (visual hardware verification at init)
// =============================================================================
// Mimics the 7-segment gold standard showLampTest(): fills ALL digit positions
// with their "all segments on" representation.  For numeric fields this is '8'
// (all 7 segments lit); for text-label fields the actual label text is shown
// (e.g. RPM, TEMP, FF, etc.).  Cycles Day -> NVG colors, then clears.
//
// Runs AFTER createSprites() — sprites are already allocated and have fonts
// loaded, so we can reuse the normal updateElement()/renderClock() paths.

static void lampTestPhase(uint32_t color) {
    // Fill every element with its "all-on" representation
    // Numeric 2-digit fields -> "88"
    strcpy(elems[RPML].value,  "88");
    strcpy(elems[RPMR].value,  "88");

    // Numeric 3-digit fields -> "888"
    strcpy(elems[TMPL].value,  "888");
    strcpy(elems[TMPR].value,  "888");
    strcpy(elems[FFL].value,   "888");
    strcpy(elems[FFR].value,   "888");
    strcpy(elems[OILL].value,  "888");
    strcpy(elems[OILR].value,  "888");

    // Fuel 6-char fields -> "888888"
    strcpy(elems[FUELU].value, "888888");
    strcpy(elems[FUELL].value, "888888");

    // Bingo -> "88888"
    strcpy(elems[BINGO_VAL].value, "88888");

    // Labels show their text (this IS the full 7-seg letter representation)
    strcpy(elems[RPMT].value,  "RPM");
    strcpy(elems[TMPT].value,  "TEMP");
    strcpy(elems[FFTU].value,  "FF");
    strcpy(elems[FFTL].value,  "X100");
    strcpy(elems[OILT].value,  "OIL");
    strcpy(elems[NOZT].value,  "NOZ");
    strcpy(elems[BINGOT].value,"BINGO");
    strcpy(elems[ZULU].value,  "Z");
    strcpy(elems[L_TAG].value, "L");
    strcpy(elems[R_TAG].value, "R");

    // Set draw color and render all elements
    drawColor = color;
    tft.fillScreen(COLOR_BG);

    for (int i = 0; i < NUM_ELEMENTS; i++) {
        if (i == CLOCKU || i == CLOCKL) continue;  // handled separately below
        if (i == NOZT) continue;                     // rendered inside nozzle band
        updateElement((DisplayName)i);
    }

    // Clock / timer -> "88:88:88"
    renderClock(CLOCKU, "88", ":", "88", ":", "88", false);
    renderClock(CLOCKL, "88", ":", "88", ":", "88", true);

    // Nozzle gauges — show all 11 positions with full scale
#if !IFEI_DEBUG_NO_NOZZLES
    {
        // Build lamp test nozzle directly on nozzleBand (one-time, no caching needed)
        nozzleBand.fillScreen(COLOR_BG);

        // Ticks — draw directly on band with correct offsets
        drawNozzleTicks(&nozzleBand, NOZL_PIVOT_X, NOZL_PIVOT_Y,
                         false, true, color);
        drawNozzleTicks(&nozzleBand, 158.0f + NOZR_PIVOT_X, NOZR_PIVOT_Y,
                         true, true, color);

        // Labels — all visible for lamp test
        drawAllNozzleLabels(&nozzleBand, color,
                             true, true, true,
                             true, true, true);

        // Show ALL 11 bar positions (0%-100%) for both — full arc verification
        for (int i = 0; i < NOZ_BARS; i++) {
            drawTaperedNeedle(&nozzleBand, nozBarsL[i], 0, color);
            drawTaperedNeedle(&nozzleBand, nozBarsR[i], 158, color);
        }

        // NOZ label in band
        labelSprite.fillScreen(COLOR_BG);
        labelSprite.setTextColor(color);
        int16_t cx = alignCursorX(&labelSprite, "NOZ", elems[NOZT].spriteWidth, ALIGN_CENTER);
        labelSprite.setCursor(cx, 0);
        labelSprite.print("NOZ");
        int16_t nozRelX = elems[NOZT].posX - NOZ_BAND_X;
        int16_t nozRelY = elems[NOZT].posY - NOZ_BAND_Y;
        labelSprite.pushSprite(&nozzleBand, nozRelX, nozRelY, COLOR_BG);

        nozzleBand.pushSprite(NOZ_BAND_X, NOZ_BAND_Y);
    }
#endif
}

static void lampTest() {
    tft.setBrightness(255);

    // Phase 1: Day (white) — all digits ON
    lampTestPhase(COLOR_DAY);
    delay(1000);

    // Phase 2: NVG (green) — all digits ON
    lampTestPhase(COLOR_NIGHT);
    delay(1000);

    // Phase 3: all OFF
    tft.fillScreen(COLOR_BG);
    tft.setBrightness(150);

    debugPrintln("  Lamp test complete (Day -> NVG -> off)");
}

// =============================================================================
// INIT
// =============================================================================
void TFTIFEIDisplay_init() {
    debugPrintln("TFT IFEI: Initializing...");

    ch422g_init();
    delay(50);

    tft.init();
    tft.fillScreen(COLOR_BG);

    tft.setBrightness(150);

    debugPrintln("  TFT initialized (800x480 RGB parallel)");

    createSprites();

#if !IFEI_DEBUG_NO_NOZZLES
    // Pre-compute nozzle bar positions (all trig done here, zero per frame)
    precomputeNozzleBars();
    debugPrintln("  Nozzle bar positions pre-computed (11 bars x 2 gauges)");
#endif

    // Lamp test: visual hardware check — all digits on, cycle Day -> NVG -> off
    lampTest();

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

#if !IFEI_DEBUG_NO_NOZZLES
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
#endif

    // Color mode and brightness
    subscribeToSelectorChange("COCKKPIT_LIGHT_MODE_SW", onLightModeChange);
    subscribeToLedChange("IFEI_DISP_INT_LT", onBrightnessChange);

    // Initial full draw
    drawColor = ifeiColor;
    drawFullScreen();
    needsFullRedraw = false;

    #if RUN_TFT_IFEI_DISPLAY_AS_TASK
    // Create FreeRTOS task for display rendering (8KB stack — TFT sprites need more than HT1622 SPI)
    // Pinned to ARDUINO_RUNNING_CORE (core 1 on dual-core S3, core 0 on single-core S2).
    // Unpinned xTaskCreate could schedule rendering on core 0, competing with WiFi
    // for CPU time during heavy sprite-to-PSRAM pushes.
    xTaskCreatePinnedToCore(TFTIFEIDisplayTask, "TFTIFEIDisp", 8192, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
    debugPrintln("  FreeRTOS display task created (30 Hz, pri 2, app core)");
    #endif

    debugPrintln("TFT IFEI: Initialized successfully");
    debugPrintf("  PSRAM free: %u bytes\n", (unsigned)ESP.getFreePsram());
}

// =============================================================================
// LOOP
// =============================================================================
void TFTIFEIDisplay_loop() {
#if IFEI_DEBUG_NO_LOOP_RENDER
    // Troubleshooting: all loop rendering disabled — screen stays static after init.
    // If shaking persists here, the issue is RGB bus timing, not rendering.
    displayDirty = false;
    return;
#endif

    #if !RUN_TFT_IFEI_DISPLAY_AS_TASK

    if (!displayDirty) return;

    const unsigned long now = millis();
    if (now - lastDrawTime < REFRESH_INTERVAL_MS) return;
    lastDrawTime = now;

    drawColor = ifeiColor;  // snapshot volatile color once per frame

    #if DEBUG_PERFORMANCE
    beginProfiling(PERF_DISPLAY_RENDER);
    #endif

    if (needsFullRedraw) {
        drawFullScreen();
        needsFullRedraw = false;
    } else {
        drawDirtyFields();
    }

    #if DEBUG_PERFORMANCE
    endProfiling(PERF_DISPLAY_RENDER);
    #endif

    displayDirty = false;

    #endif // RUN_TFT_IFEI_DISPLAY_AS_TASK
}

// =============================================================================
// MISSION START / STOP NOTIFICATIONS
// =============================================================================
void TFTIFEIDisplay_notifyMissionStart() {
    blankAllFields();           // Clear stale data from previous mission
    needsFullRedraw = true;
    displayDirty = true;
}

void TFTIFEIDisplay_notifyMissionStop() {
    // Clean reset: blank all fields and display so no stale data from the
    // previous mission survives into the next one.
    blankAllFields();
    drawColor = ifeiColor;
    drawFullScreen();
    needsFullRedraw = false;
    displayDirty = false;
}

#endif // HAS_TFT_IFEI && ENABLE_TFT_GAUGES
