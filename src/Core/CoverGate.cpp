// CoverGate.cpp - CoverGate management for CockpitOS

#include "../Globals.h"
#include "../HIDManager.h"

#define COVERGATE_DEFAULT_DELAY_MS   500u  // Default delay for cover gate open/close

// Cover open/close definitions
struct GateDef {
    const char* on_label;
    const char* off_label;
    const char* cover_label;
    uint16_t    delay_ms;
};
static const GateDef kGates[] = {
    { "GEN_TIE_SW_RESET", "GEN_TIE_SW_NORM", "GEN_TIE_COVER", COVERGATE_DEFAULT_DELAY_MS },
    { "SPIN_RECOVERY_SW_RCVY", "SPIN_RECOVERY_SW_NORM", "SPIN_RECOVERY_COVER", COVERGATE_DEFAULT_DELAY_MS },
    // ... more cover-gated selectors here ...
};
static constexpr uint8_t kGateCount = (uint8_t)(sizeof(kGates) / sizeof(kGates[0]));
enum Pending : uint8_t { P_NONE = 0, P_SEND_ON = 1, P_CLOSE_COVER = 2 };
struct GateState { Pending p; uint32_t due_ms; };
static GateState s_state[kGateCount];
static bool s_reentry = false;

void CoverGate_init() {
    for (uint8_t i = 0; i < kGateCount; ++i) { s_state[i].p = P_NONE; s_state[i].due_ms = 0u; }
}

// Returns true if event was handled (deferred/sequenced)
bool CoverGate_intercept(const char* label, bool pressed) {

    if (!label || s_reentry) return false;

    // debugPrintf("[CoverGate Received] Received: %s", label);

    for (uint8_t i = 0; i < kGateCount; ++i) {
        const GateDef& g = kGates[i];
        // --- ON (selector activate) ---
        if (strcmp(label, g.on_label) == 0 && pressed) {
            // Step 1: open cover now
            s_reentry = true;
            HIDManager_setNamedButton(g.cover_label, false, true);
            s_reentry = false;

            // Step 2: defer selector ON
            s_state[i].p = P_SEND_ON;
            s_state[i].due_ms = millis() + g.delay_ms;
            return true; // Do not send selector now
        }
        // --- OFF (selector deactivate) ---
        if (strcmp(label, g.off_label) == 0 && pressed) {
            // Step 1: fire selector OFF/NORM immediately
            s_reentry = true;
            HIDManager_setNamedButton(g.off_label, false, true);
            s_reentry = false;

            // Step 2: defer cover close
            s_state[i].p = P_CLOSE_COVER;
            s_state[i].due_ms = millis() + g.delay_ms;
            return true;
        }
    }
    return false;
}

void CoverGate_loop() {
    const uint32_t now = millis();
    for (uint8_t i = 0; i < kGateCount; ++i) {
        if (s_state[i].p == P_NONE) continue;
        if ((int32_t)(now - s_state[i].due_ms) < 0) continue; // unsigned wrap-safe
        const GateDef& g = kGates[i];

        if (s_state[i].p == P_SEND_ON) {
            s_reentry = true;
            HIDManager_setNamedButton(g.on_label, false, true);
            s_reentry = false;
        }
        else if (s_state[i].p == P_CLOSE_COVER) {
            s_reentry = true;
            HIDManager_setNamedButton(g.cover_label, false, false);
            s_reentry = false;
        }
        s_state[i].p = P_NONE;
    }
}