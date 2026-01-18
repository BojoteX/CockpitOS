# CockpitOS — Config.h Reference

> **Complete reference for all configuration options in Config.h**
> This guide documents every configurable parameter, its purpose, and recommended values.

---

## Table of Contents

1. [Transport Mode Selection](#1-transport-mode-selection)
2. [Panel Features](#2-panel-features)
3. [WiFi Configuration](#3-wifi-configuration)
4. [Debug Options](#4-debug-options)
5. [Hardware-Specific Debug](#5-hardware-specific-debug)
6. [Advanced Configuration](#6-advanced-configuration)
7. [Timing Parameters](#7-timing-parameters)
8. [Buffer Sizes](#8-buffer-sizes)
9. [USB Configuration](#9-usb-configuration)
10. [Board Detection](#10-board-detection)

---

## 1. Transport Mode Selection

Only ONE transport mode can be active at a time. These determine how CockpitOS communicates with DCS-BIOS.

| Flag | Default | Description |
|------|---------|-------------|
| `USE_DCSBIOS_USB` | 1 | **Recommended.** Native USB HID. Requires HID Manager on PC. (S2, S3, P4 only) |
| `USE_DCSBIOS_WIFI` | 0 | WiFi UDP connection. Works with all ESP32 variants (except H2). |
| `USE_DCSBIOS_SERIAL` | 0 | Legacy serial/socat mode. Works with all ESP32 variants. |
| `USE_DCSBIOS_BLUETOOTH` | 0 | *Internal use only.* BLE transport (not publicly released). |

**Important:** The compiler enforces exactly one transport mode. Enabling multiple causes a build error.

### Transport Mode Compatibility

| ESP32 Variant | USB | WiFi | Serial | BLE |
|---------------|-----|------|--------|-----|
| ESP32-S2 | Yes | Yes | Yes | No |
| ESP32-S3 | Yes | Yes | Yes | Yes |
| ESP32-P4 | Yes | Yes | Yes | No |
| ESP32 Classic | No | Yes | Yes | Yes |
| ESP32-C3/C5/C6 | No | Yes | Yes | Yes |
| ESP32-H2 | No | No | Yes | Yes |

---

## 2. Panel Features

Enable or disable major firmware features.

| Flag | Default | Description |
|------|---------|-------------|
| `ENABLE_TFT_GAUGES` | 1 | Enable TFT display support via LovyanGFX. Set to 0 to reduce firmware size if not using TFTs. |
| `ENABLE_PCA9555` | 0 | Enable PCA9555 I2C expander support. Set to 1 if your hardware includes PCA9555 chips. |
| `SEND_HID_AXES_IN_DCS_MODE` | 0 | Send HID axis reports even when in DCS mode. Useful for hybrid setups. |

### Analog Axis Thresholds

Fine-tune analog input behavior for potentiometers and axes.

| Flag | Default | Description |
|------|---------|-------------|
| `MIDDLE_AXIS_THRESHOLD` | 64 | Tolerance for detecting center position (32-64 optimal, 128-256 for noisy axes) |
| `UPPER_AXIS_THRESHOLD` | 128 | Tolerance for detecting maximum position |
| `LOWER_AXIS_THRESHOLD` | 256 | Tolerance for detecting minimum position |
| `CENTER_DEADZONE_INNER` | 256 | Easy entry threshold for center detection |
| `CENTER_DEADZONE_OUTER` | 384 | Must move beyond this to exit center zone (hysteresis) |
| `AX_DEFAULT_MIN` | 768 | Assumed worst minimum for axis auto-learning. Use 4095 to learn from scratch. |
| `AX_DEFAULT_MAX` | 3327 | Assumed worst maximum for axis auto-learning. Use 0 to learn from scratch. |

---

## 3. WiFi Configuration

Required when using `USE_DCSBIOS_WIFI` or WiFi debug output.

| Flag | Default | Description |
|------|---------|-------------|
| `WIFI_SSID` | "TestNetwork" | Your WiFi network name |
| `WIFI_PASS` | "TestingOnly" | Your WiFi password |

**Tip:** For development, use a mobile hotspot for isolated testing. For production, use your main network.

---

## 4. Debug Options

**For production, ALL debug flags should be 0.** Enable only when troubleshooting.

| Flag | Default | Description |
|------|---------|-------------|
| `DEBUG_ENABLED` | 0 | Master debug switch. Enables verbose logging. |
| `DEBUG_LISTENERS_AT_STARTUP` | 0 | Log all DCS-BIOS listener registrations at boot. Advanced troubleshooting only. |
| `VERBOSE_MODE` | 0 | Log INFO messages to both Serial and UDP. |
| `VERBOSE_MODE_SERIAL_ONLY` | 0 | Restrict verbose output to Serial only. |
| `VERBOSE_MODE_WIFI_ONLY` | 0 | Restrict verbose output to WiFi UDP only. Keeps Serial clean. |
| `VERBOSE_PERFORMANCE_ONLY` | 0 | Only output performance snapshots (requires `DEBUG_PERFORMANCE`). |

### Performance Profiling

| Flag | Default | Description |
|------|---------|-------------|
| `DEBUG_PERFORMANCE` | 0 | Enable performance snapshot logging. |
| `DEBUG_PERFORMANCE_SHOW_TASKS` | 0 | Include FreeRTOS task list in snapshots. |
| `PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS` | 60 | Seconds between performance snapshots. |

---

## 5. Hardware-Specific Debug

Enable targeted debugging for specific hardware types.

| Flag | Default | Description |
|------|---------|-------------|
| `DEBUG_ENABLED_FOR_PCA_ONLY` | 0 | Debug output for PCA9555 port/bit mapping. |
| `DEBUG_ENABLED_FOR_HC165_ONLY` | 0 | Debug output for HC165 shift register bit mapping. |
| `DEBUG_ENABLED_FOR_TM1637_ONLY` | 0 | Debug output for TM1637 key scanning. |

**Usage:** Enable one of these when you need to identify which port/bit corresponds to which physical input during initial wiring.

---

## 6. Advanced Configuration

These settings have been tuned for optimal performance. **Change only if you understand the implications.**

| Flag | Default | Description |
|------|---------|-------------|
| `TEST_LEDS` | 0 | Enable interactive LED test menu via Serial console. |
| `IS_REPLAY` | 0 | Enable loopback DCS stream for offline testing. |
| `DCSBIOS_USE_LITE_VERSION` | 1 | Use bundled DCSBIOS library (1) or external install (0). |
| `SCAN_WIFI_NETWORKS` | 0 | Output visible WiFi networks to Serial at startup. |
| `USE_WIRE_FOR_I2C` | 0 | Use Arduino Wire library (slow). 0 = faster alternative. |
| `PCA_FAST_MODE` | 1 | Enable 400kHz I2C for PCA9555. |
| `SKIP_ANALOG_FILTERING` | 0 | Disable analog input filtering for minimum latency (HID only). |
| `ADVANCED_TM1637_INPUT_FILTERING` | 0 | Extra filtering for TM1637 key ghosting issues. |
| `SUPRESS_REBOOT_VIA_CDC` | 0 | Prevent device reset via CDC serial. Sets `Serial.enableReboot(false)`. |

---

## 7. Timing Parameters

Control polling rates, timeouts, and throttling behavior.

### Polling and Refresh Rates

| Flag | Default | Description |
|------|---------|-------------|
| `POLLING_RATE_HZ` | 250 | Main loop input polling rate. 125, 250, or 500 Hz. |
| `DCS_UPDATE_RATE_HZ` | 30 | DCS-BIOS loop update rate (keep-alives). |
| `HID_REPORT_RATE_HZ` | 250 | USB HID report sending rate. |
| `DISPLAY_REFRESH_RATE_HZ` | 60 | General display refresh rate. |
| `SERVO_UPDATE_FREQ_MS` | 20 | Servo motor update interval (50Hz standard). |

### Throttling and Debouncing

| Flag | Default | Description |
|------|---------|-------------|
| `VALUE_THROTTLE_MS` | 50 | Ignore duplicate values within this window. |
| `ANY_VALUE_THROTTLE_MS` | 33 | Minimum spacing between any command values. |
| `SELECTOR_DWELL_MS` | 250 | Wait time for stable selector value before sending. |
| `STREAM_TIMEOUT_MS` | 1000 | Consider stream dead after this many ms without activity. |
| `MISSION_START_DEBOUNCE` | 500 | Wait time before panel sync on mission start. |

### Timeouts

| Flag | Default | Description |
|------|---------|-------------|
| `SERIAL_TX_TIMEOUT` | 5 | Serial transmit timeout (ms). |
| `HID_SENDREPORT_TIMEOUT` | 5 | HID report send timeout (ms). |
| `CDC_TIMEOUT_RX_TX` | 5 | CDC receive/transmit timeout (ms). |

### Keep-Alives

| Flag | Default | Description |
|------|---------|-------------|
| `DCS_KEEP_ALIVE_ENABLED` | 0 | Send periodic DCS-BIOS pings. Testing only. |
| `HID_KEEP_ALIVE_ENABLED` | 0 | Send periodic HID reports. Testing only. |

---

## 8. Buffer Sizes

Memory allocation for various subsystems. **Do not change unless experiencing overflow issues.**

### Safety Limits

| Flag | Default | Description |
|------|---------|-------------|
| `MAX_TRACKED_RECORDS` | 512 | Maximum commands in history buffer. |
| `MAX_GROUPS` | 128 | Maximum selector groups. |
| `MAX_SELECTOR_GROUPS` | 128 | Alias for MAX_GROUPS. |
| `MAX_PCA_GROUPS` | 128 | Maximum PCA selector groups. |
| `MAX_PCA9555_INPUTS` | 64 | Maximum PCA9555 input mappings. |
| `MAX_PCAS` | 8 | Maximum PCA9555 chips (addresses 0x20-0x27). |
| `MAX_PENDING_UPDATES` | 220 | Maximum pending DCS-BIOS updates. |
| `MAX_REGISTERED_DISPLAY_BUFFERS` | 64 | Maximum registered display buffers. |
| `MAX_VALIDATED_SELECTORS` | 32 | Maximum tracked validated selectors. |

### Communication Buffers

| Flag | Default | Description |
|------|---------|-------------|
| `SERIAL_RX_BUFFER_SIZE` | 512 | Incoming serial buffer. Increase if OVERFLOW messages appear. |
| `GAMEPAD_REPORT_SIZE` | 64 | HID report size. Must match descriptor. Never change. |
| `DCS_USB_RINGBUF_SIZE` | 32 | Outgoing USB command buffer slots. |
| `DCS_USB_PACKET_MAXLEN` | 64 | Maximum USB packet size. |
| `DCS_UDP_RINGBUF_SIZE` | 32-64 | Incoming packet buffer slots (varies by transport). |
| `DCS_UDP_PACKET_MAXLEN` | 64 | Maximum incoming packet size. |
| `DCS_UDP_MAX_REASSEMBLED` | 1472 | Maximum reassembled UDP frame size. |

### Debug Buffers

| Flag | Default | Description |
|------|---------|-------------|
| `DEBUGPRINTF_GENERAL_TMP_BUFFER` | 256 | Buffer for debugPrintf messages. |
| `SERIAL_DEBUG_BUFFER_SIZE` | 256 | Buffer for serial debug messages. |
| `WIFI_DEBUG_BUFFER_SIZE` | 256 | Buffer for WiFi debug messages. |
| `SERIAL_DEBUG_FLUSH_BUFFER_SIZE` | 2048 | Large message flush buffer. |
| `SERIAL_DEBUG_OUTPUT_CHUNK_SIZE` | 64 | Serial write chunk size. |
| `UDP_TMPBUF_SIZE` | 1472 | UDP output temp buffer. |
| `PERF_TMPBUF_SIZE` | 1024 | Performance logging temp buffer. |

### Ring Buffer Configuration

| Flag | Default | Description |
|------|---------|-------------|
| `SERIAL_DEBUG_USE_RINGBUFFER` | 0 | Use ring buffer for serial debug. |
| `WIFI_DEBUG_USE_RINGBUFFER` | 0 | Use ring buffer for WiFi debug. Required with WiFi DCS + CDC. |
| `MAX_UDP_FRAMES_PER_DRAIN` | 1 | Max frames to process per drain cycle. 1 = deterministic. |

---

## 9. USB Configuration

USB descriptor settings. The VID/PID identify your device to the operating system.

| Flag | Default | Description |
|------|---------|-------------|
| `USB_VID` | 0xCAFE | USB Vendor ID. Match in HID Manager's settings.ini. |
| `USB_PID` | AUTOGEN_USB_PID | USB Product ID. Auto-generated per label set. |
| `USB_SERIAL` | LABEL_SET_STR | USB serial number string. |
| `USB_PRODUCT` | USB_SERIAL | USB product name (shown in device manager). |
| `USB_MANUFACTURER` | "CockpitOS" | USB manufacturer string. |
| `USB_LANG_ID` | 0x0409 | USB language ID (English US). |

### TinyUSB Settings

| Flag | Default | Description |
|------|---------|-------------|
| `CFG_TUSB_DEBUG` | 0 | Enable TinyUSB internal debugging. |
| `CONFIG_TINYUSB_CDC_RX_BUFSIZE` | 64 | CDC receive buffer size. |
| `CONFIG_TINYUSB_CDC_TX_BUFSIZE` | 64 | CDC transmit buffer size. |
| `CONFIG_TINYUSB_HID_BUFSIZE` | 64 | HID buffer size. |
| `CONFIG_TINYUSB_HID_ENABLED` | 1 | Enable HID class. |
| `CONFIG_TINYUSB_CDC_ENABLED` | 1 | Enable CDC class. |
| `CONFIG_TINYUSB_MSC_ENABLED` | 0 | Mass storage (disabled). |
| `CONFIG_TINYUSB_MIDI_ENABLED` | 0 | MIDI class (disabled). |

---

## 10. Board Detection

These flags are automatically set based on the target ESP32 variant. **Do not modify.**

| Flag | Set When | Description |
|------|----------|-------------|
| `ESP_FAMILY_S2` | ESP32-S2 | Native USB, no BLE |
| `ESP_FAMILY_S3` | ESP32-S3 | Native USB, BLE |
| `ESP_FAMILY_P4` | ESP32-P4 | Native USB |
| `ESP_FAMILY_CLASSIC` | Original ESP32 | WiFi/Serial only, BLE |
| `ESP_FAMILY_C3` | ESP32-C3 | RISC-V, BLE |
| `ESP_FAMILY_C5` | ESP32-C5 | RISC-V, BLE |
| `ESP_FAMILY_C6` | ESP32-C6 | RISC-V, BLE |
| `ESP_FAMILY_H2` | ESP32-H2 | BLE only (no WiFi) |
| `DEVICE_HAS_HWSERIAL` | Varies | Whether hardware serial is available |

---

## Quick Reference: Production Settings

For a production build, use these settings:

```cpp
// Transport (choose ONE)
#define USE_DCSBIOS_USB     1
#define USE_DCSBIOS_WIFI    0
#define USE_DCSBIOS_SERIAL  0
#define USE_DCSBIOS_BLUETOOTH 0

// Features
#define ENABLE_TFT_GAUGES   1
#define ENABLE_PCA9555      0  // Set to 1 if using PCA chips

// All debug OFF
#define DEBUG_ENABLED       0
#define VERBOSE_MODE        0
#define DEBUG_PERFORMANCE   0
#define TEST_LEDS           0
#define IS_REPLAY           0
```

---

## Quick Reference: Development Settings

For debugging during development:

```cpp
// Enable verbose output to Serial
#define DEBUG_ENABLED       1
#define VERBOSE_MODE        1
#define VERBOSE_MODE_SERIAL_ONLY 1

// Performance monitoring
#define DEBUG_PERFORMANCE   1
#define PERFORMANCE_SNAPSHOT_INTERVAL_SECONDS 30

// Hardware-specific (enable ONE at a time)
#define DEBUG_ENABLED_FOR_PCA_ONLY    0
#define DEBUG_ENABLED_FOR_HC165_ONLY  0
#define DEBUG_ENABLED_FOR_TM1637_ONLY 0
```

---

*For transport mode details, see [TRANSPORT_MODES.md](TRANSPORT_MODES.md).*
*For hardware wiring, see [HARDWARE_WIRING.md](HARDWARE_WIRING.md).*
