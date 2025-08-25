// TEST_ONLY.cpp - Template for Panel Implementation

// ============================================================================
//  CONFIGURATION CONSTANTS
// ============================================================================

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()
#include "includes/TEST_ONLY.h"

// ----------Panel Registration---------- 
// PanelName, PanelInit, PanelLoop, DisplayInit, DisplayLoop, Tick, PollInterval
REGISTER_PANEL(TEST_ONLY, TEST_ONLY_init, TEST_ONLY_loop, nullptr, nullptr, nullptr, 100);

// Outputs only (no inputs) 
#if defined(HAS_MAIN)
    REGISTER_PANEL(LockShoot, nullptr, nullptr, nullptr, nullptr, WS2812_tick, 100);
    REGISTER_PANEL(LA, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
    REGISTER_PANEL(RA, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
#elif defined(HAS_TEST_ONLY)
    // REGISTER_PANEL(LA, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
    // REGISTER_PANEL(RA, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
#endif

// Examples:
// REGISTER_PANEL(IFEI, IFEI_init, IFEI_loop, IFEIDisplay_init, IFEIDisplay_loop, nullptr, 100);
// REGISTER_PANEL(AnalogGauge, nullptr, nullptr, nullptr, nullptr, AnalogG_tick, 100);

// ============================================================================
//  DATA STRUCTURES — MAPPINGS, INPUTS, STATE
// ============================================================================

// HC165 (Shift Register) State
static uint64_t hc165Bits = ~0ULL, hc165PrevBits = ~0ULL;

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

        if (HC165_BITS > 0) {
            HC165_init(HC165_CONTROLLER_PL, HC165_CONTROLLER_CP, HC165_CONTROLLER_QH, HC165_BITS);
			debugPrintf("ℹ️ HC165: %u bits on PL=%d CP=%d QH=%d\n", HC165_BITS, HC165_CONTROLLER_PL, HC165_CONTROLLER_CP, HC165_CONTROLLER_QH);
        }
        else {
            debugPrintln("⚠️ HC165: Disabled (HC165_BITS=0)");
		}
    
		// GPIO Inputs Init
        buildAutoAnalogInputs();
        buildGPIOEncoderStates();
        buildGpioGroupDefs();

		// HC165 Inputs Init
		buildHC165ResolvedInputs();

        // PCA9555 Inputs Init      
        buildPCA9555ResolvedInputs();
        buildPcaList();

		// Do not re-run
        ranOnce = true;
    }

    CoverGate_init();
    // --- [Per-mission: State/HIDManager Sync & Firing] ---

    // 1. Sync Analog Axes to Known State
    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        const auto& a = autoAnalogs[i];
        HIDManager_moveAxis(a.label, a.gpio, a.axis, true);
    }

    // 2. Fire All GPIO Selector States to Reset
    pollGPIOEncoders();
    pollGPIOSelectors(true);
    pollGPIOMomentaries(true);

    // 3. Sync All HC165 (Shift Register) States to HID
    if (HC165_BITS > 0) {
        hc165Bits = HC165_read();
        hc165PrevBits = hc165Bits;                  // prevent fake edges
        processHC165Resolved(hc165Bits, hc165PrevBits, true);
    }

    // 4. Take Fresh PCA9555 Snapshot and Fire All PCA States
    for (size_t i = 0; i < numPcas; ++i) {
        uint8_t p0, p1;
        if (readPCA9555(pcas[i].addr, p0, p1)) {
            pcas[i].p0 = p0;
            pcas[i].p1 = p1;
        }
    }
    pollPCA9555_flat(true);

	// 5. Matrix Polling (fire all)
    Matrix_poll(true);

	// 6. TM1637 Inputs (fire all)
    TM1637_poll(true);

    // --- [Done] ---
    debugPrintln("✅ TEST_ONLY panel initialized");
}

// ============================================================================
//  LOOP: TEST_ONLY_loop() — Fast, Deterministic Main Loop
// ============================================================================
void TEST_ONLY_loop() {
    // 1. Polling Interval: Return Early If Not Time ---
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
    pollGPIOSelectors(false);
    pollGPIOMomentaries(false);
    
    // ------------------------------------------------------------------------
    // 4. HC165 Shift Register (edge-triggered, only on change)
    // ------------------------------------------------------------------------
    if (HC165_BITS > 0) {
        const uint64_t newBits = HC165_read();
        if (newBits != hc165PrevBits) {
            processHC165Resolved(newBits, hc165PrevBits, false);
            hc165PrevBits = newBits;
        }
    }

    // ------------------------------------------------------------------------
    // 5. PCA9555 Polling (flat, O(1) for all mapped pins/groups)
    // ------------------------------------------------------------------------
    pollPCA9555_flat(false);

    // ------------------------------------------------------------------------
    // 6. Matrix Polling 
    // ------------------------------------------------------------------------
    Matrix_poll(false);

    // ------------------------------------------------------------------------
    // 7. Matrix Polling 
    // ------------------------------------------------------------------------
    TM1637_poll(false);

    // CoverGate Logic
    CoverGate_loop();
}