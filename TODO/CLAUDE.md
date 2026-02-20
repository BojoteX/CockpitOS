**MANDATORY: ALWAYS ask the user for confirmation BEFORE modifying or updating ANY file. No exceptions.**

# CockpitOS — Claude Code Reference

High-performance ESP32 firmware for DCS World cockpit panels via DCS-BIOS protocol.
Zero-heap, non-blocking, deterministic. Arduino IDE only (not PlatformIO/ESP-IDF).

## CRITICAL BUGS — CHECK BEFORE ANY CODE GENERATION

| ID | Wrong | Right | Impact |
|----|-------|-------|--------|
| 001 | `ExportStreamListener(0x0000, 0xFFFE)` | `ExportStreamListener(0x0000, 0xFFFD)` | Multi-aircraft breaks, data corruption |
| 002 | `cfg.pin_rst = -1` (IdeasPark ST7789) | `cfg.pin_rst = 4` | Display blank/corrupted |
| 003 | Default USB settings on S3/P4 | Tools→USB Mode→"USB-OTG (TinyUSB)" | USB HID not detected |
| 004 | ESP32 Arduino Core <3.3.0 | Core ≥3.3.0 | Random USB disconnects |
| 005 | `Serial.println()` | `debugPrintln()` | Debug output lost in WiFi mode |

## ABSOLUTE CONSTRAINTS — NEVER VIOLATE

- **NO HEAP**: Forbidden: `malloc`, `new`, `free`, `delete`, `std::vector`, `std::string`, `String`. Use static/stack allocation. Exception: TFT sprites may use PSRAM.
- **NO BLOCKING**: Forbidden: `delay()`, busy-wait `while` loops, blocking I/O. Use non-blocking state machines, `millis()`, hardware timers.
- **BOUNDED LOOPS**: Every `while`/`for` must have iteration cap. Typical: 64 (input), 256 (parse), 1024 (search).
- **NO EXCEPTIONS/RTTI**: Forbidden: `try/catch`, `throw`, `typeid`, `dynamic_cast`. Use return codes.
- **CONST BY DEFAULT**: Every global/static that doesn't change → `const`/`constexpr`. Use `static_assert` for invariants.
- **NO DIRECT HW IN PANELS**: Forbidden: `digitalWrite()`, `analogRead()` in panel code. Use CUtils API.
- **NO DIRECT SERIAL**: Forbidden: `Serial.print*()`. Use `debugPrint()`, `debugPrintln()`, `debugPrintf()`.

## ARCHITECTURE — 4 LAYERS

```
L4 PANEL LOGIC     src/Panels/*.cpp       Custom panels, DCS-BIOS callbacks, REGISTER_PANEL
L3 BRIDGE & HID    src/Core/*.cpp         DCSBIOSBridge, HIDManager, LEDControl, InputControl, RingBuffer
L2 HAL             lib/CUtils/src/        Unified driver API: GPIO, PCA9555, TM1637, GN1640, WS2812, HC165, HT1622
L1 HARDWARE        ESP32 + peripherals    I2C, SPI, GPIO, ADC
```

Data flow: `DCS World → DCS-BIOS LUA → UDP/USB → CockpitOS → Hardware` (and reverse)

## FILE MAP — WHERE TO FIND THINGS

```
CockpitOS.ino                  Entry point (setup/loop)
Config.h                       Master config: transport, debug, timing, thresholds
transcript.md                  APPEND-ONLY activity log (audits, bugs, decisions — see TRANSCRIPT LOG)
Pins.h                         Default pin placeholders (see CustomPins.h per label set)
Mappings.h                     PanelKind enum, panel definitions
Mappings.cpp                   CoverGate config, latched buttons list

src/
  Globals.h                    Cross-module includes/externs
  PanelRegistry.h              Panel lifecycle: register, init, loop, tick
  HIDManager.h                 HID abstraction, button/axis/mode API
  DCSBIOSBridge.h              Protocol parsing, subscription dispatch
  InputControl.h               GPIO/HC165/PCA9555 input polling
  HIDDescriptors.h             USB HID report descriptor (64-byte gamepad)
  Core/
    DCSBIOSBridge.cpp           Protocol parser, subscription callbacks, mission sync
    HIDManager.cpp              Button logic, selector groups, throttling, axis calibration
    InputControl.cpp            Input polling, selector exclusivity, encoder handling
    LEDControl.cpp              Unified LED routing (GPIO/WS2812/TM1637/GN1640/PCA9555)
    CoverGate.cpp               Guarded control state machine
    RingBuffer.cpp              Non-blocking I/O (64-byte chunks for USB/UDP)
    PerfMonitor.h               USER-EDITABLE: custom profiling labels
    debugPrint.cpp              Logging abstraction (Serial OR WiFi UDP)
  Panels/
    Generic.cpp                 Default input/LED handling (automatic)
    IFEIPanel.cpp               IFEI HT1622 segment display
    TFT_Display_CMWS.cpp        Threat ring TFT display
    TFT_Gauges_*.cpp            Gauge panels (RadarAlt, Battery, BrakePress, etc.)
    WingFold.cpp                Wing fold selector panel
  LABELS/
    active_set.h                Points to current label set
    LABEL_SET_*/
      FA-18C_hornet.json        Aircraft definition (from DCS-BIOS)
      selected_panels.txt       Enabled panels (user edits)
      generate_data.py          Generator: JSON → static tables
      InputMapping.h             Input definitions (USER EDITS preserved between markers)
      LEDMapping.h               Output definitions (USER EDITS preserved between markers)
      DCSBIOSBridgeData.h        AUTO-GENERATED address tables
      CustomPins.h               Panel-specific GPIO pin assignments
  Generated/
    PanelKind.h                 AUTO-GENERATED panel enum

lib/
  CUtils/src/CUtils.h          Hardware Abstraction Layer (unified driver API)
  DCS-BIOS/src/                 Protocol library

Tools/RAM_Walker/               HT1622 segment discovery tool
Debug Tools/                    UDP console, replay scripts
HID Controller/                 Python USB HID bridge (HID_Manager.py)
Docs/LLM/CockpitOS-LLM-Reference.md    Master LLM reference (read for deep dives)
```

## USER-EDITABLE vs HANDS-OFF

**May edit**: `Config.h`, `Mappings.cpp`, `src/Core/PerfMonitor.h`, `src/LABELS/*/CustomPins.h`, `src/LABELS/*/InputMapping.h`, `src/LABELS/*/LEDMapping.h`, `src/Panels/*.cpp`

**Must NOT edit**: `src/Core/*` (except PerfMonitor.h), `lib/CUtils/*`, `lib/DCS-BIOS/*`, any auto-generated file

## TRANSPORT MODES (only ONE active in Config.h)

| Define | MCU Required | Notes |
|--------|-------------|-------|
| `USE_DCSBIOS_USB=1` | S2/S3/P4 | Lowest latency. Needs HID_Manager.py on PC |
| `USE_DCSBIOS_WIFI=1` | All except H2/P4 | Wireless. UDP multicast 239.255.50.10:5010 |
| `USE_DCSBIOS_SERIAL=1` | All | Legacy, needs socat |
| `USE_DCSBIOS_BLUETOOTH=1` | All except S2/P4 | BLE via NimBLE |

## DCS-BIOS PROTOCOL ESSENTIALS

- **Export (DCS→ESP)**: Binary stream. Sync=`0x55 0x55 0x55 0x55`, then addr/count/data pairs. Little-endian, word-aligned. Listener range: `0x0000..0xFFFD`.
- **Import (ESP→DCS)**: ASCII `"CONTROL_NAME VALUE\n"` to UDP 127.0.0.1:7778. Variable step sends `+3200`/`-3200`. Fixed step sends `INC`/`DEC`.
- **Timing**: Export ~30Hz, input poll 250Hz, HID reports 250Hz, display 30-60Hz.

## INPUTMAPPING.H QUICK REF

```c
struct InputMapping {
  const char* label;        // Unique name
  const char* source;       // "GPIO"|"HC165"|"PCA_0xNN"|"TM1637"|"MATRIX"|"NONE"
  int8_t port;              // PIN(x) for GPIO, 0/1 for PCA port, etc.
  int8_t bit;               // Bit index, -1=fallback/one-hot
  int8_t hidId;             // HID button 1-32, or -1
  const char* oride_label;  // DCS-BIOS command name
  uint16_t oride_value;     // Command value (65535=analog passthrough)
  const char* controlType;  // "momentary"|"selector"|"analog"|"variable_step"|"fixed_step"
  uint16_t group;           // Selector group (0=standalone, >0=grouped)
};
```

Selector rules: all positions of same physical switch share non-zero `group`. `bit=-1` = fallback (active when no other position active). Use `PIN(x)` macro for all GPIO.

## LEDMAPPING.H QUICK REF

```c
enum LEDDeviceType { DEVICE_GAUGE, DEVICE_GN1640T, DEVICE_TM1637, DEVICE_GPIO, DEVICE_PCA9555, DEVICE_NONE, DEVICE_WS2812 };
struct LEDMapping { const char* label; LEDDeviceType deviceType; LEDDeviceInfo info; bool dimmable; bool activeLow; };
```

Device info union: `.gpioInfo={pin}`, `.ws2812Info={index,R,G,B,bright}`, `.tm1637Info={clk,dio,seg,bit}`, `.pcaInfo={addr,port,bit}`, `.gn1640Info={addr,col,row}`, `.gaugeInfo={gpio,minPulse,maxPulse,period}`

## PANEL CREATION

```c
// src/Panels/MyPanel.cpp
// PANEL_KIND: MyPanel       ← Must be in first 30 lines
#if defined(HAS_MY_PANEL)

static void MyPanel_init() { /* subscribe to DCS data, setup state */ }
static void MyPanel_loop() { /* called every frame ~250Hz */ }

REGISTER_PANEL(MyPanel, MyPanel_init, MyPanel_loop, nullptr, nullptr, nullptr, 100);
#endif
```

Macro: `REGISTER_PANEL(KIND, init, loop, dispInit, dispLoop, tick, priority)` — nullptr for unused slots.

## KEY APIs

### DCSBIOSBridge — Subscriptions
```c
subscribeToLedChange("LABEL", [](const char* label, uint16_t value, uint16_t maxValue) { });
subscribeToSelectorChange("LABEL", [](const char* label, uint16_t value) { });
subscribeToDisplayChange("LABEL", [](const char* label, const char* value) { });
subscribeToMetadataChange("LABEL", [](const char* label, uint16_t value) { });
bool isMissionRunning();  uint16_t getLastKnownState("LABEL");
```

### HIDManager — Input Actions
```c
HIDManager_setNamedButton("LABEL", deferSend, pressed);         // Button press/release
HIDManager_toggleIfPressed(isPressed, "LABEL", deferSend);      // Latched toggle
HIDManager_moveAxis("DCS_ID", pin, AXIS_X, force, defer);       // Analog axis
sendDCSBIOSCommand("LABEL", value, force);                       // Direct DCS command
HIDManager_commitDeferredReport("reason");                       // Flush buffered reports
bool isModeSelectorDCS();                                        // DCS vs HID mode check
```

### CUtils — Hardware (panels must use these, never raw GPIO)
```c
GPIO_setDigital(pin, activeHigh, state);   GPIO_setAnalog(pin, activeLow, intensity);
PCA9555_write(addr, port, bit, state);     WS2812_setLEDColor(pin, index, r, g, b);
GN1640_setLED(row, col, state);            tm1637_displaySingleLED(dev, grid, seg, on);
HC165_read();                              AnalogG_set(pin, value);
```

## LABEL SET WORKFLOW

**Recommended: Use the Label Creator Tool** (`python LabelCreator-START.py`):
- Create / Modify / Delete label sets via interactive TUI
- Edit InputMapping.h, LEDMapping.h, DisplayMapping.cpp, SegmentMap.h files
- Auto-generate and set as default
- See `label_creator/ARCHITECTURE.md` for full documentation

**Manual workflow** (legacy):
1. `cd src/LABELS/ && cp -r LABEL_SET_IFEI LABEL_SET_MYPANEL`
2. `cd LABEL_SET_MYPANEL && python reset_data.py`
3. Copy aircraft JSON from `%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json\` (ONE file only)
4. Edit `selected_panels.txt` (uncomment desired panels, case-sensitive)
5. `python generate_data.py` → generates tables, sets this as active set
6. Edit `InputMapping.h` / `LEDMapping.h` (between user-edit markers)
7. Arduino IDE: Select board, set "USB CDC On Boot"=Disabled, S3→USB Mode=USB-OTG
8. Compile & upload

## BUILD & VERIFY

- **Compile**: Arduino IDE 2.x, ESP32 core ≥3.3.0, board selected, USB CDC On Boot=Disabled
- **Test without DCS**: Set `IS_REPLAY=1` in Config.h
- **Test LEDs**: Set `TEST_LEDS=1` for interactive serial menu
- **Profile**: Set `DEBUG_PERFORMANCE=1`, add custom labels in PerfMonitor.h
- **Debug PCA/HC165/TM1637**: Set respective `DEBUG_ENABLED_FOR_*_ONLY=1`

## DECISION TREES

**Adding an input?** Simple button → `InputMapping.h` only. Selector → same `group` for all positions + fallback `bit=-1`. Encoder → two entries `variable_step` (CCW=0, CW=1). Pot → `analog`, `oride_value=65535`. Complex logic → custom panel .cpp.

**Adding an output?** Simple LED → `LEDMapping.h` only. TFT gauge → new `src/Panels/TFT_Gauges_XXX.cpp` as FreeRTOS task. Custom state machine → new panel with `REGISTER_PANEL`.

**Which transport?** S2/S3/P4 + low latency → USB. Wireless → WiFi. Classic/C3/C6 → Serial or WiFi. H2 → BLE only.

## ANTI-PATTERNS — INSTANT REJECTION

```c
// WRONG → RIGHT
String s = "hello";              → static const char s[] = "hello";
std::vector<int> v;              → static int v[MAX];
char* buf = new char[64];        → static char buf[64];
delay(100);                      → non-blocking state machine with millis()
while(!Serial.available()) {}    → if (Serial.available()) { process; }
Serial.println("debug");        → debugPrintln("debug");
digitalWrite(LED, HIGH);         → GPIO_setDigital(LED, true, true);
analogRead(POT);                 → HIDManager_moveAxis("LABEL", pin, AXIS_X, false, false);
while(searching) { /*no cap*/ }  → for(int i=0; i<MAX && searching; i++) { }
```

## TRANSCRIPT LOG — MANDATORY

**All LLM agents working on this codebase MUST log significant activities in `transcript.md`** at the project root. This includes:

- **Audits & Reviews**: Full audit results, findings, issue counts
- **Architecture Decisions**: Why a particular approach was chosen over alternatives
- **Bug Fixes**: What was broken, root cause, what was changed
- **Feature Additions**: What was added, which files were modified, integration points
- **Refactors**: What was restructured and why
- **Configuration Changes**: Transport mode changes, pin reassignments, timing adjustments

Each entry must include: **date, scope, summary of findings/changes, and any open issues**.

Append new entries to the end of the file. Never delete or modify previous entries — `transcript.md` is an append-only log.

## DEEP REFERENCE

For full API signatures, all config defines, TFT display pipeline details, HT1622 segment mapping, CoverGate timing, FreeRTOS task patterns:
→ **`Docs/LLM/CockpitOS-LLM-Reference.md`** (master LLM reference)

For wiring guides, transport setup, label set creation walkthroughs:
→ **`Docs/`** structured subdirectories (Getting-Started, Hardware, How-To, Reference, Advanced, Tools)
