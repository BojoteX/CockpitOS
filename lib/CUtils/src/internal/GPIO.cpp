// ************************************************************
// GPIO - Helper functions for all GPIO related Inputs/Outputs
// ************************************************************

#define MAX_GPIO_ENCODERS              		8
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





















// Init all gauges
void resetAllGauges() {
    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        const auto& led = panelLEDs[i];
        if (led.deviceType == DEVICE_GAUGE) {
            hasGauge = true;
            AnalogG_registerGauge(led.info.gaugeInfo.gpio, led.info.gaugeInfo.minPulse, led.info.gaugeInfo.maxPulse);
            debugPrintf("[GAUGE] Registered Gauge %s on PIN %u\n", led.label, led.info.gaugeInfo.gpio);

            // Optional: move gauge to default by setting value and calling AnalogG_tick repeatedly
            for (int t = 0; t < 100; ++t) { // 100 pulses = 2 seconds at 20ms per pulse
                AnalogG_pulseUs(led.info.gaugeInfo.gpio, led.info.gaugeInfo.minPulse, led.info.gaugeInfo.maxPulse, 65535); // max position
                delay(led.info.gaugeInfo.period/1000); 
            }
            for (int t = 0; t < 100; ++t) { // 100 pulses = 2 seconds at 20ms per pulse
                AnalogG_pulseUs(led.info.gaugeInfo.gpio, led.info.gaugeInfo.minPulse, led.info.gaugeInfo.maxPulse, 0); // min position
                delay(led.info.gaugeInfo.period / 1000); 
            }

        }
    }
    if (hasGauge) debugPrintln("[GAUGE] Analog gauges will update automatically.");
	PanelRegistry_setActive(PanelKind::AnalogGauge, hasGauge); // register if any gauges are present
}

void preconfigureGPIO() {
    resetAllGauges(); // if present
    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        const auto& led = panelLEDs[i];
        if (led.deviceType == DEVICE_GPIO || led.deviceType == DEVICE_GAUGE) {
            uint8_t pin = (led.deviceType == DEVICE_GPIO) ? led.info.gpioInfo.gpio
                                                          : led.info.gaugeInfo.gpio;
            pinMode(pin, OUTPUT);

            if (led.deviceType == DEVICE_GPIO) {
                // Set initial state for LEDs only
                bool isAnalog = led.dimmable;
                if (isAnalog) {
                    uint8_t value = led.activeLow ? 255 : 0;
                    analogWrite(pin, value);
                    if (DEBUG) debugPrintf("[INIT] GPIO LED   %-20s (GPIO %2d, PWM)   -> OUTPUT, PWM=%3d (OFF, %s)\n",
                        led.label, pin, value, led.activeLow ? "activeLow" : "activeHigh");
                } else {
                    uint8_t offLevel = led.activeLow ? HIGH : LOW;
                    digitalWrite(pin, offLevel);
                    if (DEBUG) debugPrintf("[INIT] GPIO LED   %-20s (GPIO %2d, Digital)-> OUTPUT, %s (OFF, %s)\n",
                        led.label, pin, led.activeLow ? "HIGH" : "LOW", led.activeLow ? "activeLow" : "activeHigh");
                }
            } else if (led.deviceType == DEVICE_GAUGE) {
                if (DEBUG) debugPrintf("[INIT] GAUGE      %-20s (GPIO %2d)         -> OUTPUT (servo)\n",
                    led.label, pin);
                // Optionally move gauge to safe position here if you wish
            }
        }
    }
}

void GPIO_setAllLEDs(bool state) {
    if (state)
        debugPrintln("🔆 Turning ALL GPIO LEDs ON");
    else
        debugPrintln("⚫ Turning ALL GPIO LEDs OFF");

    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        const auto& led = panelLEDs[i];
        if (led.deviceType == DEVICE_GPIO) {
            uint8_t pin = led.info.gpioInfo.gpio;
            bool isAnalog = led.dimmable;

	   if (isAnalog) {
              uint8_t pwm = state ? (led.activeLow ? 0 : 255)
                        : (led.activeLow ? 255 : 0);
    			analogWrite(pin, pwm);
    		if (DEBUG) debugPrintf("[LED GPIO] %-20s (GPIO %2d, PWM) -> PWM=%3d (%s)\n",
        	led.label, pin, pwm, state ? "ON" : "OFF");
	   } else {
                uint8_t level = (state ^ led.activeLow) ? HIGH : LOW;
                digitalWrite(pin, level);
                if (DEBUG) debugPrintf("[LED GPIO] %-20s (GPIO %2d, Digital) -> %s (%s)\n",
                    led.label, pin, level == HIGH ? "HIGH" : "LOW", state ? "ON" : "OFF");
           }
        }
        // If you want debug for skipped gauges, add:
        else if (led.deviceType == DEVICE_GAUGE && DEBUG) {
            debugPrintf("[LED GAUGE] %-20s (GPIO %2d) -> SKIPPED (servo)\n",
                        led.label, led.info.gaugeInfo.gpio);
        }
    }
}

void GPIO_setDigital(uint8_t pin, bool activeHigh, bool state) {
    uint8_t pinLevel = (state ^ !activeHigh) ? HIGH : LOW;
    digitalWrite(pin, pinLevel);
}

void GPIO_setAnalog(uint8_t pin, bool activeLow, uint8_t intensity) {
    uint8_t pwmValue = map(intensity, 0, 100, 0, 255);
    if (activeLow) pwmValue = 255 - pwmValue;
    analogWrite(pin, pwmValue);
}

void GPIO_offAnalog(uint8_t pin, bool activeLow) {
    analogWrite(pin, activeLow ? 255 : 0);
}