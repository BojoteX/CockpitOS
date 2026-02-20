# How To: Wire Solenoid Switches

Solenoid-actuated switches use small electromagnets to physically move toggle switches so they match the in-game position. When DCS says a switch is ON, the solenoid fires and pushes/pulls the toggle to that position -- giving you real force-feedback on your panel.

---

## What Solenoid Switches Are

A solenoid is a coil of wire around a metal plunger. When you apply voltage, the plunger moves. By mounting a solenoid behind a toggle switch, you can make the physical switch snap to match the simulator state.

**Use case:** You have a physical toggle switch panel, and you want the toggles to automatically move when the in-game state changes -- for example, when you load a saved mission where switches are in different positions, or when the instructor station flips a switch remotely.

---

## What You Need

| Item | Notes |
|---|---|
| Small pull or push solenoid | 5V or 12V, with enough stroke to move a toggle switch lever |
| N-channel MOSFET | Logic-level type (e.g., IRLZ44N, 2N7000). Must trigger at 3.3V gate. |
| Flyback diode | 1N4007 or 1N4148. Protects the MOSFET from back-EMF. |
| Gate resistor | 220-1K ohm, between ESP32 GPIO and MOSFET gate |
| Pull-down resistor | 10K ohm, between MOSFET gate and GND (prevents floating during boot) |
| External power supply | 5V or 12V depending on solenoid voltage rating |
| Toggle switch | The physical switch you want to actuate |

---

## Step 1: Understand the Circuit

The ESP32 GPIO cannot drive a solenoid directly -- solenoids draw far more current than a GPIO pin can source. Instead, the GPIO controls a MOSFET, which switches the solenoid power.

```
+----------------------------------------------------------------------+
|  SOLENOID DRIVER CIRCUIT                                              |
+----------------------------------------------------------------------+
|                                                                      |
|                          +------ V+ (5V or 12V supply)               |
|                          |                                           |
|                    +-----+-----+                                     |
|                    |  Flyback  |                                     |
|                    |  Diode    |  (cathode toward V+)                |
|                    |  1N4007   |                                     |
|                    +-----+-----+                                     |
|                          |                                           |
|                    +-----+-----+                                     |
|                    | Solenoid  |                                     |
|                    |  Coil     |                                     |
|                    +-----+-----+                                     |
|                          |                                           |
|                          +---- MOSFET Drain                          |
|                          |                                           |
|    ESP32 GPIO ---[220R]--+---- MOSFET Gate                           |
|                          |                                           |
|                  [10K]   +---- MOSFET Source                         |
|                    |                |                                 |
|                   GND              GND                               |
|                                                                      |
+----------------------------------------------------------------------+
```

### How It Works

1. ESP32 GPIO goes HIGH (3.3V)
2. MOSFET gate activates, connecting drain to source
3. Current flows through the solenoid coil
4. Solenoid plunger moves, actuating the toggle switch
5. ESP32 GPIO goes LOW
6. MOSFET turns off, solenoid releases
7. Flyback diode absorbs the voltage spike from the collapsing magnetic field

---

## Step 2: Install the Flyback Diode

**This is critical.** When a solenoid coil is de-energized, the collapsing magnetic field generates a voltage spike (back-EMF) that can destroy the MOSFET or damage the ESP32. The flyback diode clamps this spike.

- Connect the diode **across the solenoid coil** (in parallel)
- The diode's **cathode** (stripe end) goes to the positive terminal of the solenoid
- The diode's **anode** goes to the drain side (negative terminal)
- Use a 1N4007 (for 12V solenoids) or 1N4148 (for 5V solenoids)

**Do not skip this step.** Without the flyback diode, you will eventually destroy the MOSFET.

---

## Step 3: Configure in LEDMapping.h

Treat the solenoid driver as a standard GPIO LED output in LEDMapping.h:

```cpp
//  label                deviceType   info                        dimmable  activeLow
{ "MASTER_ARM_SW",      DEVICE_GPIO, { .gpioInfo = { PIN(12) } }, false,    false },
```

When DCS-BIOS sends a value of 1 for `MASTER_ARM_SW`, CockpitOS sets GPIO 12 HIGH, which turns on the MOSFET and fires the solenoid.

### Using Label Creator

1. Open Label Creator and select your label set
2. Click **Edit LEDs**
3. Find the switch state label (e.g., `MASTER_ARM_SW`)
4. Set **Device** = `GPIO`
5. Set **Port** = the GPIO pin connected to the MOSFET gate resistor
6. Set **Dimmable** = `false`
7. Set **Active Low** = `false` (or `true` if your circuit logic is inverted)
8. Save

---

## Step 4: Consider Pulsing vs. Continuous

CockpitOS LED outputs are **continuous** -- the GPIO stays HIGH as long as DCS says the switch is in that position. For solenoids, this has implications:

- **Continuous energization** keeps the solenoid pulled in and the toggle held in position, but draws continuous current and generates heat
- **Pulse mode** briefly fires the solenoid (100-200ms) to move the switch, then releases

For most cockpit applications, continuous mode is acceptable because:
- The physical toggle switch has a detent that holds it in position after the solenoid pushes it
- The solenoid only needs to overcome the detent force, not hold against it indefinitely

If your solenoid overheats under continuous load, consider:
- Using a solenoid rated for continuous duty
- Adding an external timer circuit (e.g., 555 timer for a one-shot pulse)
- Implementing a custom panel with pulse logic in firmware (see [Create Custom Panels](Create-Custom-Panel.md))

---

## Step 5: Safety Considerations

| Risk | Mitigation |
|---|---|
| GPIO current limit exceeded | Always use a MOSFET driver. Never connect a solenoid directly to a GPIO pin. ESP32 GPIOs source ~40mA max; solenoids need 200mA-2A. |
| Back-EMF damage | Always install a flyback diode across the solenoid coil. |
| Solenoid overheating | Use duty-rated solenoids. Do not energize continuously for extended periods if using a solenoid rated for intermittent duty. |
| MOSFET not switching at 3.3V | Use a logic-level MOSFET that fully turns on at 3.3V gate voltage. Standard MOSFETs may need 5V+ on the gate. |
| Floating gate during boot | Add a 10K pull-down resistor from the MOSFET gate to GND. During ESP32 boot, GPIOs may float, causing the solenoid to fire unexpectedly. |

---

## Example: Two-Position Toggle with Solenoid

For a two-position toggle switch (e.g., Master Arm ON/OFF), you typically need one solenoid to push the toggle in one direction. The toggle's own spring return handles the other direction.

```
+----------------------------------------------------------------------+
|  TOGGLE SWITCH WITH SOLENOID                                          |
+----------------------------------------------------------------------+
|                                                                      |
|     Side view:                                                       |
|                                                                      |
|       Toggle lever                                                   |
|           /                                                          |
|          / <-- moves this direction when solenoid fires              |
|     +---*---+                                                        |
|     | Pivot |                                                        |
|     +---+---+                                                        |
|         |                                                            |
|     [Solenoid]  <-- mounted to push lever from behind                |
|         |                                                            |
|     +---+---+                                                        |
|     | Panel |                                                        |
|     +-------+                                                        |
|                                                                      |
+----------------------------------------------------------------------+
```

For three-position switches, you may need two solenoids (one for each direction).

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Solenoid does not fire | MOSFET not triggering, wrong GPIO, no power | Check MOSFET gate voltage with a multimeter; verify GPIO pin; check external supply voltage |
| Solenoid fires at boot | GPIO floating during startup | Add a 10K pull-down resistor from MOSFET gate to GND |
| MOSFET gets hot | MOSFET not fully on at 3.3V gate, or solenoid drawing too much current | Use a logic-level MOSFET (Vgs threshold < 2.5V); check solenoid current rating |
| Solenoid clicks but does not move the switch | Insufficient stroke or force | Use a stronger solenoid or adjust mounting position |
| ESP32 resets when solenoid fires | Power supply dip, missing flyback diode | Add flyback diode; use a separate power supply for solenoids; add bulk capacitor on supply rail |

---

*See also: [LEDs Reference](../Hardware/LEDs.md) | [Config Reference](../Reference/Config.md) | [Create Custom Panels](Create-Custom-Panel.md)*
