# DCS-BIOS Feature Gap Analysis for CockpitOS

> **Purpose:** This document catalogs every feature present in the DCS-BIOS Arduino library
> that CockpitOS does not currently implement. Each entry includes a full explanation of what
> the feature does, the hardware it targets, concrete use cases from real DCS aircraft, and
> notes on how CockpitOS could eventually adopt it.
>
> **How to use this document:** When you encounter a hardware wiring challenge or an input
> pattern that CockpitOS doesn't cleanly support today, check this list. The solution may
> already be documented here as a future enhancement.
>
> **Date:** February 2026
> **Source library:** DCS-BIOS Arduino Library (located in Arduino/libraries/DCS-BIOS)

---

## Table of Contents

1. [Accelerated Rotary Encoder](#1-accelerated-rotary-encoder)
2. [Emulated Concentric Rotary Encoder](#2-emulated-concentric-rotary-encoder)
3. [Rotary Switch (Encoder-as-Position)](#3-rotary-switch-encoder-as-position)
4. [Toggle Button (Alternating Dual-Command)](#4-toggle-button-alternating-dual-command)
5. [Dual-Mode Button (Multi-Seat Routing)](#5-dual-mode-button-multi-seat-routing)
6. [Syncing 3-Position Switch (Continuous Bidirectional Sync)](#6-syncing-3-position-switch-continuous-bidirectional-sync)
7. [Syncing Potentiometer (Delta Feedback Loop)](#7-syncing-potentiometer-delta-feedback-loop)
8. [BCD Thumbwheel Input](#8-bcd-thumbwheel-input)
9. [Radio Preset Wheel](#9-radio-preset-wheel)
10. [Analog Multi-Position (Resistor Ladder)](#10-analog-multi-position-resistor-ladder)
11. [SetState Input (Absolute Value Commands)](#11-setstate-input-absolute-value-commands)
12. [Dynamic Control Remapping](#12-dynamic-control-remapping)
13. [Matrix-Scanned Input Variants](#13-matrix-scanned-input-variants)
14. [Switch-With-Cover (Stateful Two-Step)](#14-switch-with-cover-stateful-two-step)
15. [Summary: Priority and Difficulty Matrix](#15-summary-priority-and-difficulty-matrix)

---

## 1. Accelerated Rotary Encoder

### DCS-BIOS Class
`RotaryAcceleratedEncoder` / `RotaryAcceleratedEncoderT`
(`src/internal/Encoders.h`)

### What It Does
A rotary encoder that tracks rotational **momentum** and sends different DCS-BIOS commands
depending on whether the user is turning slowly or quickly. When the encoder is turned at a
normal pace, it sends the standard increment/decrement arguments. When the user spins it
rapidly, it switches to a separate pair of "fast" arguments that command larger jumps in the
sim.

### How It Works Internally
- Maintains a `momentum` counter (0 to 4).
- Each encoder tick within 175ms of the last tick increases momentum by 1 (capped at 4).
- If more than 500ms elapses without a tick, momentum resets to 0.
- When momentum >= 2, the encoder sends `fastIncArg` / `fastDecArg` instead of the normal
  `incArg` / `decArg`.

### Constructor
```cpp
RotaryAcceleratedEncoder(msg, decArg, incArg, fastDecArg, fastIncArg, pinA, pinB)
```

### Real-World Use Cases

**HSI Heading Bug Knob (A-10C, F/A-18C, F-16C)**
The heading bug on the HSI needs to traverse 0-360 degrees. At 1-degree steps, it takes
360 clicks to go full circle. With acceleration, slow turning adjusts by 1 degree per click,
while fast spinning jumps by 10 degrees per click. The DCS-BIOS control typically exposes
both `INC`/`DEC` and `INC_FAST`/`DEC_FAST` arguments for exactly this purpose.

**Radio Frequency Tuning (UHF/VHF knobs)**
UHF radios span 225.000-399.975 MHz. Fine tuning at 0.025 MHz per click is correct for
precise frequency setting, but traversing the full range requires thousands of clicks.
Acceleration lets the user spin quickly to get close, then slow down for fine adjustment.

**Altimeter Setting (Kollsman Window)**
The barometric pressure setting (29.92 inHg typical) needs fine adjustment but also
occasionally needs large changes when flying between high/low pressure areas.

**Course Selector Knobs**
Any 0-360 degree selector benefits from acceleration: TACAN course, ILS course, etc.

### What CockpitOS Has Today
CockpitOS encoders (`InputControl.cpp`) use a quadrature transition table and accumulate
ticks, but they always send the same DCS-BIOS command regardless of turning speed. There is
no momentum tracking and no fast/slow command differentiation.

### Implementation Notes for CockpitOS
- Could be implemented as a new encoder mode in `InputControl.cpp` or as a label-set
  configuration option (e.g., `FAST_INC` / `FAST_DEC` fields in InputMapping).
- The momentum algorithm is simple: track `lastTickTime`, maintain a counter, threshold
  at ~175ms between ticks for "fast" detection, reset after ~500ms idle.
- Requires the label mapping to carry two extra fields (fast inc/dec command strings).
- Alternatively, could be a multiplier on the existing INC/DEC — send the command N times
  based on momentum — but this is less clean than using DCS-BIOS's native fast arguments.

---

## 2. Emulated Concentric Rotary Encoder

### DCS-BIOS Class
`EmulatedConcentricRotaryEncoder` / `EmulatedConcentricRotaryEncoderT`
(`src/internal/Encoders.h`)

### What It Does
Uses a **single physical rotary encoder with a push button** to emulate two separate
concentric rotary controls. Pressing the button toggles which of the two controls the
encoder adjusts. An LED or other indicator can show which mode is active.

### Constructor
```cpp
EmulatedConcentricRotaryEncoder(msg1, dec1, inc1, msg2, dec2, inc2, pinA, pinB, buttonPin)
```

### Real-World Use Cases

**ILS Frequency/Volume (A-10C)**
Real aircraft have concentric knobs where the outer ring sets ILS frequency and the inner
knob sets audio volume. Physical concentric encoders are expensive and fragile. This
feature lets you use a $2 encoder + pushbutton to control both.

**TACAN Channel + Volume**
Same pattern: outer ring for channel selection, inner for volume. Very common across
multiple aircraft.

**UHF Radio: Frequency Dial + Volume**
Nearly every DCS aircraft has radio panels where frequency and volume share a concentric
knob pair.

**Autopilot Altitude/Heading Select**
Some panels use concentric knobs for altitude (outer) and heading (inner) on the autopilot
panel.

### What CockpitOS Has Today
No equivalent. Each CockpitOS encoder maps to exactly one DCS-BIOS control. There is no
push-to-toggle-mode mechanism.

### Implementation Notes for CockpitOS
- Could be implemented as an InputMapping configuration: mark an encoder as "concentric
  emulated" and specify a button pin that toggles between two control targets.
- The encoder logic itself doesn't change — only which DCS command string gets sent.
- State tracking is trivial: a boolean toggled on button press.
- Consider adding an LED output option to indicate which mode is active.
- Could extend to more than 2 modes (cycle through N controls) for panels with limited
  space.

---

## 3. Rotary Switch (Encoder-as-Position)

### DCS-BIOS Class
`RotarySwitch` / `RotarySwitchT`
(`src/internal/RotarySwitch.h`)

### What It Does
Turns a rotary encoder into a **virtual multi-position switch**. The encoder maintains an
internal position counter from 0 to `maxValue`. Each clockwise click increments the
position, each counterclockwise click decrements it. The absolute position value is sent
to DCS-BIOS (not INC/DEC commands).

### Constructor
```cpp
RotarySwitch(msg, pinA, pinB, maxValue)
```

### Real-World Use Cases

**Master Mode Selector (F/A-18C)**
The master mode switch (NAV/A-A/A-G) is a 3-position rotary in the real aircraft. If you
use a rotary encoder instead of a 3-position physical switch, RotarySwitch tracks position
0/1/2 and sends the absolute value.

**Fuel Selector (Multiple aircraft)**
Fuel crossfeed or tank selector switches with 4-6 positions. Using a rotary encoder saves
panel space and wiring compared to running N wires for a real multi-position switch.

**IFF Mode Selector**
Mode selectors on IFF panels (OFF/STBY/LOW/NORM/EMERGENCY) — 5 positions that can be
mapped to an encoder with maxValue=4.

**Display Brightness Knobs (Stepped)**
Some cockpit brightness controls have discrete positions (OFF/DIM/MED/BRT) rather than
continuous rotation. An encoder with maxValue=3 maps perfectly.

### What CockpitOS Has Today
CockpitOS encoders always send INC/DEC. There is no internal position tracking with bounds
clamping, and no absolute position value transmission. CockpitOS *does* have multi-position
selector support, but only via physical multi-position switches (one-hot or bit-encoded),
not via encoder emulation.

### Implementation Notes for CockpitOS
- Add a new encoder behavior mode: "positional" with a `maxValue` parameter.
- Track position internally, clamp to [0, maxValue].
- On change, send the position value as the DCS-BIOS argument.
- Could be configured in InputMapping with a flag like `ENCODER_POSITIONAL=5` (max 5
  positions).
- Consider: should position persist across power cycles (NVS)? Probably not — DCS will
  re-sync on mission start.

---

## 4. Toggle Button (Alternating Dual-Command)

### DCS-BIOS Class
`ToggleButton` / `ToggleButtonT`
(`src/internal/Buttons.h`)

### What It Does
A momentary button that **alternates between two different DCS-BIOS arguments** on each
press. First press sends `argA`, second press sends `argB`, third sends `argA` again, etc.

### Constructor
```cpp
ToggleButton(msg, argA, argB, pin)
```

### Real-World Use Cases

**Lights Toggle (Various aircraft)**
A single pushbutton that sends "1" on first press (lights on) and "0" on second press
(lights off). Different from a physical toggle switch — this is a momentary button acting
as a toggle.

**Master Arm Toggle**
Some panels use a momentary pushbutton with an indicator LED instead of a physical toggle
switch. Press once = arm, press again = safe.

**Autopilot Engage/Disengage**
A single button that alternates between AP_ON and AP_OFF commands.

**Any Momentary-to-Latching Conversion**
When you have a momentary pushbutton but the DCS control expects explicit on/off states
rather than a TOGGLE command.

### What CockpitOS Has Today
CockpitOS has toggle buttons in HID mode (latched state with rising-edge detection in
HIDManager), but in DCS-BIOS mode, buttons send a single fixed command. There is no
alternating dual-argument behavior. CockpitOS sends `TOGGLE` commands which rely on DCS
itself to track state — this works differently from explicitly alternating between two
arguments.

### Implementation Notes for CockpitOS
- Simple to implement: add an internal boolean that flips on each press, selecting which
  of two command strings to send.
- Could be a label mapping option: `TOGGLE_A=1` / `TOGGLE_B=0` alongside the existing
  command string.
- Edge case: state can desync if a press is missed. Consider optional sync via DCS
  feedback (reading the control's current state to align the toggle).

---

## 5. Dual-Mode Button (Multi-Seat Routing)

### DCS-BIOS Class
`DualModeButton` / `DualModeButtonT`
(`src/internal/DualModeButton.h`)

### What It Does
A button whose DCS-BIOS command changes based on the state of a **mode selection pin**.
When the mode pin is LOW, the button sends `msg1`. When HIGH, it sends `msg2`. This
allows a single physical panel to control different seats/stations in multi-crew aircraft.

### Constructor
```cpp
DualModeButton(modePin, buttonPin, msg1, msg2)
```

### Real-World Use Cases

**AH-64D Apache Front/Back Seat**
The Apache has a pilot (back seat) and CPG (front seat). Many controls exist in both
seats with different DCS-BIOS addresses. A single physical panel with a "SEAT SELECT"
toggle switch on the mode pin can control either seat's functions.

Example: The same physical button could be:
- Mode LOW: `PLT_CMWS_ARM` (pilot's CMWS arm)
- Mode HIGH: `CPG_CMWS_ARM` (CPG's CMWS arm)

**F-14 Pilot/RIO Panels**
The Tomcat's pilot and RIO have overlapping controls. A seat selector toggle lets one
panel serve both roles.

**Mi-24 Pilot/Operator**
Same dual-seat pattern as the Apache.

**Any Multi-Role Panel**
Even in single-seat aircraft, this pattern could route buttons to different aircraft
entirely — a "profile select" switch changing what aircraft the panel controls.

### What CockpitOS Has Today
CockpitOS has no mode-pin-based control routing. Label sets are compile-time configurations.
There is no runtime mechanism to redirect a button's DCS command based on an external
GPIO state.

### Implementation Notes for CockpitOS
- Could be implemented as a special label mapping attribute: each input gets an optional
  `ALT_CMD` and a shared `MODE_PIN` reference.
- Alternatively, could support two complete label sets loaded simultaneously with a mode
  pin selecting between them at runtime — more flexible but more complex.
- A simpler initial approach: a global mode variable toggled by a designated GPIO, checked
  in `DCSBIOSBridge` before command dispatch.
- Consider extending to N modes (not just 2) for aircraft with more than 2 seats or for
  profile switching across different aircraft.

---

## 6. Syncing 3-Position Switch (Continuous Bidirectional Sync)

### DCS-BIOS Class
`SyncingSwitch3Pos` / `SyncingSwitch3PosT`
(`src/internal/SyncingSwitches.h`)

### What It Does
A 3-position switch that **continuously monitors the DCS-BIOS state** of the control it
maps to. If the in-sim position doesn't match the physical switch position, it
automatically re-sends the correct command to force synchronization. This inherits from
both `Switch3Pos` (input) and `Int16Buffer` (output listener).

### Constructor
```cpp
SyncingSwitch3Pos(msg, pinA, pinB, syncAddress, syncMask, syncShift, debounceDelay)
```

### How It Works
1. Reads physical switch position from `pinA`/`pinB` (standard 3-pos logic).
2. Simultaneously receives DCS export data at `syncAddress` via `Int16Buffer`.
3. On each poll, compares physical position to DCS-reported position.
4. If they differ, re-sends the physical position as a DCS command.
5. Includes a settling delay to avoid command storms.

### Real-World Use Cases

**Gear Handle (A-10C, F/A-18C)**
After aircraft repair in multiplayer, DCS may reset the gear handle position. The physical
switch is still in the DOWN position, but DCS thinks it's UP. Without syncing, the gear
stays mismatched until the player manually toggles the switch. With syncing, the physical
DOWN position automatically re-commands gear down.

**Engine Start Switch (F-16C)**
The engine start sequence involves a spring-loaded 3-position switch. If DCS resets
(mission restart, aircraft repair), the physical switch position should immediately
re-assert itself.

**Anti-Ice Toggle**
Any 3-position switch where DCS state can drift from physical state due to:
- Mission restart
- Aircraft damage/repair
- Multiplayer sync issues
- Lua script modifications

**Canopy Open/Close**
The canopy switch may be overridden by other sim events. Continuous sync ensures the
physical switch always wins.

### What CockpitOS Has Today
CockpitOS has **mission-start synchronization** — when a new mission is detected, all
inputs are re-read and sent to DCS. It also has `MAX_VALIDATED_SELECTORS` for cross-checking
selectors. However, this is a **one-time sync on mission start**, not continuous
bidirectional monitoring during gameplay. If DCS state drifts mid-mission (repair, Lua
script, multiplayer), CockpitOS won't catch it.

### Implementation Notes for CockpitOS
- Requires subscribing to DCS export data for each synced control (address + mask + shift).
- On each DCS data update, compare reported value to physical switch state.
- If mismatch detected, re-send physical state after a short delay (avoid command storms).
- Label mapping would need additional fields: `SYNC_ADDRESS`, `SYNC_MASK`, `SYNC_SHIFT`.
- Consider a configurable sync interval (don't check every frame — once per second is
  probably sufficient).
- Memory cost: each synced switch needs to store its sync address, mask, and shift (6 bytes).
- Could limit to a configurable max (e.g., 16 synced switches) to bound memory use.
- **Important:** This should be opt-in per switch, not global. Most switches don't need
  continuous sync — only critical ones that DCS might override.

---

## 7. Syncing Potentiometer (Delta Feedback Loop)

### DCS-BIOS Class
`RotarySyncingPotentiometer` / `RotarySyncingPotentiometerEWMA`
(`src/internal/RotarySyncingPotentiometer.h`)

### What It Does
A potentiometer that **reads the DCS-reported value** of the control it maps to and sends
**delta corrections** if the physical position doesn't match. Instead of sending an absolute
value, it calls a user-provided callback function that calculates the required adjustment
based on the difference between physical and virtual positions.

### Constructor
```cpp
RotarySyncingPotentiometer(msg, pin, syncAddress, syncMask, syncShift, mapperCallback)
```

The `mapperCallback` signature:
```cpp
int mapperCallback(int physicalValue_0to65535, int dcsValue_0to65535)
// Returns: delta to send (positive = increase, negative = decrease, 0 = in sync)
```

### How It Works
1. Reads physical pot position (0-65535 after ADC scaling).
2. Receives DCS-reported value via `Int16Buffer` at syncAddress.
3. Calls `mapperCallback(physical, dcs)` to get the required delta.
4. If delta != 0, sends the delta value as a DCS-BIOS command.
5. Throttles sends to 100ms minimum interval.

### Real-World Use Cases

**Throttle/Fuel Lever (Various aircraft)**
Physical throttle levers may not start in the same position as the in-sim throttle on
mission load. Without sync, the sim throttle jumps to the physical position on first
movement, causing an abrupt engine response. With syncing, the potentiometer only sends
adjustments when the physical and virtual positions differ, allowing smooth catch-up.

**Trim Wheels**
Trim controls that should match physical trim wheel position. If DCS resets trim (repair,
respawn), the pot detects the mismatch and sends corrections.

**Panel Lighting Dimmers**
Brightness knobs where the initial DCS state may differ from physical knob position.

**Mixture Control**
Fuel mixture levers on warbirds (P-51, Spitfire) where position precision matters.

### What CockpitOS Has Today
CockpitOS potentiometers send absolute values with EMA filtering, deadzone, and calibration.
They do **not** read DCS feedback or calculate deltas. They are fire-and-forget: the
physical position is scaled and sent, with no knowledge of what DCS currently reports.

### Implementation Notes for CockpitOS
- Requires subscribing to DCS export data for the pot's control address.
- The mapper callback pattern is elegant but may be complex for label-set auto-generation.
  Consider a built-in default mapper that calculates a proportional delta.
- Throttle at 100ms between sends to avoid command storms.
- The most practical first implementation: detect mismatch > threshold, send the physical
  value as absolute. Only fall back to delta mode if absolute doesn't work for specific
  DCS controls.
- Memory cost per synced pot: sync address (2B) + mask (2B) + shift (1B) + last DCS
  value (2B) = ~7 bytes.

---

## 8. BCD Thumbwheel Input

### DCS-BIOS Class
`BcdWheel` / `BcdWheelT`
(`src/internal/BcdWheels.h`)

### What It Does
Reads a **Binary Coded Decimal thumbwheel** (the mechanical rotating digit wheels found
on IFF panels, altitude alerters, and radio preset selectors). These wheels have 2-4
output pins that encode the currently displayed digit in binary.

### Pin Configurations
- **2-pin mode:** Values 0-3
- **3-pin mode:** Values 0-7
- **4-pin mode:** Values 0-15 (standard BCD: 0-9 for single decimal digit)

### Constructor
```cpp
BcdWheel(msg, pinA, pinB, pinC=PIN_NC, pinD=PIN_NC)
```

### How It Works
1. Each pin is pulled up (INPUT_PULLUP).
2. A LOW pin means that bit is set.
3. The binary value across all pins is computed: `(pinD << 3) | (pinC << 2) | (pinB << 1) | pinA`.
4. The value is sent as a character string to DCS-BIOS.
5. Debounced with polling interval.

### Real-World Use Cases

**IFF Code Entry (A-10C, F/A-18C, F-16C)**
IFF (Identification Friend or Foe) panels use mechanical thumbwheels to set Mode 1,
Mode 3/A codes. These are physical BCD wheels where each digit position has 4 pins
encoding 0-7 (octal) or 0-9 (decimal). Real IFF panels have 4 such wheels for a 4-digit
code.

**Altitude Alerter (Various)**
Altitude warning systems use thumbwheels to set target altitude digits. Each wheel
represents one digit of the altitude (e.g., 25,000 ft = wheels showing 2-5-0-0-0).

**TACAN Channel Entry (Older aircraft)**
Some TACAN panels use BCD thumbwheels instead of rotary encoders for channel selection.

**Weapons Release Interval**
Bombing computers on aircraft like the A-10C use thumbwheels for release interval and
quantity settings.

### What CockpitOS Has Today
No BCD decoding. CockpitOS reads individual GPIO pins as buttons or switch positions but
has no concept of combining multiple pins into a single binary-encoded value. Selectors
use one-hot (one pin per position) or grouped bit encoding, but not true BCD.

### Implementation Notes for CockpitOS
- Straightforward to implement: read 2-4 pins, compute binary value, send as command.
- Could be a new input type in the label mapping: `TYPE=BCD` with pin assignments.
- Consider both GPIO and HC165/PCA9555 pin sources for BCD inputs.
- Real BCD wheels often have a "common" pin that should be grounded; the 2-4 data pins
  go HIGH or LOW for each bit.
- The DCS-BIOS library sends the value as a character (ASCII digit). CockpitOS should
  match this format for compatibility.
- Could also support Gray code wheels (common in military hardware) by adding a Gray-to-
  binary conversion step.

---

## 9. Radio Preset Wheel

### DCS-BIOS Class
`RadioPreset` / `RadioPresetT`
(`src/internal/BcdWheels.h`)

### What It Does
A specialized 5-pin BCD input for **radio preset channel selection wheels**. Reads 5 pins
to determine a position (0-31) and maps it to a float value (0.0 to 1.0 in steps of
1/20 = 0.05).

### Constructor
```cpp
RadioPreset(msg, pinA, pinB, pinC, pinD, pinE)
```

### How It Works
1. Reads 5 pins as a 5-bit binary value (0-31).
2. Multiplies by 3200 and adds 200 to get a uint16 value.
3. Sends as a numeric string to DCS-BIOS.
4. DCS interprets this as a normalized 0.0-1.0 float mapped to radio preset positions.

### Real-World Use Cases

**UHF Radio Preset Selector (A-10C, F-16C)**
The AN/ARC-164 UHF radio has a 20-position preset channel selector. Physical replicas
use a BCD-encoded thumbwheel or rotary with 5 output pins.

**VHF Radio Presets**
Similar preset selectors on VHF radios across multiple aircraft.

### What CockpitOS Has Today
No equivalent. This is a superset of the BCD wheel with specific float-mapping logic.

### Implementation Notes for CockpitOS
- If BCD wheel support is implemented first, RadioPreset is just a BCD wheel with a
  custom output mapping formula.
- Could be generalized: "BCD wheel with configurable output scaling" rather than a
  separate class.

---

## 10. Analog Multi-Position (Resistor Ladder)

### DCS-BIOS Class
`AnalogMultiPos` / `AnalogMultiPosT`
(`src/internal/AnalogMultiPos.h`)

### What It Does
Reads a **single analog pin** connected to a resistor ladder network and determines which
of N positions a rotary switch is in. The voltage divider creates distinct voltage levels
for each position, and the code divides the ADC range into N equal bands to determine the
current position.

### Constructor
```cpp
AnalogMultiPos(msg, pin, numOfSteps)
```

### How It Works
1. Reads the ADC value on the analog pin.
2. Divides the full ADC range (0-1023 on AVR, 0-4095 on ESP32) into `numOfSteps` equal
   bands.
3. Determines which band the current reading falls into.
4. If the band differs from the last sent value, sends the new position to DCS-BIOS.
5. Polls at 750ms intervals to debounce noisy ladder networks.

### Real-World Use Cases

**Multi-Position Rotary Switches on Cheap Panels**
Many off-the-shelf cockpit panels (like those from Winwing, Virpil, or DIY builds) wire
multi-position rotary switches as resistor ladders to save GPIO pins. A 12-position
switch that would normally need 12 GPIO pins (one-hot) only needs 1 analog pin with a
resistor ladder.

**Pin-Constrained Builds**
When building large panels on Arduino Nano or other pin-limited boards, resistor ladders
let you fit more switches on fewer pins. A single analog pin replaces 6-12 digital pins.

**Commercial Rotary Switches**
Some commercial rotary switches come pre-wired with internal resistor ladders and a single
analog output.

**Ganged Switch Panels**
Radio panels with multiple rotary switches can use one analog pin per switch instead of
a full one-hot wiring harness.

### What CockpitOS Has Today
CockpitOS analog inputs are treated exclusively as **continuous axes** (potentiometers,
joysticks). The analog pipeline applies EMA filtering, calibration, deadzone, and scales
to a continuous 0-4095 range. There is no discrete position detection from analog values.

CockpitOS does support multi-position switches, but only via:
- One-hot GPIO encoding (one pin per position)
- HC165 shift register chains
- PCA9555 I2C expander pins
- Matrix rotary strobe/data patterns

None of these use an analog pin with a resistor ladder.

### Implementation Notes for CockpitOS
- Straightforward to implement: read ADC, divide into N bands, detect position changes.
- On ESP32, ADC is 12-bit (0-4095), giving finer resolution than AVR's 10-bit (0-1023).
  This means better discrimination between ladder positions — can comfortably support
  16+ positions on a single pin.
- Should add hysteresis between bands to prevent flickering at band boundaries.
  DCS-BIOS doesn't do this (it uses a slow 750ms poll instead), but CockpitOS could do
  better with proper hysteresis.
- Could be configured in InputMapping: `TYPE=ANALOG_MULTI` with `STEPS=N`.
- Consider supporting non-uniform resistor values (custom band thresholds) for ladders
  that don't use equal-value resistors.
- Memory cost: minimal (1 byte for last position, 2 bytes for pin, 1 byte for step count).

---

## 11. SetState Input (Absolute Value Commands)

### DCS-BIOS Class
`SetStateInput` (used via direct `sendDcsBiosMessage` with numeric values)

### What It Does
Allows sending an **exact absolute numeric value** to a DCS-BIOS control, rather than
relative INC/DEC or TOGGLE commands. The value is sent as a string representation of an
integer (e.g., "32768" for mid-range on a 0-65535 control).

### Example Usage
```cpp
sendDcsBiosMessage("THROTTLE_SET", "32768");  // Set throttle to 50%
sendDcsBiosMessage("FLAP_LEVER", "0");        // Flaps fully up
```

### Real-World Use Cases

**Direct Axis-to-Control Mapping**
When a physical potentiometer should directly set a DCS control to an absolute value
rather than incrementing/decrementing. Useful when you know the exact value the control
should be at.

**Preset Recall**
Setting a control to a known-good value (e.g., radio frequency to a specific preset value,
altimeter to standard pressure 29.92).

**Automated Startup Sequences**
If implementing auto-start macros, you need to set specific controls to specific values
rather than toggling them.

**Control Reset**
Resetting a control to its default position (e.g., trim to center = 32768).

### What CockpitOS Has Today
CockpitOS sends commands as `label INC`, `label DEC`, `label TOGGLE`, or `label <value>`
for potentiometers. The potentiometer pathway does send absolute values, but there's no
general-purpose mechanism to send an arbitrary absolute value to any control from a
button press or other discrete input.

### Implementation Notes for CockpitOS
- Could be added as a command type in the label mapping: `CMD=SET_STATE` with `VALUE=N`.
- Useful for macro buttons ("Press this button to set altimeter to standard").
- Low implementation effort: just format and send the value string.
- Could combine with button inputs: "on press, send SET_STATE 65535; on release, send
  SET_STATE 0" for momentary-to-axis emulation.

---

## 12. Dynamic Control Remapping

### DCS-BIOS Class
`SetControl()` method available on most input classes

### What It Does
Allows **changing which DCS-BIOS control an input targets at runtime**. After calling
`SetControl("NEW_CONTROL_MSG")`, the same physical button/encoder/switch sends commands
to a completely different DCS control.

### Example Usage
```cpp
myButton.SetControl("F16_MASTER_ARM");   // Initially maps to F-16 master arm
// Later...
myButton.SetControl("A10_MASTER_ARM");   // Now maps to A-10 master arm
```

### Real-World Use Cases

**Multi-Aircraft Panels**
A generic button box that changes function based on which aircraft is loaded. A "profile
select" switch or software command reconfigures all inputs for the current aircraft.

**Mode-Dependent Controls**
Controls that change function based on cockpit mode (e.g., in A/A mode the TDC controls
radar cursor; in A/G mode it controls FLIR cursor — different DCS controls).

**Shared Panels in Multi-Monitor Setups**
A physical panel shared between multiple DCS instances (instructor/student stations) that
needs to switch control targets.

### What CockpitOS Has Today
CockpitOS label sets are **compile-time configurations**. The active label set is selected
at compile time via `active_set.h`. There is no runtime mechanism to change which DCS
command a physical input sends. Switching aircraft requires recompiling and reflashing.

### Implementation Notes for CockpitOS
- Full dynamic remapping is a significant architectural change — label mappings are
  currently stored as compile-time string constants.
- A lighter approach: support 2-3 pre-compiled "profiles" that can be switched via a GPIO
  or DCS-BIOS command. Essentially multiple label sets in flash, with a runtime selector.
- The heaviest approach: store label mappings in SPIFFS/LittleFS and load them dynamically.
  This enables OTA profile switching but adds significant complexity.
- Consider whether this is really needed: CockpitOS is designed for dedicated panels that
  rarely change aircraft. Generic button boxes are a different product category.
- If implemented, the mode selection could come from: a physical GPIO toggle, a DCS-BIOS
  command (aircraft name detection), or a WiFi/BLE configuration command.

---

## 13. Matrix-Scanned Input Variants

### DCS-BIOS Classes
- `MatActionButton` / `MatActionButtonT` — matrix momentary button
- `MatActionButtonToggle` / `MatActionButtonToggleT` — matrix button, fires on both edges
- `MatActionButtonSet` / `MatActionButtonSetT` — matrix button with absolute state
- `MatSwitch2Pos` / `MatSwitch2PosT` — matrix 2-position switch
- `MatSwitch3Pos` / `MatSwitch3PosT` — matrix 3-position switch
- `MatRotaryEncoder` / `MatRotaryEncoderT` — matrix rotary encoder
- `Matrix2Pos` / `Matrix2PosT` — SwitchMatrix library 2-pos switch
- `Matrix3Pos` / `Matrix3PosT` — SwitchMatrix library 3-pos switch

(`src/internal/Buttons.h`, `src/internal/Encoders.h`, `src/internal/MatrixSwitches.h`)

### What They Do
These are input classes that read from a **memory array** instead of GPIO pins. An external
matrix scanning library or hardware scans a switch matrix and writes results to a byte
array in memory. The DCS-BIOS Mat* classes read from pointers into that array.

The `Matrix2Pos` / `Matrix3Pos` variants integrate with the SwitchMatrix Arduino library
(github.com/dagoston93/SwitchMatrix) which handles the row/column scanning.

### How They Work
Instead of `digitalRead(pin)`, these classes do:
```cpp
byte value = *addressPointer;  // Read from memory array
```
The external matrix scanner periodically strobes rows, reads columns, and populates the
memory array. The DCS-BIOS classes just consume the results.

### Real-World Use Cases

**Large Button Panels (UFC, CDU)**
An Up Front Controller (UFC) or Control Display Unit (CDU) may have 30-50 buttons. Wiring
each to a GPIO is impractical. A switch matrix (e.g., 8x8 = 64 buttons on 16 pins) is the
standard solution.

**Full Cockpit Builds**
A complete cockpit replica may have 200+ switches and buttons. Matrix scanning is essential
to fit on any microcontroller.

**Commercial Matrix Scanner ICs**
Some commercial scanner ICs (like the 74HC4051 analog mux or dedicated key scanner chips)
output to memory-mapped registers. The Mat* classes interface with these.

### What CockpitOS Has Today
CockpitOS has its own matrix support:
- **HC165 shift register chains** (up to 64 bits) for buttons and selectors
- **PCA9555 I2C expander** (up to 128 I/O across 8 chips) for buttons and selectors
- **TM1637 key scanning** (up to 16 keys per module, 4 modules = 64 keys)
- **Matrix rotary switches** (strobe/data pattern decoding)

These are CockpitOS-native implementations, not compatible with the DCS-BIOS
memory-pointer pattern. CockpitOS doesn't support the SwitchMatrix library or generic
memory-mapped matrix scanning.

### Implementation Notes for CockpitOS
- CockpitOS's existing matrix solutions (HC165, PCA9555, TM1637) are more practical for
  ESP32 builds than the DCS-BIOS memory-pointer approach, which was designed for AVR.
- The SwitchMatrix library could be useful if users have existing hardware wired for
  row/column scanning, but it's AVR-oriented.
- **Recommendation:** Low priority. CockpitOS's existing I/O expansion (HC165 + PCA9555)
  already solves the pin constraint problem more elegantly than raw matrix scanning.
- If needed, could add a generic "external scanner" interface where any scanning library
  writes to a shared byte array, and CockpitOS reads from it.

---

## 14. Switch-With-Cover (Stateful Two-Step)

### DCS-BIOS Class
`SwitchWithCover2Pos` / `SwitchWithCover2PosT`
(`src/internal/Switches.h`)

### What It Does
Models a **covered switch** — a 2-position switch protected by a spring-loaded cover that
must be opened before the switch can be activated. The DCS-BIOS class manages two separate
controls: the cover (open/close) and the switch beneath it, enforcing the correct
sequence.

### Constructor
```cpp
SwitchWithCover2Pos(switchMsg, coverMsg, pin, reverse, debounceDelay)
```

### How It Works
1. The single physical GPIO pin drives a state machine.
2. **Cover closed + Switch safe** = default state.
3. First state change: sends cover OPEN command.
4. Second state change: sends switch ACTIVATE command.
5. On return to default: sends switch DEACTIVATE, then cover CLOSE.
6. The sequence is enforced — you can't activate without opening the cover first.

### Real-World Use Cases

**Launch Bar Abort Switch (F/A-18C)**
The catapult launch bar has a guarded switch — you must lift the cover before you can
abort the launch sequence.

**Ejection Seat Handle Guard**
The ejection seat typically has a safety cover/pin that must be removed before the handle
is active.

**Emergency Jettison**
Stores jettison buttons are often guarded to prevent accidental release.

**Ground Power / External Air**
These switches on some aircraft are guarded to prevent accidental disconnection.

**Engine Fire Extinguisher**
Fire extinguisher handles on multi-engine aircraft are guarded.

### What CockpitOS Has Today
CockpitOS has a **CoverGate system** that intercepts dangerous switch actions. The
`CoverGate_loop` function in `HIDManager.cpp` checks if a cover is open before allowing
the guarded switch to activate. However, the CockpitOS implementation uses **two separate
physical pins** (one for the cover, one for the switch), while DCS-BIOS's version uses
a **single pin with a state machine** to model both the cover and the switch.

The CockpitOS approach is arguably more realistic (two physical switches = two pins), but
the DCS-BIOS single-pin approach is useful when building compact panels where one toggle
switch must send two sequential DCS commands (cover + switch).

### Implementation Notes for CockpitOS
- CockpitOS's existing CoverGate is functionally equivalent for most use cases.
- The single-pin state machine variant could be useful for compact builds or when
  repurposing a single toggle to control a covered switch in DCS.
- Could be added as an InputMapping option: `TYPE=COVERED_SWITCH` with `COVER_CMD` and
  `SWITCH_CMD` fields, using a single GPIO pin.
- The state machine is simple: 4 states, transitions on pin change, with commands sent
  at each transition.

---

## 15. Summary: Priority and Difficulty Matrix

### High Value, Medium Effort

| Feature | Why It Matters | Effort |
|---|---|---|
| [Accelerated Encoder](#1-accelerated-rotary-encoder) | Dramatically improves UX for any 0-360 degree knob. Without it, traversing full range is painfully slow. Nearly every aircraft has at least one heading/course/frequency knob that benefits. | Medium — momentum tracking + dual command routing |
| [Syncing 3-Pos Switch](#6-syncing-3-position-switch-continuous-bidirectional-sync) | Prevents state drift that confuses users (physical switch says DOWN but DCS says UP). Critical for multiplayer and mission restarts. | Medium — requires DCS export subscription per switch |
| [Analog Multi-Position](#10-analog-multi-position-resistor-ladder) | Huge pin savings. One analog pin replaces 6-12 digital pins for a rotary switch. Very common wiring pattern in DIY cockpits. | Easy — ADC read + band detection |

### Medium Value, Low Effort

| Feature | Why It Matters | Effort |
|---|---|---|
| [Emulated Concentric Encoder](#2-emulated-concentric-rotary-encoder) | Space and cost saver. Real concentric encoders are $15-30; a regular encoder + button is $2. Common need on radio panels. | Easy — boolean toggle + command routing |
| [Rotary Switch (Encoder-as-Position)](#3-rotary-switch-encoder-as-position) | Common hardware substitution. Users often want to use encoders instead of hard-to-source multi-position switches. | Easy — counter + bounds clamping |
| [Toggle Button (Alternating)](#4-toggle-button-alternating-dual-command) | Useful when DCS control needs explicit on/off rather than TOGGLE. Niche but easy. | Easy — boolean flip per press |
| [SetState Input](#11-setstate-input-absolute-value-commands) | Enables macro buttons and preset recall. Small feature, useful for power users. | Easy — string formatting |
| [BCD Thumbwheel](#8-bcd-thumbwheel-input) | Required for authentic IFF panel replicas. Niche but impossible to substitute. | Easy — binary pin decode |

### Medium Value, High Effort

| Feature | Why It Matters | Effort |
|---|---|---|
| [Syncing Potentiometer](#7-syncing-potentiometer-delta-feedback-loop) | Prevents throttle/trim jumps on mission restart. Improves UX for axis controls. | High — feedback loop + delta calculation + throttling |
| [Dual-Mode Button](#5-dual-mode-button-multi-seat-routing) | Only matters for multi-seat aircraft builders (Apache, F-14, Mi-24). Small audience but high value for them. | Medium — mode pin + command routing |
| [Dynamic Control Remapping](#12-dynamic-control-remapping) | Enables multi-aircraft panels. Significant architectural change. | High — runtime label set switching |

### Low Priority

| Feature | Why It Matters | Effort |
|---|---|---|
| [Radio Preset Wheel](#9-radio-preset-wheel) | Superset of BCD wheel. Implement BCD first, this follows naturally. | Easy (after BCD) |
| [Matrix-Scanned Inputs](#13-matrix-scanned-input-variants) | CockpitOS already has superior I/O expansion (HC165, PCA9555). DCS-BIOS's approach is AVR-centric. | Low priority — existing solutions are better |
| [Switch-With-Cover (Single-Pin)](#14-switch-with-cover-stateful-two-step) | CockpitOS's CoverGate already handles this with two pins. Single-pin variant is a minor convenience. | Easy but low value |

---

## Appendix: Feature Comparison At A Glance

```
Feature                              DCS-BIOS    CockpitOS    Gap?
-------------------------------------------------------------------
INPUT TYPES
  2-Position Switch                     Yes         Yes         No
  3-Position Switch                     Yes         Yes         No
  N-Position Switch                     Yes         Yes         No
  Momentary Button                      Yes         Yes         No
  Rotary Encoder                        Yes         Yes         No
  Accelerated Encoder                   Yes         No          YES
  Emulated Concentric Encoder           Yes         No          YES
  Encoder-as-Position-Switch            Yes         No          YES
  Toggle Button (alternating)           Yes         No          YES
  Dual-Mode Button (seat select)        Yes         No          YES
  Potentiometer (with filtering)        Yes         Yes         No (CockpitOS better)
  Syncing Potentiometer                 Yes         No          YES
  BCD Thumbwheel                        Yes         No          YES
  Radio Preset Wheel                    Yes         No          YES
  Analog Multi-Position (ladder)        Yes         No          YES
  Matrix Buttons (memory-mapped)        Yes         Partial     Minor
  Switch With Cover (single-pin)        Yes         Different   Minor
  Syncing 3-Pos Switch                  Yes         Partial     YES (continuous)

OUTPUT TYPES
  Discrete LED                          Yes         Yes         No
  PWM Dimmer                            Yes         Yes         No
  Servo Gauge                           Yes         Yes         No
  Integer Buffer (callback)             Yes         Yes         No
  String Buffer (callback)              Yes         Yes         No
  WS2812 RGB LED                        No          Yes         CockpitOS only
  TM1637 7-Segment Display              No          Yes         CockpitOS only
  GN1640 LED Matrix                     No          Yes         CockpitOS only
  TFT Gauge Displays                    No          Yes         CockpitOS only

I/O EXPANSION
  HC165 Shift Register                  No          Yes         CockpitOS only
  PCA9555 I2C Expander                  No          Yes         CockpitOS only
  Matrix Rotary (strobe/data)           No          Yes         CockpitOS only
  SwitchMatrix Library                  Yes         No          Minor gap

TRANSPORTS
  Serial (250kbaud)                     Yes         Yes         No
  RS485 Master/Slave                    Yes         Yes         No (CockpitOS better)
  WiFi (ESP32)                          Yes         Yes         No
  Bluetooth BLE                         No          Yes         CockpitOS only
  USB HID Gamepad                       No          Yes         CockpitOS only

FEATURES
  Mission Sync                          Manual      Automatic   CockpitOS better
  Continuous State Sync                 Yes         No          YES
  Cover-Guarded Switches                Yes         Yes         No
  Dynamic Remapping                     Yes         No          YES
  Performance Profiling                 No          Yes         CockpitOS only
  Self-Learning Calibration             No          Yes         CockpitOS only
  Dual-Mode (HID + DCS)                No          Yes         CockpitOS only
```
