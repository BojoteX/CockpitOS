# How To: Create Custom Panels

Custom panels let you write your own C++ logic for hardware that does not fit the standard input/output model. Use cases include complex state machines (like the Wing Fold handle), special timing requirements, hardware with non-standard protocols, or any situation where the generic GPIO/PCA/HC165 scanning in `Generic.cpp` is not sufficient.

---

## When You Need a Custom Panel

The standard `Generic.cpp` panel handles most hardware automatically through InputMapping.h and LEDMapping.h configuration. You only need a custom panel when:

- A physical control has **complex state logic** (e.g., two-axis interaction like the Wing Fold)
- You need **custom timing** or sequencing between commands
- You are driving **non-standard hardware** (custom protocols, unusual ICs)
- You want to **subscribe to DCS-BIOS data** and perform custom processing
- You need a **state machine** that cannot be expressed with simple selector/momentary mappings

---

## Step 1: Create the Panel File

Create a new `.cpp` file in `src/Panels/`. Use the naming convention `YourPanel.cpp`:

```
src/Panels/MyPanel.cpp
```

### Panel File Template

```cpp
// MyPanel.cpp -- Custom panel for [describe your hardware]

// PANEL_KIND: MyPanel

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

// Guard: only compile when the matching label set is active
#if defined(HAS_MY_LABEL_SET)

#include "includes/MyPanel.h"  // optional: separate header for declarations

// Register this panel with the CockpitOS panel system
REGISTER_PANEL(MyPanel, MyPanel_init, MyPanel_loop, nullptr, nullptr, nullptr, 100);

// ---- State Variables ----
static uint16_t myValue = 0;
static bool     myFlag  = false;

// ---- DCS-BIOS Callbacks ----
static void onMyDataChange(const char* label, uint16_t value, uint16_t maxValue) {
    myValue = value;
    myFlag  = true;
}

// ---- Lifecycle: Init ----
// Called once when a mission starts (and on re-sync)
void MyPanel_init() {
    // Subscribe to DCS-BIOS exports
    subscribeToLedChange("MY_DCS_LABEL", onMyDataChange);

    // Initialize hardware
    pinMode(PIN(12), OUTPUT);
    digitalWrite(PIN(12), LOW);

    debugPrintln("MyPanel initialized");
}

// ---- Lifecycle: Loop ----
// Called every frame (~250Hz). NEVER block in this function.
void MyPanel_loop() {
    // Read inputs, update state, send commands
    if (myFlag) {
        myFlag = false;
        // React to DCS-BIOS data change
        digitalWrite(PIN(12), myValue > 0 ? HIGH : LOW);
    }

    // Send input commands to DCS
    // HIDManager_setNamedButton("MY_BUTTON", false, digitalRead(PIN(5)) == LOW);
}

#endif // HAS_MY_LABEL_SET
```

---

## Step 2: Understand the REGISTER_PANEL Macro

The `REGISTER_PANEL` macro registers your panel's lifecycle functions with the CockpitOS panel registry:

```cpp
REGISTER_PANEL(KIND, INIT, LOOP, DISP_INIT, DISP_LOOP, TICK, PRIORITY);
```

| Argument | Purpose | Notes |
|---|---|---|
| `KIND` | PanelKind enum value | Must match a value in the auto-generated `PanelKind.h` |
| `INIT` | Init function | Called when a mission starts. Pass `nullptr` if not needed. |
| `LOOP` | Loop function | Called every frame (~250Hz). Pass `nullptr` if not needed. |
| `DISP_INIT` | Display init function | For display-heavy panels. Pass `nullptr` if not needed. |
| `DISP_LOOP` | Display loop function | For display rendering. Pass `nullptr` if not needed. |
| `TICK` | Tick function | Optional per-frame work. Pass `nullptr` if not needed. |
| `PRIORITY` | Execution priority | Lower values run earlier. Default is 100. |

### Typical Registration Patterns

```cpp
// Standard input panel
REGISTER_PANEL(MyPanel, MyPanel_init, MyPanel_loop, nullptr, nullptr, nullptr, 100);

// Display panel (like IFEI)
REGISTER_PANEL(IFEI, IFEI_init, nullptr, IFEIDisplay_init, IFEIDisplay_loop, nullptr, 100);

// TFT gauge (display-only)
REGISTER_PANEL(TFTBatt, nullptr, nullptr, BatteryGauge_init, BatteryGauge_loop, nullptr, 100);
```

---

## Step 3: PanelKind Enum

Your panel needs an entry in the `PanelKind` enum. This is **auto-generated** by the `generate_panelkind.py` script, which scans all `.cpp` files in `src/Panels/`.

### How PanelKind Names Are Determined

1. If your `.cpp` file contains a `// PANEL_KIND: YourName` comment, that name is used
2. Otherwise, the filename (minus `.cpp`) is used

For example:

```cpp
// In MyPanel.cpp:
// PANEL_KIND: MyPanel    <-- This sets PanelKind::MyPanel
```

The Compiler Tool runs `generate_panelkind.py` automatically before each build, so you do not need to manually edit `PanelKind.h`.

---

## Step 4: The Compile Guard

Every custom panel must be wrapped in a `#if defined(HAS_...)` guard that matches its label set. This ensures the panel code only compiles when the corresponding label set is active.

```cpp
#if defined(HAS_CUSTOM_RIGHT)
// ... panel code ...
#endif // HAS_CUSTOM_RIGHT
```

The `HAS_*` macro is defined in the label set's `LabelSetConfig.h` file. For example:

```cpp
// In src/LABELS/LABEL_SET_CUSTOM_RIGHT/LabelSetConfig.h
#define HAS_CUSTOM_RIGHT
```

---

## Step 5: Panel Lifecycle

### Init

The `init` function is called **each time a mission starts** (not just once at boot). It is also called during re-sync. Use it to:

- Subscribe to DCS-BIOS exports
- Read initial hardware state
- Send initial commands to DCS
- Fire all selectors and switches to sync state

```cpp
void MyPanel_init() {
    // This runs at mission start AND on re-sync
    subscribeToLedChange("VOLT_U", onVoltageChange);
    pollInitialSwitchStates(true);  // true = forceSend
}
```

### Loop

The `loop` function is called **every frame at the polling rate** (~250Hz by default). Rules:

- **Never block** -- no `delay()`, no `while(true)`, no waiting for I/O
- **Keep it fast** -- target under 1ms execution time
- **Use timing guards** for slow operations:

```cpp
void MyPanel_loop() {
    static unsigned long lastPoll = 0;
    unsigned long now = millis();
    if (now - lastPoll < 4) return;  // 250Hz limit
    lastPoll = now;

    // Your fast, non-blocking logic here
}
```

---

## Step 6: DCS-BIOS Subscriptions

Subscribe to DCS-BIOS data in your `init` function:

### Numeric Values (LEDs, Gauges, Dimmers)

```cpp
// Callback receives: label, raw 16-bit value, max value
static void onValueChange(const char* label, uint16_t value, uint16_t maxValue) {
    int percent = (value * 100) / maxValue;
    // Use the value
}

void MyPanel_init() {
    subscribeToLedChange("FUEL_QTY_L", onValueChange);
}
```

### Selector Changes

```cpp
// Callback receives: label, selector position
static void onSelectorChange(const char* label, uint16_t position) {
    // React to selector position change
}

void MyPanel_init() {
    subscribeToSelectorChange("LANDING_GEAR_HANDLE", onSelectorChange);
}
```

### Metadata / String Changes

```cpp
// Callback receives: label, raw 16-bit value
static void onMetadataChange(const char* label, uint16_t value) {
    // Process raw value
}

void MyPanel_init() {
    subscribeToMetadataChange("EXT_NOZZLE_POS_L", onMetadataChange);
}
```

---

## Step 7: Sending Commands to DCS

Use the HIDManager API to send button presses and axis values:

```cpp
// Set a named button state (maps to DCS-BIOS command)
HIDManager_setNamedButton("MY_BUTTON", forceSend, value);
// forceSend = true on init, false during normal operation
// value = true (pressed) or false (released)

// Move an analog axis
HIDManager_moveAxis("MY_AXIS", gpioPin, axisIndex, isInit, forceSend);
```

---

## Step 8: Reference Examples

Study these existing panels for patterns:

| File | Complexity | What It Demonstrates |
|---|---|---|
| `Generic.cpp` | Standard | GPIO, HC165, PCA9555, analog, TM1637, and matrix scanning -- the default input handler |
| `IFEIPanel.cpp` | Complex | HT1622 segment LCD rendering, backlight control, DCS-BIOS string subscriptions, overlay logic |
| `WingFold.cpp` | Complex | Custom state machine for a two-axis switch, command queuing with timing, PCA9555 raw reads |
| `TFT_Gauges_Battery.cpp` | Display | FreeRTOS task, LovyanGFX rendering, dirty-rect DMA, PSRAM asset management |

---

## Best Practices

1. **Never block in `loop()`** -- use `millis()` timing instead of `delay()`
2. **Do all allocations in `init()`** -- avoid `malloc`/`new` in `loop()`
3. **Use CockpitOS utilities** -- `debugPrintf()`, `readPCA9555()`, `HIDManager_*` -- not raw Arduino calls
4. **Keep subscriptions in `init()`** -- subscribe once, not every frame
5. **Guard with `isMissionRunning()`** -- do not process DCS data when no mission is active
6. **Use `IS_REPLAY` mode for testing** -- set `IS_REPLAY 1` in Config.h to test your panel without DCS running
7. **Add `// PANEL_KIND: YourName`** as a comment in your `.cpp` file for clean PanelKind naming

---

## Further Reading

- [Advanced Custom Panels](../Advanced/Custom-Panels.md) -- Deep dive into the REGISTER_PANEL system, PanelKind, and lifecycle details
- [FreeRTOS Tasks](../Advanced/FreeRTOS-Tasks.md) -- Task management for TFT gauges and background processing
- [Config Reference](../Reference/Config.md) -- All Config.h settings

---

*See also: [Hardware Overview](../Hardware/README.md) | [Control Types Reference](../Reference/Control-Types.md) | [DCS-BIOS Reference](../Reference/DCS-BIOS.md)*
