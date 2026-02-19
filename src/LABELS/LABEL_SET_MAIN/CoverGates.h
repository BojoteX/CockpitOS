// CoverGates.h -- Per-label-set cover gate configuration
// Defines selectors/buttons that are physically guarded by a cover.
// Edit via the Label Creator tool or manually.

#pragma once

inline const CoverGateDef kCoverGates[] = {
    { "SPIN_RECOVERY_SW_RCVY", "SPIN_RECOVERY_SW_NORM"       , "SPIN_RECOVERY_COVER", CoverGateKind::Selector,  500,  500 },
    { "LEFT_FIRE_BTN", nullptr                       , "LEFT_FIRE_BTN_COVER", CoverGateKind::ButtonMomentary,  350,  300 },
    { "RIGHT_FIRE_BTN", nullptr                       , "RIGHT_FIRE_BTN_COVER", CoverGateKind::ButtonMomentary,  350,  300 },
};
inline const unsigned kCoverGateCount = sizeof(kCoverGates) / sizeof(kCoverGates[0]);
