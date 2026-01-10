# CockpitOS — Transport Modes Guide

> **Connecting your panel to DCS World.**  
> This guide explains how CockpitOS communicates with DCS-BIOS and walks you through configuring each transport mode — USB, WiFi, or Serial.

---

## Table of Contents

1. [Understanding Transport Modes](#1-understanding-transport-modes)
2. [Which Mode Should I Use?](#2-which-mode-should-i-use)
3. [Prerequisites](#3-prerequisites)
4. [USB Mode — The Preferred Choice](#4-usb-mode--the-preferred-choice)
5. [WiFi Mode — Wireless Connection](#5-wifi-mode--wireless-connection)
6. [Serial Mode — Legacy Support](#6-serial-mode--legacy-support)
7. [Verifying Your Connection](#7-verifying-your-connection)
8. [Debugging and Diagnostics](#8-debugging-and-diagnostics)
9. [Troubleshooting](#9-troubleshooting)
10. [Quick Reference](#10-quick-reference)

---

## 1. Understanding Transport Modes

CockpitOS needs a way to exchange data with DCS World. This communication happens through **DCS-BIOS**, which exports cockpit state from the simulator and accepts commands to manipulate controls.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           THE DATA FLOW                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌───────────┐         ┌───────────┐         ┌───────────────────────┐    │
│   │           │  Export │           │         │                       │    │
│   │ DCS World │────────►│ DCS-BIOS  │────────►│  Your ESP32 Panel     │    │
│   │           │  Stream │  (Lua)    │         │  (CockpitOS Firmware) │    │
│   │           │◄────────│           │◄────────│                       │    │
│   └───────────┘ Commands└───────────┘         └───────────────────────┘    │
│                                                                             │
│   Export: Cockpit state (LEDs, displays, gauges, switch positions)         │
│   Commands: Button presses, switch changes, dial rotations                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Transport Mode** determines HOW this data travels between your ESP32 and DCS-BIOS:

| Mode | Path | Helper Required | Best For |
|------|------|-----------------|----------|
| **USB** | ESP32 ↔ USB HID ↔ HID Manager ↔ DCS-BIOS | Yes (Python script) | Production cockpits |
| **WiFi** | ESP32 ↔ UDP Multicast ↔ DCS-BIOS | No | Wireless panels, remote PC |
| **Serial** | ESP32 ↔ CDC Serial ↔ socat ↔ DCS-BIOS | Yes (socat script) | Legacy, debugging |

**Important:** Only ONE transport mode can be active at a time. The firmware enforces this at compile time.

---

## 2. Which Mode Should I Use?

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        TRANSPORT MODE SELECTION                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   START HERE                                                                │
│       │                                                                     │
│       ▼                                                                     │
│   ┌───────────────────────────────┐                                         │
│   │ Is your ESP32 an S2, S3,      │                                         │
│   │ or P4 with native USB?        │                                         │
│   └───────────────┬───────────────┘                                         │
│                   │                                                         │
│         YES ◄─────┴─────► NO                                                │
│          │                 │                                                │
│          ▼                 ▼                                                │
│   ┌─────────────┐   ┌─────────────────────┐                                 │
│   │ Do you need │   │ Does your ESP32     │                                 │
│   │ wireless?   │   │ have WiFi?          │                                 │
│   └──────┬──────┘   └──────────┬──────────┘                                 │
│          │                     │                                            │
│    YES ◄─┴─► NO          YES ◄─┴─► NO                                       │
│     │        │            │        │                                        │
│     ▼        ▼            ▼        ▼                                        │
│  ┌──────┐ ┌──────┐    ┌──────┐ ┌──────┐                                     │
│  │ WiFi │ │ USB  │    │ WiFi │ │Serial│                                     │
│  │ Mode │ │ Mode │    │ Mode │ │ Mode │                                     │
│  └──────┘ └──────┘    └──────┘ └──────┘                                     │
│              ▲                                                              │
│              │                                                              │
│         RECOMMENDED                                                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Quick Recommendation

| Your Situation | Recommended Mode |
|----------------|------------------|
| ESP32-S2 or S3, wired to PC | **USB Mode** ← Best performance |
| Wireless panel, ESP32 with WiFi | **WiFi Mode** |
| ESP32 Classic (no native USB) | **WiFi Mode** or Serial Mode |
| Debugging firmware issues | **Serial Mode** (with verbose logging) |
| Multiple panels on one PC | **USB Mode** (supports up to 32 devices) |

### Mode Comparison

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         PERFORMANCE COMPARISON                              │
├──────────────┬──────────────┬──────────────┬──────────────┬─────────────────┤
│   Attribute  │   USB Mode   │  WiFi Mode   │ Serial Mode  │     Notes       │
├──────────────┼──────────────┼──────────────┼──────────────┼─────────────────┤
│ Latency      │ Excellent    │ Good         │ Good         │ USB is fastest  │
│ Reliability  │ Excellent    │ Good*        │ Good         │ *WiFi varies    │
│ Setup        │ Moderate     │ Easy         │ Easy         │ USB needs Python│
│ Multi-device │ Up to 32     │ Unlimited    │ 1 per port   │                 │
│ CPU on PC    │ <1%          │ 0%           │ Low          │                 │
│ Wireless     │ No           │ Yes          │ No           │                 │
│ ESP32 Support│ S2/S3/P4     │ All (exc H2) │ All          │                 │
└──────────────┴──────────────┴──────────────┴──────────────┴─────────────────┘
```

---

## 3. Prerequisites

Before configuring any transport mode, ensure you have:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  CHECKLIST — ALL TRANSPORT MODES                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│  [ ] DCS World installed and working                                        │
│  [ ] DCS-BIOS (Skunkworks) installed in DCS                                 │
│  [ ] CockpitOS firmware compiling successfully                              │
│  [ ] Label Set configured and generated                                     │
│  [ ] ESP32 board flashed with CockpitOS                                     │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.1 Verify DCS-BIOS Installation

DCS-BIOS must be installed and running in DCS World. If you haven't done this yet, see **CREATING_LABEL_SETS.md** Section 2.1 for installation instructions.

If DCS-BIOS is working, it exports data via UDP multicast to `239.255.50.10:5010` whenever a mission is running.

---

## 4. USB Mode — The Preferred Choice

USB Mode provides the best performance and reliability for cockpit panels. Your ESP32 appears as a USB HID device (like a gamepad), and a Python helper application bridges it to DCS-BIOS.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           USB MODE ARCHITECTURE                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────┐      USB HID       ┌──────────────┐      UDP             │
│   │   ESP32     │◄──────────────────►│ HID Manager  │◄────────────────┐    │
│   │ (CockpitOS) │   64-byte reports  │   (Python)   │                 │    │
│   └─────────────┘                    └──────────────┘                 │    │
│                                             │                         │    │
│                                             │ UDP 7778 (commands)     │    │
│                                             ▼                         │    │
│                                      ┌──────────────┐                 │    │
│                                      │   DCS-BIOS   │                 │    │
│                                      │    (Lua)     │─────────────────┘    │
│                                      └──────────────┘  239.255.50.10:5010  │
│                                             │          (export stream)     │
│                                             ▼                              │
│                                      ┌──────────────┐                      │
│                                      │  DCS World   │                      │
│                                      └──────────────┘                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.1 Hardware Requirements

USB Mode requires an ESP32 with **native USB support**:

| Board | USB Mode Support | Notes |
|-------|------------------|-------|
| ESP32-S2 | ✅ Yes | Excellent choice |
| ESP32-S3 | ✅ Yes | Best performance |
| ESP32-P4 | ✅ Yes | Newest, powerful |
| ESP32 Classic | ❌ No | Use WiFi or Serial |
| ESP32-C3/C6 | ❌ No | Use WiFi or Serial |
| ESP32-H2 | ❌ No | No WiFi either — Serial only |

### 4.2 Configure Config.h

Open `Config.h` in your CockpitOS project and set the transport mode:

```cpp
// Config.h — Transport Mode Selection

// Set EXACTLY ONE of these to 1, all others to 0:
#define USE_DCSBIOS_WIFI                            0
#define USE_DCSBIOS_USB                             1  // ← Enable USB Mode
#define USE_DCSBIOS_SERIAL                          0
```

> **⚠️ Important for ESP32-S3 and P4**  
> You must also configure the USB mode in Arduino IDE:  
> `Tools` → `USB Mode` → Select **"USB-OTG (TinyUSB)"**  
> 
> For S2, this is the default and no change is needed.

### 4.3 Compile and Upload

**Step 4.3.1** — In Arduino IDE, verify your board settings:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ARDUINO IDE SETTINGS FOR USB MODE                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Tools Menu:                                                                │
│  ├── Board: "ESP32S2 Dev Module" (or your S2/S3/P4 board)                  │
│  ├── USB CDC On Boot: "Disabled"     ← CRITICAL!                           │
│  ├── USB Mode: "USB-OTG (TinyUSB)"   ← S3/P4 only                          │
│  └── Port: (your bootloader COM port)                                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Step 4.3.2** — Compile and upload the firmware (see GETTING_STARTED.md if needed)

**Step 4.3.3** — After upload, press RST or power-cycle the board

Your ESP32 should now appear as a USB HID device. On Windows, check Device Manager under "Human Interface Devices" — you should see a device with manufacturer "CockpitOS".

### 4.4 Install HID Manager Dependencies

The HID Manager is a Python application located in the `HID Manager/` directory of your CockpitOS installation.

**Step 4.4.1** — Ensure Python 3.8+ is installed:

```bash
python --version
# Should show Python 3.8.x or higher
```

**Step 4.4.2** — Install required Python packages:

```bash
# All platforms
pip install hidapi filelock

# Windows only — also install:
pip install windows-curses
```

**Step 4.4.3** — Verify hidapi installation:

```bash
python -c "import hid; print('hidapi OK')"
```

> **🐧 Linux Users**  
> You may need to configure udev rules for HID access. Create a file:
> ```bash
> sudo nano /etc/udev/rules.d/99-cockpitos.rules
> ```
> Add these lines:
> ```
> SUBSYSTEM=="usb", ATTR{idVendor}=="cafe", MODE="0666"
> SUBSYSTEM=="hidraw", ATTRS{idVendor}=="cafe", MODE="0666"
> ```
> Then reload:
> ```bash
> sudo udevadm control --reload-rules
> sudo udevadm trigger
> ```

### 4.5 Configure HID Manager

**Step 4.5.1** — Navigate to the HID Manager directory:

```bash
cd "HID Manager"
```

**Step 4.5.2** — The first time you run the manager, it creates `settings.ini`:

```ini
[USB]
VID = 0xCAFE          ; Vendor ID — must match firmware (don't change)

[DCS]
UDP_SOURCE_IP = 127.0.0.1   ; Auto-detected when DCS runs

[MAIN]
CONSOLE = 1           ; Console mode (always 1)
```

Normally you don't need to edit this file. The `UDP_SOURCE_IP` is automatically updated when DCS-BIOS traffic is detected.

### 4.6 Run HID Manager

**Step 4.6.1** — Connect your ESP32 panel via USB

**Step 4.6.2** — Run the HID Manager:

```bash
python HID_Manager.py
```

**Step 4.6.3** — You should see the console interface:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  CockpitOS HID Manager                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Frames: 0   Hz: 0.0   kB/s: 0.0   Avg UDP Frame size: 0   Data Source: ... │
│                                                                             │
│  Device                               Status           Reconnections        │
│  IFEI                                 WAIT HANDSHAKE   0                    │
│                                                                             │
│  [14:32:00] [IFEI] Waiting for handshake...                                 │
│                                                                             │
│  1 device(s) connected.  Press 'q' to quit.                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

**Step 4.6.4** — Start DCS World and load a mission

**Step 4.6.5** — The status should change to "READY":

```
  Device                               Status           Reconnections
  IFEI                                 READY            0

  [14:32:01] [IFEI] Handshake complete, ready to process input.
  [14:32:01] [IFEI] DCS detected on 192.168.1.50 — Starting normal operation.
```

> **✓ Checkpoint**  
> If you see "READY" status and frame count increasing, USB Mode is working correctly. Your panel can now send commands to DCS and receive cockpit state updates.

### 4.7 HID Manager Status Reference

| Status | Meaning |
|--------|---------|
| `WAIT HANDSHAKE` | Device connected, waiting for firmware response |
| `READY` | Fully operational, exchanging data |
| `DISCONNECTED` | USB connection lost |
| `STALE HANDLE` | Communication error, will auto-reconnect |

---

## 5. WiFi Mode — Wireless Connection

WiFi Mode allows your ESP32 to communicate directly with DCS-BIOS over your local network — no helper application needed on the PC.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          WIFI MODE ARCHITECTURE                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────┐                            ┌──────────────┐               │
│   │   ESP32     │◄──── WiFi Network ────────►│   DCS-BIOS   │               │
│   │ (CockpitOS) │                            │    (Lua)     │               │
│   └─────────────┘                            └──────────────┘               │
│         │                                           │                       │
│         │  UDP Multicast 239.255.50.10:5010        │                       │
│         │◄─────────────────────────────────────────┘ (export stream)       │
│         │                                                                   │
│         │  UDP Unicast port 7778                                           │
│         └─────────────────────────────────────────►  (commands)            │
│                                                                             │
│   No helper application required!                                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 5.1 Hardware Requirements

WiFi Mode works with any ESP32 that has WiFi capability:

| Board | WiFi Mode Support |
|-------|-------------------|
| ESP32 Classic | ✅ Yes |
| ESP32-S2 | ✅ Yes |
| ESP32-S3 | ✅ Yes |
| ESP32-C3 | ✅ Yes |
| ESP32-C6 | ✅ Yes |
| ESP32-P4 | ✅ Yes |
| ESP32-H2 | ❌ No WiFi hardware |

### 5.2 Configure Config.h

Open `Config.h` and configure WiFi mode:

**Step 5.2.1** — Set the transport mode:

```cpp
// Config.h — Transport Mode Selection

// Set EXACTLY ONE of these to 1, all others to 0:
#define USE_DCSBIOS_WIFI                            1  // ← Enable WiFi Mode
#define USE_DCSBIOS_USB                             0
#define USE_DCSBIOS_SERIAL                          0
```

**Step 5.2.2** — Configure your WiFi credentials:

```cpp
// Wi-Fi network credentials
#define WIFI_SSID                                  "YourNetworkName"
#define WIFI_PASS                                  "YourWiFiPassword"
```

> **⚠️ Network Requirements**  
> - Your PC running DCS and your ESP32 must be on the **same network**
> - The network must allow **UDP multicast** (most home routers do)
> - For best results, use a **2.4 GHz** network (better range, lower latency)
> - **5 GHz** works but may have reduced range

### 5.3 Compile and Upload

**Step 5.3.1** — Compile and upload the firmware

**Step 5.3.2** — After upload, the ESP32 will attempt to connect to your WiFi network

**Step 5.3.3** — Verify connection using Serial Monitor (optional):

If you have `VERBOSE_MODE` enabled in Config.h, you'll see:

```
[WiFi] Connecting to YourNetworkName...
[WiFi] Connected! IP: 192.168.1.105
[WiFi] WiFi mode enabled. Connected to YourNetworkName
```

### 5.4 Network Configuration

In most cases, WiFi Mode works automatically with no additional configuration. DCS-BIOS broadcasts data via UDP multicast, and CockpitOS listens on the standard address.

**DCS-BIOS Network Settings:**

| Setting | Value | Description |
|---------|-------|-------------|
| Export Address | 239.255.50.10 | UDP multicast group |
| Export Port | 5010 | DCS-BIOS export stream |
| Import Port | 7778 | Commands TO DCS-BIOS |


### 5.5 Start DCS and Test

**Step 5.5.1** — Power on your ESP32 panel

**Step 5.5.2** — Start DCS World

**Step 5.5.3** — Load a mission with your aircraft

**Step 5.5.4** — Your panel should begin receiving data immediately

> **✓ Checkpoint**  
> In WiFi Mode, there's no helper application to show status. The best way to verify operation is to watch your panel — LEDs should light up, displays should show data, and buttons should work in DCS.

### 5.6 WiFi Debugging

If WiFi Mode isn't working, enable debug output to diagnose:

```cpp
// Config.h — Enable WiFi debugging
#define VERBOSE_MODE_WIFI_ONLY                      1  // Send logs via UDP (not serial)
```

We have included a basic Wi-Fi console debugger inside the "Debug Tools" directory called UDP_console_debugger.py

---

## 6. Serial Mode — Legacy Support

Serial Mode is the original method used by DCS-BIOS with Arduino boards. It uses a virtual serial port (CDC) and the `socat` utility to bridge the connection.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         SERIAL MODE ARCHITECTURE                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────┐    USB CDC     ┌──────────────┐    TCP/UDP               │
│   │   ESP32     │◄──────────────►│    socat     │◄──────────────┐          │
│   │ (CockpitOS) │  Virtual COM   │   (bridge)   │               │          │
│   └─────────────┘                └──────────────┘               │          │
│                                                                  │          │
│                                        ┌──────────────┐          │          │
│                                        │   DCS-BIOS   │◄─────────┘          │
│                                        │    (Lua)     │                     │
│                                        └──────────────┘                     │
│                                               │                             │
│                                               ▼                             │
│                                        ┌──────────────┐                     │
│                                        │  DCS World   │                     │
│                                        └──────────────┘                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 6.1 When to Use Serial Mode

- **Debugging**: Serial Mode allows you to see debug output in a terminal
- **Legacy setups**: If you're migrating from Arduino-based DCS-BIOS panels
- **ESP32 Classic**: If you're using ESP32 Classic without WiFi
- **Stream replay**: For testing panels without DCS running

### 6.2 Configure Config.h

```cpp
// Config.h — Transport Mode Selection

// Set EXACTLY ONE of these to 1, all others to 0:
#define USE_DCSBIOS_WIFI                            0
#define USE_DCSBIOS_USB                             0
#define USE_DCSBIOS_SERIAL                          1  // ← Enable Serial Mode
```

For Serial Mode, you may also want verbose output:

```cpp
#define VERBOSE_MODE_SERIAL_ONLY                    1  // Output to serial console
```

### 6.3 Compile and Upload

Compile and upload the firmware. After upload, your ESP32 will appear as a virtual COM port.

### 6.4 Install socat

The `socat` utility bridges the serial port to DCS-BIOS network protocol.

**Windows:**

socat is included with DCS-BIOS. Find `connect-serial-port.cmd` in:
```
%USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\
```

Alternatively, check the `Serial Manager/` directory in CockpitOS for a `connect-serial-port.cmd` copy.

**Linux/Mac:**

```bash
# Ubuntu/Debian
sudo apt install socat

# Mac (Homebrew)
brew install socat
```

### 6.5 Connect with socat

**Step 6.5.1** — Identify your COM port

On Windows, check Device Manager → Ports (COM & LPT). Your ESP32 should appear as a COM port (e.g., COM5).

**Step 6.5.2** — Run the connection script:

```bash
# Windows — run the batch file
connect-serial-port.cmd
# When prompted, enter your COM port number (e.g., 5 for COM5)

# Linux/Mac — run socat directly
socat TCP:127.0.0.1:7778 /dev/ttyUSB0,b115200,raw,echo=0
```

**Step 6.5.3** — Leave the terminal window open while using DCS

> **💡 Tip — Hardcode COM Port**  
> To avoid entering the COM port every time, edit `connect-serial-port.cmd`:
> ```batch
> set COMPORT=5
> ```
> Replace `5` with your actual COM port number.

### 6.6 Start DCS and Test

**Step 6.6.1** — Start DCS World

**Step 6.6.2** — Load a mission

**Step 6.6.3** — Your panel should begin responding

> **✓ Checkpoint**  
> If using `DEBUG_ENABLED`, you should see extended debug messages in your console window. Do not use Serial DEBUG if you are connecting via Serial! use Wi-Fi debug instead. 

---

## 7. Verifying Your Connection

Regardless of transport mode, here's how to verify everything is working:

### 7.1 LED Test

If your panel has LEDs configured:

1. Load a mission in DCS
2. Manipulate cockpit controls that have associated LEDs
3. Your physical LEDs should respond

### 7.2 Button Test

1. Load a mission in DCS
2. Press a button on your physical panel
3. The corresponding action should occur in DCS

### 7.3 Display Test

If your panel has displays (7-segment, LCD, etc.):

1. Load a mission in DCS
2. Values should appear on your displays (fuel, clock, etc.)

### 7.4 Debug Output

Enable debug mode to see what's happening:

```cpp
// Config.h
#define DEBUG_ENABLED                                1
```

You should see messages like:

```
🛩️ [DCS-USB] MASTER_ARM_SW 1
🛩️ [DCS-USB] IFEI_MODE_BTN 1
```

---

## 8. Debugging and Diagnostics

CockpitOS provides built-in debugging tools for each transport mode.

### 8.1 Serial Debug Output

For USB or WiFi modes, you can still get debug output via serial:

```cpp
// Config.h
#define VERBOSE_MODE_SERIAL_ONLY                    1
```

Connect to your ESP32's serial port with Arduino Serial Monitor or any terminal at 250000 baud.

### 8.2 WiFi Debug Console

Send debug messages over UDP to your PC:

```cpp
// Config.h
#define VERBOSE_MODE_WIFI_ONLY                      1
```

Listen on UDP port 5005 on your PC to receive debug messages.

### 8.3 Performance Monitoring

Enable real-time performance stats:

```cpp
// Config.h
#define DEBUG_PERFORMANCE                           1
#define PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS      60
```

This outputs timing information for each firmware phase every 60 seconds.

### 8.4 Interactive LED Test

Test individual LEDs via serial console:

```cpp
// Config.h
#define TEST_LEDS                                   1
```

After boot, the serial console will show an interactive menu to test each LED.

### 8.5 Stream Replay

Test your panel without DCS running:

```cpp
// Config.h
#define IS_REPLAY                                   1
```

This replays a captured DCS-BIOS stream to verify panel operation. See ReplayData directory

---

## 9. Troubleshooting

### 9.1 USB Mode Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  HID Manager shows "WAIT HANDSHAKE" indefinitely                            │
├─────────────────────────────────────────────────────────────────────────────┤
│  POSSIBLE CAUSES & SOLUTIONS                                                │
│                                                                             │
│  1. Firmware not running properly                                           │
│     → Press RST button to restart the ESP32                                 │
│     → Verify firmware uploaded successfully                                 │
│                                                                             │
│  2. Wrong USB Mode setting (S3/P4)                                          │
│     → Check Tools → USB Mode → "USB-OTG (TinyUSB)"                         │
│     → Recompile and re-upload                                               │
│                                                                             │
│  3. USB CDC On Boot enabled                                                 │
│     → Check Tools → USB CDC On Boot → "Disabled"                            │
│     → Recompile and re-upload                                               │
│                                                                             │
│  4. Device not properly enumerated                                          │
│     → Try a different USB port                                              │
│     → Try a different USB cable                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  HID Manager shows "0 device(s) connected"                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│  POSSIBLE CAUSES & SOLUTIONS                                                │
│                                                                             │
│  1. VID mismatch                                                            │
│     → Check settings.ini VID matches firmware (default 0xCAFE)              │
│                                                                             │
│  2. Device not appearing as HID                                             │
│     → Check Device Manager for "CockpitOS" under HID devices               │
│     → If not present, check USB Mode settings and recompile                │
│                                                                             │
│  3. Permission issue (Linux)                                                │
│     → Add udev rules (see Section 4.4)                                      │
│     → Or run with sudo (not recommended for production)                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 9.2 WiFi Mode Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  ESP32 not connecting to WiFi                                               │
├─────────────────────────────────────────────────────────────────────────────┤
│  POSSIBLE CAUSES & SOLUTIONS                                                │
│                                                                             │
│  1. Wrong credentials                                                       │
│     → Double-check WIFI_SSID and WIFI_PASS in Config.h                     │
│     → SSIDs are case-sensitive                                             │
│                                                                             │
│  2. Network not in range                                                    │
│     → Move ESP32 closer to router                                           │
│     → Check with SCAN_WIFI_NETWORKS = 1                                    │
│                                                                             │
│  3. 5GHz only network                                                       │
│     → ESP32 works best with 2.4GHz networks                                │
│     → Check router settings for 2.4GHz band                                │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  WiFi connected but no DCS data                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│  POSSIBLE CAUSES & SOLUTIONS                                                │
│                                                                             │
│  1. DCS-BIOS not running                                                    │
│     → Start DCS and load a mission                                          │
│     → Verify DCS-BIOS installation                                         │
│                                                                             │
│  2. Multicast blocked                                                       │
│     → Check router/firewall settings                                        │
│     → Some corporate networks block multicast                              │
│                                                                             │
│  3. Different network segments                                              │
│     → PC and ESP32 must be on same subnet                                  │
│     → Check IP addresses (should have same first 3 octets)                 │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 9.3 Serial Mode Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  socat shows "connection refused" or similar                                │
├─────────────────────────────────────────────────────────────────────────────┤
│  POSSIBLE CAUSES & SOLUTIONS                                                │
│                                                                             │
│  1. DCS-BIOS not running                                                    │
│     → Start DCS World first                                                 │
│     → Load a mission before connecting                                     │
│                                                                             │
│  2. Wrong COM port                                                          │
│     → Check Device Manager for correct port number                          │
│     → Port may change if you use different USB port                        │
│                                                                             │
│  3. Port already in use                                                     │
│     → Close Arduino IDE Serial Monitor                                      │
│     → Close any other terminal programs                                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 9.4 General Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  Panel works initially but stops responding                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│  POSSIBLE CAUSES & SOLUTIONS                                                │
│                                                                             │
│  1. Mission ended or paused                                                 │
│     → CockpitOS requires an active mission                                  │
│     → Resume or restart the mission                                        │
│                                                                             │
│  2. DCS-BIOS stream timeout                                                 │
│     → CockpitOS detects when stream stops                                   │
│     → Will auto-resume when stream restarts                                │
│                                                                             │
│  3. USB connection dropped (USB Mode)                                       │
│     → Check HID Manager for reconnection count                             │
│     → Try a powered USB hub for stability                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 10. Quick Reference

### 10.1 Config.h Transport Settings

```cpp
// Choose ONE and set to 1:
#define USE_DCSBIOS_WIFI                            0
#define USE_DCSBIOS_USB                             1  // ← Recommended
#define USE_DCSBIOS_SERIAL                          0

// WiFi credentials (only for WiFi Mode):
#define WIFI_SSID                                  "YourNetwork"
#define WIFI_PASS                                  "YourPassword"
```

### 10.2 Arduino IDE Settings (USB Mode)

| Setting | Value |
|---------|-------|
| Board | Your ESP32-S2/S3/P4 board |
| USB CDC On Boot | **Disabled** |
| USB Mode (S3/P4) | **USB-OTG (TinyUSB)** |

### 10.3 Required Software by Mode

| Mode | PC Software Required |
|------|---------------------|
| USB | Python 3.8+, hidapi, filelock, HID_Manager.py |
| WiFi | None |
| Serial | socat, connect-serial-port.cmd |

### 10.4 Network Ports

| Port | Protocol | Direction | Purpose |
|------|----------|-----------|---------|
| 5010 | UDP Multicast | DCS → Panel | Export stream |
| 7778 | UDP Unicast | Panel → DCS | Commands |
| 5005 | UDP | Panel → PC | Debug output (optional) |

### 10.5 Startup Order

**USB Mode:**
1. Connect ESP32 panel via USB
2. Run HID_Manager.py
3. Start DCS World
4. Load mission

**WiFi Mode:**
1. Power on ESP32 panel (waits for WiFi)
2. Start DCS World
3. Load mission

**Serial Mode:**
1. Connect ESP32 panel via USB
2. Run connect-serial-port.cmd
3. Start DCS World
4. Load mission

---

## What's Next?

With your transport mode configured and working, you can:

1. **Add more controls** — Edit your Label Set's InputMapping.h
2. **Add more LEDs** — Edit your Label Set's LEDMapping.h
3. **Build additional panels** — Create new Label Sets for different aircraft or cockpit sections
4. **Fine-tune performance** — Adjust polling rates and buffer sizes in Config.h

---

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│    Need help?                                                               │
│                                                                             │
│    • GitHub Issues: github.com/BojoteX/CockpitOS/issues                     │
│    • Check the README.md in each directory for specific guidance            │
│    • A custom-trained GPT assistant is available — see the                  │
│      GitHub page for the link                                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

*Guide version: 1.0 | Last updated: January 2025*  
*Tested with: CockpitOS 1.x, DCS-BIOS Skunkworks, ESP32 Arduino Core 3.3.x*
