# CockpitOS — LLM Master Reference

> **This document is designed for AI language models.**
> It provides a structured overview of the entire CockpitOS project with links to detailed documentation. When answering questions about CockpitOS, read this document first, then follow the relevant links for deep details.

---

## 0. What Is CockpitOS?

CockpitOS is **ESP32 firmware** for building physical cockpit panels that interface with **DCS World** (a flight simulator) through **DCS-BIOS** (a LUA export protocol). It supports buttons, switches, LEDs, displays, servo gauges, and TFT screens — all configured through automated Python tools.

**Key facts:**
- Language: C++ (Arduino framework) for firmware, Python for tools
- Platform: ESP32 family (S2, S3, P4, Classic, C3/C5/C6, H2)
- Protocol: DCS-BIOS (binary export stream + text commands over UDP)
- Configuration: "Label Sets" — per-aircraft/per-panel config folders
- Tools: Three Python TUI apps automate setup, compilation, and configuration

---

## 1. Architecture Overview

```
                    DCS World (PC)
                         |
                    DCS-BIOS LUA
                         |
                    UDP Network
                         |
              ┌──────────┴──────────┐
              |                      |
         USB HID              WiFi / Serial
      (HID Manager)           (Direct UDP)
              |                      |
              └──────────┬──────────┘
                         |
                   ESP32 CockpitOS
                         |
           ┌─────────────┼─────────────┐
           |             |             |
      InputControl   LEDControl   DisplayControl
      (buttons,      (LEDs,       (HT1622, TFT,
       encoders,      WS2812,      TM1637, etc.)
       pots, etc.)    servos)
```

### Four-Layer Architecture

| Layer | Purpose | Key Files |
|-------|---------|-----------|
| **Panel Logic** | Per-aircraft behavior, state machines | `src/Panels/*.cpp` |
| **Bridge & HID** | DCS-BIOS protocol, USB HID reports | `DCSBIOSBridge.cpp`, `HIDManager.cpp` |
| **Hardware Abstraction** | Unified API for all hardware types | `lib/CUtils/`, `src/Core/InputControl.cpp`, `src/Core/LEDControl.cpp` |
| **Hardware** | Physical GPIO, I2C, SPI drivers | ESP32 Arduino Core |

**Deep dive:** [Advanced/Custom-Panels.md](../Advanced/Custom-Panels.md), [Advanced/Display-Pipeline.md](../Advanced/Display-Pipeline.md)

---

## 2. The Three Python Tools

CockpitOS ships three automation tools that eliminate manual file editing:

| Tool | Entry Script | Main Module | Purpose |
|------|-------------|-------------|---------|
| **Setup** | `Setup-START.py` | (self-contained) | Install ESP32 core v3.3.6, LovyanGFX v1.2.19 via bundled arduino-cli |
| **Compiler** | `CockpitOS-START.py` | `compiler/cockpitos.py` | Compile and upload firmware (board selection, COM port, active label set) |
| **Label Creator** | `LabelCreator-START.py` | `label_creator/label_creator.py` | Create/edit label sets with TUI editors for inputs, LEDs, displays, segment maps |

All three tools:
- Are **Windows-only**, Python 3.12+, ANSI terminal UIs
- Use `os.execl()` for seamless **tool switching** between each other
- Use a **bundled arduino-cli** (no Arduino IDE required)

**Deep dive:** [Tools/README.md](../Tools/README.md), [Tools/Setup-Tool.md](../Tools/Setup-Tool.md), [Tools/Compiler-Tool.md](../Tools/Compiler-Tool.md), [Tools/Label-Creator.md](../Tools/Label-Creator.md)

### Label Creator Editors

The Label Creator contains seven built-in editors:

| Editor | File Edited | Source Module |
|--------|-------------|---------------|
| **Input Editor** | `InputMapping.h` | `label_creator/input_editor.py` |
| **LED Editor** | `LEDMapping.h` | `label_creator/led_editor.py` |
| **Display Editor** | `DisplayMapping.cpp` | `label_creator/display_editor.py` |
| **Segment Map Editor** | `*_SegmentMap.h` | `label_creator/segment_map_editor.py` |
| **Custom Pins Editor** | `CustomPins.h` | `label_creator/custompins_editor.py` |
| **Latched Buttons Editor** | `LatchedButtons.h` | `label_creator/latched_editor.py` |
| **CoverGate Editor** | `CoverGates.h` | `label_creator/covergate_editor.py` |

**Deep dive:** [Tools/Label-Creator.md](../Tools/Label-Creator.md)

For Label Creator internals (LLM-specific): see `label_creator/LLM/LLM_GUIDE.md`, `label_creator/LLM/ARCHITECTURE.md`, `label_creator/LLM/EDITOR_FEATURES.md`

---

## 3. Project Structure

```
CockpitOS/
├── CockpitOS.ino          ← Arduino sketch entry point
├── Config.h               ← Master configuration (transport, debug, timing)
├── Setup-START.py         ← Environment setup tool
├── CockpitOS-START.py     ← Compiler tool launcher
├── LabelCreator-START.py  ← Label Creator launcher
│
├── src/
│   ├── LABELS/
│   │   ├── active_set.h   ← Points to active label set
│   │   ├── LABEL_SET_TEST_ONLY/
│   │   │   ├── InputMapping.h
│   │   │   ├── LEDMapping.h
│   │   │   ├── DisplayMapping.cpp/h
│   │   │   ├── LabelSetConfig.h
│   │   │   ├── CustomPins.h
│   │   │   ├── DCSBIOSBridgeData.h
│   │   │   ├── *_SegmentMap.h
│   │   │   ├── LatchedButtons.h      ← Per-label-set latched button config
│   │   │   ├── CoverGates.h          ← Per-label-set cover gate config
│   │   │   ├── selected_panels.txt
│   │   │   └── METADATA/
│   │   └── (other label sets...)
│   │
│   ├── Panels/            ← Panel implementations
│   │   ├── Generic.cpp    ← Handles all InputMapping/LEDMapping entries
│   │   ├── IFEIPanel.cpp  ← HT1622 segment LCD display panel
│   │   ├── WingFold.cpp   ← State machine example
│   │   └── TFT_Gauges_*.cpp  ← TFT gauge tasks
│   │
│   ├── Core/              ← Core firmware modules
│   │   ├── InputControl.cpp/h  ← Input scanning (GPIO, PCA, HC165, TM1637, MATRIX)
│   │   ├── LEDControl.cpp/h    ← Output driving (GPIO, PCA, WS2812, TM1637, GN1640T, GAUGE)
│   │   ├── PanelRegistry.cpp   ← Panel registration and lifecycle management
│   │   └── CoverGate.cpp/h     ← Guarded switch cover state machine
│   ├── DCSBIOSBridge.cpp/h ← DCS-BIOS protocol parser + subscription system
│   ├── HIDManager.cpp/h    ← USB HID report generation + input logic
│   ├── PanelRegistry.h     ← PanelHooks struct, REGISTER_PANEL macro
│   └── Globals.h           ← Shared includes and forward declarations
│
├── compiler/              ← Compiler tool source
│   ├── cockpitos.py       ← Main compiler module
│   └── arduino-cli/       ← Bundled arduino-cli binary
│
├── label_creator/         ← Label Creator source
│   ├── label_creator.py   ← Main orchestrator
│   ├── ui.py              ← TUI toolkit (ANSI, menus, editors)
│   ├── input_editor.py    ← InputMapping.h editor
│   ├── led_editor.py      ← LEDMapping.h editor
│   ├── display_editor.py  ← DisplayMapping.cpp editor
│   ├── segment_map_editor.py ← SegmentMap.h editor
│   ├── custompins_editor.py ← CustomPins.h editor
│   ├── latched_editor.py    ← LatchedButtons.h editor
│   ├── covergate_editor.py  ← CoverGates.h editor
│   ├── _core/aircraft/    ← Aircraft JSON definitions
│   └── LLM/               ← LLM-specific reference docs
│
├── lib/
│   └── CUtils/            ← Hardware abstraction library (PCA9555, TM1637, GN1640, WS2812, GPIO, HT1622, HC165, etc.)
│
├── HID Manager/           ← USB HID bridge (PC-side)
│   ├── HID_Manager.py
│   └── settings.ini
│
├── Debug Tools/           ← Debug utilities
│   ├── DEBUG_UDP_console.py
│   ├── PLAY_DCS_stream.py
│   ├── RECORD_DCS_stream.py
│   └── (other tools...)
│
└── Docs/                  ← Official structured documentation
```

---

## 4. Label Sets — The Configuration System

A **Label Set** is a folder in `src/LABELS/` containing all configuration files for a specific panel/aircraft combination.

### Key Files

| File | Purpose | Edited By |
|------|---------|-----------|
| `InputMapping.h` | Input definitions (buttons, switches, encoders, pots) | Label Creator Input Editor |
| `LEDMapping.h` | Output definitions (LEDs, displays, servos) | Label Creator LED Editor |
| `DisplayMapping.cpp` | Display field → segment map linkage | Label Creator Display Editor |
| `*_SegmentMap.h` | HT1622 RAM-to-segment mapping | Label Creator Segment Map Editor |
| `LabelSetConfig.h` | Device name, HID settings | Label Creator Misc Options |
| `CustomPins.h` | Pin assignments, feature enables (ENABLE_TFT_GAUGES, ENABLE_PCA9555) | Label Creator Custom Pins |
| `DCSBIOSBridgeData.h` | Auto-generated hash tables for DCS-BIOS lookup | Generator (auto) |
| `LatchedButtons.h` | Latched (toggle) button declarations | Label Creator Latched Buttons Editor |
| `CoverGates.h` | Guarded switch cover definitions | Label Creator CoverGate Editor |
| `selected_panels.txt` | Which aircraft panels are included | Label Creator Panel Selector |
| `METADATA/*.json` | Custom control overrides | Generator system |

### InputMapping.h Record Format

```cpp
{ "LABEL", "SOURCE", port, bit, hidButton, "dcsCommand", sendValue, "controlType", group }
```

| Field | Values | Description |
|-------|--------|-------------|
| `LABEL` | string | Human-readable name (from DCS-BIOS JSON) |
| `SOURCE` | `"GPIO"`, `"PCA_0xNN"`, `"HC165"`, `"TM1637"`, `"MATRIX"`, `"NONE"` | Hardware driver |
| `port` | int | GPIO pin number, PCA port (0-1), chain position |
| `bit` | int | Bit within the device (0-15 for PCA, 0-7 for HC165) |
| `hidButton` | int | USB HID button number (-1 for none) |
| `dcsCommand` | string | DCS-BIOS command name |
| `sendValue` | int | Value to send (0/1 for momentary, position for selector, 65535 for analog) |
| `controlType` | `"momentary"`, `"selector"`, `"analog"`, `"variable_step"`, `"fixed_step"` | Input behavior |
| `group` | int | Selector group number (0 = ungrouped) |

**Deep dive:** [Reference/Control-Types.md](../Reference/Control-Types.md)

### LEDMapping.h Record Format

```cpp
{ "LABEL", DEVICE_TYPE, {.deviceInfo = {...}}, dim, activeLow }
```

| Device Type | Info Union | Description |
|-------------|-----------|-------------|
| `DEVICE_GPIO` | `{pin}` | Direct GPIO LED |
| `DEVICE_PCA9555` | `{address, bit}` | PCA9555 I2C expander |
| `DEVICE_WS2812` | `{ledIndex}` | Addressable RGB LED |
| `DEVICE_TM1637` | `{moduleIndex, segmentIndex, bit}` | 7-segment display segment |
| `DEVICE_GN1640T` | `{column, row}` | LED matrix position |
| `DEVICE_GAUGE` | `{pin, minPulse, maxPulse}` | Servo motor gauge |

---

## 5. Hardware Support Matrix

### Input Hardware

| Source | Hardware | Pins Used | Max Inputs | Configuration |
|--------|----------|-----------|-----------|---------------|
| `GPIO` | Direct GPIO | 1 per input | ~30 per ESP32 | Port = GPIO pin number |
| `PCA` | PCA9555 I2C | 2 (SDA/SCL shared) | 16 per chip, 8 chips = 128 | Port = I2C address (0x20-0x27), Bit = pin (0-15) |
| `HC165` | 74HC165 shift register | 3 (LOAD/CLK/DATA shared) | 8 per chip, chain = hundreds | Port = chain position, Bit = bit (0-7) |
| `TM1637` | TM1637 keypad | 2 per module (CLK/DIO) | 16 keys per module | Port = module index, Bit = key index |
| `MATRIX` | Matrix scanning | N strobes + M data | N*M positions | Strobe/data pin configuration in CustomPins.h |

### Output Hardware

| Device | Hardware | Pins Used | Description |
|--------|----------|-----------|-------------|
| `DEVICE_GPIO` | Direct GPIO LED | 1 per LED | Simple on/off LEDs |
| `DEVICE_PCA9555` | PCA9555 I2C | Shared SDA/SCL | I2C-driven LEDs |
| `DEVICE_WS2812` | WS2812 RGB | 1 data pin per strip | Addressable RGB, chainable |
| `DEVICE_TM1637` | TM1637 7-segment | 2 per module | Numeric display segments |
| `DEVICE_GN1640T` | GN1640T LED matrix | 2 per module | LED grid for annunciators |
| `DEVICE_GAUGE` | Servo motor | 1 per servo | Analog gauge needles |

**Deep dive:** [Hardware/README.md](../Hardware/README.md) and all component-specific pages

---

## 6. Transport Modes

| Mode | Config Flag | Requirements | Latency | Best For |
|------|------------|--------------|---------|----------|
| **USB HID** | `USE_DCSBIOS_USB=1` | ESP32-S2/S3/P4 + HID Manager on PC | Lowest (~1ms) | Production, most reliable |
| **WiFi UDP** | `USE_DCSBIOS_WIFI=1` | Any ESP32 (except H2), same 2.4GHz network | Medium (~5-10ms) | Wireless panels, quick testing |
| **Serial/CDC** | `USE_DCSBIOS_SERIAL=1` | Any ESP32, socat bridge on PC | Low (~2ms) | Legacy setups |
| **BLE** | `USE_DCSBIOS_BLUETOOTH=1` | ESP32-S3, Classic, C3/C5/C6, H2 | Variable | Internal/experimental |

Only ONE transport mode can be active. The compiler enforces this.

**RS485 networking** is an overlay: one master (with any transport to PC) forwards data to slave panels over a 2-wire bus. Up to 31 slaves per master.

**Deep dive:** [Reference/Transport-Modes.md](../Reference/Transport-Modes.md), [How-To/Wire-RS485-Network.md](../How-To/Wire-RS485-Network.md), [Advanced/RS485-Deep-Dive.md](../Advanced/RS485-Deep-Dive.md)

---

## 7. Config.h Quick Reference

The most important settings in `Config.h`:

| Setting | Default | What It Does |
|---------|---------|-------------|
| `USE_DCSBIOS_USB` | 1 | Enable USB HID transport |
| `USE_DCSBIOS_WIFI` | 0 | Enable WiFi UDP transport |
| `WIFI_SSID` / `WIFI_PASS` | — | WiFi credentials |
| `POLLING_RATE_HZ` | 250 | Input scan rate |
| `DEBUG_ENABLED` | 0 | Master debug switch (0 for production) |
| `VERBOSE_MODE` | 0 | Verbose logging |
| `USB_VID` | 0xCAFE | USB vendor ID (must match HID Manager) |

Per-label-set settings live in `CustomPins.h`:

| Setting | Default | What It Does |
|---------|---------|-------------|
| `ENABLE_TFT_GAUGES` | 0 | Enable TFT display support |
| `ENABLE_PCA9555` | 0 | Enable I2C GPIO expanders |
| HC165 pin defines | — | Shift register wiring |
| PCA SDA/SCL pin defines | — | I2C bus wiring |

**Deep dive:** [Reference/Config.md](../Reference/Config.md)

---

## 8. DCS-BIOS Integration

CockpitOS communicates with DCS World through DCS-BIOS:

- **Export (DCS → Panel):** Binary frames over UDP multicast `239.255.50.10:5010`. Each frame contains address/value pairs representing cockpit state.
- **Import (Panel → DCS):** Text commands over UDP port `7778`. Format: `"CONTROL_NAME VALUE\n"` (e.g., `"MASTER_ARM_SW 1\n"`).

### Subscription System
Panels subscribe to specific DCS-BIOS addresses. When data changes at a subscribed address, the firmware calls the registered callback. This allows selective processing — panels only react to relevant data.

### Label Hashing
CockpitOS uses compile-time hash tables (in `DCSBIOSBridgeData.h`) for O(1) lookup of DCS-BIOS controls by name. This is auto-generated by the generator system.

**Deep dive:** [Reference/DCS-BIOS.md](../Reference/DCS-BIOS.md)

---

## 9. Common User Workflows

### First-Time Setup
1. Download CockpitOS → unzip to `Documents\Arduino\CockpitOS\`
2. Run `Setup-START.py` → installs ESP32 core + LovyanGFX
3. Run `LabelCreator-START.py` → create label set → select aircraft → select panels → auto-generate
4. Edit inputs/LEDs using built-in editors
5. Run `CockpitOS-START.py` → select board → compile → upload
6. Configure transport in `Config.h` (or via Compiler Tool misc options)
7. Test in DCS

**Deep dive:** [Getting-Started/README.md](../Getting-Started/README.md), [Getting-Started/Quick-Start.md](../Getting-Started/Quick-Start.md)

### Adding Hardware
1. Wire component to ESP32 (see Hardware guides)
2. Open Label Creator → Modify label set
3. Edit Inputs or Edit LEDs → find the DCS-BIOS control → configure Source/Device + Port/Bit
4. Recompile and upload via Compiler Tool

**Deep dive:** [Hardware/README.md](../Hardware/README.md)

### Multi-Panel Setup (RS485)
1. Set up master panel with USB/WiFi transport to PC
2. Enable `RS485_MASTER_ENABLED=1`, configure RS485 pins
3. Set up each slave with `RS485_SLAVE_ENABLED=1`, unique address
4. Wire RS485 bus: A→A, B→B, twisted pair, termination resistors
5. Each panel has its own label set

**Deep dive:** [How-To/Wire-RS485-Network.md](../How-To/Wire-RS485-Network.md)

---

## 10. Decision Router

Use this flowchart when answering questions:

```
Question about...
│
├── Setting up / getting started?
│   → Getting-Started/README.md
│
├── Wiring specific hardware?
│   → Hardware/[component].md
│
├── Step-by-step tutorial?
│   → How-To/[topic].md
│
├── Config.h settings?
│   → Reference/Config.md
│
├── Transport mode choice?
│   → Reference/Transport-Modes.md
│
├── Input type (momentary, selector, analog)?
│   → Reference/Control-Types.md
│
├── DCS-BIOS protocol?
│   → Reference/DCS-BIOS.md
│
├── Using the tools?
│   → Tools/[tool].md
│
├── Custom panel code?
│   → Advanced/Custom-Panels.md
│   → How-To/Create-Custom-Panel.md
│
├── Display system internals?
│   → Advanced/Display-Pipeline.md
│
├── Troubleshooting?
│   → Reference/Troubleshooting.md
│   → Reference/FAQ.md
│
├── Label Creator internals?
│   → label_creator/LLM/LLM_GUIDE.md
│   → label_creator/LLM/ARCHITECTURE.md
│   → label_creator/LLM/EDITOR_FEATURES.md
│
└── Firmware internals (CUtils, HIDManager, DCSBIOSBridge APIs)?
    → Advanced/Custom-Panels.md (API reference, panel lifecycle, subscriptions)
    → lib/CUtils/src/CUtils.h (hardware abstraction declarations)
```

---

## 11. Critical Rules for LLMs

When working with CockpitOS code or configuration:

1. **Never modify auto-generated sections** in mapping files. The `// AUTO-GENERATED` markers delineate protected regions that the generator system overwrites.

2. **Only ONE transport mode** can be active in Config.h. Enabling multiple causes a build error.

3. **Label sets are self-contained.** All configuration for a panel lives in its label set folder. The core firmware (`src/`) should rarely need modification.

4. **Users should use the tools**, not manually edit InputMapping.h or LEDMapping.h. The Label Creator handles format correctness, field validation, and generator integration.

5. **USB CDC On Boot must be Disabled** for USB HID mode on S2/S3 boards. This is the #1 compilation error new users hit.

6. **PCA9555 addresses are hex** (standard: 0x20-0x27, but clones may use any I2C address like 0x5B). PCA devices are auto-detected from InputMapping.h and LEDMapping.h -- no manual panel table needed.

7. **Selector groups** must all share the same DCS-BIOS command name and group number, with different sendValue per position.

8. **Servos need external power.** Never power servos from the ESP32's 3.3V or 5V pin — they draw too much current.

9. **All debug flags should be 0** for production builds. Debug output adds latency.

10. **The firmware is the same for all aircraft.** Only the label set changes. The core firmware handles protocol parsing, timing, and hardware abstraction universally.

---

## 12. Documentation Index

### Getting Started
| Document | Description |
|----------|-------------|
| [Getting-Started/README.md](../Getting-Started/README.md) | Full environment setup guide |
| [Getting-Started/Quick-Start.md](../Getting-Started/Quick-Start.md) | First panel in 30 minutes |

### Tools
| Document | Description |
|----------|-------------|
| [Tools/README.md](../Tools/README.md) | Overview of all tools |
| [Tools/Setup-Tool.md](../Tools/Setup-Tool.md) | Setup-START.py reference |
| [Tools/Compiler-Tool.md](../Tools/Compiler-Tool.md) | CockpitOS-START.py reference |
| [Tools/Label-Creator.md](../Tools/Label-Creator.md) | LabelCreator-START.py reference |
| [Tools/Debug-Tools.md](../Tools/Debug-Tools.md) | Debug and utility tools |

### Hardware
| Document | Description |
|----------|-------------|
| [Hardware/README.md](../Hardware/README.md) | Board selection, GPIO basics, power |
| [Hardware/Buttons-Switches.md](../Hardware/Buttons-Switches.md) | Buttons, toggles, rotary switches |
| [Hardware/LEDs.md](../Hardware/LEDs.md) | LEDs, WS2812 RGB strips |
| [Hardware/Encoders.md](../Hardware/Encoders.md) | Rotary encoders |
| [Hardware/Analog-Inputs.md](../Hardware/Analog-Inputs.md) | Potentiometers, analog axes |
| [Hardware/Shift-Registers.md](../Hardware/Shift-Registers.md) | 74HC165 input expansion |
| [Hardware/I2C-Expanders.md](../Hardware/I2C-Expanders.md) | PCA9555 GPIO expansion |
| [Hardware/Displays.md](../Hardware/Displays.md) | TM1637, GN1640T, HT1622 |
| [Hardware/TFT-Gauges.md](../Hardware/TFT-Gauges.md) | TFT LCD gauges |
| [Hardware/Servo-Gauges.md](../Hardware/Servo-Gauges.md) | Servo motor analog gauges |

### How-To Guides
| Document | Description |
|----------|-------------|
| [How-To/README.md](../How-To/README.md) | Guide index |
| [How-To/Wire-Analog-Gauges.md](../How-To/Wire-Analog-Gauges.md) | Servo gauge setup |
| [How-To/Wire-TFT-Gauges.md](../How-To/Wire-TFT-Gauges.md) | TFT display gauge setup |
| [How-To/Wire-Solenoid-Switches.md](../How-To/Wire-Solenoid-Switches.md) | Force-feedback solenoid switches |
| [How-To/Wire-RS485-Network.md](../How-To/Wire-RS485-Network.md) | Multi-panel RS485 network |
| [How-To/Wire-Matrix-Switches.md](../How-To/Wire-Matrix-Switches.md) | Rotary matrix scanning |
| [How-To/Wire-Segment-Displays.md](../How-To/Wire-Segment-Displays.md) | HT1622 segment LCD panels |
| [How-To/Create-Custom-Panel.md](../How-To/Create-Custom-Panel.md) | Custom panel code |
| [How-To/Use-Multiple-Aircraft.md](../How-To/Use-Multiple-Aircraft.md) | Multi-aircraft label sets |

### Reference
| Document | Description |
|----------|-------------|
| [Reference/Config.md](../Reference/Config.md) | Config.h complete reference |
| [Reference/FAQ.md](../Reference/FAQ.md) | Frequently asked questions |
| [Reference/Troubleshooting.md](../Reference/Troubleshooting.md) | Problems and solutions |
| [Reference/Transport-Modes.md](../Reference/Transport-Modes.md) | USB, WiFi, Serial, BLE, RS485 |
| [Reference/Control-Types.md](../Reference/Control-Types.md) | momentary, selector, analog, etc. |
| [Reference/DCS-BIOS.md](../Reference/DCS-BIOS.md) | DCS-BIOS protocol and integration |

### Advanced
| Document | Description |
|----------|-------------|
| [Advanced/Custom-Panels.md](../Advanced/Custom-Panels.md) | Panel system deep dive |
| [Advanced/Display-Pipeline.md](../Advanced/Display-Pipeline.md) | Display architecture internals |
| [Advanced/CoverGate.md](../Advanced/CoverGate.md) | Guarded switch covers |
| [Advanced/Latched-Buttons.md](../Advanced/Latched-Buttons.md) | Toggle state tracking |
| [Advanced/RS485-Deep-Dive.md](../Advanced/RS485-Deep-Dive.md) | RS485 protocol internals |
| [Advanced/FreeRTOS-Tasks.md](../Advanced/FreeRTOS-Tasks.md) | Task management for TFT gauges |

### Additional LLM Resources
| Document | Description |
|----------|-------------|
| `label_creator/LLM/LLM_GUIDE.md` | Label Creator complete reference |
| `label_creator/LLM/ARCHITECTURE.md` | Label Creator implementation guide |
| `label_creator/LLM/EDITOR_FEATURES.md` | Editor features and UX patterns |

---

## 13. Version Information

- **ESP32 Arduino Core:** 3.3.6 (managed by Setup Tool)
- **LovyanGFX:** 1.2.19 (managed by Setup Tool)
- **arduino-cli:** 1.4.1 (bundled at `compiler/arduino-cli/`)
- **Python:** 3.12+ required for all tools
- **Platform:** Windows only (tools use msvcrt for terminal input)

---

*This document is the entry point. For any specific topic, follow the linked documents above. The documentation is designed to be modular — read only what you need.*
