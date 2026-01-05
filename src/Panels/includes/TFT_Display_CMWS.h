// =============================================================================
// TFT_Display_CMWS.h — CockpitOS CMWS Threat Ring Display Header
// =============================================================================
// AH-64D Apache Countermeasures Warning System
//
// This header provides:
//   1. Color constants (RGB565 format)
//   2. Pre-computed trig lookup tables (15° increments)
//   3. Geometry helper types (TFT_Point)
//   4. API function declarations
//
// Design: Self-contained header with no external dependencies beyond std libs.
// =============================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>
#include <algorithm>

// =============================================================================
// COLOR CONSTANTS — RGB565 Format
// =============================================================================
// RGB565 packs 16 bits: 5 red, 6 green, 5 blue (RRRRRGGGGGGBBBBB)
// Used directly by LovyanGFX for fast pixel operations.

namespace TFT_Colors {
    static constexpr uint16_t BLACK         = 0x0000;  // (0, 0, 0)
    static constexpr uint16_t WHITE         = 0xFFFF;  // (255, 255, 255)
    static constexpr uint16_t RED           = 0xF800;  // (255, 0, 0)
    static constexpr uint16_t GREEN         = 0x07E0;  // (0, 255, 0)
    static constexpr uint16_t BLUE          = 0x001F;  // (0, 0, 255)
    static constexpr uint16_t AMBER_BRT     = 0xFDE0;  // Bright amber for active elements
    static constexpr uint16_t AMBER_DIM     = 0x8400;  // Dim amber for inactive elements
    static constexpr uint16_t TRANSPARENT   = 0x2001;  // Transparency key (arbitrary non-UI color)
}

// =============================================================================
// GEOMETRY TYPES
// =============================================================================

/**
 * @brief 2D point in screen coordinates
 * 
 * Used for caching pre-computed geometry (arrow vertices, tick endpoints).
 * Screen coordinate system: origin at top-left, X increases right, Y increases down.
 */
struct TFT_Point {
    int16_t x;  // X coordinate in pixels
    int16_t y;  // Y coordinate in pixels
};

// =============================================================================
// PRE-COMPUTED TRIGONOMETRY LOOKUP — 15° Increments
// =============================================================================
//
// Purpose: Eliminate runtime sinf()/cosf() calls for common angles.
//
// Coverage: 24 entries covering 0°, 15°, 30°, ... 345°
// Access: TFT_Trig::fastSin15(angle) / TFT_Trig::fastCos15(angle)
//
// For angles that aren't multiples of 15°, the .cpp falls back to sinf/cosf.
// But for CMWS, all arrows are at 0°, 45°, 90°, etc. — all covered by this table.
//
// Memory: 24 × 4 bytes × 2 tables = 192 bytes (constexpr, stored in flash)

namespace TFT_Trig {
    // Sin lookup: SIN_TABLE[i] = sin(i * 15°)
    static constexpr float SIN_TABLE[24] = {
        0.0f,           // 0°   — forward (up)
        0.258819045f,   // 15°
        0.5f,           // 30°
        0.707106781f,   // 45°  — diagonal
        0.866025404f,   // 60°
        0.965925826f,   // 75°
        1.0f,           // 90°  — right
        0.965925826f,   // 105°
        0.866025404f,   // 120°
        0.707106781f,   // 135° — diagonal
        0.5f,           // 150°
        0.258819045f,   // 165°
        0.0f,           // 180° — aft (down)
        -0.258819045f,  // 195°
        -0.5f,          // 210°
        -0.707106781f,  // 225° — diagonal
        -0.866025404f,  // 240°
        -0.965925826f,  // 255°
        -1.0f,          // 270° — left
        -0.965925826f,  // 285°
        -0.866025404f,  // 300°
        -0.707106781f,  // 315° — diagonal
        -0.5f,          // 330°
        -0.258819045f   // 345°
    };

    // Cos lookup: COS_TABLE[i] = cos(i * 15°)
    static constexpr float COS_TABLE[24] = {
        1.0f,           // 0°
        0.965925826f,   // 15°
        0.866025404f,   // 30°
        0.707106781f,   // 45°
        0.5f,           // 60°
        0.258819045f,   // 75°
        0.0f,           // 90°
        -0.258819045f,  // 105°
        -0.5f,          // 120°
        -0.707106781f,  // 135°
        -0.866025404f,  // 150°
        -0.965925826f,  // 165°
        -1.0f,          // 180°
        -0.965925826f,  // 195°
        -0.866025404f,  // 210°
        -0.707106781f,  // 225°
        -0.5f,          // 240°
        -0.258819045f,  // 255°
        0.0f,           // 270°
        0.258819045f,   // 285°
        0.5f,           // 300°
        0.707106781f,   // 315°
        0.866025404f,   // 330°
        0.965925826f    // 345°
    };

    /**
     * @brief Fast sine lookup for 15° multiples
     * @param angleDeg Angle in degrees (should be multiple of 15)
     * @return sin(angleDeg)
     */
    static inline float fastSin15(int angleDeg) {
        const int idx = (angleDeg / 15) % 24;
        return SIN_TABLE[idx];
    }

    /**
     * @brief Fast cosine lookup for 15° multiples
     * @param angleDeg Angle in degrees (should be multiple of 15)
     * @return cos(angleDeg)
     */
    static inline float fastCos15(int angleDeg) {
        const int idx = (angleDeg / 15) % 24;
        return COS_TABLE[idx];
    }

    /**
     * @brief Normalize angle to 0-359 range
     * @param angleDeg Any angle in degrees
     * @return Equivalent angle in [0, 359]
     */
    static inline int normalizeAngle(int angleDeg) {
        angleDeg = angleDeg % 360;
        if (angleDeg < 0) angleDeg += 360;
        return angleDeg;
    }
}

// =============================================================================
// COMPILE-TIME HELPERS
// =============================================================================

/**
 * @brief Calculate frame buffer size in bytes
 * 
 * @param w Width in pixels
 * @param h Height in pixels
 * @return Size in bytes for RGB565 buffer (w × h × 2)
 */
constexpr size_t TFT_FrameBytes(int16_t w, int16_t h) {
    return static_cast<size_t>(w) * static_cast<size_t>(h) * sizeof(uint16_t);
}

// =============================================================================
// API FUNCTIONS — Public interface for CMWS display
// =============================================================================
//
// Usage:
//   1. Call CMWSDisplay_init() once during setup()
//   2. Call CMWSDisplay_loop() every iteration of loop() (if not using task)
//   3. DCS-BIOS callbacks automatically update display state
//
// The display uses selective redraw — only changed elements are updated.
// Full redraw is triggered on init, mode changes, or CMWSDisplay_notifyMissionStart().

/**
 * @brief Initialize the CMWS display
 * 
 * Performs:
 *   - Pre-computes all arrow and tick geometry (~420 bytes cache)
 *   - Initializes TFT hardware (SPI, backlight)
 *   - Subscribes to all DCS-BIOS callbacks
 *   - Draws initial frame
 *   - Runs BIT test sequence (if RUN_BIT_TEST_ON_INIT == 1)
 *   - Starts FreeRTOS task (if RUN_AS_TASK == true)
 * 
 * Call once during setup().
 */
void CMWSDisplay_init();

/**
 * @brief Main loop handler
 * 
 * Checks for state changes and updates display if needed.
 * Rate-limited to ~30 FPS internally.
 * 
 * Call every iteration of loop() if not running as FreeRTOS task.
 * Safe to call even when nothing changed (early-exits efficiently).
 */
void CMWSDisplay_loop();

/**
 * @brief Notify display that mission has started
 * 
 * Forces full redraw on next frame to ensure clean state after
 * aircraft/mission change. Called by DCSBIOSBridge on mission start.
 */
void CMWSDisplay_notifyMissionStart();

/**
 * @brief Clean up display resources
 * 
 * Stops FreeRTOS task if running, blanks display, turns off backlight.
 * Call during shutdown or when switching to different display mode.
 */
void CMWSDisplay_deinit();

/**
 * @brief Run Built-In Test (BIT) sequence
 * 
 * Visual self-test for hardware verification (~2.5 seconds):
 *   1. All elements BRIGHT (500ms)
 *   2. All elements DIM (500ms)
 *   3. All elements OFF (500ms)
 *   4. Rotate each large arrow BRIGHT (300ms each)
 *   5. Restore previous state
 * 
 * Controlled by RUN_BIT_TEST_ON_INIT compile flag.
 * Can also be called manually for diagnostics.
 */
void CMWSDisplay_bitTest();
