# 🛩️ CockpitOS — High‑Performance Firmware for ESP32 Cockpit Panels
**Supported MCUs:** ESP32, ESP32‑S2, ESP32‑S3, ESP32‑C3, ESP32‑C6, ESP32‑H2, ESP32‑P4  
**Protocol:** [DCS‑BIOS — Skunkworks fork](https://github.com/DCS-Skunkworks/dcs-bios)

![CockpitOS Logo](./CockpitOS_logo.png)

---

## Table of Contents
- [Overview](#overview)
- [Why CockpitOS](#why-cockpitos)
- [Key Features](#key-features)
- [Architecture](#architecture)
- [Hardware Support](#hardware-support)
- [Requirements](#requirements)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Data Generation Pipeline](#data-generation-pipeline)
- [DCS‑BIOS Integration](#dcs-bios-integration)
- [Display Subsystems](#display-subsystems)
  - [HT1622 LCD Driver](#ht1622-lcd-driver)
  - [SPI TFT Gauges: Dirty‑Rect Compose + Region DMA Flush](#spi-tft-gauges-dirty-rect-compose--region-dma-flush)
- [Performance & Determinism](#performance--determinism)
- [Safety‑Critical Discipline](#safety-critical-discipline)
- [Directory Structure](#directory-structure)
- [Examples](#examples)
- [Troubleshooting](#troubleshooting)
- [FAQ](#faq)
- [Contributing](#contributing)
- [License](#license)

---

## Overview
CockpitOS is a high‑performance firmware platform for building **DCS‑BIOS‑compatible** cockpit panels, indicators, and displays on the **ESP32** family. It is designed for **deterministic execution**, **static memory**, and **fault‑tolerant** operation while keeping code clean, modular, and maintainable.

**Core goals:** zero heap, no blocking, predictable timing, and scalable performance from a handful of switches to thousands of LEDs and multi‑display panels.

---

## Key Features
- **Static lookup tables** for inputs, LEDs, and displays — auto‑generated per aircraft or panel set.
- **Group‑aware selectors** with exclusivity enforcement, debouncing, and dwell‑time control.
- **Mission & aircraft sync** — clean state handling on mission start/stop.
- **Multi‑transport DCS‑BIOS** — USB, Serial or Wi‑Fi selectable in `Config.h`.
- **Headless debugging** — UDP console for remote logs without disturbing USB CDC.
- **Profiling hooks** — monitor loop time, CPU headroom, CDC/HID readiness.
- **High‑rate operation** — typical 250 Hz panel polling, 60 Hz display refresh.
- **TFT gauge support** — SPI only, with dirty‑rectangle composition and DMA region flush for extreme speed.
- **No heap, no exceptions, no RTTI** — predictable memory and control flow.
- **Perfect‑hash dispatch** — constant‑time label→index mapping without `std::unordered_map`.

---

## Architecture
Modules are designed for **static data**, **non‑blocking I/O**, and **O(1)** lookups.

- **DCSBIOSBridge**  
  Parses the binary stream, surfaces state changes, and handles ASCII command TX.
- **InputMapping[] (auto‑generated)**  
  Single authoritative table for all inputs: selectors, buttons, rotaries, analogs. Includes group IDs and DCS command metadata.
- **LEDMapping[] / panelLEDs[] (auto‑generated)**  
  Routing for GPIO, PCA9555, WS2812, TM1637, GN1640T, HT1622 segments, etc.
- **LEDControl**  
  `setLED(label, state, intensity)` with perfect‑hash index + zero‑heap fan‑out.
- **HIDManager**  
  Centralized Input Mapping (HID or DCS Commands) with non‑blocking completion callback tracking.
- **USB CDC**  
  Non‑blocking ring buffer. Robust under RX flood.
- **Display Subsystems**  
  - HT1622 driver with timing‑accurate GPIO protocol, dirty‑only nibble commits.  
  - SPI TFT pipeline with dirty‑rect compose + DMA region flush.

---

## Hardware Support
**MCUs:** ESP32, S2, S3, C3, C6, H2, P4 (native USB strongly recommended).  
**I/O & Displays:**  
- **Inputs:** PCA9555 (I²C GPIO expander), 74HC165 (parallel‑in shift), matrix rotary scanning.  
- **LED/Segment:** GPIO PWM, PCA9555, TM1637, GN1640T, WS2812.  
- **LCD/TFT:** HT1622 segment LCDs; SPI TFTs (e.g., GC9A01, ST77xx) via compatible libraries.  

---

## Requirements
- **IDE:** Arduino IDE ≥ 2.3.6
- **ESP32 Arduino Core:** ≥ 3.2.x
- **Boards validated by users:** LOLIN S2 Mini, LOLIN S3 Mini (All ESP32 boards likely compatible)
- **Libraries:**
  - **LovyanGFX** for advanced TFT gauges (required for TFT path)
  - **DCS‑BIOS Skunkworks** (optional, a lite parser is included)

> Not tested: STM32, Teensy, ESP8266, MicroPython, ESP‑IDF, PlatformIO.

---

## Quick Start
1. **Open in Arduino IDE**  
   Make sure "esp32 by Espressif Systems" is installed (check Boards Manager) ensure ESP32 core ≥ 3.3.x is selected. Once installed, go into boards manager and select your board (e.g., LOLIN S2 Mini).
2. **Choose labels / aircraft**  
   Select an existing label set under `LABELS/` for your aircraft or panel subset. Preferably, use LABEL_SET_TEST_ONLY so you familiarize yourself with how this works.
3. **Generate data**  
   ```bash
   cd LABELS/<YOUR_LABEL_SET>
   Check InputMapping.h and LEDMapping.h, notice the GPIO assignments. Check tutorial on LABELS directory to understand how to modify the files.
   After you modify the files, run "python generate_data.py" to make this LABEL SET default.
   ```
4. **Configure transport**  
   Now go back to Arduino IDE, Select the Config.h tab and choose either USB, Serial or Wi‑Fi mode.
5. **Build & upload**  
   Compile and Upload. Power‑cycle the board
6. **Test**  
   Wire a simple button or LED to any of your board available GPIOs. Make sure they match what you selected in InputMapping.h (Buttons & Switches) and LEDMappings (LEDs) 
7. **Expand**  
   Now repeat all the above steps, starting in #3, this time, check selected_panels.txt. That file is what tells the auto-generator what panels to include in InputMapping.h and LEDMapping.h
8. **Ask our Custom trained GPT if having issues**  
   Check the link on our Github page. A Custom GPT has been trained with the entire codebase, it will answer any question and help you step by step if confused.

> Tip: Check the README.md inside the LABELS directory, including the quick guides for InputMapping.h and LEDMapping.h editing.

---

## Data Generation Pipeline
The generator ingests Skunkworks DCS‑BIOS definitions (JSON/Lua) and emits static, compile‑time artifacts:

- **`InputMapping.h/.cpp`** — one row per control with: label, source, port/bit, group ID, DCS command, max value, HID ID, flags.  
- **`LEDMapping.h/.cpp`** — per‑LED routing and device type.  

**Design choices:**  
- No dynamic memory.  
- Provide a minimal perfect hash for O(1) label→index mapping with no collisions.

---

## DCS‑BIOS Integration
**RX (binary):** Protocol parser processes address/mask/shift records → triggers IntegerBuffer/StringBuffer callbacks.  
**TX (ASCII):** Commands like `"HMD_OFF_BRT 35500\n"` are queued in a non‑blocking ring buffer.

---

### SPI TFT Gauges: Dirty‑Rect Compose + Region DMA Flush
A high‑performance SPI pipeline that updates only the pixels that changed.

**Pipeline:**  
1) **Detect change** vs `lastValue` with thresholds to avoid micro‑jitter.  
2) **Compute dirty rectangles** and merge when area permits.  
3) **Compose** sprites/primitives into a scratch buffer (integer/fixed‑point only).  
4) **DMA flush** each rect to the TFT window.  
5) **Bookkeeping** updates `lastValue` and history for rate limiting.

---

## Performance & Determinism
- **Lookup speed:** perfect‑hash label routing, modulo‑free arithmetic, O(1) dispatch.  
- **I/O:** cached I²C reads; single‑edge decoding for 74HC165; matrix rotaries with deterministic scan windows.  

---

## Safety‑Critical Discipline we followed
- Treat every code path as mission‑critical.  
- No unbounded loops without iteration caps.  
- Always check buffer bounds and return codes.  
- Prefer tables and bit‑wise ops over floating point.  
- Use `constexpr` and `static_assert` to lock invariants at compile time.

---

## Directory Structure
```
/LABELS/                 # Aircraft/panel label sets, generators, selector comments
/src/                    # Core firmware, panels, drivers, static tables
/Debug Tools/            # UDP console, DCS-BIOS replay scripts, log utilities
/HID Manager/            # Host-side tools for HID bridging (optional)
/README.md               # This file
/LICENSE                 # MIT
```

---

## FAQ

**Q: Which ESP32 board is best?**  
A: ESP32‑S2/S3 with native USB simplifies integration and testing.

**Q: Can I run Wi‑Fi and USB simultaneously?**  
A: Yes, but only if your board has the memory to spare AND WiFi is used for logs/debug only.

**Q: How do I map a new panel?**  
A: Add rows to your label set, run the generator, wire inputs/outputs, compile, and test with DCS or replay logs. Check the README.md inside LABELS

---

## License
MIT — see `LICENSE`.

> CockpitOS targets hobby simulation use. Rigor is applied, but this is **not** certified avionics software. You can use and distribute free of charge even for commercial products, just ensure you read our LICENSE

---

Developed by the CockpitOS Firmware Project.