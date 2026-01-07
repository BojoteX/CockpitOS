# CockpitOS — Getting Started Guide

> **Your first flight starts here.**  
> This guide walks you through setting up your computer, installing CockpitOS firmware, and uploading it to your ESP32 device — step by step, with nothing assumed.

---

## Table of Contents

1. [What You'll Need](#1-what-youll-need)
2. [Install Arduino IDE](#2-install-arduino-ide)
3. [Install the ESP32 Board Package](#3-install-the-esp32-board-package)
4. [Install Required Libraries](#4-install-required-libraries)
5. [Download CockpitOS](#5-download-cockpitos)
6. [Configure Arduino IDE for Your Board](#6-configure-arduino-ide-for-your-board)
7. [Compile the Firmware](#7-compile-the-firmware)
8. [Upload to Your Device](#8-upload-to-your-device)
9. [Verify It Worked](#9-verify-it-worked)
10. [Troubleshooting](#10-troubleshooting)
11. [What's Next?](#11-whats-next)

---

## 1. What You'll Need

Before you begin, make sure you have:

```
┌─────────────────────────────────────────────────────────────────────┐
│  CHECKLIST                                                          │
├─────────────────────────────────────────────────────────────────────┤
│  [ ] An ESP32 board (S2, S3, or Classic recommended)                │
│  [ ] A USB cable that supports DATA transfer (not charge-only)      │
│  [ ] A computer running Windows, macOS, or Linux                    │
│  [ ] An internet connection                                         │
│  [ ] About 30 minutes of your time                                  │
└─────────────────────────────────────────────────────────────────────┘
```

> **⚠️ Common Pitfall — USB Cables**  
> Many USB cables are "charge-only" and cannot transfer data. If your computer doesn't detect your board, try a different cable before troubleshooting further. When in doubt, use the cable that came with your board or a known data-capable cable.

---

## 2. Install Arduino IDE

Arduino IDE is the software you'll use to edit, compile, and upload firmware to your ESP32.

**Step 2.1** — Go to [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

**Step 2.2** — Download Arduino IDE 2.x for your operating system

**Step 2.3** — Run the installer and follow the on-screen prompts

**Step 2.4** — Launch Arduino IDE once installation completes

```
┌─────────────────────────────────────────────────────────────────────┐
│  VERSION REFERENCE                                                  │
├─────────────────────────────────────────────────────────────────────┤
│  This guide was written using Arduino IDE 2.3.7                     │
│  Always try the latest version first — only use 2.3.7 if you        │
│  encounter issues with newer versions.                              │
└─────────────────────────────────────────────────────────────────────┘
```

> **✓ Checkpoint**  
> Arduino IDE should now be open and ready. You'll see a mostly empty editor window with a default sketch.

---

## 3. Install the ESP32 Board Package

Arduino IDE doesn't know how to talk to ESP32 devices out of the box. You need to install the ESP32 board definitions.

**Step 3.1** — In Arduino IDE, go to the menu: `Tools` → `Board` → `Boards Manager...`

```
    ┌──────────────────────────────────────┐
    │  Tools                               │
    ├──────────────────────────────────────┤
    │  Board: "..."              ►         │
    │    ┌─────────────────────────────┐   │
    │    │  Boards Manager...         │ ◄── Click here
    │    │  ─────────────────────────  │   │
    │    │  Arduino AVR Boards        │   │
    │    │  ...                       │   │
    │    └─────────────────────────────┘   │
    └──────────────────────────────────────┘
```

**Step 3.2** — In the search box, type: `esp32`

**Step 3.3** — Find the entry that says **"esp32 by Espressif Systems"**

**Step 3.4** — Click **INSTALL** and wait for the download to complete

```
┌─────────────────────────────────────────────────────────────────────┐
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ 🔍  esp32                                                     │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │  esp32                                                        │  │
│  │  by Espressif Systems                                         │  │
│  │                                                               │  │
│  │  ESP32 Arduino Core. Supports ESP32, ESP32-S2, ESP32-S3,     │  │
│  │  ESP32-C3, ESP32-C6, ESP32-H2, and ESP32-P4...               │  │
│  │                                               ┌─────────────┐ │  │
│  │                                               │   INSTALL   │ │  │
│  │                                               └─────────────┘ │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

This download is large (several hundred MB) and may take 5–15 minutes depending on your internet speed. Be patient.

```
┌─────────────────────────────────────────────────────────────────────┐
│  VERSION REFERENCE                                                  │
├─────────────────────────────────────────────────────────────────────┤
│  This guide was written using ESP32 Arduino Core 3.3.5              │
│  Minimum tested version: 3.2.x                                      │
│  Always try the latest version first.                               │
└─────────────────────────────────────────────────────────────────────┘
```

> **✓ Checkpoint**  
> Once installation completes, close the Boards Manager. You now have ESP32 support installed.

---

## 4. Install Required Libraries

CockpitOS requires one external library for TFT display support.

**Step 4.1** — Go to the menu: `Tools` → `Manage Libraries...`

**Step 4.2** — In the search box, type: `LovyanGFX`

**Step 4.3** — Find **"LovyanGFX by lovyan03"**

**Step 4.4** — Click **INSTALL**

```
┌─────────────────────────────────────────────────────────────────────┐
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │ 🔍  LovyanGFX                                                 │  │
│  └───────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌───────────────────────────────────────────────────────────────┐  │
│  │  LovyanGFX                                                    │  │
│  │  by lovyan03                                                  │  │
│  │                                                               │  │
│  │  Library for LCD graphics driver with various display         │  │
│  │  controllers...                                               │  │
│  │                                               ┌─────────────┐ │  │
│  │                                               │   INSTALL   │ │  │
│  │                                               └─────────────┘ │  │
│  └───────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

```
┌─────────────────────────────────────────────────────────────────────┐
│  VERSION REFERENCE                                                  │
├─────────────────────────────────────────────────────────────────────┤
│  This guide was written using LovyanGFX 1.2.7                       │
│  Always try the latest version first.                               │
└─────────────────────────────────────────────────────────────────────┘
```

> **✓ Checkpoint**  
> Close the Library Manager. Your Arduino IDE environment is now fully configured.  
> **Close Arduino IDE completely before proceeding to the next step.**

---

## 5. Download CockpitOS

Now you'll download the CockpitOS firmware and place it in the correct location.

**Step 5.1** — Go to [https://github.com/BojoteX/CockpitOS/releases](https://github.com/BojoteX/CockpitOS/releases)

**Step 5.2** — Download the latest release ZIP file (e.g., `CockpitOS-vX.X.X.zip`)

**Step 5.3** — Extract the ZIP file

**Step 5.4** — Move the extracted `CockpitOS` folder to your Arduino directory

The final folder structure **must** look exactly like this:

```
    Windows:
    ────────────────────────────────────────────────────────────
    C:\Users\<YourName>\Documents\Arduino\
    └── CockpitOS\
        ├── CockpitOS.ino        ◄── This file MUST exist here
        ├── Config.h
        ├── src\
        ├── LABELS\
        └── ...
    
    macOS:
    ────────────────────────────────────────────────────────────
    /Users/<YourName>/Documents/Arduino/
    └── CockpitOS/
        ├── CockpitOS.ino        ◄── This file MUST exist here
        ├── Config.h
        ├── src/
        ├── LABELS/
        └── ...
    
    Linux:
    ────────────────────────────────────────────────────────────
    /home/<YourName>/Arduino/
    └── CockpitOS/
        ├── CockpitOS.ino        ◄── This file MUST exist here
        ├── Config.h
        ├── src/
        ├── LABELS/
        └── ...
```

> **⚠️ Critical — Folder Structure**  
> The folder containing `CockpitOS.ino` **must** be named exactly `CockpitOS`.  
> Arduino requires the folder name to match the `.ino` filename.  
> 
> **Wrong:** `Arduino/CockpitOS-main/CockpitOS.ino`  
> **Wrong:** `Arduino/CockpitOS-v2.0/CockpitOS.ino`  
> **Correct:** `Arduino/CockpitOS/CockpitOS.ino`

**Step 5.5** — Double-click `CockpitOS.ino` to open it in Arduino IDE

> **✓ Checkpoint**  
> Arduino IDE should open with multiple tabs visible: `CockpitOS.ino`, `Config.h`, and others. If you see an error about the folder name, revisit Step 5.4.

---

## 6. Configure Arduino IDE for Your Board

This is the most critical step. Incorrect settings here will cause compilation or upload failures.

### 6.1 — Select Your Board

**Step 6.1.1** — Go to: `Tools` → `Board` → `esp32` → *[Select your board]*

Use this table to find your board:

```
┌─────────────────────────────────────────────────────────────────────┐
│  YOUR DEVICE              SELECT THIS BOARD                         │
├─────────────────────────────────────────────────────────────────────┤
│  ESP32 (Classic/Original) │  "ESP32 Dev Module"                     │
│  ESP32-S2                 │  "ESP32S2 Dev Module" or your specific  │
│                           │  board (e.g., "LOLIN S2 Mini")          │
│  ESP32-S3                 │  "ESP32S3 Dev Module" or your specific  │
│                           │  board (e.g., "LOLIN S3 Mini")          │
│  ESP32-C3                 │  "ESP32C3 Dev Module"                   │
│  ESP32-C6                 │  "ESP32C6 Dev Module"                   │
│  ESP32-H2                 │  "ESP32H2 Dev Module"                   │
│  ESP32-P4                 │  "ESP32P4 Dev Module"                   │
└─────────────────────────────────────────────────────────────────────┘
```

> **💡 Tip — Which ESP32 Do I Have?**  
> Look at the main chip on your board. It's usually printed directly on the metal shield:
> - `ESP32-WROOM` or `ESP32-WROVER` → ESP32 Classic
> - `ESP32-S2` anywhere in the name → ESP32-S2
> - `ESP32-S3` anywhere in the name → ESP32-S3

### 6.2 — Configure Critical Settings

After selecting your board, you **must** configure these settings in the `Tools` menu.

```
╔═════════════════════════════════════════════════════════════════════╗
║  CRITICAL SETTINGS — FAILURE TO SET THESE WILL CAUSE ERRORS        ║
╠═════════════════════════════════════════════════════════════════════╣
║                                                                     ║
║  USB CDC On Boot:     "Disabled"          ◄── ALL BOARDS            ║
║                                                                     ║
║  USB Mode:            "USB-OTG (TinyUSB)" ◄── S3 and P4 ONLY        ║
║                       (S2 devices do not show this option —         ║
║                        USB-OTG is their only mode)                  ║
║                                                                     ║
╚═════════════════════════════════════════════════════════════════════╝
```

Here's what each board type requires:

```
    ESP32 Classic
    ────────────────────────────────────────────────────────────
    USB CDC On Boot:    "Disabled"
    (No USB Mode option — Classic uses UART bridge, not native USB)
    
    
    ESP32-S2
    ────────────────────────────────────────────────────────────
    USB CDC On Boot:    "Disabled"
    (No USB Mode option — S2 only supports USB-OTG mode)
    
    
    ESP32-S3
    ────────────────────────────────────────────────────────────
    USB CDC On Boot:    "Disabled"
    USB Mode:           "USB-OTG (TinyUSB)"    ◄── IMPORTANT!
    
    
    ESP32-P4
    ────────────────────────────────────────────────────────────
    USB CDC On Boot:    "Disabled"
    USB Mode:           "USB-OTG (TinyUSB)"    ◄── IMPORTANT!
    
    
    ESP32-C3 / C6 / H2
    ────────────────────────────────────────────────────────────
    USB CDC On Boot:    "Disabled"
    (These boards use USB-Serial bridge; native USB HID not supported)
```

> **⚠️ Warning — "USB CDC On Boot"**  
> If you forget to set "USB CDC On Boot" to "Disabled", compilation will fail with this error:
> ```
> ❌ Invalid config: Set 'USB CDC On Boot' to 'Disabled' (see Tools menu in Arduino IDE).
> ```
> This is intentional — CockpitOS cannot function with CDC enabled at boot.

> **✓ Checkpoint**  
> Your Tools menu should now show the correct board and settings. Double-check before proceeding.

---

## 7. Compile the Firmware

Before uploading to your device, you'll compile the firmware to check for errors.

**Step 7.1** — Go to: `Sketch` → `Verify/Compile`

Or use the keyboard shortcut:
- **Windows/Linux:** `Ctrl + R`
- **macOS:** `Cmd + R`

```
    ┌────────────────────────────────────────────────────────────┐
    │                                                            │
    │    ┌──────┐  ┌──────┐                                      │
    │    │  ✓   │  │  →   │                                      │
    │    │Verify│  │Upload│                                      │
    │    └──────┘  └──────┘                                      │
    │       ▲                                                    │
    │       │                                                    │
    │       └─── Click this (checkmark) to compile               │
    │                                                            │
    └────────────────────────────────────────────────────────────┘
```

**What to Expect:**

The first compilation takes a long time — typically **3 to 10 minutes** depending on your computer's speed. This is normal. Subsequent compilations are much faster due to caching.

You'll see progress messages scrolling in the output panel at the bottom of the window.

**Successful Compilation:**

When compilation succeeds, you'll see output similar to:

```
Sketch uses 843297 bytes (26%) of program storage space. Maximum is 3145728 bytes.
Global variables use 54032 bytes (16%) of dynamic memory...
```

**Failed Compilation:**

If compilation fails, scroll up in the output panel to find the error message. Common errors and solutions are covered in the [Troubleshooting](#10-troubleshooting) section.

> **✓ Checkpoint**  
> You should see "Done compiling" or storage usage statistics. If you see red error text, do not proceed — fix the errors first.

---

## 8. Upload to Your Device

Now you'll transfer the compiled firmware to your ESP32.

### 8.1 — Enter Bootloader Mode

Before your device can receive new firmware, it must be in **bootloader mode** (also called "download mode" or "flash mode").

```
╔═════════════════════════════════════════════════════════════════════╗
║  HOW TO ENTER BOOTLOADER MODE                                       ║
╠═════════════════════════════════════════════════════════════════════╣
║                                                                     ║
║  1. Disconnect your ESP32 from USB (if connected)                   ║
║                                                                     ║
║  2. Press and HOLD the "BOOT" button (sometimes labeled "0")        ║
║                                                                     ║
║  3. While HOLDING the button, connect the USB cable                 ║
║                                                                     ║
║  4. Wait for your computer to make the "USB connected" sound        ║
║                                                                     ║
║  5. Release the BOOT button                                         ║
║                                                                     ║
╚═════════════════════════════════════════════════════════════════════╝

    ┌─────────────────────────────────────────────────────────────┐
    │                                                             │
    │     Most ESP32 boards have two buttons:                     │
    │                                                             │
    │         ┌──────┐          ┌──────┐                          │
    │         │ RST  │          │ BOOT │  ◄── Hold this one       │
    │         │      │          │  (0) │                          │
    │         └──────┘          └──────┘                          │
    │                                                             │
    │     RST = Reset (restarts the device)                       │
    │     BOOT = Bootloader (enables firmware upload)             │
    │                                                             │
    └─────────────────────────────────────────────────────────────┘
```

> **💡 Tip — Alternative Method**  
> If your board is already connected:
> 1. Hold the BOOT button
> 2. Press and release the RST button (while still holding BOOT)
> 3. Release the BOOT button
> 
> This resets the device directly into bootloader mode without unplugging.

### 8.2 — Select the COM Port

**Step 8.2.1** — Go to: `Tools` → `Port`

**Step 8.2.2** — Select the COM port that appeared when you connected your device

```
    ┌──────────────────────────────────────┐
    │  Tools                               │
    ├──────────────────────────────────────┤
    │  Port: "..."                ►        │
    │    ┌─────────────────────────────┐   │
    │    │  COM3                      │   │
    │    │  COM5   ◄── Your ESP32     │   │
    │    │  COM7                      │   │
    │    └─────────────────────────────┘   │
    └──────────────────────────────────────┘
    
    macOS/Linux ports appear as:
    - /dev/ttyUSB0, /dev/ttyUSB1, etc.
    - /dev/ttyACM0, /dev/ttyACM1, etc.
    - /dev/cu.usbserial-XXXX
```

> **💡 Tip — Identifying the Correct Port**  
> If you see multiple ports and aren't sure which one is your ESP32:
> 1. Note which ports are listed
> 2. Disconnect your ESP32
> 3. Check the list again — the one that disappeared is your device
> 4. Reconnect (in bootloader mode) and select that port

### 8.3 — Upload the Firmware

**Step 8.3.1** — Go to: `Sketch` → `Upload`

Or use the keyboard shortcut:
- **Windows/Linux:** `Ctrl + U`
- **macOS:** `Cmd + U`

```
    ┌────────────────────────────────────────────────────────────┐
    │                                                            │
    │    ┌──────┐  ┌──────┐                                      │
    │    │  ✓   │  │  →   │                                      │
    │    │Verify│  │Upload│ ◄── Click this (arrow) to upload     │
    │    └──────┘  └──────┘                                      │
    │                                                            │
    └────────────────────────────────────────────────────────────┘
```

**What to Expect:**

1. Arduino IDE will first compile (if anything changed)
2. It will then connect to your device
3. You'll see progress percentages as the firmware uploads
4. Upload typically takes 30–60 seconds

**Successful Upload:**

```
Writing at 0x00010000... (100%)
Wrote 843297 bytes in 12.3 seconds (548.2 kbit/s)...
Hash of data verified.

Leaving...
Hard resetting via RTS pin...
```

Or simply: **"Done uploading."**

> **✓ Checkpoint**  
> Your ESP32 now has CockpitOS installed! The device will automatically restart and begin running the firmware.

---

## 9. Verify It Worked

After upload completes, your ESP32 will automatically restart and run CockpitOS.

**For USB-native boards (S2, S3):**

Your device should appear as a USB HID device (like a gamepad/joystick). You can verify this in:
- **Windows:** Device Manager → Human Interface Devices
- **macOS:** System Information → USB
- **Linux:** `lsusb` command

**For all boards:**

If you have LEDs configured in your label set, they may blink or light up to indicate the device is running.

> **✓ Checkpoint**  
> If your device appears as a USB device or shows signs of life (LEDs, etc.), congratulations — you've successfully installed CockpitOS!

---

## 10. Troubleshooting

### Compilation Errors

```
┌─────────────────────────────────────────────────────────────────────┐
│  ERROR                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  ❌ Invalid config: Set 'USB CDC On Boot' to 'Disabled'             │
├─────────────────────────────────────────────────────────────────────┤
│  CAUSE                                                              │
│  "USB CDC On Boot" is set to "Enabled" in the Tools menu.           │
├─────────────────────────────────────────────────────────────────────┤
│  SOLUTION                                                           │
│  Go to Tools → USB CDC On Boot → Select "Disabled"                  │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  ERROR                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  'LovyanGFX.h' file not found                                       │
│  OR                                                                 │
│  fatal error: LovyanGFX.h: No such file or directory                │
├─────────────────────────────────────────────────────────────────────┤
│  CAUSE                                                              │
│  The LovyanGFX library is not installed.                            │
├─────────────────────────────────────────────────────────────────────┤
│  SOLUTION                                                           │
│  Go to Tools → Manage Libraries → Search "LovyanGFX" → Install      │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  ERROR                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  Board "esp32" is unknown                                           │
│  OR                                                                 │
│  Compilation error: Missing FQBN (fully qualified board name)       │
├─────────────────────────────────────────────────────────────────────┤
│  CAUSE                                                              │
│  No board selected, or ESP32 board package not installed.           │
├─────────────────────────────────────────────────────────────────────┤
│  SOLUTION                                                           │
│  1. Go to Tools → Board → Boards Manager                            │
│  2. Search "esp32" and install "esp32 by Espressif Systems"         │
│  3. Go to Tools → Board → esp32 → Select your board                 │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  ERROR                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  sketch/CockpitOS.ino.cpp: file not found                           │
│  OR                                                                 │
│  The sketch folder name does not match the sketch file name         │
├─────────────────────────────────────────────────────────────────────┤
│  CAUSE                                                              │
│  The folder containing CockpitOS.ino is not named "CockpitOS".      │
├─────────────────────────────────────────────────────────────────────┤
│  SOLUTION                                                           │
│  Rename the folder to exactly "CockpitOS" (case-sensitive).         │
│  Path should be: Arduino/CockpitOS/CockpitOS.ino                    │
└─────────────────────────────────────────────────────────────────────┘
```

---

### Upload Errors

```
┌─────────────────────────────────────────────────────────────────────┐
│  ERROR                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  A fatal error occurred: Failed to connect to ESP32: Timed out      │
│  waiting for packet header                                          │
│  OR                                                                 │
│  A fatal error occurred: Could not open port                        │
├─────────────────────────────────────────────────────────────────────┤
│  CAUSE                                                              │
│  Device is not in bootloader mode, or wrong port selected.          │
├─────────────────────────────────────────────────────────────────────┤
│  SOLUTION                                                           │
│  1. Disconnect the USB cable                                        │
│  2. Hold the BOOT button                                            │
│  3. Connect USB while holding BOOT                                  │
│  4. Release BOOT after hearing the connection sound                 │
│  5. Select the correct COM port in Tools → Port                     │
│  6. Try uploading again                                             │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  ERROR                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  No port selected                                                   │
│  OR                                                                 │
│  Serial port not selected                                           │
├─────────────────────────────────────────────────────────────────────┤
│  CAUSE                                                              │
│  No COM port is selected in the Tools menu.                         │
├─────────────────────────────────────────────────────────────────────┤
│  SOLUTION                                                           │
│  1. Connect your ESP32 in bootloader mode                           │
│  2. Go to Tools → Port                                              │
│  3. Select the COM port that appeared                               │
│                                                                     │
│  If no ports appear, check your USB cable (data vs charge-only)     │
│  or try a different USB port on your computer.                      │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  ERROR                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  Access denied / Permission denied on COM port                      │
├─────────────────────────────────────────────────────────────────────┤
│  CAUSE                                                              │
│  Another program is using the port, or you lack permissions.        │
├─────────────────────────────────────────────────────────────────────┤
│  SOLUTION                                                           │
│  Windows:                                                           │
│    - Close Serial Monitor if open                                   │
│    - Close other programs that might use COM ports                  │
│    - Try running Arduino IDE as Administrator                       │
│                                                                     │
│  Linux:                                                             │
│    - Add your user to the dialout group:                            │
│      sudo usermod -a -G dialout $USER                               │
│    - Log out and log back in for changes to take effect             │
│                                                                     │
│  macOS:                                                             │
│    - Close Serial Monitor if open                                   │
│    - Check System Preferences → Security & Privacy                  │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  ERROR                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  Wrong boot mode detected (DOWNLOAD instead of SPI_FLASH)           │
│  OR                                                                 │
│  Upload succeeded but device doesn't run properly                   │
├─────────────────────────────────────────────────────────────────────┤
│  CAUSE                                                              │
│  Device is still in bootloader mode after upload.                   │
├─────────────────────────────────────────────────────────────────────┤
│  SOLUTION                                                           │
│  Press the RST (Reset) button on your board to restart it.          │
│  If no RST button exists, disconnect and reconnect USB              │
│  WITHOUT holding the BOOT button.                                   │
└─────────────────────────────────────────────────────────────────────┘
```

---

### USB / Device Detection Issues

```
┌─────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  Device not appearing as USB HID after upload (S2/S3 boards)        │
├─────────────────────────────────────────────────────────────────────┤
│  POSSIBLE CAUSES & SOLUTIONS                                        │
│                                                                     │
│  1. Wrong USB Mode (S3/P4 only)                                     │
│     → Go to Tools → USB Mode → Select "USB-OTG (TinyUSB)"           │
│     → Recompile and re-upload                                       │
│                                                                     │
│  2. Device still in bootloader mode                                 │
│     → Press RST button or reconnect without holding BOOT            │
│                                                                     │
│  3. USB cable issue                                                 │
│     → Try a different USB cable (must be data-capable)              │
│                                                                     │
│  4. USB port issue                                                  │
│     → Try a different USB port on your computer                     │
│     → Avoid USB hubs; connect directly to the computer              │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                              │
├─────────────────────────────────────────────────────────────────────┤
│  No COM port appears even in bootloader mode                        │
├─────────────────────────────────────────────────────────────────────┤
│  POSSIBLE CAUSES & SOLUTIONS                                        │
│                                                                     │
│  1. Charge-only USB cable                                           │
│     → Use a known data-capable USB cable                            │
│                                                                     │
│  2. Missing USB drivers (Windows)                                   │
│     → The ESP32 board package includes drivers, but you may need    │
│       to install them manually:                                     │
│       - Check Device Manager for unknown devices                    │
│       - Download CP210x or CH340 drivers if needed                  │
│                                                                     │
│  3. Not actually in bootloader mode                                 │
│     → Make sure you're holding BOOT before/during connection        │
│     → Try the RST+BOOT method (hold BOOT, press RST, release RST,   │
│       release BOOT)                                                 │
│                                                                     │
│  4. Defective board or USB connector                                │
│     → Inspect the USB connector for damage                          │
│     → Try a different ESP32 board if available                      │
└─────────────────────────────────────────────────────────────────────┘
```

---

### Frequently Asked Questions

```
┌─────────────────────────────────────────────────────────────────────┐
│  Q: Why does the first compilation take so long?                    │
├─────────────────────────────────────────────────────────────────────┤
│  A: Arduino IDE compiles the entire ESP32 framework the first       │
│     time. This includes many libraries and core files. Subsequent   │
│     compilations only rebuild changed files and are much faster.    │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  Q: Do I need to enter bootloader mode every time I upload?         │
├─────────────────────────────────────────────────────────────────────┤
│  A: For most uploads with CockpitOS, yes. This is because           │
│     CockpitOS runs as a USB HID device, not a serial device.        │
│     The bootloader mode exposes a serial interface for programming. │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  Q: Can I use PlatformIO instead of Arduino IDE?                    │
├─────────────────────────────────────────────────────────────────────┤
│  A: CockpitOS is developed and tested exclusively with Arduino IDE. │
│     While PlatformIO might work, it's not officially supported and  │
│     you may encounter issues.                                       │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  Q: Which ESP32 board should I buy?                                 │
├─────────────────────────────────────────────────────────────────────┤
│  A: For cockpit panels with USB HID support, we recommend:          │
│     - ESP32-S2 (e.g., LOLIN S2 Mini) — excellent, compact           │
│     - ESP32-S3 (e.g., LOLIN S3 Mini) — more powerful, USB native    │
│                                                                     │
│     ESP32 Classic works but requires an external USB-UART bridge    │
│     and doesn't support native USB HID.                             │
└─────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────┐
│  Q: I uploaded successfully but nothing seems to happen?            │
├─────────────────────────────────────────────────────────────────────┤
│  A: The default CockpitOS release is configured for minimal         │
│     operation. To see activity:                                     │
│     1. Press RST to ensure you're out of bootloader mode            │
│     2. Check if it appears as a USB device on your computer         │
│     3. The real action happens when you configure label sets and    │
│        wire actual buttons/LEDs — see the documentation for next    │
│        steps                                                        │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 11. What's Next?

Congratulations! You've successfully set up your development environment and uploaded CockpitOS to your ESP32.

To start building your cockpit panel, you'll need to:

1. **Choose a Label Set** — Select an aircraft configuration from the `LABELS/` directory
2. **Configure Your Hardware** — Edit `InputMapping.h` and `LEDMapping.h` for your specific buttons, switches, and LEDs
3. **Generate Data Files** — Run the Python generator script
4. **Wire Your Panel** — Connect physical components to your ESP32's GPIO pins
5. **Run the HID Manager** — Use the Python HID Manager on your PC to bridge DCS-BIOS data

See the `README.md` files in the CockpitOS repository for detailed instructions on each of these steps.

---

```
┌─────────────────────────────────────────────────────────────────────┐
│                                                                     │
│    Need help?                                                       │
│                                                                     │
│    • GitHub Issues: github.com/BojoteX/CockpitOS/issues             │
│    • Check the README.md in each directory for specific guidance    │
│    • A custom-trained GPT assistant is available — see the          │
│      GitHub page for the link                                       │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

*Guide version: 1.0 | Last updated: January 2025*  
*Tested with: Arduino IDE 2.3.7, ESP32 Arduino Core 3.3.5, LovyanGFX 1.2.7*
