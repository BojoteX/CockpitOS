// CockpitOS — InputControl.cpp
// Centralized GPIO/PCA/HC165 Input Control

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
// #include "../InputControl.h"

#define MAX_GPIO_ENCODERS              	8
#define ENCODER_TICKS_PER_NOTCH        	4

// Encoder Transition Table
const int8_t encoder_transition_table[16] = {
    0,  -1,   1,   0,   1,   0,   0,  -1,
   -1,   0,   0,   1,   0,   1,  -1,   0
};

// GPIO Encoder State
struct GPIOEncoderState {
    const InputMapping* pos0; // CCW (oride_value==0)
    const InputMapping* pos1; // CW  (oride_value==1)
    uint8_t pinA, pinB, lastState;
    int8_t accum;
    int32_t position;
};
GPIOEncoderState gpioEncoders[MAX_GPIO_ENCODERS];

uint8_t numGPIOEncoders = 0;
uint8_t encoderPinMask[48] = { 0 }; // GPIO numbers <48

// GPIO Selector Groups
struct GpioGroupDef { uint8_t numPins; uint8_t pins[4]; };
GpioGroupDef groupDef[MAX_SELECTOR_GROUPS];
uint16_t gpioSelectorCache[MAX_SELECTOR_GROUPS] = { 0xFFFF };

// Build the per-group pin list based on InputMappings.
void buildGpioGroupDefs() {
    for (uint16_t g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        groupDef[g].numPins = 0;
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const InputMapping& m = InputMappings[i];
            if (m.group != g || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
            // Unique pins only
            bool found = false;
            for (uint8_t k = 0; k < groupDef[g].numPins; ++k)
                if (groupDef[g].pins[k] == m.port) found = true;
            if (!found && groupDef[g].numPins < 4)
                groupDef[g].pins[groupDef[g].numPins++] = m.port;
        }
    }
}

void pollGPIOSelectors(bool forceSend) {
    for (uint16_t g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        // Step 0: Count how many selectors in this group, how many are one-hot
        int total = 0, oneHot = 0;
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const InputMapping& m = InputMappings[i];
            if (!m.label || strcmp(m.source, "GPIO") != 0 || strcmp(m.controlType, "selector") != 0) continue;
            if (m.group != g) continue;
            total++;
            if (m.bit == -1) oneHot++;
        }
        if (total == 0) continue;

        bool groupActive = false;

        // CASE 1: all entries are one-hot style (every entry bit == -1)
        if (oneHot == total) {
            // "One-hot" (one pin per position): First LOW wins
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const InputMapping& m = InputMappings[i];
                if (!m.label || strcmp(m.source, "GPIO") != 0 || strcmp(m.controlType, "selector") != 0) continue;
                if (m.group != g || m.port < 0 || m.bit != -1) continue;
                bool pressed = (digitalRead(m.port) == LOW);
                if (pressed) {
                    if (forceSend || gpioSelectorCache[g] != m.oride_value) {
                        gpioSelectorCache[g] = m.oride_value;
                        HIDManager_setNamedButton(m.label, false, true);
                    }
                    groupActive = true;
                    break; // Only one pin can be LOW
                }
            }
            // Fallback
            if (!groupActive) {
                for (size_t i = 0; i < InputMappingSize; ++i) {
                    const InputMapping& m = InputMappings[i];
                    if (!m.label || strcmp(m.source, "GPIO") != 0 || strcmp(m.controlType, "selector") != 0) continue;
                    if (m.group != g || m.port != -1 || m.bit != -1) continue;
                    if (forceSend || gpioSelectorCache[g] != m.oride_value) {
                        gpioSelectorCache[g] = m.oride_value;
                        HIDManager_setNamedButton(m.label, false, true);
                    }
                    groupActive = true;
                }
            }
        }
        // CASE 2: regular selectors (bit encodes active level)
        else {
            // Regular: For each selector, fire on pin/bit logic
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const InputMapping& m = InputMappings[i];
                if (!m.label || strcmp(m.source, "GPIO") != 0 || strcmp(m.controlType, "selector") != 0) continue;
                if (m.group != g || m.port < 0) continue;
                if (m.bit == -1) continue; // skip one-hot, handled above
                int pinState = digitalRead(m.port);
                bool isActive = (m.bit == 0) ? (pinState == LOW) : (pinState == HIGH);
                if (isActive) {
                    if (forceSend || gpioSelectorCache[g] != m.oride_value) {
                        gpioSelectorCache[g] = m.oride_value;
                        HIDManager_setNamedButton(m.label, false, true);
                    }
                    groupActive = true;
                    break;
                }
            }
            // Fallback for regular
            if (!groupActive) {
                for (size_t i = 0; i < InputMappingSize; ++i) {
                    const InputMapping& m = InputMappings[i];
                    if (!m.label || strcmp(m.source, "GPIO") != 0 || strcmp(m.controlType, "selector") != 0) continue;
                    if (m.group != g || m.port != -1) continue;
                    if (forceSend || gpioSelectorCache[g] != m.oride_value) {
                        gpioSelectorCache[g] = m.oride_value;
                        HIDManager_setNamedButton(m.label, false, true);
                    }
                    groupActive = true;
                }
            }
        }
    }
}

void buildGPIOEncoderStates() {
    numGPIOEncoders = 0;
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& mi = InputMappings[i];
        if (!mi.label || strcmp(mi.source, "GPIO") != 0) continue;
        if (!(strcmp(mi.controlType, "fixed_step") == 0 || strcmp(mi.controlType, "variable_step") == 0)) continue;
        if (mi.oride_value != 0) continue; // only anchor on value==0

        // Find matching pos1 (CW, value==1)
        for (size_t j = 0; j < InputMappingSize; ++j) {
            const InputMapping& mj = InputMappings[j];
            if (&mi == &mj) continue;
            if (!mj.label || strcmp(mj.source, "GPIO") != 0) continue;
            if (strcmp(mi.oride_label, mj.oride_label) != 0) continue;
            if (strcmp(mi.controlType, mj.controlType) != 0) continue;
            if (mj.oride_value != 1) continue;

            if (numGPIOEncoders < MAX_GPIO_ENCODERS) {
                GPIOEncoderState& e = gpioEncoders[numGPIOEncoders++];
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
                encoderPinMask[e.pinA] = 1;
                encoderPinMask[e.pinB] = 1;
            }
            break; // Only pair once per anchor
        }
    }
}

void pollGPIOEncoders() {
    for (uint8_t i = 0; i < numGPIOEncoders; ++i) {
        GPIOEncoderState& e = gpioEncoders[i];
        uint8_t a = digitalRead(e.pinA), b = digitalRead(e.pinB);
        uint8_t currState = (a << 1) | b;
        uint8_t idx = (e.lastState << 2) | currState;
        int8_t movement = encoder_transition_table[idx];

        if (movement != 0) {
            e.accum += movement;
            if (e.accum >= ENCODER_TICKS_PER_NOTCH) {
                e.position++;
                e.accum = 0;
                HIDManager_setNamedButton(e.pos1->label, false, 1);
            }
            else if (e.accum <= -ENCODER_TICKS_PER_NOTCH) {
                e.position--;
                e.accum = 0;
                HIDManager_setNamedButton(e.pos0->label, false, 0);
            }
        }
        e.lastState = currState;
    }
}


AutoAnalogInput autoAnalogs[MAX_AUTO_ANALOGS];
size_t numAutoAnalogs = 0;

void buildAutoAnalogInputs() {
    numAutoAnalogs = 0;

    // Assign axes from AXIS_CUSTOM4 downward (reverse order)
    int axisIdx = HID_AXIS_COUNT - 1; // AXIS_CUSTOM4 is the last in enum

    for (size_t i = 0; i < InputMappingSize && axisIdx >= 0; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source) continue;
        if (strcmp(m.controlType, "analog") != 0) continue;
        if (m.port < 0) continue;                     // must be a real analog pin
        if (strcmp(m.source, "NONE") == 0) continue;  // exclude NONE
        if (strncmp(m.source, "PCA_0x", 6) == 0) continue; // exclude PCA_0xNN
        if (m.port < 0) continue; // Must have a valid analog pin

        autoAnalogs[numAutoAnalogs++] = AutoAnalogInput{ m.label, (uint8_t)m.port, (HIDAxis)axisIdx };
        --axisIdx;
    }
}

void pollGPIOMomentaries() {
    static bool lastGpioMomentaryState[256] = { false };  // Size must fit max possible mappings!
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
}




// PCA Inputs

// PCA9555 Pre-resolved Flat Table
PCA9555Input pca9555Inputs[MAX_PCA9555_INPUTS];
size_t numPca9555Inputs = 0;

PcaState pcas[MAX_PCAS];
size_t numPcas = 0;

// Persistent Flat Poller State
bool    lastStatePCA9555[MAX_PCA9555_INPUTS] = { false };
int16_t lastValSelector[MAX_SELECTOR_GROUPS][MAX_PCAS] = { { -1 } };

// Build unique PCA list from InputMappings (sources like "PCA_0x22")
void buildPcaList() {
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

void buildPCA9555ResolvedInputs() {
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

void pollPCA9555_flat(bool forceSend) {
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


// --- private HC165 types ---
struct HC165Sel {
    int8_t       bit;          // -1 = fallback
    uint16_t     group;
    uint16_t     oride_value;
    const char*  label;
};
struct HC165Mom {
    uint8_t      bit;          // 0..63
    const char*  label;
};

// --- private HC165 tables/state ---
static HC165Sel  hc165Selectors[MAX_SELECTOR_GROUPS * 8];
static size_t    hc165SelCount = 0;
static int16_t   hc165FallbackByGroup[MAX_SELECTOR_GROUPS];   // index into hc165Selectors or -1
static HC165Mom  hc165Momentaries[64];
static size_t    hc165MomCount = 0;

// Latched value per selector group (HC165 path)
static uint16_t  hc165SelectorCache[MAX_SELECTOR_GROUPS];

// --- helpers (private) ---
static inline bool isSel(const InputMapping& m)  { return m.controlType && strcmp(m.controlType, "selector")  == 0; }
static inline bool isMom(const InputMapping& m)  { return m.controlType && strcmp(m.controlType, "momentary") == 0; }
static inline bool isHC165(const InputMapping& m){ return m.source      && strcmp(m.source,      "HC165")     == 0; }

static inline bool hc165Pressed(uint64_t bits, uint8_t bit) {
    const uint64_t b = bits ^ HC165_INVERT_MASK;          // optional per-bit inversion
    return ((b >> bit) & 1ULL) == 0ULL;                   // active-low after correction
}

// --- API ---
void resetHC165SelectorCache() {
    for (int g = 0; g < MAX_SELECTOR_GROUPS; ++g) hc165SelectorCache[g] = 0xFFFF;
}

void buildHC165ResolvedInputs() {
    hc165SelCount = 0;
    hc165MomCount = 0;
    for (int g = 0; g < MAX_SELECTOR_GROUPS; ++g) hc165FallbackByGroup[g] = -1;
    resetHC165SelectorCache();

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
            hc165Selectors[hc165SelCount] = HC165Sel{ (int8_t)m.bit, m.group, m.oride_value, m.label };
            if (m.bit == -1 && hc165FallbackByGroup[m.group] == -1)
                hc165FallbackByGroup[m.group] = (int16_t)hc165SelCount;
            ++hc165SelCount;
        }
    }
}

void processHC165Resolved(uint64_t currentBits, uint64_t previousBits, bool forceSend) {
    // 1) Momentaries / edges
    for (size_t i = 0; i < hc165MomCount; ++i) {
        const auto& m = hc165Momentaries[i];
        const bool now  = hc165Pressed(currentBits,  m.bit);
        const bool prev = hc165Pressed(previousBits, m.bit);
        if (forceSend || now != prev) HIDManager_setNamedButton(m.label, forceSend, now);
    }

    // 2) Selectors: decide winner per group then emit once
    const HC165Sel* winner[MAX_SELECTOR_GROUPS] = { nullptr };
    bool groupHasReal[MAX_SELECTOR_GROUPS] = { false };

    // Pass A: detect pressed real pins
    for (size_t i = 0; i < hc165SelCount; ++i) {
        const auto& e = hc165Selectors[i];
        if (e.bit < 0) continue;
        if (hc165Pressed(currentBits, (uint8_t)e.bit)) {
            if (!winner[e.group]) winner[e.group] = &e;     // first in table order wins
            groupHasReal[e.group] = true;
        }
    }

    // Pass B: prefer winner else fallback
    for (int g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        const HC165Sel* pick = winner[g];
        if (!pick && !groupHasReal[g]) {
            const int16_t idx = hc165FallbackByGroup[g];
            if (idx >= 0) pick = &hc165Selectors[idx];
        }
        if (!pick) continue;

        if (forceSend || hc165SelectorCache[g] != pick->oride_value) {
            hc165SelectorCache[g] = pick->oride_value;
            HIDManager_setNamedButton(pick->label, forceSend, true);
        }
    }
}