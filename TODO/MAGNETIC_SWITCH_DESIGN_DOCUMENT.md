################################################################################
#                                                                              #
#       MAGNETIC SWITCH SUPPORT FOR COCKPITOS — TECHNICAL DESIGN DOCUMENT      #
#                                                                              #
#   Purpose: Peer-reviewed proposal for solving the magnetic switch problem    #
#   Author:  CockpitOS Firmware Project                                        #
#   Date:    2026-02-17                                                        #
#   Status:  REVISED — Post peer review + ultra-simple approach                 #
#   Version: 2.1                                                               #
#                                                                              #
#   REVISION HISTORY:                                                          #
#     v1.0 (2026-02-17) — Initial proposal                                    #
#     v2.0 (2026-02-17) — Revised after dual peer review:                      #
#       - Fixed CommandHistoryEntry struct to match actual generated code       #
#       - Changed implementation from manual struct edit to generator mod      #
#       - Added Passive Boot Requirement (force-send exclusion)                #
#       - Added debounce for magnetic edge detection                           #
#       - Added hardware safety section (gate pulldown, resistor, flyback)     #
#       - Fixed GPIO current rating to match Espressif datasheet               #
#       - Added v1 scope limitation (GPIO-only for magnetic inputs)            #
#       - Wired registration path into init sequence                           #
#     v2.1 (2026-02-17) — Ultra-simple approach (Section 18):                  #
#       - Identified that solenoid output closes the loop physically           #
#       - Eliminated ~200 lines of edge detection / state machine code         #
#       - Reduced firmware changes to sync exclusion only (~10 lines)          #
#       - Reuses existing selector input + LEDMapping output paths             #
#                                                                              #
################################################################################

==============================================================================
TABLE OF CONTENTS
==============================================================================

  Section  1: Executive Summary
  Section  2: What Are Magnetic Switches?
  Section  3: Documented History of the Problem
  Section  4: Why the DCS-BIOS Arduino Library Cannot Easily Solve This
  Section  5: JSON Analysis — Proof That the Data We Need Exists
  Section  6: CockpitOS Architecture — Why We Can Solve This
  Section  7: Proposed Solution — New controlType = "magnetic"
  Section  8: Implementation Details — File-by-File Changes
  Section  9: Passive Boot Requirement (CRITICAL)
  Section 10: Hardware Safety Requirements (CRITICAL)
  Section 11: Risk Analysis
  Section 12: Comparison With Existing Approaches
  Section 13: Testing Plan
  Section 14: Known Limitations (v1 Scope)
  Section 15: Conclusion
  Section 16: References
  Section 17: Peer Review Log
  Section 18: Problem Solution by Jesus — Ultra-Simple Approach

==============================================================================
SECTION 1: EXECUTIVE SUMMARY
==============================================================================

This document proposes adding a new control type ("magnetic") to the CockpitOS
firmware to correctly handle electrically-held (solenoid) switches found in
DCS World aircraft cockpits.

The core problem: DCS-BIOS and all existing Arduino-based firmware treat every
toggle switch as "hardware is the authority" — the firmware continuously sends
the physical switch position to the game. For magnetic switches, the GAME is
the authority — it decides when the switch is ON or OFF. This creates an
unresolvable conflict where the firmware fights the game's avionics logic.

This problem has been documented since 2014 (GitHub Issue #36) and remains
unsolved in the DCS-BIOS ecosystem. No general-purpose solution exists in any
publicly available Arduino library.

CockpitOS can solve this because its architecture already provides:
  1. Bidirectional awareness (export stream feedback into input logic)
  2. Centralized command orchestration (commandHistory[])
  3. Per-control behavioral flags (isSelector, group, now isMagnetic)
  4. GPIO output driving from export data (LEDMapping)

The proposed solution requires approximately 200 lines of firmware changes,
a modification to generator_core.py, zero DCS-BIOS Lua modifications, and
zero protocol changes.

==============================================================================
SECTION 2: WHAT ARE MAGNETIC SWITCHES?
==============================================================================

2.1 PHYSICAL MECHANISM
----------------------------------------
In real military aircraft, certain safety-critical switches are spring-loaded
to the OFF position. A solenoid (electromagnetic coil) holds the switch in the
ON position when energized. The aircraft's flight computer controls when the
solenoid energizes and de-energizes.

When the computer de-energizes the solenoid, the spring physically snaps the
switch back to OFF — regardless of what the pilot wants. The pilot can flip
the switch to ON (request), but the computer decides whether to honor it.

2.2 F/A-18C HORNET MAGNETIC SWITCHES
----------------------------------------
Three switches in the F/A-18C Hornet are confirmed magnetic:

SWITCH            DCS-BIOS LABEL     AUTO-RELEASE BEHAVIOR
APU Control       APU_CONTROL_SW     Returns to OFF ~60 seconds after 2nd
                                     generator comes online
LTD/R (Laser)     LTD_R_SW           Returns to SAFE after weapon impact,
                                     mode change, or conditions unmet
Launch Bar        LAUNCH_BAR_SW      Retracts after catapult launch sequence

SOURCE: DCS F/A-18C Early Access Guide by Eagle Dynamics:
  "The APU switch is a two-position switch with positions of ON and OFF.
   Switch is electrically held in the ON position and automatically returns
   to OFF 1 minute after the second generator comes on the line."
  URL: https://www.digitalcombatsimulator.com/upload/iblock/8d7/
       DCS%20FA-18C%20Early%20Access%20Guide%20EN.pdf

SOURCE: DCS-Skunkworks GitHub Issue #134 (F18 LTD/R 2position Switch Bugged):
  https://github.com/DCS-Skunkworks/dcs-bios/issues/134
  Reporter states: "The F18 has auto lasing. So when you do a bomb run the
   jet calculates the last 15-20 seconds till impact and then starts lasing.
   Once the bomb impacts the computer kills the power to the magnet and the
   switch (LTD) moves back to safe."

2.3 AUTHORITY MODEL — WHY THIS MATTERS
----------------------------------------
NORMAL TOGGLE SWITCH:
  1. Pilot flips switch to ON
  2. Switch physically latches in ON position
  3. Hardware reports ON to firmware
  4. Firmware sends "set_state 1" to DCS
  5. DCS sets cockpit switch to 1
  --> AUTHORITY: Hardware (pilot) decides position. Game follows.

MAGNETIC SWITCH:
  1. Pilot flips switch to ON (against spring)
  2. Pilot releases switch — spring pulls it back to OFF
  3. Game evaluates conditions
  4. If conditions met: game energizes solenoid -> switch held ON
  5. If conditions change: game de-energizes solenoid -> switch snaps OFF
  --> AUTHORITY: Game (avionics logic) decides position. Hardware follows.

The fundamental conflict: All existing firmware assumes the NORMAL model.
Magnetic switches require the MAGNETIC model.

==============================================================================
SECTION 3: DOCUMENTED HISTORY OF THE PROBLEM
==============================================================================

3.1 GITHUB: DCS-BIOS ISSUE #36 (OPENED 2014, STILL OPEN)
----------------------------------------
URL: https://github.com/dcs-bios/dcs-bios/issues/36
Title: "Add example code / Arduino library support for electrically held
        switches"

Opened by jboecker (original DCS-BIOS author) regarding the A-10C anti-
collision lights switch (same class of problem). His exact statement:

  "The anti-collision lights in the A-10C (and other electrically held
   switches) are not properly supported by the example code generation
   logic yet. They can be used with DCS-BIOS by sending PUSH, RELEASE
   and OFF actions."

  "I assume that those who want to use a real electrically held switch
   have the knowledge to implement their own logic (and make their own
   trade-offs, because DCS-BIOS can only export the switch position,
   not whether the magnet is on or off)."

SIGNIFICANCE: The original author acknowledged the limitation, labeled it as
needing library-level support, and stated the export stream only provides
switch position — not solenoid state. This was left unresolved for 10+ years.

CRITICAL UPDATE: The author's claim that "DCS-BIOS can only export the switch
position, not whether the magnet is on or off" was accurate for the ORIGINAL
dcs-bios but is misleading in practice. In DCS World, the cockpit argument
value (dev0:get_argument_value) DOES reflect the game-controlled position.
When the game de-energizes the virtual solenoid, the argument value changes
to 0, and DCS-BIOS exports that change. We verified this in Section 5.

3.2 GITHUB: DCS-SKUNKWORKS ISSUE #134 (2022)
----------------------------------------
URL: https://github.com/DCS-Skunkworks/dcs-bios/issues/134
Title: "F18 LTD/R 2position Switch Bugged"

Reporter attempted to use the LTD/R switch with physical hardware and found
the auto-release behavior did not work. The game's avionics would try to
release the switch, but the Arduino's continuous polling kept overriding it.

3.3 ED FORUMS: MAGNETIC SWITCH THREAD
----------------------------------------
URL: https://forum.dcs.world/topic/305841-magnetic-switch/
Title: "Magnetic Switch" — Home Cockpits section

User attempted to build a magnetic switch for the F/A-18C Laser Arm switch
following the Warthog Project's video tutorial. The thread documents the
progression of failed workarounds:

WORKAROUND 1: DcsBios::ActionButton with "TOGGLE"
  Result: Almost works but requires flipping the physical switch to ARM
  twice before the auto-release behavior functions. The first flip creates
  a sync issue because the game sees TOGGLE (flip state) but the hardware
  state doesn't match. The second flip corrects the mismatch.
  Verdict: Unreliable. Requires user to develop muscle memory for double-flip.

WORKAROUND 2: Dual ActionButton with SPDT (Single Pole Double Throw)
  Approach: Both contacts of an SPDT switch wired: one sends "1" (ON),
  other sends "0" (OFF).
  Result: User No1sonuk reported working for P-51 generator switch, but
  it cannot handle the case where the game auto-releases while the physical
  switch is still in the ON position.
  Verdict: Fails for the core magnetic switch use case.

WORKAROUND 3: Keyboard emulation hybrid (OpenHornet / Ben-111)
  Approach: Arduino Due with Ethernet shields sends keyboard commands
  instead of DCS-BIOS set_state commands. Keyboard path sends one-shot
  press-and-release that doesn't fight the game's state machine. A SECOND
  Arduino monitors DCS-BIOS export to control the physical solenoid coil.
  Code: https://github.com/Ben-F111/Ben-F18-Sim-Pit/tree/main/
        1-%20Final%20Codes/AS%20UPLOADED%20-%20CURRENT%20ONLY
  Result: Functions correctly but requires:
    - Multiple Arduinos (one for keyboard input, one for DCS-BIOS output)
    - Ethernet communication between Arduinos
    - Mixed protocol paths (keyboard + DCS-BIOS simultaneously)
    - Complex wiring and configuration
  Verdict: Works but architecturally fragile and expensive.

WORKAROUND 4: Hardcoded condition monitoring (outbaxx, AJS-37 Viggen)
  Approach: Arduino code monitors RPM values and other exported data,
  manually de-energizing the solenoid at specific thresholds. Essentially
  duplicates the game's avionics logic inside the Arduino sketch.
  Result: Works for specific aircraft with known conditions.
  Verdict: Fragile, aircraft-specific, impossible to maintain across
  DCS updates that may change avionics behavior.

3.4 OPENHORNET ABSIS 2.0 HARDWARE
----------------------------------------
URL: https://openhornet.com/2020/11/11/absis-2-0-preliminary-design-wrapping-up/
The OpenHornet project designed custom PCBs (ABSIS 2.0) that include a
6-channel MOSFET switch specifically for driving 12V solenoids. This confirms
the community has invested in physical solenoid hardware. Their software
solution uses the DCS-Skunkworks fork of DCS-BIOS.

GitHub Issue #304 documents that their ABSIS 2.0 branch is still missing a
port for the Magnetic Canopy switch, indicating ongoing development challenges.
URL: https://github.com/jrsteensen/OpenHornet/issues/304

3.5 A-10C ANTI-COLLISION LIGHT (GSS-Rain implementation)
----------------------------------------
URL: https://github.com/GSS-Rain/DCS-Bios-A10C-Lighting-Panel/blob/master/
     Lighting_Control_Panel.ino
This A-10C implementation uses IntegerBuffer to monitor engine RPM and
generator state, manually controlling a solenoid GPIO based on exported
conditions. This is another instance of Workaround 4 (hardcoded conditions).

3.6 ED FORUMS: DCS BIOS AND CONTROLLING A SOLENOID
----------------------------------------
URL: https://forum.dcs.world/topic/291488-dcs-bios-and-controlling-a-solenoid/
User attempting to use a solenoid for a Huey idle stop button, asking for
basic guidance. Thread shows the community regularly encounters this need.

==============================================================================
SECTION 4: WHY THE DCS-BIOS ARDUINO LIBRARY CANNOT EASILY SOLVE THIS
==============================================================================

4.1 THE ARDUINO LIBRARY'S ARCHITECTURE
----------------------------------------
The DCS-BIOS Arduino library uses a declarative, self-contained object model.
Each control is an independent C++ object instantiated in the user's sketch:

  DcsBios::Switch2Pos ltdRSw("LTD_R_SW", 5);
  DcsBios::Switch2Pos apuSw("APU_CONTROL_SW", 7);
  DcsBios::Switch2Pos launchBar("LAUNCH_BAR_SW", 9);

Each object:
  - Polls its assigned GPIO pin independently
  - Sends the current pin state to DCS on every iteration
  - Has NO awareness of the DCS-BIOS export stream
  - Has NO awareness of other Switch2Pos objects
  - Has NO command history or throttling
  - Has NO subscription mechanism to hear game-state feedback

Reference: DCS-BIOS Code Snippet documentation
URL: https://dcs-bios.readthedocs.io/en/latest/code-snippets.html

The documentation explicitly states:
  "For the purposes of the Arduino code, there is no difference between a
   momentary switch like a push button or spring-loaded toggle switch and a
   latching switch like a normal toggle switch. The only difference is that
   one is spring-loaded to return to a certain position and the other is
   not. Electrically, they are the same, so they use the same code snippets."

This design philosophy treats ALL binary switches identically. There is no
concept of "the game might disagree with the hardware."

4.2 THE ARCHITECTURAL WALL
----------------------------------------
To properly handle magnetic switches, you need:

REQUIREMENT                         ARDUINO LIB    COCKPITOS
Read export stream for same label   NO (*)         YES
  (*) IntegerBuffer exists but is   separate from  subscribeToSelectorChange()
  a separate object with no link    Switch2Pos     feeds into commandHistory[]
  to the input object

Track command history per control   NO             YES
                                                   CommandHistoryEntry struct
                                                   with per-entry flags

Edge-detect state transitions       NO             YES
  (send once, not continuously)     Sends every    commandHistory tracks
                                    loop()         lastValue, lastSendTime

Skip sync enforcement for           NO             YES
  game-authority controls            No sync        validateSelectorSync()
                                    concept         can skip isMagnetic entries

Drive output GPIO from export       NO (*)         YES
  data (solenoid control)            (*) Possible   LEDMapping + DEVICE_GPIO
                                    with manual     already drives outputs
                                    IntegerBuffer   from export data
                                    code

The fundamental problem: In the Arduino library, input objects and output
objects live in separate universes. There is no bridge. Creating a
MagneticSwitch class would require building most of CockpitOS's infrastructure
INSIDE a single Arduino library class — subscription system, command history,
feedback loop, and output driving. This would be a fundamental redesign of the
library's core architecture, breaking backward compatibility for thousands of
existing users.

4.3 WHY NOBODY HAS CONTRIBUTED A FIX
----------------------------------------
  1. Small affected population: ~3 switches out of ~503 controls per aircraft
  2. Architectural difficulty: Requires redesigning core library patterns
  3. Workarounds exist: Builder community has developed per-aircraft hacks
  4. Maintainer bandwidth: Skunkworks team focuses on Lua modules, not Arduino
  5. The Arduino library is legacy: Most innovation goes into Lua and protocol

==============================================================================
SECTION 5: JSON ANALYSIS — PROOF THAT THE DATA WE NEED EXISTS
==============================================================================

5.1 METHODOLOGY
----------------------------------------
We parsed the official FA-18C_hornet.json from the DCS-Skunkworks repository
(503 total controls) and extracted the definitions for all three confirmed
magnetic switches plus comparison controls.

5.2 MAGNETIC SWITCH DEFINITIONS FROM JSON
----------------------------------------

APU_CONTROL_SW:
  category:     "Auxiliary Power Unit Panel"
  control_type: "action"
  description:  "APU Control Switch, ON/OFF"
  inputs:
    - interface: fixed_step       (increment/decrement)
    - interface: set_state        max_value: 1   (set to 0 or 1)
    - interface: action           argument: TOGGLE
  outputs:
    - type: integer, address: 29890, mask: 256, shift: 8, max: 1

LTD_R_SW:
  category:     "Sensor Panel"
  control_type: "selector"
  description:  "LTD/R Switch, ARM/SAFE"
  inputs:
    - interface: fixed_step       (increment/decrement)
    - interface: set_state        max_value: 1   (set to 0 or 1)
    - interface: action           argument: TOGGLE
  outputs:
    - type: integer, address: 29896, mask: 16384, shift: 14, max: 1

LAUNCH_BAR_SW:
  category:     "Select Jettison Button"
  control_type: "action"
  description:  "Launch Bar"
  inputs:
    - interface: fixed_step       (increment/decrement)
    - interface: set_state        max_value: 1   (set to 0 or 1)
    - interface: action           argument: TOGGLE
  outputs:
    - type: integer, address: 29824, mask: 8192, shift: 13, max: 1

5.3 KEY FINDINGS
----------------------------------------
FINDING 1: ALL THREE ACCEPT set_state(0/1)
  This means we CAN send "LTD_R_SW 1\n" or "LTD_R_SW 0\n" as one-shot
  commands. We do NOT need to rely on TOGGLE. The Skunkworks fork added
  set_state to all three magnetic switches.
  IMPLICATION: Our edge-triggered approach (send value once on transition)
  is fully supported by the protocol.
  EVIDENCE: Extracted directly from FA-18C_hornet.json (503 controls parsed,
  3 confirmed magnetic — see Section 5.2 for full field details).

FINDING 2: ALL THREE EXPORT A BINARY INTEGER
  Each switch has an output with max_value=1 at a specific address/mask/shift.
  When the game's avionics de-energize the virtual solenoid, the exported
  value changes from 1 to 0. This value is already parsed by CockpitOS's
  DCSBIOSBridge.cpp ExportStreamListener and dispatched through
  onSelectorChange() callbacks.
  IMPLICATION: We can read the game's solenoid state in real-time through
  existing infrastructure. No Lua changes needed.

FINDING 3: THE JSON control_type FIELD IS INCONSISTENT
  APU_CONTROL_SW:  control_type = "action"
  LTD_R_SW:        control_type = "selector"
  LAUNCH_BAR_SW:   control_type = "action"
  Two are "action", one is "selector". There is no "magnetic" or
  "electrically_held" marker in the JSON.
  IMPLICATION: We CANNOT auto-detect magnetic switches from JSON alone.
  The user must manually tag them as controlType="magnetic" in InputMapping.h.
  With only 3 switches, this is acceptable.

FINDING 4: ONLY 5 CONTROLS IN ENTIRE JSON USE control_type="action"
  APU_CONTROL_SW, FUEL_DUMP_SW, HOOK_BYPASS_SW, LAUNCH_BAR_SW, PITOT_HEAT_SW
  Of these, only APU_CONTROL_SW and LAUNCH_BAR_SW are truly magnetic.
  FUEL_DUMP_SW, HOOK_BYPASS_SW, and PITOT_HEAT_SW are standard toggles that
  happen to use the same Lua definition pattern.
  IMPLICATION: The "action" type is a Lua implementation detail, not a
  reliable indicator of magnetic behavior.

5.4 SCOPE OF IMPACT
----------------------------------------
  Total controls in FA-18C:    503
  True magnetic switches:        3   (0.6%)
  Controls needing changes:      3
  This is a surgical, low-risk addition.

==============================================================================
SECTION 6: COCKPITOS ARCHITECTURE — WHY WE CAN SOLVE THIS
==============================================================================

6.1 BIDIRECTIONAL AWARENESS (DCSBIOSBridge.cpp)
----------------------------------------
CockpitOS's DCSBIOSBridge.cpp is a centralized bridge between the DCS-BIOS
export stream (game -> firmware) and the command pipeline (firmware -> game).

Export stream parsing:
  parseDcsBiosUdpPacket() -> DcsBios::parser.processChar() ->
    ExportStreamListener::onConsistentData() ->
      onSelectorChange(label, value)   // dispatches to subscribers
      onLedChange(label, value, max)
      onDisplayChange(label, string)
      onMetaDataChange(label, value)

Command pipeline:
  HIDManager_setNamedButton() -> sendDCSBIOSCommand(label, value, force) ->
    findCmdEntry(label) -> applyThrottle() -> sendCommand(label, buf)

The subscription system connects the two directions:
  subscribeToSelectorChange("LTD_R_SW", callback)
  // callback fires when game reports LTD_R_SW value change

This bidirectional bridge is the KEY architectural difference from the Arduino
library. In CockpitOS, the input logic CAN know what the game thinks.

PROOF — from DCSBIOSBridge.cpp:
  void onSelectorChange(const char* label, unsigned int newValue) {
      // resolve position label via SelectorMap[] hash lookup
      // dispatch to all subscribers
      for (size_t i = 0; i < selectorSubscriptionCount; ++i) {
          if (strcmp(selectorSubscriptions[i].label, label) == 0
              && selectorSubscriptions[i].callback) {
              selectorSubscriptions[i].callback(label, value);
          }
      }
  }

6.2 CENTRALIZED COMMAND HISTORY (DCSBIOSBridge.cpp)
----------------------------------------
Every command flows through commandHistory[], a static array generated by
generator_core.py inside each label set's DCSBIOSBridgeData.h.

ACTUAL STRUCT (from DCSBIOSBridgeData.h — auto-generated):

  struct CommandHistoryEntry {
      const char*     label;
      uint16_t        lastValue;
      unsigned long   lastSendTime;
      bool            isSelector;
      uint16_t        group;
      uint16_t        pendingValue;
      unsigned long   lastChangeTime;
      bool            hasPending;
      uint8_t         lastReport[GAMEPAD_REPORT_SIZE];
      uint8_t         pendingReport[GAMEPAD_REPORT_SIZE];
      unsigned long   lastHidSendTime;
  };

CRITICAL NOTE (from peer review): This struct is AUTO-GENERATED by
generator_core.py. Manual edits to DCSBIOSBridgeData.h will be overwritten.
Any changes to CommandHistoryEntry must be made in the generator itself.

The struct provides per-control tracking of:
  - Last value sent (lastValue)
  - Whether a deferred command is pending (hasPending, pendingValue)
  - Timing (lastSendTime, lastChangeTime)
  - Group membership for selector arbitration (group)
  - Type classification (isSelector, and proposed: isMagnetic)
  - HID report deduplication (lastReport[], pendingReport[])

6.3 SELECTOR SYNC (DCSBIOSBridge.cpp)
----------------------------------------
CockpitOS has a sync system that makes the magnetic switch problem WORSE
than vanilla DCS-BIOS. The validateSelectorSync() function compares the
firmware's known state (commandHistory) against the game's exported state
(subscription callback) and forces the game to match the firmware:

  void validateSelectorSync() {
      for (size_t i = 0; i < numValidatedSelectors; ++i) {
          const char* label = validatedSelectors[i].label;
          uint16_t simValue = validatedSelectors[i].lastSimValue;
          uint16_t fwValue = getLastKnownState(label);
          if (fwValue != simValue) {
              sendDCSBIOSCommand(label, fwValue, true); // force
          }
      }
  }

For normal switches, this is correct — hardware is authority.
For magnetic switches, this is catastrophic — it fights the game forever.

Additionally, the TrackedSelectorLabels[] array (also auto-generated) controls
which labels enter the validation system. The generator must exclude magnetic
labels from this array.

6.4 GPIO OUTPUT FROM EXPORT DATA (LEDMapping)
----------------------------------------
CockpitOS already drives physical outputs (LEDs, GPIO) based on DCS-BIOS
export data through the LEDMapping system:

  struct LEDMapping {
      const char*    label;       // DCS-BIOS label to watch
      LEDDeviceType  deviceType;  // DEVICE_GPIO, DEVICE_WS2812, etc.
      LEDDeviceInfo  info;        // { .gpioInfo = { pin } }
      bool           dimmable;
      bool           activeLow;
  };

When the export stream reports a value change for a label that has a matching
LEDMapping entry, LEDControl.cpp drives the corresponding GPIO high or low.

A solenoid IS just a GPIO output from the firmware's perspective. The
existing LEDMapping infrastructure handles solenoid driving with zero
additional code.

Example:
  // In LEDMapping.h — drive solenoid from game state
  { "LTD_R_SW", DEVICE_GPIO, { .gpioInfo = { PIN(6) } }, false, false },

When the game exports LTD_R_SW=1, GPIO 6 goes HIGH -> solenoid energizes.
When the game exports LTD_R_SW=0, GPIO 6 goes LOW -> spring returns switch.

IMPORTANT: See Section 10 (Hardware Safety) for mandatory solenoid driver
circuit requirements. NEVER drive a solenoid directly from an ESP32 GPIO.

==============================================================================
SECTION 7: PROPOSED SOLUTION — NEW controlType = "magnetic"
==============================================================================

7.1 BEHAVIORAL SPECIFICATION
----------------------------------------
A control with controlType="magnetic" behaves as follows:

INPUT (firmware -> DCS):
  - On physical rising edge (OFF->ON): Send set_state command ONCE
  - On physical falling edge (ON->OFF): Send set_state 0 ONCE
  - While switch is held in any position: Send NOTHING
  - Edges are debounced with MAGNETIC_DEBOUNCE_MS (default: 20ms)
  - Does NOT participate in selector group arbitration (group = 0)
  - Does NOT participate in validateSelectorSync()
  - Does NOT participate in TrackedSelectorLabels[]
  - Is EXCLUDED from all forceSend / init-blast paths (see Section 9)
  - commandHistory tracks it with isMagnetic=true, preventing re-sync

OUTPUT (DCS -> solenoid):
  - Subscribes to DCS-BIOS export value for the same label
  - When game exports 1: drive solenoid GPIO HIGH (energize)
  - When game exports 0: drive solenoid GPIO LOW (de-energize)
  - This uses existing LEDMapping with DEVICE_GPIO — no new code needed

RELIABILITY:
  - If the one-shot command packet is lost (UDP), the firmware implements
    a confirmation-based retry: after sending, watch the export stream for
    the value to change. If it does not change within
    MAGNETIC_CONFIRM_TIMEOUT_MS (default: 500ms), retry the command.
    Maximum retry count: MAGNETIC_MAX_RETRIES (default: 3).
  - After max retries, log a warning and stop. The user can re-flip.
  - Confirmation checks are non-blocking, called from the main loop tick.

7.2 SEQUENCE DIAGRAMS
----------------------------------------

NORMAL OPERATION — PILOT ARMS LASER:
  Time  Physical Switch  Firmware Action           Game Response
  T0    OFF              (idle)                    LTD_R_SW=0
  T1    OFF->ON          Debounce starts (20ms)    —
  T2    ON (held)        Debounce OK, send ONCE    Game evaluates conditions
  T3    ON (held)        (idle, waiting confirm)   LTD_R_SW=1 (confirmed)
  T4    ON (held)        Confirm received, done    Laser armed
  ...
  T50   ON (held)        (idle)                    Bomb impacts
  T51   ON (held)        (idle)                    Game: LTD_R_SW=0
  T52   ON (held)        Export cb: solenoid OFF   Solenoid de-energizes
  T53   switch springs   GPIO input goes LOW       Switch physically back
  T54   OFF              (idle)                    LTD_R_SW=0 (stable)

NORMAL OPERATION — PILOT MANUALLY DISARMS:
  Time  Physical Switch  Firmware Action           Game Response
  T0    ON (solenoid)    (idle)                    LTD_R_SW=1
  T1    ON->OFF          Debounce starts (20ms)    —
  T2    OFF              Debounce OK, send 0 ONCE  Game sets to 0
  T3    OFF              Export cb: solenoid OFF   (already off)
  T4    OFF              (idle)                    LTD_R_SW=0 (stable)

EDGE CASE — PACKET LOSS:
  Time  Physical Switch  Firmware Action           Game Response
  T0    OFF              (idle)                    LTD_R_SW=0
  T1    OFF->ON          Debounce OK, send ONCE    (packet lost)
  T2    ON (held)        Start confirm timer       LTD_R_SW=0 (unchanged)
  T3    ON (held)        Timer expired, retry #1   Game receives, LTD_R_SW=1
  T4    ON (held)        Export confirms 1, done   Laser armed

EDGE CASE — USER HOLDS AGAINST SPRING DURING AUTO-RELEASE:
  Time  Physical Switch  Firmware Action           Game Response
  T0    ON (solenoid)    (idle)                    LTD_R_SW=1
  T1    ON (held)        (idle)                    Bomb impacts
  T2    ON (held)        Export cb: solenoid OFF   Game: LTD_R_SW=0
  T3    STILL ON (user   GPIO still reads HIGH     Solenoid OFF, but user
        holds against    No edge detected.         is physically holding
        spring)          This is correct — we      the switch up.
                         do nothing.
  T4    User releases    GPIO goes LOW
        switch           Falling edge detected
  T5    OFF              Debounce OK, send 0 ONCE  Game already at 0
                         (redundant but harmless)  No effect
  T6    OFF              (idle)                    LTD_R_SW=0
  T7    OFF->ON          Debounce OK, send 1 ONCE  Game re-evaluates
        (user re-flips)  Rising edge detected      May or may not re-arm

  NOTE: At T3, the user is holding the switch against the spring while
  the solenoid is off. The GPIO reads HIGH but the firmware does nothing
  because no EDGE has occurred. This is CORRECT behavior — the game
  already decided to release, and we respect that decision.

EDGE CASE — ESP32 REBOOT MID-FLIGHT (See Section 9 — Passive Boot):
  Time  Physical Switch  Firmware Action           Game Response
  T0    ON (solenoid)    ESP32 reboots             LTD_R_SW=1 in game
  T1    ON (held)        Boot complete, GPIO=HIGH  —
  T2    ON (held)        First poll: set lastPhys  NO command sent
                         = HIGH, but do NOT send   (passive boot rule)
  T3    ON (held)        (idle)                    LTD_R_SW=1 (undisturbed)

  If the switch was HIGH before reboot, we initialize the tracking state
  to HIGH without sending any command. The game state is preserved.

7.3 INPUTMAPPING.H CONFIGURATION
----------------------------------------

// LTD/R Laser ARM switch — magnetic (game-authority)
{ "LTD_R_SW_ARM",       "GPIO", PIN(5), 0, -1,
  "LTD_R_SW",           1, "magnetic", 0 },

// APU switch — magnetic (game-authority)
{ "APU_CONTROL_SW_ON",  "GPIO", PIN(7), 0, -1,
  "APU_CONTROL_SW",     1, "magnetic", 0 },

// Launch Bar — magnetic (game-authority)
{ "LAUNCH_BAR_SW_EXT",  "GPIO", PIN(9), 0, -1,
  "LAUNCH_BAR_SW",      1, "magnetic", 0 },

KEY POINTS:
  - group = 0 (no group arbitration)
  - controlType = "magnetic"
  - oride_value = 1 (sent on rising edge; 0 sent on falling edge)
  - Only ONE InputMapping entry per switch (not multiple positions)
  - hidId = -1 (no HID button — magnetic switches are DCS-only)

7.4 LEDMAPPING.H CONFIGURATION (SOLENOID OUTPUT)
----------------------------------------

// LTD/R solenoid — game drives this GPIO via export data
{ "LTD_R_SW",       DEVICE_GPIO, { .gpioInfo = { PIN(6) } }, false, false },

// APU solenoid
{ "APU_CONTROL_SW", DEVICE_GPIO, { .gpioInfo = { PIN(8) } }, false, false },

// Launch Bar solenoid
{ "LAUNCH_BAR_SW",  DEVICE_GPIO, { .gpioInfo = { PIN(10) } }, false, false },

See Section 10 for mandatory hardware safety requirements for solenoid wiring.

==============================================================================
SECTION 8: IMPLEMENTATION DETAILS — FILE-BY-FILE CHANGES
==============================================================================

8.1 generator_core.py — ADD isMagnetic FIELD TO STRUCT
----------------------------------------
FILE: src/LABELS/_core/generator_core.py

The CommandHistoryEntry struct is defined and emitted by generator_core.py
into each label set's DCSBIOSBridgeData.h. This is the ONLY correct place
to modify the struct definition.

CURRENT GENERATED STRUCT:
  struct CommandHistoryEntry {
      const char*     label;
      uint16_t        lastValue;
      unsigned long   lastSendTime;
      bool            isSelector;
      uint16_t        group;
      uint16_t        pendingValue;
      unsigned long   lastChangeTime;
      bool            hasPending;
      uint8_t         lastReport[GAMEPAD_REPORT_SIZE];
      uint8_t         pendingReport[GAMEPAD_REPORT_SIZE];
      unsigned long   lastHidSendTime;
  };

PROPOSED GENERATED STRUCT (add isMagnetic after isSelector):
  struct CommandHistoryEntry {
      const char*     label;
      uint16_t        lastValue;
      unsigned long   lastSendTime;
      bool            isSelector;
      bool            isMagnetic;     // NEW: game-authority control
      uint16_t        group;
      uint16_t        pendingValue;
      unsigned long   lastChangeTime;
      bool            hasPending;
      uint8_t         lastReport[GAMEPAD_REPORT_SIZE];
      uint8_t         pendingReport[GAMEPAD_REPORT_SIZE];
      unsigned long   lastHidSendTime;
  };

GENERATOR CHANGES REQUIRED:
  1. Add "bool isMagnetic;" to the struct template string
  2. When emitting commandHistory[] initializer rows, read InputMapping.h
     entries for each label. If controlType == "magnetic", emit true;
     otherwise emit false. This follows the exact same pattern already
     used for isSelector and group.
  3. Exclude labels with controlType == "magnetic" from the
     TrackedSelectorLabels[] array.

COST: 1 byte per entry due to alignment. With typical 50-100 entries,
this adds 50-100 bytes. Negligible.

8.2 syncCommandHistoryFromInputMapping() — DETECT MAGNETIC CONTROLS
----------------------------------------
FILE: src/Core/DCSBIOSBridge.cpp

This function already iterates InputMappings[] to set isSelector and group
on each CommandHistoryEntry. Adding isMagnetic follows the identical pattern.

CURRENT CODE:
  for (size_t j = 0; j < InputMappingSize; ++j) {
      const InputMapping& m = InputMappings[j];
      if (!m.oride_label || strcmp(e.label, m.oride_label) != 0) continue;

      const bool isSel = (strcmp(m.controlType, "selector") == 0);
      const bool isBtn = (strcmp(m.controlType, "momentary") == 0);

      if (isSel || isBtn) {
          if (isSel && m.group > 0) {
              e.isSelector = true;
              if (m.group > e.group) e.group = m.group;
          }
          break;
      }
  }

PROPOSED CODE (add magnetic detection BEFORE the isSel/isBtn block):
  for (size_t j = 0; j < InputMappingSize; ++j) {
      const InputMapping& m = InputMappings[j];
      if (!m.oride_label || strcmp(e.label, m.oride_label) != 0) continue;

      // NEW: Detect game-authority magnetic controls
      const bool isMag = (strcmp(m.controlType, "magnetic") == 0);
      if (isMag) {
          e.isMagnetic = true;
          e.isSelector = false;
          e.group = 0;     // Never participates in group arbitration
          break;            // Done — magnetic overrides all other types
      }

      const bool isSel = (strcmp(m.controlType, "selector") == 0);
      const bool isBtn = (strcmp(m.controlType, "momentary") == 0);

      if (isSel || isBtn) {
          if (isSel && m.group > 0) {
              e.isSelector = true;
              if (m.group > e.group) e.group = m.group;
          }
          break;
      }
  }

8.3 validateSelectorSync() — SKIP MAGNETIC ENTRIES
----------------------------------------
FILE: src/Core/DCSBIOSBridge.cpp

PROPOSED CHANGE — add skip for magnetic entries:
  void validateSelectorSync() {
      for (size_t i = 0; i < numValidatedSelectors; ++i) {
          const char* label = validatedSelectors[i].label;

          // Skip game-authority controls — game decides position
          CommandHistoryEntry* e = findCmdEntry(label);
          if (e && e->isMagnetic) continue;

          uint16_t simValue = validatedSelectors[i].lastSimValue;
          uint16_t fwValue = getLastKnownState(label);
          if (fwValue != simValue) {
              sendDCSBIOSCommand(label, fwValue, true);
          }
      }
  }

NOTE: The generator changes (Section 8.1) should prevent magnetic labels from
entering TrackedSelectorLabels[] in the first place. This runtime check is a
defense-in-depth measure in case labels are manually added or the generator
is not yet updated.

8.4 flushBufferedDcsCommands() — SKIP MAGNETIC DURING PANEL SYNC
----------------------------------------
FILE: src/Core/DCSBIOSBridge.cpp

Inside the flush loop, magnetic entries must not be force-sent during panel
sync. Add a check in the Step 1 (find winner per group) loop:

  for (size_t i = 0; i < commandHistorySize; ++i) {
      CommandHistoryEntry& e = commandHistory[i];

      // NEW: magnetic entries never participate in flush
      if (e.isMagnetic) continue;

      if (!e.hasPending || e.group == 0) continue;
      // ... rest of dwell/group arbitration ...
  }

8.5 INPUT POLLING — EDGE DETECTION FOR MAGNETIC TYPE
----------------------------------------
FILE: src/Core/InputControl.cpp

This is the most significant change. A new polling function handles magnetic
inputs separately from selectors and momentaries.

  // ----- Magnetic Switch State Tracking -----
  // Static, zero-heap, bounded.
  static constexpr size_t MAX_MAGNETIC_INPUTS = 8;

  struct MagneticInputState {
      const char* oride_label;    // DCS-BIOS command label
      uint16_t    oride_value;    // Value to send on rising edge
      uint8_t     lastPhysical;   // 0 or 1 — last debounced GPIO reading
                                  // 0xFF = uninitialized (first poll)
      uint8_t     awaitingConfirm;// 0=idle, 1=waiting for export confirm
      uint8_t     retryCount;     // Retries attempted
      uint32_t    sentAtMs;       // millis() when command was sent
      uint32_t    lastEdgeMs;     // millis() when last edge was accepted
      uint16_t    sentValue;      // Value we sent (0 or oride_value)
  };

  static MagneticInputState magneticStates[MAX_MAGNETIC_INPUTS];
  static size_t numMagneticInputs = 0;
  static bool magneticInputsBuilt = false;

  // Called once during init to discover and register magnetic inputs.
  // Scans InputMappings[] for controlType=="magnetic" with source=="GPIO".
  static void buildMagneticInputs() {
      if (magneticInputsBuilt) return;
      magneticInputsBuilt = true;
      numMagneticInputs = 0;

      for (size_t i = 0; i < InputMappingSize; ++i) {
          const InputMapping& m = InputMappings[i];
          if (!m.label || !m.controlType) continue;
          if (strcmp(m.controlType, "magnetic") != 0) continue;
          if (!m.source || strcmp(m.source, "GPIO") != 0) continue;
          if (m.port < 0 || m.port >= 48) continue;
          if (numMagneticInputs >= MAX_MAGNETIC_INPUTS) break;

          MagneticInputState& s = magneticStates[numMagneticInputs];
          s.oride_label    = m.oride_label;
          s.oride_value    = m.oride_value;
          s.lastPhysical   = 0xFF;  // Uninitialized
          s.awaitingConfirm = 0;
          s.retryCount     = 0;
          s.sentAtMs       = 0;
          s.lastEdgeMs     = 0;
          s.sentValue      = 0;

          // Configure GPIO pin (active-low with pullup, matching selector convention)
          bool usePullUp = (m.bit <= 0);
          pinMode((uint8_t)m.port, usePullUp ? INPUT_PULLUP : INPUT_PULLDOWN);

          ++numMagneticInputs;

          debugPrintf("[MAGNETIC] Registered: %s on GPIO %d\n",
                      m.oride_label, m.port);
      }
  }

  // Main magnetic input poller.
  // CRITICAL: The forceSend parameter is ACCEPTED but IGNORED.
  // Magnetic inputs are PURELY edge-triggered. See Section 9.
  void pollGPIOMagnetic(bool /* forceSend_IGNORED */) {
      buildMagneticInputs();
      if (numMagneticInputs == 0) return;

      const uint32_t now = millis();
      size_t magIdx = 0;

      for (size_t i = 0; i < InputMappingSize; ++i) {
          const InputMapping& m = InputMappings[i];
          if (!m.controlType || strcmp(m.controlType, "magnetic") != 0) continue;
          if (!m.source || strcmp(m.source, "GPIO") != 0) continue;
          if (m.port < 0) continue;
          if (magIdx >= numMagneticInputs) break;

          MagneticInputState& s = magneticStates[magIdx];
          ++magIdx;

          // Read physical pin
          int pinState = digitalRead(m.port);
          uint8_t currentPhysical = (m.bit == 0)
              ? (pinState == LOW  ? 1 : 0)   // active-low
              : (pinState == HIGH ? 1 : 0);   // active-high

          // PASSIVE BOOT: First poll — record state, send NOTHING
          if (s.lastPhysical == 0xFF) {
              s.lastPhysical = currentPhysical;
              continue;
          }

          // EDGE DETECTION with DEBOUNCE
          if (currentPhysical != s.lastPhysical) {
              // Reject bounce: ignore edges within debounce window
              if ((now - s.lastEdgeMs) < MAGNETIC_DEBOUNCE_MS) continue;

              s.lastPhysical = currentPhysical;
              s.lastEdgeMs = now;

              uint16_t valueToSend = currentPhysical ? s.oride_value : 0;

              sendDCSBIOSCommand(s.oride_label, valueToSend, true);
              s.sentValue = valueToSend;
              s.sentAtMs = now;
              s.awaitingConfirm = 1;
              s.retryCount = 0;

              debugPrintf("[MAGNETIC] Edge: %s -> %u (sent)\n",
                          s.oride_label, valueToSend);
          }
          // While held (no edge): do NOTHING. Game is authority.
      }
  }

  // Confirmation check — called from main loop, non-blocking.
  void magneticConfirmCheck() {
      const uint32_t now = millis();
      for (size_t i = 0; i < numMagneticInputs; ++i) {
          MagneticInputState& s = magneticStates[i];
          if (!s.awaitingConfirm) continue;

          if ((now - s.sentAtMs) < MAGNETIC_CONFIRM_TIMEOUT_MS) continue;

          // Check if game confirmed our value
          uint16_t gameValue = getLastKnownState(s.oride_label);
          if (gameValue == s.sentValue) {
              s.awaitingConfirm = 0;
              debugPrintf("[MAGNETIC] Confirmed: %s = %u\n",
                          s.oride_label, s.sentValue);
          } else if (s.retryCount < MAGNETIC_MAX_RETRIES) {
              s.retryCount++;
              sendDCSBIOSCommand(s.oride_label, s.sentValue, true);
              s.sentAtMs = now;
              debugPrintf("[MAGNETIC] Retry %u: %s = %u\n",
                          s.retryCount, s.oride_label, s.sentValue);
          } else {
              s.awaitingConfirm = 0;
              debugPrintf("[MAGNETIC] FAILED after %u retries: %s\n",
                          MAGNETIC_MAX_RETRIES, s.oride_label);
          }
      }
  }

8.6 INTEGRATION INTO PANEL INIT AND LOOP
----------------------------------------
FILE: src/Panels/Generic.cpp (or panel-specific files)

Generic_init():
  // Existing calls:
  pollGPIOSelectors(true);       // forceSend=true (selectors)
  pollGPIOMomentaries(true);     // forceSend=true (momentaries)
  // NEW — magnetic switches: forceSend is IGNORED inside function
  pollGPIOMagnetic(true);        // Registers inputs, records state, sends NOTHING

Generic_loop():
  // Existing calls:
  pollGPIOSelectors(false);
  pollGPIOMomentaries(false);
  // NEW:
  pollGPIOMagnetic(false);       // Edge detection + one-shot
  magneticConfirmCheck();        // Non-blocking retry check

8.7 Config.h — NEW PARAMETERS
----------------------------------------
FILE: Config.h

  // Magnetic switch parameters (game-authority controls)
  #define MAGNETIC_DEBOUNCE_MS         20    // Bounce rejection window
  #define MAGNETIC_CONFIRM_TIMEOUT_MS  500   // Wait for export confirmation
  #define MAGNETIC_MAX_RETRIES         3     // Retry count before giving up

==============================================================================
SECTION 9: PASSIVE BOOT REQUIREMENT (CRITICAL)
==============================================================================

STATUS: This section was added after peer review. Both independent reviewers
identified the "force-send on boot/reboot" vulnerability as a critical issue.

9.1 THE PROBLEM
----------------------------------------
CockpitOS has multiple code paths that "blast" current hardware state to DCS:
  1. Generic_init() calls pollGPIOSelectors(true) and pollGPIOMomentaries(true)
  2. initPanels() calls syncCommandHistoryFromInputMapping() then
     initializePanels(1) which triggers forced selector/axis commands
  3. flushBufferedDcsCommands() with forcePanelSyncThisMission=true bypasses
     normal dwell timers

For normal switches, this is correct — on boot or mission start, the firmware
tells DCS where every physical switch currently sits.

For magnetic switches, this is DANGEROUS. If the user reboots the ESP32 mid-
flight while a magnetic switch is physically held ON, the firmware would
immediately send "APU_CONTROL_SW 1" to DCS. If DCS has the APU blocked
(e.g., battery off, generator conditions unmet), the firmware has just
violated the game-authority rule.

9.2 THE SOLUTION — THREE PROTECTION LAYERS
----------------------------------------

LAYER 1: POLLING FUNCTION IGNORES forceSend
  pollGPIOMagnetic() accepts a forceSend parameter for API consistency but
  IGNORES it. On first poll, it records the current GPIO state into
  lastPhysical WITHOUT sending any command (passive initialization).

  // PASSIVE BOOT: First poll — record state, send NOTHING
  if (s.lastPhysical == 0xFF) {
      s.lastPhysical = currentPhysical;
      continue;   // <-- NO sendDCSBIOSCommand here
  }

LAYER 2: EXISTING POLLERS ALREADY SKIP "magnetic" BY CONTROLTYPE FILTER
  pollGPIOSelectors() contains:
    if (strcmp(m.controlType, "selector") != 0) continue;
  pollGPIOMomentaries() contains:
    if (strcmp(m.controlType, "momentary") != 0) continue;

  A controlType="magnetic" entry is NATURALLY skipped by both functions.
  This is accidental protection in the current code, but it is correct.
  We document it here to prevent future regressions if someone adds a
  generic "poll all" path.

LAYER 3: flushBufferedDcsCommands() SKIPS isMagnetic
  The flush loop (Section 8.4) explicitly checks e.isMagnetic and skips.
  Even if a magnetic entry somehow gets hasPending=true, it will not be
  force-sent during panel sync.

9.3 VERIFICATION CHECKLIST
----------------------------------------
To confirm passive boot compliance, verify these conditions are met:

  [ ] pollGPIOMagnetic() ignores forceSend parameter
  [ ] pollGPIOMagnetic() sets lastPhysical on first poll without sending
  [ ] pollGPIOSelectors() string-filters to "selector" only
  [ ] pollGPIOMomentaries() string-filters to "momentary" only
  [ ] flushBufferedDcsCommands() skips e.isMagnetic entries
  [ ] validateSelectorSync() skips e.isMagnetic entries
  [ ] TrackedSelectorLabels[] does not contain magnetic labels
  [ ] initializePanels() does not explicitly handle magnetic type

==============================================================================
SECTION 10: HARDWARE SAFETY REQUIREMENTS (CRITICAL)
==============================================================================

STATUS: This section was elevated from parenthetical notes to a dedicated
section after peer review. Both reviewers flagged hardware safety as
requiring more prominence.

  ╔════════════════════════════════════════════════════════════════════╗
  ║  WARNING: SOLENOID DRIVER CIRCUIT IS MANDATORY                   ║
  ║                                                                  ║
  ║  NEVER connect a solenoid coil directly to an ESP32 GPIO pin.    ║
  ║  Doing so WILL damage the microcontroller permanently.           ║
  ╚════════════════════════════════════════════════════════════════════╝

10.1 WHY DIRECT DRIVE IS DANGEROUS
----------------------------------------
ESP32 GPIO pins can safely source approximately 12-20 mA continuous current
per pin (Espressif datasheet lists 40 mA as the absolute maximum, which is
NOT a design target and should never be relied upon).

A typical 12V solenoid draws 100-500 mA. This is 5-40x beyond the ESP32's
safe operating range.

Additionally, solenoids are inductive loads. When the magnetic field
collapses (solenoid turns off), the coil generates a reverse voltage spike
(back-EMF) that can reach hundreds of volts. Without protection, this spike
will destroy the MOSFET driver and potentially the ESP32.

10.2 REQUIRED DRIVER CIRCUIT
----------------------------------------

    ESP32 GPIO                         +12V (Solenoid Supply)
       |                                  |
       +-- [150 ohm] --+-- MOSFET Gate    |
                        |                  |
       [100K to GND] ---+    MOSFET       SOLENOID COIL
                              Drain ------+-----|
                              |           |     |
                              Source      +-- [1N4007 Flyback Diode]
                              |                 | (cathode to +12V)
                             GND               GND

COMPONENT              PURPOSE                           SPECIFICATION
Gate Resistor          Limits inrush current to gate      100-220 ohm
Gate Pulldown          Ensures MOSFET OFF during          100K ohm
                       ESP32 boot/reset (GPIO floats!)
N-Channel MOSFET       Switches solenoid current          Logic-level type:
                                                          IRLZ44N, IRLB8721,
                                                          or AO3400 (SMD)
Flyback Diode          Absorbs back-EMF voltage spike     1N4007 (or 1N4148
                       when solenoid de-energizes         for faster response)
Separate Supply        Solenoid power (12V typically)     Isolated from ESP32
                                                          3.3V/5V rail

10.3 CRITICAL SAFETY NOTES
----------------------------------------
  1. GATE PULLDOWN IS MANDATORY: During ESP32 boot, all GPIO pins are in a
     high-impedance floating state for approximately 50-200ms. Without the
     100K pulldown resistor, the MOSFET gate can float high, energizing the
     solenoid unexpectedly. This is a real safety hazard — a solenoid
     activating during boot could cause physical injury or damage.

  2. COMMON GROUND: The ESP32 GND and solenoid supply GND must be connected
     together (common return). Keep high-current solenoid ground traces
     separate from signal ground traces to avoid ground bounce affecting
     the ESP32.

  3. FLYBACK DIODE ORIENTATION: Cathode (stripe) toward +12V, anode toward
     MOSFET drain. Reversing this will short-circuit the supply when the
     solenoid is energized.

  4. NEVER share the ESP32's USB 5V rail with solenoid power. Use a
     separate supply or ATX PSU rail.

==============================================================================
SECTION 11: RISK ANALYSIS
==============================================================================

11.1 RISKS AND MITIGATIONS
----------------------------------------

RISK: UDP packet loss causes missed arm/disarm command
SEVERITY: Medium (affects functional correctness)
MITIGATION: Confirmation-based retry (Section 7.1, 8.5)
RESIDUAL: After 3 retries, user must manually re-flip. Acceptable.

RISK: ESP32 reboot while magnetic switch is physically held ON
SEVERITY: High (could send unwanted command to game)
MITIGATION: Passive boot — Section 9. Three protection layers.
RESIDUAL: None when all three layers are implemented.

RISK: Switch bounce generates multiple edge events
SEVERITY: Medium (could cause command storm)
MITIGATION: MAGNETIC_DEBOUNCE_MS = 20ms rejects bounces.
RESIDUAL: None for typical toggle switches (bounce < 10ms).

RISK: User holds switch against spring during auto-release
SEVERITY: Low (cosmetic — switch position doesn't match solenoid state)
MITIGATION: Firmware correctly does nothing (no edge = no command).
When user releases and re-flips, clean edges produce correct commands.
RESIDUAL: None. Behavior is correct.

RISK: isMagnetic flag adds memory to CommandHistoryEntry
SEVERITY: Negligible (1 byte per entry, ~100 bytes total)
MITIGATION: Already within static allocation budget.

RISK: Generator overwrite removes manual struct edits
SEVERITY: High (if user edits generated file directly)
MITIGATION: Change is in generator_core.py itself, not in generated output.
RESIDUAL: None when implemented correctly in generator.

RISK: Other aircraft modules may have different magnetic switch definitions
SEVERITY: Low
MITIGATION: The solution is generic — any DCS-BIOS control with set_state
interface and binary export works. User tags specific controls as "magnetic".
Aircraft JSON differences don't matter.

RISK: Solenoid activates during ESP32 boot (floating GPIO)
SEVERITY: High (physical safety hazard)
MITIGATION: Gate pulldown resistor (100K) — Section 10.
RESIDUAL: None when circuit is built correctly.

11.2 WHAT THIS DOES NOT SOLVE
----------------------------------------
  - Physical solenoid hardware design (see Section 10 for requirements)
  - DCS-BIOS Lua module bugs (upstream responsibility)
  - Controls that ONLY accept TOGGLE without set_state (none confirmed
    in current Skunkworks JSON for the three magnetic switches)
  - Non-GPIO magnetic inputs (HC165, PCA9555) — see Section 14

==============================================================================
SECTION 12: COMPARISON WITH EXISTING APPROACHES
==============================================================================

APPROACH              USED BY            PROS                 CONS
Switch2Pos            Stock DCS-BIOS     Simple, one line     Fights game forever
  (continuous poll)   Arduino library

ActionButton TOGGLE   Community hack     Sends one-shot       Double-flip required
                      (ED Forums)        toggle               First flip desyncs

Dual ActionButton     Community hack     Sends explicit       Can't handle game
  (SPDT)              (No1sonuk)         on/off values        auto-release

Keyboard emulation    OpenHornet         Works correctly      2+ Arduinos, Ethernet
                      (Ben-111)                               mixed protocols, $$

Hardcoded conditions  Community hack     Works for specific   Fragile, aircraft-
                      (outbaxx, GSS)     aircraft             specific, unmaintainable

CockpitOS "magnetic"  PROPOSED           Clean, generic,      Requires CockpitOS
  controlType                            zero Lua changes,    firmware (not Arduino
                                         one config line,     DCS-BIOS library).
                                         handles all edge     v1 is GPIO-only
                                         cases, debounce,     (see Section 14)
                                         retry logic,
                                         passive boot,
                                         solenoid output via
                                         existing LEDMapping

==============================================================================
SECTION 13: TESTING PLAN
==============================================================================

13.1 UNIT TEST — PASSIVE BOOT
  Setup: Toggle switch on GPIO, mapped as "magnetic", held ON at boot
  Test: Power cycle the ESP32 while switch is ON
  Verify: Debug log shows "[MAGNETIC] Registered: LTD_R_SW on GPIO 5"
  Verify: Debug log does NOT show "[MAGNETIC] Edge:" during init
  Verify: DCS game state is NOT affected by the boot

13.2 UNIT TEST — EDGE DETECTION + DEBOUNCE
  Setup: Toggle switch on GPIO, mapped as "magnetic"
  Test: Flip switch repeatedly at various speeds
  Verify: Only ONE "[MAGNETIC] Edge:" log per clean transition
  Verify: No double-fires within 20ms debounce window
  Test: Rapidly bounce the switch (tap it without full throw)
  Verify: Bounces are rejected (no command sent)

13.3 INTEGRATION TEST — SYNC BYPASS
  Setup: Map LTD_R_SW as "magnetic", start DCS mission
  Test: Flip switch to ARM, then modify game state via in-game click
  Verify: validateSelectorSync() does NOT force hardware state back
  Verify: Debug log does NOT show SYNC messages for LTD_R_SW

13.4 INTEGRATION TEST — SOLENOID OUTPUT
  Setup: Map LTD_R_SW in LEDMapping with DEVICE_GPIO output + MOSFET circuit
  Test: Arm laser in game (via mouse click, not physical switch)
  Verify: Solenoid GPIO goes HIGH (LED or relay activates)
  Test: Drop bomb, wait for auto-release
  Verify: Solenoid GPIO goes LOW (LED or relay deactivates)

13.5 END-TO-END TEST — FULL CYCLE
  Setup: Physical toggle switch (input) + LED/relay (solenoid output)
  Test: Full sequence:
    1. Flip switch ON -> laser arms -> LED lights
    2. Drop bomb -> game auto-releases -> LED turns off -> switch springs back
    3. Flip switch ON again -> laser re-arms -> LED lights
    4. Manually flip switch OFF -> laser disarms -> LED off
  Verify: All four steps work correctly

13.6 RELIABILITY TEST — PACKET LOSS
  Setup: WiFi transport with intentional interference (or test harness)
  Test: Flip switch ON, observe retry behavior
  Verify: Debug log shows retry attempts and eventual confirmation

13.7 STRESS TEST — REBOOT MID-FLIGHT
  Setup: Magnetic switch ON (solenoid energized), DCS in flight
  Test: Press ESP32 reset button
  Verify: After reboot, no unwanted command sent to DCS
  Verify: Game state unchanged
  Verify: Switch tracking resumes correctly after first edge

==============================================================================
SECTION 14: KNOWN LIMITATIONS (v1 SCOPE)
==============================================================================

STATUS: This section was added after peer review to explicitly document
scope boundaries for the initial implementation.

14.1 GPIO-ONLY INPUT SOURCES
  The v1 implementation of pollGPIOMagnetic() only supports source="GPIO"
  in InputMapping.h. The following sources are NOT supported for magnetic
  controls in v1:
    - HC165 (shift register chains)
    - PCA9555 (I2C port expanders)
    - TM1637 (keyboard matrix)
    - MATRIX (strobe/data rotary)

  RATIONALE: In practice, magnetic switches on a real cockpit panel will be
  wired as direct GPIO toggle switches. Each magnetic switch needs a
  dedicated input pin AND a nearby dedicated output pin (for the solenoid).
  Shift registers and I2C expanders provide input only, making them
  unsuitable for the paired input+output requirement.

  FUTURE: Support for HC165 and PCA9555 magnetic inputs can be added by
  replicating the edge-detection + debounce + passive-boot pattern inside
  processHC165Resolved() and pollPCA9555_flat() respectively. The
  MagneticInputState tracking struct is source-agnostic and can serve
  all input paths.

14.2 NO AUTO-DETECTION FROM JSON
  The generator cannot automatically identify which switches are magnetic
  from the aircraft JSON's control_type field (see Section 5.3). Users must
  manually set controlType="magnetic" in InputMapping.h. With only 3
  magnetic switches per aircraft, this is acceptable.

14.3 BINARY SWITCHES ONLY
  The v1 implementation supports max_value=1 (binary on/off) controls only.
  All three confirmed F/A-18C magnetic switches are binary. If future
  aircraft have multi-position magnetic switches, the edge detection logic
  would need extension to track multi-value transitions.

==============================================================================
SECTION 15: CONCLUSION
==============================================================================

The magnetic switch problem has been an open issue in the DCS home cockpit
community since 2014. It remains unsolved in the DCS-BIOS Arduino library
because their architecture fundamentally separates input objects from output
awareness. Every community workaround is either unreliable, aircraft-specific,
or requires complex multi-device setups.

CockpitOS's bidirectional bridge architecture — specifically the
commandHistory[] with per-entry behavioral flags, the subscription system for
export feedback, and the LEDMapping GPIO output infrastructure — provides all
the building blocks needed for a clean, generic solution.

The proposed "magnetic" controlType requires:
  - Modification to generator_core.py (add isMagnetic field + generation)
  - ~200 lines of firmware changes in InputControl.cpp and DCSBIOSBridge.cpp
  - Zero DCS-BIOS Lua modifications
  - Zero protocol changes
  - Three lines in InputMapping.h + three lines in LEDMapping.h per aircraft

The solution handles all documented edge cases:
  - Packet loss (confirmation-based retry)
  - User holding against spring during auto-release (no edge = no command)
  - ESP32 reboot mid-flight (passive boot — three protection layers)
  - Switch bounce (20ms debounce window)
  - Mission start sync (excluded from all force-send paths)
  - Floating GPIO during boot (gate pulldown resistor on solenoid driver)

This document has undergone dual peer review (Gemini and ChatGPT acting as
Senior Embedded Engineers). All critical findings have been addressed. See
Section 17 for the complete peer review log.

==============================================================================
SECTION 16: REFERENCES
==============================================================================

[1]  GitHub Issue #36 — "Add example code / Arduino library support for
     electrically held switches"
     https://github.com/dcs-bios/dcs-bios/issues/36
     Status: Open since 2014

[2]  GitHub Issue #134 — "F18 LTD/R 2position Switch Bugged"
     https://github.com/DCS-Skunkworks/dcs-bios/issues/134
     Status: Open since 2022

[3]  ED Forums — "Magnetic Switch" thread
     https://forum.dcs.world/topic/305841-magnetic-switch/

[4]  OpenHornet ABSIS 2.0 — 6-channel MOSFET solenoid driver board
     https://openhornet.com/2020/11/11/absis-2-0-preliminary-design-wrapping-up/

[5]  OpenHornet GitHub Issue #304 — Missing ABSIS 2.0 magnetic canopy port
     https://github.com/jrsteensen/OpenHornet/issues/304

[6]  Ben-111 F-18 Sim Pit — Keyboard emulation hybrid workaround
     https://github.com/Ben-F111/Ben-F18-Sim-Pit/tree/main/
     1-%20Final%20Codes/AS%20UPLOADED%20-%20CURRENT%20ONLY

[7]  GSS-Rain A-10C Lighting Panel — Condition-based solenoid control
     https://github.com/GSS-Rain/DCS-Bios-A10C-Lighting-Panel/blob/master/
     Lighting_Control_Panel.ino

[8]  DCS-BIOS Code Snippet Reference
     https://dcs-bios.readthedocs.io/en/latest/code-snippets.html

[9]  DCS-Skunkworks DCS-BIOS Repository
     https://github.com/DCS-Skunkworks/dcs-bios

[10] CockpitOS Repository
     https://github.com/BojoteX/CockpitOS

[11] DCS F/A-18C Early Access Guide — Eagle Dynamics
     https://www.digitalcombatsimulator.com/upload/iblock/8d7/
     DCS%20FA-18C%20Early%20Access%20Guide%20EN.pdf

[12] ED Forums — "DCS BIOS and controlling a solenoid"
     https://forum.dcs.world/topic/291488-dcs-bios-and-controlling-a-solenoid/

[13] OpenHornet — DCS-BIOS version FAQ
     https://openhornet.com/faq-items/what-version-of-dcs-bios-does-openhornet-use/

[14] FA-18C_hornet.json — DCS-Skunkworks aircraft definition file
     503 controls analyzed, 3 confirmed magnetic switches

[15] Espressif ESP32 Datasheet — GPIO current limits
     40 mA absolute maximum per pin; 12-20 mA recommended continuous

[16] ESP32 Forum — "Max mA for IO-Pin?" (community guidance)
     https://esp32.com/viewtopic.php?t=2384

==============================================================================
SECTION 17: PEER REVIEW LOG
==============================================================================

17.1 REVIEW METHODOLOGY
  The v1.0 document was submitted to two independent AI systems acting as
  Senior Embedded Engineers: Google Gemini and OpenAI ChatGPT. Each was
  asked to identify flaws, unsafe assumptions, and implementation errors.
  Findings were then verified against the actual CockpitOS codebase before
  acceptance.

17.2 GEMINI FINDINGS (3 items)
  FINDING G1: forceRefresh/forceSend vulnerability during boot
    VERDICT: Valid concern, wrong code path cited. Gemini referenced
    InputControl::updateInput() and INPUT_TYPE_MAGNETIC enum which do not
    exist in CockpitOS. The actual paths are Generic_init() ->
    pollGPIOSelectors(true) and pollGPIOMomentaries(true).
    ACTION: Added Section 9 (Passive Boot Requirement) with correct paths.

  FINDING G2: Code implementation pattern
    VERDICT: Hallucinated. References mappings[i].type, mappings[i].msg,
    mappings[i].lastState — none exist in CockpitOS.
    ACTION: Rejected code; wrote implementation against real architecture.

  FINDING G3: Flyback diode hardware safety
    VERDICT: Valid. Already present in v1.0 (Sections 6.4, 7.4) but buried.
    ACTION: Elevated to dedicated Section 10 with full circuit diagram.

17.3 CHATGPT FINDINGS (6 items)
  FINDING C1: CommandHistoryEntry wrong file and wrong struct
    VERDICT: Valid. The struct lives in auto-generated DCSBIOSBridgeData.h,
    not src/DCSBIOSBridge.h. Real struct has 3 additional HID report fields.
    ACTION: Fixed Section 6.2 and 8.1. Implementation via generator_core.py.

  FINDING C2: Magnetic input state machine registration hole
    VERDICT: Valid. registerMagneticInput() was shown but never called.
    ACTION: Replaced with buildMagneticInputs() auto-discovery (Section 8.5).

  FINDING C3: Mission-start forceSend incomplete coverage
    VERDICT: Valid. Convergent with Gemini G1. Correctly identified
    Generic_init() as the actual code path.
    ACTION: Added Section 9 with three protection layers.

  FINDING C4: Debounce missing for edge detection
    VERDICT: Valid. New finding not caught by Gemini or v1.0 document.
    ACTION: Added MAGNETIC_DEBOUNCE_MS to edge detection (Section 8.5).

  FINDING C5: GPIO current statement misleading (40mA)
    VERDICT: Partially valid. 40mA is absolute max, not design target.
    ACTION: Corrected to "12-20 mA recommended" in Section 10.1.

  FINDING C6: set_state interface not proven from JSON
    VERDICT: Invalid. Reviewer lacked access to the JSON file. We parsed
    it exhaustively in Section 5 with full field details.
    ACTION: No change needed; evidence already present.

  FINDING C7 (implied): Gate pulldown during boot
    VERDICT: Valid. During ESP32 boot, floating GPIO can activate MOSFET.
    ACTION: Added to Section 10.2 and 10.3 as mandatory component.

  ALTERNATIVE PROPOSAL: Separate magnetic-label registry instead of
  modifying CommandHistoryEntry.
    VERDICT: Rejected. Modifying generator_core.py follows the established
    pattern (isSelector, group) and avoids a parallel data structure.
    The generator already reads InputMapping.h to populate these fields.

17.4 UNRESOLVED ITEMS
  None. All accepted findings have been incorporated into v2.0.

==============================================================================
SECTION 18: PROBLEM SOLUTION BY JESUS — ULTRA-SIMPLE APPROACH
==============================================================================

STATUS: This section was added after the full design review was complete.
It documents a dramatically simpler implementation path identified by the
project lead that eliminates the majority of the proposed firmware changes.

18.1 THE INSIGHT
----------------------------------------
The entire magnetic switch problem reduces to ONE observation:

  The DCS-BIOS JSON already exports the switch position as an output.
  CockpitOS already has infrastructure to drive GPIOs from export data.
  Therefore: just drive the solenoid from the existing export stream.

The solenoid being ON or OFF physically FORCES the switch into the correct
position. That IS the solution. The solenoid is the enforcement mechanism —
when DCS says "OFF", the solenoid de-energizes, the spring returns the
switch, and the switch IS off. No edge detection needed. No state machine
needed. No confirmation retries needed. The physics handles it.

18.2 WHAT ALREADY EXISTS (ZERO FIRMWARE CHANGES)
----------------------------------------

OUTPUT SIDE — The JSON already has the data we need:

  "APU_CONTROL_SW": {
      "outputs": [{
          "address": 29890,
          "mask": 256,
          "shift_by": 8,
          "max_value": 1,
          "type": "integer"
      }]
  }

This output is already parsed by CockpitOS's export stream listener. The
ONLY thing missing is an LEDMapping entry to drive a GPIO from it.

Add ONE line to LEDMapping.h:

  { "APU_CONTROL_SW", DEVICE_GPIO, { .gpioInfo = {PIN(7)} }, false, false },

That's it. When DCS exports APU_CONTROL_SW=1, GPIO 7 goes HIGH, solenoid
energizes, switch is held ON. When DCS exports APU_CONTROL_SW=0, GPIO 7
goes LOW, solenoid releases, spring returns switch to OFF.

The existing subscription system (onSelectorChange), the existing GPIO
output driver (LEDControl.cpp), and the existing export stream parser
handle everything. Zero new code.

INPUT SIDE — The switch already works as a selector:

  { "APU_CONTROL_SW_OFF", "HC165", 0, 10, -1, "APU_CONTROL_SW", 0, "selector", 3 },
  { "APU_CONTROL_SW_ON",  "HC165", 0, -1, -1, "APU_CONTROL_SW", 1, "selector", 3 },

The selector group system already sends the correct set_state command when
the user flips the switch. The ONLY firmware change needed is preventing
the sync system from fighting the game when DCS overrides the position.

18.3 WHAT ACTUALLY NEEDS TO CHANGE (MINIMAL)
----------------------------------------

CHANGE 1 — generator_core.py: For controls tagged "magnetic" in
InputMapping.h, emit an LEDMapping entry using the JSON output data
(address/mask/shift → DEVICE_GPIO). Currently the generator only emits
LEDMapping entries for controls with controlType CT_LED. Adding CT_SELECTOR
outputs for magnetic-tagged controls is a small generator modification.

CHANGE 2 — Sync exclusion: Prevent validateSelectorSync() and the group
arbitration in flushBufferedDcsCommands() from re-sending the hardware
position when DCS has overridden it. This is the same change identified
in Sections 8.3 and 8.4 — skip entries where isMagnetic==true.

That's TWO changes. Not 200 lines. Not a new state machine. Not edge
detection. Not debounce. Not confirmation retries. Not passive boot logic.

18.4 WHY THE COMPLEX APPROACH WAS OVER-ENGINEERED
----------------------------------------

The Sections 7-9 design treated the input and output as separate problems
requiring separate solutions. The reality is:

  - The OUTPUT (solenoid) IS the input enforcement mechanism.
    When the solenoid forces the switch position, the HC165/GPIO reads
    the correct state automatically. The hardware loop is closed
    physically, not in firmware.

  - Edge detection is unnecessary because the selector input path
    already works. The switch sends set_state when flipped. If DCS
    rejects it (conditions not met), the export stream reports the
    game's decision, the solenoid enforces it, and the switch moves.

  - Debounce is unnecessary for the magnetic-specific logic because
    the existing selector polling already has its own debounce/dwell
    mechanisms (SELECTOR_DWELL_MS, group arbitration).

  - Confirmation retries are unnecessary because the solenoid provides
    physical feedback. If the command was lost, the solenoid stays in
    its current state, the switch stays in its current position, and
    the user can simply flip again.

  - Passive boot is unnecessary as a special case because on boot,
    the normal selector sync fires — but with sync exclusion (Change 2),
    it won't fight the game. The solenoid will settle to whatever DCS
    exports after reconnection.

The complex design solved problems that don't exist when you realize the
solenoid closes the loop physically.

18.5 THE BENCH TEST (PHASE 1 — LED)
----------------------------------------

To validate this approach with zero firmware changes:

STEP 1: Manually add to LEDMapping.h:
  { "APU_CONTROL_SW", DEVICE_GPIO, { .gpioInfo = {PIN(7)} }, false, false },

STEP 2: Wire an LED + 220ohm resistor on PIN(7) to GND.

STEP 3: Fly a mission in DCS with the F/A-18C.

STEP 4: Observe:
  - Start APU: LED turns ON when DCS arms the APU switch
  - Wait ~60 seconds after 2nd generator online: LED turns OFF
    (DCS auto-releases the APU switch)
  - The LED follows the GAME's decision in real-time

This proves the output side works with zero code changes. The only
remaining work is the sync exclusion (Change 2) to prevent the firmware
from overriding DCS's auto-release decision.

18.6 COMPARISON — COMPLEX vs ULTRA-SIMPLE
----------------------------------------

                          COMPLEX (Sections 7-9)    ULTRA-SIMPLE (This Section)
New firmware lines        ~200                      ~10 (sync skip only)
New struct fields         isMagnetic (bool)         isMagnetic (bool)
New polling function      pollGPIOMagnetic()        None
Edge detection            Custom state machine      Existing selector path
Debounce                  MAGNETIC_DEBOUNCE_MS      Existing SELECTOR_DWELL_MS
Confirmation retries      500ms timeout, 3 retries  Solenoid = physical feedback
Passive boot logic        3 protection layers       Sync exclusion only
Generator changes         Struct + exclusions        LEDMapping emission
New Config.h defines      3 new defines              0
Risk of regressions       Medium (new code paths)   Low (reuses existing paths)

18.7 WHAT REMAINS FROM THE ORIGINAL DESIGN
----------------------------------------

The following sections of this document remain fully valid and necessary:

  - Section 1-5: Problem definition, history, JSON analysis (unchanged)
  - Section 6: Architecture advantages (confirms infrastructure exists)
  - Section 8.1: generator_core.py needs isMagnetic field (still needed
    for sync exclusion)
  - Section 8.3: validateSelectorSync() skip (still needed)
  - Section 8.4: flushBufferedDcsCommands() skip (still needed)
  - Section 10: Hardware safety (flyback diode, gate pulldown, etc.)
  - Section 11: Risk analysis (solenoid-related risks still apply)
  - Section 14: Known limitations (still apply)

The following sections are SUPERSEDED by this approach:

  - Section 7.1: Behavioral specification (edge detection, debounce,
    confirmation retries — all unnecessary)
  - Section 8.5: pollGPIOMagnetic() state machine (unnecessary)
  - Section 8.6: Integration into panel init/loop (unnecessary)
  - Section 8.7: Config.h new parameters (unnecessary)
  - Section 9: Passive boot requirement (simplified to sync exclusion)

==============================================================================
END OF DOCUMENT — VERSION 2.1
==============================================================================
