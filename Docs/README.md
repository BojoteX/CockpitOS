# CockpitOS Documentation

> **Build real cockpit panels for DCS World using ESP32 microcontrollers.**
>
> CockpitOS is open-source firmware that turns ESP32 boards into fully functional flight simulator panels with buttons, switches, LEDs, displays, analog gauges, and more.

---

## Quick Links

| I want to... | Go to |
|---|---|
| **Set up my environment for the first time** | [Getting Started](Getting-Started/README.md) |
| **Build my first panel in 30 minutes** | [Quick Start Guide](Getting-Started/Quick-Start.md) |
| **Learn how the Python tools work** | [Tools Overview](Tools/README.md) |
| **Wire specific hardware** | [Hardware Guides](Hardware/README.md) |
| **Follow a step-by-step how-to** | [How-To Guides](How-To/README.md) |
| **Look up a Config.h setting** | [Config Reference](Reference/Config.md) |
| **Troubleshoot a problem** | [FAQ & Troubleshooting](Reference/FAQ.md) |
| **Build something advanced** | [Custom Panels](Advanced/Custom-Panels.md) |

---

## Documentation Map

```
Docs/
  |
  +-- Getting-Started/
  |     README.md ............... Environment setup guide
  |     Quick-Start.md ......... First panel in 30 minutes
  |
  +-- Tools/
  |     README.md ............... Overview of all three tools
  |     Setup-Tool.md .......... Environment installer (Setup-START.py)
  |     Compiler-Tool.md ....... Firmware compiler (CockpitOS-START.py)
  |     Label-Creator.md ....... Label set editor (LabelCreator-START.py)
  |     Debug-Tools.md ......... Debug & replay utilities + HID Manager
  |
  +-- Hardware/
  |     README.md ............... Hardware overview & board selection
  |     Buttons-Switches.md .... Buttons, toggles, rotary switches
  |     LEDs.md ................. Single LEDs, WS2812 RGB strips
  |     Encoders.md ............. Rotary encoders
  |     Analog-Inputs.md ....... Potentiometers and analog axes
  |     Shift-Registers.md ..... 74HC165 input expanders
  |     I2C-Expanders.md ....... PCA9555 GPIO expanders
  |     Displays.md ............. TM1637, GN1640T, HT1622, MAX7219
  |     TFT-Gauges.md .......... TFT LCD gauge panels
  |     Servo-Gauges.md ........ Analog servo gauges
  |
  +-- How-To/
  |     README.md ............... Guide index
  |     Wire-Analog-Gauges.md .. Servo-driven analog gauges
  |     Wire-TFT-Gauges.md ..... TFT display gauges (LovyanGFX)
  |     Wire-Solenoid-Switches.md  Solenoid-actuated switches
  |     Wire-RS485-Network.md .. Multi-panel RS485 networking
  |     Wire-Matrix-Switches.md   Rotary matrix switches
  |     Wire-Segment-Displays.md  HT1622 / segment LCD panels
  |     Create-Custom-Panel.md .. Writing custom panel code
  |     Use-Multiple-Aircraft.md  Multi-aircraft label sets
  |
  +-- Reference/
  |     Config.md ............... Config.h complete reference
  |     FAQ.md .................. Frequently asked questions
  |     Troubleshooting.md ..... Common problems & solutions
  |     Transport-Modes.md ..... USB, WiFi, Serial, BLE
  |     Control-Types.md ....... momentary, selector, analog, etc.
  |     DCS-BIOS.md ............. DCS-BIOS protocol & integration
  |
  +-- Advanced/
  |     Custom-Panels.md ....... REGISTER_PANEL, PanelKind, lifecycle
  |     Display-Pipeline.md .... Display system architecture
  |     CoverGate.md ........... Guarded switch covers
  |     Latched-Buttons.md ..... Toggle state tracking
  |     RS485-Deep-Dive.md ..... RS485 protocol internals
  |     FreeRTOS-Tasks.md ...... Task management for TFT gauges
  |
  +-- LLM/
        CockpitOS-LLM-Reference.md   Master reference for AI assistants
```

---

## What is CockpitOS?

CockpitOS is ESP32 firmware for building physical cockpit panels that interface with DCS World through DCS-BIOS. It supports:

- **Inputs**: Buttons, toggle switches, rotary switches, rotary encoders, potentiometers, keypads, matrix switches
- **Outputs**: LEDs, WS2812 RGB LEDs, 7-segment displays (TM1637, GN1640T), segment LCDs (HT1622), TFT gauges, servo motors
- **Hardware expansion**: PCA9555 I2C expanders, 74HC165 shift registers, matrix scanning
- **Connectivity**: USB HID, WiFi UDP, Serial/CDC, BLE, RS485 multi-panel networking
- **Automation**: Three Python tools handle environment setup, firmware compilation, and label set creation/editing with zero manual file editing required

---

## The Three Tools

CockpitOS ships with three Python tools that automate the entire workflow:

| Tool | Script | Purpose |
|---|---|---|
| **Setup Tool** | `Setup-START.py` | Installs ESP32 core, libraries, and arduino-cli |
| **Compiler Tool** | `CockpitOS-START.py` | Compiles and uploads firmware to your board |
| **Label Creator** | `LabelCreator-START.py` | Creates and edits label sets (inputs, LEDs, displays) |

All three tools feature a terminal UI (TUI) with menus, guided editors, and contextual help. No Arduino IDE is required for compilation or upload.

Each tool can launch the others directly from its menu, so you can flow from setup to compilation to label editing without leaving the terminal.

---

## Supported Hardware

### ESP32 Boards

| Variant | USB HID | WiFi | BLE | Recommended |
|---|---|---|---|---|
| ESP32-S3 | Yes | Yes | Yes | Best all-around choice |
| ESP32-S2 | Yes | Yes | No | Good budget option |
| ESP32-P4 | Yes | Yes | No | High performance |
| ESP32 Classic | No | Yes | Yes | WiFi/Serial only |
| ESP32-C3/C5/C6 | No | Yes | Yes | Compact, RISC-V |
| ESP32-H2 | No | No | Yes | BLE only |

### Input Devices
GPIO buttons/switches, PCA9555 I2C expanders, 74HC165 shift registers, TM1637 keypads, matrix scanning, analog potentiometers

### Output Devices
GPIO LEDs, PCA9555 outputs, WS2812 RGB strips, TM1637 7-segment displays, GN1640T LED matrices, HT1622 segment LCDs, TFT gauges (LovyanGFX), servo motors (analog gauges)

---

## Getting Help

- **GitHub Discussions**: [github.com/BojoteX/CockpitOS/discussions](https://github.com/BojoteX/CockpitOS/discussions)
- **GitHub Issues**: [github.com/BojoteX/CockpitOS/issues](https://github.com/BojoteX/CockpitOS/issues)
- **FAQ**: [Reference/FAQ.md](Reference/FAQ.md)

---

*CockpitOS is open-source software. Contributions welcome!*
