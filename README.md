<p align="center">
  <img src="./CockpitOS_logo_small.png" alt="CockpitOS" width="400">
</p>

<h3 align="center">Build real cockpit panels for DCS World</h3>

<p align="center">
  ESP32 firmware that connects physical switches, LEDs, displays, and gauges<br>
  to DCS World through the DCS-BIOS protocol. No programming required.
</p>

<p align="center">
  <a href="https://github.com/BojoteX/CockpitOS/actions/workflows/ci-build.yml"><img src="https://github.com/BojoteX/CockpitOS/actions/workflows/ci-build.yml/badge.svg" alt="Build"></a>
  <a href="https://github.com/BojoteX/CockpitOS/releases/latest"><img src="https://img.shields.io/github/v/release/BojoteX/CockpitOS?label=latest" alt="Release"></a>
  <a href="https://github.com/BojoteX/CockpitOS/blob/main/LICENSE"><img src="https://img.shields.io/github/license/BojoteX/CockpitOS" alt="License"></a>
  <a href="https://github.com/BojoteX/CockpitOS/stargazers"><img src="https://img.shields.io/github/stars/BojoteX/CockpitOS?style=flat" alt="Stars"></a>
</p>

---

## Quick Start

Open a terminal (CMD or PowerShell) and paste:

```
powershell -c "irm https://bojotex.github.io/CockpitOS/install.ps1 | iex"
```

That's it. The installer handles Python, downloads the latest release, and launches the Setup tool.

> **Prefer a manual install?** Install [Python 3.10+](https://www.python.org/downloads/), [download CockpitOS](https://github.com/BojoteX/CockpitOS/releases/latest/download/CockpitOS.zip), unzip, and double-click `START.py`.

---

## What is CockpitOS?

CockpitOS is ESP32 firmware that turns off-the-shelf dev boards into fully functional DCS World cockpit controllers. It runs natively across the entire ESP32 family -- Classic, C3, C5, C6, P4, S2, and S3 -- and supports buttons, switches, encoders, LEDs, segment displays, TFT gauges, and stepper motors out of the box.

Three bundled Python tools handle the entire workflow -- setup, compilation, and panel configuration -- so you never touch the Arduino IDE or edit source files by hand.

Think of it as the [DCS-BIOS Arduino Library](https://github.com/DCS-Skunkworks/dcs-bios-arduino-library) reimagined for performance and scale on ESP32.

---

## Features

<table>
<tr>
<td width="50%" valign="top">

**Inputs**
- Buttons, toggles, rotary encoders, multi-position selectors
- Analog axes with self-calibration
- I2C expanders (PCA9555), shift registers (74HC165)
- Matrix scanning for rotary switches
- Built-in debouncing and edge detection

</td>
<td width="50%" valign="top">

**Outputs**
- GPIO LEDs with PWM dimming
- WS2812 addressable RGB LEDs
- TM1637 and GN1640T LED segment drivers
- HT1622 segment LCD displays (IFEI, UFC, etc.)
- SPI/RGB TFT gauges via LovyanGFX
- Stepper motors (28BYJ-48, X27.168)

</td>
</tr>
<tr>
<td width="50%" valign="top">

**Connectivity**
- USB HID (recommended) -- no drivers needed
- WiFi UDP -- for wireless panels
- BLE -- Bluetooth Low Energy
- Serial -- legacy support (socat)
- RS485 -- multi-device bus networking

</td>
<td width="50%" valign="top">

**Architecture**
- Static memory allocation -- zero heap fragmentation
- Non-blocking I/O throughout -- no watchdog resets
- O(1) label lookups via perfect hashing
- 250 Hz input polling, 30-60 Hz display refresh
- Per-aircraft configuration via Label Sets

</td>
</tr>
</table>

---

## Supported Hardware

| MCU | USB | WiFi / BLE | Status |
|-----|-----|------------|--------|
| **ESP32-S2** | Native USB | WiFi only | Recommended |
| **ESP32-S3** | Native USB | WiFi + BLE | Recommended |
| ESP32 (Classic) | -- | WiFi + BLE | Serial/WiFi only |
| ESP32-C3 / C6 | -- | WiFi + BLE | Serial/WiFi only |
| ESP32-P4 | Native USB | -- | USB/Serial only |

Popular boards: **LOLIN S2 Mini**, **LOLIN S3 Mini**, or any S2/S3 dev board.

---

## How It Works

```
 DCS World  ──>  DCS-BIOS (Lua)  ──>  CockpitOS (ESP32)  ──>  Your Panel
     ^                                        |
     └────────────────────────────────────────┘
                    Commands flow back
```

[DCS-BIOS](https://github.com/DCS-Skunkworks/dcs-bios) exports cockpit state from the simulator as a binary stream. CockpitOS receives this data and drives your physical hardware -- LEDs light up, displays update, gauges move. When you flip a switch, CockpitOS sends the command back through DCS-BIOS to the sim.

---

## Bundled Tools

| Tool | What it does |
|------|-------------|
| **Setup Tool** (`Setup-START.py`) | Installs ESP32 board support and libraries via bundled arduino-cli |
| **Compiler** (`CockpitOS-START.py`) | Compiles and uploads firmware -- select board, transport, label set |
| **Label Creator** (`LabelCreator-START.py`) | Visual editors for inputs, LEDs, displays, segments, and pins |

All tools are Windows-native TUI applications. They switch between each other seamlessly.

---

## Label Sets

Label Sets define your panel's configuration. Each one lives in `src/LABELS/LABEL_SET_<name>/` and contains:

| File | Purpose |
|------|---------|
| `InputMapping.h` | Buttons, switches, encoders -- their pins and DCS-BIOS commands |
| `LEDMapping.h` | LEDs and indicators -- hardware type and DCS-BIOS bindings |
| `DisplayMapping.cpp/h` | Display field definitions for segment LCDs and TFTs |
| `CustomPins.h` | Pin assignments and feature enables |
| `LabelSetConfig.h` | Device name, USB PID, panel metadata |

Label sets are created and edited using the **Label Creator** tool. Hash tables and runtime data are auto-generated.

---

## Documentation

| Guide | Description |
|-------|-------------|
| [Getting Started](Docs/Getting-Started/README.md) | Environment setup guide |
| [Quick Start Guide](Docs/Getting-Started/Quick-Start.md) | Build your first panel in 30 minutes |
| [Tools Overview](Docs/Tools/README.md) | Setup Tool, Compiler, Label Creator |
| [Hardware Guides](Docs/Hardware/README.md) | Wiring for all input/output types |
| [How-To Guides](Docs/How-To/README.md) | Step-by-step tutorials |
| [Config Reference](Docs/Reference/Config.md) | Complete Config.h flag reference |
| [Transport Modes](Docs/Reference/Transport-Modes.md) | USB, WiFi, Serial, BLE configuration |
| [DCS-BIOS Integration](Docs/Reference/DCS-BIOS.md) | Protocol details and debug tools |
| [Advanced Topics](Docs/Advanced/Custom-Panels.md) | Custom panels, CoverGate, display pipeline |
| [FAQ & Troubleshooting](Docs/Reference/FAQ.md) | Common questions and solutions |

---

## Need Help?

| Resource | Link |
|----------|------|
| AI Assistant | [Chat with CockpitOS Assistant](https://go.bojote.com/CockpitOS) -- knows the entire codebase |
| Web Uploader | [Flash firmware from the browser](https://cockpitos.bojote.com/upload/?v=1) |

---

## Project Structure

```
CockpitOS/
+-- CockpitOS.ino          # Entry point
+-- Config.h               # Master config (transport, debug, timing)
+-- Mappings.cpp/h         # Panel init/loop orchestration
+-- src/
|   +-- Core/              # DCSBIOSBridge, HIDManager, LEDControl, InputControl
|   +-- Panels/            # Panel implementations (IFEI, TFT gauges, etc.)
|   +-- Generated/         # Auto-generated PanelKind.h
|   +-- LABELS/            # Aircraft/panel configurations (one folder per label set)
|       +-- _core/         # Generator modules and aircraft JSON data
+-- compiler/              # Compiler tool (cockpitos.py)
+-- label_creator/         # Label Creator tool (label_creator.py)
+-- lib/CUtils/            # Hardware drivers (GPIO, I2C, displays, steppers)
+-- HID Manager/           # PC-side USB HID bridge
+-- Debug Tools/           # UDP console, stream recorder/player
+-- Docs/                  # Full documentation
```

---

## Requirements

- **Windows 10/11**
- **[Python 3.12+](https://www.python.org/downloads/)** (or from the [Microsoft Store](https://apps.microsoft.com/detail/9pnrbtzxmb4z))
- **[DCS World](https://www.digitalcombatsimulator.com/)** with **[DCS-BIOS](https://github.com/DCS-Skunkworks/dcs-bios/releases)** installed

Everything else is installed automatically by the Setup Tool.

---

## Design Principles

- **No dynamic memory** -- All buffers statically allocated, no heap fragmentation over long sessions
- **No blocking calls** -- State machines and interrupts, never `delay()`
- **Bounded execution** -- All loops have iteration limits, watchdog-safe
- **Fail-safe defaults** -- Graceful handling of disconnections, transport loss, and protocol errors

---

<details>
<summary><strong>For Developers</strong> -- Internal architecture details</summary>

### Protocol Implementation

CockpitOS adapts to DCS-BIOS, it doesn't reinvent it. We kept the original `protocol.cpp` state machine intact -- the same sync-frame detection, address parsing, and delta-compressed 16-bit word handling. The wire format is efficient; we saw no reason to change it.

### Transport Abstraction

Panel logic (inputs, LEDs, displays) is fully decoupled from the data transport:

```
+------------------------------------------------------------------+
|                         PANEL LOGIC                               |
|          (subscriptions, callbacks, hardware drivers)             |
+------+-------+--------+--------+--------+-----------+
       |       |        |        |        |
   USB HID    BLE   WiFi UDP   Serial   RS485
```

Swapping transports requires no panel code changes. This also means the architecture could theoretically adapt to different protocols -- if another sim exposed a binary stream with address/value pairs, you'd swap the transport layer, not rewrite panels.

### Selective Subscriptions (Label Sets)

Traditional DCS-BIOS clients receive the entire aircraft state -- every address, every update, whether you need it or not. CockpitOS inverts this:

- Each **Label Set** defines which DCS-BIOS addresses the panel cares about
- At compile time, hash tables are generated for O(1) address lookup
- At runtime, irrelevant addresses are skipped in microseconds
- Result: A panel with 50 controls doesn't process 2,000+ addresses per frame

This matters for multi-panel pits where each ESP32 handles a specific section.

### USB HID Approach

Native USB on ESP32-S2/S3 eliminates socat and virtual COM ports entirely:

- Device exposes as HID gamepad with **Feature Reports** as a bidirectional mailbox
- `HID_Manager.py` polls Feature Reports and relays to/from DCS-BIOS UDP
- Lightweight: **<1% CPU** on Raspberry Pi, tested with **20+ devices** on Windows 11
- No COM port enumeration, no driver issues, no socat configuration

Trade-off: USB has hub/controller limits that serial doesn't. For typical pit builds (5-15 panels), USB is simpler. For 30+ devices, serial or RS485 scales better.

### Why This Exists

The [DCS-BIOS Arduino Library](https://github.com/DCS-Skunkworks/dcs-bios-arduino-library) is excellent for getting started. CockpitOS exists because we needed:

- Native USB support (not available on classic Arduino)
- WiFi and BLE transports for wireless panels
- Static memory model for long session stability
- Per-panel address filtering at scale
- RS485 bus networking for large pit builds

We're not replacing DCS-BIOS -- we're providing an alternative client implementation that respects the protocol while expanding hardware and transport options.

</details>

---

## License

MIT -- See [LICENSE](LICENSE)

Free for personal and commercial use. Not certified for actual aircraft.

---

## Acknowledgments

- [DCS-BIOS Skunkworks](https://github.com/DCS-Skunkworks/dcs-bios) -- The protocol that makes this possible
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) -- TFT display library
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) -- Lightweight BLE stack for ESP32
- [DCS-BIOS Arduino Library](https://github.com/DCS-Skunkworks/dcs-bios-arduino-library) -- The library that inspired this project

---

*Built for cockpit builders, by a cockpit builder.*
