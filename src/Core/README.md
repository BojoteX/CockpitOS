# CockpitOS Core Modules

This directory contains the **core, non-user-modifiable infrastructure** for CockpitOS. These files provide essential runtime services that every panel and device relies on.

> Except for `PerfMonitor.h` (where you may add profiling labels), **users should not modify any core file**.

---

## Contents

### DCSBIOSBridge
Listens to the DCS-BIOS export stream, decodes updates, routes events to HIDManager, LEDControl, and display subsystems. Manages command throttling, group buffering, mission detection, and command history.

### LEDControl
Unified LED control router. Controls any LED by label regardless of underlying hardware (GPIO, PCA9555, TM1637, GN1640T, WS2812). API: `setLED(label, state, intensity)`.

### HIDManager
HID (gamepad/USB joystick) abstraction. Handles group selectors, button toggles, throttling, gamepad report construction, DCS/HID mode switching, and covers all high-level panel interaction logic.

### InputControl
Centralized GPIO/PCA/HC165 input management. Provides analog acquisition with EMA filtering and window statistics (up to 64 pins). Supports up to 8 GPIO encoders with 4 ticks per notch.

### CoverGate / CoverGateDef.h
Cover gate protection logic — prevents accidental activation of critical actions by requiring a physical cover to be opened first. Manages three gate kinds: Selector (2-position guarded), ButtonMomentary, and ButtonLatched. Uses stateful sequencing with deferred action timers.

### PanelRegistry
Runtime panel registration and activation system. Maintains bitmasked tracking of compiled panels vs. runtime-enabled panels. Supports up to 32 panels with priority-sorted insertion and deduplication.

### Bootloader
Remote bootloader entry for firmware updates over the network. Responds to magic packet `"COCKPITOS:REBOOT:<target>\n"` to trigger reboot into bootloader mode. Supports ESP32-S2, S3, C3, C6, and H2.

### RingBuffer
High-performance, static (lock-free) ring buffers for I/O. Buffers incoming and outgoing DCS-BIOS, debug, or HID data. Handles chunked transport, overflow tracking, and non-blocking transfers for CDC/UDP.

### debugPrint / WiFiDebug
Central logger and remote debugging infrastructure. `debugPrint` routes messages to Serial or UDP. `WiFiDebug` enables full network debugging via UDP with a ring buffer for non-blocking operation.

### PerfMonitor
Built-in real-time performance profiler. Records timing stats for every firmware phase (loop, rendering, HID, DCS, each LED driver). Use `beginProfiling(PERF_LABEL)` / `endProfiling(PERF_LABEL)` to measure custom code sections. **This is the only core file users are expected to edit** (to add new profiling labels).

### Private/BLEManager
Bluetooth Low Energy HID implementation using NimBLE. Provides wireless cockpit panel control with configurable connection parameters, battery monitoring, bond management, and WS2812 LED status indicators.

---

## Related Files (outside src/Core/)

- `src/Globals.h` — master include that pulls together all core interfaces
- `src/PinMap.h` — PIN() macro for S2/S3 GPIO translation
- `src/PsramConfig.h` — PSRAM allocation configuration
