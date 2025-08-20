// TEST_ONLY.cpp - Template for Panel Implementation

// ============================================================================
//  CONFIGURATION CONSTANTS
// ============================================================================
#define DISABLE_PCA9555                0   // 1 = skip PCA logic, 0 = enable
#define MAX_PCA9555_INPUTS             64  // Max PCA input mappings
#define MAX_PCA_GROUPS                 32  // Max selector groups
#define MAX_PCAS                       8   // Max PCA9555 chips (0x20–0x27)

#define HC165_TEST_PANEL_BITS          0   // Number of bits in HC165 shift register (0 = disabled)
#define HC165_TEST_PANEL_CONTROLLER_PL -1
#define HC165_TEST_PANEL_CONTROLLER_CP -1
#define HC165_TEST_PANEL_CONTROLLER_QH -1

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()
#include "includes/TEST_ONLY.h"

// ----------Panel Registration---------- 
REGISTER_PANEL(TEST_ONLY,
    TEST_ONLY_init,
    TEST_ONLY_loop,
    nullptr, 
    nullptr,   // no display hooks
    nullptr,   // no tick
    100);      // priority

// Examples:
// REGISTER_PANEL(IFEI, IFEI_init, IFEI_loop, IFEIDisplay_init, IFEIDisplay_loop, nullptr, 100);
// REGISTER_PANEL(LockShoot, LockShoot_init, LockShoot_loop, nullptr, nullptr, WS2812_tick, 100);
// REGISTER_PANEL(AnalogGauge, nullptr, nullptr, nullptr, nullptr, AnalogG_tick, 100);

// ============================================================================
//  DATA STRUCTURES — MAPPINGS, INPUTS, STATE
// ============================================================================

// PCA9555 Pre-resolved Flat Table
struct PCA9555Input {
    uint8_t addr, port, bit;  // bit==255 for fallback (-1)
    bool isMomentary, isSelector;
    int16_t group, oride_value;
    const char* label;
};
static PCA9555Input pca9555Inputs[MAX_PCA9555_INPUTS];
static size_t numPca9555Inputs = 0;

// PCA9555 Chip State
struct PcaState { uint8_t addr, p0 = 0xFF, p1 = 0xFF; };
static PcaState pcas[MAX_PCAS];
static size_t numPcas = 0;

// Persistent Flat Poller State
static bool    lastStatePCA9555[MAX_PCA9555_INPUTS] = { false };
static int16_t lastValSelector[MAX_SELECTOR_GROUPS][MAX_PCAS] = { { -1 } };

// HC165 (Shift Register) State
static uint64_t hc165Bits = ~0ULL, hc165PrevBits = ~0ULL;

// Analog Axis Inputs
struct AutoAnalogInput { const char* label; uint8_t gpio; HIDAxis axis; };
static AutoAnalogInput autoAnalogs[HID_AXIS_COUNT];
static size_t numAutoAnalogs = 0;

// ========== END FILE-SCOPE SECTION ==========
// (Implementation of buildGpioGroupDefs, pollGPIOSelectors, etc. continues below

static void buildAutoAnalogInputs() {
    numAutoAnalogs = 0;

    // Assign axes from AXIS_CUSTOM4 downward (reverse order)
    int axisIdx = HID_AXIS_COUNT - 1; // AXIS_CUSTOM4 is the last in enum

    for (size_t i = 0; i < InputMappingSize && axisIdx >= 0; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source) continue;
        if (strcmp(m.controlType, "analog") != 0) continue;
        if (m.port < 0) continue; // Must have a valid analog pin

        // Assign axis
        autoAnalogs[numAutoAnalogs++] = AutoAnalogInput{ m.label, (uint8_t)m.port, (HIDAxis)axisIdx };
        --axisIdx;
    }
}

// Build unique PCA list from InputMappings (sources like "PCA_0x22")
static void buildPcaList() {
    bool seen[256] = { false };
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];

#if defined(DISABLE_PCA9555) && DISABLE_PCA9555 == 1
        continue; // Skip PCA9555 if disabled
#endif

        if (!m.source) continue;
        if (startsWith(m.source, "PCA_0x")) {
            uint8_t a = parseHexByte(m.source + 4);
            if (a == 0x00) continue; // <---- SKIP PCA_0x00!
            if (!seen[a] && numPcas < 8) {
                seen[a] = true;
                pcas[numPcas++].addr = a;
            }
        }
    }
}

static void buildPCA9555ResolvedInputs() {
    numPca9555Inputs = 0;
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || !startsWith(m.source, "PCA_0x")) continue;
        if (numPca9555Inputs >= MAX_PCA9555_INPUTS) break;
        PCA9555Input& rec = pca9555Inputs[numPca9555Inputs++];
        rec.addr = parseHexByte(m.source + 4);
        rec.port = m.port;
        rec.bit = m.bit; // -1 means fallback, will show as 255 when treated as uint8_t
        rec.isMomentary = (strcmp(m.controlType, "momentary") == 0);
        rec.isSelector = (strcmp(m.controlType, "selector") == 0);
        rec.group = m.group;
        rec.oride_value = m.oride_value;
        rec.label = m.label;
    }
}

static void pollPCA9555_flat(bool forceSend = false) {
    for (size_t chip = 0; chip < numPcas; ++chip) {
        uint8_t addr = pcas[chip].addr;
        uint8_t port0, port1;
        if (!readPCA9555(addr, port0, port1)) continue;

        // --- Momentaries ---
        for (size_t i = 0; i < numPca9555Inputs; ++i) {
            const PCA9555Input& pin = pca9555Inputs[i];
            if (pin.addr != addr) continue;

            // Only process REAL bits for momentaries!
            if (pin.isMomentary && pin.bit >= 0 && pin.bit < 8) {
                uint8_t pval = (pin.port == 0) ? port0 : port1;
                bool pressed = ((pval & (1 << pin.bit)) == 0); // active LOW
                if (pressed != lastStatePCA9555[i] || forceSend) {
                    HIDManager_setNamedButton(pin.label, forceSend, pressed);
                    lastStatePCA9555[i] = pressed;
                }
            }
            // Do NOT process pin.bit == -1 or 255!
        }

        // --- Selector group logic ---
        for (int group = 1; group < MAX_SELECTOR_GROUPS; ++group) {
            const PCA9555Input* winner = nullptr;
            const PCA9555Input* fallback = nullptr;

            for (size_t i = 0; i < numPca9555Inputs; ++i) {
                const PCA9555Input& pin = pca9555Inputs[i];
                if (pin.addr != addr || !pin.isSelector || pin.group != group) continue;
                if (pin.bit == -1 || pin.bit == 255) { fallback = &pin; continue; }

                uint8_t pval = (pin.port == 0) ? port0 : port1;
                if ((pval & (1 << pin.bit)) == 0) { winner = &pin; break; }
            }
            if (!winner && fallback) winner = fallback;

            // Latch & fire once per group per chip
            if (winner && (forceSend || lastValSelector[group][chip] != winner->oride_value)) {
                lastValSelector[group][chip] = winner->oride_value;
                HIDManager_setNamedButton(winner->label, forceSend, true);
            }
        }

        // (Optional: update pcas[] cached values for diagnostic/logging)
        pcas[chip].p0 = port0;
        pcas[chip].p1 = port1;
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
        buildPCA9555ResolvedInputs();
        buildGPIOEncoderStates();
        buildGpioGroupDefs();
        buildAutoAnalogInputs();
        buildPcaList();
        CoverGate_init();

        if (HC165_TEST_PANEL_BITS > 0) {
            HC165_init(HC165_TEST_PANEL_CONTROLLER_PL,
                HC165_TEST_PANEL_CONTROLLER_CP,
                HC165_TEST_PANEL_CONTROLLER_QH,
                HC165_TEST_PANEL_BITS);
        }
        ranOnce = true;
    }

    // --- [Per-mission: State/HIDManager Sync & Firing] ---

    // 1. Sync Analog Axes to Known State
    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        const auto& a = autoAnalogs[i];
        HIDManager_moveAxis(a.label, a.gpio, a.axis, true);
    }

    // 2. Take Fresh PCA9555 Snapshot and Fire All PCA States
    for (size_t i = 0; i < numPcas; ++i) {
        uint8_t p0, p1;
        if (readPCA9555(pcas[i].addr, p0, p1)) {
            pcas[i].p0 = p0;
            pcas[i].p1 = p1;
        }
    }
    pollPCA9555_flat(true);

    // 3. Sync All HC165 (Shift Register) States to HID
    if (HC165_TEST_PANEL_BITS > 0) {
        hc165Bits = hc165PrevBits = HC165_read();
        bool groupActive[MAX_SELECTOR_GROUPS] = { false };
        // Momentaries and Selectors
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const auto& m = InputMappings[i];
            if (!m.label || !m.source || strcmp(m.source, "HC165") != 0) continue;
            if (m.bit >= 0 && m.bit < 64) {
                bool pressed = ((hc165Bits >> m.bit) & 1) == 0;
                if (strcmp(m.controlType, "momentary") == 0) {
                    HIDManager_setNamedButton(m.label, true, pressed);
                }
                else if (m.group > 0 && pressed) {
                    groupActive[m.group] = true;
                    HIDManager_setNamedButton(m.label, true, true);
                }
            }
        }
        // Fallback selectors for HC165
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const auto& m = InputMappings[i];
            if (!m.label || !m.source || strcmp(m.source, "HC165") != 0) continue;
            if (m.bit == -1 && m.group > 0 && !groupActive[m.group]) {
                HIDManager_setNamedButton(m.label, true, true);
            }
        }
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

    // Handle GPIO momentaries (not part of encoder mask)
    static bool lastGpioMomentaryState[256] = { false };
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
        if (encoderPinMask[m.port]) continue; // skip encoder pins
        if (strcmp(m.controlType, "momentary") != 0) continue;

        int pinState = digitalRead(m.port);
        bool isActive = (m.bit == 0) ? (pinState == LOW) : (pinState == HIGH);

        if (isActive != lastGpioMomentaryState[i]) {
            HIDManager_setNamedButton(m.label, false, isActive ? 1 : 0);
            lastGpioMomentaryState[i] = isActive;
        }
    }

    // ------------------------------------------------------------------------
    // 5. HC165 Shift Register (edge-triggered, only on change)
    // ------------------------------------------------------------------------
    if (HC165_TEST_PANEL_BITS > 0) {
        uint64_t bits = HC165_read();
        if (bits != hc165PrevBits) {
            bool groupActive[MAX_SELECTOR_GROUPS] = { false };
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const auto& m = InputMappings[i];
                if (!m.label || !m.source || strcmp(m.source, "HC165") != 0 || m.bit < 0 || m.bit >= 64) continue;
                bool nowPressed = ((bits >> m.bit) & 1) == 0;
                bool prevPressed = ((hc165PrevBits >> m.bit) & 1) == 0;
                if (strcmp(m.controlType, "momentary") == 0) {
                    if (nowPressed != prevPressed)
                        HIDManager_setNamedButton(m.label, false, nowPressed);
                }
                else if (m.group > 0 && nowPressed) {
                    groupActive[m.group] = true;
                    HIDManager_setNamedButton(m.label, false, true);
                }
            }
            // Fallback selectors for HC165 (bit==-1, group not active)
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const auto& m = InputMappings[i];
                if (!m.label || !m.source || strcmp(m.source, "HC165") != 0 || m.bit != -1 || m.group == 0) continue;
                if (!groupActive[m.group]) {
                    HIDManager_setNamedButton(m.label, false, true);
                }
            }
            hc165PrevBits = bits;
        }
    }

    // ------------------------------------------------------------------------
    // 6. PCA9555 Polling (flat, O(1) for all mapped pins/groups)
    // ------------------------------------------------------------------------
    pollPCA9555_flat(false);
    CoverGate_loop();
}