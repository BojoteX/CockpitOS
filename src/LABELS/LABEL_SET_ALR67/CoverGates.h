// CoverGates.h -- Per-label-set cover gate configuration
// Defines selectors/buttons that are physically guarded by a cover.
// Edit via the Label Creator tool or manually.

#pragma once

const CoverGateDef kCoverGates[] = {
    // { "ACTION", "RELEASE_OR_nullptr", "COVER", CoverGateKind::Selector, delay_ms, close_delay_ms },
};
const unsigned kCoverGateCount = sizeof(kCoverGates) / sizeof(kCoverGates[0]);
