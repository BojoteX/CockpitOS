# CockpitOS Debug Tools

Python utilities for debugging, recording, replaying, and testing DCS-BIOS communication with CockpitOS panels.

---

## Scripts

| Script | Description |
|--------|-------------|
| `CONSOLE_UDP_debug.py` | Receives and logs debug messages from CockpitOS devices over WiFi UDP |
| `RECORD_DCS_stream.py` | Records DCS-BIOS UDP multicast stream to JSON for offline replay |
| `PLAY_DCS_stream.py` | Replays captured DCS-BIOS streams over multicast at configurable FPS |
| `SEND_CommandTester.py` | Interactive menu for sending individual DCS-BIOS commands |
| `SEND_bulk_CommandTester.py` | Sends multiple DCS-BIOS commands in sequence with configurable delay |
| `LOG_DCS_commands.py` | Listens for DCS-BIOS commands sent by panels on UDP port 7778 |
| `FRAME_avg_size.py` | Analyzes captured stream files for frame size and bandwidth statistics |
| `BOOTLOADER_reset_tool.py` | Sends magic UDP packet to trigger ESP32 bootloader mode remotely |

## Subdirectories

- `streams/` — Captured DCS-BIOS stream files for replay testing
- `logs/` — Debug log output
- `misc/` — Additional utilities (BLE latency testing)

## Usage

All scripts are standalone Python 3 programs. Run from this directory:
```
python CONSOLE_UDP_debug.py
python RECORD_DCS_stream.py
```

The UDP console (`CONSOLE_UDP_debug.py`) is the primary debugging tool — it receives real-time debug output from panels running in WiFi transport mode.
