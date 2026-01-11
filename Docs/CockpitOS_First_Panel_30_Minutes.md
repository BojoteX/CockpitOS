# CockpitOS — Your First Panel in 30 Minutes (WiFi)

Build a working **F/A-18C Hornet panel** on an **ESP32-S3**, connected wirelessly to DCS World.

> **Why WiFi?** No bridge software needed. Your panel talks directly to DCS-BIOS over your network.

---

## What You'll Build

A simple test panel. **You don't need all of these** — even one button + one LED is enough!

| Component | DCS Function | GPIO Pin |
|-----------|--------------|----------|
| Push Button | Master Caution Reset | GPIO 5 |
| Toggle Switch | Master Arm (ARM/SAFE) | GPIO 6 |
| Potentiometer | RWR Volume | GPIO 4 |
| LED | Master Caution Light | GPIO 7 |

---

## 1) Install Prerequisites

### Arduino IDE
Download **Arduino IDE 2.x** from https://www.arduino.cc/en/software

### ESP32 Board Support
1. **Tools → Board → Boards Manager**
2. Search **"esp32"**, install **"esp32 by Espressif Systems"** version **3.3.5**

### Python 3
Type `python` in a command prompt — Windows will offer to install from Microsoft Store. Or download from python.org.

### DCS-BIOS
Install from https://github.com/DCS-Skunkworks/dcs-bios/releases

---

## 2) Get CockpitOS

1. Download from https://github.com/BojoteX/CockpitOS/releases/
2. Unzip to `My Documents\Arduino\CockpitOS\` (example):
   - `My Documents\Arduino\CockpitOS\CockpitOS.ino` (Make sure the directory structure matches this)
3. Confirm you see:
   - `CockpitOS.ino`
   - `Config.h`
   - `src\LABELS\...`

---

## 3) Run the Generator

Open a command prompt:
```
cd Documents\Arduino\CockpitOS\src\LABELS\LABEL_SET_TEST_ONLY
python generate_data.py
```

Done. The label set includes the F/A-18C Hornet with Master Arm Panel, Communication Panel, and Master Caution Light.

---

## 4) Edit InputMapping.h

Open `src\LABELS\LABEL_SET_TEST_ONLY\InputMapping.h`

Find these entries and change `"NONE"` to `"GPIO"` with your pin numbers:

### Push Button (Master Caution Reset)
```cpp
{ "MASTER_CAUTION_RESET_SW", "GPIO", 5, 0, -1, "MASTER_CAUTION_RESET_SW", 1, "momentary", 0 },
```

### Toggle Switch (Master Arm — 2 positions)
```cpp
{ "MASTER_ARM_SW_ARM",  "GPIO", 6,  0, -1, "MASTER_ARM_SW", 1, "selector", 1 },
{ "MASTER_ARM_SW_SAFE", "GPIO", 6, -1, -1, "MASTER_ARM_SW", 0, "selector", 1 },
```

### Potentiometer (RWR Volume)
```cpp
{ "COM_RWR", "GPIO", 4, 0, -1, "COM_RWR", 65535, "analog", 0 },
```

Only add what you're wiring. Skip the rest.

---

## 5) Edit LEDMapping.h

Open `src\LABELS\LABEL_SET_TEST_ONLY\LEDMapping.h`

Find `MASTER_CAUTION_LT`:
```cpp
{ "MASTER_CAUTION_LT", DEVICE_GPIO, {.gpioInfo = {7}}, false, false },
```

---

## 6) Wire Your Hardware

```
ESP32-S3
─────────────────────────────
3.3V ──── Pot left pin
GND  ──── Pot right / Button / Switch / LED (-)

GPIO 4 ── Pot wiper (middle)
GPIO 5 ── Push Button (other leg to GND)
GPIO 6 ── Toggle Switch center (one side to GND)
GPIO 7 ── 330Ω ── LED (+)
```

Use **3.3V only**. Wire only what you need.

---

## 7) Configure WiFi

Open `Config.h`:

```cpp
#define USE_DCSBIOS_WIFI                            1
#define USE_DCSBIOS_USB                             0
#define USE_DCSBIOS_SERIAL                          0

#define WIFI_SSID                                  "YourNetwork"
#define WIFI_PASS                                  "YourPassword"
```

ESP32 and PC must be on the same **2.4GHz** network.

To see debug output, also set:
```cpp
#define VERBOSE_MODE_WIFI_ONLY                      1
```

---

## 8) Arduino IDE Settings

| Setting | Value |
|---------|-------|
| Board | ESP32S3 Dev Module |
| **USB CDC On Boot** | **Disabled** ⚠️ |
| USB Mode | USB-OTG (TinyUSB) |
| Port | Your ESP32's port |

To find the port: unplug ESP32, note which port disappears, plug back in.

---

## 9) Upload

1. Click **Verify** (checkmark)
2. Click **Upload** (arrow)

If upload fails, hold **BOOT** button while plugging in USB.

---

## 10) Test

### Debug Console
Run `CockpitOS\Debug Tools\UDP_console_debugger.py` to see WiFi status and commands.

### In DCS
1. Load an F/A-18C mission
2. Press your button — see it in the debug console
3. In cockpit, trigger Master Caution — your LED lights up!

---

## Troubleshooting

| Problem | Fix |
|---------|-----|
| WiFi won't connect | Check SSID/password, must be 2.4GHz |
| Compile error about USB CDC | Tools → USB CDC On Boot → Disabled |
| No DCS data | Check DCS-BIOS is installed. If firewall issues, allow UDP 5010/7778 |

Questions? https://github.com/BojoteX/CockpitOS/discussions

---

## Quick Checklist

- [ ] Arduino IDE + ESP32 Core 3.3.5
- [ ] Python installed
- [ ] DCS-BIOS installed
- [ ] Ran `python generate_data.py`
- [ ] InputMapping.h edited
- [ ] LEDMapping.h edited
- [ ] Config.h: WiFi enabled + credentials
- [ ] USB CDC On Boot = Disabled
- [ ] Uploaded successfully
- [ ] Tested in DCS

---

*CockpitOS | WiFi Quick Start | F/A-18C*
