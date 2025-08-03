# [ESP32] CockpitOS Firmware Project: For ESP32-S2, S3, C3 devices

---

## 🚀 What is CockpitOS?

CockpitOS is a next-generation, mission-critical firmware platform for building **DCS-BIOS**-compatible cockpit panels, indicators, and displays—built exclusively for the **ESP32** family of microcontrollers.

- **Ultra-lean:** Every buffer, lookup, and mapping is statically allocated—no heap, no dynamic memory, no malloc, ever.
- **Real-world aerospace standards:** Deterministic timing, rock-solid safety, and fault tolerance inspired by real avionics engineering.
- **Blazing fast:** Designed to operate at the lowest latency possible.

---

## ✈️ Why CockpitOS?

- **Zero dynamic memory**—every table, buffer, and mapping is static, so you'll *never* run out of RAM, leak, or fragment. 100% reliability
- **Hard real-time, deterministic loop:** No runtime allocations, no blocking calls.
- **Panel-agnostic, modular mapping:** Auto-generated, hash-optimized lookup tables for every control, LED, selector, and display, all tied to your current aircraft or instrument set.
- **Advanced selector dwell/debouncing logic:**  
  - Per-group selector state tracking with time-dwelling and stability enforcement (see `DCSBIOSBridge`).
  - Eliminates spurious inputs and ensures **100% stable panel state synchronization**—even during fast knob spins or mode switches.
- **Debug anywhere:**  
  - WiFi UDP console and remote debugging for *headless* cockpit development.
  - Built-in profiling tools: see panel timings, loop headroom, memory fragmentation, and USB/CDC health at runtime.
- **Plug-and-play expansion:** Add new panels by copying a reference file, update your label set, regenerate, and go.
- **Very few external dependencies:**  
  - No Arduino libraries (except optionally DCS-BIOS, but you can use the built-in "lite" version). For TFT Displays (digital gauges) you'll need TFT_eSPI.
  - No PlatformIO needed, no ESP-IDF, no extras—**just Arduino IDE + ESP32 Arduino core (2.x or 3.x) and compile!**.

---

## 🛠️ Development Environment

- **Target Platform:** ESP32 family (developed on ESP32-S2 Mini, “LOLIN S2 Mini” board in Arduino IDE)
- **IDE:** Arduino IDE 2.3.6
- **ESP32 Arduino Core:** v3.3.0
- **No other microcontrollers/tool chains supported/tested (so they might work)** (NO STM32, Teensy, ESP8266, ESP-IDF, PlatformIO, or MicroPython)

---

## 📦 Features

- **High-performance DCS-BIOS stream parsing:** Handles both USB and WiFi transport (selectable in Config.h)
- **Panel auto-mapping and lookup tables:** Auto-generated from your `selected_panels.txt` and aircraft JSON files.
- **Panel time-dwelling and group debouncing:**  
  - No false selector activations, even with fast user input or bouncing contacts.
  - *Every* selector state is time-stabilized and group-exclusivity is enforced.
- **Mission/aircraft synchronization:**  
  - Auto-resets state on mission start/stop (with airframe detection).
- **Remote debug tools:**  
  - Built-in WiFi/UDP logging, command viewer, DCS-BIOS replay, and more (see Debug Tools directory).
- **Python HID Manager bridge:**  
  - Run panels on any PC in your LAN, not just the DCS host—full network transparency for the ultimate simpit.
- **Zero-waste, high-rate operation:**  
  - 250 Hz polling, 60 Hz display refresh, ultra-fast selector response, and blazing USB throughput.

---

## 🔧 Quick Start

1. **Choose your label set**  
   - In `Config.h`, set your desired `LABEL_SET_XXXX` and save.
2. **Edit your panel list**  
   - In your label set directory, update `selected_panels.txt` to match your hardware.
3. **Run the code generator**  
   - `python generate_data.py` in your label set directory.
4. **Open in Arduino IDE**  
   - Open the main `.ino` file, select your board, Core = 3.3.0.
5. **Build and upload**  
   - Upload to your ESP32 or compatible hardware.
6. **(Optional) Set up WiFi debug/transport**  
   - Configure SSID/PASS and comms mode in `Config.h` as needed.
7. **(Optional) Use companion Python tools**  
   - See the `HID Manager` and `Debug Tools` directories for network bridging, replay, and logging.

---

## 🧑‍💻 IDE/Platform Details

- **IDE:** Arduino IDE 2.3.6 (NO PlatformIO needed!)
- **ESP32 Arduino Core:** v3.3.0
- **Board:** Wemos LOLIN S2 Mini (or compatible e.g S3, C3 etc)
- **No additional libraries required except TFT_eSPI for TFT Displays**
- **No external dependencies** except (optionally) DCS-BIOS Library ("lite" version included with only parser/state machine)

---

## 📁 Directory Structure

- **LABELS/**: Panel label sets and mapping generators.
- **src/**: Core firmware code, all panel modules, drivers, CUtils, core, and mappings.
- **Debug Tools/**: Network console, command viewer, DCS stream replay, UDP logger, etc.
- **HID Manager/**: Python script for network-to-HID bridging (run panels from any PC in your LAN).
- **README.md**: You’re here!

---

## 🔥 Advanced Panel Debouncing & Group Dwell

CockpitOS goes beyond ordinary DCS-BIOS firmwares by implementing **per-selector group dwell time, stable-state filtering, and group exclusivity**:

- No more double-fires or accidental position skips.
- All rotary switches, selectors, and multi-position switches are filtered for minimum dwell/stability before a new state is reported or command sent.
- **Your cockpit state in DCS will always match your physical panel.**

---

## 🤝 Contributing

**No contributions or pull requests are accepted.**
- **Fork it and do whatever you want.**
- This codebase is open and unencumbered, but is not accepting upstream changes.

---

## 🔗 Looking for a Simpler Alternative?

If you just need a minimal USB HID + UDP bridge for DCS-BIOS, check out  
**[DCS-Anywhere](https://github.com/BojoteX/DCS-Anywhere)** — an official CockpitOS "lite" project.

---

## ⚠️ Limitations / Known Issues

- **ESP32 ONLY:** No support for STM32, Teensy, ESP8266, MicroPython, ESP-IDF, or PlatformIO.
- **CDC & HID:** Do not enable both at the same time (see Config.h; use HID or CDC).
- **Panel mappings:** Always update your label set and regenerate mappings before a new build.
- **Debug output:** When using WiFi debug, ensure your network allows UDP broadcast/multicast.

---

## 📝 License

MIT License (see LICENSE)

---

Made by the CockpitOS Firmware Project team.