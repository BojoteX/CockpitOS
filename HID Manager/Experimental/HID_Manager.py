#!/usr/bin/env python3
"""
CockpitOS HID Manager v5 - Fixed Architecture
==============================================

Combines the PROVEN patterns from the original with clean code structure.

Key Design (borrowed from working original):
--------------------------------------------
1. RX: Per-device threads with BLOCKING HID reads - NO LOCKS during read
2. TX: Per-device workers with Condition variable wake - NO LOCKS during write
3. UDP: Direct blocking recvfrom() - no select() overhead
4. Single writer per HID handle - no lock contention

What was wrong with v4 (experimental):
--------------------------------------
- Held lock during blocking read() → TX completely blocked
- Held lock during feature drain → more blocking
- Sharded TX pool added lookup overhead
- select() wrapper added syscall overhead

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

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

IS_WINDOWS = (os.name == 'nt') or sys.platform.startswith('win')
IS_LINUX = sys.platform.startswith('linux')
IS_MACOS = sys.platform == 'darwin'

# ══════════════════════════════════════════════════════════════════════════════
# DEPENDENCY CHECK
# ══════════════════════════════════════════════════════════════════════════════

REQUIRED_MODULES = {
    "hid": "hidapi",
    "filelock": "filelock",
    "curses": "windows-curses" if IS_WINDOWS else None,
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

import hid
import curses
from filelock import FileLock, Timeout

# ══════════════════════════════════════════════════════════════════════════════
# SINGLE INSTANCE LOCK
# ══════════════════════════════════════════════════════════════════════════════

LOCKFILE = os.path.join(SCRIPT_DIR, "hid_manager.lock")
_instance_lock: Optional[FileLock] = None


def acquire_instance_lock() -> FileLock:
    """Acquire single-instance lock."""
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
    """Release the single-instance lock if held."""
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

# Protocol constants
DEFAULT_REPORT_SIZE    = 64
DEFAULT_MULTICAST_IP   = "239.255.50.10"
DEFAULT_UDP_PORT       = 5010
DEFAULT_DCS_TX_PORT    = 7778
HANDSHAKE_REQ          = b"DCSBIOS-HANDSHAKE"
FEATURE_REPORT_ID      = 0

# Tunable constants
MAX_DEVICES            = 32
MAX_HANDSHAKE_ATTEMPTS = 50   # ~10 seconds at 0.2s intervals
MAX_FEATURE_DRAIN      = 64   # Max messages to drain per trigger
IDLE_TIMEOUT           = 2.0
LOG_KEEP               = 2000
HOTPLUG_INTERVAL_S     = 3    # Scan every 3 seconds (reduce CPU spikes)

# Settings file
SETTINGS_PATH = os.path.join(SCRIPT_DIR, "settings.ini")

# ══════════════════════════════════════════════════════════════════════════════
# SETTINGS
# ══════════════════════════════════════════════════════════════════════════════

def read_settings() -> Tuple[int, Optional[int], str]:
    """Read settings from INI file."""
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
        pid = None

    try:
        dcs_ip = config['DCS'].get('UDP_SOURCE_IP', '127.0.0.1')
    except Exception:
        dcs_ip = '127.0.0.1'

    return vid, pid, dcs_ip


def write_settings_dcs_ip(new_ip: str) -> None:
    """Update DCS IP in settings."""
    config = configparser.ConfigParser()
    config.read(SETTINGS_PATH)
    if 'DCS' not in config:
        config['DCS'] = {}
    config['DCS']['UDP_SOURCE_IP'] = new_ip
    with open(SETTINGS_PATH, 'w') as f:
        config.write(f)


def is_valid_ipv4(ip: str) -> bool:
    """Validate IPv4 address."""
    try:
        ip_obj = ipaddress.ip_address(ip)
        return (
            isinstance(ip_obj, ipaddress.IPv4Address)
            and not ip_obj.is_multicast
            and not ip_obj.is_unspecified
        )
    except Exception:
        return False


USB_VID, USB_PID, STORED_DCS_IP = read_settings()

# Global stats (like original)
stats = {
    "frame_count_total": 0,
    "frame_count_window": 0,
    "bytes": 0,
    "start_time": time.time(),
    "bytes_rolling": 0,
    "frames_rolling": 0,
}
global_stats_lock = threading.Lock()

# Reply address (mutable list for thread sharing, like original)
reply_addr = [STORED_DCS_IP]

# Reconnection tracking
prev_reconnections: Dict[str, int] = {}

# ══════════════════════════════════════════════════════════════════════════════
# DEVICE ENTRY (from original - simple and correct)
# ══════════════════════════════════════════════════════════════════════════════

def is_bt_serial(s: str) -> bool:
    """Check if serial looks like a Bluetooth MAC address."""
    import re
    return bool(re.fullmatch(
        r'(?i)(?:[0-9a-f]{2}[:-]){5}[0-9a-f]{2}|[0-9a-f]{12}|bx-[0-9a-f]{16}', s))


class DeviceEntry:
    """Simple device state container (from original)."""
    
    def __init__(self, dev, dev_info: dict):
        self.dev = dev
        self.info = dev_info

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
        return self.info.get('serial_number', '') or str(self.info.get('path', b''))

# ══════════════════════════════════════════════════════════════════════════════
# HID FUNCTIONS (from original - proven to work)
# ══════════════════════════════════════════════════════════════════════════════

def list_target_devices() -> List[dict]:
    """List HID devices matching our VID/PID."""
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
    """Perform FIFO handshake with device (from original, with timeout fix)."""
    payload = HANDSHAKE_REQ.ljust(DEFAULT_REPORT_SIZE, b'\x00')
    attempts = 0
    
    while attempts < MAX_HANDSHAKE_ATTEMPTS:
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

        try:
            dev.send_feature_report(bytes([FEATURE_REPORT_ID]) + payload)
        except Exception as e:
            if uiq:
                uiq.put(('handshake', device_name, f"SET FEATURE exception: {e}"))
            time.sleep(0.2)
            continue

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
    
    return False  # Timeout


def _close_stale_handle(entry: DeviceEntry, uiq: queue.Queue, where: str, exc) -> None:
    """Mark device as disconnected and log error."""
    try:
        entry.dev.close()
    except Exception:
        pass
    entry.disconnected = True
    uiq.put(('status', entry.name, f"STALE HANDLE ({where})"))
    uiq.put(('log', entry.name, f"[stale] {where} exception: {exc}"))

# ══════════════════════════════════════════════════════════════════════════════
# DEVICE READER (from original - NO LOCKS, proven to work)
# ══════════════════════════════════════════════════════════════════════════════

def device_reader(entry: DeviceEntry, uiq: queue.Queue, udp_send) -> None:
    """
    Per-device reader thread (from original).
    
    CRITICAL: NO LOCKS around HID operations!
    - Only this thread reads from this device
    - TX worker only writes to this device
    - No contention = no locks needed
    """
    dev = entry.dev
    
    try:
        # Handshake
        while not entry.handshaked and not entry.disconnected:
            entry.handshaked = try_fifo_handshake(dev, uiq=uiq, device_name=entry.name)
            if not entry.handshaked:
                _close_stale_handle(entry, uiq, "HANDSHAKE", "no reply")
                return
            uiq.put(('status', entry.name, "READY"))
            uiq.put(('log', entry.name, "Handshake complete, ready to process input."))
            entry.status = "READY"

        # Wait for DCS
        if reply_addr[0] is None and not entry.disconnected:
            uiq.put(('log', entry.name, "Waiting for DCS mission start..."))
        while reply_addr[0] is None and not entry.disconnected:
            time.sleep(0.2)
        if entry.disconnected:
            return

        uiq.put(('log', entry.name, f"DCS detected on {reply_addr[0]} — Starting normal operation."))

        # Clear backlog (bounded)
        cleared = False
        attempt = 0
        while not cleared and not entry.disconnected and attempt < 100:
            try:
                resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                if not any(d):
                    cleared = True
            except Exception as e:
                _close_stale_handle(entry, uiq, "FEATURE-DRAIN", e)
                return
            attempt += 1
        if not cleared:
            _close_stale_handle(entry, uiq, "FEATURE-DRAIN", "timeout")
            return

        # Main loop - BLOCKING read, NO LOCK
        while not entry.disconnected and entry.handshaked:
            try:
                # BLOCKING read - kernel wakes us when data arrives
                # NO LOCK here! Only this thread reads from this device
                data = dev.read(DEFAULT_REPORT_SIZE, timeout_ms=-1)
                if not data:
                    continue
            except Exception as e:
                _close_stale_handle(entry, uiq, "READ", e)
                return

            # Drain feature reports - NO LOCK here either
            drain = 0
            while not entry.disconnected and drain < MAX_FEATURE_DRAIN:
                try:
                    resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                    d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                    msg = d.rstrip(b'\x00').decode(errors="replace").strip()
                    if not msg or msg == HANDSHAKE_REQ.decode():
                        break
                    uiq.put(('log', entry.name, f"IN: {msg}"))
                    udp_send(msg + "\n")
                    drain += 1
                except Exception as e:
                    _close_stale_handle(entry, uiq, "FEATURE", e)
                    return

    finally:
        try:
            dev.close()
        except Exception:
            pass

# ══════════════════════════════════════════════════════════════════════════════
# NETWORK MANAGER (from original - per-device TX workers, proven to work)
# ══════════════════════════════════════════════════════════════════════════════

class NetworkManager:
    """
    Network manager with per-device TX workers (from original).
    
    CRITICAL: One TX worker per device!
    - Only that worker writes to that device's HID handle
    - No lock contention with reader thread
    - Uses Condition variable for efficient wake (not polling)
    """

    class _DeviceTxWorker(threading.Thread):
        """Per-device TX worker (from original)."""
        
        def __init__(self, entry: DeviceEntry, uiq: queue.Queue):
            super().__init__(daemon=True)
            self.entry = entry
            self.uiq = uiq
            self.q = deque()  # Unbounded FIFO
            self.cv = threading.Condition()
            self._running = True

        def enqueue(self, reports_tuple: tuple) -> None:
            with self.cv:
                self.q.append(reports_tuple)
                self.cv.notify()

        def stop(self) -> None:
            with self.cv:
                self._running = False
                self.cv.notify()

        def run(self) -> None:
            dev = self.entry.dev
            while self._running and not self.entry.disconnected:
                # Wait for work (blocks efficiently via Condition)
                with self.cv:
                    while self._running and not self.q:
                        self.cv.wait(timeout=0.2)  # Increased from 0.05 for less CPU
                    if not self._running or self.entry.disconnected:
                        break
                    # Grab all pending work
                    batch = list(self.q)
                    self.q.clear()

                # Soft backlog warning
                if len(batch) > 100:
                    try:
                        self.uiq.put(('log', self.entry.name, f"TX backlog={len(batch)}"))
                    except Exception:
                        pass

                # Write batch - NO LOCK, only this thread writes to this device
                for reports in batch:
                    for rep in reports:
                        try:
                            dev.write(rep)
                        except Exception:
                            self.entry.disconnected = True
                            self.uiq.put(('status', self.entry.name, "DISCONNECTED"))
                            return

                # Removed: time.sleep(0.0005) - unnecessary yield

    def __init__(self, uiq: queue.Queue, reply_addr_ref: list, get_devices_callback):
        self.uiq = uiq
        self.reply_addr = reply_addr_ref
        self.get_devices = get_devices_callback
        self.udp_rx_sock: Optional[socket.socket] = None
        self.udp_tx_sock: Optional[socket.socket] = None
        self._running = threading.Event()
        self._ip_committed = False
        self._workers: Dict[int, NetworkManager._DeviceTxWorker] = {}

    def start(self) -> None:
        self._running.set()
        threading.Thread(target=self._udp_rx_processor, daemon=True).start()
        self.udp_tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    def stop(self) -> None:
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
        """Get or create TX worker for device."""
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

    def _udp_rx_processor(self) -> None:
        """UDP receiver - direct blocking recvfrom (from original)."""
        try:
            self.udp_rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            try:
                self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
            except Exception:
                pass
            self.udp_rx_sock.bind(('', DEFAULT_UDP_PORT))
            mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
            self.udp_rx_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

            while self._running.is_set():
                # Direct blocking recvfrom - NO select() overhead
                data, addr = self.udp_rx_sock.recvfrom(16384)

                # Learn DCS address
                if (not self._ip_committed and addr and is_valid_ipv4(addr[0])
                        and self.reply_addr[0] != addr[0]):
                    self.reply_addr[0] = addr[0]
                    write_settings_dcs_ip(addr[0])
                    self._ip_committed = True
                    self.uiq.put(('data_source', None, addr[0]))

                # Update stats
                with global_stats_lock:
                    stats["frame_count_total"] += 1
                    stats["frame_count_window"] += 1
                    stats["bytes_rolling"] += len(data)
                    stats["frames_rolling"] += 1
                    stats["bytes"] += len(data)

                # Pre-slice into HID reports (done once, shared)
                reports = []
                offset = 0
                while offset < len(data):
                    chunk = data[offset:offset + DEFAULT_REPORT_SIZE]
                    rep = bytes([0]) + chunk
                    rep += b'\x00' * ((DEFAULT_REPORT_SIZE + 1) - len(rep))
                    reports.append(rep)
                    offset += DEFAULT_REPORT_SIZE
                reports = tuple(reports)

                # Fan out to device workers
                for entry in self.get_devices():
                    if entry.handshaked and not entry.disconnected:
                        w = self._ensure_worker(entry)
                        if w:
                            w.enqueue(reports)

        except Exception as e:
            if self._running.is_set():
                self.uiq.put(('log', "UDP", f"UDP RX processor error: {e}"))

    def udp_send_report(self, msg: str, port: int = 7778) -> None:
        """Send command to DCS."""
        if self.udp_tx_sock and self.reply_addr[0]:
            try:
                self.udp_tx_sock.sendto(msg.encode(), (self.reply_addr[0], port))
            except Exception as e:
                self.uiq.put(('log', "UDP", f"[UDP SEND ERROR] {e}"))

# ══════════════════════════════════════════════════════════════════════════════
# CONSOLE UI (from original, with deque optimization)
# ══════════════════════════════════════════════════════════════════════════════

def _ts() -> str:
    return datetime.now().strftime("%H:%M:%S")


class ConsoleUI:
    """Curses-based console UI."""

    def __init__(self, get_devices_cb):
        self.get_devices = get_devices_cb
        self.uiq: queue.Queue = queue.Queue()
        self._running = threading.Event()
        self._log: deque = deque(maxlen=LOG_KEEP)  # O(1) append, auto-truncate
        self._stats = {
            "frames": "0",
            "hz": "0.0",
            "bw": "0.0",
            "avgudp": "0.0",
            "src": reply_addr[0] or "(waiting...)"
        }
        self._rows: List[tuple] = []

    def post(self, evt) -> None:
        self.uiq.put(evt)

    def _consume(self) -> None:
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
                self._log.append(line)  # deque handles maxlen automatically

        rows = []
        for e in self.get_devices():
            rows.append((e.name, getattr(e, 'status', '?'), getattr(e, 'reconnections', 0)))
        rows.sort(key=lambda r: r[0])
        self._rows = rows

    def _paint(self, stdscr) -> None:
        stdscr.erase()
        h, w = stdscr.getmaxyx()
        
        hdr = (f"Frames: {self._stats['frames']}   Hz: {self._stats['hz']}   "
               f"kB/s: {self._stats['bw']}   Avg UDP Frame size (Bytes): {self._stats['avgudp']}   "
               f"Data Source: {self._stats['src']}")
        stdscr.addnstr(0, 0, hdr, w - 1, curses.A_BOLD)
        stdscr.addnstr(3, 0, f"{'Device':<38} {'Status':<16} {'Reconnections':<14}", w - 1)
        
        y = 4
        for name, status, reconn in self._rows:
            attr = curses.A_NORMAL
            sl = status.lower()
            if 'ready' in sl:
                attr = curses.color_pair(2)
            elif ('wait' in sl) or ('handshake' in sl):
                attr = curses.color_pair(3)
            elif ('off' in sl) or ('disconn' in sl):
                attr = curses.color_pair(1)
            stdscr.addnstr(y, 0, f"{name:<38} {status:<16} {reconn:<14}", w - 1, attr)
            y += 1
        
        y += 2
        avail = max(0, h - y - 1)
        if avail > 0 and self._log:
            # Efficiently get last N items from deque
            tail = list(self._log)[-avail:]
            for i, line in enumerate(tail):
                stdscr.addnstr(y + i, 0, line, w - 1)
        
        dev_cnt = len(self._rows)
        stdscr.addnstr(h - 1, 0, f"{dev_cnt} device(s) connected.  Press 'q' to quit.", w - 1, curses.A_DIM)
        stdscr.noutrefresh()
        curses.doupdate()

    def _loop(self, stdscr) -> None:
        curses.curs_set(0)
        curses.use_default_colors()
        curses.init_pair(1, curses.COLOR_RED, -1)
        curses.init_pair(2, curses.COLOR_GREEN, -1)
        curses.init_pair(3, curses.COLOR_YELLOW, -1)
        stdscr.timeout(100)  # 100ms refresh
        self._running.set()
        
        while self._running.is_set():
            self._consume()
            self._paint(stdscr)
            ch = stdscr.getch()
            if ch in (ord('q'), 27):
                self._running.clear()
            # NO time.sleep() here - timeout handles it

    def run(self) -> None:
        curses.wrapper(self._loop)

    def stop(self) -> None:
        self._running.clear()

# ══════════════════════════════════════════════════════════════════════════════
# MAIN APPLICATION
# ══════════════════════════════════════════════════════════════════════════════

def start_console_mode() -> None:
    """Main entry point for console mode."""
    
    devices: List[DeviceEntry] = []
    device_lock = threading.Lock()

    def get_devices() -> List[DeviceEntry]:
        with device_lock:
            return list(devices)

    ui = ConsoleUI(get_devices_cb=get_devices)
    net = NetworkManager(ui.uiq, reply_addr, get_devices)

    def _device_monitor() -> None:
        """Hotplug monitor thread."""
        global prev_reconnections
        while True:
            dev_infos = list_target_devices()
            current_serials = {d.get('serial_number', '') for d in dev_infos}
            
            # Detect disconnections
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
            
            # Detect new connections
            for d in dev_infos:
                serial = d.get('serial_number', '')
                with device_lock:
                    exists = any(x.info.get('serial_number', '') == serial for x in devices)
                if not exists:
                    dev = hid.device()
                    try:
                        dev.open_path(d['path'])
                    except Exception:
                        continue
                    entry = DeviceEntry(dev, d)
                    entry.reconnections = prev_reconnections.get(serial, 0)
                    prev_reconnections[serial] = entry.reconnections + 1
                    with device_lock:
                        devices.append(entry)
                    threading.Thread(
                        target=device_reader,
                        args=(entry, ui.uiq, net.udp_send_report),
                        daemon=True
                    ).start()
                    ui.post(('status', entry.name, "WAIT HANDSHAKE"))
            
            with device_lock:
                ui.post(('statusbar', None, f"{len(devices)} device(s) connected."))
            
            # Sleep in chunks for responsive shutdown
            for _ in range(HOTPLUG_INTERVAL_S * 10):
                time.sleep(0.1)

    def _stats_updater() -> None:
        """Stats update thread."""
        while True:
            time.sleep(1)
            with global_stats_lock:
                avg_frame = (stats["bytes_rolling"] / stats["frames_rolling"]) if stats["frames_rolling"] else 0
                duration = time.time() - stats["start_time"]
                hz = stats["frame_count_window"] / duration if duration > 0 else 0
                kbps = (stats["bytes"] / 1024) / duration if duration > 0 else 0
                ui.post(('globalstats', {
                    'frames': stats["frame_count_total"],
                    'hz': f"{hz:.1f}",
                    'bw': f"{kbps:.1f}",
                    'avgudp': f"{avg_frame:.1f}",
                }))
                # Reset window
                stats["frame_count_window"] = 0
                stats["bytes"] = 0
                stats["bytes_rolling"] = 0
                stats["frames_rolling"] = 0
                stats["start_time"] = time.time()

    # Start threads
    threading.Thread(target=_device_monitor, daemon=True).start()
    threading.Thread(target=_stats_updater, daemon=True).start()
    net.start()

    try:
        ui.run()
    finally:
        net.stop()


def main() -> None:
    """Main entry point."""
    acquire_instance_lock()

    try:
        print("CockpitOS HID Bridge v5 - Fixed Architecture")
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
