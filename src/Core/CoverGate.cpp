// CoverGate.cpp - CoverGate management for CockpitOS

#include "../Globals.h"
#include "../../Mappings.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"   // getLastKnownState(), findCmdEntry()

enum Pending : uint8_t { P_NONE = 0u, P_SEND_ON = 1u, P_CLOSE_COVER = 2u };

struct GateState { Pending p; uint32_t due_ms; };

static GateState s_state[MAX_COVER_GATES];
static bool s_reentry = false;

// Local helper: use cached command history as the single source of truth
static inline bool cg_isLatchedOn(const char* inputLabel) {
    CommandHistoryEntry* e = findCmdEntry(inputLabel);
    return (e && (e->lastValue > 0));
}

void CoverGate_init() {
    if (kCoverGateCount > MAX_COVER_GATES) {
        debugPrintf("❌ kCoverGateCount=%u exceeds MAX_COVER_GATES=%u\n",
            (unsigned)kCoverGateCount, (unsigned)MAX_COVER_GATES);
    }
    const uint8_t n = (kCoverGateCount < MAX_COVER_GATES) ? (uint8_t)kCoverGateCount : (uint8_t)MAX_COVER_GATES;
    for (uint8_t i = 0u; i < n; ++i) {
        s_state[i].p = P_NONE;
        s_state[i].due_ms = 0u;
    }
}

// Returns true if event was handled (deferred/sequenced)
bool CoverGate_intercept(const char* label, bool pressed) {
    if ((label == nullptr) || s_reentry) return false;

    const uint8_t n = (kCoverGateCount < MAX_COVER_GATES) ? (uint8_t)kCoverGateCount : (uint8_t)MAX_COVER_GATES;

    for (uint8_t i = 0u; i < n; ++i) {
        const CoverGateDef& g = kCoverGates[i];

        // --- 2-Pos SELECTOR guarded logic ---
        if (g.kind == CoverGateKind::Selector) {
            // Armed/ON position selected
            if (g.action_label && (strcmp(label, g.action_label) == 0) && pressed) {
                s_reentry = true;
                HIDManager_setNamedButton(g.cover_label, false, true);   // open cover now
                s_reentry = false;

                s_state[i].p = P_SEND_ON;
                s_state[i].due_ms = millis() + g.delay_ms;
                return true;
            }
            // Safe/OFF position selected
            if (g.release_label && (strcmp(label, g.release_label) == 0) && pressed) {
                s_reentry = true;
                HIDManager_setNamedButton(g.release_label, false, true); // enforce safe position now
                s_reentry = false;

                s_state[i].p = P_CLOSE_COVER;
                s_state[i].due_ms = millis() + g.close_delay_ms;
                return true;
            }
        }

        // --- GUARDED LATCHED MOMENTARY BUTTON (e.g., FIRE) ---
        else if (g.kind == CoverGateKind::ButtonMomentary) {
            if (g.action_label && (strcmp(label, g.action_label) == 0)) {
                if (pressed) {
                    const uint16_t coverState = getLastKnownState(g.cover_label);   // >0 = open

                    if (coverState == 0u) {
                        // Cover closed: open now, then (after delay) toggle to ON if currently OFF
                        s_reentry = true;
                        HIDManager_setNamedButton(g.cover_label, false, true);      // open
                        s_reentry = false;

                        s_state[i].p = P_SEND_ON;
                        s_state[i].due_ms = millis() + g.delay_ms;
                    } else {
                        // Cover open: toggle unconditionally, then schedule close
                        s_reentry = true;
                        HIDManager_setToggleNamedButton(g.action_label, false);     // toggle now
                        s_reentry = false;

                        s_state[i].p = P_CLOSE_COVER;
                        s_state[i].due_ms = millis() + g.close_delay_ms;
                    }
                    return true; // handled; block fallback
                } else {
                    // RELEASE: do nothing; block fallback to avoid double-send
                    return true;
                }
            }
        }
    }
    return false; // not handled here
}

// --- Main CoverGate loop: handles all pending actions ---
void CoverGate_loop() {
    const uint32_t now = millis();

    const uint8_t n = (kCoverGateCount < MAX_COVER_GATES) ? (uint8_t)kCoverGateCount : (uint8_t)MAX_COVER_GATES;
    for (uint8_t i = 0u; i < n; ++i) {
        if (s_state[i].p == P_NONE) continue;
        if ((int32_t)(now - s_state[i].due_ms) < 0) continue;

        const CoverGateDef& g = kCoverGates[i];

        // --- Selector deferred armed action ---
        if ((g.kind == CoverGateKind::Selector) && (s_state[i].p == P_SEND_ON)) {
            s_reentry = true;
            HIDManager_setNamedButton(g.action_label, false, true);  // select armed/ON
            s_reentry = false;

            s_state[i].p = P_NONE;
            continue;
        }

        // --- Selector deferred cover close ---
        if ((g.kind == CoverGateKind::Selector) && (s_state[i].p == P_CLOSE_COVER)) {
            s_reentry = true;
            HIDManager_setNamedButton(g.cover_label, false, false);  // close cover
            s_reentry = false;

            s_state[i].p = P_NONE;
            continue;
        }

        // --- Guarded latched momentary: deferred ON (only if currently OFF per cache) ---
        if ((g.kind == CoverGateKind::ButtonMomentary) && (s_state[i].p == P_SEND_ON)) {
            if (!cg_isLatchedOn(g.action_label)) {
                s_reentry = true;
                HIDManager_setToggleNamedButton(g.action_label, false); // latch ON
                s_reentry = false;
            }
            s_state[i].p = P_NONE;
            continue;
        }

        // --- Guarded latched momentary: deferred cover close ---
        if ((g.kind == CoverGateKind::ButtonMomentary) && (s_state[i].p == P_CLOSE_COVER)) {
            s_reentry = true;
            HIDManager_setNamedButton(g.cover_label, false, false);  // close cover
            s_reentry = false;

            s_state[i].p = P_NONE;
            continue;
        }
    }
}
