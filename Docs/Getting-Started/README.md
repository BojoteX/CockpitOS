# CockpitOS -- Getting Started

> **Your first flight starts here.**
> This guide walks you through setting up your computer, installing CockpitOS, and uploading firmware to your ESP32 -- step by step, with nothing assumed.

---

## Table of Contents

1. [What You Need](#1-what-you-need)
2. [Download CockpitOS](#2-download-cockpitos)
3. [Run the Setup Tool](#3-run-the-setup-tool)
4. [Create Your First Label Set](#4-create-your-first-label-set)
5. [Compile and Upload](#5-compile-and-upload)
6. [Configure Transport Mode](#6-configure-transport-mode)
7. [Test in DCS](#7-test-in-dcs)
8. [What's Next?](#8-whats-next)

---

## 1. What You Need

Before you begin, gather everything on this checklist:

```
+-----------------------------------------------------------------------+
|  CHECKLIST                                                            |
+-----------------------------------------------------------------------+
|  [ ] ESP32 board (S2, S3, or P4 recommended for USB HID)             |
|  [ ] USB cable that supports DATA transfer (not charge-only!)         |
|  [ ] Windows PC (the Python tools are Windows-only)                   |
|  [ ] Python 3.12 or newer installed                                   |
|  [ ] DCS World with DCS-BIOS installed                                |
|  [ ] Internet connection (for initial setup only)                     |
+-----------------------------------------------------------------------+
```

```
+-----------------------------------------------------------------------+
|  TIP -- Which ESP32 Should I Buy?                                     |
+-----------------------------------------------------------------------+
|                                                                       |
|  ESP32-S3  -->  Best all-around choice (USB HID + WiFi + BLE)         |
|  ESP32-S2  -->  Good budget option (USB HID + WiFi)                   |
|  ESP32-P4  -->  High performance (USB HID + WiFi)                     |
|  ESP32 Classic --> WiFi or Serial only (no native USB HID)            |
|                                                                       |
|  If you want USB HID (the recommended transport), you MUST use        |
|  an S2, S3, or P4. The Classic ESP32 does not support native USB.     |
+-----------------------------------------------------------------------+
```

```
+-----------------------------------------------------------------------+
|  WARNING -- USB Cables                                                |
+-----------------------------------------------------------------------+
|  Many USB cables are "charge-only" and cannot transfer data.          |
|  If your computer does not detect your board, try a different cable   |
|  before doing any other troubleshooting.                              |
+-----------------------------------------------------------------------+
```

### Install Python

If you do not have Python yet, download it from [python.org](https://www.python.org/downloads/). During installation, **check the box that says "Add Python to PATH"**. CockpitOS requires Python 3.12 or newer.

You can verify your installation by opening a command prompt and typing:

```
python --version
```

You should see something like `Python 3.12.x` or newer.

### Install DCS-BIOS

DCS-BIOS is the bridge between DCS World and your hardware panel. Download and install it from:

**https://github.com/DCS-Skunkworks/dcs-bios/releases**

Follow the DCS-BIOS installation instructions on that page. CockpitOS needs DCS-BIOS running inside DCS World to send and receive cockpit data.

---

## 2. Download CockpitOS

**Step 2.1** -- Go to the CockpitOS releases page:

**https://github.com/BojoteX/CockpitOS/releases/**

**Step 2.2** -- Download the latest release ZIP file

**Step 2.3** -- Extract the ZIP file to your Arduino directory:

```
C:\Users\<YourName>\Documents\Arduino\CockpitOS\
```

**Step 2.4** -- Verify the folder structure looks like this:

```
Documents\Arduino\
  +-- CockpitOS\
        +-- CockpitOS.ino        <-- This file MUST exist here
        +-- Config.h
        +-- Pins.h
        +-- Mappings.h
        +-- Setup-START.py       <-- Setup Tool
        +-- CockpitOS-START.py   <-- Compiler Tool
        +-- LabelCreator-START.py <-- Label Creator
        +-- src\
        |     +-- LABELS\        <-- Label Sets live here
        +-- lib\
        +-- ...
```

```
+-----------------------------------------------------------------------+
|  WARNING -- Folder Name                                               |
+-----------------------------------------------------------------------+
|  The folder containing CockpitOS.ino MUST be named exactly            |
|  "CockpitOS". Arduino requires the folder name to match the .ino     |
|  filename.                                                            |
|                                                                       |
|  WRONG:   Arduino\CockpitOS-main\CockpitOS.ino                       |
|  WRONG:   Arduino\CockpitOS-v2.0\CockpitOS.ino                       |
|  CORRECT: Arduino\CockpitOS\CockpitOS.ino                            |
+-----------------------------------------------------------------------+
```

> **Checkpoint:** You should see `CockpitOS.ino`, `Config.h`, and the three Python tool scripts in the CockpitOS folder.

---

## 3. Run the Setup Tool

The Setup Tool installs everything you need to compile CockpitOS -- no Arduino IDE required. It uses a bundled `arduino-cli` and handles all the configuration automatically.

**Step 3.1** -- Double-click `Setup-START.py` in the CockpitOS folder

Or run it from a command prompt:

```
cd Documents\Arduino\CockpitOS
python Setup-START.py
```

**Step 3.2** -- The Setup Tool opens a terminal menu:

```
+-----------------------------------------------------------------------+
|                                                                       |
|  CockpitOS Setup Tool                                                 |
|                                                                       |
|  [1] Install / Update                                                 |
|  [2] Reset to Manifest Versions                                       |
|  [Q] Quit                                                             |
|                                                                       |
+-----------------------------------------------------------------------+
```

**Step 3.3** -- Select option **1** (Install / Update)

The tool will:
- Download and configure the bundled `arduino-cli`
- Install **ESP32 Arduino Core v3.3.6** (the exact version CockpitOS is tested with)
- Install **LovyanGFX v1.2.19** (required for TFT display support)

Components are installed to standard Arduino locations:
- ESP32 Core: `%LOCALAPPDATA%\Arduino15\`
- Libraries: `Documents\Arduino\libraries\`

```
+-----------------------------------------------------------------------+
|  NOTE -- Installation Time                                            |
+-----------------------------------------------------------------------+
|  The first install downloads several hundred MB of data.              |
|  This typically takes 5-15 minutes depending on your internet speed.  |
|  Subsequent runs are much faster since they only update if needed.    |
+-----------------------------------------------------------------------+
```

**Step 3.4** -- When setup completes, the tool offers to launch the Compiler Tool. You can accept, or come back to it later.

```
+-----------------------------------------------------------------------+
|  TIP -- Reset to Manifest Versions                                    |
+-----------------------------------------------------------------------+
|  If you ever run into compilation issues after updating libraries,    |
|  use option [2] in the Setup Tool to reset everything back to the     |
|  exact versions specified in the CockpitOS manifest. This is the      |
|  fastest way to fix "it was working yesterday" problems.              |
+-----------------------------------------------------------------------+
```

> **Checkpoint:** The Setup Tool should finish with a success message. You now have everything needed to compile CockpitOS firmware.

---

## 4. Create Your First Label Set

A **Label Set** is the configuration that tells CockpitOS which aircraft you are building a panel for, which cockpit panels to include, and how your physical inputs and outputs are mapped. The Label Creator tool handles all of this through a guided menu -- no manual file editing required.

**Step 4.1** -- Double-click `LabelCreator-START.py` in the CockpitOS folder

Or run from a command prompt:

```
python LabelCreator-START.py
```

**Step 4.2** -- Select **"Create New Label Set"** from the main menu

**Step 4.3** -- Pick your aircraft from the list (for example, `FA-18C_hornet`)

**Step 4.4** -- Give your label set a name (for example, `MY_FIRST_PANEL`)

**Step 4.5** -- Select which cockpit panels to include. The tool shows you all available panels for your aircraft. Pick the ones that match what you are building (for example, Master Arm Panel and Master Caution Light).

**Step 4.6** -- The tool auto-generates all mapping files

After panel selection, the Label Creator generates everything:
- `InputMapping.h` -- all button, switch, encoder, and analog input definitions
- `LEDMapping.h` -- all LED output definitions
- `DisplayMapping.cpp/h` -- display definitions (if your panels have displays)
- `DCSBIOSBridgeData.h` -- DCS-BIOS protocol data
- `LabelSetConfig.h` -- label set configuration
- Supporting files for the firmware

**Step 4.7** -- Edit inputs and LEDs using the built-in editors

After generation, you can immediately edit your mappings:

- **Edit Inputs** -- Assign physical GPIO pins (or PCA9555/shift register sources) to each control
- **Edit LEDs** -- Assign physical GPIO pins (or WS2812/PCA9555) to each indicator light
- **Edit Displays** -- Configure segment displays and TFT gauges
- **Edit Segment Maps** -- Customize HT1622 segment LCD layouts

Everything is done through the tool's guided editors. You do not need to open any `.h` files in a text editor.

```
+-----------------------------------------------------------------------+
|  TIP -- Come Back Any Time                                            |
+-----------------------------------------------------------------------+
|  You can always re-run LabelCreator-START.py and choose               |
|  "Modify Existing Label Set" to change your mappings later.           |
|  The tool preserves your existing edits when you add or remove        |
|  panels.                                                              |
+-----------------------------------------------------------------------+
```

> **Checkpoint:** You should now have a label set folder under `src\LABELS\` with your generated mapping files. The Label Creator will confirm this on screen.

---

## 5. Compile and Upload

The Compiler Tool handles firmware compilation and upload. It uses the bundled `arduino-cli` (installed by the Setup Tool) so you do not need Arduino IDE.

**Step 5.1** -- Double-click `CockpitOS-START.py` in the CockpitOS folder

Or run from a command prompt:

```
python CockpitOS-START.py
```

You can also launch it directly from the Label Creator's menu.

**Step 5.2** -- Select your board type from the menu

The tool lists all supported ESP32 variants:

```
+-----------------------------------------------------------------------+
|  Board Selection                                                      |
+-----------------------------------------------------------------------+
|  [1] ESP32-S3 Dev Module                                              |
|  [2] ESP32-S2 Dev Module                                              |
|  [3] ESP32 Dev Module (Classic)                                       |
|  [4] ESP32-P4 Dev Module                                              |
|  [5] ESP32-C3 Dev Module                                              |
|  [6] ESP32-C6 Dev Module                                              |
|  ...                                                                  |
+-----------------------------------------------------------------------+
```

**Step 5.3** -- Select your COM port

The tool scans for connected ESP32 boards and shows the available ports. If your board does not appear, check your USB cable and try putting the board in bootloader mode (hold BOOT, plug in USB, release BOOT).

**Step 5.4** -- Compile

Select the Compile option. The first compilation takes **3 to 10 minutes** depending on your computer -- this is normal. The entire ESP32 framework must be built from scratch on the first run. Subsequent compilations are much faster because of caching.

**Step 5.5** -- Upload

After a successful compile, select Upload. The tool transfers the firmware to your ESP32. This typically takes 30-60 seconds.

```
+-----------------------------------------------------------------------+
|  TIP -- Bootloader Mode                                               |
+-----------------------------------------------------------------------+
|  If upload fails, your board may need to be in bootloader mode:       |
|                                                                       |
|  1. Disconnect your ESP32 from USB                                    |
|  2. Press and HOLD the BOOT button (sometimes labeled "0")            |
|  3. While HOLDING the button, connect the USB cable                   |
|  4. Wait for the "USB connected" sound on your PC                     |
|  5. Release the BOOT button                                           |
|  6. Try the upload again                                              |
|                                                                       |
|  Alternative: If already connected, hold BOOT, press and release      |
|  RST, then release BOOT.                                              |
+-----------------------------------------------------------------------+
```

```
+-----------------------------------------------------------------------+
|  NOTE -- The Compiler Tool Handles All Settings                       |
+-----------------------------------------------------------------------+
|  You do NOT need to configure any Arduino IDE board settings           |
|  manually. The Compiler Tool automatically sets:                      |
|                                                                       |
|  - USB CDC On Boot: Disabled                                          |
|  - USB Mode: USB-OTG (TinyUSB) for S3/P4                             |
|  - Partition scheme, flash size, and all other board-specific flags   |
|                                                                       |
|  This eliminates the most common source of compilation errors.        |
+-----------------------------------------------------------------------+
```

> **Checkpoint:** After a successful upload, your ESP32 will automatically restart and begin running CockpitOS. For USB-native boards (S2, S3, P4), the device should appear as a USB HID device in Device Manager.

---

## 6. Configure Transport Mode

The **transport mode** determines how your ESP32 panel communicates with DCS World on your PC. You configure this in `Config.h` (located in the CockpitOS root folder).

Only **one** transport can be active at a time.

### Option A: USB HID (Recommended)

Best for: ESP32-S2, ESP32-S3, ESP32-P4

This is the **recommended** transport for production panels. It is the fastest and most reliable connection method. Your ESP32 connects directly to your PC via USB.

In `Config.h`, set:

```cpp
#define USE_DCSBIOS_USB     1
#define USE_DCSBIOS_WIFI    0
#define USE_DCSBIOS_SERIAL  0
```

You also need to run the **HID Manager** on your PC. The HID Manager bridges USB HID data between your ESP32 and DCS-BIOS. It is included with CockpitOS in the `HID Manager` folder.

```
+-----------------------------------------------------------------------+
|  TIP -- HID Manager                                                   |
+-----------------------------------------------------------------------+
|  The HID Manager auto-detects all CockpitOS USB devices and handles   |
|  the DCS-BIOS bridging. Just run it and leave it open while flying.   |
|                                                                       |
|  Start it with:  python "HID Manager\HID_Manager.py"                 |
+-----------------------------------------------------------------------+
```

### Option B: WiFi

Best for: Any ESP32 with WiFi (all variants except H2 and some P4)

WiFi requires no bridge software on your PC. Your ESP32 talks directly to DCS-BIOS over your local network. This is the easiest transport to get started with, but has slightly higher latency than USB.

In `Config.h`, set:

```cpp
#define USE_DCSBIOS_WIFI    1
#define USE_DCSBIOS_USB     0
#define USE_DCSBIOS_SERIAL  0
```

You also need to set your WiFi credentials. CockpitOS looks for a credentials file first, and falls back to the values in `Config.h`:

```cpp
#define WIFI_SSID   "YourNetworkName"
#define WIFI_PASS   "YourPassword"
```

```
+-----------------------------------------------------------------------+
|  WARNING -- WiFi Requirements                                         |
+-----------------------------------------------------------------------+
|  - Your ESP32 and your PC MUST be on the same network                 |
|  - ESP32 only connects to 2.4 GHz networks (not 5 GHz)               |
|  - Your router must support WPA2-PSK (AES/CCMP)                      |
+-----------------------------------------------------------------------+
```

### Option C: Serial (Legacy)

Best for: Any ESP32 (all variants)

Serial transport requires a `socat` bridge on your PC to relay data between the serial port and DCS-BIOS UDP.

In `Config.h`, set:

```cpp
#define USE_DCSBIOS_SERIAL  1
#define USE_DCSBIOS_USB     0
#define USE_DCSBIOS_WIFI    0
```

After changing `Config.h`, you must recompile and re-upload the firmware using the Compiler Tool.

---

## 7. Test in DCS

Once your ESP32 is flashed and your transport is configured, it is time to fly.

**Step 7.1** -- If using USB HID, start the HID Manager on your PC

**Step 7.2** -- Start DCS World

**Step 7.3** -- Load a mission with the aircraft that matches your label set

**Step 7.4** -- Your panel should come alive:
- LEDs respond to cockpit state (warning lights, indicators)
- Buttons and switches control DCS cockpit functions
- Displays show cockpit readouts (if configured)

```
+-----------------------------------------------------------------------+
|  TIP -- Debugging                                                     |
+-----------------------------------------------------------------------+
|  If things are not working, try these quick checks:                   |
|                                                                       |
|  - USB HID: Is the HID Manager running? Does it show your device?    |
|  - WiFi: Is the ESP32 connected? Run the DEBUG_UDP_console.py tool   |
|    in "Debug Tools" to see WiFi status and DCS-BIOS data flow.       |
|  - DCS-BIOS: Is it installed and running inside DCS? Check the       |
|    DCS-BIOS control reference page in your browser.                  |
|  - Wiring: Double-check your GPIO pin assignments match the          |
|    physical wiring on your board.                                    |
+-----------------------------------------------------------------------+
```

> **Checkpoint:** If you can press a button on your panel and see it affect the DCS cockpit (or trigger a cockpit warning and see your LED light up), congratulations -- you have a working CockpitOS panel!

---

## 8. What's Next?

You have CockpitOS running. Here is where to go from here:

| I want to... | Go to |
|---|---|
| **Build a complete panel in 30 minutes** | [Quick Start Guide](Quick-Start.md) |
| **Learn how the three Python tools work** | [Tools Overview](../Tools/README.md) |
| **Wire specific hardware (buttons, LEDs, encoders...)** | [Hardware Guides](../Hardware/README.md) |
| **Follow a step-by-step how-to guide** | [How-To Guides](../How-To/README.md) |
| **Look up a Config.h setting** | [Config Reference](../Reference/Config.md) |
| **Understand transport modes in depth** | [Transport Modes](../Reference/Transport-Modes.md) |
| **Troubleshoot a problem** | [FAQ & Troubleshooting](../Reference/FAQ.md) |

---

```
+-----------------------------------------------------------------------+
|                                                                       |
|  Need help?                                                           |
|                                                                       |
|  GitHub Discussions: github.com/BojoteX/CockpitOS/discussions         |
|  GitHub Issues:      github.com/BojoteX/CockpitOS/issues             |
|                                                                       |
+-----------------------------------------------------------------------+
```

---

*CockpitOS Getting Started Guide | Last updated: February 2026*
*Tested with: ESP32 Arduino Core 3.3.6, LovyanGFX 1.2.19*
