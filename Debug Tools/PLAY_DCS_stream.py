#!/usr/bin/env python3
"""
CockpitOS DCS-BIOS Stream Replay Tool
=====================================

Replays captured DCS-BIOS data streams for offline testing.
Sends multicast on a user-selected network interface (or all).

Fully interactive — just run the script and follow the prompts:
  1. Select network interface
  2. Select stream
  3. Split oversized frames? (only asked when stream has frames > WiFi MTU)
  4. Chunk size (if splitting)

By default, replays at a fixed 30 FPS — matching DCS-BIOS's internal tick
rate — to produce a stable, realistic stream for HID Manager testing.
The recorded per-frame timing is inherently noisy (OS-level timestamp
jitter, Windows timer granularity) and does not represent the true DCS
export rate, so we don't use it unless explicitly requested via --raw-timing.

All prompts can be bypassed via CLI flags for scripting:
    python PLAY_DCS_stream.py                    # Interactive (default)
    python PLAY_DCS_stream.py --stream LAST      # Skip stream picker
    python PLAY_DCS_stream.py --split-frames     # Skip split prompt (use firmware UDP_MAX_SIZE)
    python PLAY_DCS_stream.py --chunk-size 512   # Split at 512B (implies --split-frames)
    python PLAY_DCS_stream.py --fps 60           # Fixed 60 FPS
    python PLAY_DCS_stream.py --raw-timing       # Use recorded timing

Author: CockpitOS Project
"""

import socket
import json
import time
import binascii
import struct
import os
import sys
import re as _re
import msvcrt
import ctypes
import argparse
import winreg
from pathlib import Path
from datetime import datetime

# ═══════════════════════════════════════════════════════════════════════════════
# DEPENDENCY CHECK
# ═══════════════════════════════════════════════════════════════════════════════

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
UDP_PORT = 5010
STREAMS_DIR = "streams"
FILE_PREFIX = "dcsbios_data.json."

# Fallback chunk size if Config.h / DCS-BIOS detection fails
DEFAULT_CHUNK_SIZE = 1460

# ═══════════════════════════════════════════════════════════════════════════════
# DYNAMIC CONFIG READERS
# ═══════════════════════════════════════════════════════════════════════════════

def _read_firmware_udp_max_size():
    """Read UDP_MAX_SIZE from ../Config.h. Returns (value, True) or (fallback, False)."""
    config_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Config.h")
    try:
        with open(config_path, "r") as f:
            for line in f:
                m = _re.match(r'^\s*#define\s+UDP_MAX_SIZE\s+(\d+)', line)
                if m:
                    return int(m.group(1)), True
    except OSError:
        pass
    return DEFAULT_CHUNK_SIZE, False


def _get_saved_games_folder():
    """Get the Windows Saved Games folder using the Shell API."""
    fallback = Path(os.environ.get("USERPROFILE", "")) / "Saved Games"
    try:
        import ctypes.wintypes
        guid_bytes = (ctypes.c_ubyte * 16)(
            0xFF, 0x32, 0x5C, 0x4C, 0x9D, 0xBB, 0xB0, 0x43,
            0xB5, 0xB4, 0x2D, 0x72, 0xE5, 0x4E, 0xAA, 0xA4,
        )
        buf = ctypes.c_wchar_p()
        hr = ctypes.windll.shell32.SHGetKnownFolderPath(
            ctypes.byref(guid_bytes), 0, None, ctypes.byref(buf)
        )
        if hr == 0 and buf.value:
            result = Path(buf.value)
            ctypes.windll.ole32.CoTaskMemFree(buf)
            if result.is_dir():
                return result
    except Exception:
        pass
    return fallback


def _find_dcs_bios_paths():
    """Find DCS-BIOS lib directories via registry + saved games scan.

    Returns list of Path objects pointing to DCS-BIOS/lib/ directories.
    """
    saved_games = _get_saved_games_folder()
    candidates = []

    # Registry detection
    for reg_path, folder_name in [
        (r"SOFTWARE\Eagle Dynamics\DCS World", "DCS"),
        (r"SOFTWARE\Eagle Dynamics\DCS World OpenBeta", "DCS.openbeta"),
    ]:
        try:
            key = winreg.OpenKey(winreg.HKEY_CURRENT_USER, reg_path)
            try:
                winreg.QueryValueEx(key, "Path")
                key.Close()
                p = saved_games / folder_name / "Scripts" / "DCS-BIOS" / "lib"
                if p.is_dir():
                    candidates.append(p)
            except OSError:
                key.Close()
        except OSError:
            pass

    # Fallback: scan Saved Games
    if not candidates and saved_games.is_dir():
        for entry in saved_games.iterdir():
            if entry.is_dir() and (entry.name == "DCS" or entry.name.startswith("DCS.")):
                p = entry / "Scripts" / "DCS-BIOS" / "lib"
                if p.is_dir():
                    candidates.append(p)

    return candidates


def _read_dcsbios_max_payload():
    """Read MAX_PAYLOAD_SIZE from ConnectionManager.lua. Returns (value, path) or (None, None)."""
    for lib_dir in _find_dcs_bios_paths():
        lua_path = lib_dir / "ConnectionManager.lua"
        if not lua_path.exists():
            continue
        try:
            with open(lua_path, "r") as f:
                for line in f:
                    m = _re.match(r'\s*MAX_PAYLOAD_SIZE\s*=\s*(\d+)', line)
                    if m:
                        return int(m.group(1)), str(lua_path)
        except OSError:
            pass
    return None, None


# Read values at import time so they're available for CLI help text
_FIRMWARE_CHUNK, _FIRMWARE_FOUND = _read_firmware_udp_max_size()
_DCSBIOS_PAYLOAD, _DCSBIOS_PATH = _read_dcsbios_max_payload()

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

_ANSI_RE = _re.compile(r'\033\[[0-9;]*[A-Za-z]')

def _box_line(text):
    """Print a box line with perfect right-side alignment. Inner width = 64 visible chars."""
    visible = _ANSI_RE.sub('', text)
    pad = 64 - len(visible)
    if pad < 0:
        pad = 0
    print(f"|{text}{' ' * pad}|")

# ═══════════════════════════════════════════════════════════════════════════════
# ARROW-KEY PICKER (reusable)
# ═══════════════════════════════════════════════════════════════════════════════

def arrow_pick(prompt, options):
    """Arrow-key picker. options = list of (label, value). Returns value."""
    if not options:
        return None

    total = len(options)
    idx = 0

    print()
    _w(f"  {BOLD}{prompt}{RESET}\n")
    _w(f"  {DIM}(Up/Down to move, Enter to select){RESET}\n")

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

            elif ch == "\x1b":      # Esc — return first option as default
                _w(SHOW_CUR)
                return options[0][1]

    except KeyboardInterrupt:
        _w(SHOW_CUR)
        sys.exit(0)

# ═══════════════════════════════════════════════════════════════════════════════
# MULTI-INTERFACE NETWORK SUPPORT
# ═══════════════════════════════════════════════════════════════════════════════

def get_all_interface_ips():
    """
    Discover all usable IPv4 interface addresses on this machine.
    Returns list of (interface_name, ip_address) tuples.
    """
    interfaces = []

    for adapter in ifaddr.get_adapters():
        for ip in adapter.ips:
            if isinstance(ip.ip, str):
                if ip.ip.startswith('127.') or ip.ip.startswith('169.254.'):
                    continue
                short_name = shorten_adapter_name(adapter.nice_name)
                interfaces.append((short_name, ip.ip))

    return interfaces


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


def pick_interface(interfaces):
    """Arrow-key picker for network interfaces. Returns list of (name, ip) to use."""
    options = [("All interfaces", "ALL"),
               ("Localhost (127.0.0.1)", "127.0.0.1")]
    for name, ip in interfaces:
        options.append((f"{ip:<17} {DIM}({name}){RESET}", ip))

    chosen = arrow_pick("Select network interface:", options)

    if chosen == "ALL":
        return interfaces  # all of them
    elif chosen == "127.0.0.1":
        return [("Localhost", "127.0.0.1")]
    else:
        for name, ip in interfaces:
            if ip == chosen:
                return [(name, ip)]
        return interfaces  # fallback


def create_multicast_socket(interface_ip, ttl=1):
    """
    Create a UDP socket configured to send multicast on a specific interface.
    Uses IP_MULTICAST_IF to direct outgoing multicast to the specified NIC.
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, ttl)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(interface_ip))
    return sock


# ═══════════════════════════════════════════════════════════════════════════════
# STREAM DISCOVERY & SELECTION
# ═══════════════════════════════════════════════════════════════════════════════

def get_relative_age(file_path):
    """Calculate human-readable age string for a file."""
    mtime = os.path.getmtime(file_path)
    age_seconds = time.time() - mtime

    if age_seconds < 60:
        return f"{int(age_seconds)} seconds old"
    elif age_seconds < 3600:
        minutes = int(age_seconds / 60)
        return f"{minutes} minute{'s' if minutes != 1 else ''} old"
    elif age_seconds < 86400:
        hours = int(age_seconds / 3600)
        return f"{hours} hour{'s' if hours != 1 else ''} old"
    elif age_seconds < 2592000:  # 30 days
        days = int(age_seconds / 86400)
        return f"{days} day{'s' if days != 1 else ''} old"
    else:
        months = int(age_seconds / 2592000)
        return f"{months} month{'s' if months != 1 else ''} old"


def discover_streams(streams_path):
    """Returns list of tuples: (identifier, full_path, display_name, age_string)"""
    streams = []

    if not os.path.isdir(streams_path):
        return streams

    for filename in os.listdir(streams_path):
        if filename.startswith(FILE_PREFIX):
            identifier = filename[len(FILE_PREFIX):]
            full_path = os.path.join(streams_path, filename)

            if identifier == "LAST":
                display_name = "LAST (Last captured stream)"
            else:
                display_name = identifier

            age_string = get_relative_age(full_path)
            streams.append((identifier, full_path, display_name, age_string))

    # Sort: LAST first (if present), then alphabetically
    streams.sort(key=lambda x: (x[0] != "LAST", x[0]))
    return streams


def select_stream(streams):
    """Arrow-key picker for stream selection."""
    options = []
    for identifier, path, display_name, age in streams:
        label = f"{display_name:<32} {DIM}({age}){RESET}"
        options.append((label, identifier))

    chosen = arrow_pick("Select stream:", options)

    for s in streams:
        if s[0] == chosen:
            return s

    return streams[0]  # fallback


# ═══════════════════════════════════════════════════════════════════════════════
# PROTOCOL-AWARE FRAME SPLITTING
# ═══════════════════════════════════════════════════════════════════════════════

SYNC_MARKER = b'\x55\x55\x55\x55'
END_MARKER_ADDR = 0xFFFE

def parse_dcsbios_frame(data):
    """
    Parse a DCS-BIOS binary frame into structural components.

    DCS-BIOS wire format per UDP packet:
      SYNC (4x 0x55)
      [address_lo, address_hi, count_lo, count_hi, data...] (repeating)
      End marker: address=0xFFFE, count=N, N data bytes (frame seq counter)

    Returns: (blocks, end_block, trailing) or None if no SYNC found.
      blocks:    list of (address, raw_data_bytes) for regular data blocks
      end_block: bytes of the complete end marker (header + data), or None
      trailing:  any leftover bytes after end marker
    """
    sync_pos = data.find(SYNC_MARKER)
    if sync_pos < 0:
        return None

    pos = sync_pos + 4
    # Skip any extra 0x55 bytes beyond the 4-byte SYNC
    while pos < len(data) and data[pos] == 0x55:
        pos += 1

    blocks = []
    end_block = None

    while pos + 4 <= len(data):
        block_start = pos
        addr  = struct.unpack_from('<H', data, pos)[0]
        count = struct.unpack_from('<H', data, pos + 2)[0]
        pos += 4

        if addr == END_MARKER_ADDR:
            # End marker block: header + count bytes (frame sequence counter)
            data_end = min(pos + count, len(data))
            end_block = data[block_start:data_end]
            pos = data_end
            break

        # Regular data block
        data_end = min(pos + count, len(data))
        blocks.append((addr, data[pos:data_end]))
        pos = data_end

    trailing = data[pos:] if pos < len(data) else b''
    return blocks, end_block, trailing


def split_frame_protocol_aware(binary_data, max_size):
    """
    Split an oversized DCS-BIOS frame at protocol data block boundaries.

    Instead of cutting at arbitrary byte offsets (which injects mid-stream
    corruption when HID Manager zero-pads the last 64B report), this splits
    at data block boundaries so every sub-frame is structurally valid.

    Each sub-frame is prefixed with a SYNC header (0x55555555) so the
    parser can synchronize.  Large data blocks that exceed max_size are
    split into smaller sub-blocks with sequential addresses — protocol-
    correct because the parser auto-increments address by 2 per word.

    The original end marker (with its frame-sequence-counter data) is
    preserved in the last sub-frame.

    Returns: list of bytes objects, each <= max_size.
    """
    result = parse_dcsbios_frame(binary_data)
    if result is None:
        # Unparseable — fall back to raw byte-boundary split
        return [binary_data[i:i+max_size] for i in range(0, len(binary_data), max_size)]

    blocks, end_block, trailing = result

    # Build "atoms" — smallest protocol-correct units.
    # Each atom = block header (4B) + data bytes.
    # Blocks larger than the available payload are split into sub-blocks
    # with sequential starting addresses.
    atoms = []
    max_atom_data = max_size - len(SYNC_MARKER) - 4   # room after SYNC + header
    max_atom_data = max(max_atom_data & ~1, 2)         # word-aligned, min 1 word

    for addr, block_data in blocks:
        if len(block_data) <= max_atom_data:
            atom = struct.pack('<HH', addr, len(block_data)) + block_data
            atoms.append(atom)
        else:
            # Split large block into sub-blocks with sequential addresses
            offset = 0
            cur_addr = addr
            while offset < len(block_data):
                chunk = block_data[offset:offset + max_atom_data]
                # Word-align unless it's the last piece
                if len(chunk) % 2 != 0 and offset + len(chunk) < len(block_data):
                    chunk = chunk[:-1]
                atom = struct.pack('<HH', cur_addr & 0xFFFF, len(chunk)) + chunk
                atoms.append(atom)
                cur_addr = (cur_addr + len(chunk)) & 0xFFFF
                offset += len(chunk)

    # Group atoms into sub-frames, each prefixed with SYNC
    sub_frames = []
    current = bytearray()

    for atom in atoms:
        projected = len(SYNC_MARKER) + len(current) + len(atom)
        if projected > max_size and len(current) > 0:
            sub_frames.append(SYNC_MARKER + bytes(current))
            current = bytearray()
        current.extend(atom)

    # Append end marker + trailing bytes to the last sub-frame
    suffix = (end_block or b'') + trailing
    if suffix:
        if len(SYNC_MARKER) + len(current) + len(suffix) > max_size and len(current) > 0:
            sub_frames.append(SYNC_MARKER + bytes(current))
            current = bytearray(suffix)
        else:
            current.extend(suffix)

    if current:
        sub_frames.append(SYNC_MARKER + bytes(current))

    return sub_frames if sub_frames else [binary_data]


# ═══════════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════════

def analyze_frames(frames, threshold):
    """Scan frames and return (oversized_count, max_frame_size) relative to threshold."""
    oversized_count = 0
    max_frame_size = 0
    for frame in frames:
        hex_data = frame.get("data", "")
        fsize = len(hex_data) // 2  # hex pairs -> bytes
        if fsize > max_frame_size:
            max_frame_size = fsize
        if fsize > threshold:
            oversized_count += 1
    return oversized_count, max_frame_size


def main():
    # === ARGUMENT PARSING (CLI overrides — all optional) ===
    parser = argparse.ArgumentParser(description="DCS-BIOS UDP Replay Tool")
    parser.add_argument("--fps", type=float, default=30.0,
                        help="Fixed replay rate in FPS (default: 30, matching DCS-BIOS tick rate)")
    parser.add_argument("--raw-timing", action="store_true",
                        help="Use recorded per-frame timing instead of fixed FPS")
    parser.add_argument("--speed", type=float, default=1.0,
                        help="Speed multiplier for --raw-timing mode (e.g. 2.0 = 2x faster)")
    parser.add_argument("--stream", type=str,
                        help="Stream identifier to load directly (bypasses menu)")
    parser.add_argument("--split-frames", action="store_true",
                        help="Split oversized UDP frames (bypasses interactive prompt)")
    parser.add_argument("--chunk-size", type=int, default=None,
                        help=f"Chunk size in bytes for --split-frames (default: {_FIRMWARE_CHUNK})")
    args = parser.parse_args()

    # Change to script directory
    os.chdir(os.path.dirname(os.path.abspath(__file__)))

    # === 1. INTERFACE SELECTION ===
    interfaces = get_all_interface_ips()
    selected_interfaces = pick_interface(interfaces)
    print("\033[2J\033[H", end="", flush=True)

    # === 2. STREAM SELECTION ===
    streams_path = os.path.join(os.getcwd(), STREAMS_DIR)
    available_streams = discover_streams(streams_path)

    if not available_streams:
        print("\nNo stream files found!")
        print(f"   Expected location: {streams_path}")
        print(f"   Expected pattern:  {FILE_PREFIX}<identifier>")
        print("\n   Run RECORD_DCS_stream.py to capture a stream.")
        sys.exit(1)

    if args.stream:
        match = None
        for stream in available_streams:
            if stream[0].upper() == args.stream.upper():
                match = stream
                break
        if not match:
            print(f"\nStream '{args.stream}' not found!")
            print("   Available streams:", ", ".join(s[0] for s in available_streams))
            sys.exit(1)
        selected = match
    else:
        selected = select_stream(available_streams)

    identifier, INPUT_JSON_FILE, display_name, age = selected

    # === 3. LOAD & ANALYZE ===
    with open(INPUT_JSON_FILE, "r") as f:
        frames = json.load(f)

    oversized_count, max_frame_size = analyze_frames(frames, _FIRMWARE_CHUNK)

    # === 4. SPLIT FRAMES (interactive unless CLI override) ===
    split_enabled = args.split_frames
    chunk_size = args.chunk_size or _FIRMWARE_CHUNK

    # --chunk-size without --split-frames implies splitting
    if args.chunk_size and not args.split_frames:
        split_enabled = True

    if not args.split_frames and not args.chunk_size and oversized_count > 0:
        # Interactive: show analysis and ask
        print()
        print(f"  {YELLOW}{BOLD}Stream has {oversized_count} frame(s) exceeding WiFi MTU{RESET}")
        print(f"  {DIM}Largest frame: {max_frame_size}B — WiFi safe limit: ~{_FIRMWARE_CHUNK}B{RESET}")

        split_enabled = arrow_pick("Split oversized frames?", [
            (f"No  — send as-is {DIM}(may cause IP fragmentation){RESET}", False),
            (f"Yes — split into smaller UDP packets {DIM}(recommended for WiFi){RESET}", True),
        ])

        if split_enabled:
            # Build chunk size options dynamically from detected values
            fw_label = f"{_FIRMWARE_CHUNK}B — CockpitOS firmware UDP_MAX_SIZE"
            if _FIRMWARE_FOUND:
                fw_label += f" {DIM}(from Config.h, recommended){RESET}"
            else:
                fw_label += f" {DIM}(default — Config.h not found){RESET}"

            chunk_options = [
                (fw_label, _FIRMWARE_CHUNK),
            ]

            if _DCSBIOS_PAYLOAD is not None and _DCSBIOS_PAYLOAD != _FIRMWARE_CHUNK:
                chunk_options.append(
                    (f"{_DCSBIOS_PAYLOAD}B — DCS-BIOS MAX_PAYLOAD_SIZE {DIM}(default, causes IP fragmentation){RESET}",
                     _DCSBIOS_PAYLOAD),
                )

            chunk_options.append(
                (f"Custom {DIM}(enter a value from 1 to 2048){RESET}", -1),
            )

            chunk_size = arrow_pick("Chunk size:", chunk_options)

            if chunk_size == -1:
                # Custom value entry
                print()
                while True:
                    sys.stdout.write(f"  Enter chunk size (1-2048): ")
                    sys.stdout.flush()
                    val_str = ""
                    while True:
                        ch = msvcrt.getwch()
                        if ch == "\r":
                            print()
                            break
                        elif ch == "\b":
                            if val_str:
                                val_str = val_str[:-1]
                                sys.stdout.write("\b \b")
                                sys.stdout.flush()
                        elif ch.isdigit() and len(val_str) < 4:
                            val_str += ch
                            sys.stdout.write(ch)
                            sys.stdout.flush()
                    try:
                        chunk_size = int(val_str)
                        if 1 <= chunk_size <= 2048:
                            break
                        print(f"  {RED}Value must be between 1 and 2048{RESET}")
                    except ValueError:
                        print(f"  {RED}Invalid number{RESET}")

    # Recompute oversized count against chosen chunk size (might differ from _FIRMWARE_CHUNK)
    if split_enabled:
        oversized_count, _ = analyze_frames(frames, chunk_size)

    # === 5. SETUP SOCKETS ===
    multicast_sockets = []

    for iface_name, ip in selected_interfaces:
        try:
            sock = create_multicast_socket(ip)
            multicast_sockets.append((ip, iface_name, sock))
        except Exception:
            pass  # Silently skip interfaces that fail

    if not multicast_sockets:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
            multicast_sockets.append(("default", "default route", sock))
        except Exception as e:
            print(f"\nNo usable network interfaces found! ({e})")
            sys.exit(1)

    # === 6. CLEAR SCREEN & STATUS BOX ===
    print("\033[2J\033[H", end="", flush=True)

    iface_list = ", ".join(f"{ip} ({name})" for ip, name, _ in multicast_sockets)

    if args.raw_timing:
        timing_label = f"recorded x{args.speed}"
    else:
        timing_label = f"{args.fps:.0f} FPS ({1000/args.fps:.1f}ms/frame)"

    print()
    print("+----------------------------------------------------------------+")
    _box_line(f"  {BOLD}CockpitOS DCS-BIOS Stream Replay{RESET}")
    print("+----------------------------------------------------------------+")
    _box_line(f"  Stream:    {CYAN}{display_name}{RESET}")
    _box_line(f"  Frames:    {CYAN}{len(frames)}{RESET} {DIM}(max {max_frame_size}B){RESET}")
    _box_line(f"  Target:    {CYAN}{MULTICAST_IP}:{UDP_PORT}{RESET}")
    _box_line(f"  Interface: {CYAN}{iface_list}{RESET}")
    _box_line(f"  Timing:    {CYAN}{timing_label}{RESET}")
    if oversized_count > 0:
        if split_enabled:
            _box_line(f"  Split:     {GREEN}ON{RESET} — {oversized_count} frame(s) >{chunk_size}B will be chunked")
        else:
            _box_line(f"  Split:     {YELLOW}OFF{RESET} — {oversized_count} frame(s) >{_FIRMWARE_CHUNK}B (may fragment)")
    else:
        if split_enabled:
            _box_line(f"  Split:     {GREEN}ON{RESET} {DIM}(no frames >{chunk_size}B){RESET}")
        else:
            _box_line(f"  Split:     {DIM}N/A — no frames >{_FIRMWARE_CHUNK}B{RESET}")
    _box_line(f"  {DIM}Press Ctrl+C to stop{RESET}")
    print("+----------------------------------------------------------------+")
    print()

    # === 7. REPLAY LOOP ===
    frame_timestamps = []
    accum_time = 0.0
    for frame in frames:
        frame_timestamps.append(accum_time)
        if args.raw_timing:
            accum_time += frame.get("timing", 0) / args.speed
        else:
            accum_time += 1.0 / args.fps

    stream_start_time = time.perf_counter()
    cycle = 0

    try:
        while True:
            for i, frame in enumerate(frames):
                hex_data = frame.get("data", "")
                if not hex_data:
                    continue

                target_time = stream_start_time + frame_timestamps[i] + (cycle * accum_time)
                now = time.perf_counter()
                wait_time = target_time - now

                if wait_time > 0:
                    time.sleep(wait_time)

                try:
                    binary_data = binascii.unhexlify(hex_data)

                    if split_enabled and len(binary_data) > chunk_size:
                        # Protocol-aware split: cut at DCS-BIOS data block
                        # boundaries so each sub-frame is structurally valid.
                        # Each sub-frame gets a SYNC header; large blocks are
                        # split into sub-blocks with sequential addresses.
                        sub_frames = split_frame_protocol_aware(binary_data, chunk_size)
                        for sub_frame in sub_frames:
                            for _, _, sock in multicast_sockets:
                                sock.sendto(sub_frame, (MULTICAST_IP, UDP_PORT))
                    else:
                        for _, _, sock in multicast_sockets:
                            sock.sendto(binary_data, (MULTICAST_IP, UDP_PORT))

                except Exception as e:
                    print(f"  {RED}[ERROR]{RESET} {e}")

            cycle += 1
            print(f"  {GREEN}Completed cycle {cycle}{RESET}")

    except KeyboardInterrupt:
        print("\n\nStopped by user")

    finally:
        for _, _, sock in multicast_sockets:
            try:
                sock.close()
            except Exception:
                pass


if __name__ == "__main__":
    main()
