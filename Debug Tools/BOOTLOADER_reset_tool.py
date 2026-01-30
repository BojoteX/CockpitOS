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
# MULTI-INTERFACE NETWORK SUPPORT
# ═══════════════════════════════════════════════════════════════════════════════

def get_all_interface_ips():
    """
    Discover all IPv4 interface addresses on this machine.
    Returns list of IP strings, excluding loopback.
    
    Uses multiple discovery methods to ensure we find interfaces that may
    not be on the default route (e.g., WiFi hotspots, secondary NICs).
    """
    ips = set()
    
    # Method 1: Query hostname - gets IPs registered with DNS/hosts
    try:
        hostname = socket.gethostname()
        _, _, ip_list = socket.gethostbyname_ex(hostname)
        for ip in ip_list:
            if not ip.startswith('127.'):
                ips.add(ip)
    except Exception:
        pass
    
    # Method 2: Probe various subnets to discover routed interfaces
    # Each connect() reveals which local IP would reach that destination.
    # This finds interfaces the OS has routes for, even if not default.
    probe_targets = [
        ('8.8.8.8', 80),           # Internet (default gateway)
        ('10.255.255.255', 1),     # Private Class A
        ('172.16.255.255', 1),     # Private Class B
        ('192.168.0.1', 1),        # Common home router
        ('192.168.1.1', 1),        # Common home router
        ('192.168.137.1', 1),      # Windows Mobile Hotspot default
        ('192.168.49.1', 1),       # Windows Wi-Fi Direct default
    ]
    
    for target, port in probe_targets:
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.settimeout(0.001)  # Non-blocking, we don't actually send
            s.connect((target, port))
            ip = s.getsockname()[0]
            s.close()
            if ip and not ip.startswith('127.') and not ip.startswith('0.'):
                ips.add(ip)
        except Exception:
            pass
    
    # Fallback: if nothing found, return localhost (will still work for local testing)
    return sorted(ips) if ips else ['127.0.0.1']


def create_multicast_socket(interface_ip, ttl=2):
    """
    Create a UDP socket configured to send multicast on a specific interface.
    Uses IP_MULTICAST_IF to direct outgoing multicast to the specified NIC.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)
    # Bind multicast output to this specific interface
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(interface_ip))
    return sock

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
    """Send the magic bootloader packet via UDP multicast on ALL interfaces."""
    message = f"{MAGIC_PREFIX}{target}\n".encode('utf-8')
    
    # Discover all network interfaces
    interface_ips = get_all_interface_ips()
    
    success_count = 0
    fail_count = 0
    
    print(f"\n📡 Sending bootloader command on {len(interface_ips)} interface(s)...")
    
    for ip in interface_ips:
        try:
            sock = create_multicast_socket(ip)
            
            # Send multiple times for reliability
            for _ in range(3):
                sock.sendto(message, (MULTICAST_IP, MULTICAST_PORT))
            
            sock.close()
            print(f"   ✓ {ip}")
            success_count += 1
        except Exception as e:
            print(f"   ✗ {ip} ({e})")
            fail_count += 1
    
    if success_count > 0:
        print(f"\n✅ Sent: {MAGIC_PREFIX}{target}")
        return True
    else:
        print(f"\n❌ Failed to send on any interface")
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
    print("║   [*] Reboot ALL devices                                       ║")
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
            send_bootloader_command('*')
            return
        
        try:
            idx = int(choice)
            if 1 <= idx <= len(devices):
                device = devices[idx - 1]
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
            send_bootloader_command('*')
            return
        
        if choice == '2':
            print("\nEnter LABEL_SET_NAME (e.g., IFEI, HORNET_FRNT_RIGHT):")
            name = input("LABEL_SET_NAME: ").strip()
            if name:
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
    
    args = parser.parse_args()
    
    # Direct command mode
    if args.all:
        send_bootloader_command('*')
        return
    
    if args.device:
        send_bootloader_command(args.device)
        return
    
    if args.pid:
        pid = args.pid.upper()
        if not pid.startswith('0X'):
            pid = '0x' + pid
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
