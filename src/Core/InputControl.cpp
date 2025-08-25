// CockpitOS — InputControl.cpp
// Centralized GPIO/PCA/HC165 Input Control

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

// ===== GPIO INPUT HANDLING=====
#define MAX_GPIO_ENCODERS                8
#define ENCODER_TICKS_PER_NOTCH          4

// Encoder Transition Table
static const int8_t encoder_transition_table[16] = {
     0, -1,  1,  0,  1,  0,  0, -1,
    -1,  0,  0,  1,  0,  1, -1,  0
};

// GPIO Encoder State
struct GPIOEncoderState {
    const InputMapping* pos0;   // CCW (oride_value==0)
    const InputMapping* pos1;   // CW  (oride_value==1)
    uint8_t pinA, pinB, lastState;
    int8_t  accum;
    int32_t position;
};

static GPIOEncoderState gpioEncoders[MAX_GPIO_ENCODERS];

// Mark pins consumed by encoders so we skip them in momentary polling
uint8_t          numGPIOEncoders = 0;
uint8_t encoderPinMask[48] = { 0 }; // GPIO numbers < 48

// GPIO Selector Groups
struct GpioGroupDef { uint8_t numPins; uint8_t pins[4]; };
static GpioGroupDef groupDef[MAX_SELECTOR_GROUPS];

// Per-group selector cache (oride_value). 0xFFFF = unknown.
static uint16_t gpioSelectorCache[MAX_SELECTOR_GROUPS];

// One-time GPIO input init
static uint8_t gpioInputsInitialized = 0;
static uint8_t gpioPinConfigured[48] = { 0 };

static inline void resetGPIOSelectorCache() {
    for (uint16_t g = 0; g < MAX_SELECTOR_GROUPS; ++g) gpioSelectorCache[g] = 0xFFFFu;
}

static void initGPIOInputsOnce() {
    if (gpioInputsInitialized) return;
    gpioInputsInitialized = 1;

    resetGPIOSelectorCache();

    // Configure pulls for all GPIO selectors & momentaries (unique pins)
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || strcmp(m.source, "GPIO") != 0) continue;
        if (m.port < 0 || m.port >= 48) continue;
        if (encoderPinMask[m.port]) continue; // encoder pins already configured
        const bool isSelector = (m.controlType && strcmp(m.controlType, "selector") == 0);
        const bool isMomentary = (m.controlType && strcmp(m.controlType, "momentary") == 0);
        if (!(isSelector || isMomentary)) continue;
        if (gpioPinConfigured[m.port]) continue;

        // Pull policy:
        //  - Selectors: bit==-1 or bit==0 => active-low => PULLUP; bit==1 => active-high => PULLDOWN
        //  - Momentaries: bit==0 => active-low => PULLUP; bit==1 => active-high => PULLDOWN
        bool usePullUp = true;
        if (isSelector) {
            usePullUp = (m.bit <= 0);     // -1 or 0 => pullup, 1 => pulldown
        }
        else { // momentary
            usePullUp = (m.bit == 0);
        }

        pinMode((uint8_t)m.port, usePullUp ? INPUT_PULLUP : INPUT_PULLDOWN);
        gpioPinConfigured[m.port] = 1;
    }
}

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
                if (groupDef[g].pins[k] == m.port) { found = true; break; }
            if (!found && groupDef[g].numPins < 4)
                groupDef[g].pins[groupDef[g].numPins++] = m.port;
        }
    }
}

void buildGPIOEncoderStates() {
    numGPIOEncoders = 0;
    for (uint8_t p = 0; p < 48; ++p) encoderPinMask[p] = 0;

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& mi = InputMappings[i];
        if (!mi.label || strcmp(mi.source, "GPIO") != 0) continue;
        if (!(strcmp(mi.controlType, "fixed_step") == 0 || strcmp(mi.controlType, "variable_step") == 0)) continue;
        if (mi.oride_value != 0) continue; // anchor on value==0

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
                e.pinA = (uint8_t)mi.port;
                e.pinB = (uint8_t)mj.port;
                pinMode(e.pinA, INPUT_PULLUP);
                pinMode(e.pinB, INPUT_PULLUP);
                uint8_t a = (uint8_t)digitalRead(e.pinA);
                uint8_t b = (uint8_t)digitalRead(e.pinB);
                e.lastState = (uint8_t)((a << 1) | b);
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
        uint8_t a = (uint8_t)digitalRead(e.pinA);
        uint8_t b = (uint8_t)digitalRead(e.pinB);
        uint8_t currState = (uint8_t)((a << 1) | b);
        uint8_t idx = (uint8_t)((e.lastState << 2) | currState);
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

void pollGPIOSelectors(bool forceSend) {
    initGPIOInputsOnce();

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
            // "One-hot" (one pin per position): first LOW wins
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const InputMapping& m = InputMappings[i];
                if (!m.label || strcmp(m.source, "GPIO") != 0 || strcmp(m.controlType, "selector") != 0) continue;
                if (m.group != g || m.port < 0 || m.bit != -1) continue;
                bool pressed = (digitalRead(m.port) == LOW);
                if (pressed) {
                    if (forceSend || gpioSelectorCache[g] != (uint16_t)m.oride_value) {
                        gpioSelectorCache[g] = (uint16_t)m.oride_value;
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
                    if (forceSend || gpioSelectorCache[g] != (uint16_t)m.oride_value) {
                        gpioSelectorCache[g] = (uint16_t)m.oride_value;
                        HIDManager_setNamedButton(m.label, false, true);
                    }
                    groupActive = true;
                    break; // ensure single fallback emit
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
                    if (forceSend || gpioSelectorCache[g] != (uint16_t)m.oride_value) {
                        gpioSelectorCache[g] = (uint16_t)m.oride_value;
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
                    if (forceSend || gpioSelectorCache[g] != (uint16_t)m.oride_value) {
                        gpioSelectorCache[g] = (uint16_t)m.oride_value;
                        HIDManager_setNamedButton(m.label, false, true);
                    }
                    groupActive = true;
                    break; // ensure single fallback emit
                }
            }
        }
    }
}

void pollGPIOMomentaries(bool forceSend) {
    initGPIOInputsOnce();
    static bool lastGpioMomentaryState[256] = { false };

    for (size_t i = 0; i < InputMappingSize && i < 256; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
        if (encoderPinMask[m.port]) continue;
        if (strcmp(m.controlType, "momentary") != 0) continue;

        int pinState = digitalRead(m.port);
        bool isActive = (m.bit == 0) ? (pinState == LOW) : (pinState == HIGH);

        if (forceSend || isActive != lastGpioMomentaryState[i]) {
            HIDManager_setNamedButton(m.label, forceSend, isActive ? 1 : 0);
            lastGpioMomentaryState[i] = isActive;
        }
    }
}

/*
void pollGPIOMomentaries() {
    initGPIOInputsOnce();

    // Size must fit max possible mappings. Bump if your generator grows.
    static bool lastGpioMomentaryState[256] = { false };

    for (size_t i = 0; i < InputMappingSize && i < 256; ++i) {
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
*/

// ----- Auto-analogs (unchanged except early exits) -----

AutoAnalogInput autoAnalogs[MAX_AUTO_ANALOGS];
size_t numAutoAnalogs = 0;

#define ANALOG_AXIS_DESCENDING 0   // 1 = last→first, 0 = first→last

void buildAutoAnalogInputs() {
    numAutoAnalogs = 0;

#if ANALOG_AXIS_DESCENDING
    int axisIdx = HID_AXIS_COUNT - 1;
#define AXIS_AVAIL()   (axisIdx >= 0)
#define NEXT_AXIS()    ((HIDAxis)(axisIdx--))
#else
    int axisIdx = 0;
#define AXIS_AVAIL()   (axisIdx < HID_AXIS_COUNT)
#define NEXT_AXIS()    ((HIDAxis)(axisIdx++))
#endif

    for (size_t i = 0; i < InputMappingSize && AXIS_AVAIL(); ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source) continue;
        if (strcmp(m.controlType, "analog") != 0) continue;
        if (m.port < 0) continue;                                   // must be ADC pin
        if (strcmp(m.source, "NONE") == 0) continue;                // skip placeholders
        if (strncmp(m.source, "PCA_0x", 6) == 0) continue;          // not analog

        autoAnalogs[numAutoAnalogs++] = AutoAnalogInput{ m.label, (uint8_t)m.port, NEXT_AXIS() };
    }

#undef AXIS_AVAIL
#undef NEXT_AXIS
}

/*
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

        autoAnalogs[numAutoAnalogs++] = AutoAnalogInput{ m.label, (uint8_t)m.port, (HIDAxis)axisIdx };
        --axisIdx;
    }
}
*/
// ===== END GPIO INPUT HANDLING =====




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


// ===== MATRIX rotary (strobe/data) — generic, uses ONLY source/port/bit =====
// Rules per row in InputMappings:
//  - source="MATRIX"
//  - One multi-bit row: port=dataPin, bit=all-ones mask (e.g., 15 for 4 strobes)  → anchors data pin.
//  - One row per position: port=strobe GPIO, bit=(1<<strobe_index) in decimal.
//  - Optional fallback: bit=-1, port=dataPin.

struct MatrixPos {
    uint8_t     pattern;      // from mapping.bit (decimal). -1 handled via fallbackIdx
    const char* label;        // selector label to emit
};

struct MatrixRotary {
    const char* family;       // label prefix (text before last '_') — pointer into existing literal
    uint8_t     familyLen;

    uint8_t     dataPin;                      // inferred from multi-bit or fallback row's port
    uint8_t     strobeCount;
    uint8_t     strobes[MAX_MATRIX_STROBES];  // index = bit position; value = GPIO, 0xFF if unknown

    MatrixPos   pos[MAX_MATRIX_POS];
    uint8_t     posCount;
    int8_t      fallbackIdx;                  // row with bit == -1, else -1

    uint8_t     lastPattern;                  // 0xFE = unknown
    bool        configured;                   // GPIO configured
};

static MatrixRotary g_matrix[MAX_MATRIX_ROTARIES];
static uint8_t      g_matCount = 0;
static bool         g_matBuilt = false;

static inline uint8_t _lastUnderscore(const char* s) {
    uint8_t p = 0, i = 0; while (s[i]) { if (s[i] == '_') p = i; ++i; if (i == 255) break; } return p;
}
static inline bool _sameKey(const char* a, uint8_t alen, const char* b, uint8_t blen) {
    if (alen != blen) return false; for (uint8_t i = 0; i < alen; ++i) if (a[i] != b[i]) return false; return true;
}
static inline uint8_t _popcount8(uint8_t x) {
    x = x - ((x >> 1) & 0x55u); x = (x & 0x33u) + ((x >> 2) & 0x33u); return (uint8_t)((x + (x >> 4)) & 0x0Fu);
}
static inline int8_t _oneBitIndex(uint8_t x) {
    if (x == 0u || (x & (uint8_t)(x - 1u))) return -1; uint8_t i = 0; while (((x >> i) & 1u) == 0u) ++i; return (int8_t)i;
}
static inline uint8_t _scanPattern(const uint8_t* strobes, uint8_t n, uint8_t dataPin) {
    uint8_t pat = 0;
    for (uint8_t i = 0; i < n; ++i) {
        const uint8_t s = strobes[i]; if (s == 0xFF) continue;
        digitalWrite(s, LOW);
        delayMicroseconds(1);
        if (digitalRead(dataPin) == LOW) pat |= (uint8_t)(1u << i);   // LOW-active sense
        digitalWrite(s, HIGH);
    }
    return pat;
}

static void Matrix_buildOnce() {
    if (g_matBuilt) return;
    g_matBuilt = true;

    // Discover unique families
    for (size_t i = 0; i < InputMappingSize && g_matCount < MAX_MATRIX_ROTARIES; ++i) {
        const auto& m = InputMappings[i];
        if (!m.source || strcmp(m.source, "MATRIX") != 0 || !m.label) continue;
        const uint8_t cut = _lastUnderscore(m.label); if (cut == 0) continue;
        bool found = false;
        for (uint8_t r = 0; r < g_matCount; ++r)
            if (_sameKey(g_matrix[r].family, g_matrix[r].familyLen, m.label, cut)) { found = true; break; }
        if (found) continue;

        MatrixRotary& R = g_matrix[g_matCount++];
        R.family = m.label; R.familyLen = cut;
        R.dataPin = 0xFF; R.strobeCount = 0; for (uint8_t k = 0; k < MAX_MATRIX_STROBES; ++k) R.strobes[k] = 0xFF;
        R.posCount = 0; R.fallbackIdx = -1; R.lastPattern = 0xFE; R.configured = false;
    }

    // Build each family: collect positions and infer pins
    for (uint8_t r = 0; r < g_matCount; ++r) {
        MatrixRotary& R = g_matrix[r];

        for (size_t i = 0; i < InputMappingSize; ++i) {
            const auto& m = InputMappings[i];
            if (!m.source || strcmp(m.source, "MATRIX") != 0 || !m.label) continue;
            const uint8_t cut = _lastUnderscore(m.label);
            if (!_sameKey(R.family, R.familyLen, m.label, cut)) continue;

            // Collect label+pattern from bit
            if (R.posCount < MAX_MATRIX_POS) {
                if ((int8_t)m.bit < 0 && R.fallbackIdx < 0) R.fallbackIdx = (int8_t)R.posCount;
                R.pos[R.posCount++] = MatrixPos{ (uint8_t)m.bit, m.label };
            }

            // Infer data pin from multi-bit OR fallback row
            if (m.port >= 0) {
                const uint8_t pc = _popcount8((uint8_t)m.bit);
                if ((pc > 1 && R.dataPin == 0xFF) || ((int16_t)m.bit < 0 && R.dataPin == 0xFF))
                    R.dataPin = (uint8_t)m.port;
            }

            // Infer strobe pin from one-bit rows
            if (m.port >= 0) {
                const int8_t idx = _oneBitIndex((uint8_t)m.bit);
                if (idx >= 0 && idx < (int8_t)MAX_MATRIX_STROBES) R.strobes[(uint8_t)idx] = (uint8_t)m.port;
            }
        }

        // Finalize strobe count
        uint8_t maxIdx = 0; for (uint8_t i = 0; i < MAX_MATRIX_STROBES; ++i) if (R.strobes[i] != 0xFF && i > maxIdx) maxIdx = i;
        R.strobeCount = (uint8_t)(maxIdx + 1);

        // If data pin still unknown, grab any port from family
        if (R.dataPin == 0xFF) {
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const auto& m = InputMappings[i];
                if (!m.source || strcmp(m.source, "MATRIX") != 0 || !m.label) continue;
                const uint8_t cut = _lastUnderscore(m.label);
                if (!_sameKey(R.family, R.familyLen, m.label, cut)) continue;
                if (m.port >= 0) { R.dataPin = (uint8_t)m.port; break; }
            }
        }

        // GPIO configure once
        if (R.dataPin != 0xFF) {
            pinMode(R.dataPin, INPUT_PULLUP);
            for (uint8_t i = 0; i < R.strobeCount; ++i) {
                const uint8_t s = R.strobes[i]; if (s == 0xFF) continue;
                pinMode(s, OUTPUT); digitalWrite(s, HIGH);
            }
            R.configured = true;
        }
    }
}

void Matrix_poll(bool forceSend) {
    Matrix_buildOnce();  // lazy build from InputMappings[]

    for (uint8_t r = 0; r < g_matCount; ++r) {
        MatrixRotary& R = g_matrix[r];
        if (!R.configured || !R.posCount || !R.strobeCount) continue;

        const uint8_t pat = _scanPattern(R.strobes, R.strobeCount, R.dataPin);
        if (!forceSend && pat == R.lastPattern) continue;

        int match = -1;
        for (uint8_t i = 0; i < R.posCount; ++i) { if (R.pos[i].pattern == pat) { match = (int)i; break; } }
        if (match < 0) match = R.fallbackIdx;
        if (match >= 0) HIDManager_setNamedButton(R.pos[match].label, forceSend, true);
        R.lastPattern = pat;
    }
}
// ===== END MATRIX

// ===== TM1637 momentary keys — generic, uses ONLY source/port/bit =====
// Rules per row in InputMappings:
//   source="TM1637", port = LA_DIO_PIN or RA_DIO_PIN, bit = key index 0..7.
// Pressed is active-LOW: (finalKeys & (1<<bit)) == 0.  :contentReference[oaicite:1]{index=1}

// Forward declarations from your driver
// struct TM1637Device;
// extern TM1637Device LA_Device;
// extern TM1637Device RA_Device;
// extern bool    tm1637_handleSamplingWindow(TM1637Device& dev, uint16_t& sampleCounter, uint8_t& finalKeys);  // debounced
// extern uint8_t tm1637_readKeys(TM1637Device& dev);                                                             // raw      :contentReference[oaicite:2]{index=2}

// RA/LA pin IDs are macros in your project; used to resolve device by port
static inline TM1637Device* _resolveTMDev(uint8_t port) {
#ifdef RA_DIO_PIN
    if (port == (uint8_t)RA_DIO_PIN) return &RA_Device;
#endif
#ifdef LA_DIO_PIN
    if (port == (uint8_t)LA_DIO_PIN) return &LA_Device;
#endif
    return nullptr;
}

struct TMKeyMap { const char* label; uint8_t devIdx; uint8_t bitIdx; };
struct TMDev { TM1637Device* dev; uint8_t prevKeys; uint16_t sampleCtr; bool present; };

static TMDev    tmDevs[MAX_TM1637_DEV];
static uint8_t  numTMDevs = 0;
static TMKeyMap tmKeys[MAX_TM1637_KEYS];
static uint8_t  numTMKeys = 0;
static bool     tmBuilt = false;

static inline int8_t _findOrAddTMDev(TM1637Device* dev) {
    if (!dev) return -1;
    for (uint8_t i = 0; i < numTMDevs; i++) if (tmDevs[i].dev == dev) return (int8_t)i;
    if (numTMDevs >= MAX_TM1637_DEV) return -1;
    tmDevs[numTMDevs] = TMDev{ dev, 0xFFu, 0u, true };
    return (int8_t)(numTMDevs++);
}

static void _TM1637_buildOnce() {
    if (tmBuilt) return;
    tmBuilt = true;
    numTMDevs = 0; numTMKeys = 0;

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.source || strcmp(m.source, "TM1637") != 0 || !m.label) continue;
        if (m.port < 0 || m.bit < 0) continue;                 // only real keys here
        TM1637Device* dev = _resolveTMDev((uint8_t)m.port);
        const int8_t d = _findOrAddTMDev(dev);
        if (d < 0) continue;
        if (numTMKeys >= MAX_TM1637_KEYS) continue;
        tmKeys[numTMKeys++] = TMKeyMap{ m.label, (uint8_t)d, (uint8_t)m.bit };
    }
}

void TM1637_poll(bool forceSend) {
    _TM1637_buildOnce();

    // match LA/RA cadence (~100 Hz)
    static uint32_t lastMs = 0;
    const uint32_t now = millis();
    if (!forceSend && (uint32_t)(now - lastMs) < 10) return;
    lastMs = now;

    for (uint8_t d = 0; d < numTMDevs; ++d) {
        TMDev& D = tmDevs[d];
        if (!D.present || !D.dev) continue;

        if (forceSend) {
            // init behavior = panels: raw read once, set baseline, emit current state
            const uint8_t raw = tm1637_readKeys(*D.dev);
            for (uint8_t k = 0; k < numTMKeys; ++k) {
                const TMKeyMap& M = tmKeys[k];
                if (M.devIdx != d) continue;
                const uint8_t mask = (uint8_t)(1u << M.bitIdx);
                const bool pressed = ((raw & mask) == 0); // active-LOW
                HIDManager_setNamedButton(M.label, true, pressed);
            }
            D.prevKeys = raw;
            continue;
        }

        uint8_t finalKeys = D.prevKeys;
        if (!tm1637_handleSamplingWindow(*D.dev, D.sampleCtr, finalKeys)) continue;  // wait for debounced byte 

        if (finalKeys == D.prevKeys) continue; // byte-level gate like LA/RA 

        // per-key edges, active-LOW like reference
        for (uint8_t k = 0; k < numTMKeys; ++k) {
            const TMKeyMap& M = tmKeys[k];
            if (M.devIdx != d) continue;
            const uint8_t mask = (uint8_t)(1u << M.bitIdx);
            const bool prevPressed = ((D.prevKeys & mask) == 0);
            const bool currPressed = ((finalKeys & mask) == 0); // e.g., LEFT bit3, RIGHT bit0 
            if (currPressed != prevPressed) {
                HIDManager_setNamedButton(M.label, false, currPressed);
            }
        }
        D.prevKeys = finalKeys;
    }
}
// ===== END TM1637
