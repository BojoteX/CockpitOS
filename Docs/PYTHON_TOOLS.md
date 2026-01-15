# CockpitOS — Python Tools Reference

> **Complete reference for all Python tools included with CockpitOS**
> This guide documents command-line arguments, configuration options, and usage examples.

---

## Table of Contents

1. [HID Manager](#1-hid-manager)
2. [Label Set Generators](#2-label-set-generators)
3. [Debug Tools](#3-debug-tools)
4. [Replay Data Generator](#4-replay-data-generator)

---

## 1. HID Manager

The HID Manager (`HID Manager/HID_Manager.py`) bridges your ESP32's USB HID interface to DCS-BIOS UDP. It runs on your Windows PC.

### 1.1 Configuration File (settings.ini)

Located in `HID Manager/settings.ini`:

```ini
[USB]
vid = 0xCAFE

[DCS]
udp_source_ip = 192.168.137.1

[MAIN]
console = 1
```

| Section | Key | Description |
|---------|-----|-------------|
| `[USB]` | `vid` | USB Vendor ID to match (must match `USB_VID` in Config.h) |
| `[DCS]` | `udp_source_ip` | IP address to bind for DCS-BIOS UDP (auto-detected if omitted) |
| `[MAIN]` | `console` | Show console window (1) or run headless (0) |

### 1.2 Running HID Manager

```bash
# Basic usage
python HID_Manager.py

# The manager automatically:
# - Detects all CockpitOS devices by VID
# - Bridges HID reports to/from DCS-BIOS UDP
# - Reconnects if devices are unplugged/replugged
```

### 1.3 Key Features

- **Auto-discovery**: Finds all devices with matching VID
- **Multi-device support**: Up to 32 simultaneous devices (`MAX_DEVICES = 32`)
- **IP auto-learning**: Learns DCS-BIOS IP from first incoming UDP packet
- **Single-instance locking**: Prevents multiple managers running simultaneously
- **Handshake retry**: Up to 50 attempts (`MAX_HANDSHAKE_ATTEMPTS = 50`)

---

## 2. Label Set Generators

Each Label Set contains Python scripts for generating mapping files.

### 2.1 generate_data.py

Generates all mapping files from the aircraft JSON and your configuration.

```bash
cd src/LABELS/LABEL_SET_MYPANEL
python generate_data.py
```

**No command-line arguments** — all configuration is via files:
- `selected_panels.txt` — Which panels to include
- `METADATA/*.json` — Custom control overlays
- `FA-18C_hornet.json` (or other aircraft JSON)

**Output files:**
- `DCSBIOSBridgeData.h`
- `InputMapping.h`
- `LEDMapping.h`
- `DisplayMapping.cpp/h`
- `CT_Display.cpp/h`
- `LabelSetConfig.h`
- `panels.txt`

### 2.2 display_gen.py

Generates display-specific mapping files. Called automatically by `generate_data.py`.

```bash
python display_gen.py
```

### 2.3 reset_data.py

Removes all auto-generated files from a Label Set.

```bash
python reset_data.py
```

**Interactive prompt:**
```
The following files will be DELETED if present:
  - DCSBIOSBridgeData.h
  - InputMapping.h
  - LEDMapping.h
  - DisplayMapping.cpp
  - DisplayMapping.h
  - CT_Display.cpp
  - CT_Display.h
  - LabelSetConfig.h

Are you absolutely sure you want to DELETE these files? (yes/NO):
```

---

## 3. Debug Tools

Located in `Debug Tools/` directory. These help with development and troubleshooting.

### 3.1 DCS_stream_generator.py

Replays captured DCS-BIOS data streams for offline testing.

```bash
python DCS_stream_generator.py [--speed MULTIPLIER] [--fps FPS]
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--speed` | 1.0 | Replay speed multiplier (e.g., 2.0 = 2x faster) |
| `--fps` | (none) | Override timing with fixed FPS (e.g., 60) |

**Examples:**
```bash
# Normal speed replay
python DCS_stream_generator.py

# 2x speed replay
python DCS_stream_generator.py --speed 2.0

# Fixed 60 FPS (ignores recorded timing)
python DCS_stream_generator.py --fps 60
```

**Requirements:**
- `dcsbios_data.json` file in the same directory (captured stream data)
- Sends to multicast `239.255.50.10:5010`

### 3.2 UDP_console_debugger.py

Receives and logs debug messages from CockpitOS devices over WiFi UDP.

```bash
python UDP_console_debugger.py [--ip IP_ADDRESS]
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--ip` | (none) | Only log messages from this IP address |

**Examples:**
```bash
# Log all debug messages
python UDP_console_debugger.py

# Filter to specific device
python UDP_console_debugger.py --ip 192.168.1.50
```

**Notes:**
- Listens on port 4210
- Logs to `logs/udpLogger.log`
- Previous log saved to `logs/prevLog.log`

### 3.3 BLE_latency_check.py

Measures round-trip latency for BLE HID devices.

```bash
python BLE_latency_check.py [--vid VID] [--pid PID] [--product NAME] [--debug]
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--vid` | 0xCAFE | USB Vendor ID |
| `--pid` | (auto) | USB Product ID (auto-detected from active_set.h) |
| `--product` | "" | Product name substring filter |
| `--debug` | false | Enable debug output |

**Examples:**
```bash
# Auto-detect device
python BLE_latency_check.py

# Specific VID/PID
python BLE_latency_check.py --vid 0xCAFE --pid 0xB5D3

# Filter by product name
python BLE_latency_check.py --product "IFEI"

# Enable debug output
python BLE_latency_check.py --debug
```

**Requirements:**
- `pip install hid` (hidapi library)
- Firmware compiled with `MEASURE_LATENCY_TEST = 1`
- BLE device connected to computer

**Usage:**
1. Run the script
2. Press the latency test button on your device repeatedly
3. Press Ctrl+C to stop and see statistics

### 3.4 bulk_send_debug.py

Sends multiple DCS-BIOS commands in sequence with optional delays.

```bash
python bulk_send_debug.py [--debounce MS] [COMMANDS...]
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--debounce` | 0 | Delay in milliseconds between each command |
| `COMMANDS` | (defaults) | Commands to send (space-separated) |

**Examples:**
```bash
# Send default commands instantly
python bulk_send_debug.py

# Send with 100ms between commands
python bulk_send_debug.py --debounce 100

# Send custom commands
python bulk_send_debug.py "MASTER_ARM_SW 1" "FIRE_BTN 1"

# Custom commands with delay
python bulk_send_debug.py --debounce 50 "WING_FOLD_PULL 0" "WING_FOLD_ROTATE 1"
```

**Default commands (if none specified):**
```
WING_FOLD_PULL 0
WING_FOLD_ROTATE 1
```

**Notes:**
- Sends to `192.168.7.166:7778` (edit `DCS_IP` in script if needed)
- Each command sent as UDP packet with newline terminator

### 3.5 DCS_stream_capture.py

Captures live DCS-BIOS data stream to a JSON file for later replay.

```bash
python DCS_stream_capture.py
```

**No arguments** — captures until Ctrl+C is pressed.

**Output:** `dcsbios_data.json`

### 3.6 DCS_commands_logger.py

Logs all DCS-BIOS commands sent by CockpitOS devices.

```bash
python DCS_commands_logger.py
```

### 3.7 avg_frame_size.py

Analyzes captured DCS-BIOS data to calculate average frame sizes.

```bash
python avg_frame_size.py
```

### 3.8 CommandTester.py

Interactive tool for sending individual DCS-BIOS commands.

```bash
python CommandTester.py
```

---

## 4. Replay Data Generator

Located in `ReplayData/` directory.

### 4.1 generate_ReplayData.py

Converts captured stream data into a format suitable for `IS_REPLAY` mode testing.

```bash
cd ReplayData
python generate_ReplayData.py
```

**Usage with firmware:**
1. Capture stream data using `DCS_stream_capture.py`
2. Place the JSON file in `ReplayData/`
3. Run `generate_ReplayData.py`
4. Set `IS_REPLAY = 1` in Config.h
5. Compile and upload firmware
6. Device will loop through captured data without needing DCS

---

## Quick Reference: Common Workflows

### Testing Without DCS

```bash
# 1. Capture live DCS data
cd "Debug Tools"
python DCS_stream_capture.py
# (fly around in DCS, press Ctrl+C when done)

# 2. Replay the captured data
python DCS_stream_generator.py --speed 1.0
```

### Debugging WiFi Connection

```bash
# 1. Start UDP logger on PC
cd "Debug Tools"
python UDP_console_debugger.py

# 2. Enable verbose mode in firmware
# Set VERBOSE_MODE = 1 and VERBOSE_MODE_WIFI_ONLY = 1 in Config.h
# Compile and upload

# 3. Watch debug messages in console
```

### Measuring BLE Latency

```bash
# 1. Enable latency test in firmware
# Set MEASURE_LATENCY_TEST = 1 in relevant panel code
# Compile and upload

# 2. Run latency checker
cd "Debug Tools"
python BLE_latency_check.py --debug

# 3. Press the test button on your device repeatedly
# 4. Press Ctrl+C to see statistics
```

---

## Dependencies

Most tools only need Python 3.x with standard libraries. Some tools require:

| Tool | Dependency | Install |
|------|------------|---------|
| `BLE_latency_check.py` | hidapi | `pip install hid` |
| `HID_Manager.py` | hidapi | `pip install hid` |

---

*For firmware configuration, see [CONFIG_REFERENCE.md](CONFIG_REFERENCE.md).*
*For label set creation, see [CREATING_LABEL_SETS.md](CREATING_LABEL_SETS.md).*
