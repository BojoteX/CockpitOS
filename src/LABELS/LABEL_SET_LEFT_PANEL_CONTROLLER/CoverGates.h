// CoverGates.h -- Per-label-set cover gate configuration
// Defines selectors/buttons that are physically guarded by a cover.
// Edit via the Label Creator tool or manually.

#pragma once

inline const CoverGateDef kCoverGates[] = {
    { "GAIN_SWITCH_POS1", "GAIN_SWITCH_POS0"            , "GAIN_SWITCH_COVER", CoverGateKind::Selector,  500,  500 },
};
inline const unsigned kCoverGateCount = sizeof(kCoverGates) / sizeof(kCoverGates[0]);
