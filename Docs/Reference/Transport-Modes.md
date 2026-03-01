# Transport Modes

CockpitOS supports four transport modes for communicating with DCS-BIOS, plus RS-485 networking for multi-panel setups. This guide covers each mode in detail: how it works, how to configure it, which hardware supports it, and how to verify it is working.

---

## Overview

```
+-------------------------------------------------------------------+
|                        DCS World (PC)                              |
|                             |                                      |
|                         DCS-BIOS                                   |
|                       (LUA export)                                 |
|                             |                                      |
|              UDP multicast 239.255.50.10:5010 (export)             |
|              UDP port 7778 (commands)                               |
|                             |                                      |
+----+-----------+-----------+-----------+---------+----------------+
     |           |           |           |         |
   WiFi        USB         CDC       BLE       RS-485
   (UDP)     (HID+        (Serial)  (BLE)     (Master
            Manager)                          forwards)
     |           |           |           |         |
+----+-----------+-----------+-----------+---------+----------------+
|                     ESP32 Panel (CockpitOS)                        |
+-------------------------------------------------------------------+
```

### Quick Comparison

| | USB HID | WiFi UDP | Serial/CDC | BLE |
|---|---------|----------|------------|-----|
| **Latency** | Lowest | Low | Low | Medium |
| **Reliability** | Highest | Good | Good | Fair |
| **PC Software** | HID Manager | None | socat / connect-serial-port | None |
| **Wireless** | No | Yes | No | Yes |
| **ESP32 Variants** | S2, S3, P4 | All except H2, P4 | All | Classic, S3, C2/C3/C5/C6, H2 |
| **Max Panels** | 32 (via HID Manager) | Unlimited | 1 per COM port | 1 per connection |

---

## Decision Flowchart

```
Start
  |
  v
Is your board an S2, S3, or P4?
  |           |
  Yes         No
  |           |
  v           v
Use USB HID   Do you need wireless?
(recommended)   |           |
                Yes         No
                |           |
                v           v
              Use WiFi    Use Serial/CDC
```

---

## 1. USB HID (Recommended)

USB HID is the recommended transport for production panels. The ESP32 presents itself as a USB HID device (gamepad), and the HID Manager application on the PC bridges between the USB device and DCS-BIOS over UDP.

### Requirements

- **Board:** ESP32-S2, ESP32-S3, or ESP32-P4
- **PC Software:** HID Manager (Python companion app)
- **Arduino IDE setting:** USB Mode = USB-OTG (TinyUSB) [S3/P4 only]
- **Arduino IDE setting:** USB CDC On Boot = Disabled

### Config.h Settings

```cpp
#define USE_DCSBIOS_USB      1
#define USE_DCSBIOS_WIFI     0
#define USE_DCSBIOS_SERIAL   0
#define USE_DCSBIOS_BLUETOOTH 0
#define RS485_SLAVE_ENABLED  0
```

### How It Works

```
ESP32                          PC
+----------+    USB HID     +-------------+      UDP      +---------+
| CockpitOS| ------------> | HID Manager | -----------> | DCS-BIOS|
|          | <------------ |             | <----------- |         |
+----------+  64-byte rpts +-------------+  5010/7778   +---------+
```

1. CockpitOS sends button/switch/axis data as 64-byte USB HID reports.
2. HID Manager receives these reports and converts them into DCS-BIOS text commands (e.g., `"MASTER_ARM_SW 1\n"`) sent to UDP port 7778.
3. DCS-BIOS exports cockpit state as binary frames over UDP multicast 239.255.50.10:5010.
4. HID Manager receives these frames and forwards them to the ESP32 as USB HID reports.

### USB Identifiers

| Setting | Value | Notes |
|---------|-------|-------|
| `USB_VID` | `0xCAFE` | Must match `settings.ini` in HID Manager |
| `USB_PID` | Auto-generated | Unique per label set. Allows HID Manager to identify which panel is which. |
| `USB_MANUFACTURER` | `"CockpitOS"` | |
| `USB_PRODUCT` | Label set name | Shown in Device Manager and HID Manager |

### Setup Steps

1. In the Compiler Tool, select **Role / Transport** and choose **USB Native**. The tool writes all Config.h flags automatically.
2. Compile and upload using the Compiler Tool.
3. On the PC, start HID Manager. It auto-discovers devices with VID `0xCAFE`.
4. Start DCS World and load a mission.
5. HID Manager status should show "Connected" for your panel.

### Verification

- **Device Manager (Windows):** Your ESP32 should appear under "USB HID devices" or "Game controllers."
- **HID Manager console:** Shows device name, VID/PID, and connection status.
- **Test input:** Press a button on your panel. HID Manager should log the command being sent.

---

## 2. WiFi UDP

WiFi transport uses UDP multicast to communicate directly with DCS-BIOS over your local network. No bridge software is needed on the PC.

### Requirements

- **Board:** Any ESP32 variant with WiFi (all except H2 and P4)
- **Network:** 2.4 GHz WPA2-PSK (AES/CCMP)
- **PC Software:** None (DCS-BIOS handles UDP natively)
- **Firewall:** UDP ports 5010 and 7778 must be open

### Config.h Settings

```cpp
#define USE_DCSBIOS_USB      0
#define USE_DCSBIOS_WIFI     1
#define USE_DCSBIOS_SERIAL   0
#define USE_DCSBIOS_BLUETOOTH 0
#define RS485_SLAVE_ENABLED  0
```

WiFi credentials:
```cpp
#define WIFI_SSID  "YourNetworkName"
#define WIFI_PASS  "YourPassword"
```

Or store them in `.credentials/wifi.h` (recommended to avoid overwriting on updates):
```cpp
#define WIFI_SSID  "YourNetworkName"
#define WIFI_PASS  "YourPassword"
```

### How It Works

```
ESP32                                       PC
+----------+         WiFi Network        +---------+
| CockpitOS| -- UDP 239.255.50.10:5010 ->| DCS-BIOS|
|          | <- UDP 239.255.50.10:5010 --|         |
|          | -- UDP port 7778 ---------->|         |
+----------+                             +---------+
```

1. CockpitOS joins the UDP multicast group 239.255.50.10 on port 5010 to receive DCS-BIOS export data.
2. CockpitOS sends DCS-BIOS text commands to UDP port 7778 on the PC.
3. No intermediate software is needed -- DCS-BIOS handles multicast natively.

### Setup Steps

1. In the Compiler Tool, select **Role / Transport** and choose **WiFi**. Then set your WiFi credentials in **Misc Options** > **Wi-Fi Credentials**. The tool writes all Config.h flags and `.credentials/wifi.h` automatically.
2. Compile and upload.
3. Ensure your PC and ESP32 are on the same 2.4 GHz network.
4. Start DCS World and load a mission. The panel connects automatically.

### Verification

- **Serial monitor:** At boot, CockpitOS logs the WiFi connection status and IP address.
- **WiFi debug console:**
  ```
  python "Debug Tools\CONSOLE_UDP_debug.py"
  ```
  Shows incoming DCS-BIOS frames and outgoing commands.
- **Network scan:** Set `SCAN_WIFI_NETWORKS=1` in `Config.h`, recompile, and check Serial output to see what networks the ESP32 detects.

### WiFi Troubleshooting

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| No WiFi connection | Wrong SSID/password | Check credentials (case-sensitive) |
| Connects but no data | 5 GHz network | Switch to 2.4 GHz |
| Intermittent drops | WPA3 or enterprise auth | Use WPA2-PSK |
| Firewall blocking | Windows Firewall | Allow UDP 5010 and 7778 inbound/outbound |
| Cannot see network | Range or interference | Move closer to router, check 2.4 GHz band is enabled |

---

## 3. Serial / CDC

Serial transport is the legacy method used by original DCS-BIOS Arduino panels. The ESP32 communicates over a virtual serial port (USB CDC), and a bridge program on the PC relays data between the serial port and DCS-BIOS UDP.

### Requirements

- **Board:** Any ESP32 variant
- **PC Software:** `socat` (Linux/Mac) or DCS-BIOS `connect-serial-port.cmd` (Windows)
- **USB Cable:** Data-capable USB cable

### Config.h Settings

```cpp
#define USE_DCSBIOS_USB      0
#define USE_DCSBIOS_WIFI     0
#define USE_DCSBIOS_SERIAL   1
#define USE_DCSBIOS_BLUETOOTH 0
#define RS485_SLAVE_ENABLED  0
```

### How It Works

```
ESP32                           PC
+----------+   USB Serial   +--------+      UDP      +---------+
| CockpitOS| ------------> | socat  | -----------> | DCS-BIOS|
|          | <------------ |  or    | <----------- |         |
+----------+    CDC port   | bridge |  5010/7778   +---------+
                           +--------+
```

1. CockpitOS sends and receives DCS-BIOS protocol data over the USB CDC serial port.
2. The bridge software (`socat` or `connect-serial-port.cmd`) relays serial data to/from DCS-BIOS UDP.

### Setup Steps

1. In the Compiler Tool, select **Role / Transport** and choose **Serial (CDC/Socat)**. The tool writes all Config.h flags automatically.
2. Compile and upload.
3. Note the COM port assigned to the ESP32.
4. Run the serial bridge:
   - **Windows:** Use `connect-serial-port.cmd` from the DCS-BIOS tools folder, specifying the COM port.
   - **Linux/Mac:** Use `socat` to bridge the serial device to UDP.
5. Start DCS World and load a mission.

### Verification

- Open a serial monitor at 115200 baud. You should see DCS-BIOS binary data flowing when a mission is loaded.
- The serial bridge program should report a successful connection.

---

## 4. Bluetooth Low Energy (BLE)

BLE transport is internal/experimental and **not included in the open-source release** of CockpitOS. It is mentioned here for completeness.

### Requirements

- **Board:** ESP32 Classic, S3, C2, C3, C5, C6, or H2 (not S2 or P4 -- they lack BLE hardware)
- **Availability:** Private builds only

### Config.h Settings

```cpp
#define USE_DCSBIOS_USB      0
#define USE_DCSBIOS_WIFI     0
#define USE_DCSBIOS_SERIAL   0
#define USE_DCSBIOS_BLUETOOTH 1
#define RS485_SLAVE_ENABLED  0
```

### Notes

- BLE has higher latency than USB or WiFi.
- Useful for completely wireless panels where WiFi is not available (e.g., ESP32-H2 which has no WiFi radio).
- The open-source build will produce a compile error if `USE_DCSBIOS_BLUETOOTH=1`:
  ```
  "The Open source version of CockpitOS does NOT include BLE support."
  ```

---

## 5. RS-485 Networking

RS-485 is not a transport to DCS-BIOS itself. Instead, it is a **multi-panel networking layer**. One ESP32 acts as the master (connected to DCS-BIOS via USB, WiFi, or Serial), and one or more ESP32 slaves receive forwarded data over an RS-485 bus.

### Architecture

```
                DCS-BIOS (PC)
                     |
                 USB / WiFi
                     |
              +------+------+
              | ESP32 Master |
              | (RS485_MASTER|
              |  _ENABLED=1) |
              +------+------+
                     |
            RS-485 Bus (A/B lines)
          +----------+----------+
          |          |          |
     +----+----+ +--+-----+ +-+------+
     | Slave 1 | | Slave 2| | Slave 3|
     | Addr: 1 | | Addr: 2| | Addr: 3|
     +---------+ +--------+ +--------+
```

### Master Configuration

The master device needs one standard transport (USB, WiFi, or Serial) **plus** the RS-485 master flag:

```cpp
// Standard transport (pick one)
#define USE_DCSBIOS_USB          1
// ... others = 0

// RS-485 Master
#define RS485_MASTER_ENABLED     1
#define RS485_SMART_MODE         0   // 1 = filter by master's DcsOutputTable
#define RS485_MAX_SLAVE_ADDRESS  127 // highest address to poll
```

### Slave Configuration

Each slave uses `RS485_SLAVE_ENABLED` as its transport and has a unique address:

```cpp
#define USE_DCSBIOS_USB          0
#define USE_DCSBIOS_WIFI         0
#define USE_DCSBIOS_SERIAL       0
#define USE_DCSBIOS_BLUETOOTH    0
#define RS485_SLAVE_ENABLED      1

#define RS485_SLAVE_ADDRESS      1   // unique, 1-126
```

### Relay Mode vs Smart Mode

By default, the master operates in **Relay Mode** (`RS485_SMART_MODE=0`), forwarding the entire DCS-BIOS stream to all slaves with no filtering and no additional configuration required.

When `RS485_SMART_MODE=1`, the master filters the DCS-BIOS data stream using its own DcsOutputTable and only forwards addresses covered by the master's panels. This provides 20-50x bandwidth reduction but requires that the master's `selected_panels.txt` include ALL panels that ANY slave on the bus relies on. Use the Label Creator to add slave panels to the master's label set.

### Wiring

RS-485 uses a differential pair (A and B lines). See the hardware wiring guide for connector pinouts and termination resistor placement.

For detailed RS-485 wiring instructions, see the How-To guide (when available).

---

## Verification Steps Summary

### For All Modes

1. Compile succeeds with no transport errors.
2. Upload completes and the device boots (LED blinks or serial output appears).
3. DCS-BIOS is installed and a mission is loaded in DCS World.

### USB HID Specific

4. Device appears in Windows Device Manager under game controllers.
5. HID Manager shows the device as connected.
6. Press a button -- HID Manager logs the command.

### WiFi Specific

4. Serial monitor shows WiFi connected with an IP address.
5. `CONSOLE_UDP_debug.py` shows data flowing.
6. Press a button -- debug console logs the outgoing command.

### Serial Specific

4. COM port appears in Device Manager.
5. Serial bridge (socat / connect-serial-port) is running and connected.
6. Serial monitor shows binary data when a mission is loaded.

---

## Debug Tips

| Tool | Purpose | How to Enable |
|------|---------|---------------|
| Verbose Serial | Log all activity to Serial (use with USB transport) | Compiler Tool > Misc Options > Debug Toggles |
| Verbose WiFi | Log all activity to WiFi debug console (use with WiFi transport) | Compiler Tool > Misc Options > Debug Toggles |
| Performance profiling | Show CPU load and memory usage | Compiler Tool > Misc Options > Debug Toggles |
| `CONSOLE_UDP_debug.py` | Monitor UDP traffic between ESP32 and DCS-BIOS | Run from `Debug Tools/` |
| `LOG_DCS_commands.py` | Log all DCS-BIOS commands being sent | Run from `Debug Tools/` |
| `SEND_CommandTester.py` | Send test commands to DCS-BIOS manually | Run from `Debug Tools/` |
| `SCAN_WIFI_NETWORKS=1` | List WiFi networks visible to the ESP32 (Serial output) | Manual in `Config.h` |
| `RS485_DEBUG_VERBOSE=1` | Log every RS-485 poll/response (very verbose) | Manual in `Config.h` |

---

## See Also

- [Config.h Reference](Config.md) -- All transport and timing settings
- [Troubleshooting](Troubleshooting.md) -- Connection and upload problems
- [FAQ](FAQ.md) -- Choosing the right transport
- [Debug Tools](../Tools/Debug-Tools.md) -- Python debug utilities

---

*CockpitOS Transport Modes Reference | Last updated: February 2026*
