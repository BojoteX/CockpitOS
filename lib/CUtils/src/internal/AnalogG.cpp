// ************************************************************
// AnalogG - Servo/Gauge output via ESP32 LEDC hardware PWM
//
// Replaces the original bit-bang implementation with zero-CPU-cost
// hardware PWM. All existing function signatures are preserved.
// See AnalogG.cpp.BITBANG_BACKUP for the original implementation.
// ************************************************************

// ---- Internal servo state (superset of legacy GaugeState) ----
struct ServoState {
    uint8_t  pin;
    uint16_t minPulseUs;
    uint16_t maxPulseUs;
    uint16_t freqHz;         // default 50 (standard servo)
    uint8_t  bits;           // default 16 (65536 ticks per period)
    volatile uint16_t value; // 0-65535 DCS-BIOS range
    bool     enabled;        // true = LEDC should be active
    bool     attached;       // true = LEDC channel currently assigned
};

static ServoState servoArray_internal[MAX_GAUGES];
static uint8_t    servoCount = 0;
static bool       ledcInitialized = false;  // deferred init flag

// Legacy extern globals (mirrored for backward compatibility)
GaugeState gaugeArray[MAX_GAUGES];
uint8_t    gaugeCount = 0;

// ---- Shared helper: calculate duty and write to LEDC ----
static void servoApplyDuty(const ServoState& s) {
    if (!s.attached || !s.enabled) return;
    uint32_t periodUs = 1000000UL / s.freqHz;
    uint32_t maxDuty  = (1UL << s.bits);
    int pulseUs = s.minPulseUs +
        (int)(((long)(s.maxPulseUs - s.minPulseUs) * s.value) / 65535L);
    uint32_t duty = ((uint32_t)pulseUs * maxDuty) / periodUs;
    ledcWrite(s.pin, duty);
}

// Mirror internal state to legacy gaugeArray
static void mirrorToLegacy(uint8_t idx) {
    gaugeArray[idx].pin        = servoArray_internal[idx].pin;
    gaugeArray[idx].minPulseUs = servoArray_internal[idx].minPulseUs;
    gaugeArray[idx].maxPulseUs = servoArray_internal[idx].maxPulseUs;
    gaugeArray[idx].value      = servoArray_internal[idx].value;
}

// ============================================================
// LEGACY API (signatures unchanged -- called by GPIO.cpp and LEDControl.cpp)
// ============================================================

void AnalogG_registerGauge(uint8_t pin, int minPulseUs, int maxPulseUs) {
    if (servoCount >= MAX_GAUGES) return;

    ServoState& s = servoArray_internal[servoCount];
    s.pin        = pin;
    s.minPulseUs = (uint16_t)minPulseUs;
    s.maxPulseUs = (uint16_t)maxPulseUs;
    s.freqHz     = 50;
    s.bits       = 16;
    s.value      = 0;
    s.enabled    = true;
    s.attached   = false;  // deferred -- LEDC attach happens in first tick()

    // Keep pin as plain OUTPUT for the init sweep (bit-banged AnalogG_pulseUs)
    pinMode(pin, OUTPUT);

    mirrorToLegacy(servoCount);
    servoCount++;
    gaugeCount = servoCount;
}

void AnalogG_set(uint8_t pin, uint16_t value) {
    for (uint8_t i = 0; i < servoCount; ++i) {
        if (servoArray_internal[i].pin == pin) {
            servoArray_internal[i].value = value;
            gaugeArray[i].value = value;
            servoApplyDuty(servoArray_internal[i]);
            return;
        }
    }
}

void AnalogG_tick() {
    // Deferred LEDC initialization: attach all registered gauges on first call.
    // This runs AFTER preconfigureGPIO() has finished calling pinMode() on gauge
    // pins, so the LEDC channels won't be clobbered.
    if (!ledcInitialized && servoCount > 0) {
        ledcInitialized = true;
        for (uint8_t i = 0; i < servoCount; ++i) {
            ServoState& s = servoArray_internal[i];
            if (s.attached) {
                // Already attached (e.g. by Servo_attach in a custom panel) — skip
                debugPrintf("[SERVO] LEDC already attached pin %u — skipping\n", s.pin);
                continue;
            }
            if (s.enabled) {
                s.attached = ledcAttach(s.pin, s.freqHz, s.bits);
                if (s.attached) {
                    servoApplyDuty(s);
                    debugPrintf("[SERVO] LEDC attached pin %u (%uHz %u-bit)\n",
                                s.pin, s.freqHz, s.bits);
                } else {
                    debugPrintf("[SERVO] LEDC attach FAILED pin %u\n", s.pin);
                }
            }
        }
    }
    // After initialization, this function is a no-op.
    // Hardware PWM maintains servo signals with zero CPU cost.
}

// Bit-bang pulse for init sweep (called by GPIO.cpp before LEDC is attached)
void AnalogG_pulseUs(uint8_t pin, int minPulseUs, int maxPulseUs, uint16_t value) {
    int pulseUs = minPulseUs + (int)(((long)(maxPulseUs - minPulseUs) * value) / 65535L);
    digitalWrite(pin, HIGH);
    delayMicroseconds(pulseUs);
    digitalWrite(pin, LOW);
}

void AnalogG_initPin(uint8_t pin) {
    // No-op: pin configuration is handled by AnalogG_registerGauge() or Servo_attach().
    (void)pin;
}

// ============================================================
// NEW SERVO PUBLIC API (for custom panels)
// ============================================================

uint8_t Servo_attachEx(uint8_t pin, uint16_t minPulseUs, uint16_t maxPulseUs,
                       uint16_t freqHz, uint8_t bits) {
    if (servoCount >= MAX_GAUGES) return 0xFF;

    uint8_t id = servoCount;
    ServoState& s = servoArray_internal[id];
    s.pin        = pin;
    s.minPulseUs = minPulseUs;
    s.maxPulseUs = maxPulseUs;
    s.freqHz     = freqHz;
    s.bits       = bits;
    s.value      = 0;
    s.enabled    = true;

    // Custom panels call this from init(), which runs after preconfigureGPIO(),
    // so ledcAttach() is safe here -- no subsequent pinMode() will clobber it.
    s.attached = ledcAttach(pin, freqHz, bits);
    if (s.attached) {
        servoApplyDuty(s);  // set to min position
        debugPrintf("[SERVO] Attached pin %u (%uHz %u-bit) id=%u\n",
                    pin, freqHz, bits, id);
    } else {
        debugPrintf("[SERVO] Attach FAILED pin %u\n", pin);
    }

    mirrorToLegacy(id);
    servoCount++;
    gaugeCount = servoCount;
    return id;
}

uint8_t Servo_attach(uint8_t pin, uint16_t minPulseUs, uint16_t maxPulseUs) {
    return Servo_attachEx(pin, minPulseUs, maxPulseUs, 50, 16);
}

void Servo_write(uint8_t id, uint16_t value) {
    if (id >= servoCount) return;
    ServoState& s = servoArray_internal[id];
    s.value = value;
    gaugeArray[id].value = value;
    if (!s.attached || !s.enabled) {
        debugPrintf("[SERVO] write id=%u BLOCKED (attached=%u enabled=%u)\n",
                    id, s.attached, s.enabled);
    }
    servoApplyDuty(s);
}

void Servo_writeMicroseconds(uint8_t id, uint16_t pulseUs) {
    if (id >= servoCount) return;
    ServoState& s = servoArray_internal[id];

    if (s.attached && s.enabled) {
        uint32_t periodUs = 1000000UL / s.freqHz;
        uint32_t maxDuty  = (1UL << s.bits);
        uint32_t duty = ((uint32_t)pulseUs * maxDuty) / periodUs;
        ledcWrite(s.pin, duty);
    }

    // Back-calculate the 0-65535 value for state consistency
    if (s.maxPulseUs > s.minPulseUs) {
        uint16_t clamped = constrain(pulseUs, s.minPulseUs, s.maxPulseUs);
        s.value = (uint16_t)(((long)(clamped - s.minPulseUs) * 65535L) /
                             (s.maxPulseUs - s.minPulseUs));
        gaugeArray[id].value = s.value;
    }
}

void Servo_enable(uint8_t id) {
    if (id >= servoCount) return;
    ServoState& s = servoArray_internal[id];
    if (!s.enabled) {
        s.enabled = true;
        s.attached = ledcAttach(s.pin, s.freqHz, s.bits);
        if (s.attached) {
            servoApplyDuty(s);  // restore last position
        }
    }
}

void Servo_disable(uint8_t id) {
    if (id >= servoCount) return;
    ServoState& s = servoArray_internal[id];
    if (s.enabled) {
        s.enabled = false;
        if (s.attached) {
            ledcDetach(s.pin);
            s.attached = false;
        }
    }
}

void Servo_detach(uint8_t id) {
    if (id >= servoCount) return;
    ServoState& s = servoArray_internal[id];
    if (s.attached) {
        ledcDetach(s.pin);
        s.attached = false;
    }
    s.enabled = false;
}
