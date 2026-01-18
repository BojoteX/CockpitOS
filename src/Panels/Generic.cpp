// Generic.cpp - All panel inputs (GPIO, HC165, PCA9555, Analog & TM1637)

// ============================================================================
//  CONFIGURATION CONSTANTS
// ============================================================================

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()
#include "includes/Generic.h"

// Defaults - only if not already defined (e.g., by CustomPins.h)
#ifndef HC165_BITS
    #define HC165_BITS                 0
#endif
#ifndef HC165_CONTROLLER_PL
    #define HC165_CONTROLLER_PL       -1
#endif
#ifndef HC165_CONTROLLER_CP
    #define HC165_CONTROLLER_CP       -1
#endif
#ifndef HC165_CONTROLLER_QH
    #define HC165_CONTROLLER_QH       -1
#endif

// Main Inputs Panel (Generic for GPIO, HC165, PCA9555, Analog & TM1637)
REGISTER_PANEL(Generic, Generic_init, Generic_loop, nullptr, nullptr, nullptr, 100); // Main Inputs

// ============================================================================
//  DATA STRUCTURES — MAPPINGS, INPUTS, STATE
// ============================================================================

// HC165 (Shift Register) State
static uint64_t hc165Bits = ~0ULL, hc165PrevBits = ~0ULL;

// ============================================================================
//  INIT: Generic_init() — Fast, Deterministic init for Cockpit Panel
// ============================================================================
void Generic_init() {
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
        if (HC165_BITS > 0) {
            buildHC165ResolvedInputs();
        }

        // PCA9555 Inputs Init      
        #if ENABLE_PCA9555 
            buildPCA9555ResolvedInputs();
            buildPcaList();
        #else
		    debugPrintln("⚠️ PCA9555: Disabled (ENABLE_PCA9555=0)");
        #endif
	
        // Do not re-run
        ranOnce = true;
    }

    CoverGate_init();
    // --- [Per-mission: State/HIDManager Sync & Firing] ---

    // 1. Fire All GPIO Selector States to Reset
    pollGPIOEncoders();
    pollGPIOSelectors(true);
    pollGPIOMomentaries(true);

    // 2. Sync All HC165 (Shift Register) States to HID
    if (HC165_BITS > 0) {
        hc165Bits = HC165_read();
        hc165PrevBits = hc165Bits;                  // prevent fake edges
        processHC165Resolved(hc165Bits, hc165PrevBits, true);
    }

    #if ENABLE_PCA9555 
        // 3. Take Fresh PCA9555 Snapshot and Fire All PCA States
        for (size_t i = 0; i < numPcas; ++i) {
            uint8_t p0, p1;
            if (readPCA9555(pcas[i].addr, p0, p1)) {
                pcas[i].p0 = p0;
                pcas[i].p1 = p1;
            }
        }
        pollPCA9555_flat(true);
    #endif

	// 4. Matrix Polling (fire all)
    Matrix_poll(true);

	// 5. TM1637 Inputs (fire all)
    TM1637_poll(true);

    // 6. Sync Analog Axes to Known State
    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        const auto& a = autoAnalogs[i];
        HIDManager_moveAxis(a.label, a.gpio, a.axis, true, true);
    }

    // --- [Done] ---
    debugPrintln("✅ Generic panel initialized");
}

// ============================================================================
//  LOOP: Generic_loop() — Fast, Deterministic Main Loop
// ============================================================================

void Generic_loop() {

    // Disabled as it has a big impact on performance. No oversampling
    /*
    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        AnalogAcq::sample(autoAnalogs[i].gpio);   // fast, every loop
    }
    */

    // 1. Polling Interval: Return Early If Not Time ---
    static unsigned long lastPoll = 0;
    if (!shouldPollMs(lastPoll)) return;

    // ------------------------------------------------------------------------
    // 2. Analog Axes (HIDManager_moveAxis, debounced per loop)
    // ------------------------------------------------------------------------
    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        const auto& a = autoAnalogs[i];
        HIDManager_moveAxis(a.label, a.gpio, a.axis, false, false);
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
    #if ENABLE_PCA9555 
        pollPCA9555_flat(false);
    #endif

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