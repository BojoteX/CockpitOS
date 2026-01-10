# CockpitOS — Advanced Controls Guide

This guide covers advanced topics for users who want to create custom panels, implement covered controls, or understand the panel registration system.

---

## Table of Contents

1. [Creating Custom Panels](#1-creating-custom-panels)
2. [The REGISTER_PANEL Macro](#2-the-register_panel-macro)
3. [PanelKind and Auto-Generation](#3-panelkind-and-auto-generation)
4. [Reference Panel Implementations](#4-reference-panel-implementations)
5. [CoverGate System](#5-covergate-system)
6. [Latched Buttons](#6-latched-buttons)
7. [Panel Lifecycle](#7-panel-lifecycle)
8. [Best Practices](#8-best-practices)
9. [METADATA Directory (Advanced)](#9-metadata-directory-advanced)

---

## 1. Creating Custom Panels

Custom panels let you add specialized logic beyond what InputMapping and LEDMapping provide. Use them when you need:

- Complex state machines (e.g., WingFold's push/pull + fold/spread logic)
- Custom timing or sequencing
- Hardware that doesn't fit the standard GPIO/PCA/HC165 model
- Display rendering with DCS-BIOS subscriptions

### 1.1 Basic Panel Structure

Every panel file follows this template:

```cpp
// MyPanel.cpp

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

// Only compile if this label set includes your panel
#if defined(HAS_MY_PANEL)
REGISTER_PANEL(MyPanel, MyPanel_init, MyPanel_loop, nullptr, nullptr, nullptr, 100);
#endif

void MyPanel_init() {
    // Called on mission start
    // Initialize hardware, sync state with DCS
}

void MyPanel_loop() {
    // Called every frame (~250 Hz)
    // Poll inputs, update outputs
}
```

### 1.2 Where to Put Your Panel

```
src/Panels/
├── MyPanel.cpp          ← Your new panel
├── includes/
│   └── MyPanel.h        ← Header (optional, for complex panels)
├── Generic.cpp          ← Handles GPIO, HC165, PCA9555 automatically
├── IFEIPanel.cpp        ← Reference: complex display panel
├── WingFold.cpp         ← Reference: state machine panel
├── LockShoot.cpp        ← Reference: simple LED-only panel
└── ...
```

### 1.3 Enabling Your Panel

In your Label Set's generated header, define the activation flag:

```cpp
// In your label set or Config.h
#define HAS_MY_PANEL 1
```

The `#if defined(HAS_MY_PANEL)` guard ensures your panel only compiles when enabled.

---

## 2. The REGISTER_PANEL Macro

The `REGISTER_PANEL` macro registers your panel with CockpitOS's automatic lifecycle management.

### 2.1 Syntax

```cpp
REGISTER_PANEL(KIND, INIT, LOOP, DISP_INIT, DISP_LOOP, TICK, PRIORITY);
```

| Parameter | Type | Description |
|-----------|------|-------------|
| `KIND` | Identifier | Must match a `PanelKind` enum entry (see Section 3) |
| `INIT` | `void (*)()` | Called on mission start. Use `nullptr` if not needed |
| `LOOP` | `void (*)()` | Called every frame. Use `nullptr` if not needed |
| `DISP_INIT` | `void (*)()` | Display initialization. Use `nullptr` if not needed |
| `DISP_LOOP` | `void (*)()` | Display update loop. Use `nullptr` if not needed |
| `TICK` | `void (*)()` | Per-frame tick (for LED drivers, etc.). Use `nullptr` if not needed |
| `PRIORITY` | `uint8_t` | Lower runs first. Default is `100` |

### 2.2 Examples

**Simple panel (inputs only):**
```cpp
REGISTER_PANEL(MyPanel, MyPanel_init, MyPanel_loop, nullptr, nullptr, nullptr, 100);
```

**Display panel (no input loop):**
```cpp
REGISTER_PANEL(TFTCmws, nullptr, nullptr, CMWSDisplay_init, CMWSDisplay_loop, nullptr, 100);
```

**LED-only panel (just needs tick):**
```cpp
REGISTER_PANEL(LockShoot, nullptr, nullptr, nullptr, nullptr, WS2812_tick, 100);
```

**Annunciator (TM1637 tick):**
```cpp
REGISTER_PANEL(LA, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
```

---

## 3. PanelKind and Auto-Generation

### 3.1 How It Works

PanelKind is now **auto-generated** from panel `.cpp` files. The generator scans `src/Panels/*.cpp` and creates `src/Generated/PanelKind.h`.

**The identifier is determined by:**
1. If the file contains `// PANEL_KIND: XXX` in the first 30 lines → use `XXX`
2. Otherwise → use the filename without `.cpp`

### 3.2 Adding PANEL_KIND Metadata

For existing panels that need a specific identifier different from the filename:

```cpp
// CautionAdvisory.cpp - Caution Advisory panel
// PANEL_KIND: CA

#include "../Globals.h"
// ...
```

This ensures `CautionAdvisory.cpp` generates `PanelKind::CA` instead of `PanelKind::CautionAdvisory`.

### 3.3 Files That Need Metadata

| File | Required Metadata | Result |
|------|-------------------|--------|
| `CautionAdvisory.cpp` | `// PANEL_KIND: CA` | `PanelKind::CA` |
| `LeftAnnunciator.cpp` | `// PANEL_KIND: LA` | `PanelKind::LA` |
| `RightAnnunciator.cpp` | `// PANEL_KIND: RA` | `PanelKind::RA` |
| `IFEIPanel.cpp` | `// PANEL_KIND: IFEI` | `PanelKind::IFEI` |
| `ECMPanel.cpp` | `// PANEL_KIND: ECM` | `PanelKind::ECM` |
| `TFT_Display_CMWS.cpp` | `// PANEL_KIND: TFTCmws` | `PanelKind::TFTCmws` |
| `TFT_Gauges_Battery.cpp` | `// PANEL_KIND: TFTBatt` | `PanelKind::TFTBatt` |
| `Generic.cpp` | *(none needed)* | `PanelKind::Generic` |
| `WingFold.cpp` | *(none needed)* | `PanelKind::WingFold` |
| `LockShoot.cpp` | *(none needed)* | `PanelKind::LockShoot` |

### 3.4 Running the Generator

```bash
cd CockpitOS
python generate_panelkind.py
```

Output:
```
Scanning 25 panel files...
  CautionAdvisory.cpp                    → CA                   (from PANEL_KIND)
  Generic.cpp                            → Generic              (from filename)
  IFEIPanel.cpp                          → IFEI                 (from PANEL_KIND)
  ...

Generated: src/Generated/PanelKind.h
  → 25 panel entries + COUNT
```

### 3.5 Generated File Location

```
src/Generated/PanelKind.h
```

This file is included by `Mappings.h`:

```cpp
// Mappings.h
#pragma once
#include "src/Generated/PanelKind.h"  // Auto-generated enum
```

### 3.6 Adding a New Panel (No Manual Enum Editing!)

1. Create `src/Panels/MyPanel.cpp`
2. Use `REGISTER_PANEL(MyPanel, ...)` — the identifier matches the filename
3. Run `python generate_panelkind.py`
4. Done! `PanelKind::MyPanel` is automatically created

**Or with a custom identifier:**

1. Create `src/Panels/MyAwesomePanel.cpp`
2. Add `// PANEL_KIND: AwesomePanel` in the first 30 lines
3. Use `REGISTER_PANEL(AwesomePanel, ...)`
4. Run the generator
5. Done! `PanelKind::AwesomePanel` is created

### 3.7 Integration with Label Set Generator

The PanelKind generator runs automatically at the end of `generate_data.py`:

```python
# End of generate_data.py
import subprocess, sys, os
root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
subprocess.run([sys.executable, os.path.join(root, "generate_panelkind.py"), root])
```

---

## 4. Reference Panel Implementations

Study these existing panels to understand different patterns:

### 4.1 Simple LED Panel: LockShoot.cpp

The simplest panel — just registers a tick function for WS2812 LEDs:

```cpp
#if defined(HAS_MAIN)
    REGISTER_PANEL(LockShoot, nullptr, nullptr, nullptr, nullptr, WS2812_tick, 100);
#endif
```

**Key points:**
- No init or loop — LEDs are driven by LEDMapping callbacks
- Only needs `tick` to flush WS2812 updates
- Hardware configuration is in LEDMapping, not panel code

### 4.2 Annunciator Panel: LeftAnnunciator.cpp / RightAnnunciator.cpp

TM1637-based indicator panels:

```cpp
#if defined(HAS_MAIN)
    REGISTER_PANEL(LA, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
#endif
```

**Key points:**
- Similar to LockShoot — just needs tick for TM1637 driver
- LED states come from LEDMapping subscriptions
- Hardware pins defined in `Pins.h`

### 4.3 GN1640T Panel: CautionAdvisory.cpp

Matrix LED driver for caution/advisory lights:

```cpp
#if defined(HAS_CUSTOM_RIGHT)
    REGISTER_PANEL(CA, nullptr, nullptr, nullptr, nullptr, GN1640_tick, 100);
#endif
```

**Key points:**
- Uses GN1640T matrix driver
- Initialization happens in `Mappings.cpp` via `GN1640_init()`
- Panel just provides the tick

### 4.4 Complex State Machine: WingFold.cpp

The most complex input panel — decodes a multi-switch wing fold mechanism:

```cpp
#if defined(HAS_CUSTOM_RIGHT)
REGISTER_PANEL(WingFold, WingFold_init, WingFold_loop, nullptr, nullptr, nullptr, 100);
#endif
```

**Key features:**
- Custom state machine with debouncing
- Reads from PCA9555 directly (not via Generic.cpp)
- Command queuing with timing constraints
- Resolves wiring from InputMappings at runtime

**Study this panel to learn:**
- How to implement custom debouncing
- How to read PCA9555 directly
- How to send commands with proper spacing
- How to resolve hardware configuration from InputMappings

### 4.5 Display Panel: IFEIPanel.cpp

The most advanced panel — HT1622 segment display with button input:

```cpp
#if defined(HAS_IFEI)
    REGISTER_PANEL(IFEI, IFEI_init, nullptr, IFEIDisplay_init, IFEIDisplay_loop, nullptr, 100);
#endif
```

**Key features:**
- Subscribes to DCS-BIOS display fields
- Renders text to HT1622 segment displays
- Manages backlight modes (green/white/NVG)
- Uses IFEIDisplay class for display abstraction

**Study this panel to learn:**
- Display subscription system
- Segment rendering
- Region-based commit optimization

### 4.6 TFT Gauge: TFT_Display_CMWS.cpp

SPI TFT display with sprite-based rendering:

```cpp
#if defined(HAS_CMWS_DISPLAY)
    REGISTER_PANEL(TFTCmws, nullptr, nullptr, CMWSDisplay_init, CMWSDisplay_loop, nullptr, 100);
#endif
```

**Key features:**
- Uses LovyanGFX for TFT rendering
- Subscribes to multiple DCS-BIOS fields
- Dirty-flag pattern for efficient redraws
- Can run as FreeRTOS task for performance

---

## 5. CoverGate System

CoverGate handles **physically guarded controls** — buttons and switches with protective covers that must open before the control can activate.

### 5.1 How It Works

When you press a covered button:

```
1. User presses FIRE button
   ↓
2. CoverGate intercepts the event
   ↓
3. Opens cover (sends COVER_LABEL = 1)
   ↓
4. Waits delay_ms (for animation)
   ↓
5. Sends actual action (FIRE_BTN toggle)
   ↓
6. Waits close_delay_ms
   ↓
7. Closes cover (sends COVER_LABEL = 0)
```

This creates realistic behavior — the virtual cover opens, the button activates, then the cover closes.

### 5.2 Configuration

Covered controls are defined in `Mappings.cpp`:

```cpp
const CoverGateDef kCoverGates[] = {
    // Guarded SELECTORS (2-position switches with covers)
    //   action_label        release_label         cover_label              kind                    open_delay  close_delay
    { "GAIN_SWITCH_POS1",   "GAIN_SWITCH_POS0",   "GAIN_SWITCH_COVER",     CoverGateKind::Selector,       500,        500 },
    { "GEN_TIE_SW_RESET",   "GEN_TIE_SW_NORM",    "GEN_TIE_COVER",         CoverGateKind::Selector,       500,        500 },
    
    // Guarded BUTTONS (momentary/latching)
    //   action_label        release_label         cover_label              kind                           open_delay  close_delay
    { "LEFT_FIRE_BTN",      nullptr,              "LEFT_FIRE_BTN_COVER",   CoverGateKind::ButtonMomentary,    350,        300 },
    { "RIGHT_FIRE_BTN",     nullptr,              "RIGHT_FIRE_BTN_COVER",  CoverGateKind::ButtonMomentary,    350,        300 },
};
```

### 5.3 CoverGateDef Structure

```cpp
struct CoverGateDef {
    const char* action_label;    // The button/selector being protected
    const char* release_label;   // Off position (selectors only, nullptr for buttons)
    const char* cover_label;     // The cover control in DCS-BIOS
    CoverGateKind kind;          // Selector, ButtonMomentary, or ButtonLatched
    uint16_t delay_ms;           // Wait after opening cover before action
    uint16_t close_delay_ms;     // Wait after action before closing cover
};
```

### 5.4 CoverGateKind Types

| Kind | Use Case | Behavior |
|------|----------|----------|
| `Selector` | 2-position guarded switch | Opens cover → waits → sends position → waits → closes cover |
| `ButtonMomentary` | Fire buttons, emergency buttons | Opens cover → waits → toggles button → waits → closes cover |
| `ButtonLatched` | Reserved for future use | — |

### 5.5 Adding a Covered Control

1. **Find the labels** in your aircraft's DCS-BIOS JSON:
   ```json
   "LEFT_FIRE_BTN": { ... },
   "LEFT_FIRE_BTN_COVER": { ... }
   ```

2. **Add to kCoverGates** in `Mappings.cpp`:
   ```cpp
   { "LEFT_FIRE_BTN", nullptr, "LEFT_FIRE_BTN_COVER", CoverGateKind::ButtonMomentary, 350, 300 },
   ```

3. **Wire your physical button** — it triggers `LEFT_FIRE_BTN`, CoverGate handles the rest

### 5.6 Timing Guidelines

| Control Type | Open Delay | Close Delay | Notes |
|--------------|------------|-------------|-------|
| Fire buttons | 300-400ms | 250-350ms | Fast response needed |
| Emergency switches | 400-600ms | 400-600ms | More deliberate |
| Selector switches | 500ms | 500ms | Standard timing |

Adjust based on your aircraft's cover animation speed in DCS.

---

## 6. Latched Buttons

Some buttons in DCS toggle state (press once = ON, press again = OFF) rather than being momentary. CockpitOS tracks these specially.

### 6.1 Configuration

Define latched buttons in `Mappings.cpp`:

```cpp
const char* kLatchedButtons[] = {
    "APU_FIRE_BTN",
    "CMSD_JET_SEL_BTN",
    "RWR_POWER_BTN",
    "SJ_CTR",
    "SJ_LI",
    "SJ_LO",
    "SJ_RI",
    "SJ_RO",
    // Add more as needed
};
```

### 6.2 How It Works

For latched buttons, CockpitOS:
1. Tracks the current state (ON/OFF) in command history
2. Sends toggle commands instead of momentary presses
3. Allows CoverGate to check state before toggling

### 6.3 Checking Latched State

```cpp
#include "../DCSBIOSBridge.h"

// Check if a latched button is currently ON
if (isToggleOn("APU_FIRE_BTN")) {
    // Button is latched ON
}

// Equivalent macro
if (isCoverOpen("LEFT_FIRE_BTN_COVER")) {
    // Cover is open
}
```

---

## 7. Panel Lifecycle

Understanding when each function is called:

### 7.1 Startup Sequence

```
1. setup()
   ├── initMappings()         ← Validates InputMapping, LEDMapping
   ├── initializeDisplays()   ← Calls PanelRegistry_forEachDisplayInit()
   ├── initializeLEDs()       ← Configures LED drivers
   └── initializePanels(true) ← Calls PanelRegistry_forEachInit() (forced)

2. loop()
   └── panelLoop()
       ├── PanelRegistry_forEachLoop()        ← Your panel_loop()
       ├── PanelRegistry_forEachDisplayLoop() ← Your display_loop()
       └── PanelRegistry_forEachTick()        ← Your tick()
```

### 7.2 Mission Start

When a mission starts in DCS:

```
onMissionStart()
└── initializePanels(false)
    └── PanelRegistry_forEachInit()  ← Your panel_init() again
```

This re-syncs all panel states with DCS.

### 7.3 Function Purposes

| Function | When Called | Purpose |
|----------|-------------|---------|
| `init` | Mission start | Sync state with DCS, reset hardware |
| `loop` | Every frame (~250 Hz) | Poll inputs, update state |
| `disp_init` | Once at startup | Initialize display hardware |
| `disp_loop` | Every frame | Render display updates |
| `tick` | Every frame | Flush LED drivers (WS2812, TM1637, etc.) |

---

## 8. Best Practices

### 8.1 Panel Code Guidelines

**DO:**
- Use `HIDManager_setNamedButton()` for all button/selector actions
- Use static variables for state (no heap allocation)
- Check `#if defined(HAS_YOUR_PANEL)` to gate compilation
- Use `debugPrintf()` for logging
- Keep loop functions fast (<1ms typical)

**DON'T:**
- Use `delay()` — it blocks the entire system
- Allocate with `new` or `malloc`
- Access hardware directly without going through CUtils
- Duplicate logic that's already in Generic.cpp

### 8.2 When to Create a Custom Panel

| Scenario | Solution |
|----------|----------|
| Standard buttons/switches | Use InputMapping + Generic.cpp |
| Standard LEDs | Use LEDMapping + appropriate tick |
| Complex state machine | Create custom panel |
| Custom hardware protocol | Create custom panel |
| Display with subscriptions | Create custom panel |

### 8.3 Debugging Tips

Enable debug output:
```cpp
// In Config.h
#define DEBUG 1
```

Add panel-specific logging:
```cpp
void MyPanel_loop() {
    static uint32_t lastLog = 0;
    if (millis() - lastLog > 1000) {
        debugPrintf("[MyPanel] state=%d\n", myState);
        lastLog = millis();
    }
}
```

### 8.4 Performance Considerations

- **Polling rate:** Generic.cpp runs at ~250 Hz. Match this for consistency.
- **Debouncing:** Use 4-20ms debounce for mechanical switches
- **Command spacing:** DCS-BIOS can handle ~30 commands/second safely
- **Display refresh:** 30-60 Hz is sufficient for most displays

---

## 9. METADATA Directory (Advanced)

Each Label Set can include a `METADATA/` subdirectory containing JSON overlay files. These allow you to customize, extend, or correct control definitions without modifying the base aircraft JSON.

### 9.1 Use Cases

- **Fix incorrect DCS-BIOS definitions** — Override addresses or masks
- **Add custom controls** — Define controls not in the standard JSON
- **Extend panel definitions** — Add metadata for custom hardware
- **Per-build customization** — Different settings for different cockpit builds

### 9.2 Directory Structure

```
src/LABELS/LABEL_SET_MYCOCKPIT/
├── METADATA/
│   ├── panel_overrides.json      ← Your custom overlays
│   └── custom_controls.json
├── selected_panels.txt
├── InputMapping.h
└── ...
```

### 9.3 How It Works

When `generate_data.py` runs:

1. Loads the base aircraft JSON (e.g., `FA-18C_hornet.json`)
2. Scans the `METADATA/` directory for `.json` files
3. Deep-merges overlays into the base definitions
4. Generates mappings from the merged result

### 9.4 Example Override

To fix an incorrect address for a control:

```json
{
  "My Panel": {
    "BROKEN_SWITCH": {
      "outputs": [{
        "address": 12345,
        "mask": 255,
        "shift_by": 0
      }]
    }
  }
}
```

This overrides only the specified fields — all other properties remain from the base JSON.

### 9.5 Best Practices

- Use descriptive filenames (`rwr_fixes.json`, `custom_gauges.json`)
- Document your changes with comments in the JSON (use `"_comment": "..."`)
- Keep overlays minimal — only override what's necessary
- Version control your METADATA directory with your label set

---

## Summary

| Topic | Key File(s) | Purpose |
|-------|-------------|---------|
| Panel registration | `src/PanelRegistry.h` | REGISTER_PANEL macro, lifecycle |
| PanelKind enum | `Mappings.h` | Panel identity (will be auto-generated) |
| CoverGate config | `Mappings.cpp` | kCoverGates[] table |
| Latched buttons | `Mappings.cpp` | kLatchedButtons[] table |
| CoverGate logic | `src/Core/CoverGate.cpp` | State machine for covered controls |
| METADATA overlays | `LABEL_SET_XXX/METADATA/` | JSON customizations per label set |
| Reference panels | `src/Panels/*.cpp` | Study existing implementations |

**Next steps:**
- Study `WingFold.cpp` for complex state machines
- Study `IFEIPanel.cpp` for display integration
- Study `LockShoot.cpp` for simple LED panels

---

*For display-specific topics (HT1622, TFT, segment mapping), see [ADVANCED_DISPLAYS.md](ADVANCED_DISPLAYS.md).*
