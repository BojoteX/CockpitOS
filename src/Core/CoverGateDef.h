// CoverGateDef.h - CoverGate type definitions
// Shared by Mappings.h (declarations) and per-label-set CoverGates.h (data).

#pragma once
#include <stdint.h>

enum class CoverGateKind : uint8_t {
    Selector,
    ButtonMomentary,
    ButtonLatched,
};

struct CoverGateDef {
    const char* action_label;
    const char* release_label;
    const char* cover_label;
    CoverGateKind kind;
    uint16_t delay_ms;
    uint16_t close_delay_ms;
};
