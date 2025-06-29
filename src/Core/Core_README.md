# CockpitOS Core Modules

This directory contains the **core, non-user-modifiable infrastructure** for CockpitOS.  
These files provide essential runtime services, advanced diagnostics, and high-performance protocol support that every panel and device relies on.

**Important:**  
> Except for `PerfMonitor.h` (where you may add your own profiling labels), **users should not modify any core file**.  
> Updates here are rare and only required when upgrading CockpitOS or adding new global features.

---

## Contents & Description

### 1. **DCSBIOSBridge / DCSBIOSBridge.h**
- **Role:** The “brain” of the firmware.
- **Function:** Listens to the DCS-BIOS export stream, decodes updates, routes events to HIDManager, LEDControl, and display subsystems, and manages subscriptions for selector, LED, and metadata changes.  
- Handles command throttling, group buffering, mission detection, and command history.
- *Not for user modification*—all customizations should use its subscription/event interface.

---

### 2. **LEDControl**
- **Role:** Unified LED control router.
- **Function:** Lets you control any LED (by label) regardless of its underlying hardware (GPIO, PCA9555, TM1637, GN1640, WS2812, etc.).
- **API:** `setLED(label, state, intensity)`, auto-selects the correct driver and abstraction.
- Expandable: New driver types can be registered here for advanced custom hardware.

---

### 3. **HIDManager**
- **Role:** HID (gamepad/USB joystick) abstraction and group/selector logic.
- **Function:** Handles group selectors, button toggles, throttling, gamepad report construction and transmission, DCS/HID mode switching, and covers all high-level panel interaction logic.
- **Usage:** All panels call its routines for logical actions—never duplicate this logic.

---

### 4. **RingBuffer**
- **Role:** High-performance, static (lock-free) ring buffers for I/O.
- **Function:** Buffers incoming and outgoing DCSBIOS, debug, or HID data (especially over USB or UDP, where packet size is limited to 64 bytes).
- **Features:** Handles chunked transport, overflow tracking, stats, and ensures deterministic, non-blocking transfers for CDC/UDP.

---

### 5. **debugPrint / WiFiDebug**
- **Role:** Central logger and remote debugging infrastructure.
- **Function:**  
  - `debugPrint` routes debug messages to Serial or UDP, according to config.
  - `WiFiDebug` allows full network debugging via UDP—essential for remote, headless, or in-sim debugging, and uses a ring buffer for non-blocking operation.
- **Features:** Supports chunked, formatted, and line-based logging.  
- **Best practice:** Always use these for all firmware debug output, not `Serial.print`.

---

### 6. **PerfMonitor / PerfMonitor.h**
- **Role:** Built-in real-time performance profiler and diagnostic tool.
- **Function:**  
  - Records timing stats for every main firmware phase (loop, rendering, HID, DCS, each LED driver, etc.).
  - Aggregates and periodically outputs a performance snapshot: load, headroom, system status, USB/CDC health, and ringbuffer stats.
- **User extension:**  
  - *The only core file you’re expected to edit:* add new profiling labels in `PerfMonitor.h` to measure custom code sections or panels.
  - Use `beginProfiling(PERF_LABEL)` / `endProfiling(PERF_LABEL)` to bracket any block you want profiled.
- **Features:** Integration with debugPrint/WiFiDebug for reporting, memory fragmentation stats, watchdog reset diagnostics, and more.

---

### 7. **Globals.h**
- **Role:** Exposes all key cross-module externs and configuration.
- **Function:**  
  - Pulls together all core interfaces for unified access from anywhere in firmware.
  - No user modification required.

---

## Advanced Features Provided by Core

- **UDP network debugging** (WiFiDebug) for remote cockpit development
- **High-performance, chunked USB/UDP transfer** (RingBuffer) for robust sim integration
- **Mission/aircraft detection and automatic panel sync** (DCSBIOSBridge)
- **Live panel/LED/selector display state** and group logic
- **Real-time performance profiling**—add your own labels for new panels and see detailed timing/load
- **Full separation of user panel code from protocol/transport internals**
- **Single-point logging abstraction** for all firmware debug output (serial/network)

---

## What May I Edit Here?

- **Only `PerfMonitor.h`** (to add new performance labels for profiling your own panels/devices).
- **Do NOT edit any other file.**  
  Any changes to core should be coordinated with firmware updates or as part of the CockpitOS project itself.

---

For further details on usage, refer to code comments in each file, and see panel/driver documentation for example integration.

