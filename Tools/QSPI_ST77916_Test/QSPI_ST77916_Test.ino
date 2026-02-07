/*
 * ST77916 QSPI Cabin Pressure Altimeter — Standalone Test
 * ────────────────────────────────────────────────────────
 * Target:   Waveshare ESP32-S3-LCD-1.85 (360×360, ST77916, QSPI)
 * Purpose:  Prove the hybrid architecture:
 *             Rendering  → LovyanGFX sprites (in-memory, no hardware)
 *             Transport  → esp_lcd QSPI (native ST77916 opcode framing)
 *
 * Architecture:
 *   ┌────────────────────────────────────────────────────────┐
 *   │ RENDERING (LovyanGFX sprites — pure RAM, no HW)        │
 *   │   bgCache[] ──memcpy──► frameSpr                        │
 *   │   needleSpr ──pushRotateZoom──► frameSpr                │
 *   └────────────────────┬───────────────────────────────────┘
 *                        │ getBuffer()
 *                        ▼
 *   ┌────────────────────────────────────────────────────────┐
 *   │ TRANSPORT (esp_lcd — native QSPI with byte-swap)        │
 *   │   stripe copy + swap → esp_lcd_panel_draw_bitmap()      │
 *   └────────────────────────────────────────────────────────┘
 *
 * Board:    ESP32S3 Dev Module (PSRAM: OPI, Flash: QIO 80MHz)
 * Core:     Arduino Core 3.3.0+
 * Libs:     LovyanGFX (any version — sprites only, no bus)
 *
 * Pinout (Waveshare ESP32-S3-LCD-1.85):
 *   SCLK=40  D0=46  D1=45  D2=42  D3=41  CS=21  BL=5
 */

// ═════════════════════════════════════════════════════════════════════════
// INCLUDES
// ═════════════════════════════════════════════════════════════════════════

#include <LovyanGFX.hpp>       // Sprites only — no Bus or Panel used
#include <cstring>
#include <cmath>
#include <algorithm>

// esp_lcd (IDF component, included in Arduino Core 3.x)
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "driver/spi_master.h"
#include "esp_heap_caps.h"

// ST77916 driver (Espressif esp-iot-solution v2.0.3, Apache 2.0)
#include "esp_lcd_st77916.h"

// ═════════════════════════════════════════════════════════════════════════
// PIN DEFINITIONS — Waveshare ESP32-S3-LCD-1.85
// ═════════════════════════════════════════════════════════════════════════

static constexpr gpio_num_t PIN_SCLK = GPIO_NUM_40;
static constexpr gpio_num_t PIN_D0   = GPIO_NUM_46;
static constexpr gpio_num_t PIN_D1   = GPIO_NUM_45;
static constexpr gpio_num_t PIN_D2   = GPIO_NUM_42;
static constexpr gpio_num_t PIN_D3   = GPIO_NUM_41;
static constexpr gpio_num_t PIN_CS   = GPIO_NUM_21;
static constexpr gpio_num_t PIN_BL   = GPIO_NUM_5;

// ═════════════════════════════════════════════════════════════════════════
// DISPLAY CONSTANTS
// ═════════════════════════════════════════════════════════════════════════

static constexpr int16_t  SCREEN_W  = 360;
static constexpr int16_t  SCREEN_H  = 360;
static constexpr int16_t  CENTER_X  = 180;
static constexpr int16_t  CENTER_Y  = 180;

// Needle geometry (matches CockpitOS cabin pressure)
static constexpr int16_t  NEEDLE_W       = 23;
static constexpr int16_t  NEEDLE_H       = 238;
static constexpr int16_t  NEEDLE_PIVOT_X = 12;
static constexpr int16_t  NEEDLE_PIVOT_Y = 165;

// Angle range (matches CockpitOS cabin pressure)
static constexpr int16_t ANG_MIN = -181;
static constexpr int16_t ANG_MAX =  125;

// DMA stripe height (lines per DMA transfer)
static constexpr int      STRIPE_H     = 8;
static constexpr size_t   STRIPE_BYTES = size_t(SCREEN_W) * STRIPE_H * sizeof(uint16_t);

// Transparent key for needle sprite composition
static constexpr uint16_t TRANSPARENT_KEY = 0x2001;

// Frame buffer size
static constexpr size_t FRAME_PIXELS = size_t(SCREEN_W) * size_t(SCREEN_H);
static constexpr size_t FRAME_BYTES  = FRAME_PIXELS * sizeof(uint16_t);

// ═════════════════════════════════════════════════════════════════════════
// STATIC ASSERTIONS
// ═════════════════════════════════════════════════════════════════════════

static_assert(SCREEN_W > 0 && SCREEN_H > 0, "bad dims");
static_assert(STRIPE_H > 0 && STRIPE_H <= SCREEN_H, "bad STRIPE_H");
static_assert((FRAME_BYTES % 16u) == 0u, "FRAME_BYTES must be 16-byte aligned");

// ═════════════════════════════════════════════════════════════════════════
// esp_lcd HANDLES
// ═════════════════════════════════════════════════════════════════════════

static esp_lcd_panel_io_handle_t io_handle    = NULL;
static esp_lcd_panel_handle_t    panel_handle = NULL;

// ═════════════════════════════════════════════════════════════════════════
// LovyanGFX SPRITES (no device — rendering only)
// ═════════════════════════════════════════════════════════════════════════

// frameSpr: full-screen composition target in PSRAM
// needleSpr: small needle sprite in PSRAM
// Neither has a parent device — we never call pushSprite().
static lgfx::LGFX_Sprite frameSpr(nullptr);
static lgfx::LGFX_Sprite needleSpr(nullptr);

// Background cache in PSRAM (single mode for test — no NVG variant)
static uint16_t* bgCache = nullptr;

// DMA bounce buffers — MUST be in internal DMA-capable RAM
static uint16_t* dmaBounce[2] = { nullptr, nullptr };

// ═════════════════════════════════════════════════════════════════════════
// DIRTY-RECT UTILITIES (identical to CockpitOS)
// ═════════════════════════════════════════════════════════════════════════

struct Rect { int16_t x, y, w, h; };

static inline bool rectEmpty(const Rect& r) { return r.w <= 0 || r.h <= 0; }

static inline Rect rectClamp(const Rect& r) {
    int16_t x = r.x, y = r.y, w = r.w, h = r.h;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    return { x, y, w, h };
}

static inline Rect rectUnion(const Rect& a, const Rect& b) {
    if (rectEmpty(a)) return b;
    if (rectEmpty(b)) return a;
    int16_t x1 = std::min(a.x, b.x);
    int16_t y1 = std::min(a.y, b.y);
    int16_t x2 = std::max<int16_t>(a.x + a.w, b.x + b.w);
    int16_t y2 = std::max<int16_t>(a.y + a.h, b.y + b.h);
    return rectClamp({ x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1) });
}

static inline Rect rectPad(const Rect& r, int16_t px) {
    return rectClamp({ (int16_t)(r.x - px), (int16_t)(r.y - px),
                       (int16_t)(r.w + 2 * px), (int16_t)(r.h + 2 * px) });
}

// Axis-aligned bounding box of a rotated rectangle
static Rect rotatedAABB(int cx, int cy, int w, int h, int px, int py, float deg) {
    const float rad = deg * (float)M_PI / 180.0f;
    const float s = sinf(rad), c = cosf(rad);
    const float xs[4] = { (float)-px, (float)(w - px), (float)(w - px), (float)-px };
    const float ys[4] = { (float)-py, (float)-py, (float)(h - py), (float)(h - py) };
    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
    for (int i = 0; i < 4; ++i) {
        float X = (float)cx + xs[i] * c - ys[i] * s;
        float Y = (float)cy + xs[i] * s + ys[i] * c;
        if (X < minx) minx = X; if (X > maxx) maxx = X;
        if (Y < miny) miny = Y; if (Y > maxy) maxy = Y;
    }
    Rect r;
    r.x = (int16_t)floorf(minx);
    r.y = (int16_t)floorf(miny);
    r.w = (int16_t)ceilf(maxx - minx);
    r.h = (int16_t)ceilf(maxy - miny);
    return rectClamp(rectPad(r, 2));
}

// ═════════════════════════════════════════════════════════════════════════
// BACKGROUND BLIT (identical to CockpitOS)
// ═════════════════════════════════════════════════════════════════════════

static inline void blitBGRectToFrame(const uint16_t* bg, int x, int y, int w, int h) {
    if (w <= 0 || h <= 0) return;
    uint16_t* dst = (uint16_t*)frameSpr.getBuffer();
    const int pitch = SCREEN_W;
    for (int row = 0; row < h; ++row) {
        std::memcpy(&dst[(y + row) * pitch + x],
                    &bg [(y + row) * pitch + x],
                    size_t(w) * sizeof(uint16_t));
    }
}

// ═════════════════════════════════════════════════════════════════════════
// FLUSH TO DISPLAY — esp_lcd transport (replaces LovyanGFX Bus_SPI)
// ═════════════════════════════════════════════════════════════════════════
//
// Key difference from CockpitOS:
//   - Byte-swap during stripe copy (sprites store LE, ST77916 expects BE)
//   - esp_lcd_panel_draw_bitmap() handles CASET/RASET/RAMWR internally
//   - No startWrite/endWrite — esp_lcd manages the SPI transaction
//
// WCET: ~1.2 ms per full-screen flush at 80 MHz QSPI (measured estimate)

static void flushRectToDisplay(const uint16_t* src, const Rect& rr)
{
    Rect r = rectClamp(rr);
    if (rectEmpty(r)) return;

    const int pitch     = SCREEN_W;
    const int lines_per = STRIPE_H;
    int bb = 0;

    for (int y = r.y; y < r.y + r.h; y += lines_per, bb ^= 1) {
        const int h = std::min(lines_per, (int)(r.y + r.h - y));

        // Pack stripe into bounce buffer with byte-swap
        // Source: PSRAM frameSpr (LE RGB565) → Dest: internal RAM (BE RGB565)
        for (int row = 0; row < h; ++row) {
            const uint16_t* src_row = src + (y + row) * pitch + r.x;
            uint16_t*       dst_row = dmaBounce[bb] + row * r.w;
            for (int col = 0; col < r.w; ++col) {
                const uint16_t px = src_row[col];
                dst_row[col] = (px >> 8) | (px << 8);  // byte-swap LE→BE
            }
        }

        // Push stripe via esp_lcd (handles CASET/RASET/RAMWR + QSPI opcodes)
        esp_lcd_panel_draw_bitmap(panel_handle,
                                  r.x, y,
                                  r.x + r.w, y + h,
                                  dmaBounce[bb]);
    }
}

// ═════════════════════════════════════════════════════════════════════════
// SYNTHETIC ASSET GENERATION
// ═════════════════════════════════════════════════════════════════════════
//
// Generates a pressure altimeter gauge face and needle procedurally.
// Replace with real PROGMEM assets from CockpitOS for production.

// Convert RGB888 to RGB565 (native LE for sprite buffer)
static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static void generateBackground(uint16_t* buf) {
    // Dark circular gauge face with tick marks
    const uint16_t BG_COLOR   = rgb565(20, 20, 20);
    const uint16_t FACE_COLOR = rgb565(30, 35, 30);
    const uint16_t TICK_COLOR = rgb565(200, 200, 200);
    const uint16_t TEXT_COLOR = rgb565(180, 180, 180);
    const int FACE_R = 170;
    const int TICK_OUTER = 160;
    const int TICK_INNER_MAJ = 140;
    const int TICK_INNER_MIN = 150;

    // Fill background
    for (size_t i = 0; i < FRAME_PIXELS; ++i) buf[i] = BG_COLOR;

    // Draw circular face
    for (int y = 0; y < SCREEN_H; ++y) {
        for (int x = 0; x < SCREEN_W; ++x) {
            int dx = x - CENTER_X;
            int dy = y - CENTER_Y;
            if (dx * dx + dy * dy <= FACE_R * FACE_R) {
                buf[y * SCREEN_W + x] = FACE_COLOR;
            }
        }
    }

    // Draw tick marks around the dial
    // Cabin pressure range: angles from ANG_MIN (-181°) to ANG_MAX (125°)
    // Total sweep = 306°, ~10 major ticks
    for (int tick = 0; tick <= 10; ++tick) {
        float angle_deg = (float)ANG_MIN + ((float)tick / 10.0f) * (float)(ANG_MAX - ANG_MIN);
        float rad = angle_deg * (float)M_PI / 180.0f;
        float s = sinf(rad), c = cosf(rad);

        int inner = (tick % 2 == 0) ? TICK_INNER_MAJ : TICK_INNER_MIN;
        int outer = TICK_OUTER;

        // Draw thick tick line (3px wide)
        for (int t = inner; t <= outer; ++t) {
            float fx = (float)CENTER_X + (float)t * s;
            float fy = (float)CENTER_Y - (float)t * c;
            int ix = (int)roundf(fx);
            int iy = (int)roundf(fy);
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    int px = ix + dx, py = iy + dy;
                    if (px >= 0 && px < SCREEN_W && py >= 0 && py < SCREEN_H) {
                        buf[py * SCREEN_W + px] = TICK_COLOR;
                    }
                }
            }
        }
    }

    // Draw center hub
    const int HUB_R = 12;
    for (int y = CENTER_Y - HUB_R; y <= CENTER_Y + HUB_R; ++y) {
        for (int x = CENTER_X - HUB_R; x <= CENTER_X + HUB_R; ++x) {
            int dx = x - CENTER_X, dy = y - CENTER_Y;
            if (dx * dx + dy * dy <= HUB_R * HUB_R) {
                buf[y * SCREEN_W + x] = rgb565(80, 80, 80);
            }
        }
    }
}

static void generateNeedle(lgfx::LGFX_Sprite& spr) {
    // Fill with transparent key
    spr.fillScreen(TRANSPARENT_KEY);

    // Draw a tapered needle shape
    // Thin at tip (top), wider at pivot, tapers to base
    const uint16_t NEEDLE_COLOR = rgb565(255, 255, 255);
    const uint16_t NEEDLE_OUTLINE = rgb565(60, 60, 60);

    for (int y = 0; y < NEEDLE_H; ++y) {
        // Width varies: thin at y=0 (tip), widest at pivot, medium at base
        float progress = (float)y / (float)NEEDLE_H;
        int half_w;
        if (y < NEEDLE_PIVOT_Y) {
            // Tip to pivot: widens linearly
            half_w = 1 + (int)(4.0f * ((float)y / (float)NEEDLE_PIVOT_Y));
        } else {
            // Pivot to base: narrows
            float base_progress = (float)(y - NEEDLE_PIVOT_Y) / (float)(NEEDLE_H - NEEDLE_PIVOT_Y);
            half_w = 5 - (int)(3.0f * base_progress);
        }

        int cx = NEEDLE_PIVOT_X;  // center of needle width
        for (int x = cx - half_w; x <= cx + half_w; ++x) {
            if (x >= 0 && x < NEEDLE_W) {
                // Outline vs fill
                if (x == cx - half_w || x == cx + half_w) {
                    spr.drawPixel(x, y, NEEDLE_OUTLINE);
                } else {
                    spr.drawPixel(x, y, NEEDLE_COLOR);
                }
            }
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════
// DRAW FUNCTION (same pattern as CockpitOS TFT_Gauges_CabinPressure.cpp)
// ═════════════════════════════════════════════════════════════════════════

static int16_t lastDrawnAngle = INT16_MIN;

static void drawGauge(int16_t angleDeg, bool fullFlush) {
    int16_t u = angleDeg;
    if (u < ANG_MIN) u = ANG_MIN;
    if (u > ANG_MAX) u = ANG_MAX;

    // Compute dirty rectangle
    Rect dirty;
    if (fullFlush || lastDrawnAngle == INT16_MIN) {
        dirty = { 0, 0, SCREEN_W, SCREEN_H };
    } else {
        Rect oldR = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H,
                                NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)lastDrawnAngle);
        Rect newR = rotatedAABB(CENTER_X, CENTER_Y, NEEDLE_W, NEEDLE_H,
                                NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y, (float)u);
        dirty = rectUnion(oldR, newR);
    }

    // Restore background in dirty region
    blitBGRectToFrame(bgCache, dirty.x, dirty.y, dirty.w, dirty.h);

    // Compose rotated needle onto frame (clipped to dirty rect)
    frameSpr.setClipRect(dirty.x, dirty.y, dirty.w, dirty.h);
    needleSpr.pushRotateZoom(&frameSpr, CENTER_X, CENTER_Y,
                             (float)u, 1.0f, 1.0f, TRANSPARENT_KEY);
    frameSpr.clearClipRect();

    // Flush dirty region to display via esp_lcd
    const uint16_t* buf = (const uint16_t*)frameSpr.getBuffer();
    flushRectToDisplay(buf, dirty);

    lastDrawnAngle = u;
}

// ═════════════════════════════════════════════════════════════════════════
// esp_lcd INITIALIZATION
// ═════════════════════════════════════════════════════════════════════════

static bool initDisplay() {
    esp_err_t ret;

    // 1. Configure QSPI bus
    spi_bus_config_t bus_cfg = {};
    bus_cfg.sclk_io_num    = PIN_SCLK;
    bus_cfg.data0_io_num   = PIN_D0;
    bus_cfg.data1_io_num   = PIN_D1;
    bus_cfg.data2_io_num   = PIN_D2;
    bus_cfg.data3_io_num   = PIN_D3;
    bus_cfg.max_transfer_sz = SCREEN_W * STRIPE_H * sizeof(uint16_t);
    bus_cfg.flags          = SPICOMMON_BUSFLAG_MASTER;

    ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        Serial.printf("❌ SPI bus init failed: 0x%x\n", ret);
        return false;
    }

    // 2. Attach panel IO (QSPI, 80 MHz)
    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.cs_gpio_num       = PIN_CS;
    io_cfg.dc_gpio_num       = -1;         // No DC in QSPI mode
    io_cfg.spi_mode          = 0;
    io_cfg.pclk_hz           = 80 * 1000 * 1000;
    io_cfg.trans_queue_depth  = 10;
    io_cfg.lcd_cmd_bits      = 32;         // QSPI: opcode + cmd in 32-bit word
    io_cfg.lcd_param_bits    = 8;
    io_cfg.flags.quad_mode   = true;

    ret = esp_lcd_new_panel_io_spi(SPI2_HOST, &io_cfg, &io_handle);
    if (ret != ESP_OK) {
        Serial.printf("❌ Panel IO init failed: 0x%x\n", ret);
        return false;
    }

    // 3. Create ST77916 panel
    st77916_vendor_config_t vendor_cfg = {};
    vendor_cfg.init_cmds                  = NULL;  // Built-in default sequence
    vendor_cfg.init_cmds_size             = 0;
    vendor_cfg.flags.use_qspi_interface   = 1;

    esp_lcd_panel_dev_config_t panel_cfg = {};
    panel_cfg.reset_gpio_num   = -1;       // No HW reset (software reset)
    panel_cfg.rgb_ele_order    = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_cfg.bits_per_pixel   = 16;       // RGB565
    panel_cfg.vendor_config    = &vendor_cfg;

    ret = esp_lcd_new_panel_st77916(io_handle, &panel_cfg, &panel_handle);
    if (ret != ESP_OK) {
        Serial.printf("❌ Panel create failed: 0x%x\n", ret);
        return false;
    }

    // 4. Reset → Init → Display ON
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);
    esp_lcd_panel_disp_on_off(panel_handle, true);

    return true;
}

// ═════════════════════════════════════════════════════════════════════════
// SETUP
// ═════════════════════════════════════════════════════════════════════════

void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n═══ ST77916 QSPI Cabin Pressure Test ═══");

    // ── Backlight ON ─────────────────────────────────────────────────
    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);

    // ── Init esp_lcd QSPI display ────────────────────────────────────
    if (!initDisplay()) {
        Serial.println("❌ Display init FAILED — halting.");
        while (1) delay(1000);
    }
    Serial.println("✅ esp_lcd QSPI display initialized");

    // ── Allocate DMA bounce buffers (internal RAM) ───────────────────
    dmaBounce[0] = (uint16_t*)heap_caps_aligned_alloc(
        32, STRIPE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    dmaBounce[1] = (uint16_t*)heap_caps_aligned_alloc(
        32, STRIPE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!dmaBounce[0] || !dmaBounce[1]) {
        Serial.printf("❌ dmaBounce alloc failed (%u bytes each)\n", (unsigned)STRIPE_BYTES);
        while (1) delay(1000);
    }
    Serial.println("✅ DMA bounce buffers allocated (internal RAM)");

    // ── Allocate background cache (PSRAM) ────────────────────────────
    bgCache = (uint16_t*)heap_caps_aligned_alloc(
        16, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!bgCache) {
        Serial.println("❌ bgCache alloc failed — no PSRAM?");
        while (1) delay(1000);
    }

    // Generate synthetic gauge face
    Serial.println("Generating gauge background...");
    generateBackground(bgCache);
    Serial.println("✅ Background generated (PSRAM)");

    // ── Create frame sprite (PSRAM, 360×360) ─────────────────────────
    frameSpr.setColorDepth(16);
    frameSpr.setPsram(true);
    frameSpr.setSwapBytes(false);  // We handle byte-swap in flush
    if (!frameSpr.createSprite(SCREEN_W, SCREEN_H)) {
        Serial.println("❌ frameSpr alloc failed!");
        while (1) delay(1000);
    }
    Serial.printf("✅ frameSpr created (%dx%d, PSRAM)\n", SCREEN_W, SCREEN_H);

    // ── Create needle sprite ─────────────────────────────────────────
    needleSpr.setColorDepth(16);
    needleSpr.setPsram(false);     // Small enough for internal RAM
    needleSpr.setSwapBytes(false);
    if (!needleSpr.createSprite(NEEDLE_W, NEEDLE_H)) {
        Serial.println("❌ needleSpr alloc failed!");
        while (1) delay(1000);
    }
    needleSpr.setPivot(NEEDLE_PIVOT_X, NEEDLE_PIVOT_Y);
    generateNeedle(needleSpr);
    Serial.printf("✅ needleSpr created (%dx%d)\n", NEEDLE_W, NEEDLE_H);

    // ── First paint (full flush) ─────────────────────────────────────
    Serial.println("Painting initial frame...");
    drawGauge(ANG_MIN, true);
    Serial.println("✅ First frame pushed via esp_lcd QSPI");

    // ── BIT test: sweep needle across full range ─────────────────────
    Serial.println("Running BIT test sweep...");
    const int STEP = 6;
    const int DELAY_MS = 8;

    // Sweep forward
    for (int a = ANG_MIN; a <= ANG_MAX; a += STEP) {
        drawGauge((int16_t)a, false);
        delay(DELAY_MS);
    }
    // Sweep back
    for (int a = ANG_MAX; a >= ANG_MIN; a -= STEP) {
        drawGauge((int16_t)a, false);
        delay(DELAY_MS);
    }
    // Park at center
    drawGauge(0, false);

    Serial.println("✅ BIT test complete. Entering continuous animation.");
    Serial.println("═══════════════════════════════════════════");
}

// ═════════════════════════════════════════════════════════════════════════
// LOOP — continuous sine-wave animation to demonstrate real-time updates
// ═════════════════════════════════════════════════════════════════════════

void loop()
{
    // Smooth sinusoidal needle sweep
    float t = (float)millis() / 2000.0f;  // 2-second period
    float norm = (sinf(t * 2.0f * (float)M_PI) + 1.0f) / 2.0f;  // 0.0 → 1.0
    int16_t angle = (int16_t)((float)ANG_MIN + norm * (float)(ANG_MAX - ANG_MIN));

    drawGauge(angle, false);

    // Target ~75 fps (dirty-rect updates are fast)
    delay(13);
}
