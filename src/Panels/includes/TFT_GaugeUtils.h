// =============================================================================
// TFT_GaugeUtils.h — Shared utilities for CockpitOS TFT gauge panels
// =============================================================================
//
// Extracted from individual gauge .cpp files to eliminate duplication.
// All functions are static inline — compiled per translation unit.
//
// REQUIREMENT: Each gauge must define SCREEN_W and SCREEN_H as
//              static constexpr int16_t BEFORE including this header.
//
// =============================================================================

#pragma once

#include <cstdint>
#include <cmath>
#include <algorithm>

// =============================================================================
// SHARED CONSTANTS
// =============================================================================

static constexpr uint16_t TRANSPARENT_KEY = 0x2001;   // Sprite transparency key (not present in assets)
static constexpr uint16_t NVG_THRESHOLD   = 6553;     // Dimmer threshold for Day/NVG mode switch

// =============================================================================
// DIRTY-RECT UTILITIES
// =============================================================================

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
    int16_t x1 = std::min(a.x, b.x), y1 = std::min(a.y, b.y);
    int16_t x2 = std::max<int16_t>(a.x + a.w, b.x + b.w);
    int16_t y2 = std::max<int16_t>(a.y + a.h, b.y + b.h);
    return rectClamp({ x1, y1, (int16_t)(x2 - x1), (int16_t)(y2 - y1) });
}

static inline Rect rectPad(const Rect& r, int16_t px) {
    return rectClamp({ (int16_t)(r.x - px), (int16_t)(r.y - px),
                       (int16_t)(r.w + 2 * px), (int16_t)(r.h + 2 * px) });
}

// =============================================================================
// ROTATED BOUNDING BOX
// =============================================================================

static inline Rect rotatedAABB(int cx, int cy, int w, int h, int px, int py, float deg) {
    const float rad = deg * (float)M_PI / 180.0f;
    float s = sinf(rad), c = cosf(rad);
    const float xs[4] = { (float)-px, (float)w - px, (float)w - px, (float)-px };
    const float ys[4] = { (float)-py, (float)-py, (float)h - py, (float)h - py };
    float minx = 1e9f, maxx = -1e9f, miny = 1e9f, maxy = -1e9f;
    for (int i = 0; i < 4; ++i) {
        float xr = xs[i] * c - ys[i] * s;
        float yr = xs[i] * s + ys[i] * c;
        float X = (float)cx + xr;
        float Y = (float)cy + yr;
        if (X < minx) minx = X; if (X > maxx) maxx = X;
        if (Y < miny) miny = Y; if (Y > maxy) maxy = Y;
    }
    Rect r;
    r.x = (int16_t)floorf(minx); r.y = (int16_t)floorf(miny);
    r.w = (int16_t)ceilf(maxx - minx); r.h = (int16_t)ceilf(maxy - miny);
    return rectClamp(rectPad(r, 2));
}
