# --- main.py --- (dependencies and lock logic at the top)
import sys, re

REQUIRED_MODULES = {
    "hid":    "hidapi",
    "filelock": "filelock",
    "tkinter": None,  # handled specially
}

missing = []

for mod, pip_name in REQUIRED_MODULES.items():
    if mod == "tkinter":
        try:
            import tkinter
        except ImportError:
            missing.append("tkinter")
    else:
        try:
            __import__(mod)
        except ImportError:
            missing.append(pip_name if pip_name else mod)

if missing:
    pip_cmd = "pip install " + " ".join(m for m in missing if m != "tkinter")
    msg =  "Some required modules are missing:\n\n"
    for mod in missing:
        if mod == "tkinter":
            msg += "- tkinter (usually installable via system package manager)\n"
        else:
            msg += f"- {mod}\n"
    if pip_cmd.strip() != "pip install":
        msg += f"\nTo install the missing Python modules, run:\n\n    {pip_cmd}\n"
    msg += "\nAfter installing, restart this program."
    try:
        import tkinter as tk
        from tkinter import scrolledtext

        root = tk.Tk()
        root.title("Missing Required Modules")
        root.geometry("560x320")
        root.resizable(False, False)
        lbl = tk.Label(root, text="Missing required modules for CockpitController HID Handler:", font=("Arial", 12, "bold"), pady=10)
        lbl.pack()
        text = scrolledtext.ScrolledText(root, width=68, height=10, font=("Consolas", 10))
        text.pack(padx=12, pady=(0,10))
        text.insert("1.0", msg)
        text.config(state='normal')
        text.focus()
        def close():
            root.destroy()
            sys.exit(1)
        btn = tk.Button(root, text="Close", command=close, width=18)
        btn.pack(pady=10)
        root.protocol("WM_DELETE_WINDOW", close)
        root.mainloop()
    except Exception:
        print(msg)
    sys.exit(1)

import os
from filelock import FileLock, Timeout

LOCKFILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cockpitos_dashboard.lock")

try:
    lock = FileLock(LOCKFILE + ".lock")
    lock.acquire(timeout=0.1)
except Timeout:
    print("ERROR: Another instance of CockpitController HID Handler is already running.")
    sys.exit(1)

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
            and not ip_obj.is_loopback  # Remove if you want to allow 127.x.x.x for local test only!
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

def device_reader(entry, uiq, udp_send):
    dev = entry.dev
    try:
        while not entry.handshaked and not entry.disconnected:
            entry.handshaked = try_fifo_handshake(dev, uiq=uiq, device_name=entry.name)
            if not entry.handshaked:
                entry.disconnected = True
                uiq.put(('status', entry.name, "DISCONNECTED"))
                return
            uiq.put(('status', entry.name, "READY"))
            uiq.put(('log', entry.name, "Handshake complete, ready to process input."))
            entry.status = "READY"
        if reply_addr[0] is None and not entry.disconnected:
            uiq.put(('log', entry.name, "Waiting for DCS mission start..."))
        while reply_addr[0] is None and not entry.disconnected:
            time.sleep(0.2)
        if entry.disconnected:
            return
        uiq.put(('log', entry.name, f"DCS detected on {reply_addr[0]} — Starting normal operation."))
        # Clear backlog with attempt limit
        cleared = False
        max_attempts = 100
        attempt = 0
        while not cleared and not entry.disconnected and attempt < max_attempts:
            try:
                resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                if not any(d):
                    cleared = True
            except Exception:
                entry.disconnected = True
                uiq.put(('status', entry.name, "DISCONNECTED"))
                return
            attempt += 1
        if not cleared:
            entry.disconnected = True
            uiq.put(('status', entry.name, "DISCONNECTED (backlog timeout)"))
            return
        while not entry.disconnected and entry.handshaked:
            try:
                data = dev.read(DEFAULT_REPORT_SIZE, timeout_ms=-1)  # Blocking read
                if data:
                    while not entry.disconnected:
                        try:
                            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                            msg = d.rstrip(b'\x00').decode(errors="replace").strip()
                            if not msg or msg == HANDSHAKE_REQ.decode():
                                break
                            uiq.put(('log', entry.name, f"IN: {msg}"))
                            udp_send(msg + "\n")
                        except Exception:
                            entry.disconnected = True
                            uiq.put(('status', entry.name, "DISCONNECTED"))
                            return
            except Exception:
                entry.disconnected = True
                uiq.put(('status', entry.name, "DISCONNECTED"))
                return
    finally:
        try:
            dev.close()
        except Exception:
            pass

# --- network.py ---
import socket
import struct
import threading

class NetworkManager:
    def __init__(self, uiq, reply_addr_ref, get_devices_callback):
        self.uiq = uiq
        self.reply_addr = reply_addr_ref
        self.get_devices = get_devices_callback
        self.udp_rx_sock = None
        self.udp_tx_sock = None
        self._running = threading.Event()

    def start(self):
        self._running.set()
        threading.Thread(target=self._udp_rx_processor, daemon=True).start()
        self._start_udp_tx_sock()

    def stop(self):
        self._running.clear()
        if self.udp_rx_sock:
            self.udp_rx_sock.close()
        if self.udp_tx_sock:
            self.udp_tx_sock.close()

    def _start_udp_tx_sock(self):
        self.udp_tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    def _udp_rx_processor(self):
        self._ip_committed = False  # Only allow one commit per run
        try:
            self.udp_rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.udp_rx_sock.bind(('', DEFAULT_UDP_PORT))
            mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
            self.udp_rx_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            while self._running.is_set():
                data, addr = self.udp_rx_sock.recvfrom(4096)
                # Only do IP learning once, only if valid and different from current, and not already committed
                if (
                    not self._ip_committed
                    and addr
                    and is_valid_ipv4(addr[0])
                    and self.reply_addr[0] != addr[0]
                ):
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
                devices = self.get_devices()
                for entry in devices:
                    if entry.handshaked and not entry.disconnected:
                        offset = 0
                        while offset < len(data):
                            chunk = data[offset:offset + DEFAULT_REPORT_SIZE]
                            report = bytes([0]) + chunk
                            report += b'\x00' * ((DEFAULT_REPORT_SIZE + 1) - len(report))
                            try:
                                entry.dev.write(report)
                            except Exception:
                                entry.disconnected = True
                                self.uiq.put(('status', entry.name, "DISCONNECTED"))
                            offset += DEFAULT_REPORT_SIZE
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
                    threading.Thread(
                        target=device_reader,
                        args=(entry, self.uiq, self.network_mgr.udp_send_report),
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


# --- main.py (continued) ---
def main():
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