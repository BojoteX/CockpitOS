# Changelog

## v1.1.4 — 2026-02-22

### Added
- Label Creator tool — full TUI for creating and editing label sets (inputs, LEDs, displays, segment maps, custom pins, latched buttons, cover gates)
- Compiler tool — TUI-based firmware compiler and uploader, no Arduino IDE needed
- RS485 master/slave support for multi-board cockpit setups
- Cover gate system for switch covers that block inputs until opened
- Latched button support per label set
- CI build pipeline with automated compilation of all label sets
- Self-aligned DCS-BIOS sync implementation

### Improved
- Performance optimizations across input polling and LED updates
- HID Manager stability fixes
- Setup tool handles ESP32 core version downgrades correctly

### Fixed
- Analog gauges not working on certain label sets
- CMWS display tick marks not rendering

## v1.1.0 — 2026-01-10

### Added
- CMWS TFT display panel for AH-64D
- Centralized GPIO pin management for custom panels

### Improved
- Refactored outputs to be fully panel-agnostic
- Label set auto-generated scripts redesigned

## v1.0.6 — 2025-12-25

### Added
- HID Manager v2 with O(1) fixed thread pool architecture (handles 200+ devices)
- Self-learning axis calibration that improves over time
- NVS calibration persistence across reboots

## v1.0.5 — 2025-12-23

### Improved
- Enhanced TM1637 support for both inputs and LED outputs in custom panels

## v1.0.4 — 2025-11-19

### Improved
- TM1637 display support expanded for easier custom panel integration

## v1.0.0 — 2025-09-02

### Added
- Initial release
- ESP32 firmware for DCS World cockpit panels via DCS-BIOS
- Native USB HID support with companion HID Manager
- Wi-Fi debugging with hotspot auto-connect
- GPIO, PCA9555, HC165, WS2812, HT1622 hardware support
- Label set system for per-panel configurations
