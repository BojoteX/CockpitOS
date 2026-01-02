#!/usr/bin/env python3
"""
CockpitOS HID Manager - Refactored for Scale (v2)
Handles 200+ devices with fixed thread pool (O(1) threads, not O(N))

Features:
- Fixed thread pool architecture (12 threads regardless of device count)
- Non-blocking HID polling
- Professional curses-based console UI
- Cross-platform (Windows, Linux, macOS)

Author: CockpitOS Project
License: MIT
"""

# ══════════════════════════════════════════════════════════════════════════════
# IMPORTS AND BOOTSTRAP
# ══════════════════════════════════════════════════════════════════════════════

import sys
import os
import re
import time
import socket
import struct
import threading
import queue
import configparser
import ipaddress
from collections import deque
from datetime import datetime
from typing import Optional, List, Dict, Callable, Tuple, Any

# Change to script directory
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(SCRIPT_DIR)

# Platform detection
IS_WINDOWS = (os.name == 'nt') or sys.platform.startswith('win')

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
try:
    lock = FileLock(LOCKFILE)
    lock.acquire(timeout=0.1)
except Timeout:
    print("ERROR: Another instance of CockpitOS HID Manager is already running.")
    input("Press Enter to exit...")
    sys.exit(1)

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

# Architecture tuning (fixed thread counts)
TX_WORKERS             = 4       # HID write workers
RX_POLLERS             = 4       # HID read pollers
RX_POLL_INTERVAL_MS    = 1       # Polling interval (ms)
MAX_DEVICES            = 256     # Registry capacity
UDP_RX_BUFFER_SIZE     = 4 * 1024 * 1024  # 4MB kernel buffer
TX_QUEUE_SIZE          = 1024    # Dispatch queue depth
LOG_HISTORY_SIZE       = 2000    # Max log lines kept

# UI refresh
UI_REFRESH_MS          = 50      # Console refresh interval

# ══════════════════════════════════════════════════════════════════════════════
# SETTINGS FILE HANDLING
# ══════════════════════════════════════════════════════════════════════════════

SETTINGS_PATH = os.path.join(SCRIPT_DIR, "settings.ini")

def read_settings() -> Tuple[int, Optional[int], str]:
    """Read VID, PID, and DCS IP from settings.ini"""
    config = configparser.ConfigParser()
    
    if not os.path.isfile(SETTINGS_PATH):
        config['USB'] = {'VID': '0xCAFE'}
        config['DCS'] = {'UDP_SOURCE_IP': '127.0.0.1'}
        config['MAIN'] = {'CONSOLE': '1'}
        with open(SETTINGS_PATH, 'w') as f:
            config.write(f)
    
    config.read(SETTINGS_PATH)
    
    try:
        vid = int(config.get('USB', 'VID', fallback='0xCAFE'), 0)
    except ValueError:
        vid = 0xCAFE
    
    try:
        pid_str = config.get('USB', 'PID', fallback='')
        pid = int(pid_str, 0) if pid_str else None
    except ValueError:
        pid = None
    
    dcs_ip = config.get('DCS', 'UDP_SOURCE_IP', fallback='127.0.0.1')
    
    return vid, pid, dcs_ip

def write_dcs_ip(ip: str):
    """Update DCS IP in settings.ini"""
    config = configparser.ConfigParser()
    config.read(SETTINGS_PATH)
    if 'DCS' not in config:
        config['DCS'] = {}
    config['DCS']['UDP_SOURCE_IP'] = ip
    with open(SETTINGS_PATH, 'w') as f:
        config.write(f)

def is_valid_ipv4(ip: str) -> bool:
    """Validate IPv4 address (non-multicast, non-unspecified)"""
    try:
        ip_obj = ipaddress.ip_address(ip)
        return (
            isinstance(ip_obj, ipaddress.IPv4Address)
            and not ip_obj.is_multicast
            and not ip_obj.is_unspecified
        )
    except ValueError:
        return False

# Load settings
USB_VID, USB_PID, STORED_DCS_IP = read_settings()

# ══════════════════════════════════════════════════════════════════════════════
# DEVICE STATE
# ══════════════════════════════════════════════════════════════════════════════

class DeviceState:
    """
    Thread-safe device state container.
    Uses __slots__ for memory efficiency at scale.
    """
    
    __slots__ = (
        'dev', 'info', 'name', 'serial', 'path',
        'state', 'status_text', 'lock',
        'last_activity', 'packets_tx', 'packets_rx',
        'reconnections', 'poller_id', 'tx_worker_id'
    )
    
    # State machine constants
    DISCONNECTED = 0
    HANDSHAKING  = 1
    READY        = 2
    ERROR        = 3
    
    # Status text mapping
    STATE_TEXT = {
        DISCONNECTED: "DISCONNECTED",
        HANDSHAKING:  "HANDSHAKING",
        READY:        "READY",
        ERROR:        "ERROR",
    }
    
    def __init__(self, dev, dev_info: dict):
        self.dev = dev
        self.info = dev_info
        self.serial = dev_info.get('serial_number', '') or ''
        self.path = dev_info.get('path', b'')
        
        # Derive display name (prefer serial, fallback to product)
        product = dev_info.get('product_string', '') or ''
        self.name = self.serial if self.serial else (product or f"Device-{id(dev) & 0xFFFF:04X}")
        
        # State
        self.state = self.HANDSHAKING
        self.status_text = "WAIT HANDSHAKE"
        self.lock = threading.Lock()
        self.last_activity = time.monotonic()
        
        # Stats
        self.packets_tx = 0
        self.packets_rx = 0
        self.reconnections = 0
        
        # Worker assignments (set by HandshakeManager after handshake)
        self.poller_id: int = -1      # RX poller assignment
        self.tx_worker_id: int = -1   # TX worker assignment (CRITICAL for single-writer)
    
    def is_ready(self) -> bool:
        return self.state == self.READY
    
    def set_state(self, new_state: int, status_text: Optional[str] = None):
        self.state = new_state
        self.status_text = status_text or self.STATE_TEXT.get(new_state, "UNKNOWN")
    
    def mark_activity(self):
        self.last_activity = time.monotonic()

# ══════════════════════════════════════════════════════════════════════════════
# DEVICE REGISTRY
# ══════════════════════════════════════════════════════════════════════════════

class DeviceRegistry:
    """
    Central registry of all connected devices.
    Optimized for frequent reads (snapshot pattern), infrequent writes.
    """
    
    def __init__(self):
        self._devices: Dict[str, DeviceState] = {}
        self._lock = threading.RLock()
        self._snapshot: List[DeviceState] = []
        self._version = 0
    
    def add(self, device: DeviceState) -> bool:
        with self._lock:
            key = device.serial or str(device.path)
            if key in self._devices:
                return False
            self._devices[key] = device
            self._rebuild_snapshot()
            return True
    
    def remove(self, device: DeviceState) -> bool:
        with self._lock:
            key = device.serial or str(device.path)
            if key not in self._devices:
                return False
            del self._devices[key]
            self._rebuild_snapshot()
            return True
    
    def remove_by_key(self, key: str) -> Optional[DeviceState]:
        with self._lock:
            device = self._devices.pop(key, None)
            if device:
                self._rebuild_snapshot()
            return device
    
    def get(self, key: str) -> Optional[DeviceState]:
        with self._lock:
            return self._devices.get(key)
    
    def get_all(self) -> List[DeviceState]:
        """Returns snapshot copy - safe to iterate without lock, prevents mutation."""
        return list(self._snapshot)
    
    def get_ready(self) -> List[DeviceState]:
        """Returns only READY devices."""
        return [d for d in self._snapshot if d.is_ready()]
    
    def get_by_poller(self, poller_id: int) -> List[DeviceState]:
        """Returns devices assigned to a specific RX poller."""
        return [d for d in self._snapshot if d.poller_id == poller_id]
    
    def get_by_tx_worker(self, tx_worker_id: int) -> List[DeviceState]:
        """Returns READY devices assigned to a specific TX worker."""
        return [d for d in self._snapshot if d.is_ready() and d.tx_worker_id == tx_worker_id]
    
    def count(self) -> int:
        return len(self._snapshot)
    
    def count_ready(self) -> int:
        return sum(1 for d in self._snapshot if d.is_ready())
    
    def _rebuild_snapshot(self):
        # Sort by name for consistent display order
        self._snapshot = sorted(self._devices.values(), key=lambda d: d.name)
        self._version += 1

# ══════════════════════════════════════════════════════════════════════════════
# TX DISPATCHER (UDP → HID fan-out with device sharding)
# ══════════════════════════════════════════════════════════════════════════════

class TxDispatcher:
    """
    Receives UDP frames, slices into HID reports, dispatches to worker pool.
    
    CRITICAL DESIGN: Each device is assigned to exactly ONE TX worker.
    This ensures single-writer-per-device, avoiding HID handle race conditions.
    
    Architecture:
    - Each worker has its own queue
    - UDP frames are broadcast to ALL worker queues (same data)
    - Each worker only writes to devices assigned to it (sharding)
    - No two threads ever write to the same HID handle
    """
    
    def __init__(self, registry: DeviceRegistry, ui_queue: queue.Queue,
                 num_workers: int = TX_WORKERS):
        self.registry = registry
        self.uiq = ui_queue
        self.num_workers = num_workers
        
        # One queue per worker for job broadcast
        self.worker_queues: List[queue.Queue] = [
            queue.Queue(maxsize=TX_QUEUE_SIZE) for _ in range(num_workers)
        ]
        self.workers: List[threading.Thread] = []
        self._running = threading.Event()
        self._sequence = 0
        
        # Round-robin counter for TX worker assignment
        self._next_tx_worker = 0
        self._tx_worker_lock = threading.Lock()
        
        # Lock for atomic all-or-nothing broadcast (prevents TOCTOU race)
        self._broadcast_lock = threading.Lock()
        
        # Stats (atomic via GIL)
        self.jobs_dispatched = 0
        self.jobs_dropped = 0
        self.jobs_partial = 0  # Rare: partial enqueue due to queue race
        self.worker_busy = [False] * num_workers
        
        # Start workers
        self._running.set()
        for i in range(num_workers):
            t = threading.Thread(target=self._worker_loop, args=(i,), daemon=True)
            t.name = f"TxWorker-{i}"
            self.workers.append(t)
            t.start()
    
    def get_next_tx_worker(self) -> int:
        """Assign next TX worker ID (round-robin). Called by HandshakeManager."""
        with self._tx_worker_lock:
            worker_id = self._next_tx_worker
            self._next_tx_worker = (self._next_tx_worker + 1) % self.num_workers
            return worker_id
    
    def dispatch(self, udp_data: bytes):
        """
        Called by UDP RX thread - slices data and broadcasts to all workers.
        
        BACKPRESSURE POLICY: Best-effort all-or-nothing per frame.
        We attempt to enqueue to all workers. If any queue is full, we handle
        it gracefully without crashing. Partial enqueues are rare and acceptable
        since DCS-BIOS frames overwrite state (next frame fixes any inconsistency).
        
        Thread-safety: Exception-safe broadcast that never crashes UDP RX thread.
        """
        # Bound input size to prevent memory spikes (max ~64 reports per frame)
        MAX_UDP_FRAME_SIZE = 4096  # 64 reports × 64 bytes
        if len(udp_data) > MAX_UDP_FRAME_SIZE:
            self.jobs_dropped += 1
            return
        
        # Pre-slice into HID reports (done once, shared immutable tuple)
        reports = []
        offset = 0
        while offset < len(udp_data):
            chunk = udp_data[offset:offset + DEFAULT_REPORT_SIZE]
            # Report format: [report_id=0] + [data padded to 64 bytes]
            report = bytes([FEATURE_REPORT_ID]) + chunk.ljust(DEFAULT_REPORT_SIZE, b'\x00')
            reports.append(report)
            offset += DEFAULT_REPORT_SIZE
        
        job = (tuple(reports), time.monotonic(), self._sequence)
        self._sequence += 1
        
        # EXCEPTION-SAFE BROADCAST
        # Lock serializes producers; try/except handles rare consumer races
        enqueued_count = 0
        with self._broadcast_lock:
            for wq in self.worker_queues:
                try:
                    wq.put_nowait(job)
                    enqueued_count += 1
                except queue.Full:
                    # Rare: queue filled between check iterations or due to timing
                    # Log partial enqueue but don't crash
                    if enqueued_count > 0:
                        self.jobs_partial += 1  # Some workers got it, some didn't
                    else:
                        self.jobs_dropped += 1  # No workers got it
                    return
        
        self.jobs_dispatched += 1
    
    def _worker_loop(self, worker_id: int):
        """
        Worker thread - pulls jobs, writes ONLY to devices assigned to this worker.
        
        CRITICAL: Each device has a tx_worker_id assigned at handshake time.
        This worker ONLY writes to devices where tx_worker_id == worker_id.
        This guarantees single-writer-per-device (no HID handle races).
        
        Per-device lock is held during write burst for cross-platform safety.
        """
        my_queue = self.worker_queues[worker_id]
        
        while self._running.is_set():
            try:
                job = my_queue.get(timeout=0.05)
            except queue.Empty:
                self.worker_busy[worker_id] = False
                continue
            
            self.worker_busy[worker_id] = True
            reports, timestamp, seq = job
            
            # CRITICAL: Only write to MY assigned devices (sharding)
            devices = self.registry.get_by_tx_worker(worker_id)
            for dev_state in devices:
                try:
                    # Lock protects against concurrent HID access from other subsystems
                    with dev_state.lock:
                        for report in reports:
                            dev_state.dev.write(report)
                    dev_state.packets_tx += len(reports)
                    dev_state.mark_activity()
                except Exception as e:
                    dev_state.set_state(DeviceState.ERROR, "TX ERROR")
                    self.uiq.put(('log', dev_state.name, f"TX error: {e}"))
            
            self.worker_busy[worker_id] = False
    
    def stop(self):
        self._running.clear()
        for t in self.workers:
            t.join(timeout=1.0)
    
    def get_queue_depth(self) -> int:
        """Returns max queue depth across all workers."""
        return max(wq.qsize() for wq in self.worker_queues) if self.worker_queues else 0

# ══════════════════════════════════════════════════════════════════════════════
# RX POLLER (non-blocking HID reads)
# ══════════════════════════════════════════════════════════════════════════════

class RxPoller:
    """
    Polls assigned devices for incoming data using non-blocking reads.
    One poller handles a subset of devices (round-robin assignment).
    """
    
    def __init__(self, poller_id: int, registry: DeviceRegistry,
                 ui_queue: queue.Queue,
                 on_command: Callable[[str], None]):
        self.poller_id = poller_id
        self.registry = registry
        self.uiq = ui_queue
        self.on_command = on_command
        
        self._running = threading.Event()
        self._thread: Optional[threading.Thread] = None
        self.is_busy = False
    
    def start(self):
        self._running.set()
        self._thread = threading.Thread(target=self._poll_loop, daemon=True)
        self._thread.name = f"RxPoller-{self.poller_id}"
        self._thread.start()
    
    def stop(self):
        self._running.clear()
        if self._thread:
            self._thread.join(timeout=1.0)
    
    def _poll_loop(self):
        """
        Main polling loop - implements trigger-based drain pattern.
        
        CRITICAL: The firmware sends an Input Report as a TRIGGER to signal
        that there's data in the Feature Report buffer to drain. We must
        only call get_feature_report() AFTER receiving this trigger.
        
        Per-device lock protects HID operations for cross-platform safety.
        Network/UI operations happen OUTSIDE the lock to prevent contention.
        """
        while self._running.is_set():
            devices = self.registry.get_by_poller(self.poller_id)
            devices = [d for d in devices if d.is_ready()]
            
            if not devices:
                self.is_busy = False
                time.sleep(0.01)
                continue
            
            self.is_busy = True
            
            for dev_state in devices:
                if dev_state.state != DeviceState.READY:
                    continue
                
                # Collect messages while holding lock, send after releasing
                messages_to_send = []
                
                try:
                    # Lock protects HID operations ONLY
                    with dev_state.lock:
                        # Non-blocking read - check for TRIGGER (Input Report)
                        data = dev_state.dev.read(DEFAULT_REPORT_SIZE, timeout_ms=0)
                        
                        # CRITICAL: Only drain Feature Reports if we received a trigger!
                        if data:
                            messages_to_send = self._drain_feature_reports_locked(dev_state)
                    
                    # OUTSIDE LOCK: Send to network and UI (no contention risk)
                    for msg in messages_to_send:
                        self.on_command(msg + "\n")
                        self.uiq.put(('log', dev_state.name, f"IN: {msg}"))
                    
                except Exception as e:
                    dev_state.set_state(DeviceState.ERROR, "RX ERROR")
                    self.uiq.put(('log', dev_state.name, f"RX error: {e}"))
            
            self.is_busy = False
            time.sleep(RX_POLL_INTERVAL_MS / 1000.0)
    
    def _drain_feature_reports_locked(self, dev_state: DeviceState) -> List[str]:
        """
        Read ALL pending Feature Reports. MUST be called with dev_state.lock held.
        Returns list of command strings to send (processed outside lock).
        """
        MAX_DRAIN_ITERATIONS = 64
        drained_count = 0
        messages = []
        
        for _ in range(MAX_DRAIN_ITERATIONS):
            try:
                resp = dev_state.dev.get_feature_report(
                    FEATURE_REPORT_ID,
                    DEFAULT_REPORT_SIZE + 1
                )
                payload = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                
                if not any(payload):
                    break
                
                msg = payload.rstrip(b'\x00').decode(errors='replace').strip()
                
                if not msg:
                    continue
                
                if msg == HANDSHAKE_REQ.decode():
                    continue
                
                # Collect message (will be sent outside lock)
                messages.append(msg)
                dev_state.packets_rx += 1
                dev_state.mark_activity()
                drained_count += 1
                
            except Exception:
                break
        
        if drained_count >= MAX_DRAIN_ITERATIONS:
            self.uiq.put(('log', dev_state.name, 
                         f"WARNING: Drain hit safety cap ({MAX_DRAIN_ITERATIONS})"))
        
        return messages

# ══════════════════════════════════════════════════════════════════════════════
# HANDSHAKE MANAGER
# ══════════════════════════════════════════════════════════════════════════════

class HandshakeManager:
    """
    Handles device handshakes in a dedicated thread.
    New devices queue here after hotplug, transition to READY when complete.
    """
    
    def __init__(self, registry: DeviceRegistry, ui_queue: queue.Queue,
                 get_next_poller: Callable[[], int],
                 get_next_tx_worker: Callable[[], int]):
        self.registry = registry
        self.uiq = ui_queue
        self.get_next_poller = get_next_poller
        self.get_next_tx_worker = get_next_tx_worker
        
        self.pending: queue.Queue = queue.Queue()
        self._running = threading.Event()
        self._thread: Optional[threading.Thread] = None
    
    def start(self):
        self._running.set()
        self._thread = threading.Thread(target=self._handshake_loop, daemon=True)
        self._thread.name = "HandshakeMgr"
        self._thread.start()
    
    def stop(self):
        self._running.clear()
        if self._thread:
            self._thread.join(timeout=2.0)
    
    def enqueue(self, dev_state: DeviceState):
        self.pending.put(dev_state)
    
    def _handshake_loop(self):
        while self._running.is_set():
            try:
                dev_state = self.pending.get(timeout=0.1)
            except queue.Empty:
                continue
            
            self.uiq.put(('log', dev_state.name, "Starting handshake..."))
            self.uiq.put(('status', dev_state.name, None))
            
            if self._do_handshake(dev_state):
                # Clear any stale data in the Feature Report buffer
                # This prevents old commands from being sent to DCS on reconnection
                if not self._clear_backlog(dev_state):
                    dev_state.set_state(DeviceState.ERROR, "BACKLOG CLEAR FAILED")
                    self.uiq.put(('log', dev_state.name, "Failed to clear backlog"))
                    self.uiq.put(('status', dev_state.name, None))
                    continue
                
                # Assign to workers (CRITICAL: must happen before READY state)
                dev_state.poller_id = self.get_next_poller()
                dev_state.tx_worker_id = self.get_next_tx_worker()  # Single-writer assignment
                
                # Enable non-blocking mode BEFORE setting READY (proper ordering)
                try:
                    dev_state.dev.set_nonblocking(1)
                except Exception as e:
                    self.uiq.put(('log', dev_state.name, f"Note: set_nonblocking failed ({e})"))
                    # Continue anyway - polling will still work
                
                dev_state.set_state(DeviceState.READY, "READY")
                
                self.uiq.put(('log', dev_state.name, 
                             f"Handshake complete, ready to process input."))
                self.uiq.put(('status', dev_state.name, None))
            else:
                dev_state.set_state(DeviceState.ERROR, "HANDSHAKE FAILED")
                self.uiq.put(('log', dev_state.name, "Handshake failed"))
                self.uiq.put(('status', dev_state.name, None))
    
    def _clear_backlog(self, dev_state: DeviceState) -> bool:
        """
        Clear any stale Feature Report data after handshake.
        Lock protects HID operations for cross-platform safety.
        """
        dev = dev_state.dev
        max_attempts = 100
        
        for attempt in range(max_attempts):
            if not self._running.is_set():
                return False
            
            try:
                with dev_state.lock:
                    resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                    payload = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                    
                    # Empty buffer = all zeros
                    if not any(payload):
                        return True
                    
            except Exception:
                return False
        
        # Timeout - buffer never cleared
        return False
    
    def _do_handshake(self, dev_state: DeviceState) -> bool:
        """Perform FIFO handshake sequence. Lock protects HID operations."""
        dev = dev_state.dev
        payload = HANDSHAKE_REQ.ljust(DEFAULT_REPORT_SIZE, b'\x00')
        
        for attempt in range(50):  # Max ~10 seconds
            if not self._running.is_set():
                return False
            
            try:
                with dev_state.lock:
                    # Check if device already echoed handshake
                    resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                    data = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                    if data.rstrip(b'\x00') == HANDSHAKE_REQ:
                        return True
                    
                    # Send handshake request
                    dev.send_feature_report(bytes([FEATURE_REPORT_ID]) + payload)
                    
                    # Check response
                    resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                    data = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                    if data.rstrip(b'\x00') == HANDSHAKE_REQ:
                        return True
                
            except Exception:
                pass
            
            time.sleep(0.2)
        
        return False

# ══════════════════════════════════════════════════════════════════════════════
# HOTPLUG MONITOR
# ══════════════════════════════════════════════════════════════════════════════

class HotplugMonitor:
    """Monitors for USB device connect/disconnect events."""
    
    def __init__(self, vid: int, pid: Optional[int],
                 registry: DeviceRegistry,
                 handshake_mgr: HandshakeManager,
                 ui_queue: queue.Queue,
                 reconnection_tracker: Dict[str, int]):
        self.vid = vid
        self.pid = pid
        self.registry = registry
        self.handshake_mgr = handshake_mgr
        self.uiq = ui_queue
        self.reconnection_tracker = reconnection_tracker
        
        self._running = threading.Event()
        self._thread: Optional[threading.Thread] = None
    
    def start(self):
        self._running.set()
        self._thread = threading.Thread(target=self._monitor_loop, daemon=True)
        self._thread.name = "HotplugMon"
        self._thread.start()
    
    def stop(self):
        self._running.clear()
        if self._thread:
            self._thread.join(timeout=2.0)
    
    def _monitor_loop(self):
        while self._running.is_set():
            try:
                self._scan_devices()
            except Exception as e:
                self.uiq.put(('log', 'Hotplug', f"Scan error: {e}"))
            
            time.sleep(1.0)
    
    def _scan_devices(self):
        # Build map of currently connected USB devices
        usb_devices: Dict[str, dict] = {}
        for d in hid.enumerate():
            if d['vendor_id'] != self.vid:
                continue
            if self.pid is not None and d['product_id'] != self.pid:
                continue
            
            key = d.get('serial_number', '') or str(d.get('path', b''))
            if key:
                usb_devices[key] = d
        
        # Detect disconnections
        for dev_state in self.registry.get_all():
            key = dev_state.serial or str(dev_state.path)
            if key not in usb_devices:
                self._handle_disconnect(dev_state)
        
        # Detect new connections
        for key, info in usb_devices.items():
            existing = self.registry.get(key)
            if existing is None:
                self._handle_connect(info)
    
    def _handle_connect(self, info: dict):
        key = info.get('serial_number', '') or str(info.get('path', b''))
        
        try:
            dev = hid.device()
            dev.open_path(info['path'])
        except Exception as e:
            self.uiq.put(('log', 'Hotplug', f"Failed to open device: {e}"))
            return
        
        dev_state = DeviceState(dev, info)
        dev_state.reconnections = self.reconnection_tracker.get(key, 0)
        self.reconnection_tracker[key] = dev_state.reconnections + 1
        
        if self.registry.add(dev_state):
            # First connect shows "Connected", subsequent show reconnect count
            if dev_state.reconnections == 0:
                self.uiq.put(('log', dev_state.name, "Connected"))
            else:
                self.uiq.put(('log', dev_state.name, f"Reconnected (#{dev_state.reconnections})"))
            self.uiq.put(('status', dev_state.name, None))
            self.handshake_mgr.enqueue(dev_state)
    
    def _handle_disconnect(self, dev_state: DeviceState):
        self.registry.remove(dev_state)
        dev_state.set_state(DeviceState.DISCONNECTED)
        
        try:
            dev_state.dev.close()
        except Exception:
            pass
        
        self.uiq.put(('log', dev_state.name, "Disconnected"))
        self.uiq.put(('status', dev_state.name, None))

# ══════════════════════════════════════════════════════════════════════════════
# UDP NETWORK LAYER
# ══════════════════════════════════════════════════════════════════════════════

class UdpNetwork:
    """Handles UDP multicast RX (from DCS) and unicast TX (to DCS)."""
    
    def __init__(self, tx_dispatcher: TxDispatcher, ui_queue: queue.Queue):
        self.tx_dispatcher = tx_dispatcher
        self.uiq = ui_queue
        
        self.reply_addr: Optional[str] = STORED_DCS_IP
        self.rx_sock: Optional[socket.socket] = None
        self.tx_sock: Optional[socket.socket] = None
        
        self._running = threading.Event()
        self._rx_thread: Optional[threading.Thread] = None
        self._ip_committed = False
        
        # Stats
        self.frames_total = 0
        self.frames_window = 0
        self.bytes_window = 0
        self.window_start = time.monotonic()
    
    def start(self):
        self._running.set()
        
        # TX socket
        self.tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        
        # RX thread
        self._rx_thread = threading.Thread(target=self._rx_loop, daemon=True)
        self._rx_thread.name = "UdpRx"
        self._rx_thread.start()
    
    def stop(self):
        self._running.clear()
        
        # Explicitly leave multicast group (platform-dependent but "flawless" cleanup)
        if self.rx_sock:
            try:
                mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
                self.rx_sock.setsockopt(socket.IPPROTO_IP, socket.IP_DROP_MEMBERSHIP, mreq)
            except Exception:
                pass
        
        # Close sockets
        for sock in (self.rx_sock, self.tx_sock):
            if sock:
                try:
                    sock.close()
                except Exception:
                    pass
        
        # Join RX thread for deterministic shutdown (NASA-grade lifecycle)
        if self._rx_thread:
            self._rx_thread.join(timeout=1.0)
    
    def send_command(self, cmd: str):
        """Send ASCII command to DCS-BIOS."""
        if self.tx_sock and self.reply_addr:
            try:
                self.tx_sock.sendto(cmd.encode(), (self.reply_addr, DEFAULT_DCS_TX_PORT))
            except Exception as e:
                self.uiq.put(('log', 'UDP', f"TX error: {e}"))
    
    def _rx_loop(self):
        """Receive UDP multicast from DCS-BIOS."""
        try:
            self.rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            
            try:
                self.rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, UDP_RX_BUFFER_SIZE)
            except Exception:
                pass
            
            self.rx_sock.bind(('', DEFAULT_UDP_PORT))
            
            # CRITICAL: Set timeout for deterministic shutdown (NASA-grade requirement)
            self.rx_sock.settimeout(0.5)
            
            mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
            self.rx_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            
            self.uiq.put(('log', 'UDP', f"Listening on {DEFAULT_MULTICAST_IP}:{DEFAULT_UDP_PORT}"))
            
            while self._running.is_set():
                try:
                    data, addr = self.rx_sock.recvfrom(4096)  # Match MAX_UDP_FRAME_SIZE
                except socket.timeout:
                    continue
                except OSError:
                    break
                
                # Learn DCS address from first packet
                if not self._ip_committed and addr and is_valid_ipv4(addr[0]):
                    if self.reply_addr != addr[0]:
                        self.reply_addr = addr[0]
                        write_dcs_ip(addr[0])
                        self._ip_committed = True
                        self.uiq.put(('data_source', None, addr[0]))
                        self.uiq.put(('log', 'UDP', f"DCS detected at {addr[0]}"))
                
                # Stats
                self.frames_total += 1
                self.frames_window += 1
                self.bytes_window += len(data)
                
                # Dispatch
                self.tx_dispatcher.dispatch(data)
                
        except Exception as e:
            self.uiq.put(('log', 'UDP', f"RX fatal error: {e}"))
    
    def get_stats(self) -> Dict[str, Any]:
        """Get current stats and reset window counters."""
        now = time.monotonic()
        duration = max(now - self.window_start, 0.001)
        
        hz = self.frames_window / duration
        kbps = (self.bytes_window / 1024.0) / duration
        avg_size = self.bytes_window / max(self.frames_window, 1)
        
        stats = {
            'frames': self.frames_total,
            'hz': f"{hz:.1f}",
            'kbps': f"{kbps:.1f}",
            'avg_size': f"{avg_size:.1f}",
        }
        
        # Reset window
        self.frames_window = 0
        self.bytes_window = 0
        self.window_start = now
        
        return stats

# ══════════════════════════════════════════════════════════════════════════════
# CURSES CONSOLE UI
# ══════════════════════════════════════════════════════════════════════════════

class ConsoleUI:
    """
    Professional curses-based console interface.
    Matches original CockpitOS HID Manager aesthetic.
    """
    
    # Color pair IDs
    COLOR_RED    = 1
    COLOR_GREEN  = 2
    COLOR_YELLOW = 3
    COLOR_CYAN   = 4
    COLOR_DIM    = 5
    
    def __init__(self, registry: DeviceRegistry, udp_network: 'UdpNetwork',
                 tx_dispatcher: TxDispatcher, rx_pollers: List[RxPoller],
                 ui_queue: Optional[queue.Queue] = None):
        self.registry = registry
        self.udp = udp_network
        self.tx_dispatcher = tx_dispatcher
        self.rx_pollers = rx_pollers
        
        # Use provided queue or create new one
        self.uiq: queue.Queue = ui_queue if ui_queue is not None else queue.Queue()
        self._running = threading.Event()
        
        # UI state
        self._log: List[str] = []
        self._stats = {
            'frames': '0',
            'hz': '0.0',
            'kbps': '0.0',
            'avg_size': '0.0',
            'src': STORED_DCS_IP or '(waiting...)',
        }
    
    def post(self, event):
        """Thread-safe event posting."""
        self.uiq.put(event)
    
    def run(self):
        """Main entry point - runs curses wrapper."""
        curses.wrapper(self._main_loop)
    
    def stop(self):
        self._running.clear()
    
    def _main_loop(self, stdscr):
        """Main curses loop."""
        # Initialize curses
        curses.curs_set(0)  # Hide cursor
        curses.use_default_colors()
        
        # Initialize color pairs
        curses.init_pair(self.COLOR_RED, curses.COLOR_RED, -1)
        curses.init_pair(self.COLOR_GREEN, curses.COLOR_GREEN, -1)
        curses.init_pair(self.COLOR_YELLOW, curses.COLOR_YELLOW, -1)
        curses.init_pair(self.COLOR_CYAN, curses.COLOR_CYAN, -1)
        curses.init_pair(self.COLOR_DIM, curses.COLOR_WHITE, -1)
        
        stdscr.timeout(UI_REFRESH_MS)
        self._running.set()
        
        last_stats_update = 0
        
        while self._running.is_set():
            # Process events
            self._consume_events()
            
            # Update stats periodically
            now = time.monotonic()
            if now - last_stats_update >= 1.0:
                stats = self.udp.get_stats()
                self._stats['frames'] = str(stats['frames'])
                self._stats['hz'] = stats['hz']
                self._stats['kbps'] = stats['kbps']
                self._stats['avg_size'] = stats['avg_size']
                last_stats_update = now
            
            # Render
            self._paint(stdscr)
            
            # Handle input (drain buffer, but only 'q' quits)
            ch = stdscr.getch()
            if ch == ord('q') or ch == 27:  # 'q' or ESC
                self._running.clear()
            
            time.sleep(0.01)
    
    def _consume_events(self):
        """Process all pending UI events."""
        while True:
            try:
                event = self.uiq.get_nowait()
            except queue.Empty:
                break
            
            typ, *rest = event
            
            if typ == 'data_source':
                # Event format: ('data_source', None, ip_address)
                self._stats['src'] = rest[-1]  # Explicitly use last element
            
            elif typ == 'log':
                dev_name, msg = rest
                timestamp = datetime.now().strftime("%H:%M:%S")
                line = f"[{timestamp}] [{dev_name}] {msg}"
                self._log.append(line)
                if len(self._log) > LOG_HISTORY_SIZE:
                    self._log = self._log[-LOG_HISTORY_SIZE:]
            
            elif typ == 'status':
                # Status updates are pulled directly from registry
                pass
    
    def _paint(self, stdscr):
        """Render the entire screen."""
        stdscr.erase()
        h, w = stdscr.getmaxyx()
        
        y = 0
        
        # === HEADER: Stats bar ===
        header = (
            f"Frames: {self._stats['frames']}   "
            f"Hz: {self._stats['hz']}   "
            f"kB/s: {self._stats['kbps']}   "
            f"Avg UDP Frame size (Bytes): {self._stats['avg_size']}   "
            f"Data Source: {self._stats['src']}"
        )
        self._addstr(stdscr, y, 0, header, w, curses.A_BOLD)
        y += 2
        
        # === DEVICE TABLE ===
        # Header row
        col_device = 38
        col_status = 16
        col_reconn = 14
        
        table_header = f"{'Device':<{col_device}} {'Status':<{col_status}} {'Reconnections':<{col_reconn}}"
        self._addstr(stdscr, y, 0, table_header, w, curses.A_NORMAL)
        y += 1
        
        # Device rows
        devices = self.registry.get_all()
        for dev in devices:
            if y >= h - 2:  # Leave room for log and status bar
                break
            
            # Determine color based on status
            status = dev.status_text
            attr = curses.A_NORMAL
            status_lower = status.lower()
            
            if 'ready' in status_lower:
                attr = curses.color_pair(self.COLOR_GREEN)
            elif 'handshak' in status_lower or 'wait' in status_lower:
                attr = curses.color_pair(self.COLOR_YELLOW)
            elif 'error' in status_lower or 'disconnect' in status_lower or 'off' in status_lower:
                attr = curses.color_pair(self.COLOR_RED)
            
            # Truncate device name if needed
            name = dev.name[:col_device-1] if len(dev.name) >= col_device else dev.name
            
            row = f"{name:<{col_device}} {status:<{col_status}} {dev.reconnections:<{col_reconn}}"
            self._addstr(stdscr, y, 0, row, w, attr)
            y += 1
        
        # === EVENT LOG ===
        y += 1  # Blank line separator
        
        # Calculate available space for log
        log_start = y
        log_end = h - 1  # Last line is status bar
        log_lines = max(0, log_end - log_start)
        
        # Show most recent log entries that fit
        if log_lines > 0 and self._log:
            visible_log = self._log[-log_lines:]
            for i, line in enumerate(visible_log):
                self._addstr(stdscr, log_start + i, 0, line, w, curses.A_NORMAL)
        
        # === STATUS BAR ===
        device_count = self.registry.count()
        ready_count = self.registry.count_ready()
        
        status_bar = f"{device_count} device(s) connected ({ready_count} ready).  Press 'q' to quit."
        self._addstr(stdscr, h - 1, 0, status_bar, w, curses.A_DIM)
        
        # Refresh
        stdscr.noutrefresh()
        curses.doupdate()
    
    def _addstr(self, stdscr, y: int, x: int, text: str, max_width: int, attr: int):
        """Safe addstr that handles boundaries."""
        h, w = stdscr.getmaxyx()
        if y < 0 or y >= h or x >= w:
            return
        
        # Truncate text to fit
        available = w - x - 1
        if available <= 0:
            return
        
        text = text[:available]
        
        try:
            stdscr.addstr(y, x, text, attr)
        except curses.error:
            pass  # Ignore curses errors at screen edges

# ══════════════════════════════════════════════════════════════════════════════
# MAIN APPLICATION
# ══════════════════════════════════════════════════════════════════════════════

class CockpitHidBridge:
    """Main application - orchestrates all components."""
    
    def __init__(self, vid: int, pid: Optional[int]):
        self.vid = vid
        self.pid = pid
        
        # Core state
        self.registry = DeviceRegistry()
        self.reconnection_tracker: Dict[str, int] = {}
        
        # Poller round-robin
        self._next_poller = 0
        self._poller_lock = threading.Lock()
        
        # Will be initialized in start()
        self.ui: Optional[ConsoleUI] = None
        self.tx_dispatcher: Optional[TxDispatcher] = None
        self.udp: Optional[UdpNetwork] = None
        self.handshake_mgr: Optional[HandshakeManager] = None
        self.hotplug: Optional[HotplugMonitor] = None
        self.rx_pollers: List[RxPoller] = []
    
    def _get_next_poller(self) -> int:
        with self._poller_lock:
            poller_id = self._next_poller
            self._next_poller = (self._next_poller + 1) % RX_POLLERS
            return poller_id
    
    def start(self):
        """Initialize and start all components."""
        # Create shared UI queue FIRST - all components use the same queue
        # This eliminates the placeholder/rewiring pattern for cleaner architecture
        self.ui_queue: queue.Queue = queue.Queue()
        
        self.rx_pollers = []
        
        # TX dispatcher (with shared queue from start)
        self.tx_dispatcher = TxDispatcher(
            self.registry,
            self.ui_queue,
            num_workers=TX_WORKERS
        )
        
        # UDP network (with shared queue from start)
        self.udp = UdpNetwork(self.tx_dispatcher, self.ui_queue)
        
        # RX pollers (with shared queue and proper callback from start)
        for i in range(RX_POLLERS):
            poller = RxPoller(
                i, self.registry,
                self.ui_queue,
                on_command=lambda cmd: None  # Will be set after UDP created
            )
            self.rx_pollers.append(poller)
        
        # Now wire RX pollers to UDP send (UDP is created, callback is valid)
        for poller in self.rx_pollers:
            poller.on_command = self.udp.send_command
        
        # Create UI with the same shared queue
        self.ui = ConsoleUI(
            self.registry,
            self.udp,
            self.tx_dispatcher,
            self.rx_pollers,
            ui_queue=self.ui_queue  # Pass the shared queue
        )
        
        # Handshake manager (with shared queue from start)
        self.handshake_mgr = HandshakeManager(
            self.registry,
            self.ui_queue,
            self._get_next_poller,
            self.tx_dispatcher.get_next_tx_worker
        )
        
        # Hotplug monitor (with shared queue from start)
        self.hotplug = HotplugMonitor(
            self.vid, self.pid,
            self.registry,
            self.handshake_mgr,
            self.ui_queue,
            self.reconnection_tracker
        )
        
        # Start all subsystems
        self.ui.post(('log', 'System', 'Starting CockpitOS HID Bridge (Scalable Edition)...'))
        self.ui.post(('log', 'System', f'VID: 0x{self.vid:04X}, PID: {"Any" if self.pid is None else f"0x{self.pid:04X}"}'))
        self.ui.post(('log', 'System', f'Thread pool: {TX_WORKERS} TX + {RX_POLLERS} RX + 3 system'))
        
        self.udp.start()
        self.handshake_mgr.start()
        self.hotplug.start()
        
        for poller in self.rx_pollers:
            poller.start()
        
        self.ui.post(('log', 'System', 'All subsystems started'))
    
    def run(self):
        """Run the application (blocks until quit)."""
        self.start()
        
        try:
            self.ui.run()
        finally:
            self.stop()
    
    def stop(self):
        """Shutdown all components."""
        if self.ui:
            self.ui.post(('log', 'System', 'Shutting down...'))
        
        if self.hotplug:
            self.hotplug.stop()
        if self.handshake_mgr:
            self.handshake_mgr.stop()
        for poller in self.rx_pollers:
            poller.stop()
        if self.tx_dispatcher:
            self.tx_dispatcher.stop()
        if self.udp:
            self.udp.stop()

# ══════════════════════════════════════════════════════════════════════════════
# ENTRY POINT
# ══════════════════════════════════════════════════════════════════════════════

def main():
    print("CockpitOS HID Bridge - Scalable Edition")
    print(f"VID: 0x{USB_VID:04X}, PID: {'Any' if USB_PID is None else f'0x{USB_PID:04X}'}")
    print(f"Architecture: {TX_WORKERS} TX workers + {RX_POLLERS} RX pollers (fixed)")
    print("Starting curses interface...")
    print()
    
    bridge = CockpitHidBridge(USB_VID, USB_PID)
    
    try:
        bridge.run()
    except KeyboardInterrupt:
        pass
    finally:
        lock.release()
    
    print("Goodbye!")

if __name__ == "__main__":
    main()
