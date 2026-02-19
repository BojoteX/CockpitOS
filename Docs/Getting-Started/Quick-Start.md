# CockpitOS -- Your First Panel in 30 Minutes

Build a working **F/A-18C Hornet** test panel on an ESP32, connected to DCS World. This guide uses the automated Python tools -- no manual file editing required.

> **What changed?** If you followed the old version of this guide, the big difference is that you no longer need to manually edit `InputMapping.h` or `LEDMapping.h` in a text editor. The Label Creator tool handles everything through guided menus.

---

## What You'll Build

A simple test panel with four components. You do not need all of them -- even one button and one LED is enough to follow along.

```
+-----------------------------------------------------------------------+
|  YOUR TEST PANEL                                                      |
+-----------------------------------------------------------------------+
|                                                                       |
|  Component          DCS Function              GPIO Pin                |
|  -----------------------------------------------------------------    |
|  Push Button        Master Caution Reset      GPIO 5                  |
|  Toggle Switch      Master Arm (ARM / SAFE)   GPIO 6                  |
|  Potentiometer      RWR Volume                GPIO 4                  |
|  LED                Master Caution Light       GPIO 7                  |
|                                                                       |
+-----------------------------------------------------------------------+
```

When you are done, pressing your physical button will reset the Master Caution in DCS, flipping your toggle will arm/safe the Master Arm switch, turning your pot will adjust RWR volume, and the LED will light up whenever the Master Caution is active in the cockpit.

---

## Prerequisites

Before starting this tutorial, make sure you have:

- [ ] Run the **Setup Tool** (`Setup-START.py`) at least once -- see [Getting Started](README.md)
- [ ] **DCS World** installed with **DCS-BIOS** -- see [DCS-BIOS releases](https://github.com/DCS-Skunkworks/dcs-bios/releases)
- [ ] An **ESP32 board** connected to your PC via a USB data cable
- [ ] **Python 3.12+** installed
- [ ] Basic components for wiring (button, toggle switch, potentiometer, LED, 330 ohm resistor, jumper wires)

---

## Step 1: Create the Label Set

Open the Label Creator tool. Double-click `LabelCreator-START.py` in the CockpitOS folder, or run:

```
python LabelCreator-START.py
```

### 1.1 -- Create a New Label Set

From the main menu, select **"Create New Label Set"**.

### 1.2 -- Select Your Aircraft

Choose **FA-18C_hornet** from the aircraft list.

### 1.3 -- Name Your Label Set

Enter a name. For this tutorial, use:

```
MY_FIRST_PANEL
```

### 1.4 -- Select Panels

The tool shows you all available F/A-18C cockpit panels. Select these two:

- **Master Arm Panel** (contains the Master Arm switch)
- **Master Caution Light** (contains the Master Caution Reset button and the Master Caution indicator light)

```
+-----------------------------------------------------------------------+
|  TIP -- Panel Selection                                               |
+-----------------------------------------------------------------------+
|  You only need to include the panels that contain the controls you    |
|  plan to wire. Selecting fewer panels means less clutter in your      |
|  mapping files and faster compile times.                              |
+-----------------------------------------------------------------------+
```

### 1.5 -- Auto-Generate

After you confirm your panel selection, the Label Creator automatically generates all mapping files:

- `InputMapping.h`
- `LEDMapping.h`
- `DisplayMapping.cpp` / `DisplayMapping.h`
- `DCSBIOSBridgeData.h`
- `LabelSetConfig.h`
- Supporting files

Your new label set is created at:

```
CockpitOS\src\LABELS\MY_FIRST_PANEL\
```

---

## Step 2: Wire Your Hardware

Here is the wiring diagram for the four test components. Use **3.3V only** -- never connect 5V to ESP32 GPIO pins.

```
ESP32 Board
+------------------+
|                  |
|  3.3V  -------+-------- Pot Pin 1 (left outer pin)
|                |
|  GND   -------+-------- Pot Pin 3 (right outer pin)
|                |-------- Button (one leg)
|                |-------- Toggle Switch (one terminal)
|                +-------- LED cathode (-) via ground rail
|                  |
|  GPIO 4 -------------- Pot Pin 2 (center wiper)
|                  |
|  GPIO 5 -------------- Button (other leg)
|                  |
|  GPIO 6 -------------- Toggle Switch (center terminal)
|                  |
|  GPIO 7 ----[330R]---- LED anode (+)
|                  |
+------------------+
```

### Wiring Details

**Push Button (Master Caution Reset) -- GPIO 5**
- One leg to GPIO 5
- Other leg to GND
- CockpitOS enables the internal pull-up resistor automatically. No external resistor needed.

**Toggle Switch (Master Arm) -- GPIO 6**
- Center terminal to GPIO 6
- One side terminal to GND
- The switch connects GPIO 6 to GND when flipped to ARM. When flipped to SAFE, the internal pull-up holds the pin HIGH.

**Potentiometer (RWR Volume) -- GPIO 4**
- Left outer pin to 3.3V
- Right outer pin to GND
- Center wiper pin to GPIO 4

**LED (Master Caution Light) -- GPIO 7**
- GPIO 7 to a 330 ohm resistor
- Resistor to LED anode (longer leg / +)
- LED cathode (shorter leg / -) to GND

```
+-----------------------------------------------------------------------+
|  WARNING -- Potentiometer Voltage                                     |
+-----------------------------------------------------------------------+
|  Connect the potentiometer outer pins to 3.3V and GND -- NOT 5V.     |
|  The ESP32 ADC expects a maximum of 3.3V. Feeding 5V into a GPIO     |
|  pin can permanently damage your board.                               |
+-----------------------------------------------------------------------+
```

```
+-----------------------------------------------------------------------+
|  TIP -- Wire Only What You Have                                       |
+-----------------------------------------------------------------------+
|  You do not need all four components. If you only have a button and   |
|  an LED, just wire those two and skip the rest. CockpitOS handles     |
|  missing hardware gracefully -- unmapped controls are simply ignored. |
+-----------------------------------------------------------------------+
```

---

## Step 3: Edit Inputs

Go back to the Label Creator. From the main menu, select **"Modify Existing Label Set"**, then select your `MY_FIRST_PANEL` label set.

### 3.1 -- Open the Input Editor

Select **"Edit Inputs"** from the label set menu.

The editor shows every available input control from your selected panels. Each row represents one DCS-BIOS control. You need to tell CockpitOS where each control is physically connected.

### 3.2 -- Map the Push Button

Find `MASTER_CAUTION_RESET_SW` in the list and set:

| Field | Value |
|---|---|
| Source | `GPIO` |
| Port | `5` |

This tells CockpitOS: "The Master Caution Reset button is connected to GPIO pin 5."

### 3.3 -- Map the Toggle Switch

Find `MASTER_ARM_SW_ARM` and `MASTER_ARM_SW_SAFE` in the list. These are part of a **selector group** -- they share the same physical switch on the same GPIO pin, but represent different positions.

For **MASTER_ARM_SW_ARM**:

| Field | Value |
|---|---|
| Source | `GPIO` |
| Port | `6` |

For **MASTER_ARM_SW_SAFE**:

| Field | Value |
|---|---|
| Source | `GPIO` |
| Port | `6` |

Both entries use the same GPIO pin (6). CockpitOS uses the selector group to determine which position the switch is in.

### 3.4 -- Map the Potentiometer

Find `COM_RWR` in the list and set:

| Field | Value |
|---|---|
| Source | `GPIO` |
| Port | `4` |

### 3.5 -- Save and Exit

The editor saves your changes automatically when you exit. Your `InputMapping.h` is updated without you ever opening a text editor.

```
+-----------------------------------------------------------------------+
|  TIP -- Leave Unused Controls as NONE                                 |
+-----------------------------------------------------------------------+
|  Any control you do not wire should have its Source left as "NONE".    |
|  CockpitOS ignores NONE entries at runtime, so there is no            |
|  performance cost for having them in the file.                        |
+-----------------------------------------------------------------------+
```

---

## Step 4: Edit LEDs

From the label set menu in the Label Creator, select **"Edit LEDs"**.

### 4.1 -- Map the Master Caution Light

Find `MASTER_CAUTION_LT` in the list and set:

| Field | Value |
|---|---|
| Device | `GPIO` |
| Port | `7` |

This tells CockpitOS: "Drive the Master Caution indicator LED on GPIO pin 7."

### 4.2 -- Save and Exit

The editor saves your changes when you exit, just like the input editor.

---

## Step 5: Configure Transport

For this tutorial, we will use **WiFi** because it is the simplest transport to get started with. You do not need any bridge software on your PC -- the ESP32 talks directly to DCS-BIOS over your network.

Open `Config.h` in the CockpitOS root folder (use any text editor) and set:

```cpp
#define USE_DCSBIOS_WIFI    1
#define USE_DCSBIOS_USB     0
#define USE_DCSBIOS_SERIAL  0
```

Then set your WiFi credentials:

```cpp
#define WIFI_SSID   "YourNetworkName"
#define WIFI_PASS   "YourPassword"
```

```
+-----------------------------------------------------------------------+
|  WARNING -- WiFi Requirements                                         |
+-----------------------------------------------------------------------+
|  - Your ESP32 and PC MUST be on the same network                      |
|  - ESP32 only connects to 2.4 GHz networks (not 5 GHz)               |
|  - Your router must support WPA2-PSK (AES/CCMP)                      |
|  - If you have a dual-band router, make sure the 2.4 GHz band        |
|    is enabled and your PC can reach it                                |
+-----------------------------------------------------------------------+
```

```
+-----------------------------------------------------------------------+
|  NOTE -- WiFi vs USB for Production                                   |
+-----------------------------------------------------------------------+
|  WiFi is great for getting started, but for production panels we      |
|  recommend USB HID transport (on S2, S3, or P4 boards). USB HID      |
|  is faster, more reliable, and does not depend on your network.       |
|                                                                       |
|  To switch to USB later, just change USE_DCSBIOS_USB to 1 and        |
|  USE_DCSBIOS_WIFI to 0 in Config.h, recompile, and run the           |
|  HID Manager on your PC.                                              |
+-----------------------------------------------------------------------+
```

---

## Step 6: Compile and Upload

Open the Compiler Tool. Double-click `CockpitOS-START.py`, or run:

```
python CockpitOS-START.py
```

You can also launch the Compiler Tool directly from the Label Creator's menu.

### 6.1 -- Select Your Board

Choose your ESP32 variant from the list. For example, **ESP32-S3 Dev Module**.

### 6.2 -- Select Your COM Port

The tool scans for connected boards and lists available ports. Select the one that matches your ESP32.

```
+-----------------------------------------------------------------------+
|  TIP -- Finding Your COM Port                                         |
+-----------------------------------------------------------------------+
|  If you are not sure which port is your ESP32:                        |
|  1. Note the ports listed                                             |
|  2. Disconnect your ESP32                                             |
|  3. Check the list again -- the one that disappeared is your board    |
|  4. Reconnect and select that port                                    |
+-----------------------------------------------------------------------+
```

### 6.3 -- Compile

Select the Compile option. The first build takes 3-10 minutes. Subsequent builds are much faster.

### 6.4 -- Upload

After compilation succeeds, select Upload. If the upload fails, put your board in bootloader mode:

1. Disconnect USB
2. Hold the **BOOT** button
3. Connect USB while holding BOOT
4. Release BOOT
5. Try uploading again

---

## Step 7: Test in DCS

**Step 7.1** -- Make sure DCS-BIOS is installed and running inside DCS World

**Step 7.2** -- Start DCS World and load a mission with the F/A-18C Hornet

**Step 7.3** -- Your panel should respond:

- **Press the button** -- Master Caution resets in the cockpit
- **Flip the toggle** -- Master Arm switch moves between ARM and SAFE
- **Turn the potentiometer** -- RWR volume changes
- **Trigger Master Caution in DCS** (for example, by pulling the throttle to idle) -- your LED lights up

### WiFi Debugging

If things are not working, run the WiFi debug console to see what your ESP32 is doing:

```
python "Debug Tools\DEBUG_UDP_console.py"
```

This shows WiFi connection status, incoming DCS-BIOS data, and outgoing commands from your panel.

You can also enable verbose WiFi logging in `Config.h`:

```cpp
#define VERBOSE_MODE_WIFI_ONLY    1
```

Recompile and re-upload after making this change.

---

## Quick Checklist

Run through this list to make sure you have not missed anything:

```
+-----------------------------------------------------------------------+
|  FINAL CHECKLIST                                                      |
+-----------------------------------------------------------------------+
|  [ ] Setup Tool ran successfully (ESP32 Core + LovyanGFX installed)   |
|  [ ] DCS-BIOS installed in DCS World                                  |
|  [ ] Label Set created via Label Creator (MY_FIRST_PANEL)             |
|  [ ] Inputs mapped in Label Creator (GPIO pins assigned)              |
|  [ ] LEDs mapped in Label Creator (GPIO pin assigned)                 |
|  [ ] Hardware wired (button, toggle, pot, LED)                        |
|  [ ] Config.h: WiFi enabled, credentials set                         |
|  [ ] Firmware compiled and uploaded via Compiler Tool                 |
|  [ ] DCS running with F/A-18C mission                                 |
|  [ ] Panel responds to cockpit state and sends commands               |
+-----------------------------------------------------------------------+
```

---

## Common Problems

```
+-----------------------------------------------------------------------+
|  PROBLEM: WiFi will not connect                                       |
+-----------------------------------------------------------------------+
|  - Double-check SSID and password (they are case-sensitive)           |
|  - Make sure your network is 2.4 GHz, not 5 GHz only                 |
|  - Verify WPA2-PSK (AES/CCMP) is supported by your router            |
|  - Run DEBUG_UDP_console.py to see connection status                  |
+-----------------------------------------------------------------------+

+-----------------------------------------------------------------------+
|  PROBLEM: Button press does nothing in DCS                            |
+-----------------------------------------------------------------------+
|  - Verify the GPIO pin in the Label Creator matches your wiring       |
|  - Check that the button is wired between the GPIO pin and GND        |
|  - Run the debug console to see if commands are being sent            |
|  - Make sure DCS-BIOS is running (check the DCS log)                  |
+-----------------------------------------------------------------------+

+-----------------------------------------------------------------------+
|  PROBLEM: LED does not light up                                       |
+-----------------------------------------------------------------------+
|  - Check LED polarity (longer leg is anode / +, goes toward resistor) |
|  - Verify the 330 ohm resistor is in series                           |
|  - Confirm the GPIO pin in the Label Creator matches your wiring      |
|  - Trigger the Master Caution in DCS to test (pull throttle to idle)  |
+-----------------------------------------------------------------------+

+-----------------------------------------------------------------------+
|  PROBLEM: Compilation fails                                           |
+-----------------------------------------------------------------------+
|  - Re-run the Setup Tool and choose "Reset to Manifest Versions"      |
|  - Make sure only ONE transport is set to 1 in Config.h               |
|  - The Compiler Tool handles board settings automatically, so if you  |
|    previously used Arduino IDE, those old settings will not interfere  |
+-----------------------------------------------------------------------+

+-----------------------------------------------------------------------+
|  PROBLEM: Upload fails                                                |
+-----------------------------------------------------------------------+
|  - Put the board in bootloader mode (hold BOOT, plug in USB)          |
|  - Try a different USB cable (must be a data cable)                   |
|  - Try a different USB port on your PC                                |
|  - Close any other program that might be using the COM port           |
+-----------------------------------------------------------------------+
```

---

## What's Next?

You have a working panel. Here is where to go from here:

- **Add more hardware** -- See the [Hardware Guides](../Hardware/README.md) for wiring rotary encoders, shift registers, I2C expanders, TFT displays, and more
- **Switch to USB HID** -- Change `Config.h` to use `USE_DCSBIOS_USB=1`, recompile, and run the [HID Manager](../Tools/HID-Manager.md) on your PC for lower latency
- **Add another aircraft** -- Create additional label sets for different aircraft using the Label Creator
- **Build multi-panel setups** -- Use [RS485 networking](../How-To/Wire-RS485-Network.md) to connect multiple ESP32 boards together
- **Create custom panels** -- Write your own panel code using [REGISTER_PANEL](../Advanced/Custom-Panels.md) for special behavior
- **Explore the tools** -- Read the [Tools Overview](../Tools/README.md) for detailed documentation on all three Python tools

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

*CockpitOS Quick Start Guide | F/A-18C Hornet | Last updated: February 2026*
*Tested with: ESP32 Arduino Core 3.3.6, LovyanGFX 1.2.19*
