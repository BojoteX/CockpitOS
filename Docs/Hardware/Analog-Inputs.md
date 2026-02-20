# Analog Inputs

Analog inputs let you read continuous values from potentiometers, sliders, and other variable-resistance devices. CockpitOS maps the raw ADC reading to DCS-BIOS ranges automatically, with built-in calibration, dead zones, and noise filtering.

---

## How Potentiometers Work

A potentiometer (pot) is a three-terminal variable resistor. When wired as a voltage divider between 3.3V and GND, its wiper (center terminal) outputs a voltage proportional to the knob position.

```
                    ESP32
                 +---------+
                 |         |
                 |   3.3V  +-----------------+
                 |         |                 |
                 |         |              +--+--+
                 | GPIO 4  +--------------+WIPER+ (center terminal)
                 |  (ADC)  |              +--+--+
                 |         |                 |
                 |    GND  +-----------------+
                 |         |
                 +---------+

   Full CCW --> Wiper near GND   --> ADC reads ~0
   Center   --> Wiper at middle  --> ADC reads ~2048
   Full CW  --> Wiper near 3.3V  --> ADC reads ~4095

   ESP32 ADC is 12-bit: values range from 0 to 4095
```

### ADC-Capable Pins

Only specific GPIO pins support analog input. The available pins vary by board:

| Board Variant | Typical ADC Pins          |
|---------------|---------------------------|
| ESP32-S2      | GPIO 1-10                 |
| ESP32-S3      | GPIO 1-10                 |
| ESP32 Classic | GPIO 32-39 (ADC1), GPIO 25-27 (ADC2) |
| ESP32-C3      | GPIO 0-4                  |

Always check your specific board's pinout documentation. On ESP32 Classic, ADC2 pins cannot be used while Wi-Fi is active.

---

## InputMapping.h Configuration

An analog input requires one row with `controlType = "analog"`:

```cpp
//  label           source   port     bit  hidId  DCSCommand     value   Type      group
{ "HMD_OFF_BRT",   "GPIO",  PIN(18),  0,   -1,  "HMD_OFF_BRT", 65535,  "analog",    0 },
```

**Field reference:**

| Field       | Value             | Meaning                                     |
|-------------|-------------------|---------------------------------------------|
| label       | `"HMD_OFF_BRT"`   | Unique identifier                           |
| source      | `"GPIO"`          | Direct GPIO (ADC-capable pin required)       |
| port        | `PIN(18)`         | GPIO pin connected to pot wiper              |
| bit         | `0`               | Not used for analog, keep as 0               |
| hidId       | `-1`              | HID axis ID (-1 = no HID output)            |
| oride_label | `"HMD_OFF_BRT"`   | DCS-BIOS command name                       |
| oride_value | `65535`           | Special value meaning "send raw analog reading" |
| controlType | `"analog"`        | Continuous analog input                      |
| group       | `0`               | Analog inputs do not use groups              |

The `oride_value = 65535` is the key marker that tells CockpitOS this entry is an analog axis. The firmware reads the ADC, maps it to the DCS-BIOS range, and sends the value.

---

## Companion Encoder Entries

In many label sets, an analog control also has companion `variable_step` entries for DEC/INC. These allow the same DCS-BIOS control to be driven by either a potentiometer or a rotary encoder, depending on how the hardware is wired.

```cpp
{ "HMD_OFF_BRT",     "GPIO", PIN(18), 0, -1, "HMD_OFF_BRT", 65535, "analog",        0 },
{ "HMD_OFF_BRT_DEC", "NONE",  0,      0, -1, "HMD_OFF_BRT",     0, "variable_step", 0 },
{ "HMD_OFF_BRT_INC", "NONE",  0,      0, -1, "HMD_OFF_BRT",     1, "variable_step", 0 },
```

When the pot is the physical input, the DEC/INC rows remain with `source = "NONE"` (disabled). If you switch to a rotary encoder, set the DEC/INC rows to GPIO and disable the analog row.

---

## Axis Auto-Learning

CockpitOS includes automatic min/max learning for analog axes. As you move the potentiometer through its range, the firmware expands the observed minimum and maximum values and stores them in non-volatile storage (NVS).

### How It Works

1. On first boot, the axis starts with conservative default boundaries
2. As you move the pot, the firmware tracks the lowest and highest ADC readings
3. These learned boundaries are saved to NVS and persist across reboots
4. The full DCS-BIOS output range (0-65535) is mapped across the learned min/max

This means you do not need to manually calibrate most axes -- just move the pot to both extremes once and the firmware remembers.

---

## Calibration Settings in Config.h

For fine-tuning, several parameters in Config.h control axis behavior:

### Default Boundaries

```cpp
#define AX_DEFAULT_MIN    768   // Assumed worst-case minimum
#define AX_DEFAULT_MAX   3327   // Assumed worst-case maximum
```

These are the starting boundaries before auto-learning expands them. For completely fresh calibration, set `AX_DEFAULT_MIN = 4095` and `AX_DEFAULT_MAX = 0`, then move each pot through its full range.

### Center Dead Zone

For controls that have a meaningful center position (like a trim axis), the center dead zone prevents jitter around the midpoint.

```cpp
#define CENTER_DEADZONE_INNER   256   // Entry threshold -- easy to enter
#define CENTER_DEADZONE_OUTER   384   // Exit threshold -- must move further to escape
```

The inner/outer hysteresis prevents the value from flickering in and out of the dead zone.

### Threshold Tuning

These control how close to the extremes (min/max) and center the value can reach:

```cpp
#define MIDDLE_AXIS_THRESHOLD    64   // Center snap range (32-64 optimal, 128-256 for noisy axes)
#define UPPER_AXIS_THRESHOLD    128   // Max snap range
#define LOWER_AXIS_THRESHOLD    256   // Min snap range
```

If your axis does not quite reach 0% or 100%, increase the corresponding threshold. If the center position is too "sticky," decrease `MIDDLE_AXIS_THRESHOLD`.

---

## Noise Reduction

### Firmware Filtering

By default, CockpitOS applies a moving-average filter to analog inputs to smooth out noise. This adds a small amount of latency but eliminates jitter.

```cpp
#define SKIP_ANALOG_FILTERING   0   // 0 = filtering ON (default), 1 = skip for minimum latency
```

Set `SKIP_ANALOG_FILTERING = 1` only if you need the absolute lowest input latency (HID axis mode) and your hardware is clean.

### Hardware Noise Reduction

For noisy potentiometers, add a small capacitor across the pot:

```
                 +---+
   3.3V ---------| P |--------- GND
                 | O |
   Wiper --------| T |
                 +---+
                   |
                 [ADC]
                   |
            +------+------+
            |             |
          [100nF]       [GPIO]
            |
           GND
```

A 100nF (0.1uF) ceramic capacitor between the wiper and GND acts as a low-pass filter, reducing high-frequency noise.

---

## HID Axes

If you want a potentiometer to register as a USB HID gamepad axis (for use with simulators other than DCS), set the `hidId` field to an axis identifier. Set it to `-1` to disable HID axis output.

CockpitOS can also send HID axes while in DCS mode if enabled:

```cpp
#define SEND_HID_AXES_IN_DCS_MODE   0   // Set to 1 to send HID axes even when DCS is active
```

---

## Multiple Potentiometers

Each potentiometer uses one ADC-capable GPIO pin. Configure each as a separate row in InputMapping.h:

```cpp
{ "FORMATION_DIMMER", "GPIO", PIN(1), 0, -1, "FORMATION_DIMMER", 65535, "analog", 0 },
{ "POSITION_DIMMER",  "GPIO", PIN(2), 0, -1, "POSITION_DIMMER",  65535, "analog", 0 },
{ "HMD_OFF_BRT",      "GPIO", PIN(18), 0, -1, "HMD_OFF_BRT",     65535, "analog", 0 },
```

Each pot needs its own ADC pin -- there is no way to share a single ADC pin between multiple pots.

---

## Reversing Direction

If your potentiometer reads backwards (turning clockwise decreases the value when it should increase), simply swap the 3.3V and GND connections on the outer terminals of the pot. No firmware changes needed.

---

## Label Creator Configuration

1. Open Label Creator and select your Label Set
2. Click **Edit Inputs**
3. Find the analog control (look for entries with `"analog"` control type)
4. Set **Source** = `GPIO`
5. Set **Port** = your ADC-capable GPIO pin number
6. Verify **Value** = `65535` and **Type** = `analog`
7. Save

---

## Quick Reference

| Parameter                  | Default | Purpose                              |
|----------------------------|---------|--------------------------------------|
| `AX_DEFAULT_MIN`           | 768     | Starting minimum boundary            |
| `AX_DEFAULT_MAX`           | 3327    | Starting maximum boundary            |
| `CENTER_DEADZONE_INNER`    | 256     | Dead zone entry threshold            |
| `CENTER_DEADZONE_OUTER`    | 384     | Dead zone exit threshold             |
| `MIDDLE_AXIS_THRESHOLD`    | 64      | Center snap sensitivity              |
| `UPPER_AXIS_THRESHOLD`     | 128     | Max snap sensitivity                 |
| `LOWER_AXIS_THRESHOLD`     | 256     | Min snap sensitivity                 |
| `SKIP_ANALOG_FILTERING`    | 0       | 1 = disable smoothing filter         |
| `SEND_HID_AXES_IN_DCS_MODE`| 0      | 1 = send HID axes while in DCS mode  |

---

*See also: [Hardware Overview](README.md) | [Encoders](Encoders.md) | [Buttons and Switches](Buttons-Switches.md)*
