# How To: Wire Stepper Motors

Stepper-driven gauges use 4-wire stepper motors to move physical needles on gauge faces. DCS-BIOS sends 16-bit values (0-65535), and CockpitOS maps them to step positions using a non-blocking 8-phase half-step sequence.

---

## What You Need

### For X27.168 / VID29 (Direct Drive)

| Item | Notes |
|---|---|
| X27.168 or VID29-05P stepper motor | 720 steps/rev, ~315 deg stock sweep. Modded (end stop removed) for full 360 deg. |
| 4 hookup wires | Signal wires from ESP32 GPIO to motor coils |
| Gauge face (printed or fabricated) | Markings for the needle to traverse |
| Needle | Press-fit or glue to motor shaft |

No driver board or external power supply is needed -- the X27.168 draws only ~20mA per coil, well within ESP32 GPIO limits.

### For 28BYJ-48 (Geared, with ULN2003)

| Item | Notes |
|---|---|
| 28BYJ-48 stepper motor | 4096 steps/rev, continuous 360 deg. Cheap and widely available. |
| ULN2003 driver board | Required -- do NOT connect the 28BYJ-48 directly to ESP32 GPIO |
| External 5V power supply (1A+) | Powers the motor through the ULN2003 board |
| 4 hookup wires | Signal wires from ESP32 GPIO to ULN2003 IN1-IN4 |
| Gauge face and needle | Same as above |

---

## Step 1: Understand the Wiring

### X27.168 (Direct to GPIO)

The X27.168 has 4 wires that connect directly to any ESP32 GPIO pins:

```
+----------------------------------------------------------------------+
|  X27.168 WIRING (Direct to GPIO -- no driver board)                  |
+----------------------------------------------------------------------+
|                                                                      |
|     ESP32                            X27.168 Motor                   |
|     +---------+                     +----------+                     |
|     |         |                     |          |                     |
|     | GPIO 34 +---------------------| Coil 1   |                    |
|     | GPIO 36 +---------------------| Coil 2   |                    |
|     | GPIO 38 +---------------------| Coil 3   |                    |
|     | GPIO 40 +---------------------| Coil 4   |                    |
|     |         |                     |          |                    |
|     |   GND   +---------------------| GND      |                    |
|     |         |                     |          |                    |
|     +---------+                     +----------+                     |
|                                                                      |
|  That's it. No driver board, no external power.                      |
|                                                                      |
+----------------------------------------------------------------------+
```

### 28BYJ-48 + ULN2003 (Driver Board Required)

The 28BYJ-48 draws ~240mA -- far too much for direct GPIO. The ULN2003 board sits between the ESP32 and the motor:

```
+----------------------------------------------------------------------+
|  28BYJ-48 WIRING (ULN2003 driver board required)                     |
+----------------------------------------------------------------------+
|                                                                      |
|     External 5V Supply                                               |
|     +-----------+                                                    |
|     |  +5V  GND |                                                    |
|     +--+----+---+                                                    |
|        |    |                                                        |
|        |    +---------------------------+                            |
|        |                                |                            |
|        |    ESP32         ULN2003       |    28BYJ-48                 |
|        |    +--------+   +--------+    |    +---------+              |
|        |    |        |   |        |    |    |         |              |
|        |    | GPIO 5 +---| IN1    |    |    |  Motor  |              |
|        |    | GPIO 6 +---| IN2    |    |    | (plugs  |              |
|        |    | GPIO 7 +---| IN3    |    |    |  into   |              |
|        |    | GPIO 8 +---| IN4    |    |    |  board) |              |
|        |    |        |   |        |    |    |         |              |
|        |    |  GND   +-+-| GND    |    |    +---------+              |
|        |    |        | | |   VCC  |----+                             |
|        |    +--------+ | +--------+                                  |
|        |               |                                             |
|        +---------------+                                             |
|           COMMON GROUND (required!)                                  |
|                                                                      |
+----------------------------------------------------------------------+
```

**Critical rules:**
- The ESP32 GND and the external 5V supply GND **must** be connected together (common ground)
- The ULN2003 board's VCC connects to the external 5V supply, NOT the ESP32
- The 28BYJ-48 plugs directly into the ULN2003 board's motor connector (5-pin header)

---

## Step 2: Configure in LEDMapping.h

Stepper motors are configured as LED outputs with `DEVICE_STEPPER` device type. Each stepper gets one row in LEDMapping.h:

```cpp
//  label                deviceType       info                                                    dimmable  activeLow
{ "STBY_ALT_100_FT_PTR", DEVICE_STEPPER, {.stepperInfo = {34, 36, 38, 40, 720, 100, true}}, false, false },
```

**Field reference for stepperInfo:**

| Field | Example | Meaning |
|---|---|---|
| pin1 | `34` | GPIO pin connected to motor coil IN1 |
| pin2 | `36` | GPIO pin connected to motor coil IN2 |
| pin3 | `38` | GPIO pin connected to motor coil IN3 |
| pin4 | `40` | GPIO pin connected to motor coil IN4 |
| totalSteps | `720` | Number of steps for the full gauge range |
| usPerStep | `100` | Microseconds per step (motor speed limit) |
| continuous | `true` | `true` for 360-degree wraparound, `false` for limited sweep |

---

## Step 3: Configure via Label Creator

1. Open Label Creator (`LabelCreator-START.py`) and select your label set
2. Click **Edit LEDs**
3. Find the gauge indicator label (must match a DCS-BIOS output, e.g., `STBY_ALT_100_FT_PTR`)
4. Set **Device** = `STEPPER`
5. The wizard guides you through:
   - **Motor type:** pick X27.168 or 28BYJ-48
   - **GPIO pins:** IN1 through IN4 (with wiring hints specific to your motor)
   - **Needle behavior:** "Wraps around 360 degrees" or "Partial sweep"
   - **Angle:** enter degrees (e.g., 300 for cabin altimeter, 30 for brake pressure)
6. Save

The wizard automatically calculates `totalSteps` and `usPerStep` from the motor type and angle. No manual math required.

---

## Step 4: Attach the Needle

The needle attachment procedure is different from servo gauges. With steppers, the motor holds a precise reference position at power-up:

1. Power up the ESP32 -- the motor snaps to phase 0 (its zero detent)
2. While the motor is holding, press-fit or glue your needle pointing at the **zero mark** on the gauge face
3. The init sweep will run next (sweeps forward then back to zero) -- watch it to confirm the range matches your gauge markings
4. If the sweep overshoots or undershoots, adjust the angle in Label Creator and re-upload

---

## Step 5: Verify the Sweep

At power-up, CockpitOS automatically runs an init sweep for each stepper:

1. Sweeps forward to `totalSteps` (capped at 4096 to keep it short)
2. Sweeps backward to step 0
3. Motor holds at zero, ready for DCS data

**What to look for:**
- Needle should return to exactly the zero mark after the sweep
- Sweep should cover the full range of your gauge face markings
- Needle should move smoothly without stalling or skipping

If the 28BYJ-48 vibrates but does not turn during the sweep, increase `usPerStep` (try 1000 or higher).

---

## Step 6: Test in DCS

1. Start DCS World with DCS-BIOS running
2. Load a mission with the relevant aircraft
3. The gauge needle should respond to cockpit state changes
4. Check debug output for `[STEPPER]` messages confirming target positions

---

## Multiple Steppers

Each stepper uses 4 GPIO pins. Up to 4 steppers are supported simultaneously (MAX_STEPPERS=4):

```cpp
{ "STBY_ALT_100_FT_PTR", DEVICE_STEPPER, {.stepperInfo = {34, 36, 38, 40, 720, 100, true}},  false, false },
{ "HYD_IND_BRAKE",       DEVICE_STEPPER, {.stepperInfo = {5,  6,  7,  8,  60,  100, false}}, false, false },
{ "PRESSURE_ALT",        DEVICE_STEPPER, {.stepperInfo = {9, 10, 11, 12, 600, 1000, false}}, false, false },
```

Each stepper can use a different motor type, angle, and wrap behavior.

---

## Mounting Tips

- Mount the motor behind the gauge face with the shaft centered on the needle pivot
- X27.168 motors are very small -- they mount flush behind thin panel stock
- 28BYJ-48 motors are larger and heavier -- plan your panel depth accordingly
- The ULN2003 board can be mounted separately from the motor
- Keep motor wires short to avoid electrical noise (especially for the 28BYJ-48)
- For continuous-rotation gauges (altimeters), ensure the needle clears any obstructions through the full 360 degrees

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Motor does not move at all | Wrong GPIO pins or no wiring | Verify pin numbers match your wiring; check all 4 connections |
| Motor vibrates but does not turn | 28BYJ-48 speed too high | Increase usPerStep to 1000 or higher |
| Needle spins the wrong direction | Motor coil order reversed | Swap pin2 and pin3 in LEDMapping.h (or in Label Creator) |
| Needle does not reach full scale | Angle too small (too few totalSteps) | Increase the angle in Label Creator |
| Needle overshoots the gauge | Angle too large (too many totalSteps) | Decrease the angle in Label Creator |
| Needle starts at wrong position | Not attached at phase 0 | Re-mount: power up, wait for motor to hold, attach at zero mark |
| ESP32 resets on startup | 28BYJ-48 drawing power from ESP32 pins | Use external 5V supply through ULN2003 board |
| Needle wraps when it should not | continuous=true on a partial-sweep gauge | Set continuous=false |
| Needle takes the long way around | continuous=false on a wrapping instrument | Set continuous=true |

---

## Quick Reference

| Parameter | X27.168 | 28BYJ-48 |
|---|---|---|
| Steps per revolution | 720 | 4096 |
| usPerStep (recommended) | 100 | 1000 |
| Driver board | None | ULN2003 |
| External power | None | 5V 1A+ |
| Continuous 360 | Requires end stop removal | Always |

---

*See also: [Stepper Motor Reference](../Hardware/Stepper-Motors.md) | [Servo Gauges How-To](Wire-Analog-Gauges.md) | [Hardware Overview](../Hardware/README.md)*
