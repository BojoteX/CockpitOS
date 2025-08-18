# 🛩 CockpitOS Firmware Project  
**For ESP32-S2, ESP32-S3, and ESP32-C3 Devices**  

![CockpitOS Logo](./CockpitOS_logo.png)

---

## 📖 Overview

CockpitOS is a high-performance firmware platform for building **DCS-BIOS**-compatible cockpit panels, indicators, and displays on the **ESP32** family of microcontrollers.

- 🗂 **Static memory allocation only** — no `malloc`, no heap usage (except for TFT gauges), no fragmentation.  
- ⏱ **Deterministic execution** — all main loop operations are real-time safe.  
- ⚡ **Low-latency design** — fast selector, LED, and gauge updates.  
- 🛡 **Aerospace-inspired engineering** — fault-tolerant, predictable, blazing fast & maintainable.  

---

## ✨ Key Features

- 📋 **Static lookup tables** for controls, LEDs, selectors, and displays — auto-generated for the active aircraft or instrument set.  
- 🎛 **Per-group selector tracking** — dwell-time enforcement and debounce filtering for stable input.  
- 🛫 **Mission & aircraft sync** — auto-reset states on mission start/stop.  
- 🔌 **Multi-transport DCS-BIOS** — USB CDC or WiFi UDP selectable in `Config.h`.  
- 📡 **WiFi UDP console** — remote debug for headless operation.  
- 📊 **Profiling tools** — track loop timing, CPU headroom, and USB/CDC status.  
- ⏩ **High-rate operation** — 250 Hz panel polling and 60 Hz display refresh.  
- 🖥 **TFT gauge support** — powered by [LovyanGFX](https://github.com/lovyan03/LovyanGFX).  

---

## 🛠 Requirements

- **IDE:** Arduino IDE ≥ 2.3.6  
- **ESP32 Arduino Core:** v3.2.1 (2.x also supported)  
- **ESP32 Boards Tested:** LOLIN S2 Mini, LOLIN S3 Mini, LOLIN C3 Mini (All boards by WEMOS)  
- **Libraries:**  
  - [LovyanGFX](https://github.com/lovyan03/LovyanGFX) — **required** for TFT gauge implementations.  
  - DCS-BIOS library (optional; a "lite" parser is included).  

❌ Not tested: STM32, Teensy, ESP8266, MicroPython, ESP-IDF, PlatformIO.

---

## 📂 Directory Structure

- **`LABELS/`** — panel label sets and mapping generators.  
- **`src/`** — core firmware code, panel modules, drivers, mapping tables.  
- **`Debug Tools/`** — network console, DCS-BIOS replay, UDP logger.  
- **`HID Manager/`** — Python tools for network-to-HID bridging.  
- **`README.md`** — project documentation.  

---

## 🚀 Quick Start

1. **Select label set**  
   Set `LABEL_SET_xxxx` in `Config.h`.  
2. **Edit panel list**  
   Update `selected_panels.txt` in your label set directory.  
3. **Generate mappings**  
   Run `python generate_data.py` inside the label set directory.  
4. **Open in Arduino IDE**  
   Select your ESP32 board and verify core version.  
5. **Build & upload**  
   Upload firmware to your ESP32 target.  
6. **(Optional) Configure transport**  
   Set WiFi or USB mode in `Config.h`.  
7. **(Optional) Use companion tools**  
   See `Debug Tools/` and `HID Manager/`.  

There are README.md files on each directory explaining functions and in some cases tutorials are included.
---

## ⚠ Limitations

- ✅ ESP32 family only (S2, S3, C3 tested).  
- ❌ Do not enable USB CDC and HID simultaneously unless configured for it.  
- 🔄 Always regenerate mapping files after editing panel lists.  
- 🌐 For WiFi debug, ensure UDP broadcast/multicast is allowed on your network.  

---

## 📜 License

MIT License — see `LICENSE`.

---

Developed by the CockpitOS Firmware Project team.