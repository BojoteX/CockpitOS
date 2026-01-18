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

def run():
    print("CockpitOS Auto-Generated Data Reset Script")
    print("=" * 50)
    print("The following files will be DELETED if present:")
    print()
    to_delete = []
    for fname in AUTOGEN_FILES:
        if os.path.isfile(fname):
            print("  -", fname)
            to_delete.append(fname)
        else:
            print("  -", fname, "(not found)")
    print()
    if not to_delete:
        print("No auto-generated files found to delete. Nothing to do.")
        return
    print()
    response = input("Are you absolutely sure you want to DELETE these files? (yes/NO): ")
    if response.lower().strip() != "yes":
        print("Aborted. No files were deleted.")
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
    print()
    print(f"Done. {deleted} file(s) deleted.")

if __name__ == "__main__":
    run()
