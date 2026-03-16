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

// ============================================================
// STEPPER MOTOR — supports two 4-wire stepper types:
//
//   28BYJ-48 + ULN2003  (geared, continuous 360, unlimited turns)
//     4096 steps/rev, 800us/step, needs ULN2003 driver board
//     Use for: altimeters, heading indicators, clocks, throttles
//
//   X27.168 / VID29     (direct drive, fast, ~315 deg max)
//     720 steps/rev, 100us/step, wires directly to ESP32 GPIO
//     Use for: oil/hyd pressure, fuel, RPM (limited-sweep gauges)
//
// Non-blocking: Stepper_tick() rate-limits each stepper to its
// configured usPerStep timing. No delays, no loops.
// ============================================================

struct StepperState {
    uint8_t  pins[4];           // IN1, IN2, IN3, IN4 (GPIO)
    uint16_t totalSteps;        // total steps for full range (4096 = 1 rev for 28BYJ-48)
    uint16_t usPerStep;         // microseconds per half-step (speed limit for this motor)
    volatile int32_t targetStep;// desired position
    int32_t  currentStep;       // actual position
    uint32_t lastStepUs;        // micros() timestamp of last step (for rate-limiting in tick)
    uint8_t  phaseIndex;        // 0-7 half-step sequence index
    bool     enabled;
    bool     continuous;        // true = 360 unlimited rotation (wraparound), false = limited sweep (no wraparound)
};

static StepperState stepperArray[MAX_STEPPERS];
static uint8_t      stepperCount = 0;

// 8-phase half-step sequence for 28BYJ-48 through ULN2003.
// Each entry is a 4-bit pattern: bit0=IN1, bit1=IN2, bit2=IN3, bit3=IN4
static const uint8_t HALF_STEP_SEQ[8] = {
    0b0001,  // phase 0: IN1
    0b0011,  // phase 1: IN1 + IN2
    0b0010,  // phase 2: IN2
    0b0110,  // phase 3: IN2 + IN3
    0b0100,  // phase 4: IN3
    0b1100,  // phase 5: IN3 + IN4
    0b1000,  // phase 6: IN4
    0b1001,  // phase 7: IN4 + IN1
};

// Apply the current phase pattern to the 4 GPIO pins
static void stepperApplyPhase(const StepperState& s) {
    uint8_t pattern = HALF_STEP_SEQ[s.phaseIndex];
    digitalWrite(s.pins[0], (pattern & 0x01) ? HIGH : LOW);
    digitalWrite(s.pins[1], (pattern & 0x02) ? HIGH : LOW);
    digitalWrite(s.pins[2], (pattern & 0x04) ? HIGH : LOW);
    digitalWrite(s.pins[3], (pattern & 0x08) ? HIGH : LOW);
}

// De-energize all coils for a single stepper (saves power)
static void stepperAllOff(const StepperState& s) {
    for (uint8_t i = 0; i < 4; ++i)
        digitalWrite(s.pins[i], LOW);
}

void Stepper_register(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4,
                      uint16_t totalSteps, uint16_t usPerStep, bool continuous) {
    if (stepperCount >= MAX_STEPPERS) return;

    StepperState& st = stepperArray[stepperCount];
    st.pins[0]     = pin1;
    st.pins[1]     = pin2;
    st.pins[2]     = pin3;
    st.pins[3]     = pin4;
    st.totalSteps  = totalSteps;
    st.usPerStep   = usPerStep ? usPerStep : 1000;  // default to safe 28BYJ-48 speed
    st.targetStep  = 0;
    st.currentStep = 0;
    st.lastStepUs  = 0;
    st.phaseIndex  = 0;
    st.enabled     = true;
    st.continuous  = continuous;

    // Set all 4 pins as OUTPUT
    for (uint8_t i = 0; i < 4; ++i)
        pinMode(st.pins[i], OUTPUT);

    // Energize phase 0 — motor snaps to its zero detent and holds.
    // This is the repeatable reference position for needle attachment:
    // attach the needle while the motor is holding here, pointing at "0".
    stepperApplyPhase(st);

    stepperCount++;
    debugPrintf("[STEPPER] Registered id=%u pins=%u,%u,%u,%u totalSteps=%u usPerStep=%u %s (holding phase 0)\n",
                stepperCount - 1, pin1, pin2, pin3, pin4, totalSteps, st.usPerStep,
                st.continuous ? "continuous" : "limited-sweep");
}

void Stepper_initSweep(uint8_t pin1) {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        if (stepperArray[i].pins[0] != pin1) continue;
        StepperState& st = stepperArray[i];

        // Cap sweep to one revolution (4096 half-steps) for visual self-test.
        // Even multi-rev instruments only need one turn to confirm needle zero.
        const int32_t sweepSteps = (st.totalSteps < 4096) ? st.totalSteps : 4096;

        // Use this stepper's configured speed — fast motors sweep fast, slow motors sweep slow.
        const uint32_t stepDelay = st.usPerStep;

        debugPrintf("[STEPPER] Init sweep: %ld steps forward (%luus/step)...\n",
                    (long)sweepSteps, (unsigned long)stepDelay);

        // Full revolution forward
        for (int32_t s = 0; s < sweepSteps; ++s) {
            st.phaseIndex = (st.phaseIndex + 1) % 8;
            stepperApplyPhase(st);
            delayMicroseconds(stepDelay);
            if ((s & 0xFF) == 0) delay(0);  // feed watchdog every 256 steps
        }

        delay(0);  // feed watchdog between directions
        debugPrintf("[STEPPER] Init sweep: returning to zero...\n");

        // Full revolution backward to zero
        for (int32_t s = 0; s < sweepSteps; ++s) {
            st.phaseIndex = (st.phaseIndex - 1 + 8) % 8;
            stepperApplyPhase(st);
            delayMicroseconds(stepDelay);
            if ((s & 0xFF) == 0) delay(0);  // feed watchdog every 256 steps
        }

        // Confirm we're back at phase 0, position 0
        st.currentStep = 0;
        st.targetStep  = 0;
        debugPrintln("[STEPPER] Init sweep complete — holding at zero.");
        return;
    }
}

void Stepper_set(uint8_t pin1, int32_t targetStep) {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        if (stepperArray[i].pins[0] == pin1) {
            int32_t ts = (int32_t)stepperArray[i].totalSteps;
            if (stepperArray[i].continuous) {
                // Wrap target into [0, totalSteps) for continuous-rotation
                targetStep = targetStep % ts;
                if (targetStep < 0) targetStep += ts;
            } else {
                // Clamp target for limited-sweep motors (never exceed end stops)
                if (targetStep < 0)   targetStep = 0;
                if (targetStep >= ts) targetStep = ts - 1;
            }
            stepperArray[i].targetStep = targetStep;
            return;
        }
    }
}

void Stepper_tick() {
    uint32_t now = micros();

    for (uint8_t i = 0; i < stepperCount; ++i) {
        StepperState& st = stepperArray[i];
        if (!st.enabled) continue;
        if (st.currentStep == st.targetStep) {
            // De-energize coils after 2 seconds idle to prevent overheating.
            // The gearbox holds position through friction with coils off.
            if (st.lastStepUs && (now - st.lastStepUs) > 2000000UL) {
                stepperAllOff(st);
                st.lastStepUs = 0;  // only de-energize once
            }
            continue;
        }

        // Rate-limit: respect this motor's usPerStep timing
        if ((now - st.lastStepUs) < st.usPerStep) continue;
        st.lastStepUs = now;

        int32_t delta = st.targetStep - st.currentStep;

        if (st.continuous) {
            // Shortest-path wraparound for continuous-rotation instruments
            // (e.g., altimeter needle crossing the 12-o'clock position)
            int32_t half = (int32_t)st.totalSteps / 2;
            if (delta > half)        delta -= st.totalSteps;
            else if (delta < -half)  delta += st.totalSteps;
        }
        // Limited-sweep motors: delta is used as-is (no wraparound),
        // so the motor always traverses the valid sweep range.

        int8_t dir = (delta > 0) ? 1 : -1;
        st.currentStep += dir;

        // Wrap currentStep to [0, totalSteps) — only relevant for continuous motors
        // but safe for limited-sweep (they never cross the boundary)
        if (st.currentStep < 0)                       st.currentStep += st.totalSteps;
        else if (st.currentStep >= st.totalSteps)     st.currentStep -= st.totalSteps;

        st.phaseIndex = (uint8_t)((int8_t)st.phaseIndex + dir + 8) % 8;
        stepperApplyPhase(st);
    }
}

void Stepper_setAllOff() {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        stepperAllOff(stepperArray[i]);
    }
}
