# How To: Wire Analog Gauges

Servo-driven analog gauges use standard hobby servo motors to move physical needles on printed gauge faces. DCS-BIOS sends 16-bit values (0-65535), and CockpitOS maps them to servo pulse widths that set the needle position.

---

## What You Need

| Item | Notes |
|---|---|
| Servo motor (SG90 or MG90S) | Micro servos fit behind most gauge faces. MG90S (metal gear) is more precise and durable than SG90 (plastic gear). |
| External 5V power supply (2A+) | Dedicated supply for servo power. Do NOT use the ESP32 power pins. |
| Hookup wire | Signal, power, and ground connections |
| Gauge face (printed or fabricated) | Markings for the needle to traverse |
| Servo horn or needle adapter | Connects the needle to the servo shaft |

---

## Step 1: Understand the Wiring

Each servo has three wires:

| Wire Color | Function |
|---|---|
| Brown or Black | GND |
| Red | VCC (5V) |
| Orange or White | Signal (PWM) |

**Critical rule: Do NOT power servos from the ESP32's 3.3V or 5V pins.** A single servo draws 100-500mA and can spike above 1A during fast movement. This will brown out or damage the ESP32. Always use a separate 5V power supply.

```
+----------------------------------------------------------------------+
|  WIRING DIAGRAM                                                       |
+----------------------------------------------------------------------+
|                                                                      |
|     External 5V Supply                                               |
|     +-----------+                                                    |
|     |  +5V  GND |                                                    |
|     +--+----+---+                                                    |
|        |    |                                                        |
|        |    +---------------------------+                            |
|        |                                |                            |
|        |    ESP32                        |    Servo Motor             |
|        |    +---------+                 |    +----------+            |
|        |    |         |                 |    |          |            |
|        |    | GPIO 18 +----- Signal --------+| Signal   |            |
|        |    |         |                 |    | (Orange) |            |
|        |    |         |                 +----| GND      |            |
|        |    |   GND   +---+                  | (Brown)  |            |
|        |    |         |   |             +----| VCC      |            |
|        |    +---------+   |             |    | (Red)    |            |
|        |                  |             |    +----------+            |
|        +------------------+-------------+                            |
|             COMMON GROUND (required!)                                |
|                                                                      |
+----------------------------------------------------------------------+
```

The ESP32 GND and the external 5V supply GND **must** be connected together (common ground). Without this, the PWM signal will not be referenced correctly and the servo will behave erratically.

---

## Step 2: Configure in LEDMapping.h

Servo gauges are configured as LED outputs with `DEVICE_GAUGE` device type. Each gauge gets one row in LEDMapping.h:

```cpp
//  label           deviceType    info                                          dimmable  activeLow
{ "FUEL_QTY_L",    DEVICE_GAUGE, { .gaugeInfo = { PIN(18), 1000, 2000, 20000 } }, true,   false },
```

**Field reference for gaugeInfo:**

| Field | Example | Meaning |
|---|---|---|
| gpio | `PIN(18)` | PWM-capable GPIO pin connected to the servo signal wire |
| minPulse | `1000` | Minimum pulse width in microseconds (needle at zero) |
| maxPulse | `2000` | Maximum pulse width in microseconds (needle at full scale) |
| period | `20000` | PWM period in microseconds (20000 = 50Hz, standard for servos) |

**Important:** The `dimmable` flag must be set to `true`. This tells CockpitOS to map the full DCS-BIOS value range (0-65535) to the servo's pulse range, rather than treating it as a binary on/off output.

---

## Step 3: Configure via Label Creator

1. Open Label Creator (`LabelCreator-START.py`) and select your label set
2. Click **Edit LEDs**
3. Find the gauge indicator label (must match a DCS-BIOS output, e.g., `FUEL_QTY_L`)
4. Set **Device** = `GAUGE`
5. Set **Port** = the GPIO pin number (e.g., 18)
6. Set **minPulse**, **maxPulse**, **period** values
7. Set **Dimmable** = `true`
8. Save

---

## Step 4: Calibrate the Pulse Range

DCS-BIOS sends a 16-bit value (0-65535). CockpitOS maps it to a servo pulse:

```
pulse = minPulse + ((value * (maxPulse - minPulse)) / 65535)
```

Default values of 1000-2000 microseconds work for most servos, but you will likely need to adjust them to match your gauge face:

| Problem | Fix |
|---|---|
| Needle does not reach full scale | Increase maxPulse (try 2100, 2200) |
| Needle overshoots full scale | Decrease maxPulse |
| Needle does not return to zero | Decrease minPulse (try 900, 800) |
| Needle sits past zero mark | Increase minPulse |
| Gauge only uses 90 degrees of travel | Set minPulse and maxPulse to span only 90 degrees (e.g., 1000 to 1500) |

### Startup Calibration Sweep

At power-up, CockpitOS automatically runs a calibration sweep for each servo:

1. Moves to minimum position (minPulse) -- holds 500ms
2. Moves to maximum position (maxPulse) -- holds 500ms
3. Returns to minimum position -- ready for DCS data

Watch this sweep to verify the mechanical travel matches your gauge face markings. If the sweep is too wide or too narrow, adjust minPulse/maxPulse and re-upload.

---

## Step 5: Set the Update Rate

The servo update rate is defined in `Config.h`:

```cpp
#define SERVO_UPDATE_FREQ_MS    20    // 50Hz update rate (standard for hobby servos)
```

- **20ms (50Hz)** is the standard and works for virtually all hobby servos
- Lowering this value may cause jitter on some servos
- Raising it reduces needle responsiveness but can help with jitter

---

## Multiple Gauges

Each servo needs its own GPIO pin. Configure each as a separate row in LEDMapping.h:

```cpp
{ "FUEL_QTY_L",   DEVICE_GAUGE, { .gaugeInfo = { PIN(18), 1000, 2000, 20000 } }, true, false },
{ "FUEL_QTY_R",   DEVICE_GAUGE, { .gaugeInfo = { PIN(19), 1000, 2000, 20000 } }, true, false },
{ "OIL_PRESS_L",  DEVICE_GAUGE, { .gaugeInfo = { PIN(20),  900, 2100, 20000 } }, true, false },
```

Each servo can have different minPulse/maxPulse values to account for different gauge face layouts and mechanical travel.

---

## Mounting Tips

- Mount the servo behind the gauge face with the shaft centered on the needle pivot point
- Use a small servo horn or custom 3D-printed adapter to attach the needle
- Ensure the needle can move freely through the full range without hitting mechanical stops
- If your gauge face only spans a portion of the servo's 180-degree range, set minPulse and maxPulse to match that subset
- Add a small deadband at the limits if the needle buzzes at the endpoints

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Servo jitters at rest | Insufficient power supply, noise on signal line | Add a 100uF capacitor across servo VCC/GND; ensure a solid common ground; try a higher-current power supply |
| Needle does not move at all | Wrong GPIO pin, DEVICE_GAUGE not set, no DCS-BIOS data | Verify the GPIO pin in LEDMapping.h matches your wiring; confirm Device is GAUGE, not GPIO; check DCS-BIOS connection |
| Needle moves to wrong range | minPulse/maxPulse not calibrated | Watch the startup sweep and adjust pulse values |
| Needle is inverted (zero at max) | minPulse and maxPulse are swapped | Swap the values, or physically rotate the servo 180 degrees |
| Needle moves in steps instead of smoothly | DCS-BIOS update rate is low | This is normal for some DCS-BIOS exports that update at low frequency; the servo will move as fast as data arrives |
| ESP32 resets when servo moves | Servo drawing power from ESP32 | Use an external 5V power supply. This is the most common mistake. |

---

## Quick Reference

| Parameter | Default | Purpose |
|---|---|---|
| `SERVO_UPDATE_FREQ_MS` | 20 | Update interval in ms (20ms = 50Hz) |
| `minPulse` | 1000 | Minimum servo pulse in microseconds |
| `maxPulse` | 2000 | Maximum servo pulse in microseconds |
| `period` | 20000 | PWM period in microseconds (50Hz standard) |

---

*See also: [Servo Gauges Reference](../Hardware/Servo-Gauges.md) | [TFT Gauges](Wire-TFT-Gauges.md) | [LEDs Reference](../Hardware/LEDs.md)*
