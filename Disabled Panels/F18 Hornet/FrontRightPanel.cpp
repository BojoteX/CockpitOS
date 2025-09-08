#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

#define MAX_SELECTOR_GROUPS 32

// ---------- Label->Physical GPIO mapper (S3 only) ----------
#if defined(ESP_FAMILY_S3)
// S3 pins for these labels:
static constexpr uint8_t S3_PIN_RADALT_TEST_SW = 3;
static constexpr uint8_t S3_PIN_RADALT_HEIGHT_CUSTOM_0 = 5; // POS0 (CCW)
static constexpr uint8_t S3_PIN_RADALT_HEIGHT_CUSTOM_1 = 2; // POS1 (CW)
#endif

static inline uint8_t physPinFor(const InputMapping& m)
{
#if defined(ESP_FAMILY_S3)
    if (!m.label) return (uint8_t)m.port;

    // Map by LABEL only:
    if (strcmp(m.label, "RADALT_TEST_SW") == 0)
        return S3_PIN_RADALT_TEST_SW;

    if (strcmp(m.label, "RADALT_HEIGHT_CUSTOM_POS0") == 0)
        return S3_PIN_RADALT_HEIGHT_CUSTOM_0;

    if (strcmp(m.label, "RADALT_HEIGHT_CUSTOM_POS1") == 0)
        return S3_PIN_RADALT_HEIGHT_CUSTOM_1;

    // All other labels use the port as-is.
    return (uint8_t)m.port;
#else
    (void)m;
    return (uint8_t)m.port;     // S2 and others: keep the pins from InputMapping
#endif
}

static inline int fastRead(uint8_t pin) { return digitalRead(pin); }

// ---------- State ----------
struct SelectorGroupState { uint16_t currentOverride = 0xFFFF; };
static SelectorGroupState selectorStates[MAX_SELECTOR_GROUPS];

struct GpioGroupDef { uint8_t numPins; uint8_t pins[4]; };
static GpioGroupDef groupDef[MAX_SELECTOR_GROUPS];

// ---- Momentary GPIO support ----
#define MAX_MOMENTARY 32
struct MomentaryDef {
    int8_t      pin;
    const char* label;
    bool        activeLow;
    bool        lastPressed;
};
static MomentaryDef momentaries[MAX_MOMENTARY];
static uint8_t momentaryCount = 0;

static inline bool strEq(const char* a, const char* b) { return (a && b) ? (strcmp(a, b) == 0) : false; }

// ---- Encoder support (Front Right) ----
#define MAX_FR_ENCODERS 2
#define FR_ENCODER_TICKS_PER_NOTCH 4

static const int8_t fr_enc_transition[16] = {
     0,-1, 1, 0,
     1, 0, 0,-1,
    -1, 0, 0, 1,
     0, 1,-1, 0
};

struct FREncoder {
    const InputMapping* pos0;  // oride_value==0 (CCW)
    const InputMapping* pos1;  // oride_value==1 (CW)
    uint8_t pinA;              // mapped pos0 pin
    uint8_t pinB;              // mapped pos1 pin
    uint8_t lastState;         // 2-bit
    int8_t  accum;
    int32_t pos;
};
static FREncoder frEncoders[MAX_FR_ENCODERS];
static uint8_t frEncCount = 0;

// ---------- Builders ----------
static void buildFrontRightGPIOEncoders() {
    frEncCount = 0;
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& mi = InputMappings[i];
        if (!mi.label || !strEq(mi.source, "GPIO")) continue;
        if (!(strEq(mi.controlType, "fixed_step") || strEq(mi.controlType, "variable_step"))) continue;
        if (mi.oride_value != 0) continue; // anchor on value==0

        for (size_t j = 0; j < InputMappingSize; ++j) {
            const InputMapping& mj = InputMappings[j];
            if (&mi == &mj) continue;
            if (!mj.label || !strEq(mj.source, "GPIO")) continue;
            if (!strEq(mi.oride_label, mj.oride_label)) continue;
            if (!strEq(mi.controlType, mj.controlType)) continue;
            if (mj.oride_value != 1) continue;

            if (frEncCount < MAX_FR_ENCODERS) {
                FREncoder& e = frEncoders[frEncCount++];
                e.pos0 = &mi;
                e.pos1 = &mj;
                e.pinA = physPinFor(mi);
                e.pinB = physPinFor(mj);
                pinMode(e.pinA, INPUT_PULLUP);
                pinMode(e.pinB, INPUT_PULLUP);
                const uint8_t a = (uint8_t)fastRead(e.pinA);
                const uint8_t b = (uint8_t)fastRead(e.pinB);
                e.lastState = (a << 1) | b;
                e.accum = 0;
                e.pos = 0;
            }
            break; // pair once
        }
    }
}

static void buildMomentaryGpioList() {
    momentaryCount = 0;
    for (size_t i = 0; i < InputMappingSize && momentaryCount < MAX_MOMENTARY; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label) continue;
        if (!strEq(m.source, "GPIO")) continue;
        if (m.port < 0) continue;
        if (!strEq(m.controlType, "momentary")) continue;

        MomentaryDef& md = momentaries[momentaryCount++];
        md.pin = (int8_t)physPinFor(m);
        md.label = m.label;
        md.activeLow = true;    // INPUT_PULLUP
        md.lastPressed = false;
    }
}

void buildGpioGroupDefsFrontRightPanel() {
    for (uint16_t g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        groupDef[g].numPins = 0;
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const InputMapping& m = InputMappings[i];
            if (m.group != g || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;

            const uint8_t phys = physPinFor(m);
            bool found = false;
            for (uint8_t k = 0; k < groupDef[g].numPins; ++k) {
                if (groupDef[g].pins[k] == phys) { found = true; break; }
            }
            if (!found && groupDef[g].numPins < 4) {
                groupDef[g].pins[groupDef[g].numPins++] = phys;
            }
        }
    }
}

// ---------- Pollers ----------
static void pollFrontRightGPIOEncoders() {
    for (uint8_t i = 0; i < frEncCount; ++i) {
        FREncoder& e = frEncoders[i];
        const uint8_t a = (uint8_t)fastRead(e.pinA);
        const uint8_t b = (uint8_t)fastRead(e.pinB);
        const uint8_t s = (a << 1) | b;
        const int8_t mv = fr_enc_transition[(e.lastState << 2) | s];

        if (mv) {
            e.accum += mv;
            if (e.accum >= FR_ENCODER_TICKS_PER_NOTCH) {
                e.accum = 0; e.pos++;
                if (strEq(e.pos1->controlType, "fixed_step")) sendCommand(e.pos1->oride_label, "INC");
                else                                         sendCommand(e.pos1->oride_label, "+3200");
            }
            else if (e.accum <= -FR_ENCODER_TICKS_PER_NOTCH) {
                e.accum = 0; e.pos--;
                if (strEq(e.pos0->controlType, "fixed_step")) sendCommand(e.pos0->oride_label, "DEC");
                else                                          sendCommand(e.pos0->oride_label, "-3200");
            }
        }
        e.lastState = s;
    }
}

static void pollFrontRightGPIOMomentaries(bool forceSend) {
    for (uint8_t i = 0; i < momentaryCount; ++i) {
        const int lvl = fastRead((uint8_t)momentaries[i].pin);
        const bool pressed = momentaries[i].activeLow ? (lvl == LOW) : (lvl == HIGH);
        if (forceSend || pressed != momentaries[i].lastPressed) {
            momentaries[i].lastPressed = pressed;
            HIDManager_setNamedButton(momentaries[i].label, /*deferred*/false, /*state*/pressed);
        }
    }
}

void pollFrontRightGPIOSelectors(bool forceSend = false, const char* caller = nullptr) {
    (void)caller;

    for (uint16_t g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        const uint8_t n = groupDef[g].numPins;
        if (n == 0) continue;

        uint8_t state = 0u;
        for (uint8_t k = 0; k < n; ++k) {
            state |= (fastRead(groupDef[g].pins[k]) == HIGH ? (1u << k) : 0u);
        }

        int bestIdx = -1;
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const InputMapping& m = InputMappings[i];
            if (m.group != g || strcmp(m.source, "GPIO") != 0) continue;

            const uint8_t phys_m = physPinFor(m);
            uint8_t expected = 0u;
            for (uint8_t k = 0; k < n; ++k) {
                if (groupDef[g].pins[k] == phys_m) expected |= (m.bit ? (1u << k) : 0u);
                else                               expected |= (1u << k); // all other pins HIGH
            }
            if (state == expected) { bestIdx = (int)i; break; }
        }

        if (bestIdx >= 0) {
            const InputMapping& m = InputMappings[bestIdx];
            if (forceSend || selectorStates[g].currentOverride != m.oride_value) {
                selectorStates[g].currentOverride = m.oride_value;
                HIDManager_setNamedButton(m.label, false, true);
            }
        }
    }
}

// ---------- Init / Loop ----------
void FrontRightPanelButtons_init() {
    // Configure all GPIO inputs with pullups using mapped labels
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (!m.label || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
        pinMode(physPinFor(m), INPUT_PULLUP);
    }

    buildGpioGroupDefsFrontRightPanel();
    buildFrontRightGPIOEncoders();
    buildMomentaryGpioList();

    for (uint16_t g = 0; g < MAX_SELECTOR_GROUPS; ++g) selectorStates[g].currentOverride = 0xDEAD;

    // Initial publish (deferred for momentaries)
    pollFrontRightGPIOSelectors(true, "INIT");
    for (uint8_t i = 0; i < momentaryCount; ++i) {
        const int lvl = fastRead((uint8_t)momentaries[i].pin);
        const bool pressed = momentaries[i].activeLow ? (lvl == LOW) : (lvl == HIGH);
        momentaries[i].lastPressed = pressed;
        HIDManager_setNamedButton(momentaries[i].label, /*deferred*/true, /*state*/pressed);
    }

    debugPrintln("[FrontRight] GPIO init complete");
}

void FrontRightPanelButtons_loop() {
    static unsigned long lastPoll = 0;
    if (!shouldPollMs(lastPoll)) return;

    pollFrontRightGPIOSelectors(false, "LOOP");
    pollFrontRightGPIOMomentaries(false);
    pollFrontRightGPIOEncoders();
}