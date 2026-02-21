// LatchedButtons.h -- Per-label-set latched button configuration
// Buttons listed here toggle ON/OFF instead of acting as momentary press/release.
// Edit via the Label Creator tool or manually.

#pragma once

static const char* kLatchedButtons[] = {
    "THROTTLE_ATC_SW",
};
static const unsigned kLatchedButtonCount = sizeof(kLatchedButtons)/sizeof(kLatchedButtons[0]);
