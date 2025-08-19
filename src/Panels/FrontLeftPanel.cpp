#include "../Globals.h"
#if defined(LABEL_SET_FRONT_LEFT_PANEL) || defined(LABEL_SET_ALL)

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

#define MAX_SELECTOR_GROUPS 32

struct AnalogInput {
    const char* label;
    uint8_t     gpio;
    HIDAxis     axis;
};

static const AnalogInput AnalogInputs[] = {
    { "FORMATION_DIMMER", FORMATION_LTS_KNOB_PIN, AXIS_SLIDER1 },
    { "POSITION_DIMMER",  POSITION_LTS_KNOB_PIN,  AXIS_SLIDER2 },
};

struct SelectorGroupState {
    uint16_t currentOverride = 0xFFFF;
};
static SelectorGroupState selectorStates[MAX_SELECTOR_GROUPS];

struct GpioGroupDef {
    uint8_t numPins;
    uint8_t pins[4]; // Max 4 pins per selector group
};
static GpioGroupDef groupDef[MAX_SELECTOR_GROUPS];

// ---- GEN TIE Virtual Cover Logic and deferred Pos1 setting ----
static bool pendingGenTieCoverClose = false;
static bool pendingGenTiePos1 = false;
static unsigned long genTieCoverOpenedMs = 0;

void buildGpioGroupDefs() {
    for (uint16_t g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        groupDef[g].numPins = 0;
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const InputMapping& m = InputMappings[i];
            if (m.group != g || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
            // Add unique pins only
            bool found = false;
            for (uint8_t k = 0; k < groupDef[g].numPins; ++k)
                if (groupDef[g].pins[k] == m.port) found = true;
            if (!found && groupDef[g].numPins < 4)
                groupDef[g].pins[groupDef[g].numPins++] = m.port;
        }
    }
}

void pollFrontLeftGPIOSelectors(bool forceSend = false, const char* caller = nullptr) {
    for (uint16_t g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        const uint8_t n = groupDef[g].numPins;
        if (n == 0) continue;

        uint8_t state = 0;
        for (uint8_t k = 0; k < n; ++k)
            state |= (digitalRead(groupDef[g].pins[k]) == HIGH ? (1 << k) : 0);

        int bestIdx = -1;
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const InputMapping& m = InputMappings[i];
            if (m.group != g || strcmp(m.source, "GPIO") != 0) continue;

            // Skip GEN TIE manual handled mappings
            if (m.label && (
                strcmp(m.label, "GEN_TIE_SW_NORM") == 0 ||
                strcmp(m.label, "GEN_TIE_SW_RESET") == 0
                )) continue;

            // Build the expected pattern for this mapping:
            uint8_t expected = 0;
            for (uint8_t k = 0; k < n; ++k) {
                if (groupDef[g].pins[k] == m.port)
                    expected |= (m.bit ? (1 << k) : 0);
                else
                    expected |= (1 << k); // all other pins HIGH
            }

            if (state == expected) {
                bestIdx = (int)i;
                break;
            }
        }

        if (bestIdx >= 0) {
            const InputMapping& m = InputMappings[bestIdx];
            if (forceSend) {
                selectorStates[g].currentOverride = m.oride_value;
                HIDManager_setNamedButton(m.label, false, true);
            }
            else if (selectorStates[g].currentOverride != m.oride_value) {
                selectorStates[g].currentOverride = m.oride_value;
                HIDManager_setNamedButton(m.label, false, true);
            }
        }
    }
}

void handleDeferredGenTieCoverClose() {
    if (pendingGenTieCoverClose) {
        // Only close cover once selector value is observed as POS0 (NORM)
        if (getLastKnownState("GEN_TIE_SW") == 0) {
            HIDManager_setNamedButton("GEN_TIE_COVER", false, false);
            pendingGenTieCoverClose = false;
        }
    }
}

void handleDeferredGenTiePos1() {
    if (pendingGenTiePos1) {
        // Only set GEN TIE to RESET if cover is already open
        if (getLastKnownState("GEN_TIE_COVER") == 1 && (millis() - genTieCoverOpenedMs > 100)) {
            HIDManager_setNamedButton("GEN_TIE_SW_RESET", false, true);
            pendingGenTiePos1 = false;
        }
    }
}

void updateGenTieSwitch(bool pressed) {
    static bool prevGenTiePressed = false;

    if (pressed != prevGenTiePressed) {
        if (pressed) {
            // Button pressed: always open cover, then set RESET
            HIDManager_setNamedButton("GEN_TIE_COVER", false, true);
            genTieCoverOpenedMs = millis();
            pendingGenTieCoverClose = false;
            pendingGenTiePos1 = true;
        }
        else {
            // Button released: set POS0 (NORM), and schedule cover close
            HIDManager_setNamedButton("GEN_TIE_SW_NORM", false, true);
            pendingGenTieCoverClose = true;
            pendingGenTiePos1 = false;
        }
        prevGenTiePressed = pressed;
    }
}

void FrontLeftPanelButtons_init() {
    // Set all GPIO selector pins to INPUT_PULLUP before any polling
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
        pinMode(m.port, INPUT_PULLUP);
    }
    for (const AnalogInput& a : AnalogInputs)
        HIDManager_moveAxis(a.label, a.gpio, a.axis, true);

    buildGpioGroupDefs();

    // Forced poll with debug
    for (auto& state : selectorStates) state.currentOverride = 0xDEAD;
    pollFrontLeftGPIOSelectors(true, "INIT");

    // Manually handle GEN TIE switch on init based on GPIO 6 state
    if (digitalRead(6) == LOW)
        HIDManager_setNamedButton("GEN_TIE_SW_RESET", true, true);
    else
        HIDManager_setNamedButton("GEN_TIE_SW_NORM", true, true);

    debugPrintln("✅ Initialized Front Left Panel Buttons");
}

void FrontLeftPanelButtons_loop() {
    static unsigned long lastPoll = 0;
    if (!shouldPollMs(lastPoll)) return;

    for (const AnalogInput& a : AnalogInputs)
        HIDManager_moveAxis(a.label, a.gpio, a.axis);

    pollFrontLeftGPIOSelectors(false, "LOOP");

    // GEN TIE logic: LOW=RESET, HIGH=NORM
    bool currGenTiePressed = (digitalRead(6) == LOW);

    updateGenTieSwitch(currGenTiePressed);
    handleDeferredGenTieCoverClose();
    handleDeferredGenTiePos1();
}
#endif // #if defined(LABEL_SET_FRONT_LEFT_PANEL) || defined(LABEL_SET_ALL)