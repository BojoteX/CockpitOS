// RightPanelController.cpp - Handles the right panel controller buttons and axes
// This implementation uses the InputMappings.h for automated button and selector mappings, axes are managed separately (see AnalogInputs).

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

// --- Configuration ---
#define MAX_SELECTOR_GROUPS 32

// --- Axis Input ---
struct AnalogInput {
    const char* label;
    uint8_t     gpio;
    HIDAxis     axis;
};

static const AnalogInput AnalogInputs[] = {
    { "FLOOD_DIMMER",         FLOOD_DIMMER_KNOB_PIN,       AXIS_SLIDER1 },
    { "INST_PNL_DIMMER",      INST_PNL_DIMMER_KNOB_PIN,    AXIS_SLIDER2 },
    { "SUIT_TEMP",            SUIT_TEMP_KNOB_PIN,          AXIS_CUSTOM1 },
    { "CABIN_TEMP",           CABIN_TEMP_KNOB_PIN,         AXIS_CUSTOM2 },
    { "CONSOLES_DIMMER",      CONSOLES_DIMMER_KNOB_PIN,    AXIS_CUSTOM3 },
    { "WARN_CAUTION_DIMMER",  WARN_CAUTION_DIMMER_KNOB_PIN,AXIS_CUSTOM4 },
    { "CHART_DIMMER",         CHART_DIMMER_KNOB_PIN,       AXIS_DIAL }
};

// --- Selector Cache ---
struct SelectorGroupState {
    uint16_t currentOverride = 0xFFFF;
    bool     hasActiveBit = false;
};

static SelectorGroupState selectorStates[MAX_SELECTOR_GROUPS];

// --- Shift Register (HC165) ---
constexpr uint64_t INVERTED_BITS_MASK = (1ULL << 28) | (1ULL << 29);
#define HC165_BITS 48

static uint64_t buttonBits = 0xFFFFFFFFFFFFULL;
static uint64_t prevButtonBits = 0xFFFFFFFFFFFFULL;

inline bool isPressedCorrected(uint64_t bits, uint8_t bit) {
    bool logic = ((bits >> bit) & 1);
    if (INVERTED_BITS_MASK & (1ULL << bit)) logic = !logic;
    return logic == 0;
}

// --- GPIO Selectors ---
static uint16_t gpioSelectorCache[MAX_SELECTOR_GROUPS] = { 0xFFFF };

void pollGPIOSelectors(bool forceSend = false) {
    bool groupActive[MAX_SELECTOR_GROUPS] = { false };

    // ------------------------------
    // STEP 1: Handle all real pins
    // ------------------------------
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || m.group == 0 || m.group >= MAX_SELECTOR_GROUPS) continue;
        if (strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;

        bool pressed = (digitalRead(m.port) == LOW);
        if (pressed) {
            groupActive[m.group] = true;
            if (forceSend || gpioSelectorCache[m.group] != m.oride_value) {
                gpioSelectorCache[m.group] = m.oride_value;
                HIDManager_setNamedButton(m.label, false, true);
            }
        }
    }

    // ------------------------------
    // STEP 2: Handle fallbacks (port == -1)
    // ------------------------------
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || m.group == 0 || m.group >= MAX_SELECTOR_GROUPS) continue;
        if (strcmp(m.source, "GPIO") != 0 || m.port != -1) continue;

        // If no pin in group was LOW, fallback is used
        if (!groupActive[m.group]) {
            if (forceSend || gpioSelectorCache[m.group] != m.oride_value) {
                gpioSelectorCache[m.group] = m.oride_value;
                HIDManager_setNamedButton(m.label, false, true);
            }
            groupActive[m.group] = true;
        }
    }
}

void RightPanelButtons_init() {
    // Initialize GPIO selector pins
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
        pinMode(m.port, INPUT_PULLUP);
    }

    // Axes init
    for (const AnalogInput& a : AnalogInputs) {
        HIDManager_moveAxis(a.label, a.gpio, a.axis, true);
    }

    // HC165 init
    HC165_init(HC165_RIGHT_PANEL_CONTROLLER_PL, HC165_RIGHT_PANEL_CONTROLLER_CP, HC165_RIGHT_PANEL_CONTROLLER_QH, HC165_BITS);
    buttonBits = HC165_read();
    prevButtonBits = buttonBits;

    // Selector state clear
    for (auto& state : selectorStates) state = {};

    // Initial HC165 selector dispatch
    bool groupActive[MAX_SELECTOR_GROUPS] = { false };

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "HC165") != 0 || m.bit < 0 || m.bit >= 64) continue;
        if (m.group >= MAX_SELECTOR_GROUPS) continue;

        if (isPressedCorrected(buttonBits, m.bit)) {
            groupActive[m.group] = true;
            selectorStates[m.group].currentOverride = m.oride_value;
            HIDManager_setNamedButton(m.label, true, true);
        }
    }

    // Fallback OFFs for HC165 (bit == -1)
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "HC165") != 0 || m.bit != -1) continue;
        if (m.group >= MAX_SELECTOR_GROUPS) continue;

        if (!groupActive[m.group]) {
            selectorStates[m.group].currentOverride = m.oride_value;
            HIDManager_setNamedButton(m.label, true, true);
        }
    }

    // pollGPIOSelectors();
    pollGPIOSelectors(true);
    debugPrintln("✅ Initialized Right Panel Controller Buttons");
}

void RightPanelButtons_loop() {
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

    bool groupActive[MAX_SELECTOR_GROUPS] = { false };

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "HC165") != 0 || m.bit < 0 || m.bit >= 64) continue;
        if (m.group >= MAX_SELECTOR_GROUPS) continue;

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
                    HIDManager_setNamedButton(m.label, false, true);
                }
            }
        }
    }

    // Fallback OFF entries
    bool groupFallbackHandled[MAX_SELECTOR_GROUPS] = { false };

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "HC165") != 0 || m.bit != -1) continue;
        if (m.group >= MAX_SELECTOR_GROUPS) continue;

        if (!groupActive[m.group] && !groupFallbackHandled[m.group]) {
            if (selectorStates[m.group].currentOverride != m.oride_value) {
                selectorStates[m.group].currentOverride = m.oride_value;
                HIDManager_setNamedButton(m.label, false, true);
            }
            groupFallbackHandled[m.group] = true;
        }
    }

    if (changed) prevButtonBits = buttonBits;

    pollGPIOSelectors();
}