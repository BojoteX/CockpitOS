# CockpitOS HID Manager

A high-performance USB HID bridge for DCS-BIOS cockpit panels running CockpitOS firmware.

## Overview

This Python application bridges DCS World (via DCS-BIOS) with ESP32-based cockpit panels over native USB HID. It replaces the legacy Serial/CDC + socat approach with direct HID communication, enabling true plug-and-play operation.

**Key Features:**
- Near-zero CPU usage (~0% idle, <1% active)
- Native USB HID — no serial drivers or socat required
- Multi-device support (up to 32 panels simultaneously)
- Automatic device discovery and hot-plug detection
- Cross-platform (Windows, Linux, Raspberry Pi)

## Requirements

- Python 3.8+
- CockpitOS firmware with `USE_DCSBIOS_USB = 1`
- DCS World with DCS-BIOS installed

### Python Dependencies

```bash
pip install hidapi filelock
```

**Windows only:**
```bash
pip install windows-curses
```

## Installation

1. Clone or download the repository
2. Install dependencies (see above)
3. Run the manager:

```bash
python HID_Manager.py
```

## Configuration

Settings are stored in `settings.ini` (auto-created on first run):

```ini
[USB]
VID = 0xCAFE          ; Vendor ID (must match firmware)
; PID = 0x0001        ; Optional: filter by Product ID

[DCS]
UDP_SOURCE_IP = 127.0.0.1   ; Auto-detected from DCS-BIOS

[MAIN]
CONSOLE = 1           ; Console mode (always 1 for this version)
```

## Usage

1. Connect your CockpitOS panels via USB
2. Start DCS World and load a mission
3. Run `python HID_Manager.py`
4. Panels will auto-detect, handshake, and synchronize

### Console Interface

```
Frames: 12847   Hz: 30.0   kB/s: 42.3   Avg UDP Frame size: 1420   Data Source: 192.168.1.50

Device                                 Status           Reconnections
UFC                                    READY            0
IFEI                                   READY            0
Left Console                           READY            1

[14:32:01] [UFC] Handshake complete, ready to process input.
[14:32:01] [UFC] DCS detected on 192.168.1.50 — Starting normal operation.
[14:32:05] [UFC] IN: UFC_1 1

3 device(s) connected.  Press 'q' to quit.
```

### Keyboard Commands

| Key | Action |
|-----|--------|
| `q` | Quit |
| `Esc` | Quit |

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         DCS World                               │
│                      (DCS-BIOS Export)                          │
└──────────────────────────┬──────────────────────────────────────┘
                           │ UDP Multicast (239.255.50.10:5010)
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                     HID Manager                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐              │
│  │ UDP RX      │  │ Device A    │  │ Device B    │  ...         │
│  │ (blocking)  │──│ TX Worker   │──│ TX Worker   │              │
│  └─────────────┘  │ RX Reader   │  │ RX Reader   │              │
│                   └─────────────┘  └─────────────┘              │
└──────────────────────────┬──────────────────────────────────────┘
                           │ USB HID (per device)
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│              ESP32 Panels (CockpitOS Firmware)                  │
└─────────────────────────────────────────────────────────────────┘
```

### Thread Model

| Thread | Purpose | Blocking On |
|--------|---------|-------------|
| UDP RX | Receive DCS-BIOS frames | `recvfrom()` |
| Device RX (×N) | Read input triggers, drain feature reports | `hid.read()` |
| Device TX (×N) | Write HID reports to panels | `Condition.wait()` |
| Hotplug Monitor | Detect connect/disconnect | `time.sleep()` |
| Stats Updater | Calculate Hz, bandwidth | `time.sleep()` |
| UI | Console rendering | `getch()` timeout |

## Performance

| Scenario | CPU Usage |
|----------|-----------|
| Idle (no devices) | ~0% |
| Idle (10 devices, no DCS) | ~0% |
| Active (10 devices, DCS @ 30Hz) | <1% |

The design uses blocking I/O throughout — threads sleep in the kernel until data arrives, consuming zero CPU while waiting.

## Tunable Constants

Located at the top of `HID_Manager.py`:

| Constant | Default | Description |
|----------|---------|-------------|
| `MAX_DEVICES` | 32 | Maximum simultaneous panels |
| `MAX_HANDSHAKE_ATTEMPTS` | 50 | ~10 second handshake timeout |
| `MAX_FEATURE_DRAIN` | 64 | Messages per input trigger |
| `HOTPLUG_INTERVAL_S` | 3 | Device scan interval (seconds) |
| `LOG_KEEP` | 2000 | Console log history lines |

## Troubleshooting

### Device not detected

- Verify `VID` in `settings.ini` matches firmware (default: `0xCAFE`)
- Check USB connection and try different port
- On Linux, ensure user has HID permissions:
  ```bash
  sudo usermod -a -G plugdev $USER
  ```

### Handshake fails

- Ensure firmware has `USE_DCSBIOS_USB = 1` in `Config.h`
- Power cycle the device
- Check for firmware crash (Serial monitor if available)

### High latency

- Verify DCS-BIOS is running (check for UDP traffic)
- Ensure no other application is accessing the HID device

### Permission denied (Linux)

Create a udev rule for your devices:

```bash
sudo nano /etc/udev/rules.d/99-cockpitos.rules
```

Add:
```
SUBSYSTEM=="usb", ATTR{idVendor}=="cafe", MODE="0666"
SUBSYSTEM=="hidraw", ATTRS{idVendor}=="cafe", MODE="0666"
```

Reload:
```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

## Protocol

### DCS → Panel (TX)

1. UDP multicast frame received from DCS-BIOS
2. Frame sliced into 64-byte HID reports
3. Reports written to each panel via `hid.write()`

### Panel → DCS (RX)

1. Panel sends HID Input Report (trigger)
2. Manager drains Feature Reports (ASCII commands)
3. Commands sent to DCS-BIOS via UDP unicast (port 7778)

## License

MIT License — See [LICENSE](LICENSE) for details.

## Credits

Part of the [CockpitOS](https://github.com/BojoteX/CockpitOS) project.
