// TEST_ONLY.cpp — tiny, readable template panel
// Uses ONLY: HIDManager_setNamedButton / HIDManager_toggleIfPressed
// Auto-detects GPIO, HC165, PCA9555 from InputMappings.
// Fill the 3 HC165 pin defines and the bit count for your panel.

// ---------- Includes ----------
#include "../Globals.h"
#if defined(LABEL_SET_TEST_ONLY) || defined(LABEL_SET_ALL)

#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"   // for timing helpers like shouldPollMs()

// ---------- HC165 config ----------
#define HC165_BITS 48

// ---------- Quadrature Encoder Logic  ----------
#define MAX_GPIO_ENCODERS 8
#define ENCODER_TICKS_PER_NOTCH 4

static const int8_t encoder_transition_table[16] = {
    0,  -1,   1,   0,
    1,   0,   0,  -1,
   -1,   0,   0,   1,
    0,   1,  -1,   0
};

struct GPIOEncoderState {
    const InputMapping* pos0;   // CCW mapping (oride_value==0)
    const InputMapping* pos1;   // CW mapping (oride_value==1)
    uint8_t pinA, pinB;
    uint8_t lastState;
    int8_t accum;
    int32_t position;
};
static uint8_t encoderPinMask[48] = { 0 }; // GPIO numbers <48
static GPIOEncoderState gpioEncoders[MAX_GPIO_ENCODERS];
static uint8_t numGPIOEncoders = 0;

#define MAX_SELECTOR_GROUPS 32
struct GpioGroupDef {
    uint8_t numPins;
    uint8_t pins[4]; // Up to 4 pins per group (change if you need more)
};
static GpioGroupDef groupDef[MAX_SELECTOR_GROUPS];
static uint16_t gpioSelectorCache[MAX_SELECTOR_GROUPS] = { 0xFFFF };

// Build the per-group pin list based on InputMappings.
static void buildGpioGroupDefs() {
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

/*
static void pollGPIOSelectors(bool forceSend = false, const char* caller = nullptr) {
    (void)caller;
    for (uint16_t g = 1; g < MAX_SELECTOR_GROUPS; ++g) {
        const uint8_t n = groupDef[g].numPins;
        if (n == 0) continue;

        // Build the actual state bitmask for this group
        uint8_t state = 0;
        for (uint8_t k = 0; k < n; ++k)
            state |= (digitalRead(groupDef[g].pins[k]) == HIGH ? (1 << k) : 0);

        int bestIdx = -1;
        // Walk InputMappings to find which mapping matches
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const InputMapping& m = InputMappings[i];
            if (m.group != g || strcmp(m.source, "GPIO") != 0 || strcmp(m.controlType, "selector") != 0) continue;

            // Calculate expected state for this mapping
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

        // Found a matching selector position
        if (bestIdx >= 0) {
            const InputMapping& m = InputMappings[bestIdx];
            if (forceSend || gpioSelectorCache[g] != m.oride_value) {
                gpioSelectorCache[g] = m.oride_value;
                HIDManager_setNamedButton(m.label, false, true);
            }
        }
        else {
            // Fallback: fire port == -1 mapping for this group if nothing matched
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const InputMapping& m = InputMappings[i];
                if (m.group != g || strcmp(m.source, "GPIO") != 0 || m.port != -1 || strcmp(m.controlType, "selector") != 0) continue;
                if (forceSend || gpioSelectorCache[g] != m.oride_value) {
                    gpioSelectorCache[g] = m.oride_value;
                    HIDManager_setNamedButton(m.label, false, true);
                }
            }
        }
    }
}
*/

void pollGPIOSelectors(bool forceSend = false) {
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

static void buildGPIOEncoderStates() {
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

static void pollGPIOEncoders() {
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


// ---------- Minimal helpers ----------
static inline bool startsWith(const char* s, const char* pfx) {
  return s && pfx && strncmp(s, pfx, strlen(pfx)) == 0;
}
static uint8_t hexNib(char c){ return (c>='0'&&c<='9')?c-'0':(c>='a'&&c<='f')?10+c-'a':(c>='A'&&c<='F')?10+c-'A':0; }
static uint8_t parseHexByte(const char* s){ // expects "0xNN"
  if (!s || strlen(s)<3) return 0;
  return (hexNib(s[2])<<4) | hexNib(s[3]);
}

// ---------- PCA9555 tracking ----------
struct PcaState { uint8_t addr; uint8_t p0=0xFF, p1=0xFF; };
static PcaState pcas[8]; static size_t numPcas = 0;

// Build unique PCA list from InputMappings (sources like "PCA_0x22")
static void buildPcaList() {
  bool seen[256] = {false};
  for (size_t i=0;i<InputMappingSize;++i){
    const auto& m = InputMappings[i];
    if (!m.source) continue;
    if (startsWith(m.source, "PCA_0x")) {
      uint8_t a = parseHexByte(m.source+4);
      if (!seen[a] && numPcas < 8) { seen[a]=true; pcas[numPcas++].addr = a; }
    }
  }
}

// ---------- HC165 state ----------
static uint64_t hc165Bits      = ~0ULL;
static uint64_t hc165PrevBits  = ~0ULL;

// Helper struct for automated analog assignment
struct AutoAnalogInput {
    const char* label;
    uint8_t gpio;
    HIDAxis axis;
};
static AutoAnalogInput autoAnalogs[HID_AXIS_COUNT];
static size_t numAutoAnalogs = 0;

void buildAutoAnalogInputs() {
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

// ---------- INIT ----------
void TEST_ONLY_init() {

    // 1) GPIO inputs → INPUT_PULLUP for any mapping with source="GPIO" and port>=0
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (m.source && strcmp(m.source, "GPIO") == 0 && m.port >= 0)
            pinMode(m.port, INPUT_PULLUP);
    }
    buildGPIOEncoderStates();
    buildGpioGroupDefs();

    buildAutoAnalogInputs();
    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        const auto& a = autoAnalogs[i];
        HIDManager_moveAxis(a.label, a.gpio, a.axis, true);
    }

    // 2) HC165 (optional)
    if (HC165_BITS > 0) {
        HC165_init(HC165_RIGHT_PANEL_CONTROLLER_PL, HC165_RIGHT_PANEL_CONTROLLER_CP, HC165_RIGHT_PANEL_CONTROLLER_QH, HC165_BITS);
        hc165Bits = hc165PrevBits = HC165_read();
    }

    // 3) PCA9555 list + first snapshot
    buildPcaList();
    for (size_t i = 0; i < numPcas; ++i) {
        uint8_t p0, p1;
        if (readPCA9555(pcas[i].addr, p0, p1)) {
            pcas[i].p0 = p0;
            pcas[i].p1 = p1;
        }
    }

    // 5a) --- Initial state for HC165 (unchanged) ---
    if (HC165_BITS > 0) {
        bool groupActive[MAX_SELECTOR_GROUPS] = { false };
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const auto& m = InputMappings[i];
            if (!m.label || !m.source || strcmp(m.source, "HC165") != 0) continue;
            if (m.bit >= 0 && m.bit < 64) {
                bool pressed = ((hc165Bits >> m.bit) & 1) == 0; // active LOW convention
                if (strcmp(m.controlType, "momentary") == 0) {
                    HIDManager_setNamedButton(m.label, true, pressed);
                }
                else if (m.group > 0 && pressed) {
                    groupActive[m.group] = true;
                    HIDManager_setNamedButton(m.label, true, true);
                }
            }
        }
        // fallbacks (bit == -1)
        for (size_t i = 0; i < InputMappingSize; ++i) {
            const auto& m = InputMappings[i];
            if (!m.label || !m.source || strcmp(m.source, "HC165") != 0) continue;
            if (m.bit == -1 && m.group > 0 && !groupActive[m.group]) {
                HIDManager_setNamedButton(m.label, true, true);
            }
        }
    }

    // 5b) --- GPIO initial state: ONLY selectors ---
    pollGPIOSelectors(true);

    // 5c) --- Initial state for PCA9555 (unchanged) ---
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || !startsWith(m.source, "PCA_0x")) continue;
        uint8_t a = parseHexByte(m.source + 4);
        // locate state
        const PcaState* ps = nullptr;
        for (size_t k = 0; k < numPcas; ++k)
            if (pcas[k].addr == a) { ps = &pcas[k]; break; }
        if (!ps) continue;
        uint8_t byte = (m.port == 0) ? ps->p0 : ps->p1;
        bool pressed = ((byte >> m.bit) & 1) == 0; // active LOW
        if (strcmp(m.controlType, "momentary") == 0) {
            HIDManager_setNamedButton(m.label, true, pressed);
        }
        else if (m.group > 0 && pressed) {
            HIDManager_setNamedButton(m.label, true, true);
        }
    }

    debugPrintln("✅ TEST_ONLY panel initialized");
}


// ---------- LOOP ----------
void TEST_ONLY_loop() {
    static unsigned long lastPoll = 0;
    if (!shouldPollMs(lastPoll)) return;

    for (size_t i = 0; i < numAutoAnalogs; ++i) {
        const auto& a = autoAnalogs[i];
        HIDManager_moveAxis(a.label, a.gpio, a.axis, false);
    }
    
	// 1) --- GPIO encoders ---
    static bool lastState[256] = { false };
    pollGPIOEncoders();
    pollGPIOSelectors(false);
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || strcmp(m.source, "GPIO") != 0 || m.port < 0) continue;
        if (encoderPinMask[m.port]) continue; // skip encoders
        if (strcmp(m.controlType, "momentary") != 0) continue;

        int pinState = digitalRead(m.port);
        bool isActive = (m.bit == 0) ? (pinState == LOW) : (pinState == HIGH);

        if (isActive != lastState[i]) {
            HIDManager_setNamedButton(m.label, false, isActive ? 1 : 0);
            lastState[i] = isActive;
        }
    }

    // 2) --- HC165 edge handling (unchanged) ---
    if (HC165_BITS > 0) {
        uint64_t bits = HC165_read();
        if (bits != hc165PrevBits) {
            bool groupActive[MAX_SELECTOR_GROUPS] = { false };
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const auto& m = InputMappings[i];
                if (!m.label || !m.source || strcmp(m.source, "HC165") != 0 || m.bit < 0 || m.bit >= 64) continue;
                bool nowPressed = ((bits >> m.bit) & 1) == 0;
                bool prevPressed = ((hc165PrevBits >> m.bit) & 1) == 0;
                if (strcmp(m.controlType, "momentary") == 0) {
                    if (nowPressed != prevPressed) HIDManager_setNamedButton(m.label, false, nowPressed);
                }
                else if (m.group > 0 && nowPressed) {
                    groupActive[m.group] = true;
                    HIDManager_setNamedButton(m.label, false, true);
                }
            }
            // HC165 fallbacks (bit == -1)
            for (size_t i = 0; i < InputMappingSize; ++i) {
                const auto& m = InputMappings[i];
                if (!m.label || !m.source || strcmp(m.source, "HC165") != 0 || m.bit != -1 || m.group == 0) continue;
                if (!groupActive[m.group]) HIDManager_setNamedButton(m.label, false, true);
            }
            hc165PrevBits = bits;
        }
    }

    // 3) --- PCA9555: unchanged ---
    for (size_t i = 0; i < numPcas; ++i) {
        uint8_t p0, p1;
        if (!readPCA9555(pcas[i].addr, p0, p1)) continue;
        if (p0 == pcas[i].p0 && p1 == pcas[i].p1) continue; // no change

        for (size_t j = 0; j < InputMappingSize; ++j) {
            const auto& m = InputMappings[j];
            if (!m.label || !m.source || !startsWith(m.source, "PCA_0x")) continue;
            if (parseHexByte(m.source + 4) != pcas[i].addr) continue;

            uint8_t oldB = (m.port == 0) ? pcas[i].p0 : pcas[i].p1;
            uint8_t newB = (m.port == 0) ? p0 : p1;
            bool prev = ((oldB >> m.bit) & 1) == 0;
            bool now = ((newB >> m.bit) & 1) == 0;

            if (strcmp(m.controlType, "momentary") == 0) {
                if (prev != now) HIDManager_setNamedButton(m.label, false, now);
            }
            else if (m.group > 0 && now && !prev) {
                HIDManager_setNamedButton(m.label, false, true);
            }
        }
        pcas[i].p0 = p0; pcas[i].p1 = p1;
    }
}

#endif // #if defined(LABEL_SET_TEST_ONLY) || defined(LABEL_SET_ALL)