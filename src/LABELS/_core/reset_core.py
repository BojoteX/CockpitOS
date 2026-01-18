#!/usr/bin/env python3
"""
CockpitOS Reset Data - Core Module
===================================
Called from LABEL_SET_XXX/reset_data.py wrappers.
CWD is set by the wrapper BEFORE this module is imported.
"""
import os

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
    # "panels.txt",
    # "selected_panels.txt",
    "LabelSetConfig.h",
    # Add more files here if needed
]

# Template for CustomPins.h (blanked or newly created)
CUSTOM_PINS_TEMPLATE = """\
// CustomPins.h - Label-set-specific pin definitions for CockpitOS
//
// This file is automatically included by LabelSetConfig.h if present.
// Define pins here that are unique to THIS label set (panel).
//
// Example: HC165 shift register pins
// #define HC165_BITS                 16
// #define HC165_CONTROLLER_PL        PIN(39)
// #define HC165_CONTROLLER_CP        PIN(38)
// #define HC165_CONTROLLER_QH        PIN(40)
//
// Example: Panel-specific display pins
// #define MY_DISPLAY_CS_PIN          PIN(10)
// #define MY_DISPLAY_DC_PIN          PIN(11)
//
// Note: The PIN(X) macro converts S2 pins to S3 equivalents automatically.
"""

def run():
    print("CockpitOS Auto-Generated Data Reset Script")
    print("=" * 50)
    print()

    # Check for METADATA directory
    metadata_dir = "METADATA"
    has_metadata = os.path.isdir(metadata_dir)

    print("The following files will be DELETED if present:")
    print()

    to_delete = []
    for fname in AUTOGEN_FILES:
        if os.path.isfile(fname):
            print("  -", fname)
            to_delete.append(fname)
        else:
            print("  -", fname, "(not found)")

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

if __name__ == "__main__":
    run()