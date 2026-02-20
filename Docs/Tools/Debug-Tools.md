# Debug and Utility Tools

CockpitOS includes a collection of standalone Python tools for bridging USB HID to DCS-BIOS, debugging panel communication, recording and replaying DCS data streams, and measuring latency.

## HID Manager

**Location:** `HID Manager/HID_Manager.py`

The HID Manager is the host-side bridge between DCS World (via DCS-BIOS UDP multicast) and your ESP32 cockpit panels (via USB HID). It is required when using USB transport mode.

### What It Does

- Receives DCS-BIOS export data on UDP multicast `239.255.50.10:5010`
- Slices each UDP frame into 64-byte HID Output Reports and sends them to connected panels
- Drains ASCII DCS-BIOS commands from each panel's Feature Report mailbox and forwards them to DCS-BIOS on UDP port `7778`
- Performs a Feature Report handshake on connect to verify the USB pipe is working end-to-end

### Key Features

| Feature | Details |
|---------|---------|
| Auto-discovery | Finds CockpitOS devices by VID `0xCAFE` -- no manual pairing |
| Multi-device | Supports up to 32 simultaneous panels |
| Hotplug | Detects connect/disconnect every 3 seconds |
| Multi-NIC | Joins multicast on ALL network interfaces, fixing Windows multi-adapter setups |
| Single-instance | File lock prevents two managers from fighting over HID handles |
| Adaptive UI | curses-based console with color-coded device status, live stats (Hz, kB/s, avg frame size), and a scrolling log |
| Frame-skip | When a panel falls behind, skips stale frames and jumps to the latest state |

### Configuration -- settings.ini

The HID Manager reads its configuration from `settings.ini` in the same directory as the script. The file is auto-created on first run.

```ini
[USB]
VID = 0xCAFE
; PID = 0xC8DD    ; Optional — omit to accept all CockpitOS devices

[DCS]
UDP_SOURCE_IP = 127.0.0.1    ; Auto-learned from first DCS packet

[MAIN]
CONSOLE = 1
```

| Setting | Section | Description |
|---------|---------|-------------|
| `VID` | `[USB]` | USB Vendor ID to match. Default: `0xCAFE` |
| `PID` | `[USB]` | Optional USB Product ID filter. Omit to accept all label sets. |
| `UDP_SOURCE_IP` | `[DCS]` | DCS-BIOS source IP. Auto-learned and persisted after first detection. |
| `CONSOLE` | `[MAIN]` | Enable console UI mode |

### Dependencies

```
pip install hidapi filelock windows-curses ifaddr
```

### How to Run

```
python "HID Manager/HID_Manager.py"
```

Press `q` or `Esc` to quit.

### UI Layout

```
Frames: 12345   Hz: 30.0   kB/s: 42.1   Avg UDP Frame size: 1434.2   Data Source: 127.0.0.1

Device                                 Status           Reconnections
FA18-MAIN                              READY            0
FA18-IFEI                              READY            1

[09:15:23] [FA18-MAIN] Handshake complete, ready to process input.
[09:15:23] [FA18-MAIN] DCS detected on 127.0.0.1 — Starting normal operation.
[09:15:24] [FA18-MAIN] IN: UFC_1 1

1 device(s) connected.  Press 'q' to quit.
```

---

## Debug Tools

All debug tools live in the `Debug Tools/` folder. Most are standalone scripts you run from the command line.

### CONSOLE_UDP_debug.py -- WiFi Debug Console

Receives and logs debug messages broadcast by CockpitOS devices over WiFi UDP on port `4210`. Messages are timestamped and logged to `logs/udpLogger.log` with automatic log rotation.

```
python "Debug Tools/CONSOLE_UDP_debug.py"
python "Debug Tools/CONSOLE_UDP_debug.py" --ip 192.168.1.50
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--ip` | *(none)* | Filter messages to only this source IP address |

### PLAY_DCS_stream.py -- Stream Replay

Replays captured DCS-BIOS data streams for offline testing. Sends multicast on ALL network interfaces so connected panels (or the HID Manager) receive the data as if DCS were running.

```
python "Debug Tools/PLAY_DCS_stream.py"
python "Debug Tools/PLAY_DCS_stream.py" --stream LAST
python "Debug Tools/PLAY_DCS_stream.py" --fps 60
python "Debug Tools/PLAY_DCS_stream.py" --raw-timing --speed 2.0
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--fps` | `30.0` | Fixed replay rate in frames per second (matches DCS-BIOS tick rate) |
| `--raw-timing` | *(off)* | Use the recorded per-frame timing instead of fixed FPS |
| `--speed` | `1.0` | Speed multiplier when using `--raw-timing` (e.g., `2.0` = 2x faster) |
| `--stream` | *(interactive)* | Stream identifier to load directly, bypassing the selection menu |

**Dependency:** `pip install ifaddr`

Stream files are stored in `Debug Tools/streams/` with the naming pattern `dcsbios_data.json.<identifier>`.

### RECORD_DCS_stream.py -- Stream Recorder

Captures live DCS-BIOS multicast data to a JSON file for later replay. Joins multicast on all network interfaces for reliable capture.

```
python "Debug Tools/RECORD_DCS_stream.py"
```

No command-line arguments. Output is saved to `Debug Tools/streams/dcsbios_data.json.LAST`. Press `Ctrl+C` to stop recording.

**Dependency:** `pip install ifaddr`

### LOG_DCS_commands.py -- Command Logger

Listens on UDP port `7778` and logs all DCS-BIOS commands sent by panels. Useful for verifying that button presses and switch changes produce the expected commands.

```
python "Debug Tools/LOG_DCS_commands.py"
```

No command-line arguments.

**Warning:** Do not run this while DCS is running. It binds to the same port DCS-BIOS uses for receiving commands, which would intercept commands meant for DCS.

### SEND_CommandTester.py -- Interactive Command Sender

An interactive menu-driven tool for sending individual DCS-BIOS commands via UDP. Includes pre-defined command sets for common controls (battery, LTD/R, gain, hook).

```
python "Debug Tools/SEND_CommandTester.py"
python "Debug Tools/SEND_CommandTester.py" --ip 192.168.1.100
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--ip` | `127.0.0.1` | Target IP address for DCS-BIOS commands |

### SEND_bulk_CommandTester.py -- Batch Command Sender

Sends multiple DCS-BIOS commands in sequence with an optional delay between each. Useful for testing multi-step operations or simulating rapid inputs.

```
python "Debug Tools/SEND_bulk_CommandTester.py" "BATTERY_SW 2" "GAIN_SWITCH 1"
python "Debug Tools/SEND_bulk_CommandTester.py" --ip 192.168.1.100 "BATTERY_SW 2"
python "Debug Tools/SEND_bulk_CommandTester.py" --debounce 100 "WING_FOLD_PULL 0" "WING_FOLD_ROTATE 1"
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--ip` | `127.0.0.1` | Target IP address for DCS-BIOS commands |
| `--debounce` | `0` | Delay in milliseconds between each command |
| `commands` | *(built-in defaults)* | Commands to send, each as a separate argument |

If no commands are provided on the command line, the tool uses built-in defaults for quick testing.

### FRAME_avg_size.py -- Frame Size Analyzer

Analyzes captured DCS-BIOS stream files to calculate average frame size. Helps understand bandwidth requirements for different aircraft and cockpit states.

```
python "Debug Tools/FRAME_avg_size.py"
python "Debug Tools/FRAME_avg_size.py" --stream SHORT
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--stream` | *(interactive)* | Stream identifier to analyze (prompts if not specified) |

### BOOTLOADER_reset_tool.py -- Remote Bootloader Trigger

Sends a magic UDP multicast packet to put ESP32 devices into bootloader mode remotely. Works for both WiFi-connected devices and USB devices (the HID Manager forwards the packet). Sends on all network interfaces.

```
python "Debug Tools/BOOTLOADER_reset_tool.py"
python "Debug Tools/BOOTLOADER_reset_tool.py" --device IFEI
python "Debug Tools/BOOTLOADER_reset_tool.py" --pid 0xC8DD
python "Debug Tools/BOOTLOADER_reset_tool.py" --all
```

| Argument | Short | Default | Description |
|----------|-------|---------|-------------|
| `--device` | `-d` | *(interactive)* | Target a specific device by its `LABEL_SET_NAME` |
| `--pid` | `-p` | *(interactive)* | Target a specific device by its USB PID (e.g., `0xC8DD`) |
| `--all` | `-a` | *(off)* | Reboot ALL discovered devices (use with caution) |

Without arguments, the tool presents an interactive menu to select which device to reboot.

**Dependency:** `pip install ifaddr`

### BLE_latency_check.py -- BLE Latency Measurement

**Location:** `Debug Tools/misc/BLE_latency_check.py`

Measures round-trip latency for BLE HID gamepads using a device-initiated protocol. Requires the firmware to have `MEASURE_LATENCY_TEST = 1` enabled and a physical test button wired to `LAT_TEST_PIN`.

```
python "Debug Tools/misc/BLE_latency_check.py"
python "Debug Tools/misc/BLE_latency_check.py" --vid 0xCAFE --pid 0xB5D3
python "Debug Tools/misc/BLE_latency_check.py" --product "FA18" --debug
```

| Argument | Default | Description |
|----------|---------|-------------|
| `--vid` | `0xCAFE` | USB/BLE Vendor ID |
| `--pid` | *(auto-detect)* | Product ID. Auto-detected from `active_set.h` if not specified. |
| `--product` | *(none)* | Product name substring filter |
| `--debug` | *(off)* | Enable verbose debug output |

The tool measures the full round-trip: button press on device, Feature Report to host, echo via Output Report back to device, RTT calculation. One-way BLE latency is approximately RTT / 2.

**Dependency:** `pip install hidapi`

---

## ReplayData Tools

The `ReplayData/` folder contains a utility for converting captured streams into embedded firmware replay data.

### generate_ReplayData.py -- Stream-to-Header Converter

Converts a `dcsbios_data.json` capture file into a C header file (`DcsbiosReplayData.h`) containing the stream as a binary blob. This enables on-device replay without a network connection, useful for demo and testing purposes.

```
cd ReplayData
python generate_ReplayData.py
```

No command-line arguments. Reads `dcsbios_data.json` from the current directory and writes `DcsbiosReplayData.h` to the same directory. The output format is `IS_REPLAY` compatible: each entry is packed as `[float delay][uint16 length][raw data bytes]`.

---

## Dependency Summary

| Tool | Extra pip packages |
|------|--------------------|
| HID Manager | `hidapi`, `filelock`, `windows-curses`, `ifaddr` |
| PLAY_DCS_stream.py | `ifaddr` |
| RECORD_DCS_stream.py | `ifaddr` |
| BOOTLOADER_reset_tool.py | `ifaddr` |
| BLE_latency_check.py | `hidapi` |
| All others | None (standard library only) |
