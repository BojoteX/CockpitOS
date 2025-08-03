// LeftPanelController.cpp - Handles the left panel controller buttons and axes
// This implementation uses the InputMappings.h for automated button and selector mappings, axes are managed separately (see AnalogInputs).
// It supports GPIO encoders, HC165 shift register buttons, and a virtual cover for the gain switch.

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

#define MAX_ENCODERS 4
#define ENCODER_TICKS_PER_NOTCH 4

static const int8_t encoder_transition_table[16] = {
    0,  -1,   1,   0,
    1,   0,   0,  -1,
   -1,   0,   0,   1,
    0,   1,  -1,   0
};

struct EncoderState {
    const InputMapping* pos0;   // GPIO entry, oride_value==0 (CCW)
    const InputMapping* pos1;   // GPIO entry, oride_value==1 (CW)
    uint8_t pinA;               // GPIO# for pos0
    uint8_t pinB;               // GPIO# for pos1
    uint8_t lastState;          // Quadrature state (00..11)
    int8_t  accum;              // Step accumulator
    int32_t position;           // Optional: detent position count
};
static EncoderState gpioEncoders[MAX_ENCODERS];
static uint8_t numEncoders = 0;

// ---- GAIN_SWITCH Virtual Cover Logic and deferred Pos1 setting ----
static bool pendingGainCoverClose = false;
static bool pendingGainPos1 = false;
static unsigned long gainCoverOpenedMs = 0;

// Correct Enums
enum HC165Bits {
	GAIN_SWITH_BIT = 32 // GAIN_SWITCH bit in HC165, used for deferred cover logic
};

// --- Configuration ---
#define MAX_SELECTOR_GROUPS 32

// --- Axis Input ---
struct AnalogInput {
    const char* label;
    uint8_t     gpio;
    HIDAxis     axis;
};

static const AnalogInput AnalogInputs[] = {
    { "COM_AUX",            COM_AUX_KNOB_PIN,       AXIS_SLIDER1 },
    { "COM_ICS",            COM_ICS_KNOB_PIN,       AXIS_SLIDER2 },
    { "COM_MIDS_A",         COM_MIDS_A_KNOB_PIN,    AXIS_CUSTOM1 },
    { "COM_MIDS_B",         COM_MIDS_B_KNOB_PIN,    AXIS_CUSTOM2 },
    { "COM_RWR",            COM_RWR_KNOB_PIN,       AXIS_CUSTOM3 },
    { "COM_TACAN",          COM_TACAN_KNOB_PIN,     AXIS_CUSTOM4 },
    { "COM_VOX",            COM_VOX_KNOB_PIN,       AXIS_DIAL },
    { "COM_WPN",            COM_WPN_KNOB_PIN,       AXIS_RX },
    { "OXY_FLOW",           OXYFLOW_KNOB_PIN,       AXIS_RY },
};

// --- Selector Cache ---
struct SelectorGroupState {
    uint16_t currentOverride = 0xFFFF;
    bool     hasActiveBit = false;
};

static SelectorGroupState selectorStates[MAX_SELECTOR_GROUPS];

// --- Shift Register (HC165) ---
constexpr uint64_t INVERTED_BITS_MASK = 0ULL; // No inverted bits for now
#define HC165_BITS 40

static uint64_t buttonBits = 0xFFFFFFFFFFFFULL;
static uint64_t prevButtonBits = 0xFFFFFFFFFFFFULL;

inline bool isPressedCorrected(uint64_t bits, uint8_t bit) {
    bool logic = ((bits >> bit) & 1);
    if (INVERTED_BITS_MASK & (1ULL << bit)) logic = !logic;
    return logic == 0;
}

void buildGPIOEncoderStates() {
    numEncoders = 0;
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& mi = InputMappings[i];
        if (!mi.label || strcmp(mi.source, "GPIO") != 0) continue;
        // Only pick the 0 (CCW) side as anchor to avoid duplicates
        if (!(strcmp(mi.controlType, "fixed_step") == 0 || strcmp(mi.controlType, "variable_step") == 0)) continue;
        if (mi.oride_value != 0) continue;

        // Find matching pos1 (CW, same label, value==1)
        for (size_t j = 0; j < InputMappingSize; ++j) {
            const InputMapping& mj = InputMappings[j];
            if (&mi == &mj) continue;
            if (!mj.label || strcmp(mj.source, "GPIO") != 0) continue;
            if (strcmp(mi.oride_label, mj.oride_label) != 0) continue;
            if (strcmp(mi.controlType, mj.controlType) != 0) continue;
            if (mj.oride_value != 1) continue;

            if (numEncoders < MAX_ENCODERS) {
                EncoderState& e = gpioEncoders[numEncoders++];
                e.pos0 = &mi;
                e.pos1 = &mj;
                e.pinA = mi.port;
                e.pinB = mj.port;
                pinMode(e.pinA, INPUT_PULLUP);
                pinMode(e.pinB, INPUT_PULLUP);
                uint8_t a = digitalRead(e.pinA), b = digitalRead(e.pinB);
                e.lastState = (a << 1) | b;
                e.accum = 0;
                e.position = 0;
            }
            break; // Only pair once per anchor
        }
    }
}

void leftPanelPollGPIOEncoders() {
    for (uint8_t i = 0; i < numEncoders; ++i) {
        EncoderState& e = gpioEncoders[i];
        uint8_t a = digitalRead(e.pinA), b = digitalRead(e.pinB);
        uint8_t currState = (a << 1) | b;
        uint8_t idx = (e.lastState << 2) | currState;
        int8_t movement = encoder_transition_table[idx];
        if (movement != 0) {
            e.accum += movement;
            if (e.accum >= ENCODER_TICKS_PER_NOTCH) {
                e.position++;
                e.accum = 0;
                // CW step
                if (strcmp(e.pos1->controlType, "fixed_step") == 0)
                    sendCommand(e.pos1->oride_label, "INC");
                else if (strcmp(e.pos1->controlType, "variable_step") == 0)
                    sendCommand(e.pos1->oride_label, "+3200");
            }
            else if (e.accum <= -ENCODER_TICKS_PER_NOTCH) {
                e.position--;
                e.accum = 0;
                // CCW step
                if (strcmp(e.pos0->controlType, "fixed_step") == 0)
                    sendCommand(e.pos0->oride_label, "DEC");
                else if (strcmp(e.pos0->controlType, "variable_step") == 0)
                    sendCommand(e.pos0->oride_label, "-3200");
            }
        }
        e.lastState = currState;
    }
}

void handleDeferredGainCoverClose() {
    if (pendingGainCoverClose) {
        // Only close cover once selector value is observed as POS0
        // If your selector label is different, adjust here!
        if (getLastKnownState("GAIN_SWITCH") == 0) {
            HIDManager_setNamedButton("GAIN_SWITCH_COVER", false, false);
            pendingGainCoverClose = false;
        }
    }
}

void handleDeferredGainPos1() {
    if (pendingGainPos1) {
        // Only set Gain to ORIDE if cover is already open
        if (getLastKnownState("GAIN_SWITCH_COVER") == 1 && (millis() - gainCoverOpenedMs > 100)) {
            HIDManager_setNamedButton("GAIN_SWITCH_POS1", false, true);
            pendingGainPos1 = false;
        }
    }
}

void updateGainSwitch(bool pressed) {
    static bool prevGainPressed = false;

    if (pressed != prevGainPressed) {
        if (pressed) {
            // Button pressed: always open cover, then set RCVY
            HIDManager_setNamedButton("GAIN_SWITCH_COVER", false, pressed);
            gainCoverOpenedMs = millis();       // << store the time!
            pendingGainCoverClose = false; // Cancel any pending close
            pendingGainPos1 = true; // Schedule deferred Pos1 (Gain ORIDE)
        }
        else {
            // Button released: set POS0, and schedule cover close for a later loop
            HIDManager_setNamedButton("GAIN_SWITCH_POS0", false, true);
            pendingGainCoverClose = true; // Schedule deferred cover close
            pendingGainPos1 = false; // defensive flag, only pending task is closing the cover, not setting Pos1
        }
        prevGainPressed = pressed;
    }
}

void LeftPanelButtons_init() {
    // Initialize GPIO selector pins
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
        pinMode(m.port, INPUT_PULLUP);
    }

    buildGPIOEncoderStates();

    // Axes init
    for (const AnalogInput& a : AnalogInputs) {
        HIDManager_moveAxis(a.label, a.gpio, a.axis);
    }

    // HC165 init
    HC165_init(HC165_LEFT_PANEL_CONTROLLER_PL, HC165_LEFT_PANEL_CONTROLLER_CP, HC165_LEFT_PANEL_CONTROLLER_QH, HC165_BITS);
    buttonBits = HC165_read();
    prevButtonBits = buttonBits;

    bool currGain = isPressedCorrected(buttonBits, GAIN_SWITH_BIT);
    if (currGain)
        HIDManager_setNamedButton("GAIN_SWITCH_POS1", true);
    else
        HIDManager_setNamedButton("GAIN_SWITCH_POS0", true);

    // Selector state clear
    for (auto& state : selectorStates) state = {};

    // Initial HC165 selector dispatch
    bool groupActive[MAX_SELECTOR_GROUPS] = { false };

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "HC165") != 0 || m.bit < 0 || m.bit >= 64) continue;
        if (m.group >= MAX_SELECTOR_GROUPS) continue;

        // Skip gain switch handling here, it is done separately
        if (m.label && (strcmp(m.label, "GAIN_SWITCH_POS0") == 0 || strcmp(m.label, "GAIN_SWITCH_POS1") == 0)) continue; // Skip gain switch, handled separately

        if (isPressedCorrected(buttonBits, m.bit)) {
            groupActive[m.group] = true;
            selectorStates[m.group].currentOverride = m.oride_value;
            HIDManager_setNamedButton(m.label, true);
        }
    }

    // Fallback OFFs for HC165 (bit == -1)
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "HC165") != 0 || m.bit != -1) continue;
        if (m.group >= MAX_SELECTOR_GROUPS) continue;

        // Skip gain switch handling here, it is done separately
        if (m.label && (strcmp(m.label, "GAIN_SWITCH_POS0") == 0 || strcmp(m.label, "GAIN_SWITCH_POS1") == 0)) continue; // Skip gain switch, handled separately

        if (!groupActive[m.group]) {
            selectorStates[m.group].currentOverride = m.oride_value;
            HIDManager_setNamedButton(m.label, true);
        }
    }

    debugPrintln("✅ Initialized Left Panel Controller Buttons");
}

void LeftPanelButtons_loop() {
    static unsigned long lastPoll = 0;
    if (!shouldPollMs(lastPoll)) return;

    // Axes
    for (const AnalogInput& a : AnalogInputs) {
        HIDManager_moveAxis(a.label, a.gpio, a.axis);
    }

    // Shift register read
    uint64_t newBits = HC165_read();
    bool changed = (newBits != prevButtonBits);
    buttonBits = newBits;

    bool currGain = isPressedCorrected(buttonBits, GAIN_SWITH_BIT);
    updateGainSwitch(currGain);
    handleDeferredGainCoverClose();
    handleDeferredGainPos1();

    bool groupActive[MAX_SELECTOR_GROUPS] = { false };
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];

        if (!m.label || strcmp(m.source, "HC165") != 0 || m.bit < 0 || m.bit >= 64) continue;
        if (m.group >= MAX_SELECTOR_GROUPS) continue;

		// Skip gain switch handling here, it is done separately
		if (m.label && (strcmp(m.label, "GAIN_SWITCH_POS0") == 0 || strcmp(m.label, "GAIN_SWITCH_POS1") == 0)) continue; // Skip gain switch, handled separately

        bool pressed = isPressedCorrected(buttonBits, m.bit);
        if (strcmp(m.controlType, "momentary") == 0) {
            bool prev = isPressedCorrected(prevButtonBits, m.bit);
            if (pressed != prev) {
                HIDManager_setNamedButton(m.label, false, pressed);
            }
        }
        else if (m.group > 0) {
            if (pressed) {
                groupActive[m.group] = true;
                if (selectorStates[m.group].currentOverride != m.oride_value) {
                    selectorStates[m.group].currentOverride = m.oride_value;
                    HIDManager_setNamedButton(m.label, true);
                }
            }
        }
    }

    // Fallback OFF entries (the ones with bit == -1)
    bool groupFallbackHandled[MAX_SELECTOR_GROUPS] = { false };

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "HC165") != 0 || m.bit != -1) continue;
        if (m.group >= MAX_SELECTOR_GROUPS) continue;

        // Skip gain switch handling here, it is done separately
        if (m.label && (strcmp(m.label, "GAIN_SWITCH_POS0") == 0 || strcmp(m.label, "GAIN_SWITCH_POS1") == 0)) continue; // Skip gain switch, handled separately

        if (!groupActive[m.group] && !groupFallbackHandled[m.group]) {
            if (selectorStates[m.group].currentOverride != m.oride_value) {
                selectorStates[m.group].currentOverride = m.oride_value;
                HIDManager_setNamedButton(m.label, true);
            }
            groupFallbackHandled[m.group] = true;
        }
    }

    if (changed) prevButtonBits = buttonBits;

    leftPanelPollGPIOEncoders();

}