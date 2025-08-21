// TEST_ONLY.cpp - Template for Panel Implementation

// ============================================================================
//  CONFIGURATION CONSTANTS
// ============================================================================

#define HC165_TEST_PANEL_BITS                 40   // Number of bits in HC165 shift register (0 = disabled)
#define HC165_TEST_PANEL_CONTROLLER_QH        PIN(36)  // QH
#define HC165_TEST_PANEL_CONTROLLER_CP        PIN(38)  // CP
#define HC165_TEST_PANEL_CONTROLLER_PL        PIN(39)  // PL

/*
// Right Panel HC165 Controller
#define HC165_TEST_PANEL_BITS                 48   // Number of bits in HC165 shift register (0 = disabled)
#define HC165_TEST_PANEL_CONTROLLER_QH        PIN(33)   // Serial Data Out (QH)
#define HC165_TEST_PANEL_CONTROLLER_CP        PIN(34)   // Shift Clock (CP)
#define HC165_TEST_PANEL_CONTROLLER_PL        PIN(35)   // Latch/Load (PL)
*/

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()
#include "includes/TEST_ONLY.h"

// ----------Panel Registration---------- 
// PanelName, PanelInit, PanelLoop, DisplayInit, DisplayLoop, Tick, PollInterval
REGISTER_PANEL(TEST_ONLY, TEST_ONLY_init, TEST_ONLY_loop, nullptr, nullptr, nullptr, 100);

// Examples:
// REGISTER_PANEL(IFEI, IFEI_init, IFEI_loop, IFEIDisplay_init, IFEIDisplay_loop, nullptr, 100);
// REGISTER_PANEL(LockShoot, LockShoot_init, LockShoot_loop, nullptr, nullptr, WS2812_tick, 100);
// REGISTER_PANEL(AnalogGauge, nullptr, nullptr, nullptr, nullptr, AnalogG_tick, 100);

// ============================================================================
//  DATA STRUCTURES — MAPPINGS, INPUTS, STATE
// ============================================================================

// HC165 (Shift Register) State
static uint64_t hc165Bits = ~0ULL, hc165PrevBits = ~0ULL;
static uint16_t selectorOverrideCache[MAX_SELECTOR_GROUPS] = { 0xFFFF };

// ---- HC165 resolved tables (static, no heap) ----
struct HC165Sel {
    int8_t       bit;          // -1 = fallback
    uint16_t     group;
    uint16_t     oride_value;
    const char* label;
};
struct HC165Mom {
    uint8_t      bit;          // real bit 0..63
    const char* label;
};

static HC165Sel  hc165Selectors[MAX_SELECTOR_GROUPS * 8];
static size_t    hc165SelCount = 0;
static int16_t   hc165FallbackByGroup[MAX_SELECTOR_GROUPS]; // index into hc165Selectors or -1
static HC165Mom  hc165Momentaries[64];
static size_t    hc165MomCount = 0;

static inline bool isSel(const InputMapping& m) { return strcmp(m.controlType, "selector") == 0; }
static inline bool isMom(const InputMapping& m) { return strcmp(m.controlType, "momentary") == 0; }
static inline bool isHC165(const InputMapping& m) { return m.source && strcmp(m.source, "HC165") == 0; }

void buildHC165ResolvedInputs() {
    hc165SelCount = 0;
    hc165MomCount = 0;
    for (int g = 0; g < MAX_SELECTOR_GROUPS; ++g) hc165FallbackByGroup[g] = -1;

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !isHC165(m)) continue;

        if (isMom(m)) {
            if (m.bit >= 0 && m.bit < 64 && hc165MomCount < 64) {
                hc165Momentaries[hc165MomCount++] = HC165Mom{ (uint8_t)m.bit, m.label };
            }
            continue;
        }

        if (isSel(m) && m.group > 0 && m.group < MAX_SELECTOR_GROUPS && hc165SelCount < (MAX_SELECTOR_GROUPS * 8)) {
            // store selector entry (including fallbacks with bit==-1)
            hc165Selectors[hc165SelCount] = HC165Sel{ (int8_t)m.bit, m.group, m.oride_value, m.label };
            // first fallback per group sticks
            if (m.bit == -1 && hc165FallbackByGroup[m.group] == -1) hc165FallbackByGroup[m.group] = (int16_t)hc165SelCount;
            ++hc165SelCount;
        }
    }
}

// If you have inverted HC165 lines, xor them here before tests.
static inline bool hc165Pressed(uint64_t bits, uint8_t bit) {
    // active-low
    return (((bits >> bit) & 1ULL) == 0ULL);
}

void processHC165Resolved(uint64_t currentBits, uint64_t previousBits, bool forceSend) {
    // 1) Momentaries: edge-based
    for (size_t i = 0; i < hc165MomCount; ++i) {
        const auto& m = hc165Momentaries[i];
        const bool now = hc165Pressed(currentBits, m.bit);
        const bool prev = hc165Pressed(previousBits, m.bit);
        if (forceSend || now != prev) {
            HIDManager_setNamedButton(m.label, forceSend, now);
        }
    }

    // 2) Selectors: choose winner per group, then emit once
    const HC165Sel* winner[MAX_SELECTOR_GROUPS] = { nullptr };
    bool groupHasReal[MAX_SELECTOR_GROUPS] = { false };

    // Pass A: find first real pressed per group
    for (size_t i = 0; i < hc165SelCount; ++i) {
        const auto& e = hc165Selectors[i];
        if (e.bit < 0) continue; // skip fallbacks here
        if (hc165Pressed(currentBits, (uint8_t)e.bit)) {
            if (!winner[e.group]) winner[e.group] = &e;  // first in table order wins
            groupHasReal[e.group] = true;
        }
    }

    // Pass B: fallback if no real pressed
    for (int g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        const HC165Sel* pick = winner[g];
        if (!pick && !groupHasReal[g]) {
            const int16_t idx = hc165FallbackByGroup[g];
            if (idx >= 0) pick = &hc165Selectors[idx];
        }
        if (!pick) continue;

        if (forceSend || selectorOverrideCache[g] != pick->oride_value) {
            selectorOverrideCache[g] = pick->oride_value;
            HIDManager_setNamedButton(pick->label, forceSend, true);
        }
    }
}

// ============================================================================
//  INIT: TEST_ONLY_init() — Fast, Deterministic init for Cockpit Panel
// ============================================================================
void TEST_ONLY_init() {
    static bool ranOnce = false;
    if (!ranOnce) {
        // --- [Run-once: Hardware and Flat Table Setup] ---
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const auto& m = InputMappings[i];
            if (m.source && strcmp(m.source, "GPIO") == 0 && m.port >= 0)
                pinMode(m.port, INPUT_PULLUP);
        }

        if (HC165_TEST_PANEL_BITS > 0) {
            HC165_init(HC165_TEST_PANEL_CONTROLLER_PL,
                HC165_TEST_PANEL_CONTROLLER_CP,
                HC165_TEST_PANEL_CONTROLLER_QH,
                HC165_TEST_PANEL_BITS);
        }

        buildAutoAnalogInputs();
        buildGPIOEncoderStates();
        buildGpioGroupDefs();
        buildPCA9555ResolvedInputs();
        buildPcaList();
        CoverGate_init();
		buildHC165ResolvedInputs();
        ranOnce = true;
    }

    // --- [Per-mission: State/HIDManager Sync & Firing] ---

    // 2. Take Fresh PCA9555 Snapshot and Fire All PCA States
    for (size_t i = 0; i < numPcas; ++i) {
        uint8_t p0, p1;
        if (readPCA9555(pcas[i].addr, p0, p1)) {
            pcas[i].p0 = p0;
            pcas[i].p1 = p1;
        }
    }
    pollPCA9555_flat(true);

    // 1. Sync Analog Axes to Known State
    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        const auto& a = autoAnalogs[i];
        HIDManager_moveAxis(a.label, a.gpio, a.axis, true);
    }

    // 3. Sync All HC165 (Shift Register) States to HID
    if (HC165_TEST_PANEL_BITS > 0) {
        hc165Bits = HC165_read();
        hc165PrevBits = hc165Bits;                  // prevent fake edges
        processHC165Resolved(hc165Bits, hc165PrevBits, true);
    }

    // 4. Fire All GPIO Selector States to Reset
    pollGPIOSelectors(true);

    // --- [Done] ---
    debugPrintln("✅ TEST_ONLY panel initialized");
}

// ============================================================================
//  LOOP: TEST_ONLY_loop() — Fast, Deterministic Main Loop
// ============================================================================
void TEST_ONLY_loop() {
    // --- 1. Polling Interval: Return Early If Not Time ---
    static unsigned long lastPoll = 0;
    if (!shouldPollMs(lastPoll)) return;

    // ------------------------------------------------------------------------
    // 2. Analog Axes (HIDManager_moveAxis, debounced per loop)
    // ------------------------------------------------------------------------
    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        const auto& a = autoAnalogs[i];
        HIDManager_moveAxis(a.label, a.gpio, a.axis, false);
    }

    // ------------------------------------------------------------------------
    // 3. GPIO Encoders (Quadrature, high-speed polling)
    // ------------------------------------------------------------------------
    pollGPIOEncoders();

    // ------------------------------------------------------------------------
    // 4. GPIO Selectors and Momentaries (digitalRead, O(1) per pin)
    // ------------------------------------------------------------------------
    pollGPIOSelectors(false);

	// GPIO Momentaries (digitalRead, O(n) per pin)
    pollGPIOMomentaries();

    // ------------------------------------------------------------------------
    // 5. HC165 Shift Register (edge-triggered, only on change)
    // ------------------------------------------------------------------------
    if (HC165_TEST_PANEL_BITS > 0) {
        const uint64_t newBits = HC165_read();
        if (newBits != hc165PrevBits) {
            processHC165Resolved(newBits, hc165PrevBits, false);
            hc165PrevBits = newBits;
        }
    }

    // ------------------------------------------------------------------------
    // 6. PCA9555 Polling (flat, O(1) for all mapped pins/groups)
    // ------------------------------------------------------------------------
    pollPCA9555_flat(false);
    CoverGate_loop();
}