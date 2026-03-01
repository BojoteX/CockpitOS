#!/usr/bin/env python3
"""
CockpitOS Remote Bootloader Tool
================================

Sends a magic UDP packet to trigger ESP32 devices into bootloader mode.
Works for both WiFi and USB devices (HID Manager forwards UDP to USB).
Arrow-key interface and device selection.

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
import msvcrt
import ctypes
import argparse
from pathlib import Path

# ═══════════════════════════════════════════════════════════════════════════════
# TERMINAL SETUP
# ═══════════════════════════════════════════════════════════════════════════════

def _enable_ansi():
    """Enable ANSI escape processing on Windows 10+."""
    kernel32 = ctypes.windll.kernel32
    h = kernel32.GetStdHandle(-11)
    mode = ctypes.c_ulong()
    kernel32.GetConsoleMode(h, ctypes.byref(mode))
    kernel32.SetConsoleMode(h, mode.value | 0x0004)

_enable_ansi()

# ═══════════════════════════════════════════════════════════════════════════════
# DEPENDENCY CHECK
# ═══════════════════════════════════════════════════════════════════════════════

try:
    import ifaddr
except ImportError:
    print()
    print("+----------------------------------------------------------------+")
    print("|  ERROR: Missing Required Module: ifaddr                        |")
    print("+----------------------------------------------------------------+")
    print("|                                                                |")
    print("|  This tool requires 'ifaddr' for network interface discovery.  |")
    print("|                                                                |")
    print("|      pip install ifaddr                                        |")
    print("|                                                                |")
    print("+----------------------------------------------------------------+")
    print()
    input("\nPress <ENTER> to exit...")
    sys.exit(1)

# ═══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION
# ═══════════════════════════════════════════════════════════════════════════════

MULTICAST_IP = "239.255.50.10"
MULTICAST_PORT = 5010
MAGIC_PREFIX = "COCKPITOS:REBOOT:"

# ═══════════════════════════════════════════════════════════════════════════════
# ANSI CONSTANTS
# ═══════════════════════════════════════════════════════════════════════════════

CYAN     = "\033[96m"
GREEN    = "\033[92m"
YELLOW   = "\033[93m"
RED      = "\033[91m"
BOLD     = "\033[1m"
DIM      = "\033[2m"
RESET    = "\033[0m"
REV      = "\033[7m"
HIDE_CUR = "\033[?25l"
SHOW_CUR = "\033[?25h"
ERASE_LN = "\033[2K"

def _w(s):
    sys.stdout.write(s)
    sys.stdout.flush()

import re as _re
_ANSI_RE = _re.compile(r'\033\[[0-9;]*[A-Za-z]')

def _box_line(text):
    """Print a box line with perfect right-side alignment. Inner width = 64 visible chars."""
    visible = _ANSI_RE.sub('', text)
    pad = 64 - len(visible)
    if pad < 0:
        pad = 0
    print(f"|{text}{' ' * pad}|")

# ═══════════════════════════════════════════════════════════════════════════════
# NETWORK INTERFACE DISCOVERY
# ═══════════════════════════════════════════════════════════════════════════════

def shorten_adapter_name(name):
    """Shorten long Windows adapter names to something readable."""
    replacements = [
        ("Network Adapter", ""),
        ("Dual Band Simultaneous (DBS)", ""),
        ("Wi-Fi 7", "WiFi7"),
        ("Wi-Fi 6", "WiFi6"),
        ("Wi-Fi", "WiFi"),
        ("Qualcomm FastConnect 7800", "Qualcomm"),
        ("Intel(R)", "Intel"),
        ("Realtek", "Realtek"),
        ("Virtual", "Virt"),
        ("Ethernet", "Eth"),
        ("Wireless", "WiFi"),
        ("  ", " "),
    ]
    result = name
    for old, new in replacements:
        result = result.replace(old, new)
    return result.strip()[:35]


def get_all_interface_ips():
    """Discover all usable IPv4 interface addresses. Returns list of (name, ip)."""
    interfaces = []
    for adapter in ifaddr.get_adapters():
        for ip in adapter.ips:
            if isinstance(ip.ip, str):
                if ip.ip.startswith('127.') or ip.ip.startswith('169.254.'):
                    continue
                short_name = shorten_adapter_name(adapter.nice_name)
                interfaces.append((short_name, ip.ip))
    return interfaces

# ═══════════════════════════════════════════════════════════════════════════════
# ARROW-KEY PICKER
# ═══════════════════════════════════════════════════════════════════════════════

def arrow_pick(prompt, options):
    """Arrow-key picker. options = list of (label, value). Returns value or None on Esc."""
    if not options:
        return None

    total = len(options)
    idx = 0

    print()
    _w(f"  {BOLD}{prompt}{RESET}\n")
    _w(f"  {DIM}(Up/Down to move, Enter to select, Esc to go back){RESET}\n")

    _w(HIDE_CUR)
    for i, (label, _) in enumerate(options):
        if i == idx:
            _w(f"  {REV} > {label} {RESET}\n")
        else:
            _w(f"     {label}\n")

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                old = idx
                if ch2 == "H":          # Up
                    idx = (idx - 1) % total
                elif ch2 == "P":        # Down
                    idx = (idx + 1) % total
                else:
                    continue
                if old != idx:
                    _w(f"\033[{total - old}A")
                    _w(f"\r{ERASE_LN}     {options[old][0]}")
                    if idx > old:
                        _w(f"\033[{idx - old}B")
                    else:
                        _w(f"\033[{old - idx}A")
                    _w(f"\r{ERASE_LN}  {REV} > {options[idx][0]} {RESET}")
                    remaining = total - idx
                    if remaining > 0:
                        _w(f"\033[{remaining}B")
                    _w("\r")

            elif ch == "\r":        # Enter
                _w(SHOW_CUR)
                return options[idx][1]

            elif ch == "\x1b":      # Esc
                _w(SHOW_CUR)
                return None

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        sys.exit(0)


def pick_interface(interfaces):
    """Arrow-key picker for network interfaces. Returns list of (name, ip) to use."""
    options = [("All interfaces", "ALL"),
               ("Localhost (127.0.0.1)", "127.0.0.1")]
    for name, ip in interfaces:
        options.append((f"{ip:<17} {DIM}({name}){RESET}", ip))

    chosen = arrow_pick("Select network interface:", options)

    if chosen is None or chosen == "ALL":
        return interfaces
    elif chosen == "127.0.0.1":
        return [("Localhost", "127.0.0.1")]
    else:
        for name, ip in interfaces:
            if ip == chosen:
                return [(name, ip)]
        return interfaces


def create_multicast_socket(interface_ip, ttl=2):
    """Create a UDP socket configured to send multicast on a specific interface."""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)
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
        print(f"  {YELLOW}Warning:{RESET} Could not parse {config_path}: {e}")
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

def send_bootloader_command(target, selected_interfaces):
    """Send the magic bootloader packet via UDP multicast on selected interfaces."""
    message = f"{MAGIC_PREFIX}{target}\n".encode('utf-8')

    successful_interfaces = []

    for iface_name, ip in selected_interfaces:
        try:
            sock = create_multicast_socket(ip)
            for _ in range(3):  # Send multiple times for reliability
                sock.sendto(message, (MULTICAST_IP, MULTICAST_PORT))
            sock.close()
            successful_interfaces.append((ip, iface_name))
        except Exception:
            pass  # Silently skip interfaces that fail

    # Fallback if no interfaces worked
    if not successful_interfaces:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 2)
            for _ in range(3):
                sock.sendto(message, (MULTICAST_IP, MULTICAST_PORT))
            sock.close()
            successful_interfaces.append(("default", "default route"))
        except Exception as e:
            print(f"\n  {RED}[FAIL]{RESET} No usable network interfaces ({e})")
            return False

    # Report success
    iface_list = ", ".join(f"{ip} ({name})" for ip, name in successful_interfaces)
    print(f"\n  {GREEN}[SENT]{RESET} {MAGIC_PREFIX}{target}")
    print(f"  {DIM}via: {iface_list}{RESET}")
    return True

# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
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

    # Always pick interface first
    interfaces = get_all_interface_ips()
    selected_interfaces = pick_interface(interfaces)
    print("\033[2J\033[H", end="", flush=True)

    # Build interface label for header
    iface_label = ", ".join(f"{ip} ({name})" for name, ip in selected_interfaces)

    # Header
    print()
    print("+----------------------------------------------------------------+")
    _box_line(f"  {BOLD}CockpitOS Remote Bootloader Tool{RESET}")
    print("+----------------------------------------------------------------+")
    _box_line(f"  Interface: {CYAN}{iface_label}{RESET}")
    print("+----------------------------------------------------------------+")

    # Direct command mode
    if args.all:
        send_bootloader_command('*', selected_interfaces)
        return

    if args.device:
        send_bootloader_command(args.device, selected_interfaces)
        return

    if args.pid:
        pid = args.pid.upper()
        if not pid.startswith('0X'):
            pid = '0x' + pid
        send_bootloader_command(pid, selected_interfaces)
        return

    # Interactive mode — discover devices
    labels_dir = find_labels_directory()
    devices = []

    if labels_dir:
        devices = discover_label_sets(labels_dir)

    if devices:
        # Build arrow-key device picker
        options = []
        for dev in devices:
            label = f"{CYAN}{dev['name']:<20}{RESET} {DIM}({dev['pid']})  \"{dev['fullname']}\"{RESET}"
            options.append((label, dev['name']))

        options.append((f"{YELLOW}Reboot ALL devices{RESET}", "*"))

        print(f"\n  {DIM}Found {len(devices)} configured panel(s){RESET}")

        choice = arrow_pick("Select device to reboot:", options)

        if choice is None:
            print(f"\n  {DIM}Cancelled.{RESET}")
            return

        send_bootloader_command(choice, selected_interfaces)

    else:
        # Standalone mode — no devices found
        print(f"\n  {YELLOW}No LABELS directory found — standalone mode{RESET}")

        options = [
            (f"{YELLOW}Reboot ALL devices (wildcard){RESET}", "*"),
            (f"Enter device name manually", "MANUAL"),
        ]

        choice = arrow_pick("Select action:", options)

        if choice is None:
            print(f"\n  {DIM}Cancelled.{RESET}")
            return

        if choice == "*":
            send_bootloader_command('*', selected_interfaces)
        elif choice == "MANUAL":
            _w(SHOW_CUR)
            print(f"\n  Enter LABEL_SET_NAME (e.g., IFEI, HORNET_FRNT_RIGHT):")
            name = input("  > ").strip()
            if name:
                send_bootloader_command(name, selected_interfaces)
            else:
                print(f"  {DIM}No name entered.{RESET}")


if __name__ == "__main__":
    main()
