// CoverGates.h -- Per-label-set cover gate configuration
// Defines selectors/buttons that are physically guarded by a cover.
// Edit via the Label Creator tool or manually.

#pragma once

static const CoverGateDef kCoverGates[] = {
    { "GEN_TIE_SW_RESET", "GEN_TIE_SW_NORM"             , "GEN_TIE_COVER", CoverGateKind::Selector,  500,  500 },
};
static const unsigned kCoverGateCount = sizeof(kCoverGates) / sizeof(kCoverGates[0]);
