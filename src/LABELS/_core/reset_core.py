#!/usr/bin/env python3
"""
CockpitOS Reset Data - Core Module
===================================
Called from LABEL_SET_XXX/reset_data.py wrappers.
CWD is set by the wrapper BEFORE this module is imported.
"""
import os, glob
from aircraft_selector import select_aircraft

# When launched from the Label Creator tool, skip interactive confirmations.
_BATCH = os.environ.get("COCKPITOS_BATCH") == "1"

# -----------------------------
# EDIT THIS TABLE AS NEEDED!
# List all auto-generated files (edit as you add/remove)
AUTOGEN_FILES = [
    "DCSBIOSBridgeData.h",
    "InputMapping.h",
    "LEDMapping.h",
    "DisplayMapping.cpp",
    "DisplayMapping.cpp.DISABLED",
    "DisplayMapping.h",
    "CT_Display.cpp",
    "CT_Display.cpp.DISABLED",
    "CT_Display.h",
    "panels.txt",
    "selected_panels.txt",
    "LabelSetConfig.h",
]

# Template for CustomPins.h (blanked or newly created)
# Comprehensive template with ALL known pin categories from existing label sets.
CUSTOM_PINS_TEMPLATE = """\
// CustomPins.h - Label-set-specific pin definitions for CockpitOS
//
// This file is automatically included by LabelSetConfig.h if present.
// Uncomment and set ONLY the pins your hardware actually uses.
// The PIN(X) macro converts S2 pins to S3 equivalents automatically.
//
// ============================================================================

#pragma once

// --- Feature Enables --------------------------------------------------------
#define ENABLE_TFT_GAUGES                         0  // 1 = Enable TFT gauge rendering (only if TFT displays are present)
#define ENABLE_PCA9555                            0  // 1 = Enable PCA9555 I/O expander logic (only if PCA chips are present)

// --- I2C Pins ---------------------------------------------------------------
// Used by PCA9555 expanders, OLED displays, or any I2C peripherals.
// #define SDA_PIN                                PIN(8)
// #define SCL_PIN                                PIN(9)

// --- HC165 Shift Register ---------------------------------------------------
// For reading banks of physical switches via daisy-chained 74HC165 chips.
// HC165_BITS = total number of inputs across all chained chips (8 per chip).
// Set to 0 or leave commented to disable.
// #define HC165_BITS                             16
// #define HC165_CONTROLLER_PL                    PIN(39)  // Latch (PL)
// #define HC165_CONTROLLER_CP                    PIN(38)  // Clock (CP)
// #define HC165_CONTROLLER_QH                    PIN(40)  // Data  (QH)

// --- WS2812B Addressable LED Strip ------------------------------------------
// #define WS2812B_PIN                            PIN(35)

// --- Mode Switch ------------------------------------------------------------
// HID selector switch for mode control (e.g., rotary or toggle).
// #define MODE_SWITCH_PIN                        PIN(33)

// --- TM1637 / HT16K33 7-Segment Displays -----------------------------------
// Pairs of DIO + CLK pins for each 7-segment display module.
// #define LA_DIO_PIN                             PIN(39)  // Left Angle display DIO
// #define LA_CLK_PIN                             PIN(37)  // Left Angle display CLK
// #define RA_DIO_PIN                             PIN(40)  // Right Angle display DIO
// #define RA_CLK_PIN                             PIN(37)  // Right Angle display CLK
// #define CA_DIO_PIN                             PIN(36)  // Caution Advisory display DIO
// #define CA_CLK_PIN                             PIN(37)  // Caution Advisory display CLK
// #define JETT_DIO_PIN                           PIN(8)   // Jettison Select display DIO
// #define JETT_CLK_PIN                           PIN(9)   // Jettison Select display CLK

// --- TFT Gauge Display Pins ------------------------------------------------
// Each TFT gauge needs: CS, DC, MOSI, SCLK (required) + RST, MISO (optional).
// Use -1 for unused RST/MISO pins.
//
// Example (Cabin Pressure gauge):
// #define CABIN_PRESSURE_CS_PIN                  PIN(36)
// #define CABIN_PRESSURE_DC_PIN                  PIN(18)
// #define CABIN_PRESSURE_MOSI_PIN                PIN(39)
// #define CABIN_PRESSURE_SCLK_PIN                PIN(40)
// #define CABIN_PRESSURE_RST_PIN                 -1       // Not connected
// #define CABIN_PRESSURE_MISO_PIN                -1       // Not connected
//
// Example (Brake Pressure gauge):
// #define BRAKE_PRESSURE_CS_PIN                  PIN(5)
// #define BRAKE_PRESSURE_DC_PIN                  PIN(7)
// #define BRAKE_PRESSURE_MOSI_PIN                PIN(10)
// #define BRAKE_PRESSURE_SCLK_PIN                PIN(11)
// #define BRAKE_PRESSURE_RST_PIN                 -1
// #define BRAKE_PRESSURE_MISO_PIN                -1
//
// Example (Hydraulic Pressure gauge):
// #define HYD_PRESSURE_CS_PIN                    PIN(17)
// #define HYD_PRESSURE_DC_PIN                    PIN(3)
// #define HYD_PRESSURE_MOSI_PIN                  PIN(4)
// #define HYD_PRESSURE_SCLK_PIN                  PIN(12)
// #define HYD_PRESSURE_RST_PIN                   -1
// #define HYD_PRESSURE_MISO_PIN                  -1
//
// Example (Radar Altimeter gauge):
// #define RADARALT_CS_PIN                        PIN(10)
// #define RADARALT_DC_PIN                        PIN(11)
// #define RADARALT_MOSI_PIN                      PIN(13)
// #define RADARALT_SCLK_PIN                      PIN(14)
// #define RADARALT_RST_PIN                       -1
// #define RADARALT_MISO_PIN                      -1
//
// Example (Battery gauge):
// #define BATTERY_CS_PIN                         PIN(36)
// #define BATTERY_DC_PIN                         PIN(13)
// #define BATTERY_MOSI_PIN                       PIN(8)
// #define BATTERY_SCLK_PIN                       PIN(9)
// #define BATTERY_RST_PIN                        PIN(12)

// --- IFEI LCD Panel Pins ----------------------------------------------------
// Parallel-interface LCD pairs (left + right) for the IFEI display.
// #define DATA0_PIN                              PIN(34)  // Left LCD data
// #define WR0_PIN                                PIN(35)  // Left LCD write enable
// #define CS0_PIN                                PIN(36)  // Left LCD chip select
// #define DATA1_PIN                              PIN(18)  // Right LCD data
// #define WR1_PIN                                PIN(21)  // Right LCD write enable
// #define CS1_PIN                                PIN(33)  // Right LCD chip select

// --- IFEI Backlight Pins ----------------------------------------------------
// #define BL_GREEN_PIN                           PIN(1)   // Day mode backlight
// #define BL_WHITE_PIN                           PIN(2)   // White backlight
// #define BL_NVG_PIN                             PIN(4)   // NVG mode backlight

// --- ALR-67 Countermeasures Panel -------------------------------------------
// #define ALR67_STROBE_1                         PIN(16)
// #define ALR67_STROBE_2                         PIN(17)
// #define ALR67_STROBE_3                         PIN(21)
// #define ALR67_STROBE_4                         PIN(37)
// #define ALR67_DataPin                          PIN(38)
// #define ALR67_BACKLIGHT_PIN                    PIN(14)

// --- RS485 Communication Pins -----------------------------------------------
// Only needed for RS485 Slave or Master label sets.
// #define RS485_TX_PIN                           17       // GPIO for TX (connect to RS485 board DI)
// #define RS485_RX_PIN                           18       // GPIO for RX (connect to RS485 board RO)
// #define RS485_DE_PIN                           -1       // GPIO for DE pin (-1 if not used)

// --- RS485 Test Pins (development only, remove for production) --------------
// All three feed into int8_t fields → use -1 for disabled.
// #define RS485_TEST_BUTTON_GPIO                 -1
// #define RS485_TEST_SWITCH_GPIO                 -1
// #define RS485_TEST_LED_GPIO                    -1
"""

def run():
    print("CockpitOS Auto-Generated Data Reset Script")
    print("=" * 50)
    print()

    # Check for METADATA directory
    metadata_dir = "METADATA"
    has_metadata = os.path.isdir(metadata_dir)

    # Read current aircraft selection (if any) for re-pick default
    current_aircraft = None
    if os.path.isfile("aircraft.txt"):
        current_aircraft = open("aircraft.txt", "r", encoding="utf-8").read().strip()

    print("The following files will be DELETED if present:")
    print()

    to_delete = []
    for fname in AUTOGEN_FILES:
        if os.path.isfile(fname):
            print("  -", fname)
            to_delete.append(fname)
        else:
            print("  -", fname, "(not found)")

    # Also detect legacy local aircraft JSON copies for cleanup
    local_jsons = [f for f in glob.glob("*.json") if "METADATA" not in f]
    for fname in local_jsons:
        print(f"  - {fname} (legacy local copy)")
        to_delete.append(fname)

    # CustomPins.h gets special handling (blank, not delete)
    custom_pins_file = "CustomPins.h"
    custom_pins_exists = os.path.isfile(custom_pins_file)
    print()
    if custom_pins_exists:
        print(f"  * {custom_pins_file} will be BLANKED (reset to template)")
    else:
        print(f"  * {custom_pins_file} will be CREATED (with template)")

    # METADATA notice
    print()
    print("-" * 50)
    if has_metadata:
        print(f"  NOTE: METADATA/ directory will be PRESERVED")
        print(f"        (CommonData.json, MetadataStart.json, MetadataEnd.json, etc.)")
        print(f"        See Docs/CREATING_LABEL_SETS.md Section 9 for details.")
    else:
        print(f"  NOTE: No METADATA/ directory found in this label set.")
    print("-" * 50)

    print()
    if not to_delete:
        print("No auto-generated files found to delete.")

    print()
    if _BATCH:
        print("Batch mode: proceeding automatically...")
    else:
        response = input("Are you absolutely sure you want to proceed? (yes/NO): ")
        if response.lower().strip() != "yes":
            print("Aborted. No changes were made.")
            return

    print()
    deleted = 0
    for fname in to_delete:
        try:
            os.remove(fname)
            print("Deleted:", fname)
            deleted += 1
        except Exception as e:
            print(f"Could not delete {fname}: {e}")

    # Handle CustomPins.h (blank or create)
    try:
        with open(custom_pins_file, "w", encoding="utf-8") as f:
            f.write(CUSTOM_PINS_TEMPLATE)
        if custom_pins_exists:
            print(f"Blanked: {custom_pins_file}")
        else:
            print(f"Created: {custom_pins_file}")
    except Exception as e:
        print(f"Could not write {custom_pins_file}: {e}")

    print()
    print(f"Done. {deleted} file(s) deleted, CustomPins.h ready.")
    if has_metadata:
        print(f"METADATA/ directory preserved.")

    # ── Aircraft selection ──────────────────────────────────────────────────
    # Clear screen and present aircraft selector
    os.system("cls" if os.name == "nt" else "clear")

    current_label_set = os.path.basename(os.getcwd())
    _ls_name = current_label_set[len("LABEL_SET_"):] if current_label_set.startswith("LABEL_SET_") else current_label_set

    print()
    print("=" * 55)
    print(f"  LABEL_SET: {_ls_name}")
    print("=" * 55)
    print()
    if deleted > 0:
        print(f"  {deleted} auto-generated file(s) wiped.")
        print()
    print("  Select the aircraft for which this")
    print("  LABEL_SET will generate its configuration.")
    print()
    print("-" * 55)

    core_dir = os.path.dirname(os.path.abspath(__file__))
    selected = select_aircraft(core_dir, current=current_aircraft)
    if selected:
        with open("aircraft.txt", "w", encoding="utf-8") as f:
            f.write(selected + "\n")
        print()
        print("=" * 55)
        print(f"  SUCCESS! LABEL_SET '{_ls_name}' is ready.")
        print(f"  Aircraft: {selected}")
        print("=" * 55)
    else:
        print()
        print("  Aircraft selection cancelled.")

    print()
    if not _BATCH:
        input("Press <ENTER> to exit...")

if __name__ == "__main__":
    run()