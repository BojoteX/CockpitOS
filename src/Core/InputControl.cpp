// CockpitOS — InputControl.cpp
// Centralized GPIO/PCA/HC165 Input Control

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
// #include "../InputControl.h"

// ===== GPIO INPUT HANDLING=====
#define MAX_GPIO_ENCODERS                8
#define ENCODER_TICKS_PER_NOTCH          4


// ---- CURRENTLY DISABLED -----
// ---- Analog acquisition (EMA + window stats) 
// Up to 64 GPIOs. K=3 → α=1/8 EMA.
namespace AnalogAcq {
    constexpr uint8_t MAX_PINS = 64;
    constexpr uint8_t K = 3; // EMA strength

    struct Slot {
        uint16_t ema = 0;        // running EMA (12-bit domain)
        uint32_t sum = 0;        // window sum
        uint16_t minv = 0x0FFF;  // window min
        uint16_t maxv = 0x0000;  // window max
        uint16_t cnt = 0;        // window count
        bool     boot = false;   // seeded?
    };
    static Slot s[MAX_PINS];

    void sample(uint8_t pin) {
        if (pin >= MAX_PINS) return;
        uint16_t raw = (uint16_t)analogRead(pin);   // single conversion
        Slot& a = s[pin];
        if (!a.boot) { a.boot = true; a.ema = raw; a.minv = raw; a.maxv = raw; }
        // EMA: ema += (raw - ema) >> K  (integer, fast)
        a.ema = (uint16_t)(a.ema + ((int32_t)raw - (int32_t)a.ema) / (1 << K));
        // Window stats for this 250 Hz window
        a.sum += raw; a.cnt++;
        if (raw < a.minv) a.minv = raw;
        if (raw > a.maxv) a.maxv = raw;
    }

    // Consume current window and reset stats. Returns avg/min/max/ema (all 12-bit space).
    void consume(uint8_t pin, uint16_t& avg12, uint16_t& min12, uint16_t& max12, uint16_t& ema12) {
        avg12 = 0; min12 = 0; max12 = 0; ema12 = 0;
        if (pin >= MAX_PINS) return;
        Slot& a = s[pin];
        ema12 = a.ema;
        if (a.cnt) { avg12 = (uint16_t)(a.sum / a.cnt); min12 = a.minv; max12 = a.maxv; }
        else { avg12 = a.ema;                      min12 = a.ema; max12 = a.ema; }
        // reset window for next 250 Hz period
        a.sum = 0; a.cnt = 0; a.minv = 0x0FFF; a.maxv = 0x0000;
    }
}
// ---- CURRENTLY DISABLED -----



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

    // Build a reserved-axis mask from explicit hidId assignments.
    // This prevents auto-assigned axes from colliding with explicit ones
    // when mixing explicit hidId and hidId=-1 (auto) in the same label set.
    // For analogs, hidId >= 0 is valid (0=AXIS_X, 1=AXIS_Y, etc.).
    // Only hidId == -1 means "auto-assign".
    bool reservedAxis[HID_AXIS_COUNT] = {};
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source) continue;
        if (strcmp(m.controlType, "analog") != 0) continue;
        if (m.hidId >= 0 && m.hidId < HID_AXIS_COUNT) {
            reservedAxis[m.hidId] = true;
        }
    }

#if ANALOG_AXIS_DESCENDING
    int axisIdx = HID_AXIS_COUNT - 1;
#define AXIS_AVAIL()   (axisIdx >= 0)
#define NEXT_AXIS()    ((HIDAxis)(axisIdx--))
#else
    int axisIdx = 0;
#define AXIS_AVAIL()   (axisIdx < HID_AXIS_COUNT)
#define NEXT_AXIS()    ((HIDAxis)(axisIdx++))
#endif

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source) continue;
        if (strcmp(m.controlType, "analog") != 0) continue;
        if (m.port < 0) continue;                                   // must be ADC pin
        if (strcmp(m.source, "NONE") == 0) continue;                // skip placeholders
        if (strncmp(m.source, "PCA_0x", 6) == 0) continue;          // not analog

        // If InputMapping specifies a valid hidId (>= 0), use it as the axis.
        // For analogs: hidId 0=AXIS_X, 1=AXIS_Y, ... 15=AXIS_CUSTOM8.
        // Only hidId == -1 means "auto-assign sequentially".
        // This keeps axis assignment consistent with the RS485 master HID gate
        // which also uses hidId for analog→axis mapping.
        HIDAxis axis;
        if (m.hidId >= 0 && m.hidId < HID_AXIS_COUNT) {
            axis = (HIDAxis)m.hidId;
        } else {
            // Skip reserved axes so auto-assign never collides with explicit ones
            while (AXIS_AVAIL() && reservedAxis[axisIdx]) {
#if ANALOG_AXIS_DESCENDING
                axisIdx--;
#else
                axisIdx++;
#endif
            }
            if (!AXIS_AVAIL()) break;
            axis = NEXT_AXIS();
        }

        if (numAutoAnalogs >= MAX_AUTO_ANALOGS) break;
        autoAnalogs[numAutoAnalogs++] = AutoAnalogInput{ m.label, (uint8_t)m.port, axis };
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

#if !ENABLE_PCA9555
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
    memset(lastValSelector, 0xFF, sizeof(lastValSelector)); // init all to -1
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

            // Only process REAL bits for momentaries! (bit is uint8_t, always >= 0)
            if (pin.isMomentary && pin.bit < 8) {
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
                if (pin.bit == 255) { fallback = &pin; continue; }  // 255 = fallback sentinel (uint8_t)

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
        bool anyStrobe = false;
        uint8_t maxIdx = 0;
        for (uint8_t i = 0; i < MAX_MATRIX_STROBES; ++i) {
            if (R.strobes[i] != 0xFF) { if (i > maxIdx || !anyStrobe) maxIdx = i; anyStrobe = true; }
        }
        R.strobeCount = anyStrobe ? (uint8_t)(maxIdx + 1) : 0;

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
//
// Semantics for TM1637 mappings:
//   - source == "TM1637"
//   - port   == DIO pin (LA_DIO_PIN, RA_DIO_PIN, JETT_DIO_PIN, etc.)
//   - bit    == key index 0..15 (decoded from TM1637 scan code)
//
// Low-level (in TM1637.cpp):
//   - tm1637_readKeys(dev) returns 8-bit raw TM1637 key scan code (0xFF = no key)
//
// Here we do:
//   1) Decode raw -> key index using TM1637_KEY_CODES[] (same as standalone).
//   2) Depending on ADVANCED_TM1637_INPUT_FILTERING:
//        - 1: use window-based temporal filtering + multi-key guard (robust).
//        - 0: accept decoded index directly (no advanced filtering; only edge detection).
//   3) Map accepted key index -> InputMapping.bit and emit HID edges.
//

static inline TM1637Device* _resolveTMDev(uint8_t port) {
    return TM1637_findByDIO(port);
}

struct TMKeyMap {
    const char* label;
    uint8_t     devIdx;    // index into tmDevs[]
    uint8_t     keyIndex;  // 0..15 (InputMapping.bit)
};

// Per-device state: tracks which key is currently accepted
struct TMButtonLogger {
    int8_t prevKey = -1;  // previously accepted key index (-1 = none)
    int8_t currentKey = -1;  // current accepted key index (-1 = none)
};

static constexpr uint8_t TM_NONE_INDEX = 16;  // special "no key" index

// Window-based temporal filter per device (only used when ADVANCED == 1)
static constexpr uint8_t TM_WINDOW_SIZE = 8;  // window length
static constexpr uint8_t TM_DOM_ENTER_COUNT = 5;  // require >=5 samples out of 8

struct TMKeyWindow {
    uint8_t buf[TM_WINDOW_SIZE];  // indices 0..16
    uint8_t counts[17];           // 0..15 keys, 16 = none
    uint8_t size = 0;             // number of valid entries (<= TM_WINDOW_SIZE)
    uint8_t head = 0;             // next position to overwrite
};

struct TMDev {
    TM1637Device* dev = nullptr;
    TMButtonLogger logger;
    TMKeyWindow    window;
    bool           present = false;
};

static TMDev    tmDevs[MAX_TM1637_DEV];
static uint8_t  numTMDevs = 0;
static TMKeyMap tmKeys[MAX_TM1637_KEYS];
static uint8_t  numTMKeys = 0;
static bool     tmBuilt = false;

// --- TM1637 decode table (same as your working standalone) ---
static const uint8_t TM1637_KEY_CODES[16] = {
    0xF7,0xF6,0xF5,0xF4,0xF3,0xF2,0xF1,0xF0, // row K1, SG1..8
    0xEF,0xEE,0xED,0xEC,0xEB,0xEA,0xE9,0xE8  // row K2, SG1..8
};

static int8_t TM1637_decodeKey(uint8_t raw) {
    if (raw == 0xFF) return -1; // no key
    for (uint8_t i = 0; i < 16; ++i) {
        if (raw == TM1637_KEY_CODES[i]) return (int8_t)i;
    }
    return -2; // unknown / noise
}

// --- Window helpers (ADVANCED mode) ---

static void TM1637_windowReset(TMKeyWindow& w) {
    w.size = 0;
    w.head = 0;
    for (uint8_t i = 0; i < 17; ++i) w.counts[i] = 0;
}

#if ADVANCED_TM1637_INPUT_FILTERING
static void TM1637_windowAdd(TMKeyWindow& w, uint8_t idx) {
    // idx in 0..16 (16 == none)
    if (w.size < TM_WINDOW_SIZE) {
        w.buf[w.head++] = idx;
        w.counts[idx]++;
        w.size++;
        if (w.head >= TM_WINDOW_SIZE) w.head = 0;
    }
    else {
        uint8_t old = w.buf[w.head];
        if (w.counts[old] > 0) w.counts[old]--;
        w.buf[w.head++] = idx;
        w.counts[idx]++;
        if (w.head >= TM_WINDOW_SIZE) w.head = 0;
    }
}

// Returns:
//   0..16 if some index dominates,
//   -1    if no index meets the threshold.
static int8_t TM1637_windowDominant(const TMKeyWindow& w,
    uint8_t enterThresholdCount) {
    if (w.size == 0) return -1;

    uint8_t bestIdx = 0;
    uint8_t bestCount = 0;
    for (uint8_t i = 0; i < 17; ++i) {
        if (w.counts[i] > bestCount) {
            bestCount = w.counts[i];
            bestIdx = i;
        }
    }
    if (bestCount < enterThresholdCount) {
        return -1;
    }
    return (int8_t)bestIdx;
}
#endif

// --- Build TM1637 mapping from InputMappings[] ---

static inline int8_t _findOrAddTMDev(TM1637Device* dev) {
    if (!dev) return -1;
    for (uint8_t i = 0; i < numTMDevs; i++) {
        if (tmDevs[i].dev == dev) return (int8_t)i;
    }
    if (numTMDevs >= MAX_TM1637_DEV) return -1;
    TMDev& D = tmDevs[numTMDevs];
    D.dev = dev;
    D.present = true;
    D.logger = TMButtonLogger{};
    TM1637_windowReset(D.window);
    return (int8_t)(numTMDevs++);
}

static void _TM1637_build() {
    numTMDevs = 0;
    numTMKeys = 0;

    #if DEBUG_ENABLED_FOR_TM1637_ONLY
    bool hasTmMappings = false;
    #endif

    // 1) Build from InputMappings[] (normal case)
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.source || strcmp(m.source, "TM1637") != 0 || !m.label) continue;
        if (m.port < 0 || m.bit < 0) continue; // only real keys here

        #if DEBUG_ENABLED_FOR_TM1637_ONLY
        hasTmMappings = true;
        #endif

        TM1637Device* dev = _resolveTMDev((uint8_t)m.port);
        const int8_t d = _findOrAddTMDev(dev);
        if (d < 0) continue;
        if (numTMKeys >= MAX_TM1637_KEYS) continue;

        tmKeys[numTMKeys++] = TMKeyMap{ m.label, (uint8_t)d, (uint8_t)m.bit };
    }

#if DEBUG_ENABLED_FOR_TM1637_ONLY
    // 2) Discovery mode: if there are NO TM1637 mappings, build devices from registry
    if (!hasTmMappings) {
        uint8_t count = TM1637_getDeviceCount();
        for (uint8_t i = 0; i < count && numTMDevs < MAX_TM1637_DEV; ++i) {
            TM1637Device* dev = TM1637_getDeviceAt(i);
            if (!dev) continue;

            TMDev& D = tmDevs[numTMDevs++];
            D.dev = dev;
            D.present = true;
            D.logger = TMButtonLogger{};
            TM1637_windowReset(D.window);
        }
    }
#endif

    for (uint8_t d = 0; d < numTMDevs; ++d) {
        TMDev& D = tmDevs[d];
        debugPrintf("  dev[%u]: devPtr=%p present=%d\n",
            d, (void*)D.dev, (int)D.present);
    }
    for (uint8_t k = 0; k < numTMKeys; ++k) {
        const TMKeyMap& M = tmKeys[k];
        debugPrintf("  key[%u]: label=%s devIdx=%u keyIndex=%u\n",
            k, M.label, M.devIdx, M.keyIndex);
    }
}

static inline void _TM1637_buildOnce() {
    if (!tmBuilt) {
        _TM1637_build();
        tmBuilt = true;
    }
}

#if DEBUG_ENABLED_FOR_TM1637_ONLY

// Find an existing InputMapping for this TM1637 DIO + keyIndex (if any)
static const InputMapping* TM1637_findMappingForKey(uint8_t dio, uint8_t keyIndex) {
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.source) continue;
        if (strcmp(m.source, "TM1637") != 0) continue;
        if (m.port != (int8_t)dio) continue;
        if (m.bit != (int8_t)keyIndex) continue;
        return &m;
    }
    return nullptr;
}

// Log key changes for mapping/discovery
static void TM1637_debugLogKeyChange(const TMDev& D, int8_t prevKey, int8_t nowKey) {
    if (!D.dev) return;

    uint8_t dio = D.dev->dioPin;

    // Only log presses (new key became active)
    if (nowKey >= 0 && nowKey < 16) {
        const InputMapping* m = TM1637_findMappingForKey(dio, (uint8_t)nowKey);
        if (m) {
            // Already mapped: show which label fired
            debugPrintf("🔑 TM1637 DIO=%u key#%d (K%u, SG%u) → %s\n",
                dio,
                nowKey,
                (unsigned)(nowKey / 8 + 1),
                (unsigned)(nowKey % 8),
                m->label);
        }
        else {
            // Unmapped: print a template line to add to InputMappings
            debugPrintf("🆕 TM1637 DIO=%u key#%d (K%u, SG%u) PRESSED — no mapping found.\n",
                dio,
                nowKey,
                (unsigned)(nowKey / 8 + 1),
                (unsigned)(nowKey % 8));

            debugPrintf("    Discovery: { \"<LABEL>\", \"TM1637\", PIN(%u), %d, /* hidId */ -1, \"<DCS_CMD>\", <value>, \"momentary\", 0 },\n",
                dio, nowKey);
        }
    }
    // For mapping, presses are enough; releases can be added similarly if desired.
}

#endif // DEBUG_ENABLED_FOR_TM1637_ONLY

// Process one device: read raw, decode, update state, decide if state changed.
static bool TM1637_processDevice(TMDev& D, bool forceSend) {
    if (!D.dev || !D.present) return false;

    // 1) Raw read + decode
    uint8_t raw = tm1637_readKeys(*D.dev);
    int8_t  k = TM1637_decodeKey(raw);  // 0..15, -1 (none), -2 (noise)

    // Map decode to index
    uint8_t idx = TM_NONE_INDEX;          // 16 = "none"
    if (k >= 0 && k < 16) {
        idx = (uint8_t)k;
    }
    else {
        // -1 or -2 → treat as "none"
        idx = TM_NONE_INDEX;
    }

#if ADVANCED_TM1637_INPUT_FILTERING

    // 2A) ADVANCED: window-based temporal filtering + soft multi-key guard

    // Optional debug on forceSend (init / resync)
    if (forceSend) {
        debugPrintf("[TM raw dev %p] raw=0x%02X decode=%d (idx=%u)\n",
            (void*)D.dev, raw, (int)k, idx);
    }

    // Window update
    TM1637_windowAdd(D.window, idx);

    // Dominant index in window
    int8_t dom = TM1637_windowDominant(D.window, TM_DOM_ENTER_COUNT);
    int8_t acceptedIndex = -1;
    if (dom >= 0 && dom <= (int8_t)TM_NONE_INDEX) {
        acceptedIndex = (dom == (int8_t)TM_NONE_INDEX) ? -1 : dom;
    }

    // Soft multi-key guard: if more than one real key (0..15) appears at least 2 times,
    // treat this as ambiguous and hold current state.
    if (!forceSend && D.window.size >= 3) {
        uint8_t strongKeys = 0;
        for (uint8_t i = 0; i < 16; ++i) {   // only real keys, skip "none"
            if (D.window.counts[i] >= 2) {
                strongKeys++;
                if (strongKeys >= 2) break;
            }
        }
        if (strongKeys >= 2) {
            return false;
        }
    }

    TMButtonLogger& L = D.logger;

    if (forceSend) {
        // On forceSend, accept whatever dominantIndex says now
        if (acceptedIndex != L.currentKey) {
            L.prevKey = L.currentKey;
            L.currentKey = acceptedIndex;
            return true;
        }
        return false;
    }

    // Normal path: only change when dominant index differs from currentKey
    if (acceptedIndex != L.currentKey) {
        L.prevKey = L.currentKey;
        L.currentKey = acceptedIndex;
        return true;
    }
    return false;

#else  // !ADVANCED_TM1637_INPUT_FILTERING

    // 2B) SIMPLE: no advanced filtering. Accept decoded index immediately.
    //             We only do edge detection (prev vs curr), no window, no soft guard.

    TMButtonLogger& L = D.logger;
    int8_t acceptedIndex = -1;
    if (idx == TM_NONE_INDEX) {
        acceptedIndex = -1;
    }
    else {
        acceptedIndex = (int8_t)idx; // 0..15
    }

    if (forceSend) {
        if (acceptedIndex != L.currentKey) {
            L.prevKey = L.currentKey;
            L.currentKey = acceptedIndex;
            return true;
        }
        return false;
    }

    if (acceptedIndex != L.currentKey) {
        L.prevKey = L.currentKey;
        L.currentKey = acceptedIndex;
        return true;
    }
    return false;

#endif // ADVANCED_TM1637_INPUT_FILTERING
}

// Main poller
void TM1637_poll(bool forceSend) {
    _TM1637_buildOnce();

    // ~100 Hz cadence
    static uint32_t lastMs = 0;
    const uint32_t now = millis();
    if (!forceSend && (uint32_t)(now - lastMs) < 10) return;
    lastMs = now;

    for (uint8_t d = 0; d < numTMDevs; ++d) {
        TMDev& D = tmDevs[d];
        if (!D.present || !D.dev) continue;

        if (!TM1637_processDevice(D, forceSend)) continue;

        // Debounced state changed on this device
        TMButtonLogger& L = D.logger;
        int8_t nowKey = L.currentKey; // -1 = none, 0..15 = key index
        int8_t prevKey = L.prevKey;

#if DEBUG_ENABLED_FOR_TM1637_ONLY
        TM1637_debugLogKeyChange(D, prevKey, nowKey);
#endif

        // Per-mapping edges
        for (uint8_t k = 0; k < numTMKeys; ++k) {
            const TMKeyMap& M = tmKeys[k];
            if (M.devIdx != d) continue;

            bool prevPressed = (prevKey >= 0 && (uint8_t)prevKey == M.keyIndex);
            bool currPressed = (nowKey >= 0 && (uint8_t)nowKey == M.keyIndex);

            if (forceSend || prevPressed != currPressed) {
                HIDManager_setNamedButton(M.label, forceSend, currPressed);
            }
        }
    }
}

// ===== END TM1637