GaugeState gaugeArray[MAX_GAUGES];
uint8_t gaugeCount = 0;

void AnalogG_registerGauge(uint8_t pin, int minPulseUs, int maxPulseUs) {
    if (gaugeCount < MAX_GAUGES) {
        gaugeArray[gaugeCount].pin = pin;
        gaugeArray[gaugeCount].minPulseUs = minPulseUs;
        gaugeArray[gaugeCount].maxPulseUs = maxPulseUs;
        gaugeArray[gaugeCount].value = 0;
        pinMode(pin, OUTPUT);
        gaugeCount++;
    }
}

void AnalogG_set(uint8_t pin, uint16_t value) {
    for (uint8_t i = 0; i < gaugeCount; ++i) {
        if (gaugeArray[i].pin == pin) {
            gaugeArray[i].value = value;
            return;
        }
    }
}

void AnalogG_tick() {
    static uint32_t lastTickMs = 0;
    uint32_t now = millis();
    if ((now - lastTickMs) < SERVO_UPDATE_FREQ_MS) return;  // Only run every SERVO_UPDATE_FREQ_MS

    lastTickMs = now;

    for (uint8_t i = 0; i < gaugeCount; ++i) {
        int pulseUs = gaugeArray[i].minPulseUs +
            (int)(((long)(gaugeArray[i].maxPulseUs - gaugeArray[i].minPulseUs) * gaugeArray[i].value) / 65535L);
        digitalWrite(gaugeArray[i].pin, HIGH);
        delayMicroseconds(pulseUs);
        digitalWrite(gaugeArray[i].pin, LOW);
    }
}

// Generate one servo pulse at the given pin, min/max calibration, and 0-65535 value.
// Only call this once per DCS-BIOS update (30Hz) or at your desired rate.
void AnalogG_pulseUs(uint8_t pin, int minPulseUs, int maxPulseUs, uint16_t value) {
    // value is uint16_t — max is already 65535, no clamp needed
    int pulseUs = minPulseUs + (int)(((long)(maxPulseUs - minPulseUs) * value) / 65535L);
    digitalWrite(pin, HIGH);
    delayMicroseconds(pulseUs);
    digitalWrite(pin, LOW);
}

// (Optional) Basic setup utility if you want to use it
void AnalogG_initPin(uint8_t pin) {
    pinMode(pin, OUTPUT);
}