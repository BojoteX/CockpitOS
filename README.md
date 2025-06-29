# [ESP32] CockpitOS Firmware Project: For ESP32-S2 Mini and others

---

## 🚀 What is CockpitOS?

CockpitOS is a next-generation, mission-critical firmware platform for building **DCS-BIOS**-compatible cockpit panels, indicators, and displays—built exclusively for the **ESP32** family of microcontrollers.

- **Ultra-lean:** Every buffer, lookup, and mapping is statically allocated—no heap, no dynamic memory, no malloc, ever.
- **Real-world aerospace standards:** Deterministic timing, rock-solid safety, and fault tolerance inspired by real avionics engineering.
- **Blazing fast:** Designed to operate at the highest polling rates and with the lowest latency of any DCS-BIOS firmware—period.

---

## ✈️ Why CockpitOS? (The "Mission-Critical" Difference)

- **Zero dynamic memory**—every table, buffer, and mapping is static, so you'll *never* run out of RAM, leak, or fragment.
- **Hard real-time, deterministic loop:** No runtime allocations, no blocking calls, no surprises.
- **Panel-agnostic, modular mapping:** Auto-generated, hash-optimized lookup tables for every control, LED, selector, and display, all tied to your current aircraft or instrument set.
- **Advanced selector dwell/debouncing logic:**  
  - Per-group selector state tracking with time-dwelling and stability enforcement (see `DCSBIOSBridge`).
  - Eliminates spurious inputs and ensures **100% stable panel state synchronization**—even during fast knob spins or mode switches.
- **Debug anywhere:**  
  - WiFi UDP console and remote debugging for *headless* cockpit development.
  - Built-in profiling tools: see panel timings, loop headroom, memory fragmentation, and USB/CDC health at runtime.
- **Plug-and-play expansion:** Add new panels by copying a reference file, update your label set, regenerate, and go.
- **No external dependencies:**  
  - No Arduino libraries (except optionally DCS-BIOS, but you can use the built-in "lite" version).
  - No PlatformIO, no ESP-IDF, no extras—**just Arduino IDE and ESP32 Arduino core and compile!**.

---

## 🛠️ Development Environment

- **Target Platform:** ESP32 family (developed on ESP32-S2 Mini, “ESP32S2 Dev Module” board in Arduino IDE)
- **IDE:** Arduino IDE 2.3.6
- **ESP32 Arduino Core:** v3.2.0
- **No other microcontrollers supported/tested (so they might work)** (NO STM32, Teensy, ESP8266, ESP-IDF, PlatformIO, or MicroPython)

> **Note:** Do NOT attempt to use this firmware on non-ESP32 hardware, or with any non-Arduino toolchain, use Arduino IDE 2.x and an ESP32 based microcontroller.

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
   - Open the main `.ino` file, ensure board = "ESP32S2 Dev Module", Core = 3.2.0.
5. **Build and upload**  
   - Upload to your ESP32-S2 Mini or compatible hardware.
6. **(Optional) Set up WiFi debug/transport**  
   - Configure SSID/PASS and comms mode in `Config.h` as needed.
7. **(Optional) Use companion Python tools**  
   - See the `HID Manager` and `Debug Tools` directories for network bridging, replay, and logging.

---

## 🧑‍💻 IDE/Platform Details

- **IDE:** Arduino IDE 2.3.6 (NO PlatformIO!)
- **ESP32 Arduino Core:** v3.2.0
- **Board:** ESP32S2 Dev Module (or compatible)
- **No additional libraries required**
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
- **CDC & HID:** Do not enable both at the same time (see Config.h; use HID or CDC/USB).
- **Panel mappings:** Always update your label set and regenerate mappings before a new build.
- **Debug output:** When using WiFi debug, ensure your network allows UDP broadcast/multicast.

---

## 📝 License

MIT License (see LICENSE)

---

Made by the CockpitOS Firmware Project team.  
**Plug in. Fly anywhere. With mission-critical confidence.**
