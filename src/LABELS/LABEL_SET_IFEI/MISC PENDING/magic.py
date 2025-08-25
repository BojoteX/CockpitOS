#!/usr/bin/env python3
import os, sys, json
from collections import defaultdict
os.chdir(os.path.dirname(os.path.abspath(__file__)))

import os

WHITELIST = ['CT_Display.cpp', 'DisplayMapping.cpp']

def print_and_disable_cpp_files():
    current_dir = os.path.abspath(os.path.dirname(__file__))
    current_dir_name = os.path.basename(current_dir)
    parent_dir = os.path.dirname(current_dir)
    parent_dir_name = os.path.basename(parent_dir)

    print(f"Current directory: {current_dir_name}")
    print(f"LABELS directory: {parent_dir_name}")

    # SAFETY CHECK: must be inside a LABELS directory
    if parent_dir_name != "LABELS":
        print("ERROR: This script must be run from a subdirectory of 'LABELS'!")
        input("\nPress <ENTER> to exit...")
        return

    print("Directories in LABELS (excluding current):")
    for entry in os.listdir(parent_dir):
        entry_path = os.path.join(parent_dir, entry)
        if os.path.isdir(entry_path) and entry != current_dir_name:
            print(f"- {entry}")
            cpp_files = [f for f in os.listdir(entry_path)
                         if os.path.isfile(os.path.join(entry_path, f)) 
                         and f.endswith('.cpp') 
                         and f in WHITELIST]
            if cpp_files:
                for f in cpp_files:
                    src = os.path.join(entry_path, f)
                    dst = src + ".DISABLED"
                    # If .DISABLED exists, remove it first
                    if os.path.exists(dst):
                        try:
                            os.remove(dst)
                        except Exception as e:
                            print(f"    Could not remove existing {os.path.basename(dst)}: {e}")
                            continue
                    try:
                        os.rename(src, dst)
                        print(f"    {f} -> {os.path.basename(dst)} (renamed/overwritten)")
                    except Exception as e:
                        print(f"    {f} -> ERROR: {e}")
            else:
                print("    (No .cpp files from whitelist)")
if __name__ == "__main__":
    print_and_disable_cpp_files()
    input("\nPress <ENTER> to exit...")