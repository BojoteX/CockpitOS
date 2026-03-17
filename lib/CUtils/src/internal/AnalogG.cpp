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

// Init sweep state machine — non-blocking self-test at boot
enum SweepPhase : uint8_t { SWEEP_NONE = 0, SWEEP_FWD, SWEEP_BWD };

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
    // Init sweep state (non-blocking)
    SweepPhase sweepPhase;      // current sweep state
    int32_t    sweepSteps;      // total steps for sweep (capped at 4096)
    int32_t    sweepProgress;   // steps completed in current direction
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

// Apply the current phase pattern to the 4 GPIO pins.
// Uses ESP-IDF gpio_set_level() for lower overhead than Arduino's
// digitalWrite() — same API used by HT1622 and PCA9555 drivers.
static void stepperApplyPhase(const StepperState& s) {
    uint8_t pattern = HALF_STEP_SEQ[s.phaseIndex];
    gpio_set_level((gpio_num_t)s.pins[0], (pattern & 0x01) ? 1 : 0);
    gpio_set_level((gpio_num_t)s.pins[1], (pattern & 0x02) ? 1 : 0);
    gpio_set_level((gpio_num_t)s.pins[2], (pattern & 0x04) ? 1 : 0);
    gpio_set_level((gpio_num_t)s.pins[3], (pattern & 0x08) ? 1 : 0);
}

// De-energize all coils for a single stepper (saves power)
static void stepperAllOff(const StepperState& s) {
    for (uint8_t i = 0; i < 4; ++i)
        gpio_set_level((gpio_num_t)s.pins[i], 0);
}

uint8_t Stepper_register(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4,
                         uint16_t totalSteps, uint16_t usPerStep, bool continuous) {
    if (stepperCount >= MAX_STEPPERS) return 0xFF;

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
    st.enabled       = true;
    st.continuous    = continuous;
    st.sweepPhase    = SWEEP_NONE;
    st.sweepSteps    = 0;
    st.sweepProgress = 0;

    // Set all 4 pins as OUTPUT
    for (uint8_t i = 0; i < 4; ++i)
        pinMode(st.pins[i], OUTPUT);

    // Energize phase 0 — motor snaps to its zero detent and holds.
    // This is the repeatable reference position for needle attachment:
    // attach the needle while the motor is holding here, pointing at "0".
    stepperApplyPhase(st);

    uint8_t id = stepperCount++;
    debugPrintf("[STEPPER] Registered id=%u pins=%u,%u,%u,%u totalSteps=%u usPerStep=%u %s (holding phase 0)\n",
                id, pin1, pin2, pin3, pin4, totalSteps, st.usPerStep,
                st.continuous ? "continuous" : "limited-sweep");
    return id;
}

// Non-blocking init sweep trigger — sets up the sweep state machine.
// The actual sweep runs inside Stepper_tick() at the motor's physical speed.
// All steppers sweep concurrently; boot time = max(single) not sum(all).
void Stepper_startSweep(uint8_t pin1) {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        if (stepperArray[i].pins[0] != pin1) continue;
        StepperState& st = stepperArray[i];

        // Cap sweep to one revolution (4096 half-steps) for visual self-test.
        st.sweepSteps    = (st.totalSteps < 4096) ? st.totalSteps : 4096;
        st.sweepProgress = 0;
        st.sweepPhase    = SWEEP_FWD;
        st.lastStepUs    = micros();  // start timing from now

        debugPrintf("[STEPPER] Init sweep started: %ld steps (%uus/step, non-blocking)\n",
                    (long)st.sweepSteps, st.usPerStep);
        return;
    }
}

// Legacy blocking init sweep — kept for backward compatibility but
// the non-blocking Stepper_startSweep() is preferred for boot.
void Stepper_initSweep(uint8_t pin1) {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        if (stepperArray[i].pins[0] != pin1) continue;
        StepperState& st = stepperArray[i];

        const int32_t sweepSteps = (st.totalSteps < 4096) ? st.totalSteps : 4096;
        const uint32_t stepDelay = st.usPerStep;

        debugPrintf("[STEPPER] Init sweep: %ld steps forward (%luus/step)...\n",
                    (long)sweepSteps, (unsigned long)stepDelay);

        for (int32_t s = 0; s < sweepSteps; ++s) {
            st.phaseIndex = (st.phaseIndex + 1) % 8;
            stepperApplyPhase(st);
            delayMicroseconds(stepDelay);
            if ((s & 0xFF) == 0) delay(0);
        }

        delay(0);
        debugPrintf("[STEPPER] Init sweep: returning to zero...\n");

        for (int32_t s = 0; s < sweepSteps; ++s) {
            st.phaseIndex = (st.phaseIndex - 1 + 8) % 8;
            stepperApplyPhase(st);
            delayMicroseconds(stepDelay);
            if ((s & 0xFF) == 0) delay(0);
        }

        st.currentStep = 0;
        st.targetStep  = 0;
        debugPrintln("[STEPPER] Init sweep complete — holding at zero.");
        return;
    }
}

// Clamp/wrap target and store — shared by both set functions
static void stepperApplyTarget(StepperState& st, int32_t targetStep) {
    int32_t ts = (int32_t)st.totalSteps;
    if (st.continuous) {
        targetStep = targetStep % ts;
        if (targetStep < 0) targetStep += ts;
    } else {
        if (targetStep < 0)   targetStep = 0;
        if (targetStep >= ts) targetStep = ts - 1;
    }
    st.targetStep = targetStep;
}

void Stepper_set(uint8_t pin1, int32_t targetStep) {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        if (stepperArray[i].pins[0] == pin1) {
            stepperApplyTarget(stepperArray[i], targetStep);
            return;
        }
    }
}

// O(1) direct access by index — use when you have the id from Stepper_register()
void Stepper_setById(uint8_t id, int32_t targetStep) {
    if (id >= stepperCount) return;
    stepperApplyTarget(stepperArray[id], targetStep);
}

void Stepper_tick() {
    uint32_t now = micros();

    for (uint8_t i = 0; i < stepperCount; ++i) {
        StepperState& st = stepperArray[i];
        if (!st.enabled) continue;

        // ── Non-blocking init sweep state machine ──
        // Runs before normal target-tracking. During sweep, DCS-BIOS
        // can still set targetStep — it will be picked up when sweep ends.
        if (st.sweepPhase != SWEEP_NONE) {
            uint32_t elapsed = now - st.lastStepUs;
            if (elapsed < st.usPerStep) continue;

            uint32_t stepsAvailable = elapsed / st.usPerStep;
            uint32_t remaining = (uint32_t)(st.sweepSteps - st.sweepProgress);
            uint32_t steps = (stepsAvailable < remaining) ? stepsAvailable : remaining;

            if (st.sweepPhase == SWEEP_FWD) {
                // Advance forward
                int32_t phaseAdv = (int32_t)(steps % 8);
                st.phaseIndex = (uint8_t)((st.phaseIndex + phaseAdv) % 8);
                st.sweepProgress += (int32_t)steps;
                stepperApplyPhase(st);
                st.lastStepUs = now - (elapsed % st.usPerStep);

                if (st.sweepProgress >= st.sweepSteps) {
                    st.sweepPhase    = SWEEP_BWD;
                    st.sweepProgress = 0;
                    debugPrintln("[STEPPER] Init sweep: returning to zero...");
                }
            } else {  // SWEEP_BWD
                // Reverse back to zero
                int32_t phaseAdv = -((int32_t)(steps % 8));
                st.phaseIndex = (uint8_t)((st.phaseIndex + phaseAdv + 8) % 8);
                st.sweepProgress += (int32_t)steps;
                stepperApplyPhase(st);
                st.lastStepUs = now - (elapsed % st.usPerStep);

                if (st.sweepProgress >= st.sweepSteps) {
                    st.sweepPhase  = SWEEP_NONE;
                    st.currentStep = 0;
                    st.targetStep  = 0;
                    st.phaseIndex  = 0;
                    stepperApplyPhase(st);  // ensure phase 0 is applied
                    debugPrintln("[STEPPER] Init sweep complete — holding at zero.");
                }
            }
            continue;  // skip normal target tracking while sweeping
        }

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
        uint32_t elapsed = now - st.lastStepUs;
        if (elapsed < st.usPerStep) continue;

        // Multi-step arithmetic: advance as many steps as the elapsed
        // time allows, up to the remaining delta. At 250Hz main loop
        // with a 100us/step X27, this jumps up to 40 steps per tick
        // instead of the old single-step-per-tick bottleneck.
        uint32_t stepsAvailable = elapsed / st.usPerStep;

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
        uint32_t absDelta = (uint32_t)((delta > 0) ? delta : -delta);
        uint32_t stepsToTake = (stepsAvailable < absDelta) ? stepsAvailable : absDelta;

        // Advance position by the full step count
        st.currentStep += dir * (int32_t)stepsToTake;

        // Wrap currentStep to [0, totalSteps) — only relevant for continuous motors
        // but safe for limited-sweep (they never cross the boundary)
        if (st.currentStep < 0)                       st.currentStep += st.totalSteps;
        else if (st.currentStep >= st.totalSteps)     st.currentStep -= st.totalSteps;

        // Compute final phase index arithmetically — no need to step through
        // each intermediate phase. The motor coils only see the final pattern.
        // Phase advances by +1 (forward) or -1 (backward) per step, modulo 8.
        int32_t phaseAdvance = dir * (int32_t)(stepsToTake % 8);
        st.phaseIndex = (uint8_t)((st.phaseIndex + phaseAdvance + 8) % 8);
        stepperApplyPhase(st);

        // Preserve fractional remainder for smooth timing across ticks
        st.lastStepUs = now - (elapsed % st.usPerStep);
    }
}

void Stepper_setAllOff() {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        stepperAllOff(stepperArray[i]);
    }
}
