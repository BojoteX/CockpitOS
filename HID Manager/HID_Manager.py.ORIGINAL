#!/usr/bin/env python3
"""
CockpitOS HID Manager v5.1 — Driverless Bidirectional USB Bridge
==================================================================

This script is the **host-side half** of CockpitOS's driverless USB HID
communication system.  It bridges DCS World (via DCS-BIOS UDP multicast)
and one or more ESP32 cockpit panels (via standard USB HID reports),
eliminating the need for any custom driver installation on Windows.

Architecture — "Mailbox" Bidirectional HID
-------------------------------------------
Instead of using a CDC/serial USB class (which requires signed drivers on
Windows), CockpitOS presents each ESP32 panel as a **standard USB gamepad**
whose HID descriptor (defined in firmware at src/CustomDescriptors/
BidireccionalNew.h) exposes three report pipes:

  ┌────────────────┐         USB HID (driverless)         ┌──────────────┐
  │  This Script   │ ◄──── Input  Report (64 B) ──────── │  ESP32 Panel │
  │  (HID Manager) │ ────► Output Report (64 B) ────────► │  (CockpitOS) │
  │                │ ◄──►  Feature Report (64 B) ◄──────► │              │
  └────────────────┘                                      └──────────────┘

  • Input  Report  (device → host) : gamepad axes + buttons (16×16-bit axes,
                                      32 buttons, 28 B padding = 64 B).
                                      Acts as a **doorbell**: any state change
                                      tells us to drain the Feature mailbox.
  • Output Report  (host → device) : DCS-BIOS export stream.  UDP frames are
                                      sliced into 64-byte chunks and written
                                      here via hid.write().  Firmware receives
                                      them in GPDevice::_onOutput()
                                      (src/HIDDescriptors.h).
  • Feature Report (bidirectional) : "Mailbox" FIFO for ASCII DCS-BIOS
                                      commands (panel → DCS) and the initial
                                      handshake.  Host polls via GET_FEATURE
                                      after each Input Report doorbell.
                                      Firmware side: GPDevice::_onGetFeature()
                                      drains dcsRawUsbOutRingbuf
                                      (src/Core/RingBuffer.cpp);
                                      GPDevice::_onSetFeature() pushes into it.

Data Flow
---------
  DCS → Panels (export data):
    1. DCS-BIOS sends UDP multicast to 239.255.50.10:5010
    2. _udp_rx_processor() receives the frame via blocking recvfrom()
    3. Frame is sliced into 64-byte Output Reports (prepended with 0x00
       report-ID byte required by hidapi)
    4. Reports are fanned out to per-device _DeviceTxWorker threads
    5. Each worker calls hid.write() — received by GPDevice::_onOutput()
       on firmware, which pushes into dcsUdpRingbuf for parsing

  Panels → DCS (button/axis commands):
    1. Panel firmware updates gamepad state → sends Input Report
       (via GPDevice::sendReport in src/HIDDescriptors.h)
    2. device_reader() wakes from blocking hid.read()
    3. Reader immediately drains Feature Reports via get_feature_report()
       — each contains an ASCII DCS-BIOS command like "UFC_1 1\n"
       (firmware queues these in dcsRawUsbOutRingbuf via
       src/Core/RingBuffer.cpp:dcsRawUsbOutRingbufPushChunked)
    4. Commands are forwarded to DCS-BIOS via UDP port 7778

Threading Model (zero-contention design)
-----------------------------------------
  Main Thread ............ curses UI (ConsoleUI._loop), 100 ms refresh
  Device Monitor Thread .. hotplug scan every HOTPLUG_INTERVAL_S (3 s)
  Stats Updater Thread ... 1-second rolling stats for the UI
  UDP RX Thread .......... blocking recvfrom(), slices + fans out
  Per-device Reader ...... blocking hid.read() + Feature drain (1 per panel)
  Per-device TX Worker ... Condition-variable-driven hid.write() (1 per panel)

  CRITICAL INVARIANT: each HID device handle has exactly ONE reader thread
  and ONE writer thread.  hidapi is not thread-safe per-handle, so this
  single-owner model eliminates the need for any locks around I/O calls.
  The only lock in the system (device_lock) protects the device *list*,
  never an individual HID handle.

Key Design Decisions
--------------------
  1. NO LOCKS during HID I/O — proven correct; any locking around hidapi
     causes intermittent freezes on Windows.
  2. Blocking I/O everywhere — threads sleep in kernel, near-zero CPU.
  3. Condition variable for TX wake — no polling loop, no spinlock.
  4. Multicast join on ALL interfaces — fixes Windows multi-NIC setups
     where DCS multicast only arrives on the "wrong" adapter.
  5. Single-instance file lock — prevents two managers fighting over
     the same HID handles.

Firmware Cross-References
-------------------------
  Config.h .............. USB_VID (0xCAFE), USB_PID (per label set),
                          HID_SENDREPORT_TIMEOUT (5 ms)
  src/HIDDescriptors.h .. GPDevice class (_onOutput, _onGetFeature,
                          _onSetFeature, sendReport), handshake constants
  src/CustomDescriptors/
    BidireccionalNew.h .. HID report descriptor (64 B Input/Output/Feature)
  src/Core/RingBuffer.cpp Ring buffers: dcsUdpRingbuf (DCS→device),
                          dcsRawUsbOutRingbuf (device→host commands)
  src/Core/HIDManager.cpp HIDManager_dispatchReport(), axis/button handling
  CockpitOS.ino ......... mainLoopStarted flag (gates firmware-side I/O)

Author: CockpitOS Project
License: MIT
"""

from __future__ import annotations

import sys
import os
import time
import socket
import struct
import threading
import queue
import configparser
import ipaddress
from collections import deque
from datetime import datetime
from typing import Optional, List, Dict, Tuple, Any

# ══════════════════════════════════════════════════════════════════════════════
# BOOTSTRAP AND PLATFORM DETECTION
# ══════════════════════════════════════════════════════════════════════════════

# Resolve the directory this script lives in.  Used as the base path for
# settings.ini and the single-instance lock file, so the manager can be
# launched from any working directory.
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

# Platform flags — used to select the correct pip package name for curses
# (windows-curses on Windows, built-in on Linux/macOS).
IS_WINDOWS = (os.name == 'nt') or sys.platform.startswith('win')
IS_LINUX = sys.platform.startswith('linux')
IS_MACOS = sys.platform == 'darwin'

# ══════════════════════════════════════════════════════════════════════════════
# DEPENDENCY CHECK
# ══════════════════════════════════════════════════════════════════════════════
# Validates that all required third-party modules are installed before any
# real work begins.  If anything is missing, prints a pip install command
# and exits cleanly — this avoids confusing ImportError tracebacks for
# end users who aren't Python-savvy.
#
# Module map: { import_name: pip_package_name }
#   hid      → hidapi   : USB HID communication (read/write/feature reports)
#   filelock → filelock  : cross-process single-instance locking
#   curses   → windows-curses (Windows only) : terminal UI
#   ifaddr   → ifaddr   : enumerate network interfaces for multicast joins

REQUIRED_MODULES = {
    "hid": "hidapi",
    "filelock": "filelock",
    "curses": "windows-curses" if IS_WINDOWS else None,
    "ifaddr": "ifaddr",
}

missing = []
for mod, pip_name in REQUIRED_MODULES.items():
    try:
        __import__(mod)
    except ImportError:
        if pip_name:
            missing.append(pip_name)
        else:
            missing.append(mod)

if missing:
    print("Missing required modules:")
    for m in missing:
        print(f"  - {m}")
    to_pip = [m for m in missing if m not in ("curses",)]
    if to_pip:
        print(f"\nInstall with: pip install {' '.join(to_pip)}")
    input("\nPress Enter to exit...")
    sys.exit(1)

import hid          # USB HID device enumeration, open, read, write, feature reports
import curses       # Terminal UI (ncurses wrapper)
import ifaddr       # Network interface enumeration for multicast
from filelock import FileLock, Timeout  # Cross-process file-based locking

# ══════════════════════════════════════════════════════════════════════════════
# SINGLE INSTANCE LOCK
# ══════════════════════════════════════════════════════════════════════════════
# Prevents multiple copies of this manager from running simultaneously.
# This is critical because hidapi's hid_open() would fail (or worse, succeed
# with interleaved reads/writes) if two processes held the same HID handle.
# Uses a file-based lock (hid_manager.lock) that survives terminal crashes
# — the filelock library automatically releases on process exit.

LOCKFILE = os.path.join(SCRIPT_DIR, "hid_manager.lock")
_instance_lock: Optional[FileLock] = None


def acquire_instance_lock() -> FileLock:
    """Attempt to acquire the single-instance lock with a 100 ms timeout.
    If another instance already holds the lock, print an error and exit.
    Called once at startup from main()."""
    global _instance_lock
    try:
        _instance_lock = FileLock(LOCKFILE)
        _instance_lock.acquire(timeout=0.1)
        return _instance_lock
    except Timeout:
        print("ERROR: Another instance of CockpitOS HID Manager is already running.")
        input("Press Enter to exit...")
        sys.exit(1)


def release_instance_lock() -> None:
    """Release the single-instance lock if held.
    Called in the finally block of main() to ensure cleanup on any exit path."""
    global _instance_lock
    if _instance_lock is not None:
        try:
            _instance_lock.release()
        except Exception:
            pass
        _instance_lock = None

# ══════════════════════════════════════════════════════════════════════════════
# CONFIGURATION CONSTANTS
# ══════════════════════════════════════════════════════════════════════════════

# --- Protocol constants (must match firmware) --------------------------------

# HID report payload size.  Matches the descriptor in
# src/CustomDescriptors/BidireccionalNew.h: 64-byte Input, Output, and
# Feature reports.  Also matches DCS_UDP_PACKET_MAXLEN in Config.h when
# USE_DCSBIOS_USB is enabled.
DEFAULT_REPORT_SIZE    = 64

# DCS-BIOS export multicast group.  DCS-BIOS broadcasts cockpit state on
# this address.  The firmware listens on the same group when using WiFi
# transport (src/Core/WiFiDebug.cpp: udp.listenMulticast(239.255.50.10, 5010)).
# In USB mode, this script acts as the multicast listener instead.
DEFAULT_MULTICAST_IP   = "239.255.50.10"
DEFAULT_UDP_PORT       = 5010

# DCS-BIOS command port.  Panel commands (e.g. "UFC_1 1\n") are sent here.
# Firmware defines this in src/WiFiDebug.h as DCS_REMOTE_PORT = 7778.
DEFAULT_DCS_TX_PORT    = 7778

# Handshake token.  Sent via SET_FEATURE to the firmware's ring buffer
# (GPDevice::_onSetFeature in src/HIDDescriptors.h), then read back via
# GET_FEATURE (GPDevice::_onGetFeature).  The round-trip proves the Feature
# Report pipe is functional end-to-end.
# Firmware defines: FEATURE_HANDSHAKE_REQ "DCSBIOS-HANDSHAKE" in
# src/HIDDescriptors.h:4.
# NOTE [REVIEW]: Firmware also defines FEATURE_HANDSHAKE_RESP "DCSBIOS-READY"
# (src/HIDDescriptors.h:5) but it is never used anywhere — the handshake
# just echoes the request back.  Consider whether the firmware should send
# DCSBIOS-READY instead to confirm the main loop is running (mainLoopStarted).
HANDSHAKE_REQ          = b"DCSBIOS-HANDSHAKE"

# Feature Report ID.  The descriptor in BidireccionalNew.h declares NO
# Report IDs (all reports are the default ID 0).  hidapi requires us to
# prepend the report ID byte to send_feature_report / get_feature_report.
FEATURE_REPORT_ID      = 0

# --- Tunable constants -------------------------------------------------------

MAX_DEVICES            = 32    # Max simultaneous panels (soft limit for UI)
MAX_HANDSHAKE_ATTEMPTS = 300   # ~60 s at 0.2 s sleep per attempt
MAX_FEATURE_DRAIN      = 64   # Max Feature Reports to drain per Input trigger
IDLE_TIMEOUT           = 2.0   # (Currently unused — reserved for future idle detection)
LOG_KEEP               = 2000  # Max log lines kept in the UI deque
HOTPLUG_INTERVAL_S     = 3    # Seconds between hid.enumerate() scans

# Settings file path (same directory as this script)
SETTINGS_PATH = os.path.join(SCRIPT_DIR, "settings.ini")

# ══════════════════════════════════════════════════════════════════════════════
# MULTI-INTERFACE SUPPORT
# ══════════════════════════════════════════════════════════════════════════════
# On Windows, multicast group membership is per-interface.  If DCS-BIOS
# sends multicast on the Ethernet adapter but the OS default route goes
# through WiFi, a simple INADDR_ANY join only subscribes the WiFi adapter
# and the export stream is never received.  This section discovers all
# usable IPv4 interfaces so _join_multicast_all_interfaces() can subscribe
# on every one of them.

def get_all_interface_ips() -> List[Tuple[str, str]]:
    """Discover all usable IPv4 interface addresses on this machine.
    Returns list of (interface_name, ip_address) tuples.

    Filters out:
      - Loopback (127.x.x.x)  — DCS-BIOS never sends on loopback
      - APIPA/link-local (169.254.x.x) — indicates no DHCP, not useful
    """
    interfaces = []

    for adapter in ifaddr.get_adapters():
        for ip in adapter.ips:
            # ifaddr returns str for IPv4, tuple for IPv6 — we only want IPv4
            if isinstance(ip.ip, str):
                if ip.ip.startswith('127.') or ip.ip.startswith('169.254.'):
                    continue
                short_name = adapter.nice_name[:30]
                interfaces.append((short_name, ip.ip))

    return interfaces

# ══════════════════════════════════════════════════════════════════════════════
# SETTINGS
# ══════════════════════════════════════════════════════════════════════════════
# Persistent configuration via settings.ini.  On first run the file is
# auto-created with safe defaults.  The DCS source IP is auto-learned from
# the first valid UDP packet and persisted so subsequent launches remember
# the correct address.

def read_settings() -> Tuple[int, Optional[int], str]:
    """Read VID, optional PID, and DCS source IP from settings.ini.
    Creates the file with defaults if it doesn't exist.

    Returns: (vid, pid_or_None, dcs_ip_str)

    VID must match Config.h → USB_VID (default 0xCAFE).
    PID is optional — if omitted, all devices with matching VID are accepted.
    Each CockpitOS label set generates a unique PID (e.g. LABEL_SET_MAIN →
    0xC8DD), so leaving PID blank lets this manager handle all panel types.
    """
    config = configparser.ConfigParser()
    if not os.path.isfile(SETTINGS_PATH):
        config['USB'] = {'VID': '0xCAFE'}
        config['DCS'] = {'UDP_SOURCE_IP': '127.0.0.1'}
        config['MAIN'] = {'CONSOLE': '1'}
        with open(SETTINGS_PATH, 'w') as f:
            config.write(f)
    config.read(SETTINGS_PATH)

    try:
        vid = int(config['USB']['VID'], 0)
    except Exception:
        vid = 0xCAFE

    try:
        pid = int(config['USB'].get('PID', ''), 0)
    except Exception:
        pid = None  # None = accept any PID from this VID

    try:
        dcs_ip = config['DCS'].get('UDP_SOURCE_IP', '127.0.0.1')
    except Exception:
        dcs_ip = '127.0.0.1'

    return vid, pid, dcs_ip


def write_settings_dcs_ip(new_ip: str) -> None:
    """Persist the auto-learned DCS source IP to settings.ini.
    Called once per session when the first valid UDP packet arrives
    (see _udp_rx_processor)."""
    config = configparser.ConfigParser()
    config.read(SETTINGS_PATH)
    if 'DCS' not in config:
        config['DCS'] = {}
    config['DCS']['UDP_SOURCE_IP'] = new_ip
    with open(SETTINGS_PATH, 'w') as f:
        config.write(f)


def is_valid_ipv4(ip: str) -> bool:
    """Validate that a string is a usable unicast IPv4 address.
    Rejects multicast (224.0.0.0/4) and unspecified (0.0.0.0) to avoid
    accidentally learning bogus source addresses from broadcast packets."""
    try:
        ip_obj = ipaddress.ip_address(ip)
        return (
            isinstance(ip_obj, ipaddress.IPv4Address)
            and not ip_obj.is_multicast
            and not ip_obj.is_unspecified
        )
    except Exception:
        return False


# Load settings at module level — used by all threads.
USB_VID, USB_PID, STORED_DCS_IP = read_settings()

# ── Global Shared State ─────────────────────────────────────────────────────
# These module-level variables are shared across threads.  Access patterns
# are designed to minimise locking:
#
#   stats / global_stats_lock : only the UDP RX thread writes; the stats
#       updater thread reads under lock.  Low contention (1 Hz reads).
#
#   reply_addr : mutable single-element list used as a thread-safe reference.
#       Written once by _udp_rx_processor when the DCS source IP is learned;
#       read by device_reader threads and udp_send_report.
#       NOTE [REVIEW]: This relies on CPython's GIL for atomicity.  Under
#       alternative Python runtimes (PyPy, GraalPy) a single-element list
#       mutation is not guaranteed atomic.  In practice this is fine since
#       the project targets CPython on Windows.  If portability is ever
#       needed, replace with threading.Event + a lock-protected string.
#
#   prev_reconnections : tracks how many times each serial_number has been
#       seen across hotplug cycles.  Only written by _device_monitor.

stats = {
    "frame_count_total": 0,    # Cumulative frame count (never reset)
    "frame_count_window": 0,   # Frames in the current 1-second window (reset each tick)
    "bytes": 0,                # Bytes in the current 1-second window (reset each tick)
    "start_time": time.time(), # Window start timestamp (reset each tick)
    "bytes_rolling": 0,        # Bytes in the current window (for avg frame calc)
    "frames_rolling": 0,       # Frames in the current window (for avg frame calc)
}
global_stats_lock = threading.Lock()

reply_addr = [STORED_DCS_IP]

prev_reconnections: Dict[str, int] = {}

# ══════════════════════════════════════════════════════════════════════════════
# DEVICE ENTRY
# ══════════════════════════════════════════════════════════════════════════════
# Represents a single connected CockpitOS panel.  One DeviceEntry is created
# per USB device discovered by _device_monitor, and it lives until the device
# is physically disconnected or the manager shuts down.

def is_bt_serial(s: str) -> bool:
    """Detect if a serial string is actually a Bluetooth MAC address.
    CockpitOS supports BLE transport (src/BLEManager.h) alongside USB.
    When a BLE device is enumerated via hidapi, its serial appears as a
    MAC address (e.g. "AA:BB:CC:DD:EE:FF" or "bx-0011223344556677").
    In that case we prefer the product_string as the display name since
    the MAC is not human-friendly."""
    import re
    return bool(re.fullmatch(
        r'(?i)(?:[0-9a-f]{2}[:-]){5}[0-9a-f]{2}|[0-9a-f]{12}|bx-[0-9a-f]{16}', s))


class DeviceEntry:
    """State container for a single connected CockpitOS panel.

    Attributes:
        dev ............. hidapi device handle (hid.device) — opened via
                          hid.open_path() in _device_monitor.
        info ............ dict from hid.enumerate() for this device.
        name ............ Human-readable display name for the UI.
                          For USB devices this is the serial_number, which
                          matches USB_SERIAL in firmware Config.h (set per
                          label set, e.g. "FA18-MAIN", "FA18-IFEI").
                          For BLE devices it's the product_string instead.
        status .......... Current lifecycle state shown in the UI:
                          "WAIT HANDSHAKE" → "READY" → "DISCONNECTED"
        last_sent ....... Timestamp of last TX write (currently informational).
        disconnected .... Set True to signal all threads to stop I/O on
                          this device.  Checked by device_reader,
                          _DeviceTxWorker, and _device_monitor.
        handshaked ...... True once the Feature Report handshake completes.
                          Gates TX worker creation and data flow.
        reconnections ... Count of how many times this serial has been
                          re-enumerated (survives across hotplug cycles
                          via prev_reconnections dict).
    """

    def __init__(self, dev, dev_info: dict):
        self.dev = dev
        self.info = dev_info

        # Name resolution: prefer serial_number (which is the firmware's
        # USB_SERIAL / LABEL_SET_STR), but fall back to product_string
        # for BLE devices where the serial is a MAC address.
        serial = dev_info.get('serial_number', '') or ''
        product = dev_info.get('product_string', '') or ''
        self.name = (product or serial) if is_bt_serial(serial) else (serial or product)
        if not self.name:
            self.name = f"Device-{id(dev) & 0xFFFF:04X}"

        self.status = "WAIT HANDSHAKE"
        self.last_sent = time.time()
        self.disconnected = False
        self.handshaked = False
        self.reconnections = 0

    def get_key(self) -> str:
        """Unique identity key for this device.  Uses serial_number when
        available, falls back to the OS-level HID path.
        NOTE [REVIEW]: _device_monitor uses serial_number directly (not
        this method) for identity checks.  If two panels share a blank or
        default serial, the monitor would treat them as duplicates and
        skip the second.  In practice all CockpitOS panels have unique
        serials set via LABEL_SET_STR in their LabelSetConfig.h.  If
        unconfigured boards are ever connected, using get_key() (which
        falls back to path) would be more robust."""
        return self.info.get('serial_number', '') or str(self.info.get('path', b''))

# ══════════════════════════════════════════════════════════════════════════════
# HID FUNCTIONS
# ══════════════════════════════════════════════════════════════════════════════
# Core USB HID operations: device discovery, handshake, and error recovery.

def list_target_devices() -> List[dict]:
    """Enumerate all HID devices that match our VID (and optionally PID).

    Calls hidapi's hid_enumerate() which scans the OS HID subsystem.
    On Windows this is a relatively expensive call (~5–15 ms per scan),
    so it's only called every HOTPLUG_INTERVAL_S seconds from
    _device_monitor, not in a tight loop.

    VID is matched against USB_VID from Config.h (default 0xCAFE).
    PID is only checked if explicitly set in settings.ini — otherwise
    all CockpitOS label sets are accepted (each has a unique auto-generated
    PID defined in their LabelSetConfig.h)."""
    devices = []
    for d in hid.enumerate():
        if d['vendor_id'] != USB_VID:
            continue
        if USB_PID and d['product_id'] != USB_PID:
            continue
        devices.append(d)
    return devices


def try_fifo_handshake(dev, uiq: Optional[queue.Queue] = None,
                       device_name: Optional[str] = None) -> bool:
    """Perform the Feature Report "mailbox" handshake with a panel.

    Protocol:
      1. GET_FEATURE — check if the firmware already has a handshake token
         buffered (e.g. from a previous session that didn't drain it).
      2. SET_FEATURE — write HANDSHAKE_REQ ("DCSBIOS-HANDSHAKE") into the
         firmware's ring buffer (GPDevice::_onSetFeature →
         dcsRawUsbOutRingbufPushChunked in src/Core/RingBuffer.cpp).
      3. GET_FEATURE — read back the token we just wrote.  If we get
         HANDSHAKE_REQ back, the Feature Report pipe is proven functional
         end-to-end.

    This "echo" approach works because _onSetFeature pushes into
    dcsRawUsbOutRingbuf, and _onGetFeature pops from the same buffer.
    So we're writing a token and reading it back through the firmware's
    ring buffer — confirming the entire USB Feature Report path works.

    NOTE [REVIEW]: The firmware defines FEATURE_HANDSHAKE_RESP
    "DCSBIOS-READY" (src/HIDDescriptors.h:5) but never sends it.  The
    handshake currently proves the USB pipe works but doesn't confirm the
    firmware's main loop is running (mainLoopStarted).  In practice the
    main loop starts well before USB enumeration completes so this is not
    a problem.  A future improvement could have the firmware respond with
    DCSBIOS-READY to provide a stronger guarantee.

    Timeout: MAX_HANDSHAKE_ATTEMPTS × 0.2 s ≈ 60 seconds.

    Returns True on successful handshake, False on timeout.
    """
    # Pad the handshake token to exactly 64 bytes (report size)
    payload = HANDSHAKE_REQ.ljust(DEFAULT_REPORT_SIZE, b'\x00')
    attempts = 0

    while attempts < MAX_HANDSHAKE_ATTEMPTS:
        # Step 1: Check if firmware already has a buffered handshake token
        # (handles reconnection where a previous token was never drained)
        try:
            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
            # hidapi prepends the report ID byte; strip it if present
            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
            msg = d.rstrip(b'\x00')
            if msg == HANDSHAKE_REQ:
                return True
        except Exception as e:
            if uiq:
                uiq.put(('handshake', device_name, f"GET FEATURE exception: {e}"))
            time.sleep(0.2)
            continue

        # Step 2: Write our handshake token into the firmware's ring buffer
        # hidapi requires the report ID as the first byte of the payload
        try:
            dev.send_feature_report(bytes([FEATURE_REPORT_ID]) + payload)
        except Exception as e:
            if uiq:
                uiq.put(('handshake', device_name, f"SET FEATURE exception: {e}"))
            time.sleep(0.2)
            continue

        # Step 3: Read the token back — confirms end-to-end Feature pipe
        try:
            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
            msg = d.rstrip(b'\x00')
            if msg == HANDSHAKE_REQ:
                return True
        except Exception as e:
            if uiq:
                uiq.put(('handshake', device_name, f"GET FEATURE exception: {e}"))
            time.sleep(0.2)
            continue

        attempts += 1
        if attempts % 10 == 0 and uiq:
            uiq.put(('handshake', device_name, "Waiting for handshake..."))
        time.sleep(0.2)

    return False  # Timeout — device did not respond within ~60 seconds


def _close_stale_handle(entry: DeviceEntry, uiq: queue.Queue, where: str, exc) -> None:
    """Mark a device as disconnected and close its HID handle.

    Called whenever any HID operation throws an exception — this is the
    uniform error recovery path.  Setting entry.disconnected = True signals
    both the reader thread and the TX worker to exit their loops.
    The _device_monitor will subsequently remove the entry from the
    device list and the UI will show "STALE HANDLE (<location>)".

    Args:
        entry: The device that experienced the error.
        where: Human-readable label for the failing operation
               (e.g. "READ", "FEATURE", "HANDSHAKE").
        exc:   The exception that triggered the recovery.
    """
    try:
        entry.dev.close()
    except Exception:
        pass
    entry.disconnected = True
    uiq.put(('status', entry.name, f"STALE HANDLE ({where})"))
    uiq.put(('log', entry.name, f"[stale] {where} exception: {exc}"))

# ══════════════════════════════════════════════════════════════════════════════
# DEVICE READER
# ══════════════════════════════════════════════════════════════════════════════
# One reader thread per connected panel.  Handles the full device lifecycle:
#   1. Feature Report handshake
#   2. Wait for DCS to be detected (reply_addr populated)
#   3. Drain any stale Feature Reports from previous session
#   4. Main loop: blocking Input Report read → Feature Report mailbox drain
#
# THREAD SAFETY INVARIANT:
#   This is the ONLY thread that calls dev.read() and dev.get_feature_report()
#   on this device's handle.  The corresponding _DeviceTxWorker is the ONLY
#   thread that calls dev.write().  hidapi is not thread-safe per-handle,
#   but read and write target different USB endpoints (IN vs OUT), so this
#   exclusive-ownership model avoids all contention without any locks.

def device_reader(entry: DeviceEntry, uiq: queue.Queue, udp_send) -> None:
    """Per-device reader thread.  Runs for the lifetime of a single device
    connection — from handshake through normal operation until disconnect.

    Args:
        entry:    DeviceEntry for this panel.
        uiq:      UI event queue (for status updates and log messages).
        udp_send: Callable to forward commands to DCS-BIOS (UDP port 7778).
                  This is NetworkManager.udp_send_report, bound at thread
                  creation in _device_monitor.
    """
    dev = entry.dev

    try:
        # ── Phase 1: Feature Report Handshake ──────────────────────────
        # Confirms the USB Feature Report pipe is functional.
        # See try_fifo_handshake() for the full protocol description.
        while not entry.handshaked and not entry.disconnected:
            entry.handshaked = try_fifo_handshake(dev, uiq=uiq, device_name=entry.name)
            if not entry.handshaked:
                _close_stale_handle(entry, uiq, "HANDSHAKE", "no reply")
                return
            uiq.put(('status', entry.name, "READY"))
            uiq.put(('log', entry.name, "Handshake complete, ready to process input."))
            entry.status = "READY"

        # ── Phase 2: Wait for DCS ──────────────────────────────────────
        # Block until _udp_rx_processor has received at least one valid
        # UDP packet and populated reply_addr[0] with the DCS source IP.
        # This prevents sending commands to a stale/unconfigured address.
        if reply_addr[0] is None and not entry.disconnected:
            uiq.put(('log', entry.name, "Waiting for DCS mission start..."))
        while reply_addr[0] is None and not entry.disconnected:
            time.sleep(0.2)
        if entry.disconnected:
            return

        uiq.put(('log', entry.name, f"DCS detected on {reply_addr[0]} — Starting normal operation."))

        # ── Phase 3: Drain stale Feature Reports ──────────────────────
        # After a reconnection the firmware's dcsRawUsbOutRingbuf may
        # contain leftover commands from the previous session.  We drain
        # them here (up to 500 attempts) until we get an all-zero response,
        # which means the ring buffer is empty.
        # Firmware side: GPDevice::_onGetFeature() returns memset(0) when
        # dcsRawUsbOutRingbufPending() == 0 (src/HIDDescriptors.h:57-58).
        cleared = False
        attempt = 0
        while not cleared and not entry.disconnected and attempt < 500:
            try:
                resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                if not any(d):
                    cleared = True  # All zeros = ring buffer empty
            except Exception as e:
                _close_stale_handle(entry, uiq, "FEATURE-DRAIN", e)
                return
            attempt += 1
        if not cleared:
            _close_stale_handle(entry, uiq, "FEATURE-DRAIN", "timeout")
            return

        # ── Phase 4: Main Loop (Doorbell + Mailbox Drain) ─────────────
        # The core of the bidirectional protocol:
        #
        #   1. BLOCKING hid.read() waits for an Input Report (gamepad
        #      state change).  This is the "doorbell" — the firmware sends
        #      an Input Report via HIDManager_dispatchReport() whenever a
        #      button/axis changes AND whenever it has queued a command in
        #      dcsRawUsbOutRingbuf.
        #
        #   2. When the doorbell rings, we immediately drain the Feature
        #      Report mailbox.  Each GET_FEATURE returns one 64-byte
        #      message from dcsRawUsbOutRingbuf containing an ASCII
        #      DCS-BIOS command (e.g. "UFC_1 1", "IFEI_BRIGHTNESS_UP +3200").
        #      We drain until we get an empty response or hit MAX_FEATURE_DRAIN.
        #
        #   3. Each extracted command is forwarded to DCS-BIOS via UDP
        #      port 7778 through udp_send().
        #
        # NOTE [REVIEW]: dev.read(timeout_ms=-1) blocks indefinitely.  If
        # the USB device is physically disconnected but the OS hasn't
        # released the handle yet (can happen with USB hubs), this thread
        # will block in the kernel until the OS cleans up.  On Windows with
        # direct USB connections this is not an issue — the OS releases
        # handles promptly and the exception handler fires.  For added
        # robustness, a finite timeout (e.g. 1000 ms) with a loop checking
        # entry.disconnected could prevent zombie threads in edge cases.
        while not entry.disconnected and entry.handshaked:
            try:
                # BLOCKING read — thread sleeps in kernel, zero CPU
                data = dev.read(DEFAULT_REPORT_SIZE, timeout_ms=-1)
                if not data:
                    continue
            except Exception as e:
                _close_stale_handle(entry, uiq, "READ", e)
                return

            # Drain the Feature Report mailbox (firmware → host commands)
            drain = 0
            while not entry.disconnected and drain < MAX_FEATURE_DRAIN:
                try:
                    resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                    d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                    msg = d.rstrip(b'\x00').decode(errors="replace").strip()
                    # Empty response or handshake echo = nothing left to drain
                    if not msg or msg == HANDSHAKE_REQ.decode():
                        break
                    # Forward the ASCII command to DCS-BIOS via UDP
                    uiq.put(('log', entry.name, f"IN: {msg}"))
                    udp_send(msg + "\n")
                    drain += 1
                except Exception as e:
                    _close_stale_handle(entry, uiq, "FEATURE", e)
                    return

    finally:
        # Ensure the HID handle is always closed, even on unexpected exceptions
        try:
            dev.close()
        except Exception:
            pass

# ══════════════════════════════════════════════════════════════════════════════
# NETWORK MANAGER
# ══════════════════════════════════════════════════════════════════════════════
# Manages the UDP ↔ HID bridge:
#   - Receives DCS-BIOS export data via UDP multicast (239.255.50.10:5010)
#   - Slices each UDP frame into 64-byte HID Output Reports
#   - Fans them out to per-device TX workers for writing via hid.write()
#   - Sends panel commands to DCS-BIOS via UDP unicast (port 7778)
#
# The TX side uses one dedicated _DeviceTxWorker per panel.  This ensures
# each HID device handle has exactly ONE writer thread (the reader thread
# never calls dev.write), satisfying the single-owner invariant.

class NetworkManager:
    """Bridges DCS-BIOS UDP traffic and per-device HID I/O.

    Owns:
      - UDP RX socket (multicast listener on 239.255.50.10:5010)
      - UDP TX socket (unicast sender to DCS-BIOS port 7778)
      - One _DeviceTxWorker per handshaked panel
    """

    class _DeviceTxWorker(threading.Thread):
        """Dedicated TX thread for a single panel.

        Design:
          - Sleeps on a Condition variable until _udp_rx_processor
            enqueues a tuple of pre-sliced HID Output Reports.
          - Wakes, grabs the entire pending queue in one batch, and
            writes each report via dev.write().
          - ZERO CPU when idle — no polling, no spinlock.

        Firmware side:
          Each dev.write() delivers a 64-byte Output Report to
          GPDevice::_onOutput() (src/HIDDescriptors.h:72-78), which
          pushes the data into dcsUdpRingbuf via
          dcsUdpRingbufPushChunked() (src/Core/RingBuffer.cpp).
          The firmware's DCS-BIOS parser then processes the ring buffer
          to update cockpit state (displays, LEDs, etc.).

        NOTE [REVIEW]: self.q (deque) is unbounded.  If a device stalls
        (e.g. USB congestion, slow enumeration), the UDP RX thread keeps
        enqueuing while the worker can't drain.  At ~30 Hz DCS export
        rate with typical frame sizes, a 10-second stall accumulates
        ~300 entries — not catastrophic.  For extreme robustness, a
        maxlen on the deque (dropping oldest frames) could prevent
        unbounded growth if a device is truly stuck.
        """

        def __init__(self, entry: DeviceEntry, uiq: queue.Queue):
            super().__init__(daemon=True)
            self.entry = entry
            self.uiq = uiq
            self.q = deque()              # Pending report tuples (unbounded)
            self.cv = threading.Condition()  # Wake signal from enqueue()
            self._running = True

        def enqueue(self, reports_tuple: tuple) -> None:
            """Called by _udp_rx_processor to queue a frame's worth of reports.
            The Condition.notify() wakes the worker thread from its wait()."""
            with self.cv:
                self.q.append(reports_tuple)
                self.cv.notify()

        def stop(self) -> None:
            """Signal the worker to exit its loop."""
            with self.cv:
                self._running = False
                self.cv.notify()

        def run(self) -> None:
            dev = self.entry.dev
            while self._running and not self.entry.disconnected:
                # ── Wait for work ──────────────────────────────────────
                # Blocks on Condition.wait() — thread sleeps in the OS
                # scheduler with zero CPU.  The 0.2 s timeout is a
                # safety net to check the disconnected flag even if no
                # data arrives (paranoia against missed notify).
                with self.cv:
                    while self._running and not self.q:
                        self.cv.wait(timeout=0.2)
                    if not self._running or self.entry.disconnected:
                        break
                    # Atomically grab all pending work and clear the queue
                    batch = list(self.q)
                    self.q.clear()

                # Soft backlog warning (informational, not actionable)
                if len(batch) > 100:
                    try:
                        self.uiq.put(('log', self.entry.name, f"TX backlog={len(batch)}"))
                    except Exception:
                        pass

                # ── Write batch to device ──────────────────────────────
                # Each `reports` is a tuple of 65-byte buffers (report ID
                # byte + 64 payload bytes).  dev.write() sends them as
                # HID Output Reports — received by GPDevice::_onOutput().
                # NO LOCK: only this thread writes to this device.
                for reports in batch:
                    for rep in reports:
                        try:
                            dev.write(rep)
                        except Exception:
                            self.entry.disconnected = True
                            self.uiq.put(('status', self.entry.name, "DISCONNECTED"))
                            return

    def __init__(self, uiq: queue.Queue, reply_addr_ref: list, get_devices_callback):
        self.uiq = uiq
        self.reply_addr = reply_addr_ref        # Mutable [ip_str] shared with readers
        self.get_devices = get_devices_callback  # Returns List[DeviceEntry] under lock
        self.udp_rx_sock: Optional[socket.socket] = None
        self.udp_tx_sock: Optional[socket.socket] = None
        self._running = threading.Event()
        self._ip_committed = False               # True after first valid DCS IP learned
        self._workers: Dict[int, NetworkManager._DeviceTxWorker] = {}
        self._joined_interfaces: List[str] = []

    def start(self) -> None:
        """Launch the UDP RX processor thread and create the TX socket."""
        self._running.set()
        threading.Thread(target=self._udp_rx_processor, daemon=True).start()
        self.udp_tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    def stop(self) -> None:
        """Shut down all sockets and TX workers."""
        self._running.clear()
        if self.udp_rx_sock:
            try:
                self.udp_rx_sock.close()
            except Exception:
                pass
        if self.udp_tx_sock:
            try:
                self.udp_tx_sock.close()
            except Exception:
                pass
        for w in list(self._workers.values()):
            w.stop()
        self._workers.clear()

    def _ensure_worker(self, entry: DeviceEntry) -> Optional[_DeviceTxWorker]:
        """Lazily create a TX worker for a device, or return the existing one.
        Cleans up workers for devices that have disconnected or lost handshake.
        Keyed by id(entry) since DeviceEntry instances are unique per connection."""
        w = self._workers.get(id(entry))
        if w and (entry.disconnected or not entry.handshaked):
            try:
                w.stop()
            except Exception:
                pass
            self._workers.pop(id(entry), None)
            w = None
        if not w and entry.handshaked and not entry.disconnected:
            w = NetworkManager._DeviceTxWorker(entry, self.uiq)
            self._workers[id(entry)] = w
            w.start()
        return w

    def _join_multicast_all_interfaces(self, sock: socket.socket) -> List[str]:
        """Join the DCS-BIOS multicast group on ALL network interfaces.

        On Windows, multicast group membership (IP_ADD_MEMBERSHIP) is
        per-interface.  If DCS sends on Ethernet but the OS routes via
        WiFi, a default INADDR_ANY join only subscribes WiFi and the
        export stream is missed.

        This iterates all interfaces from get_all_interface_ips() and
        joins on each.  If no interfaces are found (e.g. all adapters
        are loopback/APIPA), falls back to INADDR_ANY.

        Returns list of successfully joined interface IPs (for logging).
        """
        joined = []
        interfaces = get_all_interface_ips()

        if not interfaces:
            # Fallback: join on OS default interface (INADDR_ANY)
            try:
                mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
                sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
                joined.append("default")
                self.uiq.put(('log', "UDP", "Joined multicast on default interface"))
            except Exception as e:
                self.uiq.put(('log', "UDP", f"Failed to join multicast: {e}"))
        else:
            for iface_name, ip in interfaces:
                try:
                    # IP_ADD_MEMBERSHIP requires: multicast_addr + interface_addr
                    mreq = socket.inet_aton(DEFAULT_MULTICAST_IP) + socket.inet_aton(ip)
                    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
                    joined.append(ip)
                    self.uiq.put(('log', "UDP", f"Joined multicast on {ip} ({iface_name})"))
                except Exception:
                    pass  # Interface may not support multicast — skip silently

        return joined

    def _udp_rx_processor(self) -> None:
        """UDP receiver thread — the DCS-BIOS export data ingestion point.

        Lifecycle:
          1. Create UDP socket, bind to port 5010, join multicast on all NICs
          2. Loop: blocking recvfrom() → learn DCS IP → update stats →
             slice into 64-byte HID reports → fan out to device workers

        Slicing:
          DCS-BIOS sends variable-length UDP frames (typically 200–2000 bytes).
          We slice each into ceil(len/64) chunks of exactly 64 bytes (zero-
          padded if the last chunk is short).  Each chunk is prepended with
          a 0x00 report-ID byte (hidapi requirement for Output Reports when
          the descriptor has no Report IDs), making 65-byte buffers.

          On the firmware side, GPDevice::_onOutput() (src/HIDDescriptors.h)
          receives each 64-byte chunk and pushes it into dcsUdpRingbuf via
          dcsUdpRingbufPushChunked() (src/Core/RingBuffer.cpp).

        IP Learning:
          The first valid unicast source IP is auto-learned and persisted
          to settings.ini.  This is the address used for sending commands
          back to DCS-BIOS (port 7778).
          NOTE [REVIEW]: _ip_committed is set True after the first detection
          and never reset.  If the DCS machine's IP changes mid-session
          (DHCP renewal, network switch), the manager won't re-learn it.
          A restart is required.  This is intentional to avoid jitter from
          multicast reflections but worth documenting for users.
        """
        try:
            self.udp_rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            # Request a 4 MB receive buffer to absorb bursts during USB stalls
            try:
                self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
            except Exception:
                pass  # OS may cap at a lower value — that's fine
            self.udp_rx_sock.bind(('', DEFAULT_UDP_PORT))

            # Join multicast on ALL interfaces (fixes multi-NIC Windows issue)
            self._joined_interfaces = self._join_multicast_all_interfaces(self.udp_rx_sock)

            if not self._joined_interfaces:
                self.uiq.put(('log', "UDP", "WARNING: Could not join multicast on any interface!"))

            while self._running.is_set():
                # Blocking recvfrom — thread sleeps in kernel, zero CPU
                data, addr = self.udp_rx_sock.recvfrom(16384)

                # ── Auto-learn DCS source IP (once per session) ────────
                if (not self._ip_committed and addr and is_valid_ipv4(addr[0])
                        and self.reply_addr[0] != addr[0]):
                    self.reply_addr[0] = addr[0]
                    write_settings_dcs_ip(addr[0])
                    self._ip_committed = True
                    self.uiq.put(('data_source', None, addr[0]))

                # ── Update rolling stats ───────────────────────────────
                with global_stats_lock:
                    stats["frame_count_total"] += 1
                    stats["frame_count_window"] += 1
                    stats["bytes_rolling"] += len(data)
                    stats["frames_rolling"] += 1
                    stats["bytes"] += len(data)

                # ── Slice UDP frame into 64-byte HID Output Reports ────
                # Pre-slicing is done ONCE here, and the resulting tuple
                # is shared (immutable) across all device workers — no
                # per-device copy needed.
                reports = []
                offset = 0
                while offset < len(data):
                    chunk = data[offset:offset + DEFAULT_REPORT_SIZE]
                    # Prepend report ID 0x00 (hidapi requirement)
                    rep = bytes([0]) + chunk
                    # Zero-pad to exactly 65 bytes (1 ID + 64 payload)
                    rep += b'\x00' * ((DEFAULT_REPORT_SIZE + 1) - len(rep))
                    reports.append(rep)
                    offset += DEFAULT_REPORT_SIZE
                reports = tuple(reports)  # Immutable → safe to share

                # ── Fan out to all connected device workers ────────────
                for entry in self.get_devices():
                    if entry.handshaked and not entry.disconnected:
                        w = self._ensure_worker(entry)
                        if w:
                            w.enqueue(reports)

        except Exception as e:
            if self._running.is_set():
                self.uiq.put(('log', "UDP", f"UDP RX processor error: {e}"))

    def udp_send_report(self, msg: str, port: int = 7778) -> None:
        """Send an ASCII DCS-BIOS command to DCS via UDP.

        Called by device_reader threads when a Feature Report mailbox
        message is drained.  Commands are newline-terminated strings like
        "UFC_1 1\n" or "IFEI_BRIGHTNESS_UP +3200\n".

        Destination: reply_addr[0]:7778 (DCS-BIOS command listener).
        Firmware equivalent: sendDCSBIOSCommand() in src/DCSBIOSBridge.h.
        """
        if self.udp_tx_sock and self.reply_addr[0]:
            try:
                self.udp_tx_sock.sendto(msg.encode(), (self.reply_addr[0], port))
            except Exception as e:
                self.uiq.put(('log', "UDP", f"[UDP SEND ERROR] {e}"))

# ══════════════════════════════════════════════════════════════════════════════
# CONSOLE UI
# ══════════════════════════════════════════════════════════════════════════════
# ncurses-based terminal interface showing:
#   - Header row: cumulative frame count, Hz, kB/s, avg frame size, DCS IP
#   - Device table: name, status (colour-coded), reconnection count
#   - Scrolling log: timestamped messages from all threads
#
# The UI runs on the main thread.  All other threads communicate with it
# via a thread-safe queue (self.uiq) using tagged tuples:
#   ('data_source', None, ip_str)        — DCS IP learned
#   ('globalstats', stats_dict)          — 1-second stats snapshot
#   ('status', device_name, status_str)  — device status change
#   ('log', device_name, message_str)    — log line
#   ('handshake', device_name, msg)      — handshake progress (shown in log)
#   ('statusbar', None, text)            — (consumed but not rendered — informational)

def _ts() -> str:
    """Timestamp for log lines (HH:MM:SS)."""
    return datetime.now().strftime("%H:%M:%S")


class ConsoleUI:
    """Curses-based terminal UI.  Runs on the main thread via curses.wrapper().

    Refresh rate: 100 ms (set by stdscr.timeout(100) — curses getch blocks
    for up to 100 ms then returns ERR, which drives the paint cycle).
    This means the UI consumes ~0 CPU when idle and ~0.1% during active
    DCS sessions (just queue draining and string formatting).
    """

    def __init__(self, get_devices_cb):
        self.get_devices = get_devices_cb  # Returns List[DeviceEntry] under lock
        self.uiq: queue.Queue = queue.Queue()  # Thread-safe event queue
        self._running = threading.Event()
        self._log: deque = deque(maxlen=LOG_KEEP)  # O(1) append, auto-evicts oldest
        self._stats = {
            "frames": "0",
            "hz": "0.0",
            "bw": "0.0",
            "avgudp": "0.0",
            "src": reply_addr[0] or "(waiting...)"
        }
        self._rows: List[tuple] = []  # Cached device rows for painting

    def post(self, evt) -> None:
        """Enqueue a UI event from any thread."""
        self.uiq.put(evt)

    def _consume(self) -> None:
        """Drain all pending events from the queue and update internal state.
        Called once per paint cycle (every ~100 ms).  Non-blocking — uses
        get_nowait() in a loop until the queue is empty."""
        while True:
            try:
                typ, *rest = self.uiq.get_nowait()
            except queue.Empty:
                break
            if typ == 'data_source':
                self._stats['src'] = rest[1]
            elif typ == 'globalstats':
                d = rest[0]
                self._stats['frames'] = str(d.get('frames', "0"))
                self._stats['hz'] = d.get('hz', "0.0")
                self._stats['bw'] = d.get('bw', "0.0")
                self._stats['avgudp'] = d.get('avgudp', "0.0")
            elif typ in ('log', 'handshake'):
                dev, msg = rest
                line = f"[{_ts()}] [{dev}] {msg}"
                self._log.append(line)  # deque maxlen auto-evicts oldest

        # Rebuild device table from current device list
        rows = []
        for e in self.get_devices():
            rows.append((e.name, getattr(e, 'status', '?'), getattr(e, 'reconnections', 0)))
        rows.sort(key=lambda r: r[0])
        self._rows = rows

    def _paint(self, stdscr) -> None:
        """Render the full UI: header, device table, log, and status bar.
        Uses addnstr() everywhere to avoid curses errors on narrow terminals."""
        stdscr.erase()
        h, w = stdscr.getmaxyx()

        # ── Header: stats summary ──────────────────────────────────────
        hdr = (f"Frames: {self._stats['frames']}   Hz: {self._stats['hz']}   "
               f"kB/s: {self._stats['bw']}   Avg UDP Frame size (Bytes): {self._stats['avgudp']}   "
               f"Data Source: {self._stats['src']}")
        stdscr.addnstr(0, 0, hdr, w - 1, curses.A_BOLD)

        # ── Device table header ────────────────────────────────────────
        stdscr.addnstr(3, 0, f"{'Device':<38} {'Status':<16} {'Reconnections':<14}", w - 1)

        # ── Device rows (colour-coded by status) ──────────────────────
        y = 4
        for name, status, reconn in self._rows:
            attr = curses.A_NORMAL
            sl = status.lower()
            if 'ready' in sl:
                attr = curses.color_pair(2)      # Green
            elif ('wait' in sl) or ('handshake' in sl):
                attr = curses.color_pair(3)      # Yellow
            elif ('off' in sl) or ('disconn' in sl):
                attr = curses.color_pair(1)      # Red
            stdscr.addnstr(y, 0, f"{name:<38} {status:<16} {reconn:<14}", w - 1, attr)
            y += 1

        # ── Scrolling log (fills remaining space) ─────────────────────
        y += 2
        avail = max(0, h - y - 1)
        if avail > 0 and self._log:
            # Show only the most recent lines that fit on screen
            tail = list(self._log)[-avail:]
            for i, line in enumerate(tail):
                stdscr.addnstr(y + i, 0, line, w - 1)

        # ── Status bar (bottom row) ───────────────────────────────────
        dev_cnt = len(self._rows)
        stdscr.addnstr(h - 1, 0, f"{dev_cnt} device(s) connected.  Press 'q' to quit.", w - 1, curses.A_DIM)
        stdscr.noutrefresh()
        curses.doupdate()

    def _loop(self, stdscr) -> None:
        """Main curses loop.  Runs until 'q' or Esc is pressed."""
        curses.curs_set(0)           # Hide cursor
        curses.use_default_colors()  # Transparent background
        curses.init_pair(1, curses.COLOR_RED, -1)     # Disconnected
        curses.init_pair(2, curses.COLOR_GREEN, -1)   # Ready
        curses.init_pair(3, curses.COLOR_YELLOW, -1)  # Waiting
        stdscr.timeout(100)  # getch() blocks for max 100 ms → 10 Hz refresh
        self._running.set()

        while self._running.is_set():
            self._consume()
            self._paint(stdscr)
            ch = stdscr.getch()
            if ch in (ord('q'), 27):  # 'q' or Esc
                self._running.clear()
            # No time.sleep() needed — stdscr.timeout(100) handles pacing

    def run(self) -> None:
        """Enter the curses event loop (blocks until quit)."""
        curses.wrapper(self._loop)

    def stop(self) -> None:
        """Signal the UI loop to exit."""
        self._running.clear()

# ══════════════════════════════════════════════════════════════════════════════
# MAIN APPLICATION
# ══════════════════════════════════════════════════════════════════════════════
# Wires everything together:
#   1. Creates the shared device list (protected by device_lock)
#   2. Starts the UI, network manager, device monitor, and stats updater
#   3. Runs the curses event loop on the main thread
#   4. Cleans up on exit (net.stop() closes sockets and workers)

def start_console_mode() -> None:
    """Application entry point.  Creates all subsystems, launches daemon
    threads, and runs the curses UI on the main thread until the user quits.
    """

    # ── Shared device list ─────────────────────────────────────────────
    # Protected by device_lock.  Only _device_monitor mutates the list;
    # all other threads get a snapshot via get_devices().
    devices: List[DeviceEntry] = []
    device_lock = threading.Lock()

    def get_devices() -> List[DeviceEntry]:
        """Return a snapshot of the current device list (thread-safe)."""
        with device_lock:
            return list(devices)

    ui = ConsoleUI(get_devices_cb=get_devices)
    net = NetworkManager(ui.uiq, reply_addr, get_devices)

    # ── Device Hotplug Monitor ─────────────────────────────────────────
    # Runs every HOTPLUG_INTERVAL_S seconds (3 s).  Calls hid.enumerate()
    # to discover new devices and detect disconnections.
    #
    # For each new device:
    #   1. Opens the HID handle via hid.open_path()
    #   2. Creates a DeviceEntry
    #   3. Spawns a device_reader thread (handles handshake + main loop)
    #   4. The reader's handshake success triggers _ensure_worker() to
    #      lazily create the TX worker on the first UDP fan-out
    #
    # For disconnections:
    #   1. Marks entry.disconnected = True (signals reader + TX worker)
    #   2. Closes the HID handle
    #   3. Removes the entry from the device list
    #   NOTE [REVIEW]: Identity is based on serial_number.  If two panels
    #   share a blank/default serial, the second would be skipped.  All
    #   production CockpitOS panels have unique serials via LABEL_SET_STR.
    #   Using DeviceEntry.get_key() (which falls back to OS path) would
    #   be more robust for development/unconfigured boards.

    def _device_monitor() -> None:
        """Hotplug monitor thread (daemon).  Scans for new/removed devices."""
        global prev_reconnections
        while True:
            dev_infos = list_target_devices()
            current_serials = {d.get('serial_number', '') for d in dev_infos}

            # ── Detect disconnections ──────────────────────────────────
            with device_lock:
                stale = [e for e in devices if e.info.get('serial_number', '') not in current_serials]
            for e in stale:
                e.disconnected = True
                try:
                    e.dev.close()
                except Exception:
                    pass
                ui.post(('status', e.name, "DISCONNECTED"))
                with device_lock:
                    devices[:] = [x for x in devices if x is not e]

            # ── Detect new connections ─────────────────────────────────
            for d in dev_infos:
                serial = d.get('serial_number', '')
                with device_lock:
                    exists = any(x.info.get('serial_number', '') == serial for x in devices)
                if not exists:
                    dev = hid.device()
                    try:
                        dev.open_path(d['path'])
                    except Exception:
                        continue  # Device may have been unplugged between enumerate and open
                    entry = DeviceEntry(dev, d)
                    # Restore reconnection count from previous hotplug cycles
                    entry.reconnections = prev_reconnections.get(serial, 0)
                    prev_reconnections[serial] = entry.reconnections + 1
                    with device_lock:
                        devices.append(entry)
                    # Spawn a dedicated reader thread for this device
                    # (handles handshake → main loop → Feature drain)
                    threading.Thread(
                        target=device_reader,
                        args=(entry, ui.uiq, net.udp_send_report),
                        daemon=True
                    ).start()
                    ui.post(('status', entry.name, "WAIT HANDSHAKE"))

            with device_lock:
                ui.post(('statusbar', None, f"{len(devices)} device(s) connected."))

            # Sleep in 100 ms chunks so the thread can exit promptly
            # when the process terminates (daemon thread cleanup)
            for _ in range(HOTPLUG_INTERVAL_S * 10):
                time.sleep(0.1)

    # ── Stats Updater ──────────────────────────────────────────────────
    # Computes per-second metrics from the rolling counters and pushes
    # a 'globalstats' event to the UI queue.  Resets the window counters
    # after each tick.
    #
    # NOTE [REVIEW]: The variable names in the `stats` dict are slightly
    # misleading — "bytes" and "frame_count_window" are per-second values
    # that get reset each tick, while "frame_count_total" is the only
    # truly cumulative counter.  The math is correct (duration ≈ 1.0 s
    # since start_time is reset each tick), but renaming for clarity
    # (e.g. "bytes_window", "frames_window") would improve readability.

    def _stats_updater() -> None:
        """Stats update thread (daemon).  Posts 1-second snapshots to the UI."""
        while True:
            time.sleep(1)
            with global_stats_lock:
                # Average UDP frame size over this window
                avg_frame = (stats["bytes_rolling"] / stats["frames_rolling"]) if stats["frames_rolling"] else 0
                # Time elapsed since last reset (~1.0 s)
                duration = time.time() - stats["start_time"]
                hz = stats["frame_count_window"] / duration if duration > 0 else 0
                kbps = (stats["bytes"] / 1024) / duration if duration > 0 else 0
                ui.post(('globalstats', {
                    'frames': stats["frame_count_total"],  # Cumulative (never reset)
                    'hz': f"{hz:.1f}",
                    'bw': f"{kbps:.1f}",
                    'avgudp': f"{avg_frame:.1f}",
                }))
                # Reset per-second window counters
                stats["frame_count_window"] = 0
                stats["bytes"] = 0
                stats["bytes_rolling"] = 0
                stats["frames_rolling"] = 0
                stats["start_time"] = time.time()

    # ── Launch all threads and enter UI loop ───────────────────────────
    threading.Thread(target=_device_monitor, daemon=True).start()
    threading.Thread(target=_stats_updater, daemon=True).start()
    net.start()

    try:
        ui.run()  # Blocks on main thread until 'q' or Esc
    finally:
        net.stop()  # Close sockets and stop all TX workers


def main() -> None:
    """Top-level entry point.  Acquires single-instance lock, prints
    startup banner, runs the console UI, and ensures cleanup on exit."""
    acquire_instance_lock()

    try:
        print("CockpitOS HID Bridge v5.1 - Multi-Interface Multicast Fix")
        print(f"VID: 0x{USB_VID:04X}, PID: {'Any' if USB_PID is None else f'0x{USB_PID:04X}'}")
        print(f"Max Devices: {MAX_DEVICES}")
        print("Starting...")
        print()

        start_console_mode()

        print("Goodbye!")

    finally:
        release_instance_lock()


if __name__ == "__main__":
    main()
