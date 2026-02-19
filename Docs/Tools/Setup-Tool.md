# Setup Tool

The Setup Tool installs and manages the ESP32 Arduino core and required libraries using the bundled `arduino-cli`. No Arduino IDE installation is needed.

**Entry point:** `Setup-START.py`

## How to Run

Double-click `Setup-START.py` in the CockpitOS root folder, or run from a terminal:

```
python Setup-START.py
```

The tool opens a terminal UI showing the current status of all components and an arrow-key menu for actions.

## What It Installs

The Setup Tool manages two components, installing them into **standard Arduino locations** so they are shared with any Arduino IDE you might also have installed:

| Component | Version | Install Location |
|-----------|---------|-----------------|
| ESP32 Arduino Core (`esp32:esp32`) | 3.3.6 | `%LOCALAPPDATA%/Arduino15/` |
| LovyanGFX library | 1.2.19 | `~/Documents/Arduino/libraries/` |

Both components are required to compile CockpitOS firmware. The ESP32 core provides the board definitions and toolchains for all supported ESP32 variants. LovyanGFX provides TFT display support.

## The MANIFEST Dictionary

The source of truth for recommended versions lives in the `MANIFEST` dictionary at the top of `Setup-START.py`:

```python
MANIFEST = {
    "esp32_core": {
        "platform": "esp32:esp32",
        "version": "3.3.6",
        "board_manager_url": "https://raw.githubusercontent.com/espressif/...",
    },
    "lovyangfx": {
        "library": "LovyanGFX",
        "version": "1.2.19",
    },
}
```

When CockpitOS updates its dependencies, this dictionary is the single place that changes.

## Menu Options

### Install / Update Environment

This is the main action. It walks through three steps:

1. **Verify arduino-cli** -- Confirms the bundled CLI binary exists and runs correctly.
2. **ESP32 Arduino Core** -- Checks if the core is installed. If missing, offers to install. If outdated (older than the MANIFEST version), offers to update.
3. **LovyanGFX Library** -- Same logic: install if missing, update if outdated.

Each step asks for confirmation before making changes. The ESP32 core download is approximately 350 MB, so the first install takes a few minutes depending on your internet connection.

### Reset to Recommended Versions

Forces the exact versions specified in the MANIFEST dictionary, regardless of what is currently installed. This is useful if:

- You accidentally upgraded to a newer version that causes issues
- You want to match the exact versions that CockpitOS was tested against
- Something went wrong and you want a clean slate

This option warns you before proceeding, since it may downgrade components.

### Switch Tool

Jump directly to the Compiler Tool or Label Creator. The Setup Tool replaces itself with the selected tool in the same console window.

## Status Display

On launch, the tool shows the current state of all components:

```
     arduino-cli v1.4.1  (bundled)
     ESP32 Core: v3.3.6
     LovyanGFX: v1.2.19
```

Components are color-coded:
- **Cyan** -- Installed and up to date
- **Yellow** -- Installed but outdated (update available)
- **Red** -- Not installed

## Bundled arduino-cli

CockpitOS bundles `arduino-cli` v1.4.1 at:

```
compiler/arduino-cli/arduino-cli.exe
```

This bundled binary is used by both the Setup Tool and the [Compiler Tool](Compiler-Tool.md). You do not need to install `arduino-cli` separately.

## Requirements

| Requirement | Details |
|-------------|---------|
| OS | Windows 10 or 11 |
| Python | 3.12 or newer |
| Internet | Required for downloading components |
| Disk space | ~500 MB for the ESP32 core and toolchains |

## After Setup

Once the setup completes successfully, the tool displays a success banner and offers to launch the Compiler Tool. You can also switch to the Compiler Tool or Label Creator from the menu at any time.

If any step failed or was skipped, the tool shows a warning banner. You can re-run setup to retry, or install the components manually through the Arduino IDE.
