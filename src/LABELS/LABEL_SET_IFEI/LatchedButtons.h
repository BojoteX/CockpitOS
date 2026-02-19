// LatchedButtons.h -- Per-label-set latched button configuration
// Buttons listed here toggle ON/OFF instead of acting as momentary press/release.
// Edit via the Label Creator tool or manually.

#pragma once

inline const char* kLatchedButtons[] = {
    "SJ_CTR",
    "SJ_LI",
    "SJ_LO",
    "SJ_RI",
    "SJ_RO",
};
inline const unsigned kLatchedButtonCount = sizeof(kLatchedButtons)/sizeof(kLatchedButtons[0]);
