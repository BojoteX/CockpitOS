# CockpitOS HID Manager v2 — Scalable Edition

**High-performance USB HID bridge designed for 200+ simultaneous devices**  
*Fixed O(1) thread pool • Single-writer-per-device guarantee • Cross-platform*

---

## ✈️ Overview

CockpitOS HID Manager v2 is a complete architectural redesign optimized for large-scale cockpit builds. Unlike the original thread-per-device approach, v2 uses a **fixed thread pool** that maintains constant resource usage regardless of device count.

- **12 threads total** — whether you have 1 device or 200
- **No serial ports, no COM drivers** — pure USB HID
- **Network transparent** — panels can connect from any PC on your LAN
- **Professional curses UI** — real-time statistics with color-coded status

---

## 🚀 Key Features

| Feature | Description |
|---------|-------------|
| **O(1) Thread Scaling** | Fixed 12-thread pool handles unlimited devices |
| **Single-Writer Guarantee** | Each device assigned to exactly one TX worker — no HID races |
| **Trigger-Based Protocol** | INPUT reports signal when to read FEATURE data |
| **Bounded Queues** | Backpressure protection prevents memory exhaustion |
| **Snapshot Pattern** | Lock-free device iteration for high-frequency reads |
| **Per-Device Locks** | Cross-platform HID safety without global contention |
| **Exception-Safe Broadcast** | UDP dispatch never crashes, even under load |

---

## 🗺️ Architecture

```
═══════════════════════════════════════════════════════════════════════════════
                        DATA FLOW DIAGRAM
═══════════════════════════════════════════════════════════════════════════════

    DCS World Game
         │
         ▼ (UDP multicast 239.255.50.10:5010)
    ┌─────────────┐
    │ UdpNetwork  │ ─── receives UDP frames from DCS-BIOS
    └─────────────┘
         │
         ▼ (calls dispatch())
    ┌──────────────┐
    │ TxDispatcher │ ─── slices UDP into 64-byte HID reports
    └──────────────┘     broadcasts to worker queues
         │
         ▼ (sharded by tx_worker_id)
    ┌────────────────────────────────────────┐
    │ TxWorker-0   TxWorker-1   TxWorker-2   │ ─── each writes to assigned 
    └────────────────────────────────────────┘     devices ONLY (single-writer)
         │
         ▼ (HID write)
    ┌─────────────┐
    │ ESP32 Panel │ ─── receives cockpit state, updates displays/LEDs
    └─────────────┘
         │
         ▼ (HID Input Report = TRIGGER)
    ┌────────────────────────────────────────┐
    │ RxPoller-0   RxPoller-1   RxPoller-2   │ ─── poll assigned devices
    └────────────────────────────────────────┘     for button/switch input
         │
         ▼ (HID Feature Report = command)
    ┌─────────────┐
    │ UdpNetwork  │ ─── sends commands back to DCS
    └─────────────┘
         │
         ▼ (UDP unicast to port 7778)
    DCS World Game
```

### Thread Model (Fixed O(1) Pool)

| Thread | Count | Purpose |
|--------|-------|---------|
| **TxWorker** | 4 | Write HID OUT reports to assigned devices |
| **RxPoller** | 4 | Read HID input from assigned devices |
| **UdpRx** | 1 | Receive DCS-BIOS UDP multicast |
| **HandshakeMgr** | 1 | Initialize new devices |
| **HotplugMon** | 1 | Detect USB connect/disconnect |
| **UI** | 1 | Curses console display |
| **Total** | **12** | *Constant regardless of device count* |

### Device Lifecycle

```
DISCONNECTED ──(hotplug detected)──▶ HANDSHAKING ──(success)──▶ READY
                                          │
                                          └──(timeout/failure)──▶ ERROR
```

---

## 🔧 Critical Protocol Detail: Trigger-Based Drain

The firmware uses a **trigger pattern** that the host must implement correctly:

1. **Firmware** queues commands in a ring buffer
2. **Firmware** sends an INPUT report as a "trigger" signal
3. **Host** receives INPUT report (non-blocking poll)
4. **Host** calls `get_feature_report()` to drain the buffer
5. **Host** continues draining until empty (all zeros)

```python
# CORRECT: Only read Feature Reports after receiving Input trigger
data = dev.read(64, timeout_ms=0)  # Non-blocking
if data:  # Trigger received!
    while True:
        resp = dev.get_feature_report(0, 65)
        if not any(resp[1:]):  # Empty = done
            break
        process_command(resp)
```

> ⚠️ **Never poll `get_feature_report()` without a trigger** — this wastes USB bandwidth and may cause timing issues.

---

## 🛡️ Thread Safety Design

### Single-Writer-Per-Device (TX Sharding)

```
Device Assignment at Handshake:
┌──────────────────┐     ┌──────────────────┐
│ TxWorker-0       │     │ TxWorker-1       │
│ ├── Device-A     │     │ ├── Device-B     │
│ └── Device-C     │     │ └── Device-D     │
└──────────────────┘     └──────────────────┘
         │                        │
         ▼                        ▼
   ONLY TxWorker-0          ONLY TxWorker-1
   writes to A,C            writes to B,D
```

**Why this matters:** HID handles are not thread-safe on all platforms. By assigning each device to exactly one TX worker at handshake time, we guarantee no two threads ever write to the same device.

### Per-Device Locks

Each `DeviceState` has its own lock protecting HID operations:

```python
with dev_state.lock:
    dev.write(report)      # TX
    # or
    dev.get_feature_report(...)  # RX
```

Network/UI operations happen **outside** the lock to prevent contention.

### DeviceRegistry Snapshot Pattern

```python
# FAST: Returns immutable snapshot (no lock held during iteration)
devices = registry.get_all()
for dev in devices:
    process(dev)
```

---

## 🖥️ Requirements

### Python
- **Python 3.7+** (3.10+ recommended)

### Required Packages
```bash
pip install hidapi filelock
```

### Platform-Specific
| Platform | Additional Package |
|----------|-------------------|
| **Windows** | `pip install windows-curses` |
| **Linux/macOS** | None (curses built-in) |

### Hardware
- ESP32-S2, ESP32-S3, or ESP32-P4 running CockpitOS firmware
- DCS World with DCS-BIOS UDP export enabled

---

## 🔌 Installation

```bash
# Clone repository
git clone https://github.com/BojoteX/CockpitOS.git
cd "CockpitOS/HID Controller"

# Install dependencies (all platforms)
pip install hidapi filelock

# Windows only
pip install windows-curses
```

---

## ⚡ Usage

### Quick Start

1. **Plug in your ESP32 cockpit panel(s)**
2. **Launch DCS World** with DCS-BIOS UDP export enabled
3. **Run the HID Manager:**
   ```bash
   python CockpitOS_HID_Manager_v2_FINAL.py
   ```

### Console Interface

```
Frames: 48291   Hz: 47.8   kB/s: 22.1   Avg: 462.3 bytes   Data Source: 192.168.1.100

Device                                 Status           Reconnections
ESP32-S2-UFCP                          READY            0
ESP32-S3-LEFT-CONSOLE                  READY            1
ESP32-S2-MASTER-ARM                    HANDSHAKING      0

[14:32:15] [System] Starting CockpitOS HID Bridge (Scalable Edition)...
[14:32:15] [System] VID: 0xCAFE, PID: Any
[14:32:15] [System] Thread pool: 4 TX + 4 RX + 3 system
[14:32:16] [ESP32-S2-UFCP] Connected
[14:32:16] [ESP32-S2-UFCP] Starting handshake...
[14:32:16] [ESP32-S2-UFCP] Handshake complete, ready to process input.
[14:32:17] [ESP32-S2-UFCP] IN: MASTER_ARM_SW 1

3 device(s) connected (2 ready).  Press 'q' to quit.
```

**Status Colors:**
| Color | Status | Meaning |
|-------|--------|---------|
| 🟢 Green | READY | Device operational |
| 🟡 Yellow | HANDSHAKING / WAIT | Initializing |
| 🔴 Red | ERROR / DISCONNECTED | Problem detected |

---

## ⚙️ Configuration

### settings.ini

```ini
[USB]
# Vendor ID (required) — must match firmware
VID = 0xCAFE

# Product ID (optional) — omit to accept any PID with matching VID
# PID = 0x4011

[DCS]
# Auto-detected from first UDP packet and persisted
UDP_SOURCE_IP = 127.0.0.1

[MAIN]
CONSOLE = 1
```

### Architecture Constants (in script)

```python
TX_WORKERS          = 4       # HID write workers
RX_POLLERS          = 4       # HID read pollers
RX_POLL_INTERVAL_MS = 1       # Polling frequency
MAX_DEVICES         = 256     # Registry capacity
TX_QUEUE_SIZE       = 1024    # Backpressure threshold
```

---

## 📊 Statistics

| Stat | Description |
|------|-------------|
| **Frames** | Total UDP frames received from DCS-BIOS |
| **Hz** | Frames per second (1-second rolling window) |
| **kB/s** | Bandwidth in kilobytes per second |
| **Avg** | Average bytes per UDP frame |
| **Data Source** | Detected DCS PC IP address |
| **Reconnections** | Per-device reconnect count (survives unplugs) |

---

## 🌐 Network Topology

### Local Setup
```
[DCS PC]
    │
    ├── HID Manager
    │
    └── USB ── [ESP32 Panels]
```

### Distributed Setup
```
[DCS PC]                          [Panel PC]
    │                                  │
    └──── LAN (UDP multicast) ─────────┘
                                       │
                                  HID Manager
                                       │
                                  USB ── [ESP32 Panels]
```

> **Note:** UDP multicast typically doesn't cross routers. For multi-subnet setups, use unicast or a multicast relay.

---

## 🆚 v1 vs v2 Comparison

| Aspect | v1 (Original) | v2 (Scalable) |
|--------|---------------|---------------|
| **Thread Model** | O(N) — one per device | O(1) — fixed pool of 12 |
| **Max Devices** | ~20-30 practical | 200+ tested |
| **TX Architecture** | Per-device queues | Sharded workers with broadcast |
| **Thread Safety** | Global locks | Per-device locks + sharding |
| **Memory** | Standard objects | `__slots__` for efficiency |
| **Registry** | List iteration | Snapshot pattern |
| **UI Options** | Console + Tkinter GUI | Console only (lightweight) |
| **Backpressure** | Limited | Bounded queues, graceful drop |

---

## 🆘 Troubleshooting

### No devices detected

1. **Verify VID/PID** in `settings.ini` matches firmware
2. **Check USB CDC On Boot** = Disabled in Arduino IDE
3. **Linux permissions** — add udev rule or run with sudo
4. **Try different USB port** — avoid hubs initially

### Device stuck on HANDSHAKING

1. **Firmware mismatch** — ensure `USE_DCSBIOS_USB = 1` in Config.h
2. **Stale state** — unplug, wait 5 seconds, replug
3. **Check handshake protocol** — firmware must echo `DCSBIOS-HANDSHAKE`

### Commands not reaching DCS

1. **DCS-BIOS enabled** — verify UDP export is configured
2. **Firewall** — allow UDP 5010 (RX) and 7778 (TX)
3. **Data Source** — ensure IP appears in stats header
4. **Check logs** — look for "IN:" messages confirming receipt

### High CPU usage

- **Expected:** 2-5% total across all threads
- **If higher:** Check for USB hub issues, reduce polling rate

### "Another instance running" error

- Close existing instance or delete `hid_manager.lock`

---

## 🧑‍💻 For Developers

### Module Structure

| Class | Lines | Responsibility |
|-------|-------|----------------|
| `DeviceState` | 243-303 | Thread-safe device container with `__slots__` |
| `DeviceRegistry` | 309-375 | Central registry with snapshot pattern |
| `TxDispatcher` | 381-531 | UDP→HID fan-out with sharded workers |
| `RxPoller` | 537-658 | Non-blocking HID reads with trigger drain |
| `HandshakeManager` | 664-794 | Device initialization queue |
| `HotplugMonitor` | 800-896 | USB connect/disconnect detection |
| `UdpNetwork` | 902-1035 | Multicast RX, unicast TX |
| `ConsoleUI` | 1041-1250 | Curses display |
| `CockpitHidBridge` | 1255-1378 | Main orchestrator |

### Porting to C++

Key patterns to preserve:

1. **Sharded TX workers** — device-to-worker affinity at connect time
2. **Trigger-based drain** — never poll feature reports without input trigger
3. **Per-device locks** — hold only during HID operations
4. **Bounded queues** — backpressure with graceful degradation
5. **Snapshot registry** — lock-free iteration for hot path

Recommended stack: **libusb** + **Boost.Asio** + **ncurses**

---

## 📁 File Structure

```
HID Controller/
├── CockpitOS_HID_Manager_v2_FINAL.py   # Main application
├── settings.ini                         # Configuration (auto-created)
├── hid_manager.lock                     # Single-instance lock
└── README.md                            # This documentation
```

---

## ❓ FAQ

**Q: How many devices can this handle?**  
A: Tested with 200+ simulated devices. Real-world limit is your USB infrastructure (see main README for xHCI limitations).

**Q: Why no GUI mode?**  
A: v2 focuses on performance and server-friendly operation. The curses UI works over SSH and uses minimal resources.

**Q: Can I mix v1 and v2?**  
A: No — use one or the other. They use the same lock file and would conflict.

**Q: What's the latency?**  
A: Sub-millisecond for button presses. UDP frames dispatched within ~1ms of receipt.

**Q: Is the single-instance lock necessary?**  
A: Yes — multiple instances would fight over the same HID handles, causing data corruption.

---

## 📜 License

[MIT](LICENSE)

---

## 🔗 Related Resources

- **CockpitOS Firmware:** [github.com/BojoteX/CockpitOS](https://github.com/BojoteX/CockpitOS)
- **DCS-BIOS (Skunkworks):** [github.com/DCS-Skunkworks/dcs-bios](https://github.com/DCS-Skunkworks/dcs-bios)

---

**Built for Scale. Engineered for Reliability.**  
*— The CockpitOS Project*
