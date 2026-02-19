# Rotary Encoders

Rotary encoders are infinite-rotation input devices that generate directional pulses. Unlike potentiometers (which have a fixed range), encoders can spin endlessly in either direction, making them ideal for volume knobs, tuning dials, channel selectors, and any control that increments or decrements a value.

---

## How Rotary Encoders Work

An encoder has two output channels (A and B) that produce square waves 90 degrees out of phase. The firmware detects which channel leads the other to determine rotation direction.

```
+----------------------------------------------------------------------+
|  QUADRATURE ENCODER SIGNALS                                          |
+----------------------------------------------------------------------+
|                                                                      |
|  CLOCKWISE ROTATION:                                                 |
|                                                                      |
|  Channel A -----+   +---+   +---                                     |
|                 |   |   |   |                                        |
|                 +---+   +---+                                        |
|                                                                      |
|  Channel B ---+   +---+   +---+                                      |
|               |   |   |   |   |                                      |
|               +---+   +---+   +                                      |
|           <--- A leads B                                             |
|                                                                      |
|  COUNTER-CLOCKWISE ROTATION:                                         |
|                                                                      |
|  Channel A ---+   +---+   +---+                                      |
|               |   |   |   |   |                                      |
|               +---+   +---+   +                                      |
|                                                                      |
|  Channel B -----+   +---+   +---                                     |
|                 |   |   |   |                                        |
|                 +---+   +---+                                        |
|           <--- B leads A                                             |
|                                                                      |
+----------------------------------------------------------------------+
```

Most common encoders produce 4 transitions per detent (click). CockpitOS counts these transitions and sends a command after a full detent.

---

## Encoder vs Potentiometer

| Feature       | Potentiometer            | Rotary Encoder           |
|---------------|--------------------------|--------------------------|
| Rotation      | Limited (typically 270 degrees) | Infinite              |
| Output        | Absolute position (voltage) | Relative pulses (direction) |
| DCS use       | Throttle, brightness axes | Volume, tuning, channel select |
| Control type  | `"analog"`               | `"variable_step"` or `"fixed_step"` |
| GPIO pins     | 1 (analog-capable)       | 2 (digital)              |

---

## Wiring

### Basic 3-Pin Encoder

Connect Channel A and Channel B to two GPIO pins, and the common pin to GND. CockpitOS enables internal pull-ups on both channels automatically.

```
                    ESP32
                 +---------+
                 |         |
                 |  GPIO 5 +-------------- A (Channel A)
                 |         |
                 |  GPIO 6 +-------------- B (Channel B)
                 |         |
                 |    GND  +-------------- C (Common / Ground)
                 |         |
                 +---------+
```

### 5-Pin Encoder with Push Button

Many encoders include a built-in push button (5-pin type). The push button is wired as a separate momentary input.

```
                    ESP32
                 +---------+
                 |         |
                 |  GPIO 5 +-------------- A (Channel A)
                 |         |
                 |  GPIO 6 +-------------- B (Channel B)
                 |         |
                 |  GPIO 7 +-------------- SW (Push button)
                 |         |
                 |    GND  +---------+---- C (Encoder common)
                 |         |         +---- SW GND (Button common)
                 +---------+
```

The push button is configured as a separate momentary input in InputMapping.h (see [Buttons and Switches](Buttons-Switches.md)).

---

## fixed_step vs variable_step

DCS-BIOS supports two encoder command types. Choose based on the control you are mapping.

### fixed_step

Sends an INC (increment) or DEC (decrement) command for each detent. Good for stepped controls where each click should advance exactly one position.

**Examples:** TACAN channel selector, radio preset, mode knob

```cpp
//  label              source   port     bit  hidId  DCSCommand    value  Type          group
{ "TACAN_CHAN_DEC",    "GPIO",  PIN(5),   0,   -1,  "TACAN_CHAN",    0,  "fixed_step",    0 },
{ "TACAN_CHAN_INC",    "GPIO",  PIN(6),   0,   -1,  "TACAN_CHAN",    1,  "fixed_step",    0 },
```

### variable_step

Sends a variable-size increment or decrement (default step size is 3200 out of 65535). Rapid rotation accumulates larger values for faster response. Good for smooth controls.

**Examples:** Volume knob, brightness dimmer, heading bug

```cpp
//  label               source   port     bit  hidId  DCSCommand      value  Type            group
{ "HMD_OFF_BRT_DEC",   "GPIO",  PIN(5),   0,   -1,  "HMD_OFF_BRT",     0,  "variable_step",   0 },
{ "HMD_OFF_BRT_INC",   "GPIO",  PIN(6),   0,   -1,  "HMD_OFF_BRT",     1,  "variable_step",   0 },
```

---

## InputMapping.h Configuration

Encoders always require **two rows** -- one for decrement and one for increment.

### Field Mapping

| Field       | DEC Row            | INC Row            | Notes                           |
|-------------|--------------------|--------------------|----------------------------------|
| label       | `"CONTROL_DEC"`    | `"CONTROL_INC"`    | Convention: suffix with _DEC/_INC |
| source      | `"GPIO"`           | `"GPIO"`           | Or `"PCA_0xNN"` for expanders   |
| port        | `PIN(A_pin)`       | `PIN(B_pin)`       | Channel A GPIO, Channel B GPIO   |
| bit         | `0`                | `0`                | Keep as 0 for GPIO encoders      |
| hidId       | `-1`               | `-1`               | HID not typically used           |
| oride_label | `"DCS_COMMAND"`    | `"DCS_COMMAND"`    | Same command for both rows       |
| oride_value | `0`                | `1`                | 0 = DEC, 1 = INC                |
| controlType | `"variable_step"`  | `"variable_step"`  | Or `"fixed_step"`                |
| group       | `0`                | `0`                | Encoders do not use groups       |

**Key rule**: Both rows must share the same DCS-BIOS command name (oride_label). The firmware pairs them automatically by matching labels.

---

## Using PCA9555 for Encoders

Encoders can also be connected through PCA9555 I2C expanders. This works well when you have many encoders and limited GPIO.

```cpp
{ "RADIO_VOL_DEC", "PCA_0x20", 0, 4, -1, "RADIO_VOL", 0, "variable_step", 0 },
{ "RADIO_VOL_INC", "PCA_0x20", 0, 5, -1, "RADIO_VOL", 1, "variable_step", 0 },
```

- `port` = PCA9555 port (0 or 1)
- `bit` = pin number within the port (0-7) for Channel A and Channel B respectively

The PCA9555 scanning rate is fast enough for typical encoder use. For very high-speed encoders, direct GPIO is preferred.

---

## How CockpitOS Processes Encoder Pulses

The firmware uses a state transition table to decode quadrature signals reliably:

1. Both channels are read at the polling rate (250Hz default)
2. The previous state (A,B) and current state form a 4-bit index
3. The transition table returns -1 (CCW), 0 (no change), or +1 (CW)
4. Counts accumulate until a full detent threshold is reached (typically 4 transitions)
5. A DCS-BIOS command is sent for each complete detent

This approach provides reliable, bounce-free decoding without external hardware debouncing components.

### Encoder Sensitivity

By default, CockpitOS requires 4 transitions per detent. This is defined by:

```cpp
#define ENCODER_TICKS_PER_NOTCH  4
```

If your encoder has a different PPR (pulses per revolution), you may need to adjust this value in the firmware source.

---

## Label Creator Configuration

1. Open Label Creator and select your Label Set
2. Click **Edit Inputs**
3. Find the encoder control (look for _DEC and _INC suffixes)
4. For the DEC row: Set **Source** = `GPIO`, **Port** = Channel A pin
5. For the INC row: Set **Source** = `GPIO`, **Port** = Channel B pin
6. Verify both rows have the same DCS command and correct control type
7. Save

---

## Troubleshooting

### Encoder Skips Steps or Moves Wrong Direction

1. **Swap A and B pins** -- if direction is reversed, swap the two GPIO assignments
2. **Check wiring** -- verify both channels and the common are connected correctly
3. **Verify both rows** share the same oride_label (DCS command)
4. **Check controlType** -- must be `"fixed_step"` or `"variable_step"`, not `"momentary"` or `"selector"`
5. **Noisy encoder** -- cheap encoders can produce erratic signals. Solutions:
   - Add 10nF (0.01uF) capacitors between each channel pin and GND
   - Add 10k ohm external pull-up resistors if internal pull-ups are not strong enough
   - Try a higher-quality encoder

### No Response at All

1. Verify the common pin is connected to GND
2. Check that the GPIO pins are not on the "avoid" list for your board variant
3. Enable `DEBUG_ENABLED` in Config.h and check Serial output for input activity

---

*See also: [Hardware Overview](README.md) | [Analog Inputs](Analog-Inputs.md) | [Buttons and Switches](Buttons-Switches.md)*
