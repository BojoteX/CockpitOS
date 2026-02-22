# CockpitOS — GPT Quick Reference Index

> **Purpose:** This file is the GPT's first-hit lookup table. It surfaces the most commonly needed facts — exact API signatures, encoding schemes, defaults, and reference pointers — so the GPT retrieves correct CockpitOS-specific details instead of improvising from general knowledge.
>
> **Usage:** For simple questions, this file is self-contained. For deep detail, the section headers here match the Deep Reference file exactly — look there.
>
> Last updated: February 2026

---

## BEFORE YOU ANSWER: CHECK EXISTING CODE FIRST

Before writing any custom panel code or explaining a pattern, check if an existing implementation already does what the user needs:

| Need | Existing File | What It Does |
|------|--------------|--------------|
| State machine + PCA9555 raw reads + command pacing | `src/Panels/WingFold.cpp` | Two-axis wing fold handle with debounce, mechanical invariant enforcement, ring buffer pacing (500ms spacing) |
| HT1622 segment LCD display rendering | `src/Panels/IFEIPanel.cpp` | IFEI display with shadow RAM, overlay system, dual HT1622 chips, day/NVG backlight |
| TFT gauge (round, GC9A01, 240x240) | `src/Panels/TFT_Gauges_Battery.cpp` | Dirty-rect DMA, PSRAM sprites, FreeRTOS task, day/NVG modes |
| TFT gauge (round, ST77961, 360x360) | `src/Panels/TFT_Gauges_CabPress.cpp` | Higher resolution, multiple subscriptions, two needles |
| TFT text display | `src/Panels/TFT_Display_CMWS.cpp` | Multi-subscription tactical display, non-gauge TFT usage |
| Standard GPIO/PCA/HC165/analog inputs + LED outputs | `src/Panels/Generic.cpp` | Handles ALL InputMapping.h and LEDMapping.h entries automatically |
| Minimal custom panel template | `src/Panels/TestPanel.cpp` | Bare REGISTER_PANEL + init + loop skeleton |

**Always tell the user about the relevant existing file before writing new code.**

---

## CUSTOM PANEL CHECKLIST (REQUIRED ELEMENTS)

Every custom panel `.cpp` file in `src/Panels/` MUST have:

1. **Compile guard:** `#if defined(HAS_YOUR_LABEL_SET)` ... `#endif` wrapping the entire file
2. **PANEL_KIND comment:** `// PANEL_KIND: YourName` near the top (used by `generate_panelkind.py`)
3. **Include:** `#include "../Globals.h"` (gives access to HIDManager, DCSBIOSBridge, CUtils)
4. **REGISTER_PANEL macro:** with `nullptr` for unused hooks — never pass empty stub functions
5. **Non-blocking loop:** Never use `delay()`. Use `millis()` comparisons. Watchdog resets at ~3s
6. **Static memory only:** No `new`/`malloc` in loop paths. Allocate in init
7. **Subscribe in init, not loop:** DCS-BIOS subscriptions are one-time registrations
8. **Check `isMissionRunning()`** before processing DCS-BIOS data in display render functions

### REGISTER_PANEL Macro (exact signature)
```cpp
REGISTER_PANEL(KIND, INIT, LOOP, DINIT, DLOOP, TICK, PRIO);
```
- `KIND` = PanelKind enum value (auto-generated from `// PANEL_KIND:` comment)
- `INIT` = `void(*)()` called at mission start (or `nullptr`)
- `LOOP` = `void(*)()` called every poll cycle ~250Hz (or `nullptr`)
- `DINIT` = `void(*)()` display init at mission start (or `nullptr`)
- `DLOOP` = `void(*)()` display refresh every cycle (or `nullptr`)
- `TICK` = `void(*)()` per-frame work (or `nullptr`)
- `PRIO` = `uint8_t` execution priority, lower runs first, default 100

### Registration Patterns
```cpp
// Standard input panel:
REGISTER_PANEL(WingFold, WingFold_init, WingFold_loop, nullptr, nullptr, nullptr, 100);

// Display panel (like IFEI):
REGISTER_PANEL(IFEI, IFEI_init, nullptr, IFEIDisplay_init, IFEIDisplay_loop, nullptr, 100);

// TFT gauge (display-only):
REGISTER_PANEL(TFTBatt, nullptr, nullptr, BatteryGauge_init, BatteryGauge_loop, nullptr, 100);
```

---

## EXACT API FUNCTION SIGNATURES

### Sending Commands to DCS

```cpp
// Send a DCS-BIOS command with throttling and history tracking
void sendDCSBIOSCommand(const char* label, uint16_t value, bool force = false);

// Send a raw DCS-BIOS command (label + string value)
void sendCommand(const char* label, const char* value, bool silent = false);
```

### HID Button/Axis Control

```cpp
// Set a named button ON or OFF
void HIDManager_setNamedButton(const char* name, bool deferSend, bool pressed);

// Toggle a latched button (flip current state)
void HIDManager_setToggleNamedButton(const char* name, bool deferSend = false);

// Toggle on rising edge only (press detection for latched buttons)
void HIDManager_toggleIfPressed(bool isPressed, const char* label, bool deferSend = false);

// Move an analog axis
void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin, HIDAxis axis,
                         bool forceSend = false, bool deferredSend = false);

// Flush pending HID report
void HIDManager_dispatchReport(bool force = false);

// Commit deferred report (used by custom panels that defer multiple button changes)
void HIDManager_commitDeferredReport(const char* deviceName);
```

### DCS-BIOS Subscriptions (register in init, NEVER in loop)

```cpp
// Numeric values (LEDs, gauges, dimmers)
bool subscribeToLedChange(const char* label,
    void (*callback)(const char* label, uint16_t value, uint16_t max_value));

// Selector switch positions
bool subscribeToSelectorChange(const char* label,
    void (*callback)(const char* label, uint16_t value));

// Display string fields
bool subscribeToDisplayChange(const char* label,
    void (*callback)(const char* label, const char* value));

// Metadata/raw numeric values
bool subscribeToMetadataChange(const char* label,
    void (*callback)(const char* label, uint16_t value));
```

Subscription limits: `MAX_LED_SUBSCRIPTIONS=32`, `MAX_SELECTOR_SUBSCRIPTIONS=32`, `MAX_DISPLAY_SUBSCRIPTIONS=32`, `MAX_METADATA_SUBSCRIPTIONS=32`

### Hardware Read Functions

```cpp
// Read both ports of a PCA9555 in one call (simultaneous sample)
bool readPCA9555(uint8_t addr, byte &p0, byte &p1);

// HC165 shift register chain read (returns all bits as 64-bit value)
uint64_t HC165_read();

// Check if a DCS mission is currently active
bool isMissionRunning();

// Returns true when enough time has passed for next poll cycle (250Hz default)
bool shouldPollMs(unsigned long &lastPoll);

// Debug output (works on both WiFi and Serial depending on config)
void debugPrintf(const char* format, ...);
```

### State Query Macros

```cpp
#define isCoverOpen(label)      (findCmdEntry(label) ? (findCmdEntry(label)->lastValue > 0) : false)
#define isToggleOn(label)       isCoverOpen(label)
#define setCoverState(label,v)  sendDCSBIOSCommand(label, v ? 1 : 0, true)
#define setToggleState(label,v) setCoverState(label,v)
```

---

## INPUTMAPPING.H FIELD ENCODING

```cpp
{ "LABEL", "SOURCE", port, bit, hidButton, "dcsCommand", sendValue, "controlType", group }
```

### Source-specific field rules

| Source | port | bit | Notes |
|--------|------|-----|-------|
| `"GPIO"` | GPIO pin number | `0` (normal), `-1` (selector fallback) | Internal pull-up auto-enabled |
| `"PCA_0xNN"` | PCA port: `0` or `1` | Bit 0-7 within port; `-1` for selector fallback | Address in source string |
| `"HC165"` | Always `0` | Absolute bit position in chain (0-based across all chips) | Chip 0: bits 0-7, Chip 1: bits 8-15, etc. |
| `"TM1637"` | DIO pin number | Key index | Uses CLK/DIO of display module |
| `"MATRIX"` | **Anchor:** data pin GPIO; **Position:** strobe pin GPIO | **Anchor:** all-ones mask; **Position:** one-hot strobe index | See MATRIX section below |
| `"NONE"` | Ignored | Ignored | Placeholder, not scanned |

### Selector fallback: `bit = -1`

An entry with `bit = -1` is the **default/fallback position**. It fires when NO other position in the selector group is active. Used for:
- Spring-return center on a 3-pos toggle
- "No strobe active" state in MATRIX scanning
- Default position when a rotary switch is between detents

---

## MATRIX SCANNING — EXACT ENCODING

CockpitOS MATRIX is designed for **rotary selector switches**, NOT keypads. It uses GPIO pins directly — strobe pins as outputs, data pin as input with pull-up.

### How it works
1. Each strobe pin is driven LOW one at a time
2. The data pin is read after each strobe
3. If data reads LOW, the switch wiper is on that strobe's position
4. The resulting bit pattern identifies the position

### Anchor vs Position rows

| Row Type | `port` field | `bit` field | How firmware identifies it |
|----------|-------------|-------------|--------------------------|
| **Anchor** | Data pin GPIO | Multi-bit mask (e.g., `15` = 0b1111 for 4 strobes) | `popcount(bit) > 1` — more than one bit set |
| **Position** | Strobe pin GPIO | One-hot index (`1`, `2`, `4`, `8`, `16`, ...) | Exactly one bit set (power of 2) |
| **Fallback** | Data pin GPIO | `-1` | Negative bit value |

### One-hot bit-to-strobe mapping

| bit value | Binary | Strobe index | Meaning |
|-----------|--------|-------------|---------|
| 1 | 0b0001 | 0 | Position wired to strobes[0] |
| 2 | 0b0010 | 1 | Position wired to strobes[1] |
| 4 | 0b0100 | 2 | Position wired to strobes[2] |
| 8 | 0b1000 | 3 | Position wired to strobes[3] |
| 15 | 0b1111 | (anchor) | Anchor: 4 strobes total |
| -1 | (negative) | (fallback) | Fires when no strobe active |

### Complete worked example (from ALR67 label set)

CustomPins.h:
```cpp
#define ALR67_STROBE_1  PIN(16)   // Strobe index 0
#define ALR67_STROBE_2  PIN(17)   // Strobe index 1
#define ALR67_STROBE_3  PIN(21)   // Strobe index 2
#define ALR67_STROBE_4  PIN(37)   // Strobe index 3
#define ALR67_DataPin   PIN(38)   // Common data pin
```

InputMapping.h:
```cpp
// ANCHOR: port=DataPin, bit=15 (0b1111 = 4 strobes). Also the fallback position.
{ "RWR_DIS_TYPE_SW_F", "MATRIX", ALR67_DataPin,   15, -1, "RWR_DIS_TYPE_SW", 4, "selector", 6 },
// POSITIONS: port=strobe GPIO, bit=one-hot strobe index
{ "RWR_DIS_TYPE_SW_U", "MATRIX", ALR67_STROBE_1,   1, -1, "RWR_DIS_TYPE_SW", 3, "selector", 6 },
{ "RWR_DIS_TYPE_SW_A", "MATRIX", ALR67_STROBE_2,   2, -1, "RWR_DIS_TYPE_SW", 2, "selector", 6 },
{ "RWR_DIS_TYPE_SW_I", "MATRIX", ALR67_STROBE_3,   4, -1, "RWR_DIS_TYPE_SW", 1, "selector", 6 },
{ "RWR_DIS_TYPE_SW_N", "MATRIX", ALR67_STROBE_4,   8, -1, "RWR_DIS_TYPE_SW", 0, "selector", 6 },
```

All rows share the same `dcsCommand` ("RWR_DIS_TYPE_SW") and `group` (6).

### Scan behavior
- Firmware drives strobes[0..3] LOW one at a time, reads data pin
- Pattern = which strobe(s) saw data LOW (as one-hot bitmask)
- Pattern `4` (0b0100) = strobe index 2 active = "RWR_DIS_TYPE_SW_I" = sends value 1
- Pattern `0` (no strobe active) = fallback = "RWR_DIS_TYPE_SW_F" = sends value 4

### Limits
- `MAX_MATRIX_ROTARIES` = 8
- `MAX_MATRIX_STROBES` = 8 per rotary
- `MAX_MATRIX_POS` = 16 per rotary

---

## KEY STRUCT DEFINITIONS

### PanelHooks
```cpp
struct PanelHooks {
    const char* label;     // debug string
    PanelKind   kind;      // enum identity
    uint8_t     prio;      // lower runs first, default 100
    PanelFn     init;      // void(*)() — called at mission start
    PanelFn     loop;      // void(*)() — called every poll cycle
    PanelFn     disp_init; // void(*)() — display init at mission start
    PanelFn     disp_loop; // void(*)() — display refresh every cycle
    PanelFn     tick;      // void(*)() — optional per-frame work
};
```

### CoverGateDef
```cpp
enum class CoverGateKind : uint8_t { Selector, ButtonMomentary, ButtonLatched };

struct CoverGateDef {
    const char*   action_label;   // DCS-BIOS label for armed/ON action
    const char*   release_label;  // DCS-BIOS label for safe/OFF (nullptr for ButtonMomentary)
    const char*   cover_label;    // DCS-BIOS label for cover open/close
    CoverGateKind kind;
    uint16_t      delay_ms;       // Time between cover-open and action
    uint16_t      close_delay_ms; // Time between action and cover-close
};
```

### SegmentMap (HT1622 LCD)
```cpp
struct SegmentMap {
    uint8_t addr;   // HT1622 RAM address (0-63), 0xFF = unused
    uint8_t bit;    // Bit within address (0-3), 0xFF = unused
    uint8_t ledID;  // Which HT1622 chip (0 or 1)
};
```

---

## CRITICAL DEFAULTS (VERIFIED AGAINST CODE)

### Config.h — Manual-Only Settings

| Setting | Default | Source File |
|---------|---------|-------------|
| POLLING_RATE_HZ | 250 | Config.h |
| SELECTOR_DWELL_MS | 250 | Config.h |
| SERVO_UPDATE_FREQ_MS | 20 | Config.h |
| PCA_FAST_MODE | 1 | Config.h |
| RS485_USE_TASK | 1 | Config.h |
| RS485_TASK_CORE | 0 | Config.h |
| IS_REPLAY | 0 | Config.h |
| TEST_LEDS | 0 | Config.h |
| SCAN_WIFI_NETWORKS | 0 | Config.h |

### Firmware Limits

| Constant | Value | Where Defined |
|----------|-------|---------------|
| MAX_COVER_GATES | 16 | Mappings.h |
| MAX_LATCHED_BUTTONS | 16 | Mappings.h |
| PANELREGISTRY_MAX_PANELS | 32 | PanelRegistry.cpp |
| MAX_LED_SUBSCRIPTIONS | 32 | DCSBIOSBridge.h |
| MAX_SELECTOR_SUBSCRIPTIONS | 32 | DCSBIOSBridge.h |
| MAX_DISPLAY_SUBSCRIPTIONS | 32 | DCSBIOSBridge.h |
| MAX_METADATA_SUBSCRIPTIONS | 32 | DCSBIOSBridge.h |
| MAX_MATRIX_ROTARIES | 8 | InputControl.h |
| MAX_MATRIX_STROBES | 8 | InputControl.h |
| MAX_MATRIX_POS | 16 | InputControl.h |
| MAX_PCA9555_INPUTS | 64 | InputControl.h |
| MAX_PCAS | 8 | InputControl.h |
| MAX_GAUGES | 8 | CUtils.h |
| MAX_TM1637_DEV | 4 | InputControl.h |
| HT1622_RAM_SIZE | 64 | CUtils.h |

---

## TOOL MENU PATHS (EXACT)

### Compiler Tool (CockpitOS-START.py)
- **Transport:** Role / Transport > choose mode
- **WiFi creds:** Misc Options > Wi-Fi Credentials
- **Debug flags:** Misc Options > Debug / Verbose Toggles
- **HID default:** Misc Options > Advanced Settings > HID mode by default
- **RS485 master:** Role / Transport > RS485 Master options
- **RS485 slave:** Role / Transport > RS485 Slave > set address (1-126)

### Label Creator (LabelCreator-START.py)
- **Create:** Create New Label Set > name > aircraft > panels > generate
- **Modify:** Modify Label Set > pick set > Edit Inputs / Edit Outputs / etc.
- **Inputs:** Modify > Edit Inputs (InputMapping.h)
- **LEDs:** Modify > Edit Outputs (LEDs) (LEDMapping.h)
- **Displays:** Modify > Edit Displays (DisplayMapping.cpp)
- **Segment maps:** Modify > Segment Maps (*_SegmentMap.h)
- **Custom pins:** Modify > Edit Custom Pins (CustomPins.h)
- **Latched buttons:** Modify > Edit Latched Buttons (LatchedButtons.h)
- **CoverGates:** Modify > Edit CoverGates (CoverGates.h)

---

## RS485 — QUICK OPTIMIZATION CHECKLIST

When users report lag on RS485 networks:

1. **RS485_MAX_SLAVE_ADDRESS** — Default 127. If you have 3 slaves, set to 3. Eliminates 124 wasted poll cycles.
2. **RS485_SMART_MODE** — Set to 1 on master. Forwards only subscribed addresses instead of entire stream.
3. **RS485_USE_TASK** — Must be 1 for USB/Serial transport on master (dedicated FreeRTOS task). Set 0 only for WiFi.
4. **Debug flags** — All must be 0 on production RS485 nodes. Even minimal logging stalls the poll loop.
5. **Label set scope** — Each slave's label set should contain ONLY the panels that slave physically has. Broader sets inflate subscriptions.
6. **Wiring** — Twisted pair for A/B, 120-ohm termination at both physical ends only, common ground between all nodes.

---

## CONTROL TYPES QUICK REFERENCE

| Type | Hardware | DCS-BIOS Interface | sendValue |
|------|----------|-------------------|-----------|
| `"momentary"` | Push button, toggle | `set_state` | Value on press, 0 on release |
| `"selector"` | Multi-position switch | `set_state` | Position number (0, 1, 2...) |
| `"analog"` | Potentiometer, slider | `set_state` | 65535 (auto-scaled 0-65535) |
| `"variable_step"` | Rotary encoder | `variable_step` | 0=CCW, 1=CW |
| `"fixed_step"` | Rotary encoder | `fixed_step` | INC/DEC direction |

---

## TRANSPORT COMPATIBILITY MATRIX

| Board | USB HID | WiFi | BLE | Serial | RS485 |
|-------|---------|------|-----|--------|-------|
| ESP32-S3 | Yes | Yes | Yes | Yes | Yes |
| ESP32-S2 | Yes | Yes | No | Yes | Yes |
| ESP32-P4 | Yes | No | No | Yes | Yes |
| ESP32 Classic | No | Yes | Yes | Yes | Yes |
| ESP32-C3/C5/C6 | No | Yes | Yes | Yes | Yes |
| ESP32-H2 | No | No | Yes | Yes | Yes |
