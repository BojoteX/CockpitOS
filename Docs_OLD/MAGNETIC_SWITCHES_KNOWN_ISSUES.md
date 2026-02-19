# Magnetic Switches in DCS World — Known Issues & Solutions

> **Applies to:** CockpitOS + DCS-BIOS (Skunkworks fork)
> **Aircraft:** F/A-18C Hornet (primary), A-10C Warthog, AJS-37 Viggen, and others
> **Status:** Workarounds available, full solenoid support designed
> **Last updated:** 2026-02-18

---

## Table of Contents

1. [What Are Magnetic Switches?](#1-what-are-magnetic-switches)
2. [The Problem — Why Physical Cockpit Switches Desync](#2-the-problem--why-physical-cockpit-switches-desync)
3. [The Double-Flip Problem Explained](#3-the-double-flip-problem-explained)
4. [F/A-18C Hornet — Complete Magnetic Switch List](#4-fa-18c-hornet--complete-magnetic-switch-list)
5. [Other Aircraft With Magnetic/Electrically-Held Switches](#5-other-aircraft-with-magneticallyelectrically-held-switches)
6. [DCS-BIOS Lua-Side Workaround: defineToggleSwitchToggleOnly](#6-dcs-bios-lua-side-workaround-definetogleswitchtoggleonly)
7. [The LTD/R Bug — Still Unfixed in Stock DCS-BIOS](#7-the-ltdr-bug--still-unfixed-in-stock-dcs-bios)
8. [Solution 1: The Lua Fix (Free, No Hardware)](#8-solution-1-the-lua-fix-free-no-hardware)
9. [Solution 2: Momentary Switches (Cheap Hardware)](#9-solution-2-momentary-switches-cheap-hardware)
10. [Solution 3: Physical Solenoids via CockpitOS LEDMapping (Full Fidelity)](#10-solution-3-physical-solenoids-via-cockpitos-ledmapping-full-fidelity)
11. [How CockpitOS Solenoid Output Works — Step by Step](#11-how-cockpitos-solenoid-output-works--step-by-step)
12. [Hardware Wiring for Solenoids](#12-hardware-wiring-for-solenoids)
13. [Sync Exclusion — Preventing Firmware from Fighting DCS](#13-sync-exclusion--preventing-firmware-from-fighting-dcs)
14. [History of This Problem](#14-history-of-this-problem)
15. [Summary — Which Solution Should I Use?](#15-summary--which-solution-should-i-use)

---

## 1. What Are Magnetic Switches?

In real military aircraft, certain safety-critical switches are **spring-loaded to OFF**. An electromagnetic solenoid holds the switch in the ON position when the flight computer energizes it. The pilot can request ON by flipping the switch, but the **aircraft's avionics decide** whether to honor that request and when to release it.

When the computer de-energizes the solenoid, the spring physically snaps the switch back to OFF — regardless of pilot intent.

**This inverts the normal authority model:**

```
NORMAL TOGGLE SWITCH:
  Pilot flips switch → Hardware is truth → Game follows hardware
  AUTHORITY: The pilot (hardware) decides. Game obeys.

MAGNETIC SWITCH:
  Pilot flips switch → Game evaluates conditions → Game decides position
  AUTHORITY: The game (avionics) decides. Hardware should follow.
```

Every Arduino-based cockpit firmware (including stock DCS-BIOS Arduino library) assumes the normal model. For magnetic switches, this creates an unresolvable conflict where the firmware continuously fights the game.

---

## 2. The Problem — Why Physical Cockpit Switches Desync

The core conflict occurs when:

1. Your **physical panel** has a latching (maintained-position) toggle switch
2. The **real aircraft** uses a solenoid-held magnetic switch
3. **DCS World** auto-releases the virtual switch at some point

When DCS auto-releases, your physical switch stays in the ON position but the game has moved to OFF. Now your firmware sees "hardware says ON, game says OFF" and tries to re-send ON — but depending on how the Lua input processor works, this may or may not succeed, creating the infamous double-flip problem.

---

## 3. The Double-Flip Problem Explained

Using `LTD_R_SW` (Laser Designator/Ranger) as the example with stock `defineToggleSwitch`:

### The Scenario

You arm the laser, fire a weapon, the laser de-energizes. Your physical switch is still at ARM. Now you want to re-arm.

| Step | Physical Switch | Game State | What Happens |
|------|----------------|------------|--------------|
| 1 | ARM (1) | ARM (1) | Laser fires, weapon impacts |
| 2 | ARM (1) | SAFE (0) | DCS auto-releases. **Desync: physical=1, game=0** |
| 3 | Flip to SAFE (0) | SAFE (0) | Sends `set_state 0`. Game already at 0 — **no effect** |
| 4 | Flip to ARM (1) | SAFE (0) | Sends `set_state 1`. DCS processes — **NOW it works** |

**Result:** You had to flip the switch **twice** (OFF then ON) to get one transition. The first flip was wasted because you were "catching up" to the game's auto-release that happened while your switch was latched.

### Why It Happens

`defineToggleSwitch` (the stock Lua function) delegates to `defineTumb`, which sends **absolute position values**:

```lua
-- defineTumb input processor (Module.lua lines 1083-1115):
-- For set_state: sends the exact position value to DCS
GetDevice(device_id):performClickableAction(command, limits[1] + step * n)
```

It tells DCS: "set the switch TO position X." When your physical switch is at 1 and the game is already at 0, sending "set to 0" does nothing (already there), and only the subsequent "set to 1" actually triggers the transition.

---

## 4. F/A-18C Hornet — Complete Magnetic Switch List

### Two-Position Magnetic/Solenoid-Held Switches

| # | Switch | DCS-BIOS Label | Panel | Auto-Release Condition | DCS-BIOS Treatment | Status |
|---|--------|---------------|-------|----------------------|-------------------|--------|
| 1 | **Launch Bar** | `LAUNCH_BAR_SW` | Select Jett | Catapult launch / hydraulic loss | `defineToggleSwitchToggleOnly` | **Fixed since day 1** |
| 2 | **Hook Bypass** | `HOOK_BYPASS_SW` | Select Jett | Hook lowered / power loss | `defineToggleSwitchToggleOnly` | **Fixed since day 1** |
| 3 | **APU Control** | `APU_CONTROL_SW` | APU Panel | ~60s after 2nd generator online | `defineToggleSwitchToggleOnly` | **Fixed since day 1** |
| 4 | **Fuel Dump** | `FUEL_DUMP_SW` | Fuel Panel | Fuel quantity reaches threshold | `defineToggleSwitchToggleOnly` | **Fixed since day 1** |
| 5 | **Pitot Heater** | `PITOT_HEAT_SW` | ECS Panel | Auto-returns under certain conditions | `defineToggleSwitchToggleOnly` | **Fixed since day 1** |
| 6 | **LTD/R (Laser)** | `LTD_R_SW` | Sensor Panel | Weapon impact, FLIR→STBY, Master Arm→SAFE | `defineToggleSwitch` | **BUG — Not fixed** |

### Three-Position Magnetic/Spring-Loaded Switches

| # | Switch | DCS-BIOS Label | Panel | Behavior | DCS-BIOS Treatment | Notes |
|---|--------|---------------|-------|----------|-------------------|-------|
| 7 | **Engine Crank** | `ENGINE_CRANK_SW` | Engine Panel | Spring-return to OFF after generator online | `define3PosTumb` | Rarely an issue — see below |
| 8 | **Canopy** | `CANOPY_SW` | Right Console | OPEN→HOLD→CLOSE, spring-loaded | `define3PosTumb` | Rarely an issue — see below |

### Why Engine Crank and Canopy Are Rarely Problems

These are **spring-return** switches in the real aircraft. Home cockpit builders naturally implement them as:

- **Momentary toggles** — you hold to crank, release to center
- **Push buttons** — tap for L or R crank

Since the physical switch behavior already matches DCS (both spring back to center), there is no desync. The double-flip problem only occurs when you have a **latching** physical switch for a control that DCS auto-releases.

There is also no `define3PosTumbToggleOnly` variant in DCS-BIOS. If you ever needed one for a 3-position magnetic switch, it would require a new Lua function.

---

## 5. Other Aircraft With Magnetic/Electrically-Held Switches

### A-10C Warthog

The A-10C has the most extensive magnetic switch handling in DCS-BIOS. The Skunkworks team created a dedicated `defineElectricallyHeldSwitch` function with PUSH/RELEASE semantics (two separate commands for engage and disengage):

| Label | Panel | Description |
|-------|-------|-------------|
| `LASTE_EAC` | LASTE Panel | EAC On/Off |
| `SASP_YAW_SAS_L` | SAS Panel | Yaw SAS Left |
| `SASP_YAW_SAS_R` | SAS Panel | Yaw SAS Right |
| `SASP_PITCH_SAS_L` | SAS Panel | Pitch SAS Left |
| `SASP_PITCH_SAS_R` | SAS Panel | Pitch SAS Right |
| `ANTI_SKID_SWITCH` | Landing Gear Panel | Anti-Skid |
| `LCP_ANTICOLLISION` | Light System Control Panel | Anti-Collision Lights |

The A-10C approach uses a different input model (PUSH + RELEASE as separate DCS commands), which is specific to how the A-10C cockpit API works. This is not a general-purpose solution — it only works for aircraft where DCS exposes separate engage/disengage commands.

### AJS-37 Viggen

Two controls use `defineToggleSwitchToggleOnly`:
- `HUD_GLASS_POSITION` — HUD Reflector Glass
- `AFK_15_DEG_MODE` — AFK 15-degree mode

### SA-342 Gazelle

One control uses `defineToggleSwitchToggleOnly`:
- `MISSILE_LAUNCH_COVER` — Missile Launch Cover

---

## 6. DCS-BIOS Lua-Side Workaround: defineToggleSwitchToggleOnly

DCS-BIOS provides two toggle switch functions in `Module.lua`:

### `defineToggleSwitch` — The Stock Function

```lua
-- Sends ABSOLUTE POSITION values to DCS
-- "I want position 0" → performClickableAction(command, 0)
-- "I want position 1" → performClickableAction(command, 1)
function Module:defineToggleSwitch(identifier, device_id, command, arg_number, category, description)
    return self:defineToggleSwitchManualRange(identifier, device_id, command, arg_number, {0, 1}, category, description)
end
```

This tells DCS: "Set the switch TO this position." It does not check what position the switch is currently in.

### `defineToggleSwitchToggleOnly` — The State-Aware Function

```lua
-- Reads current game state FIRST, only sends toggle when transition is needed
-- Always sends value 1 (meaning "flip") — never sends 0
self:addInputProcessor(identifier, function(toState)
    local fromState = GetDevice(0):get_argument_value(arg_number)   -- read game state
    if
        toState == "TOGGLE"
        or fromState == 0 and (toState == "1" or toState == "INC")  -- need to go from 0→1
        or fromState == 1 and (toState == "0" or toState == "DEC")  -- need to go from 1→0
    then
        GetDevice(device_id):performClickableAction(command, 1)     -- always "flip"
    end
end)
```

This tells DCS: "Flip the switch" — but **only if a state change is actually needed**. It reads the current cockpit state before acting, preventing the firmware from sending redundant or conflicting commands.

### Version History

**Before DCS-BIOS PR #336 (October 2023)**, `defineToggleSwitchToggleOnly` was a simpler function that only responded to the `"TOGGLE"` command. It did not check game state and did not handle `set_state` (`"0"`, `"1"`) or `INC`/`DEC` commands:

```lua
-- OLD version (pre-PR #336) — dumb, blind toggle:
self:addInputProcessor(identifier, function(state)
    if state == "TOGGLE" then
        GetDevice(device_id):performClickableAction(command, 1)
    end
end)
```

**PR #336 by charliefoxtwo** (merged October 3, 2023) upgraded the function to be state-aware, adding `fromState` checking and support for `set_state`/`INC`/`DEC` inputs. This is the version that properly supports CockpitOS, which sends `set_state` commands.

If you are running DCS-BIOS version 0.8.3 or earlier AND your firmware sends `set_state` commands (not `TOGGLE`), the old `defineToggleSwitchToggleOnly` would silently ignore those commands. Upgrade to 0.9.0+ or apply the PR #336 changes to your Module.lua.

---

## 7. The LTD/R Bug — Still Unfixed in Stock DCS-BIOS

As of DCS-BIOS version **0.11.1** (latest at time of writing), `LTD_R_SW` is still defined as:

```lua
FA_18C_hornet:defineToggleSwitch("LTD_R_SW", 62, 3002, 441, "Sensor Panel", "LTD/R Switch, ARM/SAFE")
```

This is the **only 2-position magnetic switch in the F/A-18C** that was not given the `defineToggleSwitchToggleOnly` treatment. The other five (`LAUNCH_BAR_SW`, `HOOK_BYPASS_SW`, `APU_CONTROL_SW`, `FUEL_DUMP_SW`, `PITOT_HEAT_SW`) have been `defineToggleSwitchToggleOnly` since the FA-18C module was first added to DCS-BIOS.

This is a confirmed bug — the LTD/R switch behaves identically to the other magnetic switches and should use the same define function.

**Reference:** [DCS-Skunkworks GitHub Issue #134](https://github.com/DCS-Skunkworks/dcs-bios/issues/134) — "F18 LTD/R 2position Switch Bugged"

---

## 8. Solution 1: The Lua Fix (Free, No Hardware)

**Cost:** $0
**Difficulty:** One line change
**Fidelity:** Good (switch wins — physical position is always truth)
**Limitation:** You lose the auto-release feedback (physical switch stays ON when DCS releases)

### The Fix

In your DCS-BIOS installation, edit `FA-18C_hornet.lua`:

**File location:**
`%USERPROFILE%\Saved Games\DCS.openbeta\Scripts\DCS-BIOS\lib\modules\aircraft_modules\FA-18C_hornet.lua`

**Find this line** (around line 858 in stock 0.8.3, line 859 in 0.11.1):

```lua
FA_18C_hornet:defineToggleSwitch("LTD_R_SW", 62, 3002, 441, "Sensor Panel", "LTD/R Switch, ARM/SAFE")
```

**Replace with:**

```lua
-- FA_18C_hornet:defineToggleSwitch("LTD_R_SW", 62, 3002, 441, "Sensor Panel", "LTD/R Switch, ARM/SAFE")
FA_18C_hornet:defineToggleSwitchToggleOnly("LTD_R_SW", 62, 3002, 441, "Sensor Panel", "LTD/R Switch, ARM/SAFE")
```

### What This Does

With this change, when DCS auto-releases LTD/R (weapon impact, mode change, etc.):

1. DCS sets the switch to SAFE (0) internally
2. DCS-BIOS exports the new state
3. On the next export frame (~33ms), `defineToggleSwitchToggleOnly` reads `fromState = 0` (game says SAFE)
4. Your physical switch is still at ARM, so CockpitOS sends `"1"`
5. The Lua function sees `fromState == 0` and `toState == "1"` → sends toggle
6. DCS re-arms the laser

**Your physical switch wins. Every frame. No double-flip.**

The trade-off: you don't get the physical feedback of the switch snapping back to SAFE when DCS releases. Your switch stays at ARM and the game immediately re-arms to match. For Solution 3 (solenoids), see below.

---

## 9. Solution 2: Momentary Switches (Cheap Hardware)

**Cost:** ~$2 per switch
**Difficulty:** Swap the physical switch
**Fidelity:** Good (natural spring-return mimics real behavior)
**Limitation:** Not how the real aircraft switch works (latching vs momentary)

### The Approach

Replace the latching toggle switch with a **(ON)-OFF momentary toggle**. You hold the switch to ARM; when you release, it springs back to SAFE.

This perfectly mirrors how DCS treats magnetic switches internally — DCS expects a momentary input (you push, the game evaluates, the game decides to hold or release). There is zero desync because both the physical switch and the game naturally return to the OFF state.

### When This Works Well

- Switches you only toggle briefly (Engine Crank, Canopy)
- When you don't mind holding the switch during operation
- Quick prototyping before committing to a solenoid solution

### When This Doesn't Work Well

- When you want the tactile feel of a latching switch (real Hornet LTD/R latches magnetically)
- Long-duration holds (APU Control stays ON for minutes during startup)

---

## 10. Solution 3: Physical Solenoids via CockpitOS LEDMapping (Full Fidelity)

**Cost:** ~$5 per switch (solenoid + MOSFET + flyback diode + resistor)
**Difficulty:** Hardware wiring + one line in LEDMapping.h
**Fidelity:** Perfect (1:1 with real aircraft behavior)
**Limitation:** Requires the sync exclusion firmware change (not yet merged)

### The Insight

CockpitOS already has infrastructure to drive GPIOs from the DCS export stream — the **LEDMapping** system. Normally, you use this to drive indicator lights: DCS says "FIRE APU = ON" → GPIO goes HIGH → LED turns on.

A solenoid is electrically identical to an LED from the microcontroller's perspective: it's a load that needs to be ON or OFF based on a signal. The difference is current draw (solenoids need more than an LED), which is handled by a MOSFET transistor.

**The solenoid IS the solution.** When DCS says `APU_CONTROL_SW = 1`, the GPIO goes HIGH, the solenoid energizes, and the physical switch is held in the ON position. When DCS says `APU_CONTROL_SW = 0`, the GPIO goes LOW, the solenoid de-energizes, and the spring returns the switch to OFF.

Physics closes the loop. No firmware state machine needed. No edge detection. No debounce. No confirmation retries. The hardware does the work.

### What Already Exists in CockpitOS (Zero Firmware Changes for Output)

The DCS-BIOS JSON already exports the switch position as an output:

```json
"APU_CONTROL_SW": {
    "outputs": [{
        "address": 29890,
        "mask": 256,
        "shift_by": 8,
        "max_value": 1,
        "type": "integer"
    }]
}
```

CockpitOS already subscribes to this data via the export stream listener. The only thing needed is an LEDMapping entry to route the output to a GPIO pin.

### The Proof — LABEL_SET_MAGNETIC_TEST

CockpitOS already contains a test label set at `src/LABELS/LABEL_SET_MAGNETIC_TEST/LEDMapping.h` that demonstrates this concept:

```cpp
static const LEDMapping panelLEDs[] = {
  { "APU_CONTROL_SW", DEVICE_GPIO, {.gpioInfo = {15}}, false, false },
  { "APU_READY_LT",   DEVICE_NONE, {.gpioInfo = {0}},  false, false },
  { "ENGINE_CRANK_SW", DEVICE_NONE, {.gpioInfo = {0}},  false, false },
};
```

`APU_CONTROL_SW` is mapped to GPIO 15 as a `DEVICE_GPIO` output. When DCS exports `APU_CONTROL_SW = 1`, GPIO 15 goes HIGH. When DCS exports `APU_CONTROL_SW = 0`, GPIO 15 goes LOW. That GPIO drives a MOSFET which drives the solenoid.

### How to Add a Solenoid Output to Any Label Set

Add **one line** to your label set's `LEDMapping.h`:

```cpp
{ "LTD_R_SW", DEVICE_GPIO, {.gpioInfo = {PIN(7)}}, false, false },
```

That's it. CockpitOS will:
1. Subscribe to the `LTD_R_SW` output from the DCS-BIOS export stream
2. When the value changes, drive GPIO 7 HIGH (switch armed) or LOW (switch safe)
3. The GPIO drives a MOSFET, which drives the solenoid coil
4. The solenoid physically holds or releases the switch

The existing `LEDControl.cpp` output driver handles the GPIO toggling. The existing `onSelectorChange` subscription handles the data routing. Zero new firmware code is needed for the output path.

---

## 11. How CockpitOS Solenoid Output Works — Step by Step

Here is the complete data flow from DCS cockpit to physical solenoid:

```
DCS World (cockpit simulation)
    │
    ▼
DCS-BIOS Lua (export engine)
    │  dev0:get_argument_value(441) → 0 or 1
    │  Writes to binary stream at address/mask
    │
    ▼
UDP Binary Stream (30 Hz)
    │  Address 0x.... │ Mask 0x.... │ Value: 0 or 1
    │
    ▼
CockpitOS Protocol Parser (Protocol.cpp)
    │  Reassembles stream, detects address ranges
    │
    ▼
ExportStreamListener (ExportStreamListener.h)
    │  Matches address to subscribed listener
    │
    ▼
DCSBIOSBridge::onDcsBiosWrite()
    │  Dispatches to LED/selector/display handler
    │
    ▼
LEDControl (LEDControl.cpp)
    │  Finds LEDMapping entry for "LTD_R_SW"
    │  Calls digitalWrite(PIN(7), value)
    │
    ▼
ESP32 GPIO Pin 7
    │  HIGH = 3.3V, LOW = 0V
    │
    ▼
MOSFET Gate (e.g., IRLZ44N or 2N7000)
    │  3.3V on gate → MOSFET conducts → solenoid energized
    │  0V on gate → MOSFET off → solenoid de-energized
    │
    ▼
Solenoid Coil
    │  Energized → magnetic field holds switch in ON position
    │  De-energized → spring returns switch to OFF
    │
    ▼
Physical Switch Position
    │  HC165/GPIO reads the new position
    │  CockpitOS selector input reports the correct state
    │
    ▼
Loop Closed — Hardware and Game Are In Sync
```

The key insight: **the solenoid output drives the switch position, and the switch position is what the input reads**. The loop is closed physically through the hardware, not through firmware logic. No state machine. No edge detection. No race conditions.

---

## 12. Hardware Wiring for Solenoids

### Basic Circuit

```
ESP32 GPIO ──────┐
                 │
            ┌────┴────┐
            │  10kΩ   │  (gate pulldown — ensures OFF at boot)
            │ resistor │
            └────┬────┘
                 │
            ┌────┴────┐
            │  MOSFET  │
            │  Gate    │
            │         │
   VCC ─────┤ Drain   │
     │      │         │
     │      │ Source ──┤──── GND
     │      └─────────┘
     │
  ┌──┴──┐
  │     │  Solenoid Coil (5V or 12V)
  │     │
  └──┬──┘
     │
     ├──── Flyback Diode (1N4007) ────┐
     │     (cathode toward VCC)        │
     │                                 │
     └─────────────────────────────────┘
```

### Component Selection

| Component | Recommendation | Purpose |
|-----------|---------------|---------|
| **MOSFET** | IRLZ44N (logic-level, N-channel) or 2N7000 (small loads) | Switches solenoid current. Must be logic-level (3.3V gate threshold). |
| **Flyback diode** | 1N4007 | **CRITICAL.** Protects the MOSFET from voltage spikes when the solenoid de-energizes. Without this, the back-EMF from the collapsing magnetic field will destroy the MOSFET. |
| **Gate pulldown** | 10kΩ resistor, gate to GND | Ensures the MOSFET is OFF during ESP32 boot (GPIOs float during reset). Without this, the solenoid may energize randomly at power-on. |
| **Solenoid** | 5V or 12V push/pull solenoid, matched to your switch | The mechanical actuator. Must be physically integrated into your switch assembly. |
| **Power supply** | Separate from ESP32 (5V/12V as needed) | Solenoids draw significant current (200mA-1A+). Never power solenoids from the ESP32 3.3V rail. Use a separate supply with common ground. |

### Critical Safety Notes

1. **ALWAYS install the flyback diode.** Solenoids are inductors. When you turn off current through an inductor, it generates a voltage spike (back-EMF) that can reach hundreds of volts. The flyback diode clamps this spike. Without it, you will destroy the MOSFET and possibly damage the ESP32.

2. **ALWAYS install the gate pulldown resistor.** ESP32 GPIOs are high-impedance (floating) during boot. A floating gate on the MOSFET means the solenoid state is undefined during power-on. The 10kΩ pulldown ensures the MOSFET stays OFF until the firmware explicitly drives it.

3. **NEVER power solenoids from the ESP32 3.3V pin.** The ESP32's 3.3V regulator can supply ~500mA total for the entire board. A single solenoid can draw 200mA-1A. Use a separate 5V or 12V supply with common ground.

4. **ESP32 GPIO current limit: 40mA per pin, 1200mA total.** This is more than enough to drive a MOSFET gate (which draws essentially zero current), but never try to drive a solenoid directly from a GPIO pin.

---

## 13. Sync Exclusion — Preventing Firmware from Fighting DCS

### The Remaining Firmware Change

The output side (solenoid driving) works with **zero firmware changes** — just add the LEDMapping entry. However, the **input side** needs one change: preventing CockpitOS's sync system from overriding DCS's decision.

CockpitOS has two mechanisms that enforce hardware-is-truth:

1. **`validateSelectorSync()`** — On mission start or reconnect, compares every selector's hardware position to the game position. If they disagree, it forces the game to match hardware.

2. **`flushBufferedDcsCommands()`** — The group arbitration system that sends buffered selector commands to DCS. When a switch changes, it queues the command, waits for dwell time, and sends.

For magnetic switches, both of these must **skip** the magnetic-tagged controls. Otherwise:

- DCS releases APU_CONTROL_SW to OFF
- Solenoid de-energizes, switch snaps to OFF
- But before the switch physically moves, `validateSelectorSync()` fires and sees "hardware=ON, game=OFF" → forces game back to ON
- This creates a fight loop

### The Fix

Add an `isMagnetic` flag to the command history entries. In both `validateSelectorSync()` and `flushBufferedDcsCommands()`, skip entries where `isMagnetic == true`.

This is approximately **10 lines of code** — checking `if (entry.isMagnetic) continue;` at the appropriate points.

See `TODO/MAGNETIC_SWITCH_DESIGN_DOCUMENT.md` Section 18 for the complete specification.

---

## 14. History of This Problem

This problem has been documented and unsolved in the DCS home cockpit community for over a decade:

| Date | Event | Reference |
|------|-------|-----------|
| **2014** | Original DCS-BIOS author (jboecker) opens GitHub Issue #36 acknowledging the problem. States: "I assume that those who want to use a real electrically held switch have the knowledge to implement their own logic." | [dcs-bios/dcs-bios#36](https://github.com/dcs-bios/dcs-bios/issues/36) |
| **2022** | DCS-Skunkworks Issue #134 filed for F-18 LTD/R specifically. Reporter describes the auto-release behavior after weapon impact. | [DCS-Skunkworks/dcs-bios#134](https://github.com/DCS-Skunkworks/dcs-bios/issues/134) |
| **2023 (Oct)** | PR #336 by charliefoxtwo upgrades `defineToggleSwitchToggleOnly` to be state-aware (adds `fromState` checking, `set_state`/`INC`/`DEC` support). | [DCS-Skunkworks/dcs-bios#336](https://github.com/DCS-Skunkworks/dcs-bios/pull/336) |
| **2024 (Aug)** | DCS-BIOS 0.8.3 released. All Hornet magnetic switches use `defineToggleSwitchToggleOnly` **except** LTD_R_SW. |  |
| **2025+** | DCS-BIOS 0.11.1 (latest). LTD_R_SW **still** uses `defineToggleSwitch`. No fix merged. |  |
| **2026** | CockpitOS MAGNETIC_SWITCH_DESIGN_DOCUMENT v2.1 — identifies the ultra-simple solenoid approach using existing LEDMapping infrastructure. | `TODO/MAGNETIC_SWITCH_DESIGN_DOCUMENT.md` |

The original author's 2014 statement — "DCS-BIOS can only export the switch position, not whether the magnet is on or off" — was accurate for the original codebase but is misleading. The cockpit argument value (`dev0:get_argument_value`) DOES reflect the game-controlled position. When DCS de-energizes the virtual solenoid, the argument value changes to 0, and DCS-BIOS exports that change. This exported value is exactly what CockpitOS uses to drive the physical solenoid via LEDMapping.

---

## 15. Summary — Which Solution Should I Use?

| Solution | Cost | Code Changes | Hardware Changes | Fidelity | Best For |
|----------|------|-------------|-----------------|----------|----------|
| **1. Lua Fix** | $0 | 1 line in FA-18C_hornet.lua | None | Good — switch always wins, no double-flip. No auto-release feedback. | Builders without solenoids. Quick fix. |
| **2. Momentary Switch** | ~$2/switch | None | Swap latching toggle for (ON)-OFF momentary | Good — natural spring-return matches DCS. | Switches you toggle briefly (Engine Crank, Canopy). |
| **3. Physical Solenoid** | ~$5/switch | ~10 lines sync exclusion (firmware) + 1 line LEDMapping.h | MOSFET + flyback diode + solenoid per switch | Perfect — 1:1 real aircraft behavior. Physical auto-release feedback. | Full fidelity cockpit. The real solution. |

### The Bottom Line

For most builders: **Solution 1** (Lua fix) is the right answer. One line, zero hardware, eliminates the double-flip entirely. Your switch wins, the game follows.

For full fidelity: **Solution 3** (solenoids via LEDMapping) is the ultimate answer. CockpitOS's bidirectional architecture — export stream driving GPIO outputs that physically actuate the same switches the input system reads — closes the loop through physics. No other Arduino-based cockpit firmware can do this because none of them have the infrastructure to drive outputs from the export stream while simultaneously reading inputs from the same switches.

This is what CockpitOS was designed to do.
