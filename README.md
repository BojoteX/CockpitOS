
# The CockpitOS firmware project (ESP32)

**ESP32 firmware for DCS World cockpit panels connected via DCS-BIOS protocol**

![CockpitOS Logo](./CockpitOS_logo_small.png)

## Quick Start

**1. Install Python** -- [Download Python for Windows](https://www.python.org/downloads/) (click the big yellow button, check **"Add Python to PATH"** during install)

**2. Download CockpitOS** -- [Download Latest Release](https://github.com/BojoteX/CockpitOS/releases/latest/download/CockpitOS.zip)

**3. Unzip anywhere, then double-click:**

| Step | Script | What it does |
|------|--------|-------------|
| First time | `Setup-START.py` | Installs ESP32 core, libraries, and DCS-BIOS |
| Configure | `LabelCreator-START.py` | Create a label set for your aircraft and panels |
| Build | `CockpitOS-START.py` | Compile and upload firmware to your ESP32 |

> For USB mode, also run `HID Manager/HID_Manager.py` on your PC to bridge USB to DCS-BIOS.

No Arduino IDE required. All three tools are guided and switch between each other.

---

## What is CockpitOS?

CockpitOS connects physical cockpit hardware/panels to DCS World via the [DCS-BIOS protocol](https://github.com/DCS-Skunkworks/dcs-bios). It runs natively across the entire ESP32 family -- Classic, C3, C5, C6, P4, S2, and S3 -- supporting buttons, switches, encoders, LEDs, TFT displays, and segment displays out of the box. With transport options spanning legacy Serial (socat), Wi-Fi, BLE, and native USB, CockpitOS delivers the flexibility modern cockpit builders demand. Think of it as the [DCS-BIOS Arduino Library](https://github.com/DCS-Skunkworks/dcs-bios-arduino-library) -- reimagined for performance and scale on ESP32 devices.

---

## Features

**Inputs**
- Buttons, toggle switches, rotary encoders, multi-position selectors
- Analog axes with self-calibration
- I2C expanders (PCA9555) and shift registers (74HC165) for high pin counts
- Matrix scanning for rotary switches
- Debouncing and edge detection built-in

**Outputs**
- GPIO LEDs with PWM dimming
- WS2812 addressable RGB LEDs
- TM1637 and GN1640T LED drivers
- HT1622 segment LCD displays (IFEI, UFC, etc.)
- SPI TFT gauges via LovyanGFX

**Connectivity**
- USB HID (recommended) -- works with included HID Manager
- WiFi UDP -- for wireless panels
- BLE -- Bluetooth Low Energy transport
- Serial -- legacy support (socat)

**Architecture**
- Static memory allocation -- no heap fragmentation
- Non-blocking I/O throughout
- O(1) label lookups via perfect hashing
- 250 Hz input polling, 30-60 Hz display refresh
- Per-aircraft configuration via Label Sets

---

## Supported Hardware

| MCU | Status |
|-----|--------|
| ESP32-S2 | Recommended (native USB) |
| ESP32-S3 | Recommended (native USB) |
| ESP32 (original) | Works (Serial/WiFi only) |
| ESP32-C3 and C6  | Works (Serial/WiFi only) |

Popular boards: LOLIN S2 Mini, LOLIN S3 Mini (or any other Classic, S2 or S3 dev board from Amazon)

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
| AI Assistant | [Chat with CockpitOS Assistant](https://go.bojote.com/CockpitOS) -- knows the entire codebase, can guide you through any task |
| Firmware Uploader | [Web-based uploader](https://cockpitos.bojote.com/upload/?v=1) -- flash firmware directly from the browser |

---

## Project Structure

```
CockpitOS/
├── CockpitOS.ino          # Entry point
├── Config.h               # Master config (transport, debug, timing)
├── Mappings.cpp/h         # Panel init/loop orchestration
├── src/
│   ├── Core/              # DCSBIOSBridge, HIDManager, LEDControl, InputControl
│   ├── Panels/            # Panel implementations (IFEI, WingFold, TFT gauges, etc.)
│   ├── Generated/         # Auto-generated PanelKind.h
│   └── LABELS/            # Aircraft/panel configurations (one folder per label set)
│       └── _core/         # Generator modules and aircraft JSON data
├── compiler/              # Compiler tool (cockpitos.py)
├── label_creator/         # Label Creator tool (label_creator.py)
├── lib/CUtils/            # Hardware drivers (GPIO, I2C, displays)
├── HID Manager/           # PC-side USB HID bridge
├── Debug Tools/           # UDP console, stream recorder/player
└── Docs/                  # Documentation (Getting-Started, Tools, Hardware, etc.)
```

---

## How It Works

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  DCS World  │───▶│  DCS-BIOS   │────▶│  CockpitOS  │────▶│  Hardware   │
│             │◀───│  (LUA)      │◀────│  (ESP32)    │◀────│  (Panel)    │
└─────────────┘     └─────────────┘     └─────────────┘     └─────────────┘
     Sim              Export/Import       Firmware           Physical I/O
```

DCS-BIOS exports cockpit state from the simulator. CockpitOS receives this data and drives your physical hardware. When you flip a switch, CockpitOS sends the command back through DCS-BIOS to the simulator.

---

## Label Sets

Label Sets define your panel's configuration. Each label set lives in `src/LABELS/LABEL_SET_<name>/` and contains:

- **InputMapping.h** -- Buttons, switches, encoders, their GPIO pins, and DCS-BIOS commands
- **LEDMapping.h** -- LEDs, their hardware type, and which DCS-BIOS indicators they represent
- **DisplayMapping.cpp/h** -- Display field definitions for segment LCDs and TFTs
- **CustomPins.h** -- Pin assignments and feature enables
- **LabelSetConfig.h** -- Device name, USB PID, panel metadata

Label sets are created and edited using the **Label Creator** tool. Auto-generation handles all hash tables and runtime data.

---

## Requirements

- Windows 10/11
- [Python 3.12+](https://www.python.org/downloads/)
- [DCS World](https://www.digitalcombatsimulator.com/) with [DCS-BIOS](https://github.com/DCS-Skunkworks/dcs-bios/releases) installed

Everything else is installed automatically by the Setup Tool.

---

## Design Principles

CockpitOS follows embedded best practices:

- **No dynamic memory** -- All buffers statically allocated
- **No blocking calls** -- State machines and interrupts instead of delays
- **Bounded execution** -- All loops have iteration limits
- **Fail-safe defaults** -- Graceful handling of disconnections and errors

This isn't certified avionics software, but it's built with reliability in mind for long simulation sessions.

---

## For Developers

*This section covers internal architecture. Skip if you just want to build panels.*

### Protocol Implementation

CockpitOS adapts to DCS-BIOS, it doesn't reinvent it. We kept the original `protocol.cpp` state machine intact -- the same sync-frame detection, address parsing, and delta-compressed 16-bit word handling. The wire format is efficient; we saw no reason to change it.

### Transport Abstraction

Panel logic (inputs, LEDs, displays) is fully decoupled from the data transport:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            PANEL LOGIC                                      │
│             (subscriptions, callbacks, hardware drivers)                     │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
        ┌──────────┬───────────┼───────────┬──────────┐
        ▼          ▼           ▼           ▼          ▼
   ┌─────────┐ ┌───────┐ ┌───────────┐ ┌──────┐ ┌──────────┐
   │ USB HID │ │  BLE  │ │ WiFi UDP  │ │Serial│ │  RS485   │
   └─────────┘ └───────┘ └───────────┘ └──────┘ └──────────┘
```

Swapping transports requires no panel code changes. This also means the architecture could theoretically adapt to different protocols -- if another sim exposed a binary stream with address/value pairs, we'd swap the transport layer, not rewrite panels.

### Selective Subscriptions (Label Sets)

Traditional DCS-BIOS clients receive the entire aircraft state -- every address, every update, whether you need it or not. CockpitOS inverts this:

- Each **Label Set** defines which DCS-BIOS addresses the panel cares about
- At compile time, we generate hash tables for O(1) address lookup
- At runtime, irrelevant addresses are skipped in microseconds
- Result: A panel with 50 controls doesn't process 2,000+ addresses per frame

This matters for multi-panel pits where each ESP32 handles a specific section.

### USB HID Approach

Native USB on ESP32-S2/S3 eliminates socat and virtual COM ports entirely:

- Device exposes as HID gamepad with **Feature Reports** as a bidirectional mailbox
- `HID_Manager.py` polls Feature Reports and relays to/from DCS-BIOS UDP
- Lightweight: **<1% CPU** on Raspberry Pi, tested with **20+ devices** on Windows 11
- No COM port enumeration, no driver issues, no socat configuration

Trade-off: USB has hub/controller limits that serial doesn't. For typical pit builds (5-15 panels), USB is simpler. For 30+ devices, serial scales better.

### Why This Exists

The [DCS-BIOS Arduino Library](https://github.com/DCS-Skunkworks/dcs-bios-arduino-library) is excellent for getting started. CockpitOS exists because we needed:

- Native USB support (not available on classic Arduino)
- WiFi transport for wireless panels
- Static memory model for long session stability
- Per-panel address filtering at scale

We're not replacing DCS-BIOS -- we're providing an alternative client implementation that respects the protocol while expanding hardware and transport options.

---

## License

MIT -- See [LICENSE](LICENSE)

Free for personal and commercial use. Not certified for actual aircraft.

---

## Acknowledgments

- [DCS-BIOS Skunkworks](https://github.com/DCS-Skunkworks/dcs-bios) -- The protocol that makes this possible
- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) -- The TFT library that powers our displays
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino) -- Lightweight BLE stack for ESP32
- [DCS-BIOS Arduino Library](https://github.com/DCS-Skunkworks/dcs-bios-arduino-library) -- The library that inspired our project

---

*Built by the CockpitOS Project Dev Team.*
