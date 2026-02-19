# Latched Buttons

Deep dive into the CockpitOS latched button system. This document explains how physical momentary pushbuttons are converted into toggle (latched) switches in both HID and DCS-BIOS modes, covering the rising-edge detection, command history tracking, toggle logic, and group exclusion behavior.

---

## 1. What Are Latched Buttons?

In a physical cockpit, many switches are toggle (latched) switches -- push once to turn ON, push again to turn OFF. In a home cockpit, you often wire these as momentary pushbuttons because real toggle switches are expensive or unavailable for certain functions.

The latched button system converts momentary press/release events into toggle state changes. When the physical button is pressed (rising edge), the system flips the logical state. When the button is released, nothing happens. The result: press once = ON, press again = OFF, exactly like a real toggle switch.

### Key Properties

- **Rising-edge only** -- state changes happen on the press-down event. Releasing the button has no effect.
- **Dual-pipe toggle** -- in DCS mode, the toggle sends both a DCS-BIOS command and updates the HID button state. In HID-only mode, only the HID report is updated.
- **Command history** -- the current ON/OFF state is tracked in the DCS-BIOS command history table, providing a single source of truth across subsystems.
- **CoverGate compatible** -- latched buttons work with the CoverGate system for guarded switch covers.

---

## 2. Configuration

### Declaring Latched Buttons

Latched buttons are declared in the Label Creator-generated `Mappings.cpp` file:

```cpp
const char* kLatchedButtons[] = {
    "FIRE_EXT_BTN",
    "MASTER_ARM_SW",
    "HOOK_BYPASS_SW",
    // ... more labels ...
};
const unsigned kLatchedButtonCount = sizeof(kLatchedButtons) / sizeof(kLatchedButtons[0]);
```

The `kLatchedButtons[]` array and `kLatchedButtonCount` are declared as externs in `Mappings.h`:

```cpp
#define MAX_LATCHED_BUTTONS 16

extern const char* kLatchedButtons[];
extern const unsigned kLatchedButtonCount;

bool isLatchedButton(const char* label);
```

### isLatchedButton()

The `isLatchedButton()` function performs a linear scan of `kLatchedButtons[]` to determine if a given label is a latched button:

```cpp
bool isLatchedButton(const char* label) {
    for (unsigned i = 0; i < kLatchedButtonCount; i++) {
        if (strcmp(label, kLatchedButtons[i]) == 0) return true;
    }
    return false;
}
```

This function is called from `HIDManager_setNamedButton()` at decision time for every button event. The linear scan is acceptable because `MAX_LATCHED_BUTTONS` is 16 and the check only involves short string comparisons.

---

## 3. Rising-Edge Detection

### HIDManager_toggleIfPressed()

The core rising-edge detection function:

```cpp
void HIDManager_toggleIfPressed(bool isPressed, const char* label, bool deferSend) {
    CommandHistoryEntry* e = findCmdEntry(label);
    if (!e) return;

    static std::array<bool, MAX_TRACKED_RECORDS> lastStates = { false };
    int index = (int)(e - dcsbios_getCommandHistory());
    if (index < 0 || index >= (int)MAX_TRACKED_RECORDS) return;

    bool prev = lastStates[index];
    lastStates[index] = isPressed;

    if (isPressed && !prev) {
        HIDManager_setToggleNamedButton(label, deferSend);
    }
}
```

**How it works:**

1. Look up the `CommandHistoryEntry` for this label in the DCS-BIOS command history.
2. Use the entry's index to access a static `lastStates` array that tracks the previous physical state (pressed/released) of each button.
3. Compare current state against previous state:
   - If `isPressed == true` and `prev == false`, this is a rising edge. Call `HIDManager_setToggleNamedButton()`.
   - All other transitions (release, held, or already pressed) are ignored.

The `lastStates` array is indexed by the command history entry position, ensuring each latched button has its own independent state tracking.

---

## 4. Toggle Logic

### HIDManager_setToggleNamedButton()

The actual toggle function:

```cpp
void HIDManager_setToggleNamedButton(const char* name, bool deferSend) {
    const InputMapping* m = findInputByLabel(name);
    if (!m) return;

    CommandHistoryEntry* e = findCmdEntry(name);
    if (!e) return;

    // Toggle state
    const bool curOn = (e->lastValue != 0xFFFF) && (e->lastValue > 0);
    const bool newOn = !curOn;
    e->lastValue = newOn ? 1 : 0;

    // DCS path
    if (dcsAllowed && m->oride_label) {
        sendDCSBIOSCommand(m->oride_label, newOn ? m->oride_value : 0, force);
    }

    // HID path
    if (!hidAllowed || m->hidId <= 0) return;
    const uint32_t mask = (1UL << (m->hidId - 1));
    if (m->group > 0 && newOn) report.buttons &= ~groupBitmask[m->group];
    if (newOn) report.buttons |= mask;
    else       report.buttons &= ~mask;
    if (!deferSend) HIDManager_dispatchReport(false);
}
```

**Toggle steps:**

1. **Read current state** from `e->lastValue`. A value of `0xFFFF` (uninitialized) or `0` means OFF. Any value > 0 means ON.
2. **Flip the state** -- `newOn = !curOn`.
3. **Write back** -- update `e->lastValue` to 1 (ON) or 0 (OFF). This is the source of truth for all subsystems.
4. **DCS path** -- if DCS mode is active and the mapping has an `oride_label`, send a DCS-BIOS command with the appropriate value (`oride_value` for ON, `0` for OFF).
5. **HID path** -- if HID mode is active and the mapping has a valid `hidId`, update the button bitmask in the HID report. Group exclusion is applied if applicable (see Section 5).

---

## 5. Group Exclusion

When a latched button belongs to an HID button group (`m->group > 0`), toggling it ON first clears all other buttons in the same group:

```cpp
if (m->group > 0 && newOn) report.buttons &= ~groupBitmask[m->group];
```

This implements radio-button behavior -- within a group, only one button can be ON at a time. When you toggle button A ON, buttons B, C, and D in the same group are cleared.

Toggling OFF does not affect the group -- it only clears the single button.

---

## 6. Decision Flow in HIDManager_setNamedButton()

The `HIDManager_setNamedButton()` function is the main entry point for all button events. Latched buttons are detected and routed at two decision points:

### HID-Only Mode (DCS not connected)

```
HIDManager_setNamedButton(name, pressed)
   |
   +-- isModeSelectorDCS() == false
   |     |
   |     +-- isLatchedButton(name)?
   |           YES: HIDManager_toggleIfPressed(pressed, name)
   |                 (rising-edge toggle, HID report only)
   |                 return;
   |           NO:  Normal momentary HID processing
```

### DCS Mode (DCS connected)

```
HIDManager_setNamedButton(name, pressed)
   |
   +-- isModeSelectorDCS() == true
   |     |
   |     +-- CoverGate_intercept(name, pressed)
   |     |     If handled: skip DCS path for this button
   |     |
   |     +-- Not a variable_step or fixed_step?
   |     |     |
   |     |     +-- isLatchedButton(name)?
   |     |           YES: HIDManager_toggleIfPressed(pressed, name)
   |     |                 (rising-edge toggle, DCS + HID dual-pipe)
   |     |                 return;
   |     |           NO:  Normal DCS command processing
   |     |
   |     +-- HID path runs after DCS (if applicable)
```

### Key Difference

- **HID-only mode** -- latched button check happens first, before any other processing.
- **DCS mode** -- CoverGate intercept runs first (guarded switches take priority), then latched button detection happens within the DCS command path.

---

## 7. Command History as Source of Truth

The `CommandHistoryEntry.lastValue` field serves as the single source of truth for toggle state:

| `lastValue` | Meaning |
|------------|---------|
| `0xFFFF` | Uninitialized (treated as OFF) |
| `0` | OFF |
| `> 0` | ON |

This value is read and written by multiple subsystems:

- **HIDManager_setToggleNamedButton()** -- flips the value on each toggle.
- **CoverGate** -- reads via `cg_isLatchedOn()` to check if a button is already ON before toggling.
- **DCS-BIOS bridge** -- may update `lastValue` when receiving state from DCS World (e.g., after aircraft switch or mission restart).

The command history table uses O(1) hash-based lookup via `findCmdEntry()`, so checking and updating toggle state adds no measurable latency.

---

## 8. Interaction with CoverGate

When a latched button is also guarded by a CoverGate (e.g., fire extinguisher), the CoverGate system handles the entire sequence:

1. CoverGate opens the cover.
2. After `delay_ms`, CoverGate calls `HIDManager_setToggleNamedButton()` to toggle the button.
3. The toggle function checks `lastValue` and only toggles if needed.
4. After `close_delay_ms`, CoverGate closes the cover.

The `isLatchedButton()` check in `HIDManager_setNamedButton()` never fires for CoverGate-intercepted events because `CoverGate_intercept()` returns `true` first, short-circuiting the normal button processing path. The CoverGate system calls `HIDManager_setToggleNamedButton()` directly with its `s_reentry` flag set, bypassing the intercept.

---

## 9. Timing Diagram

```
Physical Button:  _____|‾‾‾‾‾‾‾‾|__________|‾‾‾‾‾‾‾‾|__________
                       ^                    ^
                       |                    |
                  Rising Edge          Rising Edge
                  (toggle ON)          (toggle OFF)

lastValue:        0 -> 1                1 -> 0

HID Report:       bit OFF -> ON         bit ON -> OFF

DCS Command:      oride_value -> 1      oride_value -> 0
```

Each rising edge flips the state. The falling edge (release) is always ignored by `HIDManager_toggleIfPressed()`.

---

## See Also

- [Advanced/CoverGate.md](CoverGate.md) -- Guarded switch cover system
- [Advanced/Custom-Panels.md](Custom-Panels.md) -- HIDManager API reference
- [Reference/Config.md](../Reference/Config.md) -- Configuration constants
