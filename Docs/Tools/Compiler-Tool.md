# Compiler Tool

The Compiler Tool is the central build system for CockpitOS. It compiles and uploads firmware to your ESP32 board without requiring the Arduino IDE.

**Entry point:** `CockpitOS-START.py`

## How to Run

Double-click `CockpitOS-START.py` in the CockpitOS root folder, or run from a terminal:

```
python CockpitOS-START.py
```

The tool opens a terminal UI with a status display, configuration summary, and an action bar at the bottom for compile/upload operations.

## Main Screen

The main screen shows your current configuration at a glance:

```
        ____            _          _ _    ___  ____
       / ___|___   ___ | | ___ __ (_) |_ / _ \/ ___|
      | |   / _ \ / __|| |/ / '_ \| | __| | | \___ \
      | |__| (_) | (__ |   <| |_) | | |_| |_| |___) |
       \____\___/ \___|_|\_\ .__/|_|\__|\___/|____/
                            |_|
              Compile Tool

     ESP32-S3 Dev Module (esp32:esp32:esp32s3)
     Standalone  ·  WiFi
     Debug is DISABLED (see Misc Options to enable)
```

Below the status, the menu and action bar are shown.

## Menu Options

### Board / Options

Select your ESP32 board variant and configure board-specific options:

| Supported Boards |
|-----------------|
| ESP32 (Classic) |
| ESP32-S2 |
| ESP32-S3 |
| ESP32-C3 |
| ESP32-C5 |
| ESP32-C6 |
| ESP32-H2 |
| ESP32-P4 |

After selecting a board, the tool presents configurable options like partition scheme, flash size, flash mode, USB mode, CPU frequency, and PSRAM. Dual-USB boards (S2, S3, P4) automatically show USB-OTG and Hardware CDC options.

### Role / Transport

Configure the device role and communication transport:

- **Role:** Standalone, RS485 Master, or RS485 Slave
- **Transport:** WiFi, USB Native, Serial (CDC/Socat), Bluetooth BLE, or RS485 Slave

The tool walks you through a two-step wizard: first the role (master or not), then the transport. For RS485 slaves, it prompts for the slave address (1-126). For RS485 masters, smart mode and max slave address are configurable.

The tool writes all transport flags to `Config.h` automatically, performs cross-validation against the selected board (for example, USB transport requires an S2/S3/P4 board), and auto-configures USB Mode on dual-USB boards.

### Misc Options

A sub-menu with four items:

**Wi-Fi Credentials** -- Set the SSID and password for WiFi transport. Saved to `.credentials/wifi.h`. Supports 2.4 GHz WPA2-PSK only.

**Debug / Verbose Toggles** -- Enable or disable debug flags in `Config.h`:
- Verbose output over WiFi
- Verbose output over Serial
- Extended debug info
- Performance profiling

**Advanced Settings** -- Toggle HID mode as default, show detailed compiler warnings.

**Clear cache/build** -- Wipes the build directory to force a full recompile on the next build. Useful when switching boards or after changing `Config.h` settings that the incremental build might not pick up.

## Action Bar

The action bar at the bottom of the screen provides the primary build operations:

| Action | Description |
|--------|-------------|
| **Compile** | Full compile flow: select label set, run `generate_data.py`, compile |
| **Quick Compile** | Recompile using the current label set, skipping label selection and generation. Only available when a label set is already active. |
| **Upload** | Flash the last successful build to the connected board |

### Compile Flow

The full compile flow works like this:

1. **Select label set** -- Pick which label set to build (arrow-key menu with all available sets)
2. **Pre-compile validation** -- Cross-checks Config.h settings against the selected board. Fatal issues block the build; warnings let you continue.
3. **Run generate_data.py** -- Regenerates all mapping files from the label set definition
4. **Compile** -- Invokes `arduino-cli compile` with your board and options
5. **Post-compile** -- On success, offers to upload immediately (press Enter) or return to menu (press Esc)

### Compilation Output

During compilation, the tool shows real-time output from `arduino-cli`. Errors and warnings are highlighted:

- Errors are shown in red with clear context
- Warnings are shown in yellow
- The "detailed warnings" toggle in Advanced Settings controls whether all compiler warnings are displayed or just the summary

### Upload

Upload uses `arduino-cli upload` with the configured board FQBN and options. The tool auto-detects available COM ports.

## Settings Persistence

The Compiler Tool saves your preferences to `compiler/compiler_prefs.json`:

- Selected board FQBN and friendly name
- Board options (partition scheme, flash size, etc.)
- Arduino CLI path
- Show detailed warnings toggle

These settings persist between sessions, so you only need to configure your board once.

## The Bundled arduino-cli

The Compiler Tool uses the same bundled `arduino-cli` binary as the [Setup Tool](Setup-Tool.md):

```
compiler/arduino-cli/arduino-cli.exe
```

On first run, the tool automatically searches for `arduino-cli` in several locations:
1. The bundled path above
2. Cached path from a previous session
3. Windows registry (Arduino IDE install)
4. Common install directories
5. System PATH

If none are found, it prompts you to enter the Arduino IDE folder path manually.

## Config.h

The `Config.h` file in the CockpitOS root controls firmware behavior. The Compiler Tool manages the most commonly changed settings through its menus:

- **Transport mode and RS485 settings** -- via Role / Transport
- **WiFi credentials** -- via Misc Options > Wi-Fi Credentials
- **Debug and verbose flags** -- via Misc Options > Debug / Verbose Toggles
- **HID mode default** -- via Misc Options > Advanced Settings

For rarely changed settings (timing parameters, buffer sizes, axis thresholds, hardware debug flags), see the [Config.h Reference](../Reference/Config.md).

## Single-Instance Guard

Only one instance of the Compiler Tool can run at a time. If you try to launch a second copy, it brings the existing window to the foreground instead.

## Tool Switching

From the menu, you can switch directly to:

- **Label Creator** -- To create or modify label sets
- **Environment Setup** -- To install or update the ESP32 core and libraries

The current tool replaces itself seamlessly in the same console window.
