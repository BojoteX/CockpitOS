# --- main.py --- (dependencies and lock logic at the top)
import sys, re, os

HEADLESS   = ('--console' in sys.argv) or ('--headless' in sys.argv)
IS_WINDOWS = (os.name == 'nt') or sys.platform.startswith('win')

REQUIRED_MODULES = {
    "hid":      "hidapi",
    "filelock": "filelock",
    # GUI needs tkinter; headless does not
    **({} if HEADLESS else {"tkinter": None}),
    # Headless on Windows needs curses via windows-curses; on non-Windows just check curses
    **({"curses": ("windows-curses" if IS_WINDOWS else None)} if HEADLESS else {}),
}

missing = []
for mod, pip_name in REQUIRED_MODULES.items():
    try:
        __import__(mod)
    except ImportError:
        missing.append(pip_name if pip_name else mod)

if missing:
    # Build a clean pip install line (skip stdlib names like 'tkinter'/'curses' when pip_name is None)
    to_pip = [m for m in missing if m not in ("tkinter", "curses")]
    msg =  "Some required modules are missing:\n\n"
    for m in missing:
        msg += f"- {m}\n"
    if to_pip:
        msg += f"\nTo install the missing Python modules, run:\n\n    pip install {' '.join(to_pip)}\n"
    msg += "\nAfter installing, restart this program."
    if HEADLESS:
        print(msg)
    else:
        import tkinter as tk
        from tkinter import scrolledtext as ST
        root = tk.Tk(); root.title("Missing Required Modules"); root.geometry("560x320"); root.resizable(False, False)
        lbl = tk.Label(root, text="Missing required modules for CockpitController HID Handler:",
                       font=("Arial", 12, "bold"), pady=10); lbl.pack()
        text = ST.ScrolledText(root, width=68, height=10, font=("Consolas", 10))
        text.pack(padx=12, pady=(0,10)); text.insert("1.0", msg); text.config(state='normal'); text.focus()
        tk.Button(root, text="Close", command=root.destroy, width=18).pack(pady=10)
        root.protocol("WM_DELETE_WINDOW", root.destroy); root.mainloop()
    sys.exit(1)

from filelock import FileLock, Timeout

LOCKFILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cockpitos_dashboard.lock")

try:
    lock = FileLock(LOCKFILE + ".lock")
    lock.acquire(timeout=0.1)
except Timeout:
    print("ERROR: Another instance of CockpitController HID Handler is already running.")
    sys.exit(1)

# Import tkinter only if GUI mode
if not HEADLESS:
    import tkinter as tk

os.chdir(os.path.dirname(os.path.abspath(__file__)))

# --- config.py ---
import configparser
import time
import threading
import ipaddress

DEFAULT_REPORT_SIZE = 64
DEFAULT_MULTICAST_IP = "239.255.50.10"
DEFAULT_UDP_PORT = 5010
HANDSHAKE_REQ = b"DCSBIOS-HANDSHAKE"
FEATURE_REPORT_ID = 0
IDLE_TIMEOUT = 2.0
MAX_DEVICES = 20
LOCKFILE = "cockpitos_dashboard.lock"

def is_bt_serial(s: str) -> bool:
    # MAC forms: AA:BB:CC:DD:EE:FF or AA-BB-CC-DD-EE-FF, plain 12-hex, or your BX-<16hex>
    return bool(re.fullmatch(
        r'(?i)(?:[0-9a-f]{2}[:-]){5}[0-9a-f]{2}|[0-9a-f]{12}|bx-[0-9a-f]{16}', s))

def read_settings_from_ini(filename="settings.ini"):
    config = configparser.ConfigParser()
    if not os.path.isfile(filename):
        config['USB'] = {'VID': '0xCAFE', 'PID': '0xCAF3'}
        config['DCS'] = {'UDP_SOURCE_IP': '127.0.0.1'}
        with open(filename, 'w') as configfile:
            config.write(configfile)
    config.read(filename)
    try:
        vid = int(config['USB']['VID'], 0)
        pid = int(config['USB']['PID'], 0)
    except Exception:
        vid, pid = 0xCAFE, 0xCAF3
    try:
        dcs_ip = config['DCS'].get('UDP_SOURCE_IP', '127.0.0.1')
    except Exception:
        dcs_ip = '127.0.0.1'
    return vid, pid, dcs_ip

def write_settings_dcs_ip(new_ip, filename="settings.ini"):
    config = configparser.ConfigParser()
    config.read(filename)
    if 'DCS' not in config:
        config['DCS'] = {}
    config['DCS']['UDP_SOURCE_IP'] = new_ip
    with open(filename, 'w') as configfile:
        config.write(configfile)

def is_valid_ipv4(ip):
    try:
        ip_obj = ipaddress.ip_address(ip)
        return (
            isinstance(ip_obj, ipaddress.IPv4Address)
            and not ip_obj.is_multicast
            and not ip_obj.is_unspecified
            # allow loopback so console header updates for local DCS
            # and not ip_obj.is_loopback
        )
    except Exception:
        return False

USB_VID, USB_PID, STORED_DCS_IP = read_settings_from_ini()
stats = {
    "frame_count_total": 0,
    "frame_count_window": 0,
    "bytes": 0,
    "start_time": time.time(),
    "bytes_rolling": 0,
    "frames_rolling": 0,
}
global_stats_lock = threading.Lock()

reply_addr = [STORED_DCS_IP]
prev_reconnections = {}

# --- hid_device.py ---
import hid
import time

def list_target_devices():
    return [d for d in hid.enumerate() if d['vendor_id'] == USB_VID and d['product_id'] == USB_PID]

def try_fifo_handshake(dev, uiq=None, device_name=None):
    payload = HANDSHAKE_REQ.ljust(DEFAULT_REPORT_SIZE, b'\x00')
    attempts = 0
    while True:
        try:
            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
            msg = d.rstrip(b'\x00')
            if msg == HANDSHAKE_REQ:
                return True
        except Exception as e:
            if uiq: uiq.put(('handshake', device_name, f"GET FEATURE exception: {e}"))
            time.sleep(0.2)
            continue

        try:
            dev.send_feature_report(bytes([FEATURE_REPORT_ID]) + payload)
        except Exception as e:
            if uiq: uiq.put(('handshake', device_name, f"SET FEATURE exception: {e}"))
            time.sleep(0.2)
            continue

        try:
            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
            msg = d.rstrip(b'\x00')
            if msg == HANDSHAKE_REQ:
                return True
        except Exception as e:
            if uiq: uiq.put(('handshake', device_name, f"GET FEATURE exception: {e}"))
            time.sleep(0.2)
            continue

        attempts += 1
        if attempts % 10 == 0 and uiq:
            uiq.put(('handshake', device_name, "Waiting for handshake..."))
        time.sleep(0.2)

class DeviceEntry:
    def __init__(self, dev, dev_info):
        self.dev = dev
        self.info = dev_info

        serial  = (dev_info.get('serial_number')  or '')
        product = (dev_info.get('product_string') or '')
        self.name = (product or serial) if is_bt_serial(serial) else (serial or product)

        self.status = "WAIT HANDSHAKE"
        self.last_sent = time.time()
        self.disconnected = False
        self.handshaked = False
        self.reconnections = 0

    def check(self):
        now = time.time()
        if self.disconnected:
            self.status = "OFF"
        elif (now - self.last_sent) > IDLE_TIMEOUT:
            self.status = "IDLE"
        else:
            self.status = "RECV"

def _close_stale_handle(entry, uiq, where, exc):
    try: entry.dev.close()
    except Exception: pass
    entry.disconnected = True  # only this entry/thread
    uiq.put(('status', entry.name, f"STALE HANDLE ({where}) — closing old thread"))
    uiq.put(('log', entry.name, f"[stale] {where} exception: {exc}"))

# replace device_reader(...) with this version
def device_reader(entry, uiq, udp_send):
    dev = entry.dev
    try:
        while not entry.handshaked and not entry.disconnected:
            entry.handshaked = try_fifo_handshake(dev, uiq=uiq, device_name=entry.name)
            if not entry.handshaked:
                _close_stale_handle(entry, uiq, "HANDSHAKE", "no reply")
                return
            uiq.put(('status', entry.name, "READY"))
            uiq.put(('log', entry.name, "Handshake complete, ready to process input."))
            entry.status = "READY"

        if reply_addr[0] is None and not entry.disconnected:
            uiq.put(('log', entry.name, "Waiting for DCS mission start..."))
        while reply_addr[0] is None and not entry.disconnected:
            time.sleep(0.2)
        if entry.disconnected: return

        uiq.put(('log', entry.name, f"DCS detected on {reply_addr[0]} — Starting normal operation."))

        # Clear backlog (bounded attempts)
        cleared = False
        max_attempts = 100
        attempt = 0
        while not cleared and not entry.disconnected and attempt < max_attempts:
            try:
                resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                if not any(d): cleared = True
            except Exception as e:
                _close_stale_handle(entry, uiq, "FEATURE-DRAIN", e)
                return
            attempt += 1
        if not cleared:
            _close_stale_handle(entry, uiq, "FEATURE-DRAIN", "timeout")
            return

        # Main loop
        while not entry.disconnected and entry.handshaked:
            try:
                data = dev.read(DEFAULT_REPORT_SIZE, timeout_ms=-1)  # blocking
                if not data: continue
            except Exception as e:
                _close_stale_handle(entry, uiq, "READ", e)
                return

            # bounded feature-drain after trigger
            drain = 0
            while not entry.disconnected and drain < 8:
                try:
                    resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                    d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                    msg = d.rstrip(b'\x00').decode(errors="replace").strip()
                    if not msg or msg == HANDSHAKE_REQ.decode(): break
                    uiq.put(('log', entry.name, f"IN: {msg}"))
                    udp_send(msg + "\n")
                    drain += 1
                except Exception as e:
                    _close_stale_handle(entry, uiq, "FEATURE", e)
                    return

    finally:
        try: dev.close()
        except Exception: pass

# --- network.py (drop-in replacement for NetworkManager) ---
import socket, struct, threading, time
from collections import deque

class NetworkManager:
    class _DeviceTxWorker(threading.Thread):
        def __init__(self, entry, uiq):
            super().__init__(daemon=True)
            self.entry = entry
            self.uiq = uiq
            self.q = deque()                 # unbounded FIFO (process everything)
            self.cv = threading.Condition()
            self._running = True

        def enqueue(self, reports_tuple):
            with self.cv:
                self.q.append(reports_tuple)  # FIFO
                self.cv.notify()

        def stop(self):
            with self.cv:
                self._running = False
                self.cv.notify()

        def run(self):
            dev = self.entry.dev
            while self._running and not self.entry.disconnected:
                # wait for work
                with self.cv:
                    while self._running and not self.q:
                        self.cv.wait(timeout=0.05)
                    if not self._running or self.entry.disconnected:
                        break
                    # move all pending work into a local batch (minimize lock time)
                    batch = list(self.q)
                    self.q.clear()

                # drain batch in FIFO order
                for reports in batch:
                    for rep in reports:
                        try:
                            dev.write(rep)  # only this thread touches the handle
                        except Exception:
                            self.entry.disconnected = True
                            self.uiq.put(('status', self.entry.name, "DISCONNECTED"))
                            return

                # tiny yield to avoid 100% CPU if traffic is sparse
                time.sleep(0.0005)

    def __init__(self, uiq, reply_addr_ref, get_devices_callback):
        self.uiq = uiq
        self.reply_addr = reply_addr_ref
        self.get_devices = get_devices_callback
        self.udp_rx_sock = None
        self.udp_tx_sock = None
        self._running = threading.Event()
        self._ip_committed = False
        self._workers = {}  # entry -> _DeviceTxWorker

    def start(self):
        self._running.set()
        threading.Thread(target=self._udp_rx_processor, daemon=True).start()
        self._start_udp_tx_sock()

    def stop(self):
        self._running.clear()
        if self.udp_rx_sock:
            try: self.udp_rx_sock.close()
            except Exception: pass
        if self.udp_tx_sock:
            try: self.udp_tx_sock.close()
            except Exception: pass
        # stop workers
        for w in list(self._workers.values()):
            w.stop()
        self._workers.clear()

    def _start_udp_tx_sock(self):
        self.udp_tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    def _ensure_worker(self, entry):
        w = self._workers.get(id(entry))
        if w and (entry.disconnected or not entry.handshaked):
            # stale worker; drop it
            try: w.stop()
            except Exception: pass
            self._workers.pop(id(entry), None)
            w = None
        if not w and entry.handshaked and not entry.disconnected:
            w = NetworkManager._DeviceTxWorker(entry, self.uiq)
            self._workers[id(entry)] = w
            w.start()
        return w

    def _udp_rx_processor(self):
        try:
            self.udp_rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            # optional: larger RX buffer to handle bursts
            try: self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4*1024*1024)
            except Exception: pass
            self.udp_rx_sock.bind(('', DEFAULT_UDP_PORT))
            mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
            self.udp_rx_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

            while self._running.is_set():
                data, addr = self.udp_rx_sock.recvfrom(4096)

                # learn data source once
                if (not self._ip_committed and addr and is_valid_ipv4(addr[0])
                        and self.reply_addr[0] != addr[0]):
                    self.reply_addr[0] = addr[0]
                    write_settings_dcs_ip(addr[0])
                    self._ip_committed = True
                    self.uiq.put(('data_source', None, addr[0]))

                with global_stats_lock:
                    stats["frame_count_total"] += 1
                    stats["frame_count_window"] += 1
                    stats["bytes_rolling"] += len(data)
                    stats["frames_rolling"] += 1
                    stats["bytes"] += len(data)

                # pre-slice once per datagram
                reports = []
                offset = 0
                while offset < len(data):
                    chunk = data[offset:offset + DEFAULT_REPORT_SIZE]
                    rep = bytes([0]) + chunk
                    rep += b'\x00' * ((DEFAULT_REPORT_SIZE + 1) - len(rep))
                    reports.append(rep)
                    offset += DEFAULT_REPORT_SIZE
                reports = tuple(reports)  # immutable, share to all workers

                # enqueue to each device worker (parallel fan-out)
                for entry in self.get_devices():
                    if entry.handshaked and not entry.disconnected:
                        w = self._ensure_worker(entry)
                        if w:
                            w.enqueue(reports)

        except Exception as e:
            if self._running.is_set():
                self.uiq.put(('log', "UDP", f"UDP RX processor error: {e}"))

    def udp_send_report(self, msg, port=7778):
        if self.udp_tx_sock and self.reply_addr[0]:
            try:
                self.udp_tx_sock.sendto(msg.encode(), (self.reply_addr[0], port))
            except Exception as e:
                self.uiq.put(('log', "UDP", f"[UDP SEND ERROR] {e}"))
        elif not self.reply_addr[0]:
            self.uiq.put(('log', "UDP", "UDP TX: reply_addr not set, cannot send."))

# --- gui.py ---
from tkinter import ttk, scrolledtext
import queue
import threading
import time
from datetime import datetime

def log_ts():
    return f"[{datetime.now().strftime('%H:%M:%S')}]"

class CockpitGUI:
    def __init__(self, root, network_mgr):
        self.devices_lock = threading.Lock()
        self.root = root
        root.title("CockpitOS Updater")
        self.uiq = queue.Queue()
        self.network_mgr = network_mgr

        self.stats_frame = ttk.LabelFrame(root, text="Global Stream Stats")
        self.stats_frame.pack(fill='x', padx=10, pady=(10,0))

        self.stats_vars = {
            'frames': tk.StringVar(value="0"),
            'hz': tk.StringVar(value="0.0"),
            'bw': tk.StringVar(value="0.0"),
            'avgudp': tk.StringVar(value="0.0"),
        }
        self.data_source_var = tk.StringVar(value=f"Data Source: {reply_addr[0]}")
        # self.data_source_var = tk.StringVar(value=f"Data Source: {self.network_mgr.reply_addr[0] if self.network_mgr else '(waiting...)'}")  

        ttk.Label(self.stats_frame, text="Frames:").grid(row=0, column=0, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['frames']).grid(row=0, column=1, padx=5)
        ttk.Label(self.stats_frame, text="Hz:").grid(row=0, column=2, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['hz']).grid(row=0, column=3, padx=5)
        ttk.Label(self.stats_frame, text="kB/s:").grid(row=0, column=4, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['bw']).grid(row=0, column=5, padx=5)
        ttk.Label(self.stats_frame, text="Avg UDP Frame size (Bytes):").grid(row=0, column=6, padx=5)  # <--- New column header
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['avgudp']).grid(row=0, column=7, padx=5)  # <--- Value
        self.data_source_label = ttk.Label(self.stats_frame, textvariable=self.data_source_var, foreground="blue")
        self.data_source_label.grid(row=0, column=8, padx=12)  # Shift to column 8

        self.devices_frame = ttk.LabelFrame(root, text="Devices")
        self.devices_frame.pack(fill='both', expand=True, padx=10, pady=10)

        self.tree = ttk.Treeview(self.devices_frame, columns=('name','status','reconn'), show='headings')
        self.tree.column('name', width=220, anchor='w')
        self.tree.column('status', width=90, anchor='center')
        self.tree.column('reconn', width=120, anchor='center')
        self.tree.tag_configure('ready', foreground='green')
        self.tree.tag_configure('wait', foreground='black')
        self.tree.tag_configure('off', foreground='red')
        self.tree.heading('name', text='Device')
        self.tree.heading('status', text='Status')
        self.tree.heading('reconn', text='Reconnections')
        self.tree.pack(fill='x', expand=True)

        self.device_nodes = {}

        ttk.Label(root, text="Event Log:").pack(anchor='w', padx=10)
        self.log_text = scrolledtext.ScrolledText(root, width=90, height=15, state='disabled')
        self.log_text.pack(fill='both', expand=True, padx=10, pady=(0,10))

        self.statusbar = ttk.Label(root, text="Initializing...", anchor='w')
        self.statusbar.pack(fill='x', side='bottom')

        self.known_devices = {}
        self.devices = []

        self._start_device_thread()
        self._start_stats_thread()
        self.update_ui()

    def _start_device_thread(self):
        threading.Thread(target=self._device_monitor, daemon=True).start()

    def _device_monitor(self):
        while True:
            dev_infos = list_target_devices()
            current_serials = {d.get('serial_number', '') for d in dev_infos}
            to_remove = set(self.known_devices) - current_serials
            for serial in to_remove:
                entry = self.known_devices[serial]
                entry.disconnected = True
                try:
                    entry.dev.close()
                except Exception:
                    pass
                # Calculate last reconnection count for this device (0 on first connect, then increments)
                reconn = prev_reconnections.get(serial, 1) - 1
                self.uiq.put(('status', entry.name, f"DISCONNECTED (Reconnects: {reconn})"))
                del self.known_devices[serial]
                if entry.name in self.device_nodes:
                    self.uiq.put(('remove_device', entry.name))
                with self.devices_lock:
                    self.devices = [dev for dev in self.devices if dev.name != entry.name]
            for d in dev_infos:
                serial = d.get('serial_number', '')
                if serial not in self.known_devices:
                    dev = hid.device()
                    try:
                        dev.open_path(d['path'])
                    except Exception:
                        continue
                    entry = DeviceEntry(dev, d)
                    # reconnections starts at 0 for first connect, increments on every unplug/replug
                    entry.reconnections = prev_reconnections.get(serial, 0)
                    prev_reconnections[serial] = entry.reconnections + 1
                    self.known_devices[serial] = entry
                    with self.devices_lock:
                        self.devices.append(entry)

                    # REPLACED BELOW
                    # threading.Thread(
                    #    target=device_reader,
                    #    args=(entry, self.uiq, self.network_mgr.udp_send_report),
                    #    daemon=True
                    # ).start()

                    # ensure network_mgr is ready (set in main() after GUI is constructed)
                    nm = self.network_mgr
                    while nm is None:
                        time.sleep(0.05)
                        nm = self.network_mgr

                    threading.Thread(
                        target=device_reader,
                        args=(entry, self.uiq, nm.udp_send_report),
                        daemon=True
                    ).start()

                    self.uiq.put(('status', entry.name, "WAIT HANDSHAKE"))
            dev_count = len(self.devices)
            if dev_count:
                self.uiq.put(('statusbar', None, f"{dev_count} device(s) connected."))
            else:
                self.uiq.put(('statusbar', None, "No CockpitOS devices found."))
                time.sleep(2)
                continue
            time.sleep(1)  # Poll every second for changes

    def _start_stats_thread(self):
        threading.Thread(target=self._stats_updater, daemon=True).start()

    def _stats_updater(self):
        while True:
            time.sleep(1)
            with global_stats_lock:
                avg_frame = (stats["bytes_rolling"] / stats["frames_rolling"]) if stats["frames_rolling"] else 0
                duration = time.time() - stats["start_time"]
                hz = stats["frame_count_window"] / duration if duration > 0 else 0
                kbps = (stats["bytes"] / 1024) / duration if duration > 0 else 0
                stat_dict = {
                    'frames': stats["frame_count_total"],
                    'hz': f"{hz:.1f}",
                    'bw': f"{kbps:.1f}",
                    'avgudp': f"{avg_frame:.1f}",
                }
                self.uiq.put(('globalstats', stat_dict))
                stats["frame_count_window"] = 0
                stats["bytes"] = 0
                stats["start_time"] = time.time()

    def update_ui(self):
        try:
            while True:
                evt = self.uiq.get_nowait()
                self._handle_event(evt)
        except queue.Empty:
            pass
        self.root.after(100, self.update_ui)

    def _handle_event(self, evt):
        typ, *rest = evt
        if typ == 'data_source':
            self.data_source_var.set(f"Data Source: {rest[1]}")
        elif typ == 'statusbar':
            self.statusbar.config(text=rest[1])
        elif typ == 'status':
            devname, data = rest
            entry = next((dev for dev in self.devices if dev.name == devname), None)
            reconns = entry.reconnections if entry else 0
            tag = ()
            status_lower = data.lower()
            if 'ready' in status_lower:
                tag = ('ready',)
            elif 'wait' in status_lower or 'handshake' in status_lower:
                tag = ('wait',)
            elif 'off' in status_lower or 'disconn' in status_lower:
                tag = ('off',)
            if devname not in self.device_nodes:
                idx = self.tree.insert('', 'end', values=(devname, data, reconns), tags=tag)
                self.device_nodes[devname] = idx
            else:
                idx = self.device_nodes[devname]
                # Defensive: check if idx is still in the tree before updating
                if self.tree.exists(idx):
                    vals = list(self.tree.item(idx)['values'])
                    vals[1] = data
                    vals[2] = reconns
                    self.tree.item(idx, values=vals, tags=tag)
                else:
                    # Stale: remove from device_nodes to avoid future errors
                    del self.device_nodes[devname]
        elif typ in ('log', 'handshake'):
            devname, data = rest
            self.log_text['state'] = 'normal'
            line = f"{log_ts()} [{devname}] {data}\n"
            self.log_text.insert('end', line)
            self.log_text.see('end')
            self.log_text['state'] = 'disabled'
        elif typ == 'globalstats':
            for k, v in rest[0].items():
                self.stats_vars[k].set(v)
        elif typ == 'remove_device':
            devname = rest[0]
            if devname in self.device_nodes:
                idx = self.device_nodes[devname]
                if self.tree.exists(idx):
                    self.tree.delete(idx)
                del self.device_nodes[devname]

    def get_devices(self):
        with self.devices_lock:
            return list(self.devices)

# --- console_ui (headless) ---
import threading, time, queue
from datetime import datetime

if HEADLESS:
    try:
        import curses
    except ImportError:
        curses = None  # we'll error out nicely below

    LOG_KEEP = 2000
    def _ts(): return datetime.now().strftime("%H:%M:%S")

    class ConsoleUI:
        def __init__(self, get_devices_cb):
            self.get_devices = get_devices_cb
            self.uiq = queue.Queue()
            self._running = threading.Event()
            self._log = []
            self._stats = {"frames":"0","hz":"0.0","bw":"0.0","avgudp":"0.0","src":"(waiting...)"}
            self._stats["src"] = reply_addr[0] or "(waiting...)"
            self._rows = []

        def post(self, evt): self.uiq.put(evt)

        def _consume(self):
            while True:
                try: typ, *rest = self.uiq.get_nowait()
                except queue.Empty: break
                if typ == 'data_source':
                    self._stats['src'] = rest[1]
                elif typ == 'globalstats':
                    d = rest[0]
                    self._stats['frames'] = str(d.get('frames',"0"))
                    self._stats['hz']     = d.get('hz',"0.0")
                    self._stats['bw']     = d.get('bw',"0.0")
                    self._stats['avgudp'] = d.get('avgudp',"0.0")
                elif typ in ('log','handshake'):
                    dev, msg = rest
                    line = f"[{_ts()}] [{dev}] {msg}"
                    self._log.append(line)
                    if len(self._log) > LOG_KEEP: self._log = self._log[-LOG_KEEP:]

            rows = []
            for e in self.get_devices():
                rows.append((e.name, getattr(e, 'status', '?'), getattr(e, 'reconnections', 0)))
            rows.sort(key=lambda r: r[0])
            self._rows = rows

        def _paint(self, stdscr):
            stdscr.erase()
            h, w = stdscr.getmaxyx()
            hdr = f"Frames: {self._stats['frames']}   Hz: {self._stats['hz']}   kB/s: {self._stats['bw']}   " \
                  f"Avg UDP Frame size (Bytes): {self._stats['avgudp']}   Data Source: {self._stats['src']}"
            stdscr.addnstr(0, 0, hdr, w-1, curses.A_BOLD)
            stdscr.addnstr(2, 0, "Devices", w-1, curses.A_UNDERLINE)
            stdscr.addnstr(3, 0, f"{'Device':<38} {'Status':<16} {'Reconnections':<14}", w-1)
            y = 4
            for name, status, reconn in self._rows:
                attr = curses.A_NORMAL
                sl = status.lower()
                if 'ready' in sl: attr = curses.color_pair(2)
                elif ('wait' in sl) or ('handshake' in sl): attr = curses.color_pair(3)
                elif ('off' in sl) or ('disconn' in sl): attr = curses.color_pair(1)
                stdscr.addnstr(y, 0, f"{name:<38} {status:<16} {reconn:<14}", w-1, attr)
                y += 1
            y += 1
            stdscr.addnstr(y, 0, "Event Log:", w-1, curses.A_UNDERLINE)
            y += 1
            avail = max(0, h - y - 1)
            tail = self._log[-avail:] if avail else []
            for i, line in enumerate(tail):
                stdscr.addnstr(y+i, 0, line, w-1)
            dev_cnt = len(self._rows)
            stdscr.addnstr(h-1, 0, f"{dev_cnt} device(s) connected.  q=quit", w-1, curses.A_DIM)
            stdscr.noutrefresh()
            curses.doupdate()

        def _loop(self, stdscr):
            curses.curs_set(0)
            curses.use_default_colors()
            curses.init_pair(1, curses.COLOR_RED,   -1)
            curses.init_pair(2, curses.COLOR_GREEN, -1)
            curses.init_pair(3, curses.COLOR_YELLOW,-1)
            stdscr.timeout(100)
            self._running.set()
            while self._running.is_set():
                self._consume()
                self._paint(stdscr)
                ch = stdscr.getch()
                if ch in (ord('q'), 27): self._running.clear()
                time.sleep(0.02)

        def run(self):
            if curses is None:
                print("Missing console backend: install with: pip install windows-curses")
                sys.exit(1)
            curses.wrapper(self._loop)

        def stop(self): self._running.clear()

    def start_console_mode():
        if curses is None:
            print("Missing console backend: install with: pip install windows-curses")
            sys.exit(1)

        def holder_devices():
            with device_lock: return list(devices)

        ui = ConsoleUI(get_devices_cb=holder_devices)

        import hid
        def _device_monitor():
            global prev_reconnections
            while True:
                dev_infos = list_target_devices()
                current_serials = {d.get('serial_number','') for d in dev_infos}
                with device_lock:
                    stale = [e for e in devices if e.info.get('serial_number','') not in current_serials]
                for e in stale:
                    e.disconnected = True
                    try: e.dev.close()
                    except Exception: pass
                    ui.post(('status', e.name, "DISCONNECTED"))
                    with device_lock:
                        devices[:] = [x for x in devices if x is not e]
                for d in dev_infos:
                    serial = d.get('serial_number','')
                    with device_lock:
                        exists = any(x.info.get('serial_number','') == serial for x in devices)
                    if not exists:
                        dev = hid.device()
                        try: dev.open_path(d['path'])
                        except Exception: continue
                        entry = DeviceEntry(dev, d)
                        entry.reconnections = prev_reconnections.get(serial, 0)
                        prev_reconnections[serial] = entry.reconnections + 1
                        with device_lock:
                            devices.append(entry)
                        threading.Thread(target=device_reader,
                                         args=(entry, ui.uiq, net.udp_send_report),
                                         daemon=True).start()
                        ui.post(('status', entry.name, "WAIT HANDSHAKE"))
                with device_lock:
                    ui.post(('statusbar', None, f"{len(devices)} device(s) connected."))
                time.sleep(1)

        devices, device_lock = [], threading.Lock()
        net = NetworkManager(ui.uiq, reply_addr, lambda: holder_devices())
        threading.Thread(target=_device_monitor, daemon=True).start()
        net.start()




        # console stats updater (mirrors GUI behavior)
        def _stats_updater():
            while True:
                time.sleep(1)
                with global_stats_lock:
                    avg_frame = (stats["bytes_rolling"] / stats["frames_rolling"]) if stats["frames_rolling"] else 0
                    duration  = time.time() - stats["start_time"]
                    hz        = stats["frame_count_window"] / duration if duration > 0 else 0
                    kbps      = (stats["bytes"] / 1024) / duration if duration > 0 else 0
                    ui.post(('globalstats', {
                        'frames': stats["frame_count_total"],
                        'hz':     f"{hz:.1f}",
                        'bw':     f"{kbps:.1f}",
                        'avgudp': f"{avg_frame:.1f}",
                    }))
                    stats["frame_count_window"] = 0
                    stats["bytes"] = 0
                    stats["start_time"] = time.time()

        threading.Thread(target=_stats_updater, daemon=True).start()




        try:
            ui.run()
        finally:
            net.stop()
else:
    # GUI mode never imports curses; provide a harmless stub
    def start_console_mode():
        print("Console mode not enabled. Run with --console or --headless.")

# --- main.py (continued) ---
def main():
    if ('--console' in sys.argv) or ('--headless' in sys.argv):
        start_console_mode()
        lock.release()
        return

    # GUI mode (original)
    root = tk.Tk()
    gui = CockpitGUI(root, None)
    net = NetworkManager(gui.uiq, reply_addr, gui.get_devices)
    gui.network_mgr = net
    net.start()
    root.mainloop()
    net.stop()
    lock.release()

if __name__ == "__main__":
    main()