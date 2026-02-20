# Config.h Reference

Complete reference for every setting in `Config.h`. All values shown are the defaults. Settings are grouped by category with recommended values for production and development use.

> **Rule of thumb:** The [Compiler Tool](../Tools/Compiler-Tool.md) manages the most commonly changed settings (transport, WiFi credentials, debug flags, HID mode) through its menus. You do not need to edit `Config.h` by hand for these. This reference documents all settings for advanced use cases.

---

## 1. Transport Mode Selection

Exactly **one** transport must be enabled (set to `1`). The compiler enforces this with a `#error` if zero or more than one are active.

| Flag | Default | Description |
|------|---------|-------------|
| `USE_DCSBIOS_USB` | `1` | Native USB HID via TinyUSB. Requires ESP32-S2, S3, or P4. Requires the HID Manager companion app on the host PC. Lowest latency, recommended for production. |
| `USE_DCSBIOS_WIFI` | `0` | WiFi UDP transport. Works on all ESP32 variants except H2. No bridge software needed on the PC. |
| `USE_DCSBIOS_SERIAL` | `0` | Legacy serial/CDC transport. Requires socat or `connect-serial-port.cmd` from DCS-BIOS. Works on all ESP32 variants. |
| `USE_DCSBIOS_BLUETOOTH` | `0` | BLE transport. Internal/private -- not included in the open-source release. |
| `RS485_SLAVE_ENABLED` | `0` | RS-485 slave mode. The device receives forwarded data from an RS-485 master instead of connecting to DCS-BIOS directly. |

### RS-485 Master/Slave Settings

| Flag | Default | Description |
|------|---------|-------------|
| `RS485_MASTER_ENABLED` | `0` | Enable RS-485 master role. The master still needs one of the four transports above to connect to DCS-BIOS, then forwards data to slaves. |
| `RS485_SLAVE_ADDRESS` | `1` | Slave address ID (1-126). Each slave on the bus must have a unique address. Only used when `RS485_SLAVE_ENABLED=1`. |
| `RS485_SMART_MODE` | `0` | Master only: filter forwarded data to only the addresses your slaves need. Reduces bus bandwidth at the cost of slightly higher latency. |
| `RS485_MAX_SLAVE_ADDRESS` | `127` | Master only: highest slave address to poll (1-127). |
| `RS485_TX_PRE_DE_DELAY_US` | `40` | Microsecond delay before slave response. Prevents collisions with AVR masters. |
| `RS485_TX_WARMUP_DELAY_US` | `0` | Manual DE: delay after DE assert before TX. |
| `RS485_TX_WARMUP_AUTO_DELAY_US` | `0` | Auto-direction: delay before TX (internal RX to TX switch). |
| `RS485_USE_TASK` | `1` | `0` = run in `loop()` (best for WiFi). `1` = dedicated FreeRTOS task (best for USB/Serial/BLE). |
| `RS485_TASK_CORE` | `0` | CPU core for the RS-485 task. `0` = Core 0 (ideal when no WiFi). `1` = Core 1 (shares with `loop()`). Ignored on single-core chips. |

### Transport Compatibility by ESP32 Variant

```
+----------+-----+------+--------+-----+-------+
| Variant  | USB | WiFi | Serial | BLE | RS485 |
+----------+-----+------+--------+-----+-------+
| Classic  |     |  Y   |   Y    |  Y  |   Y   |
| S2       |  Y  |  Y   |   Y    |     |   Y   |
| S3       |  Y  |  Y   |   Y    |  Y  |   Y   |
| C2       |     |  Y   |   Y    |  Y  |   Y   |
| C3       |     |  Y   |   Y    |  Y  |   Y   |
| C5       |     |  Y   |   Y    |  Y  |   Y   |
| C6       |     |  Y   |   Y    |  Y  |   Y   |
| H2       |     |      |   Y    |  Y  |   Y   |
| P4       |  Y  |      |   Y    |     |   Y   |
+----------+-----+------+--------+-----+-------+
```

> **Note:** USB HID mode requires the Arduino IDE "USB Mode" set to **USB-OTG (TinyUSB)** for S3 and P4 boards. S2 boards use TinyUSB by default.

---

## 2. WiFi Configuration

| Flag | Default | Description |
|------|---------|-------------|
| `WIFI_SSID` | `"TestNetwork"` | Your WiFi network name. Loaded from `.credentials/wifi.h` if that file exists; otherwise falls back to this default. |
| `WIFI_PASS` | `"TestingOnly"` | Your WiFi password. Same credential file logic as SSID. |

**Requirements:**
- Network must be **2.4 GHz** (ESP32 does not support 5 GHz)
- Router must support **WPA2-PSK (AES/CCMP)**
- ESP32 and PC must be on the **same network** for DCS-BIOS communication

> **Tip:** Store your real credentials in `.credentials/wifi.h` so they are not overwritten by updates. The file is automatically included if it exists.

---

## 3. Panel Features

| Flag | Default | Description |
|------|---------|-------------|
| `SEND_HID_AXES_IN_DCS_MODE` | `0` | When enabled, analog axis values are sent as HID gamepad axes even while DCS mode is active. Useful for dual-purpose panels. |
| `MODE_DEFAULT_IS_HID` | `0` | When enabled, the device starts in HID gamepad mode instead of DCS mode. Use for panels intended for other simulators (inputs only). |

The following features are configured per label set in `CustomPins.h`, not in `Config.h`:

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_TFT_GAUGES` | `0` | Enable TFT display-based analog gauges. Only enable if your hardware includes TFT screens. |
| `ENABLE_PCA9555` | `0` | Enable PCA9555 I2C I/O expander support. Only enable if your PCB has PCA expander chips. |

---

## 4. Analog Axis Thresholds

These values control how potentiometer and analog axis inputs are interpreted. All values are in raw ADC units (0-4095 range).

| Flag | Default | Description |
|------|---------|-------------|
| `MIDDLE_AXIS_THRESHOLD` | `64` | Deadzone around the midpoint of an axis. Increase for noisy potentiometers (try 128-256). |
| `UPPER_AXIS_THRESHOLD` | `128` | Snap-to-max zone at the top of the axis range. |
| `LOWER_AXIS_THRESHOLD` | `256` | Snap-to-min zone at the bottom of the axis range. |
| `CENTER_DEADZONE_INNER` | `256` | Entry threshold for center detent -- easy to enter the center zone. |
| `CENTER_DEADZONE_OUTER` | `384` | Exit threshold for center detent -- must move further to escape the center zone. |
| `AX_DEFAULT_MIN` | `768` | Starting assumption for the minimum axis reading. CockpitOS auto-learns the real minimum as you move the knob and saves it to NVS. Use `4095` for learning from scratch. |
| `AX_DEFAULT_MAX` | `3327` | Starting assumption for the maximum axis reading. Same auto-learn behavior. Use `0` for learning from scratch. |

---

## 5. Debug Options

**All debug flags must be `0` for production builds.** Debug output consumes significant memory and CPU.

| Flag | Default | Description |
|------|---------|-------------|
| `DEBUG_ENABLED` | `0` | Master debug enable. Use only when troubleshooting. Not required for VERBOSE modes. |
| `DEBUG_LISTENERS_AT_STARTUP` | `0` | Dump all registered DCS-BIOS listeners at boot. Advanced troubleshooting only. |
| `VERBOSE_MODE` | `0` | Full verbose output to both WiFi and Serial. Uses a lot of memory -- may fail to compile on S2 devices. |
| `VERBOSE_MODE_SERIAL_ONLY` | `0` | Verbose output to Serial only. |
| `VERBOSE_MODE_WIFI_ONLY` | `0` | Verbose output to WiFi only. |
| `VERBOSE_PERFORMANCE_ONLY` | `0` | Output performance snapshots only (no general debug). Requires one of the VERBOSE_MODE_*_ONLY flags to select the output channel. |
| `DEBUG_PERFORMANCE` | `0` | Show profiling data and memory usage. |
| `DEBUG_PERFORMANCE_SHOW_TASKS` | `0` | Include the FreeRTOS task list in performance snapshots. |
| `PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS` | `60` | Seconds between performance snapshots. |
| `RS485_DEBUG_VERBOSE` | `0` | Log every RS-485 poll/response. Extremely verbose. |
| `RS485_STATUS_INTERVAL_MS` | `60000` | Milliseconds between RS-485 status banners (60000 = 1 minute). |

> **Memory warning:** On S2 devices, enabling WiFi stack + TinyUSB + debug output together will likely exceed available memory and fail to compile. Use Serial debug when using USB transport, or WiFi debug when using WiFi transport.

---

## 6. Hardware Debug

These flags enable raw I/O tracing for specific hardware peripherals. Use only when mapping port/bit/mask values.

| Flag | Default | Description |
|------|---------|-------------|
| `DEBUG_ENABLED_FOR_PCA_ONLY` | `0` | Trace PCA9555 I2C expander reads. Shows port, bit, and mask for every state change. |
| `DEBUG_ENABLED_FOR_HC165_ONLY` | `0` | Trace 74HC165 shift register reads. Shows bit positions for every state change. |
| `DEBUG_ENABLED_FOR_TM1637_ONLY` | `0` | Trace TM1637 key scanning. Use when mapping button inputs on TM1637 display modules. |

---

## 7. Advanced Configuration

| Flag | Default | Description |
|------|---------|-------------|
| `TEST_LEDS` | `0` | Interactive serial console menu for testing LEDs one at a time. |
| `IS_REPLAY` | `0` | Simulate a loopback DCS stream from recorded data. For testing panels without DCS running. |
| `DCSBIOS_USE_LITE_VERSION` | `1` | Use the bundled lite DCS-BIOS library. Set to `0` to use the full externally-installed library. |
| `USE_WIRE_FOR_I2C` | `1` | `1` = use Arduino Wire library for I2C (compatible). `0` = use faster alternative I2C driver. |
| `PCA_FAST_MODE` | `1` | Enable 400 kHz I2C fast mode for PCA bus. Set to `0` for standard 100 kHz mode. |
| `SCAN_WIFI_NETWORKS` | `0` | At boot, scan and list visible WiFi networks to Serial. Useful for checking what the ESP32 can see. Output is Serial only (cannot use WiFi before connecting). |
| `SKIP_ANALOG_FILTERING` | `0` | HID mode only: skip all analog input filtering for minimum latency. |
| `ADVANCED_TM1637_INPUT_FILTERING` | `0` | Enable extra filtering on TM1637 key inputs. Turn on if you see ghost key presses. |
| `SUPRESS_REBOOT_VIA_CDC` | `0` | Prevent the host PC from resetting the device over CDC serial. Calls `Serial.enableReboot(false)`. |

---

## 8. Timing Parameters

| Flag | Default | Unit | Description |
|------|---------|------|-------------|
| `POLLING_RATE_HZ` | `250` | Hz | Main loop rate for scanning buttons/switches. Valid values: 125, 250, 500. |
| `DCS_UPDATE_RATE_HZ` | `30` | Hz | DCS-BIOS loop update rate. Used for keepalive timing if enabled. |
| `HID_REPORT_RATE_HZ` | `250` | Hz | USB HID report sending rate. |
| `DISPLAY_REFRESH_RATE_HZ` | `60` | Hz | General refresh rate for display updates. |
| `SERVO_UPDATE_FREQ_MS` | `20` | ms | Update interval for analog servo gauge instruments. |
| `VALUE_THROTTLE_MS` | `50` | ms | Minimum interval between sending the same value again (debounce). |
| `ANY_VALUE_THROTTLE_MS` | `33` | ms | Minimum interval between sending any value (prevents USB endpoint spam). |
| `SELECTOR_DWELL_MS` | `250` | ms | Wait time for a stable selector value before sending. Dwell-time filtering. |
| `STREAM_TIMEOUT_MS` | `1000` | ms | Milliseconds without data before declaring the DCS stream dead. |
| `MISSION_START_DEBOUNCE` | `500` | ms | Delay before panel sync after a mission start event. |
| `SERIAL_TX_TIMEOUT` | `5` | ms | CDC serial receive stream timeout. |
| `HID_SENDREPORT_TIMEOUT` | `5` | ms | Timeout for Arduino Core `HID.SendReport()` calls. |
| `CDC_TIMEOUT_RX_TX` | `5` | ms | CDC serial health check timeout. |
| `STARTUP_WATCHDOG_TIMEOUT_MS` | `30000` | ms | If the main loop is not reached within this time, the device enters bootloader mode. |

### Keepalive Settings (Testing Only)

| Flag | Default | Description |
|------|---------|-------------|
| `DCS_KEEP_ALIVE_ENABLED` | `0` | Enable DCS-BIOS keepalive pings. Testing only. |
| `HID_KEEP_ALIVE_ENABLED` | `0` | Enable HID keepalive reports. Testing only. |

---

## 9. Buffer Sizes

### Record and Group Limits

| Flag | Default | Description |
|------|---------|-------------|
| `MAX_TRACKED_RECORDS` | `512` | Maximum tracked I/O records. Do not change. |
| `MAX_GROUPS` | `128` | Maximum input groups. Do not change. |
| `MAX_SELECTOR_GROUPS` | `128` | Maximum selector groups. |
| `MAX_PCA_GROUPS` | `128` | Maximum PCA selector groups. |
| `MAX_PCA9555_INPUTS` | `64` | Maximum PCA9555 input mappings. |
| `MAX_PCAS` | `8` | Maximum PCA9555 chips on the I2C bus (addresses 0x20-0x27). |
| `MAX_PENDING_UPDATES` | `220` | Maximum queued display/output updates. Do not modify unless doing heavy customization. |
| `MAX_REGISTERED_DISPLAY_BUFFERS` | `64` | Maximum registered display buffer slots. Do not modify unless doing heavy customization. |
| `MAX_VALIDATED_SELECTORS` | `32` | Maximum selectors tracked for validation. Tune to match your panel's selector count. |

### Communication Buffers

| Flag | Default | Description |
|------|---------|-------------|
| `SERIAL_RX_BUFFER_SIZE` | `512` | Incoming buffer for DCS data in CDC/Serial mode. Increase if you see OVERFLOW messages. |
| `GAMEPAD_REPORT_SIZE` | `64` | HID report size. Must match the HID descriptor. Do not change. |
| `DCS_USB_RINGBUF_SIZE` | `32` | Outgoing USB packet ring buffer depth. 32 is optimal; use 64 for slow hosts. |
| `DCS_USB_PACKET_MAXLEN` | `64` | Max outgoing USB packet size. Matches `GAMEPAD_REPORT_SIZE`. |
| `DCS_UDP_RINGBUF_SIZE` | `32` | Incoming data ring buffer depth (USB or WiFi). |
| `DCS_UDP_PACKET_MAXLEN` | `64`/`128`/`1472` | Max incoming packet size. Auto-set: 64 for USB, 128 for WiFi/BLE, 1472 otherwise. |
| `MAX_UDP_FRAMES_PER_DRAIN` | `1` | Packets processed per drain cycle. 1 is deterministic and recommended. |

### Debug Ring Buffers

| Flag | Default | Description |
|------|---------|-------------|
| `SERIAL_DEBUG_USE_RINGBUFFER` | `0` | Use a ring buffer for serial debug messages. |
| `WIFI_DEBUG_USE_RINGBUFFER` | `0` | Use a ring buffer for WiFi debug messages. Required when using CDC + WiFi debug to avoid stalls. |

### Internal Buffers (Do Not Modify)

| Flag | Default | Description |
|------|---------|-------------|
| `TASKLIST_LINE_GENERAL_TMP_BUFFER` | `128` | Task list line buffer. Do not exceed 128. |
| `DEBUGPRINTF_GENERAL_TMP_BUFFER` | `256` | `DebugPrintf` format buffer. |
| `SERIAL_DEBUG_BUFFER_SIZE` | `256` | Serial debug message buffer. |
| `WIFI_DEBUG_BUFFER_SIZE` | `256` | WiFi debug message buffer. |
| `UDP_TMPBUF_SIZE` | `1472` | UDP output temporary buffer. |
| `PERF_TMPBUF_SIZE` | `1024` | Performance snapshot append buffer. |
| `SERIAL_DEBUG_FLUSH_BUFFER_SIZE` | `2048` | Serial debug flush buffer. Must be large enough for the biggest message. |
| `SERIAL_DEBUG_OUTPUT_CHUNK_SIZE` | `64` | Chunk size for `Serial.write()` calls. |
| `DCS_UDP_MAX_REASSEMBLED` | `1472` | Maximum reassembled UDP/frame size. |

---

## 10. USB Configuration

| Flag | Default | Description |
|------|---------|-------------|
| `USB_VID` | `0xCAFE` | USB Vendor ID. If you change this, you must also update `settings.ini` in the HID Manager. |
| `USB_PID` | `AUTOGEN_USB_PID` | USB Product ID. Auto-generated per label set for unique device identification. |
| `USB_SERIAL` | `LABEL_SET_STR` | USB serial string. Set to the label set name automatically. |
| `USB_PRODUCT` | `USB_SERIAL` | USB product string. Same as the serial string by default. |
| `USB_MANUFACTURER` | `"CockpitOS"` | USB manufacturer string. |
| `USB_LANG_ID` | `0x0409` | USB language ID. English (US). |

### TinyUSB Internal Settings

These are set automatically. Override only if you know what you are doing.

| Flag | Default | Description |
|------|---------|-------------|
| `CFG_TUSB_DEBUG` | `0` | TinyUSB internal debug logging. |
| `CONFIG_TINYUSB_CDC_RX_BUFSIZE` | `64` | CDC receive buffer. |
| `CONFIG_TINYUSB_CDC_TX_BUFSIZE` | `64` | CDC transmit buffer. |
| `CONFIG_TINYUSB_HID_BUFSIZE` | `64` | HID buffer. |
| `CONFIG_TINYUSB_HID_ENABLED` | `1` | HID class enabled. |
| `CONFIG_TINYUSB_MSC_ENABLED` | `0` | Mass storage class disabled. |
| `CONFIG_TINYUSB_MIDI_ENABLED` | `0` | MIDI class disabled. |
| `CONFIG_TINYUSB_VIDEO_ENABLED` | `0` | Video class disabled. |

---

## 11. Board Detection (Auto-Set)

These flags are set automatically based on the target chip. **Do not modify them manually.**

| Flag | Set When | WiFi | HW Serial |
|------|----------|------|-----------|
| `ESP_FAMILY_CLASSIC` | `CONFIG_IDF_TARGET_ESP32` | Yes | No |
| `ESP_FAMILY_S2` | `CONFIG_IDF_TARGET_ESP32S2` | Yes | No |
| `ESP_FAMILY_S3` | `CONFIG_IDF_TARGET_ESP32S3` | Yes | Yes |
| `ESP_FAMILY_C2` | `CONFIG_IDF_TARGET_ESP32C2` | Yes | Yes |
| `ESP_FAMILY_C3` | `CONFIG_IDF_TARGET_ESP32C3` | Yes | Yes |
| `ESP_FAMILY_C5` | `CONFIG_IDF_TARGET_ESP32C5` | Yes | Yes |
| `ESP_FAMILY_C6` | `CONFIG_IDF_TARGET_ESP32C6` | Yes | Yes |
| `ESP_FAMILY_H2` | `CONFIG_IDF_TARGET_ESP32H2` | No | Yes |
| `ESP_FAMILY_P4` | `CONFIG_IDF_TARGET_ESP32P4` | No | Yes |

---

## 12. Quick Reference

### Production Settings

For a production panel, use these values. All debug flags off, USB transport on an S3 board:

```cpp
#define USE_DCSBIOS_USB                 1
#define USE_DCSBIOS_WIFI                0
#define USE_DCSBIOS_SERIAL              0
#define USE_DCSBIOS_BLUETOOTH           0
#define RS485_SLAVE_ENABLED             0

#define DEBUG_ENABLED                   0
#define DEBUG_PERFORMANCE               0
#define VERBOSE_MODE                    0
#define VERBOSE_MODE_SERIAL_ONLY        0
#define VERBOSE_MODE_WIFI_ONLY          0
#define TEST_LEDS                       0
#define IS_REPLAY                       0
```

### Development / Troubleshooting Settings

For debugging a panel over WiFi transport:

```cpp
#define USE_DCSBIOS_USB                 0
#define USE_DCSBIOS_WIFI                1
#define USE_DCSBIOS_SERIAL              0
#define USE_DCSBIOS_BLUETOOTH           0

#define VERBOSE_MODE_WIFI_ONLY          1
#define DEBUG_PERFORMANCE               1
```

For debugging a panel over USB transport (use Serial for debug output):

```cpp
#define USE_DCSBIOS_USB                 1
#define USE_DCSBIOS_WIFI                0
#define USE_DCSBIOS_SERIAL              0
#define USE_DCSBIOS_BLUETOOTH           0

#define VERBOSE_MODE_SERIAL_ONLY        1
#define DEBUG_PERFORMANCE               1
```

---

## See Also

- [Transport Modes](Transport-Modes.md) -- Detailed guide to each transport
- [Troubleshooting](Troubleshooting.md) -- Common compilation and runtime errors
- [Quick Start](../Getting-Started/Quick-Start.md) -- Build your first panel

---

*CockpitOS Config.h Reference | Last updated: February 2026*
