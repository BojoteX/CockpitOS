# CockpitOS Firmware Project  
**For ESP32-S2, ESP32-S3, and ESP32-C3 Devices**

---

## Overview

CockpitOS is a high-performance firmware platform for building **DCS-BIOS**-compatible cockpit panels, indicators, and displays using the **ESP32** family of microcontrollers.

- **Static memory allocation only** — no dynamic allocation (`malloc`), no fragmentation, no heap usage.
- **Deterministic execution** — all main loop operations are real-time safe.
- **Optimized for low latency** — rapid response for selectors, LEDs, and gauges.
- **Aerospace-inspired engineering** — fault-tolerant, maintainable, and predictable.

---

## Key Features

- **Static lookup tables** for controls, LEDs, selectors, and displays — auto-generated for the active aircraft or instrument set.
- **Per-group selector state tracking** with dwell-time enforcement and debounce filtering.
- **Mission and aircraft synchronization** with automatic state reset on mission start/stop.
- **Multi-transport DCS-BIOS support** — USB CDC or WiFi UDP selectable in `Config.h`.
- **WiFi UDP console** for remote debugging and headless operation.
- **Built-in profiling tools** — timing metrics, loop headroom, and USB/CDC status.
- **250 Hz panel polling** and **60 Hz display refresh**.
- **Support for TFT-based gauges** via [LovyanGFX](https://github.com/lovyan03/LovyanGFX) or [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI).

---

## Requirements

- **IDE:** Arduino IDE ≥ 2.3.6  
- **ESP32 Arduino Core:** v3.3.0 (2.x also supported)  
- **Board Examples:** Wemos LOLIN S2 Mini, ESP32-S3, ESP32-C3  
- **Libraries:**  
  - [LovyanGFX](https://github.com/lovyan03/LovyanGFX) — required for TFT gauge implementations.  
  - [TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) — optional for certain TFT gauges.  
  - DCS-BIOS library (optional; a "lite" parser is included).

No support for STM32, Teensy, ESP8266, MicroPython, ESP-IDF, or PlatformIO.

---

## Directory Structure

- **`LABELS/`** — Panel label sets and mapping generators.
- **`src/`** — Core firmware code, panel modules, drivers, and mapping tables.
- **`Debug Tools/`** — Network console, DCS-BIOS replay, UDP logger.
- **`HID Manager/`** — Python tools for network-to-HID bridging.
- **`README.md`** — Project documentation.

---

## Quick Start

1. **Select label set**  
   Edit `Config.h` to set the `LABEL_SET_xxxx` macro.

2. **Edit panel list**  
   Modify `selected_panels.txt` in the chosen label set directory.

3. **Generate mapping files**  
   Run `python generate_data.py` in the label set directory.

4. **Open project in Arduino IDE**  
   Select your ESP32 board and verify Core version.

5. **Build and upload**  
   Upload firmware to the target ESP32.

6. **(Optional) Configure transport**  
   Set WiFi or USB mode in `Config.h`.

7. **(Optional) Use companion tools**  
   See `Debug Tools/` and `HID Manager/` for advanced features.

---

## Limitations

- ESP32 family only (S2, S3, C3 verified).
- Do not enable USB CDC and HID simultaneously unless explicitly configured.
- Always regenerate mapping files after modifying panel lists.
- For WiFi debug, ensure UDP broadcast/multicast is permitted on the network.

---

## License

MIT License — see `LICENSE` file.

---

Developed by the CockpitOS Firmware Project team.