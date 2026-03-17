# Stepper Motors

Stepper motors drive physical gauge needles with precise step-by-step rotation. Unlike servo gauges (which use PWM pulse widths), stepper gauges count discrete steps to position a needle -- giving you exact control over travel angle, wrap-around behavior, and multi-revolution instruments like altimeters.

---

## What Stepper Gauges Are

A stepper gauge consists of:

1. A printed or fabricated gauge face with markings
2. A 4-wire stepper motor mounted behind the face
3. A needle attached to the motor shaft
4. CockpitOS firmware that maps DCS-BIOS values to step positions

```
+----------------------------------------------------------------------+
|  STEPPER GAUGE CONCEPT                                               |
+----------------------------------------------------------------------+
|                                                                      |
|           +--------+                                                 |
|          /  gauge   \          DCS-BIOS value                        |
|         /   face     \         (0-65535)                             |
|        |   markings   |            |                                 |
|        |      |       |            v                                 |
|        |   needle     |     +-----------+                            |
|        |      |       |     | CockpitOS |                            |
|         \    [*]     /      | firmware  |                            |
|          \ stepper  /       +-----------+                            |
|           +--------+              |                                  |
|                            8-phase half-step                         |
|                            sequence on 4 GPIO                        |
|                            pins (or via driver)                      |
|                                                                      |
+----------------------------------------------------------------------+
```

---

## Supported Motor Types

CockpitOS supports two 4-wire unipolar stepper motors:

### X27.168 / VID29 (Direct Drive)

```
+----------------------------------------------------------------------+
|  X27.168 / VID29                                                     |
+----------------------------------------------------------------------+
|                                                                      |
|  Steps per revolution:  720 half-steps                               |
|  Minimum step timing:   100 us/step                                  |
|  Maximum sweep (stock):  ~315 degrees (mechanical end stop)          |
|  Maximum sweep (modded): 360 degrees (end stop removed)              |
|  Driver board:           NONE -- wires directly to ESP32 GPIO        |
|  Current draw:           ~20mA per coil (safe for ESP32 GPIO)        |
|                                                                      |
|  Best for: Oil pressure, hydraulic pressure, fuel quantity,          |
|            RPM, cabin pressure -- any gauge under 360 degrees.       |
|            With end stop removed: altimeters, heading indicators.    |
|                                                                      |
+----------------------------------------------------------------------+
```

The X27.168 (also sold as VID29-05P or Switec) is the same motor used in real automotive instrument clusters. It is fast, silent, and draws so little current that it connects directly to ESP32 GPIO pins with no driver board.

**Stock vs Modded:** The stock X27.168 has a mechanical end stop that limits rotation to about 315 degrees. For gauges that need full 360-degree continuous rotation (like altimeter needles), remove the end stop. This is a common modification -- search "X27.168 remove end stop" for guides.

### 28BYJ-48 + ULN2003 (Geared)

```
+----------------------------------------------------------------------+
|  28BYJ-48 + ULN2003                                                  |
+----------------------------------------------------------------------+
|                                                                      |
|  Steps per revolution:  4096 half-steps                              |
|  Optimal step timing:   1000 us/step (below ~800us it stalls)       |
|  Maximum sweep:          Unlimited 360-degree rotation               |
|  Driver board:           ULN2003 (required -- do NOT skip)           |
|  Current draw:           ~240mA per coil (too high for direct GPIO)  |
|                                                                      |
|  Best for: Altimeters, heading indicators, clocks, throttles --     |
|            any instrument that needs continuous multi-turn rotation.  |
|                                                                      |
+----------------------------------------------------------------------+
```

The 28BYJ-48 is a geared motor -- cheap, widely available, and very strong. The internal gearbox gives it excellent holding torque and fine resolution (4096 steps per revolution = 0.088 degrees per step). However, the gearing makes it slower than the X27.168.

**ULN2003 driver board is mandatory.** The 28BYJ-48 draws too much current for direct ESP32 GPIO. The ULN2003 board amplifies the GPIO signals and provides the motor current from a separate 5V supply.

**Speed warning:** Do not set usPerStep below 800. The gearbox cannot keep up and the motor will vibrate in place without turning. The tested optimal value is 1000 us/step.

---

## Choosing Between Motors

| Factor | X27.168 | 28BYJ-48 |
|--------|---------|----------|
| Speed | Fast (100us/step) | Slow (1000us/step) |
| Resolution | 720 steps/rev (0.5 deg) | 4096 steps/rev (0.088 deg) |
| Driver board | None needed | ULN2003 required |
| Wiring | 4 wires to GPIO | 4 wires to ULN2003 board |
| Cost | ~$3-5 each | ~$1-2 each (with board) |
| Torque | Low (direct drive) | High (geared) |
| Continuous 360 | Requires end stop removal | Always supported |
| Noise | Silent | Audible hum when moving |
| Best sweep range | Under 360 degrees | Any range |

**Rule of thumb:** Use the X27.168 for gauges with a fixed sweep (oil pressure, fuel, RPM). Use the 28BYJ-48 for instruments that wrap around (altimeters, heading indicators) or need very fine resolution.

---

## LEDMapping.h Configuration

Stepper motors are configured in LEDMapping.h using the `DEVICE_STEPPER` device type:

```cpp
//  label                deviceType       info                                                            dimmable  activeLow
{ "STBY_ALT_100_FT_PTR", DEVICE_STEPPER, {.stepperInfo = {34, 36, 38, 40, 600, 100, false}}, false, false },
```

**Field reference for stepperInfo:**

| Field | Example | Meaning |
|-------|---------|---------|
| pin1 | `34` | GPIO pin for motor coil IN1 |
| pin2 | `36` | GPIO pin for motor coil IN2 |
| pin3 | `38` | GPIO pin for motor coil IN3 |
| pin4 | `40` | GPIO pin for motor coil IN4 |
| totalSteps | `600` | Total steps for the gauge's full range |
| usPerStep | `100` | Microseconds per step (motor speed limit) |
| continuous | `false` | `true` = 360-degree wraparound, `false` = limited sweep |

**totalSteps** is NOT always the motor's steps-per-revolution. It is the number of steps for your gauge's usable range. For a 300-degree gauge on an X27.168: `720 * 300 / 360 = 600 steps`. The Label Creator calculates this automatically from the angle you enter.

**continuous** controls how the firmware moves the needle:
- `false` (limited sweep): target is clamped to `[0, totalSteps-1]`. The needle moves directly from current to target. Used for partial-arc gauges (oil pressure, fuel, RPM).
- `true` (wraparound): the firmware takes the shortest path around the circle, wrapping across the 0/max boundary. Used for instruments where the DCS value wraps (altimeter needles crossing 1000 ft, heading crossing 360).

---

## Wiring

### X27.168 (Direct to GPIO)

The X27.168 draws ~20mA per coil -- well within the ESP32's GPIO current limit. No driver board needed.

```
                    ESP32                       X27.168
                 +---------+                  +---------+
                 |         |                  |         |
                 | GPIO 34 +------------------| Coil 1  |
                 | GPIO 36 +------------------| Coil 2  |
                 | GPIO 38 +------------------| Coil 3  |
                 | GPIO 40 +------------------| Coil 4  |
                 |         |                  |         |
                 |   GND   +------------------| GND     |
                 |         |                  |         |
                 +---------+                  +---------+
```

The X27.168 typically has 4 colored wires. Pin order varies by manufacturer -- check your datasheet. If the needle spins the wrong direction, swap pin2 and pin3 in LEDMapping.h.

### 28BYJ-48 + ULN2003 (Driver Board Required)

```
         ESP32                  ULN2003 Board              28BYJ-48
      +---------+              +-------------+            +--------+
      |         |              |             |            |        |
      | GPIO 5  +----- IN1 ---| IN1    OUT1 |---+        |        |
      | GPIO 6  +----- IN2 ---| IN2    OUT2 |---+-- Motor connector
      | GPIO 7  +----- IN3 ---| IN3    OUT3 |---+   (5-pin header)
      | GPIO 8  +----- IN4 ---| IN4    OUT4 |---+        |        |
      |         |              |             |            +--------+
      |   GND   +-------- GND-| GND     VCC |---+
      |         |              +-------------+   |
      +---------+                                |
                                            External 5V
                                            Power Supply
```

**Power rules:**
- The ULN2003 board needs 5V power. Use an external supply, NOT the ESP32's 5V pin.
- Connect the ESP32 GND and the external 5V supply GND together (common ground).
- The 28BYJ-48 draws ~240mA when active. A single USB port cannot reliably power both an ESP32 and a stepper motor.

---

## Value Mapping

CockpitOS maps the 16-bit DCS-BIOS value (0-65535) to a step position:

```
target = (uint32_t)rawValue * totalSteps / 65536
```

**Examples with a 300-degree gauge (600 steps, X27.168):**

| DCS-BIOS Value | Calculation | Target Step | Needle Position |
|----------------|-------------|-------------|-----------------|
| 0 | 0 * 600 / 65536 | 0 | 0 degrees |
| 16383 | 16383 * 600 / 65536 | 149 | ~75 degrees |
| 32767 | 32767 * 600 / 65536 | 299 | ~150 degrees |
| 49151 | 49151 * 600 / 65536 | 449 | ~225 degrees |
| 65535 | 65535 * 600 / 65536 | 599 | ~300 degrees |

**Examples with a 360-degree altimeter (720 steps, X27.168 modded, continuous):**

| DCS-BIOS Value | Target Step | Needle Position |
|----------------|-------------|-----------------|
| 0 | 0 | 12 o'clock |
| 32767 | 359 | 6 o'clock |
| 65535 | 719 | Just before 12 o'clock |

With `continuous=true`, if the needle is at step 10 and the target jumps to step 710, the firmware moves 20 steps backward (shortest path) instead of 700 steps forward.

---

## Non-Blocking Operation

The stepper engine is completely non-blocking. It runs inside the main loop without ever calling `delay()`.

**How it works:**
1. `setLED()` calculates the target step from the DCS-BIOS value and calls `Stepper_set()`.
2. `Stepper_tick()` is called every frame from `tickOutputDrivers()`.
3. For each stepper, `Stepper_tick()` computes how many steps the motor should have advanced since the last tick (based on elapsed time and `usPerStep`).
4. It jumps directly to the final position using arithmetic (no per-step loop), then applies the coil pattern once via ESP-IDF `gpio_set_level()`.
5. If the target equals the current position, no work is done.

This means the motor moves at its true physical speed regardless of the main loop rate. An X27.168 at 100us/step advances up to 40 steps per tick at 250Hz, reaching full sweep (720 steps) in ~72ms. A 28BYJ-48 at 1000us/step advances up to 4 steps per tick, reaching full revolution in ~4.1s.

### Idle De-Energize

After 2 seconds at the target position, all coils are automatically de-energized:
- Saves power (no current flowing through coils)
- Prevents motor overheating
- The gearbox/friction holds the needle position with coils off
- Coils re-energize instantly when the target changes

---

## Startup Behavior

At power-up, each stepper goes through a two-phase initialization:

### Phase 1: Registration
- All 4 pins are configured as OUTPUT
- Phase 0 of the half-step sequence is applied (motor snaps to detent)
- **This is the needle attachment point** -- mount your needle at the zero mark while the motor holds phase 0

### Phase 2: Init Sweep (Self-Test)
- Motor sweeps forward to `totalSteps` (capped at 4096 steps max)
- Motor sweeps backward to step 0
- Confirms the needle returns to zero accurately
- Uses the motor's configured speed (fast for X27, slower for 28BYJ)
- **Non-blocking** -- runs inside `Stepper_tick()` concurrently with other startup tasks
- All steppers sweep at the same time; boot time = slowest motor, not the sum

The init sweep is the same visual self-test that servo gauges perform at startup. Watch it to verify your needle travel matches the gauge face markings. During the sweep, the main loop runs normally -- network, DCS-BIOS, and LED updates are all active.

---

## Calibration

### Setting the Correct Angle

The `totalSteps` value determines how far the needle travels for a full DCS-BIOS value range (0-65535). The Label Creator calculates this from the angle you enter:

```
totalSteps = stepsPerRevolution * angle / 360
```

| Motor | Steps/Rev | 90 deg | 180 deg | 270 deg | 300 deg | 315 deg | 360 deg |
|-------|-----------|--------|---------|---------|---------|---------|---------|
| X27.168 | 720 | 180 | 360 | 540 | 600 | 630 | 720 |
| 28BYJ-48 | 4096 | 1024 | 2048 | 3072 | 3413 | 3584 | 4096 |

### If the Needle Doesn't Match the Gauge Face

- **Needle travels too far:** Decrease the angle (fewer totalSteps)
- **Needle doesn't reach full scale:** Increase the angle (more totalSteps)
- **Needle spins the wrong direction:** Swap pin2 and pin3 in LEDMapping.h
- **Needle starts at the wrong position:** Re-mount the needle while the motor holds phase 0 at power-up (before init sweep begins)

### Continuous vs Limited Sweep

Choose `continuous=true` ONLY when the DCS-BIOS value represents a wrapping quantity:
- Altimeter needle (wraps every 1000 ft) -- continuous
- Heading indicator (wraps at 360) -- continuous
- Oil pressure (0-100 PSI, never wraps) -- limited sweep
- Fuel quantity (0-full, never wraps) -- limited sweep

---

## Multiple Steppers

Each stepper uses 4 GPIO pins. CockpitOS supports up to `MAX_STEPPERS=4` stepper motors simultaneously.

```cpp
{ "STBY_ALT_100_FT_PTR", DEVICE_STEPPER, {.stepperInfo = {34, 36, 38, 40, 720, 100, true}},  false, false },
{ "HYD_IND_BRAKE",       DEVICE_STEPPER, {.stepperInfo = {5,  6,  7,  8,  60,  100, false}}, false, false },
{ "PRESSURE_ALT",        DEVICE_STEPPER, {.stepperInfo = {9, 10, 11, 12, 600, 1000, false}}, false, false },
```

Each stepper runs independently with its own speed, step count, and wrap behavior.

---

## Label Creator Configuration

The Label Creator provides a guided wizard for stepper configuration:

1. Open Label Creator and select your Label Set
2. Click **Edit LEDs**
3. Find the gauge indicator label (must match a DCS-BIOS output)
4. Set **Device** = `STEPPER`
5. The wizard walks you through:
   - **Motor type:** X27.168 or 28BYJ-48 (sets speed and steps-per-rev automatically)
   - **GPIO pins:** IN1 through IN4 (with motor-specific wiring hints)
   - **Needle behavior:** "Wraps around 360 degrees" or "Partial sweep (limited angle)"
   - **Sweep angle:** In degrees, with gauge-type examples
6. Save

The wizard calculates `totalSteps` and `usPerStep` automatically based on the motor type and angle. You never need to calculate raw step counts.

---

## Real-World Gauge Examples

### Cabin Pressure Altimeter (~300 degrees, X27.168)

DCS-BIOS output `PRESSURE_ALT` (0-65535, maps to 0-50,000 ft). Gauge face spans approximately 300 degrees.

```cpp
{ "PRESSURE_ALT", DEVICE_STEPPER, {.stepperInfo = {34, 36, 38, 40, 600, 100, false}}, false, false },
// X27.168, 300 deg = 600 steps, limited sweep
```

### Brake Pressure (~30 degrees, X27.168)

DCS-BIOS output `HYD_IND_BRAKE` (0-65535, maps to 0-4000 PSI). Tiny gauge arc of about 30 degrees.

```cpp
{ "HYD_IND_BRAKE", DEVICE_STEPPER, {.stepperInfo = {5, 6, 7, 8, 60, 100, false}}, false, false },
// X27.168, 30 deg = 60 steps, limited sweep
```

### Standby Altimeter Needle (360 degrees, X27.168 modded)

DCS-BIOS output `STBY_ALT_100_FT_PTR` (0-65535, maps to one full revolution representing 0-1000 ft). The DCS value wraps -- when the altimeter passes 1000 ft, the value resets to 0.

```cpp
{ "STBY_ALT_100_FT_PTR", DEVICE_STEPPER, {.stepperInfo = {34, 36, 38, 40, 720, 100, true}}, false, false },
// X27.168 modded (end stop removed), 360 deg = 720 steps, continuous wraparound
```

---

## Practical Tips

### Needle Attachment
- Power up the ESP32 and wait for the motor to hold phase 0
- Attach the needle pointing at the zero mark on your gauge face
- The init sweep will then confirm the full travel range

### Power Management
- Stepper coils auto-de-energize after 2 seconds idle
- Calling `GPIO_setAllLEDs(false)` de-energizes all stepper coils immediately
- The gearbox (28BYJ-48) or shaft friction (X27.168) holds position with coils off

### Pin Selection
- X27.168: any GPIO pins work (low current draw)
- 28BYJ-48: pins connect to ULN2003 board, not the motor -- any GPIO works
- Avoid strapping pins (GPIO 0, 2, 12, 15 on Classic; GPIO 0 on S2/S3)

### Noise
- X27.168: essentially silent
- 28BYJ-48: audible hum/buzz during movement (normal -- it's the gearbox)
- If a 28BYJ-48 vibrates but doesn't turn, increase usPerStep (try 1000 or higher)

---

## Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| Motor does not move at all | Wrong GPIO pins, motor not wired | Verify pin numbers in LEDMapping.h match your wiring; check all 4 connections |
| Motor vibrates but does not turn | usPerStep too low for 28BYJ-48 | Increase to 1000 or higher; the gearbox stalls below ~800us |
| Needle goes the wrong direction | Pin order reversed | Swap pin2 and pin3 in LEDMapping.h |
| Needle overshoots gauge markings | totalSteps too high (angle too large) | Decrease the angle in Label Creator |
| Needle does not reach full scale | totalSteps too low (angle too small) | Increase the angle in Label Creator |
| Needle jumps to wrong position after power cycle | Needle not attached at phase 0 | Re-mount needle: power up, wait for motor to hold, attach at zero mark |
| ESP32 resets during init sweep | 28BYJ-48 drawing power from ESP32 | Use external 5V power through ULN2003 board |
| Motor gets hot | Coils energized for too long | This should not happen -- coils de-energize after 2s idle. Check firmware version |
| Needle wraps around unexpectedly | continuous=true on a limited-sweep gauge | Set continuous=false in LEDMapping.h or Label Creator |

---

## Quick Reference

| Parameter | X27.168 | 28BYJ-48 |
|-----------|---------|----------|
| Steps per revolution | 720 | 4096 |
| usPerStep (minimum) | 100 | 800-1000 |
| Driver board | None | ULN2003 |
| Max simultaneous | 4 (MAX_STEPPERS) | 4 (MAX_STEPPERS) |
| Continuous rotation | Requires mod | Always supported |
| Current per coil | ~20mA | ~240mA |

| Device Type | `DEVICE_STEPPER` |
|-------------|------------------|
| Info struct | `{.stepperInfo = {pin1, pin2, pin3, pin4, totalSteps, usPerStep, continuous}}` |
| Dimmable | `false` (not applicable) |
| Active Low | `false` (not applicable) |

| Firmware Constant | Value |
|-------------------|-------|
| `MAX_STEPPERS` | 4 |
| Idle de-energize timeout | 2 seconds |
| Half-step phases | 8 |
| Init sweep cap | min(totalSteps, 4096) |

---

*See also: [Hardware Overview](README.md) | [Wire Stepper Motors](../How-To/Wire-Stepper-Motors.md) | [Servo Gauges](Servo-Gauges.md)*
