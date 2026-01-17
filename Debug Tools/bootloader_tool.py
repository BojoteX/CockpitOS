#!/usr/bin/env python3
"""
CockpitOS Remote Bootloader Tool
================================

Sends a magic UDP packet to trigger ESP32 devices into bootloader mode.
Works for both WiFi and USB devices (HID Manager forwards UDP to USB).

Usage:
    python bootloader_tool.py                    # Interactive menu
    python bootloader_tool.py --device IFEI      # Direct device by name
    python bootloader_tool.py --pid 0xC8DD       # Direct device by PID
    python bootloader_tool.py --all              # All devices (careful!)

Author: CockpitOS Project
"""

import os
import re
import sys
import socket
import argparse
from pathlib import Path

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

MULTICAST_IP = "239.255.50.10"
MULTICAST_PORT = 5010
MAGIC_PREFIX = "COCKPITOS:REBOOT:"

# ═══════════════════════════════════════════════════════════════════════════════
# LABEL SET DISCOVERY
# ═══════════════════════════════════════════════════════════════════════════════

def find_labels_directory():
    """Search for the LABELS directory in common locations."""
    search_paths = [
        Path(__file__).parent.parent / "src" / "LABELS",
        Path(__file__).parent / "src" / "LABELS",
        Path.cwd() / "src" / "LABELS",
        Path.cwd().parent / "src" / "LABELS",
        Path(__file__).parent.parent.parent / "src" / "LABELS",
    ]
    for path in search_paths:
        if path.exists() and path.is_dir():
            return path.resolve()
    return None


def parse_label_set_config(config_path):
    """Parse a LabelSetConfig.h file and extract device information."""
    try:
        content = config_path.read_text(encoding='utf-8')
        
        name_match = re.search(r'#define\s+LABEL_SET_NAME\s+"([^"]+)"', content)
        name = name_match.group(1) if name_match else None
        
        fullname_match = re.search(r'#define\s+LABEL_SET_FULLNAME\s+"([^"]+)"', content)
        fullname = fullname_match.group(1) if fullname_match else name
        
        pid_match = re.search(r'#define\s+AUTOGEN_USB_PID\s+(0x[0-9A-Fa-f]+)', content)
        pid = pid_match.group(1).upper() if pid_match else None
        
        if name and pid:
            return {'name': name, 'fullname': fullname, 'pid': pid}
    except Exception as e:
        print(f"  Warning: Could not parse {config_path}: {e}")
    return None


def discover_label_sets(labels_dir):
    """Scan LABELS directory for all LABEL_SET_* folders."""
    devices = []
    for folder in labels_dir.iterdir():
        if folder.is_dir() and folder.name.startswith("LABEL_SET_"):
            config_file = folder / "LabelSetConfig.h"
            if config_file.exists():
                device = parse_label_set_config(config_file)
                if device:
                    devices.append(device)
    devices.sort(key=lambda d: d['name'])
    return devices

# ═══════════════════════════════════════════════════════════════════════════════
# SEND MAGIC PACKET
# ═══════════════════════════════════════════════════════════════════════════════

def send_bootloader_command(target):
    """Send the magic bootloader packet via UDP multicast."""
    message = f"{MAGIC_PREFIX}{target}\n".encode('utf-8')
    
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
        
        # Send multiple times for reliability
        for _ in range(3):
            sock.sendto(message, (MULTICAST_IP, MULTICAST_PORT))
        
        sock.close()
        print(f"✅ Sent: {MAGIC_PREFIX}{target}")
        return True
    except Exception as e:
        print(f"❌ Network error: {e}")
        return False

# ═══════════════════════════════════════════════════════════════════════════════
# USER INTERFACE
# ═══════════════════════════════════════════════════════════════════════════════

def print_header():
    print()
    print("╔════════════════════════════════════════════════════════════════╗")
    print("║           CockpitOS Remote Bootloader Tool                     ║")
    print("╠════════════════════════════════════════════════════════════════╣")


def print_device_menu(devices):
    print("║                                                                ║")
    print(f"║   Found {len(devices)} configured panel(s):".ljust(65) + "║")
    print("║                                                                ║")
    
    for i, dev in enumerate(devices, 1):
        line = f"   [{i}] {dev['name'][:20]:<20} ({dev['pid']})  \"{dev['fullname'][:20]}\""
        print(f"║{line[:62]:<63}║")
    
    print("║                                                                ║")
    print("║   [*] Reboot ALL devices (use with caution!)                   ║")
    print("║   [Q] Quit                                                     ║")
    print("║                                                                ║")
    print("╚════════════════════════════════════════════════════════════════╝")


def print_standalone_menu():
    print("║                                                                ║")
    print("║   ⚠️  No LABELS directory found - standalone mode              ║")
    print("║                                                                ║")
    print("║   [1] Reboot ALL devices (wildcard)                            ║")
    print("║   [2] Enter device LABEL_SET_NAME manually                     ║")
    print("║   [Q] Quit                                                     ║")
    print("║                                                                ║")
    print("╚════════════════════════════════════════════════════════════════╝")


def confirm_action(target, is_wildcard=False):
    print()
    if is_wildcard:
        print("⚠️  WARNING: This will reboot ALL CockpitOS devices!")
        return input("Type 'YES' to confirm: ").strip().upper() == 'YES'
    else:
        response = input(f"Reboot '{target}'? [Y/n]: ").strip().lower()
        return response in ('', 'y', 'yes')


def run_interactive_with_devices(devices):
    print_header()
    print_device_menu(devices)
    print()
    
    while True:
        choice = input("Select device: ").strip()
        
        if choice.lower() == 'q':
            print("Goodbye!")
            return
        
        if choice == '*':
            if confirm_action('*', is_wildcard=True):
                send_bootloader_command('*')
            return
        
        try:
            idx = int(choice)
            if 1 <= idx <= len(devices):
                device = devices[idx - 1]
                if confirm_action(device['name']):
                    send_bootloader_command(device['name'])
                return
            else:
                print(f"Invalid. Enter 1-{len(devices)}, *, or Q")
        except ValueError:
            print("Invalid. Enter a number, *, or Q")


def run_interactive_standalone():
    print_header()
    print_standalone_menu()
    print()
    
    while True:
        choice = input("Select option: ").strip()
        
        if choice.lower() == 'q':
            print("Goodbye!")
            return
        
        if choice == '1':
            if confirm_action('*', is_wildcard=True):
                send_bootloader_command('*')
            return
        
        if choice == '2':
            print("\nEnter LABEL_SET_NAME (e.g., IFEI, HORNET_FRNT_RIGHT):")
            name = input("LABEL_SET_NAME: ").strip()
            if name and confirm_action(name):
                send_bootloader_command(name)
            return
        
        print("Invalid. Enter 1, 2, or Q")

# ═══════════════════════════════════════════════════════════════════════════════
# COMMAND LINE
# ═══════════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description='CockpitOS Remote Bootloader Tool',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python bootloader_tool.py                    Interactive menu
  python bootloader_tool.py --device IFEI      Reboot by name
  python bootloader_tool.py --pid 0xC8DD       Reboot by PID
  python bootloader_tool.py --all              Reboot ALL devices
        """
    )
    
    parser.add_argument('--device', '-d', metavar='NAME', help='Device LABEL_SET_NAME')
    parser.add_argument('--pid', '-p', metavar='PID', help='Device USB PID (e.g., 0xC8DD)')
    parser.add_argument('--all', '-a', action='store_true', help='Reboot ALL devices')
    parser.add_argument('--yes', '-y', action='store_true', help='Skip confirmation')
    
    args = parser.parse_args()
    
    # Direct command mode
    if args.all:
        if args.yes or confirm_action('*', is_wildcard=True):
            send_bootloader_command('*')
        return
    
    if args.device:
        if args.yes or confirm_action(args.device):
            send_bootloader_command(args.device)
        return
    
    if args.pid:
        pid = args.pid.upper()
        if not pid.startswith('0X'):
            pid = '0x' + pid
        if args.yes or confirm_action(pid):
            send_bootloader_command(pid)
        return
    
    # Interactive mode
    labels_dir = find_labels_directory()
    
    if labels_dir:
        print(f"📁 Found LABELS: {labels_dir}")
        devices = discover_label_sets(labels_dir)
        if devices:
            run_interactive_with_devices(devices)
            return
    
    run_interactive_standalone()


if __name__ == "__main__":
    main()