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
// STEPPER MOTOR — supports two motor types with different drive
// sequences, step counts, and speed control:
//
//   28BYJ-48 + ULN2003  (stateCount=8, geared, 8-phase half-step)
//     4096 steps/rev, fixed speed (usPerStep), no acceleration needed
//     Use for: altimeters, heading indicators, clocks, throttles
//
//   X27.168 / VID29 / X25.168  (stateCount=6, direct drive, 6-state)
//     945 steps for 315 deg, acceleration required (3000-600us ramp)
//     Drive sequence and accel table from SwitecX25 library
//     (Guy Carpenter, Clearwater Software, BSD2 license)
//     Use for: oil/hyd pressure, fuel, RPM (limited-sweep gauges)
//
// Non-blocking: Stepper_tick() is called every frame, advances at
// most one step per stepper, returns immediately. No delays.
// ============================================================

// Init sweep state machine — non-blocking self-test at boot
enum SweepPhase : uint8_t { SWEEP_NONE = 0, SWEEP_FWD, SWEEP_BWD };

// ── State sequences ──

// 8-phase half-step for 28BYJ-48 through ULN2003.
// bit0=IN1, bit1=IN2, bit2=IN3, bit3=IN4
static const uint8_t HALF_STEP_SEQ[8] = {
    0b0001, 0b0011, 0b0010, 0b0110,
    0b0100, 0b1100, 0b1000, 0b1001,
};

// 6-state sequence for X27.168 / VID29 (from SwitecX25 library).
// Completely different motor type — NOT a standard unipolar stepper.
static const uint8_t X27_STATE_SEQ[6] = {
    0x9, 0x1, 0x7, 0x6, 0xE, 0x8,
};

// ── X27 acceleration table (from SwitecX25 library) ──
// {cumulative_steps_from_rest, delay_microseconds}
// Motor starts at 3000us/step (slow), ramps to 600us/step (fast).
// Cannot start at full speed — will stall without this ramp.
static const uint16_t X27_ACCEL_TABLE[][2] = {
    {   20, 3000 },
    {   50, 1500 },
    {  100, 1000 },
    {  150,  800 },
    {  300,  600 },
};
#define X27_ACCEL_TABLE_SIZE (sizeof(X27_ACCEL_TABLE) / sizeof(X27_ACCEL_TABLE[0]))
#define X27_MAX_VEL          300   // last entry's step count

struct StepperState {
    uint8_t  pins[4];           // IN1, IN2, IN3, IN4 (GPIO)
    uint16_t totalSteps;        // full range (4096 for 28BYJ, 945 for X27)
    uint16_t usPerStep;         // fixed speed for 28BYJ; sweep speed for X27
    volatile int32_t targetStep;// desired position (set by DCS-BIOS)
    int32_t  currentStep;       // actual position
    uint32_t time0;             // micros() timestamp of last step
    uint16_t microDelay;        // delay until next step (varies for X27 accel, fixed for 28BYJ)
    uint16_t vel;               // acceleration velocity — cumulative steps from rest (X27 only)
    int8_t   dir;               // movement direction: -1, 0, 1
    const uint8_t* stateSeq;    // pointer to state sequence array
    uint8_t  stateCount;        // 6 for X27, 8 for 28BYJ
    uint8_t  stateIndex;        // current position in state sequence
    bool     enabled;
    bool     continuous;        // true = wraparound (28BYJ), false = limited sweep
    bool     stopped;           // true when at target with vel=0 (X27 accel state)
    // Init sweep state (non-blocking)
    SweepPhase sweepPhase;
    int32_t    sweepSteps;
    int32_t    sweepProgress;
};

static StepperState stepperArray[MAX_STEPPERS];
static uint8_t      stepperCount = 0;

// Apply current state pattern to GPIO pins.
// Uses ESP-IDF gpio_set_level() for lower overhead — same API as HT1622/PCA9555.
static void stepperApplyState(const StepperState& s) {
    uint8_t mask = s.stateSeq[s.stateIndex];
    gpio_set_level((gpio_num_t)s.pins[0], (mask & 0x01) ? 1 : 0);
    gpio_set_level((gpio_num_t)s.pins[1], (mask & 0x02) ? 1 : 0);
    gpio_set_level((gpio_num_t)s.pins[2], (mask & 0x04) ? 1 : 0);
    gpio_set_level((gpio_num_t)s.pins[3], (mask & 0x08) ? 1 : 0);
}

// De-energize all coils (saves power, gearbox/friction holds position)
static void stepperAllOff(const StepperState& s) {
    for (uint8_t i = 0; i < 4; ++i)
        gpio_set_level((gpio_num_t)s.pins[i], 0);
}

// Advance one step in the given direction, update state index
static void stepperStep(StepperState& st, int8_t dir) {
    st.currentStep += dir;
    if (dir > 0)
        st.stateIndex = (st.stateIndex + 1) % st.stateCount;
    else
        st.stateIndex = (st.stateIndex + st.stateCount - 1) % st.stateCount;

    // Wrap for continuous motors (28BYJ)
    if (st.continuous) {
        if (st.currentStep < 0)                        st.currentStep += st.totalSteps;
        else if (st.currentStep >= (int32_t)st.totalSteps) st.currentStep -= st.totalSteps;
    }
    stepperApplyState(st);
}

// Look up delay from X27 acceleration table
static uint16_t x27LookupDelay(uint16_t vel) {
    for (uint8_t i = 0; i < X27_ACCEL_TABLE_SIZE; i++) {
        if (X27_ACCEL_TABLE[i][0] >= vel) return X27_ACCEL_TABLE[i][1];
    }
    return X27_ACCEL_TABLE[X27_ACCEL_TABLE_SIZE - 1][1];
}

uint8_t Stepper_register(uint8_t pin1, uint8_t pin2, uint8_t pin3, uint8_t pin4,
                         uint16_t totalSteps, uint16_t usPerStep,
                         uint8_t stateCount, bool continuous) {
    if (stepperCount >= MAX_STEPPERS) return 0xFF;

    StepperState& st = stepperArray[stepperCount];
    st.pins[0]       = pin1;
    st.pins[1]       = pin2;
    st.pins[2]       = pin3;
    st.pins[3]       = pin4;
    st.totalSteps    = totalSteps;
    st.usPerStep     = usPerStep ? usPerStep : 1000;
    st.targetStep    = 0;
    st.currentStep   = 0;
    st.time0         = 0;
    st.microDelay    = 0;
    st.vel           = 0;
    st.dir           = 0;
    st.stateCount    = (stateCount == 6) ? 6 : 8;  // only 6 or 8 supported
    st.stateSeq      = (st.stateCount == 6) ? X27_STATE_SEQ : HALF_STEP_SEQ;
    st.stateIndex    = 0;
    st.enabled       = true;
    st.continuous    = continuous;
    st.stopped       = true;
    st.sweepPhase    = SWEEP_NONE;
    st.sweepSteps    = 0;
    st.sweepProgress = 0;

    for (uint8_t i = 0; i < 4; ++i)
        pinMode(st.pins[i], OUTPUT);

    // Energize state 0 — motor snaps to zero detent for needle attachment
    stepperApplyState(st);

    uint8_t id = stepperCount++;
    debugPrintf("[STEPPER] Registered id=%u pins=%u,%u,%u,%u totalSteps=%u %s %s (holding state 0)\n",
                id, pin1, pin2, pin3, pin4, totalSteps,
                (st.stateCount == 6) ? "X27/6-state" : "28BYJ/8-phase",
                st.continuous ? "continuous" : "limited-sweep");
    return id;
}

// Non-blocking init sweep trigger — runs inside Stepper_tick().
// X27 sweeps use the same acceleration as normal tracking (self-test proves accel works).
// 28BYJ sweeps use fixed usPerStep (no acceleration needed for geared motors).
void Stepper_startSweep(uint8_t pin1) {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        if (stepperArray[i].pins[0] != pin1) continue;
        StepperState& st = stepperArray[i];

        st.sweepSteps    = (st.totalSteps < 4096) ? st.totalSteps : 4096;
        st.sweepProgress = 0;
        st.sweepPhase    = SWEEP_FWD;
        st.time0         = micros();
        st.vel           = 0;
        st.microDelay    = 0;

        debugPrintf("[STEPPER] Init sweep started: %ld steps (%s, non-blocking)\n",
                    (long)st.sweepSteps,
                    (st.stateCount == 6) ? "accelerated" : "fixed speed");
        return;
    }
}

// Legacy blocking init sweep — use Stepper_startSweep() for boot instead.
void Stepper_initSweep(uint8_t pin1) {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        if (stepperArray[i].pins[0] != pin1) continue;
        StepperState& st = stepperArray[i];

        const int32_t sweepSteps = (st.totalSteps < 4096) ? st.totalSteps : 4096;
        const uint32_t stepDelay = st.usPerStep;

        debugPrintf("[STEPPER] Init sweep: %ld steps forward (%luus/step)...\n",
                    (long)sweepSteps, (unsigned long)stepDelay);

        for (int32_t s = 0; s < sweepSteps; ++s) {
            st.stateIndex = (st.stateIndex + 1) % st.stateCount;
            stepperApplyState(st);
            delayMicroseconds(stepDelay);
            if ((s & 0xFF) == 0) delay(0);
        }

        delay(0);
        debugPrintln("[STEPPER] Init sweep: returning to zero...");

        for (int32_t s = 0; s < sweepSteps; ++s) {
            st.stateIndex = (st.stateIndex + st.stateCount - 1) % st.stateCount;
            stepperApplyState(st);
            delayMicroseconds(stepDelay);
            if ((s & 0xFF) == 0) delay(0);
        }

        st.currentStep = 0;
        st.targetStep  = 0;
        st.stateIndex  = 0;
        debugPrintln("[STEPPER] Init sweep complete — holding at zero.");
        return;
    }
}

// Clamp/wrap target and wake motor if stopped (X27 accel needs this)
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
    // Wake from stopped state (SwitecX25::setPosition pattern)
    if (st.stopped && st.currentStep != targetStep) {
        st.stopped = false;
        st.time0 = micros();
        st.microDelay = 0;
    }
}

void Stepper_set(uint8_t pin1, int32_t targetStep) {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        if (stepperArray[i].pins[0] == pin1) {
            stepperApplyTarget(stepperArray[i], targetStep);
            return;
        }
    }
}

void Stepper_setById(uint8_t id, int32_t targetStep) {
    if (id >= stepperCount) return;
    stepperApplyTarget(stepperArray[id], targetStep);
}

void Stepper_tick() {
    uint32_t now = micros();

    for (uint8_t i = 0; i < stepperCount; ++i) {
        StepperState& st = stepperArray[i];
        if (!st.enabled) continue;

        // ── Non-blocking init sweep ──
        // X27: uses acceleration (same as normal tracking — sweep proves accel works)
        // 28BYJ: fixed usPerStep (geared motor doesn't need acceleration)
        if (st.sweepPhase != SWEEP_NONE) {

            // Timing gate — X27 uses accel-driven microDelay, 28BYJ uses fixed usPerStep
            uint16_t delayUs = (st.stateCount == 6) ? st.microDelay : st.usPerStep;
            if ((now - st.time0) < delayUs) continue;
            st.time0 = now;

            // Step in sweep direction
            int8_t dir = (st.sweepPhase == SWEEP_FWD) ? 1 : -1;
            if (dir > 0)
                st.stateIndex = (st.stateIndex + 1) % st.stateCount;
            else
                st.stateIndex = (st.stateIndex + st.stateCount - 1) % st.stateCount;
            stepperApplyState(st);
            st.sweepProgress++;

            // X27 acceleration during sweep (same logic as normal tracking)
            if (st.stateCount == 6) {
                int32_t remaining = st.sweepSteps - st.sweepProgress;
                if (remaining > 0) {
                    if (remaining < (int32_t)st.vel)        st.vel--;   // decelerate near end
                    else if (st.vel < X27_MAX_VEL)          st.vel++;   // accelerate
                } else {
                    if (st.vel > 0) st.vel--;
                }
                st.microDelay = x27LookupDelay(st.vel);
            }

            if (st.sweepProgress >= st.sweepSteps) {
                if (st.sweepPhase == SWEEP_FWD) {
                    st.sweepPhase    = SWEEP_BWD;
                    st.sweepProgress = 0;
                    st.vel           = 0;       // restart acceleration for return
                    st.microDelay    = 0;
                    debugPrintln("[STEPPER] Init sweep: returning to zero...");
                } else {
                    st.sweepPhase  = SWEEP_NONE;
                    st.currentStep = 0;
                    st.stateIndex  = 0;
                    st.vel         = 0;
                    st.dir         = 0;
                    st.stopped     = true;
                    stepperApplyState(st);
                    debugPrintln("[STEPPER] Init sweep complete — holding at zero.");
                }
            }
            continue;
        }

        // ── X27 with acceleration (stateCount == 6) ──
        // Ported from SwitecX25::advance() — accel/decel/coast logic.
        if (st.stateCount == 6) {
            if (st.stopped) {
                // De-energize after 2s idle
                if (st.time0 && (now - st.time0) > 2000000UL) {
                    stepperAllOff(st);
                    st.time0 = 0;
                }
                continue;
            }

            if ((now - st.time0) < st.microDelay) continue;

            // Detect arrival at target
            if (st.currentStep == st.targetStep && st.vel == 0) {
                st.stopped = true;
                st.dir = 0;
                st.time0 = now;
                continue;
            }

            // Start from rest — pick direction
            if (st.vel == 0) {
                st.dir = (st.currentStep < st.targetStep) ? 1 : -1;
                st.vel = 1;
            }

            // Advance one step
            stepperStep(st, st.dir);

            // Accel/decel logic (from SwitecX25)
            int32_t delta = (st.dir > 0)
                ? (st.targetStep  - st.currentStep)
                : (st.currentStep - st.targetStep);

            if (delta > 0) {
                if (delta < (int32_t)st.vel)    st.vel--;       // decelerate
                else if (st.vel < X27_MAX_VEL)  st.vel++;       // accelerate
                // else: at max speed, hold
            } else {
                st.vel--;  // moving away from target (target changed mid-move)
            }

            st.microDelay = x27LookupDelay(st.vel);
            st.time0 = now;
            continue;
        }

        // ── 28BYJ-48 with fixed speed (stateCount == 8) ──
        // Original working logic — one step per tick, rate-limited by usPerStep.
        if (st.currentStep == st.targetStep) {
            if (st.time0 && (now - st.time0) > 2000000UL) {
                stepperAllOff(st);
                st.time0 = 0;
            }
            continue;
        }

        if ((now - st.time0) < st.usPerStep) continue;
        st.time0 = now;

        int32_t delta = st.targetStep - st.currentStep;

        if (st.continuous) {
            int32_t half = (int32_t)st.totalSteps / 2;
            if (delta > half)        delta -= st.totalSteps;
            else if (delta < -half)  delta += st.totalSteps;
        }

        int8_t dir = (delta > 0) ? 1 : -1;
        stepperStep(st, dir);
    }
}

void Stepper_setAllOff() {
    for (uint8_t i = 0; i < stepperCount; ++i) {
        stepperAllOff(stepperArray[i]);
    }
}
