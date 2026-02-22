# CockpitOS — GPT Knowledge Base

> This document is the complete reference for the CockpitOS Build Assistant GPT.
> It contains everything needed to help users — from first-time setup to advanced firmware development.
> Last updated: February 2026

---

## PART 1: WHAT IS COCKPITOS

CockpitOS is ESP32 firmware (C++/Arduino) for building physical cockpit panels that interface with DCS World (a combat flight simulator) through DCS-BIOS (a LUA export protocol). Users wire real buttons, switches, LEDs, displays, and gauges to ESP32 microcontroller boards. The firmware translates physical actions into DCS cockpit commands and cockpit state back into physical outputs.

### Key Facts
- Language: C++ (Arduino framework) for firmware, Python 3.12+ for tools
- Platform: ESP32 family (S2, S3, P4, Classic, C3/C5/C6, H2)
- Protocol: DCS-BIOS binary export stream + text commands over UDP
- Configuration: "Label Sets" — per-aircraft/per-panel config folders in src/LABELS/
- Tools: Three Python TUI apps automate the ENTIRE workflow — no Arduino IDE needed
- OS: Windows only (tools use msvcrt for keyboard input)
- License: MIT, open source

### What Makes CockpitOS Different
- Zero heap allocation — all memory is static, no malloc/new in loop paths
- 250 Hz input polling — faster than human perception
- O(1) hash lookups — compile-time perfect hash tables for DCS-BIOS control names
- Non-blocking I/O throughout — watchdog never triggers (resets at ~3 seconds)
- One firmware for all aircraft — only the label set changes per aircraft/panel

---

## PART 2: THE THREE PYTHON TOOLS

Three Python TUI tools automate the entire workflow. Users do NOT need Arduino IDE. The tools handle everything.

### Tool 1: Setup Tool (Setup-START.py)
- **Purpose:** Install ESP32 Arduino Core v3.3.6 and LovyanGFX v1.2.19
- **Uses:** Bundled arduino-cli v1.4.1 (no Arduino IDE required)
- **Menu options:** [1] Install/Update, [2] Reset to Manifest Versions, [Q] Quit
- **Install location:** ESP32 Core to %LOCALAPPDATA%\Arduino15\, Libraries to Documents\Arduino\libraries\
- **First run:** Downloads ~500MB, takes 5-15 minutes
- **When to re-run:** After library issues, use option [2] to reset to known-good versions

### Tool 2: Compiler Tool (CockpitOS-START.py)
- **Main module:** compiler/cockpitos.py
- **Purpose:** Board selection, transport configuration, compile, upload
- **Key menus:**
  - **Role / Transport** — Set transport mode (USB, WiFi, Serial, BLE) and RS485 master/slave role
  - **Misc Options > Wi-Fi Credentials** — Set SSID and password (saved to .credentials/wifi.h)
  - **Misc Options > Debug / Verbose Toggles** — Enable/disable DEBUG_ENABLED, VERBOSE_MODE_WIFI_ONLY, VERBOSE_MODE_SERIAL_ONLY, DEBUG_PERFORMANCE
  - **Misc Options > Advanced Settings > HID mode by default** — Toggle MODE_DEFAULT_IS_HID
- **What it manages in Config.h (TRACKED_DEFINES):** USE_DCSBIOS_USB, USE_DCSBIOS_WIFI, USE_DCSBIOS_SERIAL, USE_DCSBIOS_BLUETOOTH, RS485_MASTER_ENABLED, RS485_SLAVE_ENABLED, RS485_SMART_MODE, RS485_MAX_SLAVE_ADDRESS, RS485_SLAVE_ADDRESS, DEBUG_ENABLED, VERBOSE_MODE, VERBOSE_MODE_WIFI_ONLY, VERBOSE_MODE_SERIAL_ONLY, DEBUG_PERFORMANCE, MODE_DEFAULT_IS_HID
- **What it does NOT manage (manual Config.h editing still required):** IS_REPLAY, TEST_LEDS, SCAN_WIFI_NETWORKS, POLLING_RATE_HZ, PCA_FAST_MODE, axis thresholds, SELECTOR_DWELL_MS, SERVO_UPDATE_FREQ_MS, per-device debug flags (DEBUG_ENABLED_FOR_TM1637_ONLY, DEBUG_ENABLED_FOR_PCA_ONLY, DEBUG_ENABLED_FOR_HC165_ONLY), RS485_USE_TASK, RS485_TASK_CORE, RS485 timing parameters

### Tool 3: Label Creator (LabelCreator-START.py)
- **Main module:** label_creator/label_creator.py
- **Purpose:** Create, modify, and delete label sets with guided TUI editors
- **Main menu:** Create New Label Set, Modify Label Set, Delete Label Set, Auto-Generate Label Set, Compile Tool, Environment Setup, Exit
- **Seven built-in editors:**

| Editor | File Edited | What It Does |
|--------|-------------|--------------|
| Input Editor | InputMapping.h | Map physical inputs (buttons, switches, encoders, pots) to DCS-BIOS controls. Set Source (GPIO/PCA/HC165/TM1637/MATRIX/NONE), Port, Bit |
| LED Editor | LEDMapping.h | Map DCS-BIOS outputs to physical indicators. Set Device (GPIO/PCA9555/WS2812/TM1637/GN1640T/GAUGE), pin/address/index |
| Display Editor | DisplayMapping.cpp | Link DCS-BIOS display fields to segment map definitions |
| Segment Map Editor | *_SegmentMap.h | Define HT1622 LCD RAM-to-segment mapping for each character position |
| Custom Pins Editor | CustomPins.h | Set pin assignments (SDA, SCL, HC165 pins, WS2812 data pin, etc.) and feature enables (ENABLE_TFT_GAUGES, ENABLE_PCA9555) |
| Latched Buttons Editor | LatchedButtons.h | Declare which momentary buttons should behave as toggle (latched) buttons |
| CoverGate Editor | CoverGates.h | Define guarded switch covers (cover switch + guarded switch pairs) |

- **Also manages:** Device Name (LabelSetConfig.h), HID Mode Selector, panel selection (selected_panels.txt)
- **Tool switching:** All three tools can launch each other via os.execl()

### Critical Rule: Always Direct Users to the Tools
When a user asks how to change transport, set WiFi credentials, map an input, change an LED, or any configuration task — tell them which tool and which menu to use. Do NOT tell them to manually edit Config.h, InputMapping.h, or LEDMapping.h unless the specific setting is NOT managed by the tools.

---

## PART 3: FIRST-TIME SETUP (STEP BY STEP)

This is the complete workflow for a new user:

### Step 1: Prerequisites
- Windows PC with Python 3.12+
- ESP32 board (S2, S3, or P4 recommended for USB HID; any variant works for WiFi)
- USB DATA cable (not charge-only — this is the #1 mistake)
- DCS World installed with DCS-BIOS (https://github.com/DCS-Skunkworks/dcs-bios/releases)

### Step 2: Download CockpitOS
- Download from https://github.com/BojoteX/CockpitOS/releases/
- Extract to Documents\Arduino\CockpitOS\
- CRITICAL: The folder MUST be named exactly "CockpitOS" (Arduino requires folder name to match .ino filename)

### Step 3: Run the Setup Tool
- Double-click Setup-START.py (or: python Setup-START.py)
- Select [1] Install / Update
- Installs ESP32 Arduino Core v3.3.6 and LovyanGFX v1.2.19
- First run takes 5-15 minutes (downloads ~500MB)
- No Arduino IDE needed — the Setup Tool handles everything

### Step 4: Create a Label Set
- Double-click LabelCreator-START.py (or: python LabelCreator-START.py)
- Select "Create New Label Set"
- Pick your aircraft (e.g., FA-18C_hornet)
- Name your label set (e.g., MY_FIRST_PANEL)
- Select which cockpit panels to include
- The tool auto-generates all mapping files

### Step 5: Map Your Hardware
- In Label Creator, select "Modify Label Set" > your label set
- Use "Edit Inputs" to assign GPIO pins to buttons/switches/encoders
- Use "Edit Outputs (LEDs)" to assign GPIO pins to indicator LEDs
- Use "Edit Custom Pins" to set I2C/SPI/shift register pin assignments if using expanders
- Editors save automatically on exit

### Step 6: Configure Transport and Compile
- Run CockpitOS-START.py (or launch from Label Creator menu)
- Select Role / Transport > choose your transport (WiFi for beginners, USB HID for production)
- If WiFi: go to Misc Options > Wi-Fi Credentials > enter SSID and password
- Select your board type and COM port
- Compile (first build: 3-10 minutes, subsequent: much faster)
- Upload to ESP32

### Step 7: Test in DCS
- If USB HID: run HID Manager (python "HID Manager\HID_Manager.py")
- Start DCS World, load a mission with your aircraft
- Your panel should respond to cockpit state

---

## PART 4: LABEL SET STRUCTURE

Each label set is a folder in src/LABELS/LABEL_SET_<name>/ containing:

### InputMapping.h — Input Definitions
```cpp
{ "LABEL", "SOURCE", port, bit, hidButton, "dcsCommand", sendValue, "controlType", group }
```

| Field | Values | Description |
|-------|--------|-------------|
| LABEL | string | Human-readable name (from DCS-BIOS) |
| SOURCE | "GPIO", "PCA_0xNN", "HC165", "TM1637", "MATRIX", "NONE" | Hardware source |
| port | int | GPIO pin number; PCA port (0 or 1); HC165: always 0; MATRIX: data pin (anchor) or strobe pin (position) |
| bit | int | PCA: bit 0-7 within port; HC165: absolute bit position in chain (0-based); GPIO selector: -1 marks default/fallback position; MATRIX: strobe mask (anchor) or one-hot index (position) |
| hidButton | int | USB HID button number (-1 = none) |
| dcsCommand | string | DCS-BIOS command name |
| sendValue | int | Value to send (0/1 momentary, position for selector, 65535 analog) |
| controlType | "momentary", "selector", "analog", "variable_step", "fixed_step" | Behavior |
| group | int | Selector group (0 = ungrouped, 1+ = grouped selectors share same command) |

### Control Types Explained

**momentary** — Press-and-release. sendValue sent on press, nothing on release. Used for: push buttons, momentary switches.

**selector** — Multi-position switch. Multiple entries share the same dcsCommand and group number but different sendValues. The firmware detects which position is active. Group 0 means ungrouped. An entry with bit = -1 is the default/fallback position (fires when no other position in the group is active — used for spring-return center, or the "no strobe active" state in MATRIX scanning). Used for: toggle switches (2-pos), rotary selectors (3+ pos), spring-return switches.

**analog** — Continuous axis. sendValue is 65535 (maps 0-3.3V to 0-65535). Used for: potentiometers, sliders. Self-calibrating — the firmware auto-ranges.

**variable_step** — Relative encoder. Two entries per encoder (one for CCW with sendValue 0, one for CW with sendValue 1). The firmware generates DCS-BIOS variable_step commands with increment/decrement values.

**fixed_step** — Like variable_step but sends fixed increment/decrement commands.

### LEDMapping.h — Output Definitions
```cpp
{ "LABEL", DEVICE_TYPE, {.info = {...}}, dimmable, activeLow }
```

| Device | Union Fields | Description |
|--------|-------------|-------------|
| DEVICE_GPIO | {gpio} | Direct GPIO LED |
| DEVICE_PCA9555 | {address, port, bit} | I2C expander LED |
| DEVICE_WS2812 | {index, pin, defR, defG, defB, defBright} | Addressable RGB LED |
| DEVICE_TM1637 | {clkPin, dioPin, segment, bit} | 7-segment display segment |
| DEVICE_GN1640T | {address, column, row} | LED matrix position |
| DEVICE_GAUGE | {gpio, minPulse, maxPulse, period} | Servo gauge (PWM) |

### Other Files
- **DisplayMapping.cpp/h** — Links DCS-BIOS display string fields to segment map entries for HT1622 LCD displays
- **SegmentMap.h** — Maps HT1622 LCD RAM addresses to physical segment positions per character
- **CustomPins.h** — Pin assignments and feature enables (ENABLE_PCA9555, ENABLE_TFT_GAUGES, HC165_LOAD_PIN, WS2812_DATA_PIN, etc.)
- **LabelSetConfig.h** — USB device name, HID settings
- **DCSBIOSBridgeData.h** — Auto-generated hash tables (DO NOT EDIT)
- **LatchedButtons.h** — Declares which momentary buttons should toggle state on each press
- **CoverGates.h** — Defines cover-guarded switch pairs
- **selected_panels.txt** — List of included aircraft panels
- **METADATA/*.json** — Custom control overrides from the DCS-BIOS JSON data

---

## PART 5: HARDWARE SUPPORT

### Supported ESP32 Boards

| Board | USB HID | WiFi | BLE | Recommendation |
|-------|---------|------|-----|----------------|
| ESP32-S3 | Yes (native) | Yes | Yes | Best all-around choice |
| ESP32-S2 | Yes (native) | Yes | No | Good budget option |
| ESP32-P4 | Yes (native) | No | No | High performance |
| ESP32 Classic | No (Serial only) | Yes | Yes | WiFi or Serial only |
| ESP32-C3/C5/C6 | No | Yes | Yes | RISC-V, compact |
| ESP32-H2 | No | No | Yes | BLE only |

### Input Hardware

**GPIO Direct** — One pin per input. Internal pull-up enabled automatically. Wire switch between GPIO and GND. ~30 usable pins per ESP32.

**PCA9555 I2C Expander** — 16 I/O pins per chip, up to 8 chips on one I2C bus = 128 inputs. Address range 0x20-0x27 (standard) or any I2C address for clones. Auto-detected from mappings — no manual panel table needed. Enable with ENABLE_PCA9555=1 in CustomPins.h. Wire: SDA, SCL, address pins.

**74HC165 Shift Register** — 8 inputs per chip, daisy-chainable to hundreds. Three shared wires: LOAD, CLK, DATA. Configure pins in CustomPins.h (HC165_LOAD_PIN, HC165_CLK_PIN, HC165_DATA_PIN, HC165_NUM_CHIPS).

**TM1637 Keypad** — 16 keys per module. Uses CLK/DIO pins of TM1637 display modules. Dual-purpose: display output + key scanning input.

**Matrix Scanning** — For rotary selector switches with many positions. Uses strobe + data pin arrays. Configure in CustomPins.h.

### Output Hardware

**GPIO LEDs** — Direct drive, 3.3V. Use 330-ohm resistor in series. ActiveLow flag inverts polarity.

**PCA9555 LEDs** — Driven via I2C. Same chips as input PCA9555 — pins can be mixed input/output.

**WS2812 RGB LEDs** — Addressable, chainable. One data pin per strip. Set per-LED color and brightness in LEDMapping.h. DATA pin configured in CustomPins.h.

**TM1637 7-Segment** — Numeric displays. Each segment is individually controllable as an LED output.

**GN1640T LED Matrix** — Grid of LEDs (like annunciator panels). Addressed by column/row.

**HT1622 Segment LCD** — Large segment LCD displays (like F/A-18 IFEI, F-16 DED). Requires SegmentMap.h for RAM-to-segment mapping. Uses the Display Pipeline.

**TFT Gauges (GC9A01, etc.)** — Round TFT displays for analog gauges. Uses LovyanGFX library. Enable with ENABLE_TFT_GAUGES=1 in CustomPins.h. Runs on a FreeRTOS task on Core 0.

**Servo Gauges** — PWM-driven servo motors for physical gauge needles. DEVICE_GAUGE in LEDMapping.h. Maps DCS-BIOS value (0-65535) to servo pulse width (minPulse to maxPulse). External 5V power required — never power from ESP32.

### Wiring Rules
- ESP32 GPIO is 3.3V — NEVER connect 5V directly to a GPIO pin
- All switch inputs use internal pull-ups — wire between GPIO and GND
- LEDs need current-limiting resistors (330 ohm typical for 3.3V)
- Potentiometers: outer pins to 3.3V and GND, wiper to analog GPIO
- I2C: SDA + SCL with 4.7K pull-up resistors to 3.3V (many breakout boards include these)
- Servos: signal wire to GPIO, power from external 5V (not ESP32)

---

## PART 6: TRANSPORT MODES

Only ONE transport can be active at a time. Set via Compiler Tool > Role / Transport.

### USB HID (Recommended for production)
- Flag: USE_DCSBIOS_USB=1
- Requires: ESP32-S2, S3, or P4 (native USB)
- PC side: Run HID Manager (python "HID Manager\HID_Manager.py")
- Latency: ~1ms (lowest)
- Pros: Most reliable, lowest latency, no network dependency
- Gotcha: USB CDC On Boot must be Disabled (Compiler Tool handles this automatically)

### WiFi UDP (Easiest to get started)
- Flag: USE_DCSBIOS_WIFI=1
- Requires: Any ESP32 with WiFi + same 2.4GHz network as PC
- PC side: Nothing extra needed (DCS-BIOS uses multicast UDP)
- Latency: ~5-10ms
- Credentials: Set via Compiler Tool > Misc Options > Wi-Fi Credentials
- Gotchas: 2.4GHz only (not 5GHz), WPA2-PSK required, ESP32 and PC must be on same subnet

### Serial/CDC
- Flag: USE_DCSBIOS_SERIAL=1
- Requires: Any ESP32, socat bridge on PC
- Latency: ~2ms
- Used for: Legacy setups or when USB HID and WiFi are not available

### Bluetooth BLE
- Flag: USE_DCSBIOS_BLUETOOTH=1
- Requires: ESP32 with BLE support
- Status: Internal/experimental

### RS485 Networking (Multi-panel overlay)
- NOT a transport to PC — it's a panel-to-panel bus
- One master (with USB/WiFi/Serial to PC) forwards DCS-BIOS data to slaves
- Up to 126 slave panels per master
- Two-wire bus (A/B), twisted pair recommended
- 120-ohm termination resistors at each end
- Master config: RS485_MASTER_ENABLED=1, set DE/RE/RX/TX pins
- Slave config: RS485_SLAVE_ENABLED=1, unique RS485_SLAVE_ADDRESS (1-126)
- RS485_SMART_MODE: Master only forwards data that slaves have subscribed to
- All RS485 settings managed by Compiler Tool > Role / Transport wizard

---

## PART 7: CONFIG.H REFERENCE

### Tool-Managed Settings (use Compiler Tool, don't edit manually)
| Setting | Default | Tool Menu Path |
|---------|---------|----------------|
| USE_DCSBIOS_USB | 1 | Role / Transport |
| USE_DCSBIOS_WIFI | 0 | Role / Transport |
| USE_DCSBIOS_SERIAL | 0 | Role / Transport |
| USE_DCSBIOS_BLUETOOTH | 0 | Role / Transport |
| RS485_MASTER_ENABLED | 0 | Role / Transport |
| RS485_SLAVE_ENABLED | 0 | Role / Transport |
| RS485_SMART_MODE | 0 | Role / Transport |
| RS485_MAX_SLAVE_ADDRESS | 127 | Role / Transport |
| RS485_SLAVE_ADDRESS | 1 | Role / Transport |
| DEBUG_ENABLED | 0 | Misc Options > Debug / Verbose Toggles |
| VERBOSE_MODE | 0 | Misc Options > Debug / Verbose Toggles |
| VERBOSE_MODE_WIFI_ONLY | 0 | Misc Options > Debug / Verbose Toggles |
| VERBOSE_MODE_SERIAL_ONLY | 0 | Misc Options > Debug / Verbose Toggles |
| DEBUG_PERFORMANCE | 0 | Misc Options > Debug / Verbose Toggles |
| MODE_DEFAULT_IS_HID | 0 | Misc Options > Advanced Settings |

### Manual-Only Settings (require editing Config.h directly)
| Setting | Default | Purpose |
|---------|---------|---------|
| POLLING_RATE_HZ | 250 | Input scan rate in Hz |
| PCA_FAST_MODE | 1 | Enable 400kHz I2C for PCA9555 |
| IS_REPLAY | 0 | Enable stream replay testing mode |
| TEST_LEDS | 0 | Flash all LEDs on startup for wiring verification |
| SCAN_WIFI_NETWORKS | 0 | Scan and list available WiFi networks on boot |
| SELECTOR_DWELL_MS | 250 | Debounce delay for selector switches (ms) |
| SERVO_UPDATE_FREQ_MS | 20 | Servo gauge update interval |
| DEBUG_ENABLED_FOR_TM1637_ONLY | 0 | Per-device debug filter |
| DEBUG_ENABLED_FOR_PCA_ONLY | 0 | Per-device debug filter |
| DEBUG_ENABLED_FOR_HC165_ONLY | 0 | Per-device debug filter |
| RS485_USE_TASK | 1 | Run RS485 on separate FreeRTOS task (best for USB/Serial/BLE; set 0 for WiFi) |
| RS485_TASK_CORE | 0 | Core for RS485 task |
| RS485 timing params | varies | SLAVE_TIMEOUT_MS, RESPONSE_TIMEOUT_MS, etc. |

---

## PART 8: TROUBLESHOOTING

### WiFi Won't Connect
1. Check SSID and password (case-sensitive) — set via Compiler Tool > Misc Options > Wi-Fi Credentials
2. Must be 2.4GHz network (ESP32 does not support 5GHz)
3. Must be WPA2-PSK (AES/CCMP)
4. ESP32 and PC must be on the same subnet
5. Run Debug Tools\CONSOLE_UDP_debug.py to see connection status
6. Enable verbose WiFi logging: Compiler Tool > Misc Options > Debug / Verbose Toggles > Verbose output over WiFi

### Button Press Does Nothing in DCS
1. Verify GPIO pin in Label Creator matches your wiring
2. Check button is wired between GPIO and GND (internal pull-up is automatic)
3. Run CONSOLE_UDP_debug.py to see if commands are being sent
4. Confirm DCS-BIOS is running inside DCS World
5. Check that the DCS-BIOS control name matches the aircraft module you're flying

### LED Does Not Light Up
1. Check LED polarity (longer leg = anode = positive)
2. Verify 330-ohm resistor is in series
3. Confirm GPIO pin in Label Creator matches wiring
4. Trigger the event in DCS to test (e.g., pull throttle to idle for Master Caution)
5. Try TEST_LEDS=1 in Config.h to flash all LEDs on startup

### Compilation Fails
1. Re-run Setup Tool > option [2] Reset to Manifest Versions
2. Make sure exactly ONE transport is enabled (Compiler Tool > Role / Transport)
3. For USB HID on S2/S3: USB CDC On Boot must be Disabled (Compiler Tool handles this)
4. Verify folder name is exactly "CockpitOS"
5. Check that Python 3.12+ is installed and in PATH

### Upload Fails
1. Try bootloader mode: hold BOOT button, plug in USB, release BOOT
2. Try a different USB cable (must be DATA cable, not charge-only)
3. Try a different USB port
4. Close any program using the COM port (Arduino IDE, serial monitor, etc.)
5. Alternative bootloader: hold BOOT, press RST, release BOOT

### ESP32 Resets / Watchdog Triggered
1. Never use delay() or blocking I/O in loop code — the watchdog resets at ~3 seconds
2. Check that servos have external power (drawing current from ESP32 can cause brownout)
3. Reduce number of WS2812 LEDs if power supply is insufficient

### PCA9555 Not Detected
1. Check I2C wiring: SDA + SCL with 4.7K pull-ups to 3.3V
2. Verify address pins match the address in InputMapping.h / LEDMapping.h
3. Ensure ENABLE_PCA9555=1 in CustomPins.h (set via Label Creator > Edit Custom Pins)
4. PCA devices are auto-detected from mappings — no manual panel table needed
5. Run I2C scanner to verify the chip responds

---

## PART 9: ADVANCED FEATURES

### Custom Panels — Complete Guide

For complex behavior beyond simple input/output mapping, write a custom panel in `src/Panels/`.

**Before writing a new panel, check if an existing one already does what you need:**

| Existing File | What It Does |
|--------------|--------------|
| `Generic.cpp` | Handles ALL InputMapping.h and LEDMapping.h entries automatically — covers GPIO, PCA9555, HC165, TM1637, MATRIX, analog axes, all LED types |
| `WingFold.cpp` | State machine + PCA9555 raw reads + command ring buffer with 500ms pacing + mechanical invariant enforcement. Guard: `HAS_CUSTOM_RIGHT` |
| `IFEIPanel.cpp` | HT1622 segment LCD with shadow RAM, overlay system, dual chips, day/NVG backlight. Guard: `HAS_IFEI` |
| `TFT_Gauges_Battery.cpp` | GC9A01 240x240 TFT gauge with dirty-rect DMA, PSRAM, FreeRTOS task, day/NVG. Guard: `HAS_RIGHT_PANEL_CONTROLLER` |
| `TFT_Gauges_CabPress.cpp` | ST77961 360x360 TFT gauge, multiple subscriptions, two needles. Guard: `HAS_ALR67` |
| `TFT_Display_CMWS.cpp` | Multi-subscription text-based tactical TFT display. Guard: `HAS_CMWS_DISPLAY` |
| `TestPanel.cpp` | Minimal template. Guard: `HAS_TEST_ONLY` |

**Required elements for every custom panel:**

1. **Compile guard** wrapping the entire file:
```cpp
#if defined(HAS_YOUR_LABEL_SET)
// ... all panel code ...
#endif
```

2. **PANEL_KIND comment** near the top (used by generate_panelkind.py to create PanelKind enum):
```cpp
// PANEL_KIND: YourPanelName
```

3. **Include Globals.h** (provides access to HIDManager, DCSBIOSBridge, CUtils):
```cpp
#include "../Globals.h"
```

4. **REGISTER_PANEL with nullptr for unused hooks** (never pass empty stub functions):
```cpp
// Standard input panel:
REGISTER_PANEL(WingFold, WingFold_init, WingFold_loop, nullptr, nullptr, nullptr, 100);

// Display panel:
REGISTER_PANEL(IFEI, IFEI_init, nullptr, IFEIDisplay_init, IFEIDisplay_loop, nullptr, 100);

// TFT gauge (display-only):
REGISTER_PANEL(TFTBatt, nullptr, nullptr, BatteryGauge_init, BatteryGauge_loop, nullptr, 100);
```

5. **Non-blocking loop** — Never use `delay()`. Use `millis()` comparisons. Watchdog resets at ~3s.
6. **Static memory only** — No `new`/`malloc` in loop paths. Allocate in init.
7. **Subscribe in init, not loop** — DCS-BIOS subscriptions are one-time registrations.
8. **Check `isMissionRunning()`** — Data is undefined between missions.

**Panel lifecycle:**
- `init()` is called when a DCS mission starts (NOT at power-on). Called every time you enter a new mission.
- `loop()` is called every poll cycle at POLLING_RATE_HZ (250Hz default).
- `disp_init()` and `disp_loop()` are the display equivalents.
- `tick()` is for optional per-frame work (rarely needed — output flushing is automatic).

### Custom Panel API — Exact Function Signatures

**Sending commands to DCS:**
```cpp
void sendDCSBIOSCommand(const char* label, uint16_t value, bool force = false);
void sendCommand(const char* label, const char* value, bool silent = false);
```

**HID button/axis control:**
```cpp
void HIDManager_setNamedButton(const char* name, bool deferSend, bool pressed);
void HIDManager_setToggleNamedButton(const char* name, bool deferSend = false);
void HIDManager_toggleIfPressed(bool isPressed, const char* label, bool deferSend = false);
void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin, HIDAxis axis,
                         bool forceSend = false, bool deferredSend = false);
void HIDManager_dispatchReport(bool force = false);
void HIDManager_commitDeferredReport(const char* deviceName);
```

**DCS-BIOS subscriptions (register in init, NEVER in loop):**
```cpp
bool subscribeToLedChange(const char* label,
    void (*callback)(const char* label, uint16_t value, uint16_t max_value));

bool subscribeToSelectorChange(const char* label,
    void (*callback)(const char* label, uint16_t value));

bool subscribeToDisplayChange(const char* label,
    void (*callback)(const char* label, const char* value));

bool subscribeToMetadataChange(const char* label,
    void (*callback)(const char* label, uint16_t value));
```

**Hardware read functions:**
```cpp
bool readPCA9555(uint8_t addr, byte &p0, byte &p1);  // Read both ports in one call
uint64_t HC165_read();                                 // Read entire shift register chain
bool isMissionRunning();                               // Is a DCS mission active?
bool shouldPollMs(unsigned long &lastPoll);             // Rate limiter (250Hz)
void debugPrintf(const char* format, ...);              // Debug output (WiFi or Serial)
```

**State query macros:**
```cpp
#define isCoverOpen(label)      (findCmdEntry(label) ? (findCmdEntry(label)->lastValue > 0) : false)
#define isToggleOn(label)       isCoverOpen(label)
```

### MATRIX Scanning — How It Actually Works

CockpitOS MATRIX is designed for **multi-position rotary selector switches** (NOT keypads). It uses GPIO pins directly — strobe pins as outputs, data pin as input with pull-up.

**Scan algorithm (runs at 250Hz):**
1. Each strobe pin is driven LOW one at a time (1 microsecond settle delay)
2. The data pin (connected to rotary wiper) is read after each strobe
3. If data reads LOW, the switch wiper is on that strobe's contact
4. The resulting bit pattern (which strobes saw data LOW) identifies the position
5. If no strobe reads LOW, the fallback position fires

**Anchor vs Position rows in InputMapping.h:**

| Row Type | `port` field | `bit` field | How firmware identifies it |
|----------|-------------|-------------|--------------------------|
| **Anchor** | Data pin GPIO number | Multi-bit mask (e.g., `15` = 0b1111 for 4 strobes) | `popcount(bit) > 1` |
| **Position** | Strobe pin GPIO number | One-hot index (`1`, `2`, `4`, `8`, ...) | Exactly one bit set |
| **Fallback** | Data pin GPIO number | `-1` | Negative bit value |

**One-hot bit encoding:** bit=1 means strobe[0], bit=2 means strobe[1], bit=4 means strobe[2], bit=8 means strobe[3], etc.

**Complete worked example (ALR67 RWR display type selector, 5 positions, 4 strobes):**

CustomPins.h:
```cpp
#define ALR67_STROBE_1  PIN(16)   // Strobe index 0
#define ALR67_STROBE_2  PIN(17)   // Strobe index 1
#define ALR67_STROBE_3  PIN(21)   // Strobe index 2
#define ALR67_STROBE_4  PIN(37)   // Strobe index 3
#define ALR67_DataPin   PIN(38)   // Common data pin (wiper)
```

InputMapping.h:
```cpp
// ANCHOR: port=DataPin GPIO, bit=15 (0b1111 = 4 strobes). Also the fallback position.
{ "RWR_DIS_TYPE_SW_F", "MATRIX", ALR67_DataPin,   15, -1, "RWR_DIS_TYPE_SW", 4, "selector", 6 },
// POSITIONS: port=strobe GPIO, bit=one-hot strobe index
{ "RWR_DIS_TYPE_SW_U", "MATRIX", ALR67_STROBE_1,   1, -1, "RWR_DIS_TYPE_SW", 3, "selector", 6 },
{ "RWR_DIS_TYPE_SW_A", "MATRIX", ALR67_STROBE_2,   2, -1, "RWR_DIS_TYPE_SW", 2, "selector", 6 },
{ "RWR_DIS_TYPE_SW_I", "MATRIX", ALR67_STROBE_3,   4, -1, "RWR_DIS_TYPE_SW", 1, "selector", 6 },
{ "RWR_DIS_TYPE_SW_N", "MATRIX", ALR67_STROBE_4,   8, -1, "RWR_DIS_TYPE_SW", 0, "selector", 6 },
```

All rows share the same dcsCommand and group. The anchor row (bit=15) is the fallback — fires when no strobe is active.

**12-position scaling (4 strobes x 3 data lines):** Use 3 separate rotary groups (one per data line), each with 4 positions + 1 anchor. That's 12 positions from 7 GPIO pins.

**Limits:** MAX_MATRIX_ROTARIES=8, MAX_MATRIX_STROBES=8 per rotary, MAX_MATRIX_POS=16 per rotary.

### CoverGate System

Physical switch covers that guard important switches. Single button press: opens virtual cover, waits, fires switch, waits, closes cover.

Defined in CoverGates.h via Label Creator CoverGate Editor:
```cpp
static const CoverGateDef kCoverGates[] = {
  { "action_label", "release_label", "cover_label", CoverGateKind::Selector, 200, 300 },
};
```

**CoverGateKind values:** `Selector` (2-pos arm/safe), `ButtonMomentary` (latched momentary under cover), `ButtonLatched` (reserved).

**State machine:** P_NONE (idle) -> P_SEND_ON (action pending with delay_ms timer) -> P_CLOSE_COVER (cover close pending with close_delay_ms timer) -> P_NONE.

**CoverGate + Latched Button interaction:** When a latched button is also CoverGate-guarded, CoverGate handles the entire sequence. It calls `HIDManager_setToggleNamedButton()` directly with a reentry flag to prevent infinite recursion. The `cg_isLatchedOn()` function checks command history to prevent double-fire on rapid presses.

**Timing guidelines:** Fast switch (Master Arm): delay_ms=100-200, close_delay_ms=200-300. Safety-critical: 200-300/400-500. Animation-heavy: 300-500/500-800.

**Limit:** MAX_COVER_GATES = 16.

### Latched Buttons

Makes a momentary push button behave as a toggle. First press sends ON, second press sends OFF. Rising-edge only — release is ignored.

Defined in LatchedButtons.h via Label Creator Latched Buttons Editor:
```cpp
static const char* kLatchedButtons[] = { "FIRE_EXT_BTN", "MASTER_ARM_SW" };
```

`isLatchedButton()` in Mappings.cpp checks this list. `HIDManager_toggleIfPressed()` detects rising edge and calls `HIDManager_setToggleNamedButton()` to flip state. Command history (`CommandHistoryEntry.lastValue`) tracks ON/OFF: 0xFFFF or 0 = OFF, >0 = ON.

**Limit:** MAX_LATCHED_BUTTONS = 16.

### Display Pipeline (HT1622 Segment LCD)

For large segment LCD displays (like F/A-18 IFEI):

1. DCS-BIOS sends display string data at ~30Hz
2. `onConsistentData()` fires at end-of-frame, compares buffers, calls `onDisplayChange(label, value)` for changed fields
3. DisplayMapping.cpp maps string fields to SegmentMap entries via `fieldDefs[]` array
4. Render function writes segment patterns into shadow RAM (`ramShadow[chip][64]`)
5. `commit()` compares shadow vs last-committed state, writes only changed bytes to HT1622 via SPI

The Segment Map Editor in Label Creator provides a RAM Walker for discovering which HT1622 RAM bits control which physical LCD segments.

**HT1622 class API:**
```cpp
HT1622 chip(CS_PIN, WR_PIN, DATA_PIN);
chip.init();
chip.commit(ramShadow, lastShadow, 64);
chip.commitPartial(ramShadow, lastShadow, addrStart, addrEnd);
```

### TFT Gauges

Round TFT displays for analog gauge faces:
- Enable ENABLE_TFT_GAUGES=1 in CustomPins.h (via Label Creator > Edit Custom Pins)
- Requires ESP32-S3 or S2 with PSRAM
- Uses LovyanGFX library for rendering
- Each gauge runs in its own FreeRTOS task on Core 0 (separate from main loop on Core 1)
- Dirty-rect DMA rendering: only redraws the region where the needle moved
- Day/NVG lighting modes switch automatically based on CONSOLES_DIMMER DCS-BIOS value

**Supported TFT controllers:** GC9A01 (240x240 round), ST77916 (360x360 QSPI), ST77961 (360x360 SPI), ST7789 (240x320 rect), ILI9341 (240x320 rect/CYD)

**Performance:** 30-60 FPS, 5-15ms render per frame, 200-500KB PSRAM per gauge

### RS485 Networking — Deep Dive

**Architecture:** One master (connects to DCS-BIOS via USB/WiFi/Serial) forwards data to slaves over half-duplex RS485 bus. Master polls slaves round-robin for input commands.

**Smart Mode vs Relay Mode:**
- `RS485_SMART_MODE=0` (Relay): Master forwards entire DCS-BIOS stream. Simple, more bandwidth.
- `RS485_SMART_MODE=1` (Smart): Master filters stream, only forwards addresses slaves subscribe to. 100-1000x bandwidth reduction.

**Key optimization: RS485_MAX_SLAVE_ADDRESS** — Default 127. If you have 3 slaves with addresses 1-3, set this to 3. Eliminates 124 wasted poll cycles per round.

**RS485_USE_TASK** — Must be 1 (dedicated FreeRTOS task) for USB/Serial/BLE transport on master. Set to 0 only for WiFi transport.

**FreeRTOS task priorities (dual-core):**
| Task | Core | Priority |
|------|------|----------|
| Arduino loop() | 1 | 1 |
| TFT Gauge tasks | 0 | 2 |
| RS485 Slave | 0 | 5 |
| RS485 Master | 1 | 24 |

**Slave discovery:** Master periodically probes unknown addresses. New slaves discovered within seconds. When a poll times out (1ms), slave is marked offline.

**Wiring:** Twisted pair for A/B, 120-ohm termination at both physical ends only, common ground between all nodes. Do NOT swap A and B — nothing works if one device is swapped.

### DCS-BIOS Protocol

- **Export (DCS -> Panel):** Binary frames on UDP multicast 239.255.50.10:5010. Address/value pairs representing cockpit state. ~30Hz frame rate.
- **Import (Panel -> DCS):** Text commands on UDP port 7778. Format: `"CONTROL_NAME VALUE\n"`
- Subscription system: panels subscribe to specific addresses, callbacks fire on change
- Hash tables in DCSBIOSBridgeData.h provide O(1) control name lookup
- Subscription limits: MAX_LED_SUBSCRIPTIONS=32, MAX_SELECTOR_SUBSCRIPTIONS=32, MAX_DISPLAY_SUBSCRIPTIONS=32, MAX_METADATA_SUBSCRIPTIONS=32

---

## PART 10: PROJECT STRUCTURE

```
CockpitOS/
├── CockpitOS.ino           <- Arduino sketch entry point
├── Config.h                <- Master config (transport, debug, timing)
├── Setup-START.py          <- Setup Tool entry point
├── CockpitOS-START.py      <- Compiler Tool entry point
├── LabelCreator-START.py   <- Label Creator entry point
├── src/
│   ├── LABELS/
│   │   ├── active_set.h    <- Points to the active label set
│   │   └── LABEL_SET_*/    <- One folder per label set
│   ├── Core/               <- Core firmware (InputControl, LEDControl, CoverGate, PanelRegistry)
│   └── Panels/             <- Panel implementations (Generic.cpp + custom panels)
├── compiler/               <- Compiler Tool source
│   ├── cockpitos.py        <- Main compiler module
│   └── arduino-cli/        <- Bundled arduino-cli binary
├── label_creator/          <- Label Creator source
│   ├── label_creator.py    <- Main orchestrator
│   ├── ui.py               <- TUI toolkit
│   ├── input_editor.py     <- InputMapping.h editor
│   ├── led_editor.py       <- LEDMapping.h editor
│   ├── display_editor.py   <- DisplayMapping.cpp editor
│   ├── segment_map_editor.py <- SegmentMap editor
│   ├── custompins_editor.py <- CustomPins.h editor
│   ├── latched_editor.py   <- LatchedButtons.h editor
│   └── covergate_editor.py <- CoverGates.h editor
├── lib/CUtils/             <- Hardware abstraction library
├── HID Manager/            <- USB HID bridge (PC-side Python app)
├── Debug Tools/            <- UDP console, stream recorder/player
└── Docs/                   <- Structured documentation
```

---

## PART 11: COMMON USER SCENARIOS

### "I want to add a button"
1. Wire button between a GPIO pin and GND
2. Open Label Creator > Modify label set > Edit Inputs
3. Find the DCS-BIOS control for your button
4. Set Source = GPIO, Port = your pin number
5. Recompile and upload via Compiler Tool

### "I want to add an LED"
1. Wire LED with 330-ohm resistor between GPIO and GND
2. Open Label Creator > Modify label set > Edit Outputs (LEDs)
3. Find the DCS-BIOS indicator
4. Set Device = GPIO, Port = your pin number
5. Recompile and upload

### "I want to add a 3-position toggle switch"
1. Wire center pin to GPIO, one side to GND (other side floats or to another GPIO for 3-pos)
2. Open Label Creator > Edit Inputs
3. Find ALL entries for that selector (they share the same dcsCommand and group number)
4. Set Source = GPIO, Port = same pin for all positions in the group
5. Each position has a different sendValue
6. Recompile and upload

### "I want to add a rotary encoder"
1. Wire CLK to one GPIO, DT to another GPIO. Common to GND
2. Open Label Creator > Edit Inputs
3. Find the CCW and CW entries (they use variable_step or fixed_step)
4. Set Source = GPIO for both. CCW port = CLK pin, CW port = DT pin
5. Recompile and upload

### "I want to add a potentiometer"
1. Wire outer pins to 3.3V and GND. Center wiper to analog GPIO
2. Open Label Creator > Edit Inputs
3. Find the analog control
4. Set Source = GPIO, Port = your analog pin
5. Control type is "analog", sendValue should be 65535
6. Recompile and upload
7. CockpitOS auto-calibrates — just turn the pot full range on first use

### "I want to add WS2812 RGB LEDs"
1. Wire data pin from ESP32 to first LED's DIN. Power LEDs from external 5V
2. Open Label Creator > Edit Custom Pins > set WS2812_DATA_PIN
3. Open Label Creator > Edit Outputs (LEDs)
4. For each indicator: Set Device = WS2812, set led index, color (R,G,B), brightness
5. Recompile and upload

### "I want to add a servo gauge"
1. Wire servo signal to GPIO pin. Power servo from external 5V (NOT from ESP32)
2. Open Label Creator > Edit Outputs (LEDs)
3. Find the DCS-BIOS gauge value
4. Set Device = GAUGE, GPIO pin, minPulse (e.g., 544), maxPulse (e.g., 2400), period (20000)
5. Recompile and upload

### "I want to use PCA9555 expanders for more I/O"
1. Wire PCA9555 to I2C bus (SDA, SCL, 4.7K pull-ups to 3.3V)
2. Set address pins for unique address (0x20-0x27)
3. Open Label Creator > Edit Custom Pins > set ENABLE_PCA9555=1, SDA_PIN, SCL_PIN
4. Open Label Creator > Edit Inputs or Edit Outputs (LEDs)
5. Set Source = PCA_0x20 (or whatever address), Port = 0 or 1, Bit = 0-7
6. PCA devices are auto-detected from mappings
7. Recompile and upload

### "I want to switch from WiFi to USB"
1. Open Compiler Tool > Role / Transport
2. Select "No" for RS485 Master
3. Select USB transport
4. Recompile and upload
5. Run HID Manager on PC: python "HID Manager\HID_Manager.py"

### "I want to set up RS485 multi-panel"
1. Master panel: Compiler Tool > Role / Transport > Yes for RS485 Master > pick transport to PC (USB/WiFi)
2. Set RS485 pins (DE, RE, RX, TX)
3. Each slave: Compiler Tool > Role / Transport > RS485 Slave > set unique address (1-126)
4. Wire RS485 bus: A to A, B to B on all panels. Twisted pair recommended
5. 120-ohm termination resistors at first and last device on bus
6. Each panel has its own label set — compile and upload each separately

### "I want to support multiple aircraft"
1. Create one label set per aircraft in Label Creator
2. Switch between label sets using the Compiler Tool (it sets active_set.h)
3. Recompile and upload when switching aircraft
4. Alternative: include panels from multiple aircraft in one label set (controls that don't exist in the current aircraft are simply ignored)

---

## PART 12: DEBUG TOOLS

### WiFi Debug Console
```
python "Debug Tools\CONSOLE_UDP_debug.py"
```
Shows WiFi connection status, incoming DCS-BIOS data, and outgoing commands. Essential for WiFi troubleshooting.

### DCS-BIOS Stream Recorder/Player
```
python "Debug Tools\RECORD_DCS_stream.py"   # Record live DCS-BIOS data
python "Debug Tools\PLAY_DCS_stream.py"      # Replay recorded data (no DCS needed)
```
Useful for testing without DCS running. Set IS_REPLAY=1 in Config.h to enable replay mode.

### HID Manager
```
python "HID Manager\HID_Manager.py"
```
Required for USB HID transport. Auto-detects CockpitOS USB devices and bridges HID reports to/from DCS-BIOS UDP.

### Compiler Tool Debug Toggles
Accessible via Compiler Tool > Misc Options > Debug / Verbose Toggles:
- Extended debug info (DEBUG_ENABLED)
- Verbose output over WiFi (VERBOSE_MODE_WIFI_ONLY)
- Verbose output over Serial (VERBOSE_MODE_SERIAL_ONLY)
- Performance profiling (DEBUG_PERFORMANCE)

All debug flags should be 0 for production — debug output adds latency.

---

## PART 13: FREQUENTLY ASKED QUESTIONS

**Q: Do I need Arduino IDE?**
A: No. The three Python tools handle everything. The Setup Tool installs a bundled arduino-cli, the Compiler Tool compiles and uploads. Arduino IDE is optional for advanced users who want to edit firmware code directly.

**Q: Which ESP32 should I buy?**
A: ESP32-S3 for best all-around (USB HID + WiFi + BLE). ESP32-S2 for budget USB HID. ESP32-P4 for max performance. Classic ESP32 if you only need WiFi.

**Q: Can I mix input types on the same panel?**
A: Yes. A single label set can use GPIO, PCA9555, HC165, TM1637, and MATRIX inputs simultaneously. Same for outputs — mix GPIO LEDs, WS2812, servos, TM1637 displays freely.

**Q: How many inputs/outputs can I have?**
A: Depends on hardware. GPIO alone gives ~30 pins. Add PCA9555 for +128 I/O per I2C bus. Add HC165 for hundreds of inputs. WS2812 supports long chains. Practical limit is usually physical wiring, not firmware.

**Q: Can I use the same GPIO pin for input and output?**
A: No. Each GPIO pin is either an input OR an output.

**Q: What happens if I select panels I haven't wired?**
A: Nothing bad. Unmapped controls (Source = NONE) are ignored at runtime. No performance cost.

**Q: Can I add my own C++ panel code?**
A: Yes. Use REGISTER_PANEL() macro in src/Panels/. See Advanced/Custom-Panels.md. Most users never need this — the generic panel handles everything in InputMapping.h and LEDMapping.h.

**Q: How do I update CockpitOS?**
A: Download the new release, extract over existing folder (keep your src/LABELS/ folder!). Run Setup Tool if the required ESP32 core version changed. Recompile.

**Q: My ESP32 keeps resetting every few seconds**
A: This is the watchdog timer. Something is blocking the main loop for more than ~3 seconds. Check for delay() calls, blocking I/O, or insufficient power supply (brownout reset).

**Q: How do I find DCS-BIOS control names?**
A: Use the DCS-BIOS control reference page in your browser (served locally by DCS-BIOS when DCS is running). Or check the aircraft JSON files that ship with CockpitOS.

---

## PART 14: CRITICAL RULES FOR THE ASSISTANT

1. **Always direct users to the Python tools** for any configuration task. Never tell users to manually edit Config.h for settings the Compiler Tool manages. Never tell users to manually edit InputMapping.h or LEDMapping.h — the Label Creator editors handle these.

2. **Only ONE transport mode** can be active. The Compiler Tool enforces this.

3. **Label sets are self-contained.** All panel configuration lives in the label set folder. Core firmware (src/) should rarely need modification.

4. **Never tell users to run generator scripts directly.** The Label Creator handles generation internally. Running generate_data.py or reset_data.py manually can destroy hand-tuned configurations.

5. **USB HID requires S2, S3, or P4.** Classic ESP32, C3, C6, and H2 do NOT support native USB HID.

6. **3.3V GPIO only.** Never connect 5V to an ESP32 GPIO pin.

7. **Servos need external power.** Always mention this when discussing servo gauges.

8. **USB data cable.** When users have connection problems, always ask if they're using a DATA cable (not charge-only).

9. **DCS-BIOS must be installed separately.** CockpitOS does not include DCS-BIOS — users must install it from https://github.com/DCS-Skunkworks/dcs-bios/releases

10. **Folder must be named CockpitOS.** Arduino requires the folder name to match the .ino filename.

11. **Be helpful to ALL skill levels.** Give step-by-step guidance to beginners. Give API details and code patterns to advanced users. Ask clarifying questions when the user's skill level is unclear.

12. **When unsure, recommend the documentation.** Point users to the specific doc: Getting-Started for setup, Hardware guides for wiring, Tools docs for tool usage, Advanced for custom panel code.
