# CockpitOS — Your First Panel in 30 Minutes (WiFi)

This guide walks you from a clean PC to a working **F/A-18C Hornet panel** on an **ESP32-S3** using **CockpitOS** firmware, connected **wirelessly via WiFi** to **DCS World** through **DCS-BIOS**.

> **Why WiFi?** No bridge software required on your PC! Your panel connects directly to DCS-BIOS over your local network. Just power it up and fly.

---

## What You'll Build

By the end of this guide, you'll have a working panel with:

| Component | DCS Function | GPIO Pin |
|-----------|--------------|----------|
| Push Button | Master Caution Reset | GPIO 5 |
| Toggle Switch | Hook Bypass (CARRIER/FIELD) | GPIO 6 |
| Potentiometer | HUD Symbol Brightness | GPIO 4 |
| LED | Master Caution Indicator | GPIO 7 |

---

## 1) Install Prerequisites

### 1.1 Arduino IDE

1. Download and install **Arduino IDE 2.3.7** (or newer) from https://www.arduino.cc/en/software
2. Launch Arduino IDE once so it finishes first-time setup.

### 1.2 ESP32 Board Support (Arduino Core)

1. In Arduino IDE, go to **File → Preferences**
2. In "Additional boards manager URLs", add:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
3. Go to **Tools → Board → Boards Manager**
4. Search **"esp32"** and install **"esp32 by Espressif Systems"**
5. Select version **3.3.5** (or newer, but 3.3.x recommended).

### 1.3 Python 3 (for the CockpitOS generator)

1. Download and install **Python 3.x** from https://www.python.org/downloads/
2. **IMPORTANT:** During install on Windows, check **"Add Python to PATH"**
3. Verify installation — open a terminal (PowerShell/CMD) and run:
   ```bash
   python --version
   ```
   You should see `Python 3.x.x`

### 1.4 DCS-BIOS (DCS World side)

1. Download DCS-BIOS from https://github.com/DCS-Skunkworks/dcs-bios/releases
2. Follow the DCS-BIOS installation instructions
3. Confirm the folder exists:
   ```
   %USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\
   ```
4. Ensure DCS-BIOS is active in your `Export.lua`

> **Note:** DCS-BIOS installation is a one-time setup. If you already have it working with other panels, skip this step.

---

## 2) Get CockpitOS Firmware

1. Download the latest release from https://github.com/BojoteX/CockpitOS/releases/
2. Unzip into your Arduino folder:
   ```
   My Documents\Arduino\CockpitOS\
   ```
3. Confirm you see this structure:
   ```
   CockpitOS\
   ├── CockpitOS.ino
   ├── Config.h
   ├── Pins.h
   └── src\
       └── LABELS\
           └── LABEL_SET_TEST_ONLY\
   ```

---

## 3) Get the F/A-18C Aircraft JSON

CockpitOS needs the aircraft definition file from DCS-BIOS.

1. Navigate to:
   ```
   %USERPROFILE%\Saved Games\DCS\Scripts\DCS-BIOS\doc\json\
   ```
2. Copy **`FA-18C_hornet.json`** to:
   ```
   <your CockpitOS folder>\src\LABELS\LABEL_SET_TEST_ONLY\
   ```
3. **Delete any other `.json` files** in that folder (only ONE JSON file allowed per label set).

---

## 4) Configure Selected Panels

Edit the file **`selected_panels.txt`** in the `LABEL_SET_TEST_ONLY` folder.

Replace its entire contents with:

```
# Minimal test panel - Master Caution + Hook Bypass + HUD Brightness
MASTER CAUTION / FIRE WARNING
ARRESTING HOOK
HEADS UP DISPLAY CONTROL PANEL
```

> **Note:** Panel names must match **exactly** what appears in `panels.txt` (case-sensitive).

---

## 5) Run the Generator Script

This creates the mapping files CockpitOS needs.

1. Open a terminal (PowerShell/CMD)
2. Navigate to the label set folder:
   ```bash
   cd "My Documents\Arduino\CockpitOS\src\LABELS\LABEL_SET_TEST_ONLY"
   ```
3. Run the generator:
   ```bash
   python generate_data.py
   ```

You should see output like:
```
✓ Generated DCSBIOSBridgeData.h
✓ Generated InputMapping.h
✓ Generated LEDMapping.h
✓ Set LABEL_SET_TEST_ONLY as active
```

**What this does:**
- Marks **TEST_ONLY** as the active label set (updates `active_set.h`)
- Generates address tables from the aircraft JSON
- Creates template InputMapping.h and LEDMapping.h files

> **If you ever change panels or label sets, you must re-run the generator.**

---

## 6) Edit InputMapping.h — Assign GPIO Pins

Open **`src/LABELS/LABEL_SET_TEST_ONLY/InputMapping.h`** in a text editor.

Find and modify these entries to assign your GPIO pins:

### 6.1 Push Button (Master Caution Reset)

Find the line containing `MASTER_CAUTION_RESET_SW` and change it to:

```cpp
{ "MASTER_CAUTION_RESET_SW", "GPIO", 5, 0, -1, "MASTER_CAUTION_RESET_SW", 1, "momentary", 0 },
```

**Field meanings:**
- `"GPIO"` — Direct GPIO input (not shift register or I²C)
- `5` — GPIO pin 5
- `"momentary"` — Button press sends command, release does nothing

### 6.2 Toggle Switch (Hook Bypass — 2 positions)

Find entries for `HOOK_BYPASS_SW`. You need **TWO** entries for a 2-position switch:

```cpp
{ "HOOK_BYPASS_SW_CARRIER", "GPIO", 6,  0, -1, "HOOK_BYPASS_SW", 0, "selector", 1 },
{ "HOOK_BYPASS_SW_FIELD",   "GPIO", 6, -1, -1, "HOOK_BYPASS_SW", 1, "selector", 1 },
```

**Field meanings:**
- Both entries share `group = 1` (they belong to the same physical switch)
- First entry: `bit = 0` — active when GPIO reads LOW
- Second entry: `bit = -1` — fallback/default when GPIO reads HIGH
- `"selector"` — Multi-position selector type

### 6.3 Potentiometer (HUD Brightness)

Find the entry for `HUD_SYM_BRT` and change it to:

```cpp
{ "HUD_SYM_BRT", "GPIO", 4, 0, -1, "HUD_SYM_BRT", 65535, "analog", 0 },
```

**Field meanings:**
- `4` — GPIO 4 (must be ADC-capable pin on ESP32-S3)
- `65535` — Full 16-bit analog range
- `"analog"` — Reads ADC value and scales to DCS-BIOS range

Save the file.

---

## 7) Edit LEDMapping.h — Assign LED Pin

Open **`src/LABELS/LABEL_SET_TEST_ONLY/LEDMapping.h`**.

Find the entry for `MASTER_CAUTION_LT` and change it to:

```cpp
{ "MASTER_CAUTION_LT", DEVICE_GPIO, {.gpioInfo = {7}}, false, false },
```

**Field meanings:**
- `DEVICE_GPIO` — Standard GPIO output
- `{7}` — GPIO pin 7
- First `false` — Not dimmable (on/off only)
- Second `false` — Not active-low (HIGH = LED ON)

Save the file.

---

## 8) Wire Your Hardware

### 8.1 Wiring Diagram

```
ESP32-S3 Development Board
┌─────────────────────────────────────────┐
│                                         │
│  3.3V ─────┬─────────────── Pot VCC     │
│            │                (left pin)  │
│            │                            │
│  GND ──────┼─────────────── Pot GND     │
│            │                (right pin) │
│            │                            │
│            ├─────────────── Button GND  │
│            │                            │
│            ├─────────────── Switch GND  │
│            │                            │
│            └─────────────── LED (-)     │
│                                         │
│  GPIO 4 ───────────────────── Pot Wiper │
│                                (middle) │
│                                         │
│  GPIO 5 ───────────────────── Button    │
│                           (other → GND) │
│                                         │
│  GPIO 6 ───────────────────── Switch    │
│                         (common/center) │
│                       (one leg → GND)   │
│                                         │
│  GPIO 7 ──── 330Ω ──── LED (+)          │
│                                         │
└─────────────────────────────────────────┘
```

### 8.2 Wiring Notes

| Component | Connection Details |
|-----------|-------------------|
| **Push Button** | One leg to GPIO 5, other leg to GND. ESP32's internal pull-up keeps it HIGH until pressed. |
| **Toggle Switch** | Common/center pin to GPIO 6. One outer pin to GND. Position toward GND = CARRIER, away = FIELD. |
| **Potentiometer** | Left pin → 3.3V, Right pin → GND, Middle (wiper) → GPIO 4 |
| **LED** | GPIO 7 → 330Ω resistor → LED anode (+). LED cathode (-) → GND. |

> ⚠️ **IMPORTANT:** Use **3.3V**, NOT 5V! ESP32 GPIOs are 3.3V tolerant only.

---

## 9) Configure WiFi Transport

Open **`Config.h`** in the CockpitOS root folder.

### 9.1 Set Transport Mode

Find the transport section and enable **only WiFi**:

```cpp
#define USE_DCSBIOS_BLUETOOTH                       0
#define USE_DCSBIOS_WIFI                            1  // ← Enable this
#define USE_DCSBIOS_USB                             0
#define USE_DCSBIOS_SERIAL                          0
```

### 9.2 Set Your WiFi Credentials

Find the WiFi section and enter your network details:

```cpp
#define WIFI_SSID                                  "YourNetworkName"
#define WIFI_PASS                                  "YourWiFiPassword"
```

> ⚠️ **IMPORTANT:** The ESP32 and your DCS PC **must be on the same network**. ESP32 only supports **2.4GHz** WiFi (not 5GHz).

### 9.3 Enable Verbose Mode (for testing)

For your first test, enable verbose output:

```cpp
#define VERBOSE_MODE                                1  // ← Enable for testing
#define VERBOSE_MODE_SERIAL_ONLY                    1  // ← Output to Serial Monitor
```

Save the file.

---

## 10) Configure Arduino IDE Board Settings

### 10.1 Select Your Board

1. Go to **Tools → Board → esp32**
2. Select **"ESP32S3 Dev Module"** (or your specific board if listed)

### 10.2 Critical Settings

In the **Tools** menu, set these options:

| Setting | Value | Why |
|---------|-------|-----|
| **USB CDC On Boot** | **Disabled** | ⚠️ CRITICAL — compile will fail if Enabled |
| **USB Mode** | **Hardware CDC and JTAG** | For Serial Monitor access |
| **Flash Size** | 4MB (or match your board) | — |
| **Partition Scheme** | Default 4MB with spiffs | — |
| **PSRAM** | Disabled | Unless your board has PSRAM |
| **Upload Speed** | 921600 | Faster uploads |

### 10.3 Select Your Port

1. Connect your ESP32-S3 via a **data-capable** USB cable
2. Go to **Tools → Port**
3. **How to identify the correct port:** Disconnect the device, note which port disappears, reconnect, and select the port that reappears.

> **If no port appears:** Your cable may be charge-only (no data). Try a different cable.

### 10.4 Bootloader Mode (if upload fails)

Some ESP32-S3 boards require entering bootloader mode for first upload:

1. **Unplug** the USB cable
2. **Hold the BOOT button** (small button, often labeled "BOOT" or "0")
3. **While holding**, plug in the USB cable
4. **Release** the button after you hear the Windows connect sound
5. The board should now appear as a COM port

---

## 11) Compile and Upload

1. Click **Verify** (checkmark icon) to compile
2. Watch the output for errors
3. If compile succeeds, click **Upload** (arrow icon)
4. Wait for "Done uploading" message

### 11.1 Common Compile Errors

| Error | Solution |
|-------|----------|
| `"LABEL_SET not defined"` | Re-run `python generate_data.py` in your label set folder |
| `"USB CDC On Boot"` error | Set **Tools → USB CDC On Boot → Disabled** |
| `"undefined reference to InputMappings"` | Re-run the generator script |
| Any other error | Verify ESP32 Core version is 3.3.x |

---

## 12) Configure Windows Firewall

DCS-BIOS uses UDP multicast. Windows Firewall must allow this traffic.

### Option A: PowerShell (Run as Administrator)

```powershell
# Allow DCS-BIOS export (multicast from DCS)
netsh advfirewall firewall add rule name="DCS-BIOS Export UDP 5010" dir=in action=allow protocol=UDP localport=5010

# Allow DCS-BIOS import (commands from panel)
netsh advfirewall firewall add rule name="DCS-BIOS Import UDP 7778" dir=in action=allow protocol=UDP localport=7778
```

### Option B: Manual

1. Open **Windows Defender Firewall with Advanced Security**
2. Create **Inbound Rules** for:
   - UDP Port 5010 (Allow)
   - UDP Port 7778 (Allow)

---

## 13) Test Your Panel!

### 13.1 Open Serial Monitor

1. In Arduino IDE: **Tools → Serial Monitor**
2. Set baud rate to **115200** (bottom-right dropdown)
3. Press the **RST** button on your ESP32

You should see:
```
[WiFi] Connecting to YourNetworkName...
[WiFi] Connected! IP: 192.168.1.xxx
[DCS] Listening on 239.255.50.10:5010
[INIT] CockpitOS ready!
```

### 13.2 Launch DCS World

1. Start DCS World
2. Load a mission with the **F/A-18C Hornet**
3. Watch the Serial Monitor for:
   ```
   [DCS] Receiving data from 192.168.1.xxx
   [SYNC] Aircraft: FA-18C_hornet
   ```

### 13.3 Test Each Component

| Test | Action | Expected Serial Output |
|------|--------|----------------------|
| Push Button | Press and release | `MASTER_CAUTION_RESET_SW 1` |
| Toggle Switch | Flip to each position | `HOOK_BYPASS_SW 0` or `HOOK_BYPASS_SW 1` |
| Potentiometer | Rotate full range | `HUD_SYM_BRT xxxxx` (values 0-65535) |
| LED | Trigger Master Caution in DCS | LED lights up! |

**To trigger Master Caution in DCS:** In the Hornet cockpit, pull some circuit breakers or create a fault. The yellow MASTER CAUTION light should illuminate in-game and on your panel.

---

## 14) Troubleshooting

### WiFi Issues

| Problem | Solution |
|---------|----------|
| "WiFi won't connect" | Double-check SSID/password (case-sensitive). Ensure 2.4GHz network. |
| "Connected but no DCS data" | Check firewall rules (Step 12). Verify same network subnet. |
| WiFi disconnects randomly | Try a more stable 2.4GHz channel. Reduce distance to router. |

### Hardware Issues

| Problem | Solution |
|---------|----------|
| LED doesn't light | Check polarity (long leg = anode = +). Verify resistor in series. |
| Button not detected | Check wiring. GPIO should read HIGH when open (internal pull-up). |
| Pot values jumping | Add 0.1µF capacitor between wiper and GND for stability. |

### DCS Connection Issues

| Problem | Solution |
|---------|----------|
| No data from DCS | Verify DCS-BIOS installed and running. Check Export.lua. |
| Wrong aircraft | Ensure `FA-18C_hornet.json` is in your label set folder. |
| Commands not reaching DCS | Check UDP 7778 firewall rule. |

---

## 15) Files You'll Commonly Touch

| File | Purpose |
|------|---------|
| `Config.h` | Transport selection (WiFi/USB/Serial), WiFi credentials, debug flags |
| `src/LABELS/LABEL_SET_xxx/selected_panels.txt` | Which DCS panels to include |
| `src/LABELS/LABEL_SET_xxx/InputMapping.h` | Button/switch/pot GPIO assignments |
| `src/LABELS/LABEL_SET_xxx/LEDMapping.h` | LED GPIO assignments |

---

## 16) Quick Checklist

- [ ] Arduino IDE 2.3.7+ installed
- [ ] ESP32 Core 3.3.5+ installed
- [ ] Python 3.x installed and in PATH
- [ ] DCS-BIOS installed in DCS World
- [ ] `FA-18C_hornet.json` copied to label set folder
- [ ] `selected_panels.txt` edited with correct panels
- [ ] Ran `python generate_data.py` successfully
- [ ] `InputMapping.h` edited with GPIO pins
- [ ] `LEDMapping.h` edited with LED pin
- [ ] Hardware wired correctly (3.3V, not 5V!)
- [ ] `Config.h`: `USE_DCSBIOS_WIFI = 1`
- [ ] `Config.h`: WiFi SSID and password set
- [ ] Arduino IDE: `USB CDC On Boot = Disabled`
- [ ] Firmware uploaded successfully
- [ ] Firewall rules added (UDP 5010, 7778)
- [ ] Serial Monitor shows WiFi connected
- [ ] Tested in F/A-18C mission

---

## What's Next?

**Congratulations!** You've built your first wireless CockpitOS panel! 🎉

### Expand Your Panel
- Add more buttons/switches to available GPIOs
- Add WS2812 RGB LEDs for multi-color indicators
- Use 74HC165 shift registers for 64+ digital inputs
- Use PCA9555 I²C expanders for massive I/O

### Build Real Panels
- Design a PCB or laser-cut faceplate
- Add TFT displays (IFEI, CMWS, etc.)
- Add TM1637 7-segment displays
- Build complete console replicas

### Join the Community
- **Open Hornet Project** — Full F/A-18C pit builders
- **DCS-BIOS Discord** — Protocol help and tips
- **CockpitOS GitHub** — Report issues, contribute code

---

## One Actionable Takeaway

**Use `VERBOSE_MODE = 1` during development.** It shows you exactly what CockpitOS is doing — WiFi connection status, DCS-BIOS data flow, command transmission. Once everything works reliably, set it back to `0` for cleaner operation.

---

*Guide Version: 2.0 | CockpitOS | WiFi Transport | F/A-18C Hornet*
