// ************************************************************
// GPIO - Helper functions for all GPIO related Outputs
// ************************************************************

// Init all gauges
void resetAllGauges() {
    for (uint16_t i = 0; i < panelLEDsCount; ++i) {
        const auto& led = panelLEDs[i];
        if (led.deviceType == DEVICE_GAUGE) {
	    PanelRegistry_setActive(PanelKind::AnalogGauge, true);
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
    if (PanelRegistry_has(PanelKind::AnalogGauge)) debugPrintln("[GAUGE] Analog gauges will update automatically.");
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
        else if (led.deviceType == DEVICE_MAGNETIC) {
            const auto& m = led.info.magneticInfo;
            uint8_t offLevel = led.activeLow ? HIGH : LOW;

            // Solenoid A — always present
            pinMode(m.gpioA, OUTPUT);
            digitalWrite(m.gpioA, offLevel);

            // Solenoid B — only for 3-pos (255 = unused)
            if (m.gpioB != 255) {
                pinMode(m.gpioB, OUTPUT);
                digitalWrite(m.gpioB, offLevel);
            }

            if (DEBUG) {
                if (m.gpioB == 255) {
                    debugPrintf("[INIT] MAGNETIC   %-20s (GPIO %2d)         -> OUTPUT, OFF (2-pos)\n",
                        led.label, m.gpioA);
                } else {
                    debugPrintf("[INIT] MAGNETIC   %-20s (GPIO %2d, %2d)     -> OUTPUT, OFF (3-pos)\n",
                        led.label, m.gpioA, m.gpioB);
                }
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
        else if (led.deviceType == DEVICE_MAGNETIC) {
            const auto& m = led.info.magneticInfo;
            uint8_t level = (state ^ led.activeLow) ? HIGH : LOW;
            digitalWrite(m.gpioA, level);
            if (m.gpioB != 255) digitalWrite(m.gpioB, level);
            if (DEBUG) {
                if (m.gpioB == 255) {
                    debugPrintf("[MAGNETIC]  %-20s (GPIO %2d)          -> %s\n",
                        led.label, m.gpioA, state ? "ON" : "OFF");
                } else {
                    debugPrintf("[MAGNETIC]  %-20s (GPIO %2d, %2d)      -> %s\n",
                        led.label, m.gpioA, m.gpioB, state ? "ON" : "OFF");
                }
            }
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