# Frequently Asked Questions

---

## General

### What is CockpitOS?

CockpitOS is open-source ESP32 firmware for building physical cockpit panels that interact with DCS World. It reads switch positions, button presses, potentiometer values, and encoder rotations from your hardware, sends commands to DCS World through DCS-BIOS, and drives LEDs, 7-segment displays, TFT gauges, and servos based on cockpit state exported by DCS-BIOS.

### Which aircraft are supported?

Any aircraft that DCS-BIOS supports. CockpitOS ships with aircraft JSON definition files (in `src/LABELS/_core/aircraft/`) and the community regularly adds new ones. If DCS-BIOS has a control definition for it, CockpitOS can use it.

### Is this only for DCS World?

Yes. CockpitOS is built around the DCS-BIOS protocol, which is a LUA export script that runs inside DCS World. It does not work with other simulators natively. However, the firmware also supports a **HID gamepad mode** that presents inputs as a standard USB gamepad to the operating system, which any simulator or application can read. Enable this in the Compiler Tool under **Misc Options** > **Advanced Settings** > **HID Mode as Default**.

### What is the difference between CockpitOS and DCS-BIOS?

**DCS-BIOS** is a LUA export script that runs inside DCS World. It exports cockpit state as a binary data stream over UDP and accepts text commands to manipulate cockpit controls.

**CockpitOS** is the firmware that runs on your ESP32 hardware. It reads your physical switches and buttons, sends DCS-BIOS commands to DCS World, and processes the incoming DCS-BIOS data stream to drive your LEDs and displays.

They work together: DCS-BIOS handles the DCS World side, CockpitOS handles the hardware side.

### Do I need to be a programmer?

No. The three Python tools (Setup, Label Creator, Compiler) handle environment setup, configuration file generation, and compilation. You do not need to write code unless you are building custom panel logic.

---

## Hardware

### Which ESP32 board should I use?

**ESP32-S3** is the recommended board. It supports all transport modes (USB HID, WiFi, Serial, BLE), has dual cores, plenty of memory, and native USB-OTG for the lowest-latency HID connection.

**ESP32-S2** is a good budget option. It supports USB HID and WiFi but has less memory and a single core. Avoid enabling WiFi debug output simultaneously with USB transport on S2 -- the memory is tight.

### Can I use the original ESP32 (Classic)?

Yes, but only with WiFi or Serial transport. The original ESP32 does not have native USB-OTG, so USB HID mode is not available. It does support BLE (private builds only) and RS-485.

### How many inputs and outputs can I have?

On GPIO alone, you can use every available pin on your ESP32 board (typically 20-40 pins depending on variant). Beyond that, CockpitOS supports hardware expansion:

| Expansion Method | Adds |
|-----------------|------|
| **PCA9555** I2C expanders | 16 digital I/O per chip, up to 8 chips (128 I/O total) |
| **74HC165** shift registers | 8 digital inputs per chip, chainable |
| **TM1637** display drivers | Also support button input scanning |

### What displays are supported?

| Display Type | Description |
|-------------|-------------|
| **HT1622** | LCD segment driver (large 7-segment displays) |
| **TM1637** | 7-segment LED display + button scanning |
| **GN1640T** | LED matrix/segment driver |
| **TFT** (via LovyanGFX) | Full-color TFT screens for analog gauge rendering |
| **Servo gauges** | PWM-driven analog servo instruments |
| **WS2812** | Addressable RGB LED strips |

---

## Setup

### How do I get started?

1. **Run `Setup-START.py`** -- This installs the Arduino CLI, ESP32 board cores, and required libraries (LovyanGFX, etc.). Run it once on a fresh machine or after updates.
2. **Run `LabelCreator-START.py`** -- Create a new label set: pick your aircraft, select the cockpit panels you need, and configure your input/output wiring through the guided menus.
3. **Run `CockpitOS-START.py`** -- Select your board, COM port, and compile/upload the firmware.

See the [Quick Start Guide](../Getting-Started/Quick-Start.md) for a step-by-step walkthrough.

### What is a Label Set?

A label set is a folder inside `src/LABELS/` that contains all the configuration files for one specific panel. Each label set targets one aircraft and includes:

- `InputMapping.h` -- Physical input wiring (buttons, switches, encoders, potentiometers)
- `LEDMapping.h` -- LED and indicator output wiring
- `DisplayMapping.cpp` / `DisplayMapping.h` -- Display segment configuration
- `DCSBIOSBridgeData.h` -- DCS-BIOS address subscriptions (auto-generated)
- `LabelSetConfig.h` -- Label set metadata (name, aircraft, USB PID)
- `CustomPins.h` -- Board-level pin assignments and feature enables
- `LatchedButtons.h` -- Latched button declarations
- `CoverGates.h` -- Cover gate definitions
- `selected_panels.txt` -- Which aircraft panels are included
- `METADATA/` -- JSON overlays for custom control definitions

You can have multiple label sets (one per panel/board) and switch between them before compiling.

### How do I create a label set?

Use the **Label Creator** tool (`LabelCreator-START.py`). Select "Create New Label Set" from the main menu, pick your aircraft, choose the cockpit panels you want to replicate, and the tool generates all the mapping files automatically. You then use the built-in editors to assign GPIO pins and hardware addresses to each control.

Do **not** create label sets by manually copying folders or editing files by hand. The Label Creator manages file generation, hash tables, and cross-references that are difficult to maintain manually.

### Do I need Arduino IDE?

No. The three Python tools handle everything: the Setup Tool installs the Arduino CLI and board cores, the Label Creator generates configuration files, and the Compiler Tool compiles and uploads firmware. Arduino IDE is not required at any point.

If you prefer Arduino IDE, you *can* use it to compile and upload, but you still need the Label Creator to generate your label set files. See [Compiler Tool](../Tools/Compiler-Tool.md) for details.

### Do I need to modify the core firmware?

No. All customization is done through label sets. The core firmware files (`src/Core/`, `src/Panels/`, etc.) should not be edited. If you need custom behavior, use the `REGISTER_PANEL` system to write panel-specific code within your label set.

---

## Connectivity

### USB, WiFi, or Serial -- which should I choose?

| Transport | Latency | Reliability | Setup Complexity | Best For |
|-----------|---------|-------------|------------------|----------|
| **USB HID** | Lowest | Highest | Medium (needs HID Manager) | Production panels |
| **WiFi** | Low | Good | Low (no bridge software) | Wireless panels, getting started |
| **Serial** | Low | Good | Medium (needs socat) | Legacy setups |
| **BLE** | Medium | Fair | Low | Internal/experimental |

**Recommendation:** Use USB HID for production panels on S2/S3/P4 boards. Use WiFi when getting started or when you need wireless operation. See [Transport Modes](Transport-Modes.md) for full details.

### What is HID Manager?

HID Manager is a Python companion application that runs on your PC. It acts as a bridge between your USB HID ESP32 devices and DCS-BIOS. When using `USE_DCSBIOS_USB=1`, HID Manager is required on the host PC to relay data between DCS-BIOS (UDP) and your panels (USB HID).

Key details:
- Supports up to 32 simultaneous panels
- Auto-discovers devices by `USB_VID` (default `0xCAFE`)
- Runs as a background process
- Configuration is in `settings.ini`

### Can I use multiple panels at once?

Yes. Each panel is a separate ESP32 with its own label set. With USB HID transport, the HID Manager handles up to 32 panels simultaneously. With WiFi transport, each panel independently connects to the DCS-BIOS multicast group -- there is no practical limit.

For hardwired multi-panel setups, consider RS-485 networking: one master ESP32 connects to DCS-BIOS and forwards data to multiple slave ESP32 boards over an RS-485 bus.

### Why is my panel not connecting?

Common causes, in order of likelihood:

1. **Wrong transport mode** -- Use the Compiler Tool's **Role / Transport** menu to verify the correct transport is selected
2. **DCS-BIOS not running** -- Make sure the DCS-BIOS mod is installed and a mission is loaded
3. **USB cable** -- Many USB cables are charge-only with no data lines. Try a different cable.
4. **HID Manager not running** (USB mode) -- The HID Manager must be running on the PC
5. **WiFi credentials** (WiFi mode) -- SSID and password are case-sensitive; must be 2.4 GHz WPA2-PSK
6. **Firewall** (WiFi mode) -- Windows Firewall may block UDP ports 5010 and 7778
7. **USB CDC On Boot** -- Must be set to **Disabled** in Arduino IDE Tools menu (or the Compiler Tool handles this automatically)

See [Troubleshooting](Troubleshooting.md) for detailed diagnostic steps.

---

## The Tools

### What are the three tools?

| Tool | Entry Point | Purpose |
|------|-------------|---------|
| **Setup Tool** | `Setup-START.py` | Installs Arduino CLI, ESP32 board cores, and libraries. Run once or after updates. |
| **Compiler Tool** | `CockpitOS-START.py` | Compiles firmware and uploads to your ESP32. Selects board, port, and label set. |
| **Label Creator** | `LabelCreator-START.py` | Creates and edits label sets. Manages input/output mappings, displays, and panel selection. |

See the [Tools Overview](../Tools/README.md) for detailed documentation on each tool.

### Can I still use Arduino IDE?

Yes, but it is not recommended. The Compiler Tool uses the Arduino CLI under the hood and handles all board settings, partition schemes, and build flags automatically. If you use Arduino IDE, you must manually configure:

- Board type and variant
- USB Mode: USB-OTG (TinyUSB) for S3/P4
- USB CDC On Boot: Disabled
- Partition Scheme: appropriate for your board
- Upload Speed

The Label Creator is still needed to generate label set files regardless of which compilation method you use.

### Can I switch between tools?

Yes. Each tool's main menu includes a "Switch Tool" option that launches another tool in the same console window. You can go from Label Creator to Compiler to Setup and back without closing windows or navigating directories.

---

## See Also

- [Quick Start Guide](../Getting-Started/Quick-Start.md) -- Build your first panel
- [Config.h Reference](Config.md) -- All configuration settings
- [Transport Modes](Transport-Modes.md) -- Detailed transport comparison
- [Troubleshooting](Troubleshooting.md) -- Solving common problems
- [DCS-BIOS Integration](DCS-BIOS.md) -- How CockpitOS talks to DCS World

---

*CockpitOS FAQ | Last updated: February 2026*
