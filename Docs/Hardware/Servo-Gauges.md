# Servo Gauges

Servo gauges use standard hobby servo motors to drive physical needles on analog instrument faces. DCS-BIOS values are mapped to servo angles, giving you a real needle that moves in response to simulator data -- fuel quantity, hydraulic pressure, engine RPM, and more.

---

## What Analog Gauges Are

A servo gauge consists of:

1. A printed or fabricated gauge face with markings
2. A hobby servo motor mounted behind the face
3. A needle attached to the servo horn
4. CockpitOS firmware that maps DCS-BIOS values to servo positions

```
+----------------------------------------------------------------------+
|  SERVO GAUGE CONCEPT                                                 |
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
|          \ servo   /        +-----------+                            |
|           +--------+              |                                  |
|                                   v                                  |
|                            PWM signal to                             |
|                            servo motor                               |
|                                                                      |
+----------------------------------------------------------------------+
```

---

## LEDMapping.h Configuration

Servo gauges are configured in LEDMapping.h using the `DEVICE_GAUGE` device type:

```cpp
//  label           deviceType    info                                          dimmable  activeLow
{ "FUEL_QTY_L",    DEVICE_GAUGE, { .gaugeInfo = { PIN(18), 1000, 2000, 20000 } }, true,   false },
```

**Field reference for gaugeInfo:**

| Field    | Example    | Meaning                                       |
|----------|------------|-----------------------------------------------|
| gpio     | `PIN(18)`  | PWM-capable GPIO pin for the servo signal     |
| minPulse | `1000`     | Minimum pulse width in microseconds (0 degrees) |
| maxPulse | `2000`     | Maximum pulse width in microseconds (180 degrees) |
| period   | `20000`    | PWM period in microseconds (20000 = 50Hz)     |

The `dimmable` flag should be set to `true` for servo gauges so CockpitOS maps the full DCS-BIOS value range to the servo's pulse range.

---

## Servo Motor Basics

Standard hobby servos use a PWM (Pulse Width Modulation) signal to set their position:

```
+----------------------------------------------------------------------+
|  SERVO PWM CONTROL                                                   |
+----------------------------------------------------------------------+
|                                                                      |
|  Period: 20ms (50Hz)                                                 |
|                                                                      |
|  Pulse width determines position:                                    |
|                                                                      |
|  ~1000us (1ms)  =  0 degrees   (full left / minimum)                |
|  ~1500us (1.5ms) = 90 degrees  (center)                             |
|  ~2000us (2ms)  = 180 degrees  (full right / maximum)               |
|                                                                      |
|       -----+                           +---------------------        |
|            |                           |                             |
|            |<-- 1ms pulse -->|<-- 19ms off -->|     (0 deg)          |
|            +-------------------+-------+                             |
|                                                                      |
|       ---------+                       +---------------------        |
|                |                       |                             |
|                |<-- 2ms pulse -->|<-- 18ms off -->| (180 deg)        |
|                +-------------------+---+                             |
|                                                                      |
+----------------------------------------------------------------------+
```

---

## Wiring

### Basic Connection

```
                    ESP32                         Servo
                 +---------+                   +---------+
                 |         |                   |         |
                 | GPIO 18 +-------------------| Signal  |
                 |         |                   | (PWM)   |
                 |         |                   |         |
                 |   GND   +-------------------| GND     |
                 |         |                   | (Brown) |
                 |         |                   |         |
                 |         |      5V Supply ---| VCC     |
                 |         |                   | (Red)   |
                 +---------+                   +---------+
```

### Servo Wire Colors

Most servos use a 3-wire connector:

| Wire Color         | Function |
|--------------------|----------|
| Brown or Black     | GND      |
| Red                | VCC (5V) |
| Orange or White    | Signal   |

### Power Warning

**Do not power servos from the ESP32's 5V pin.** Servos draw significant current (100-500mA, with peaks above 1A during rapid movement). Always use a separate 5V power supply with a common ground connection to the ESP32.

```
    +-----------------------------------------------------------+
    |                 5V Power Supply (2A+)                      |
    +-------+----------------------------+---------------------+
            |                            |
    +-------+-------+           +--------+--------+
    |    ESP32       |           |   Servo Motor   |
    |   (via USB)    |           |   (5V power)    |
    +-------+-------+           +--------+--------+
            |                            |
            +----------------------------+
                   COMMON GROUND (required!)
```

Without a common ground, the servo will not respond to the PWM signal reliably, and ground loops can cause erratic behavior or damage.

---

## Value Mapping

CockpitOS maps the 16-bit DCS-BIOS value (0-65535) to the servo's configured pulse range:

```
pulse = minPulse + ((value * (maxPulse - minPulse)) / 65535)
```

**Example:**

| DCS-BIOS Value | Calculation                            | Pulse Width | Servo Position |
|----------------|----------------------------------------|-------------|----------------|
| 0              | 1000 + (0 * 1000 / 65535)              | 1000 us     | 0 degrees      |
| 16383          | 1000 + (16383 * 1000 / 65535)          | 1250 us     | 45 degrees     |
| 32767          | 1000 + (32767 * 1000 / 65535)          | 1500 us     | 90 degrees     |
| 49151          | 1000 + (49151 * 1000 / 65535)          | 1750 us     | 135 degrees    |
| 65535          | 1000 + (65535 * 1000 / 65535)          | 2000 us     | 180 degrees    |

This provides 16-bit precision for smooth, fine-grained needle movement.

---

## Calibration

### Adjusting Pulse Range

The `minPulse` and `maxPulse` values determine the servo's travel range. Default values of 1000 and 2000 microseconds work for most servos, but you may need to adjust them:

- **Needle does not reach full scale**: Increase `maxPulse` (try 2100 or 2200)
- **Needle overshoots full scale**: Decrease `maxPulse`
- **Needle does not return to zero**: Decrease `minPulse` (try 900 or 800)
- **Needle sits past zero mark**: Increase `minPulse`
- **Gauge only uses 90 degrees of travel**: Set `minPulse` and `maxPulse` to span only the needed range (e.g., 1000 to 1500 for 90 degrees)

---

## Update Rate

Servo positions update at a fixed rate defined in Config.h:

```cpp
#define SERVO_UPDATE_FREQ_MS    20    // 50Hz update rate (standard for hobby servos)
```

- **20ms (50Hz)** is the standard servo update rate and works well for most servos
- Lower values may cause jitter on some servos
- Higher values reduce needle responsiveness
- Most hobby servos expect a 50Hz refresh rate

---

## Multiple Servos

Each servo uses one GPIO pin. Configure each as a separate row in LEDMapping.h:

```cpp
{ "FUEL_QTY_L",   DEVICE_GAUGE, { .gaugeInfo = { PIN(18), 1000, 2000, 20000 } }, true, false },
{ "FUEL_QTY_R",   DEVICE_GAUGE, { .gaugeInfo = { PIN(19), 1000, 2000, 20000 } }, true, false },
{ "OIL_PRESS_L",  DEVICE_GAUGE, { .gaugeInfo = { PIN(20),  900, 2100, 20000 } }, true, false },
```

Each servo can have different minPulse/maxPulse values to account for different gauge face layouts and mechanical travel requirements.

---

## Label Creator Configuration

1. Open Label Creator and select your Label Set
2. Click **Edit LEDs**
3. Find the gauge indicator label (must match a DCS-BIOS output)
4. Set **Device** = `GAUGE`
5. Set the GPIO pin, minPulse, maxPulse, and period values
6. Set **Dimmable** = `true` (required for value-to-pulse mapping)
7. Save

---

## Practical Tips

### Servo Selection

- **Micro servos** (SG90, MG90S) are common for cockpit gauges
- Metal-gear servos (MG90S) are more precise and durable than plastic-gear (SG90)
- Digital servos offer better precision and holding torque than analog servos
- For gauges that need very fine resolution, consider servos with higher torque ratings

### Mechanical Mounting

- Mount the servo behind the gauge face with the shaft centered on the needle pivot
- Use a small servo horn or custom adapter to connect the needle
- Ensure the needle can move freely through the full range without hitting stops
- Test the full sweep range before finalizing your mount

### Noise and Jitter

If your servo jitters at rest:

1. Add a 100uF capacitor across the servo's power and ground wires
2. Keep signal wires away from motor power wires
3. Ensure the power supply can handle peak current demands
4. Verify the common ground connection is solid
5. Try increasing `SERVO_UPDATE_FREQ_MS` slightly (25ms instead of 20ms)

---

## Quick Reference

| Parameter              | Default | Purpose                                    |
|------------------------|---------|--------------------------------------------|
| `SERVO_UPDATE_FREQ_MS` | 20      | Update interval in ms (20ms = 50Hz)       |
| `minPulse`             | 1000    | Minimum servo pulse in microseconds        |
| `maxPulse`             | 2000    | Maximum servo pulse in microseconds        |
| `period`               | 20000   | PWM period in microseconds (50Hz standard) |

| Device Type    | `DEVICE_GAUGE`                                     |
|----------------|----------------------------------------------------|
| Info struct    | `{ .gaugeInfo = { gpio, minPulse, maxPulse, period } }` |
| Dimmable       | Set to `true` for value-to-pulse mapping            |
| Active Low     | Set to `false` (not applicable to servos)           |

---

*See also: [Hardware Overview](README.md) | [TFT Gauges](TFT-Gauges.md) | [LEDs](LEDs.md)*
