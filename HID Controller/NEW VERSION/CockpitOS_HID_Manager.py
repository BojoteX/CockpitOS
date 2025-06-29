# --- main.py --- (dependencies and lock logic at the top)
import sys

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

DEFAULT_REPORT_SIZE = 64
DEFAULT_MULTICAST_IP = "239.255.50.10"
DEFAULT_UDP_PORT = 5010
HANDSHAKE_REQ = b"DCSBIOS-HANDSHAKE"
FEATURE_REPORT_ID = 0
IDLE_TIMEOUT = 2.0
MAX_DEVICES = 20
LOCKFILE = "cockpitos_dashboard.lock"

def read_vid_pid_from_ini(filename="settings.ini"):
    config = configparser.ConfigParser()
    if not os.path.isfile(filename):
        config['USB'] = {'VID': '0xCAFE', 'PID': '0xCAF3'}
        with open(filename, 'w') as configfile:
            config.write(configfile)
    config.read(filename)
    try:
        vid = int(config['USB']['VID'], 0)
        pid = int(config['USB']['PID'], 0)
    except Exception:
        vid, pid = 0xCAFE, 0xCAF3
    return vid, pid

USB_VID, USB_PID = read_vid_pid_from_ini()

stats = {
    "frame_count_total": 0,
    "frame_count_window": 0,
    "bytes": 0,
    "start_time": time.time()
}
global_stats_lock = threading.Lock()

reply_addr = [None]  # As a mutable list!
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
            try:
                dev.close()
            except Exception:
                pass
            return False
        try:
            dev.send_feature_report(bytes([FEATURE_REPORT_ID]) + payload)
        except Exception as e:
            if uiq: uiq.put(('handshake', device_name, f"SET FEATURE exception: {e}"))
            try:
                dev.close()
            except Exception:
                pass
            return False
        try:
            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
            msg = d.rstrip(b'\x00')
            if msg == HANDSHAKE_REQ:
                return True
        except Exception as e:
            if uiq: uiq.put(('handshake', device_name, f"GET FEATURE exception: {e}"))
            try:
                dev.close()
            except Exception:
                pass
            return False
        attempts += 1
        if attempts % 10 == 0 and uiq:
            uiq.put(('handshake', device_name, "Waiting for handshake..."))
        time.sleep(0.2)

class DeviceEntry:
    def __init__(self, dev, dev_info):
        self.dev = dev
        self.info = dev_info
        self.name = (dev_info.get('product_string','') or '') + " [" + (dev_info.get('serial_number','') or '') + "]"
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
    global reply_addr
    try:
        while not entry.handshaked and not entry.disconnected:
            entry.handshaked = try_fifo_handshake(dev, uiq=uiq, device_name=entry.name)
            if not entry.handshaked:
                entry.disconnected = True
                try:
                    dev.close()
                except Exception:
                    pass
                uiq.put(('status', entry.name, "DISCONNECTED"))
                return
            uiq.put(('status', entry.name, "READY"))
            uiq.put(('log', entry.name, "Handshake complete, ready to process input."))
            entry.status = "READY"
        if reply_addr is None and not entry.disconnected:
            uiq.put(('log', entry.name, "Waiting for DCS mission start..."))
        while reply_addr[0] is None and not entry.disconnected:
            time.sleep(0.2)
        if entry.disconnected:
            try:
                dev.close()
            except Exception:
                pass
            return
        uiq.put(('log', entry.name, f"DCS detected on {reply_addr[0]} — Starting normal operation."))
        try:
            for _ in range(10):
                try:
                    resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                    d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                    if not any(d):
                        break
                except Exception as e:
                    entry.disconnected = True
                    try:
                        dev.close()
                    except Exception:
                        pass
                    uiq.put(('status', entry.name, "DISCONNECTED"))
                    return
        except Exception:
            entry.disconnected = True
            try:
                dev.close()
            except Exception:
                pass
            uiq.put(('status', entry.name, "DISCONNECTED"))
            return
        while not entry.disconnected and entry.handshaked:
            try:
                data = dev.read(DEFAULT_REPORT_SIZE, timeout_ms=0)
                if data:
                    while True:
                        try:
                            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                            msg = d.rstrip(b'\x00').decode(errors="replace").strip()
                            if not msg or msg == HANDSHAKE_REQ.decode():
                                break
                            uiq.put(('log', entry.name, f"IN: {msg}"))
                            udp_send(msg + "\r\n")
                        except Exception as e:
                            entry.disconnected = True
                            try:
                                dev.close()
                            except Exception:
                                pass
                            uiq.put(('status', entry.name, "DISCONNECTED"))
                            return
            except Exception as e:
                entry.disconnected = True
                try:
                    dev.close()
                except Exception:
                    pass
                uiq.put(('status', entry.name, "DISCONNECTED"))
                return
    except Exception as e:
        entry.disconnected = True
        try:
            dev.close()
        except Exception:
            pass
        uiq.put(('status', entry.name, "DISCONNECTED"))
        return

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
        self._running = False

    def start(self):
        self._running = True
        self._start_udp_rx_thread()
        self._start_udp_tx_sock()

    def stop(self):
        self._running = False

    def _start_udp_rx_thread(self):
        threading.Thread(target=self._udp_rx_processor, daemon=True).start()

    def _udp_rx_processor(self):
        print("[NetworkManager] UDP RX thread starting...")
        self.udp_rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.udp_rx_sock.bind(('', DEFAULT_UDP_PORT))
        mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
        self.udp_rx_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        while self._running:
            try:
                data, addr = self.udp_rx_sock.recvfrom(4096)
                if addr and addr[0] != '0.0.0.0':
                    if self.reply_addr[0] != addr[0]:
                        self.reply_addr[0] = addr[0]
                        self.uiq.put(('data_source', None, addr[0]))
                with global_stats_lock:
                    stats["frame_count_total"] += 1
                    stats["frame_count_window"] += 1
                    stats["bytes"] += len(data)
                for entry in self.get_devices():
                    if entry.handshaked and not entry.disconnected:
                        offset = 0
                        while offset < len(data):
                            chunk = data[offset:offset + DEFAULT_REPORT_SIZE]
                            report = bytes([0]) + chunk
                            report += bytes((DEFAULT_REPORT_SIZE + 1) - len(report))
                            entry.dev.write(report)
                            offset += DEFAULT_REPORT_SIZE
            except Exception as e:
                if "WinError 10054" not in str(e):
                    self.uiq.put(('log', "UDP", f"UDP RX processor error: {e}"))
                time.sleep(1)

    def _start_udp_tx_sock(self):
        self.udp_tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    def udp_send_report(self, msg, port=7778):
        if self.udp_tx_sock is not None and self.reply_addr[0]:
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
        }

        ttk.Label(self.stats_frame, text="Frames:").grid(row=0, column=0, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['frames']).grid(row=0, column=1, padx=5)
        ttk.Label(self.stats_frame, text="Hz:").grid(row=0, column=2, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['hz']).grid(row=0, column=3, padx=5)
        ttk.Label(self.stats_frame, text="kB/s:").grid(row=0, column=4, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['bw']).grid(row=0, column=5, padx=5)

        self.data_source_var = tk.StringVar(value="Data Source: (waiting...)")
        self.data_source_label = ttk.Label(self.stats_frame, textvariable=self.data_source_var, foreground="blue")
        self.data_source_label.grid(row=0, column=6, padx=12)

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

        self._start_device_thread()
        self._start_stats_thread()
        self.update_ui()

    def _start_device_thread(self):
        threading.Thread(target=self._device_monitor, daemon=True).start()

    def _device_monitor(self):
        import hid
        while True:
            dev_infos = list_target_devices()
            current_serials = set(d.get('serial_number', '') for d in dev_infos)
            to_remove = set(self.known_devices.keys()) - current_serials
            for serial in to_remove:
                devname = self.known_devices[serial].name
                self.uiq.put(('status', devname, "REMOVED"))
                del self.known_devices[serial]
                if devname in self.device_nodes:
                    self.tree.delete(self.device_nodes[devname])
                    del self.device_nodes[devname]
            new_devices = []
            for d in dev_infos:
                serial = d.get('serial_number', '')
                devname = (d.get('product_string','') or '') + " [" + (serial or '') + "]"
                if serial not in self.known_devices:
                    dev = hid.device()
                    try:
                        dev.open_path(d['path'])
                    except Exception:
                        continue
                    entry = DeviceEntry(dev, d)
                    entry.reconnections = prev_reconnections.get(serial, 0)
                    if serial in prev_reconnections:
                        entry.reconnections += 1
                    prev_reconnections[serial] = entry.reconnections
                    entry.name = serial
                    self.known_devices[serial] = entry
                    new_devices.append(entry)
                    threading.Thread(target=self.device_reader_wrapper, args=(entry,), daemon=True).start()
                    self.uiq.put(('status', entry.name, "WAIT HANDSHAKE"))
                else:
                    entry = self.known_devices[serial]
                    new_devices.append(entry)
            self.devices = new_devices
            dev_count = len(new_devices)
            if dev_count:
                self.uiq.put(('statusbar', None, f"{dev_count} device(s) connected."))
            else:
                self.uiq.put(('statusbar', None, "No CockpitOS devices found."))
                time.sleep(2)
                continue
            while True:
                current_dev_infos = list_target_devices()
                current_serials_now = set(d.get('serial_number', '') for d in current_dev_infos)
                if current_serials_now != set(self.known_devices.keys()):
                    break
                time.sleep(1)

    def device_reader_wrapper(self, entry):
        def udp_send(msg):
            self.network_mgr.udp_send_report(msg)
        device_reader(entry, self.uiq, udp_send)

    def _start_stats_thread(self):
        threading.Thread(target=self._stats_updater, daemon=True).start()

    def _stats_updater(self):
        while True:
            time.sleep(1)
            with global_stats_lock:
                duration = time.time() - stats["start_time"]
                hz = stats["frame_count_window"] / duration if duration > 0 else 0
                kbps = (stats["bytes"] / 1024) / duration if duration > 0 else 0
                stat_dict = {
                    'frames': stats["frame_count_total"],
                    'hz': f"{hz:.1f}",
                    'bw': f"{kbps:.1f}"
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
        if len(evt) == 3:
            typ, devname, data = evt
        elif len(evt) == 2:
            typ, data = evt
            devname = None
        else:
            print(f"[ERROR] Malformed event: {evt}")
            return

        if typ == 'data_source':
            self.data_source_var.set(f"Data Source: {data}")

        elif typ == 'statusbar':
            self.statusbar.config(text=data)

        elif typ == 'status':
            entry = None
            for dev in getattr(self, 'devices', []):
                if dev.name == devname:
                    entry = dev
                    break
            reconns = entry.reconnections if entry else 0
            tag = ()
            status_lower = str(data).lower()
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
                vals = list(self.tree.item(idx, 'values'))
                vals[0] = devname
                vals[1] = data
                vals[2] = reconns
                self.tree.item(idx, values=vals, tags=tag)

        elif typ == 'log' or typ == 'handshake':
            self.log_text['state'] = 'normal'
            ts = log_ts()
            line = f"{ts} [{devname}] {data}\n"
            self.log_text.insert('end', line)
            self.log_text.see('end')
            self.log_text['state'] = 'disabled'

        elif typ == 'globalstats':
            stats = data
            for k in self.stats_vars:
                self.stats_vars[k].set(stats.get(k, "0"))

    def get_devices(self):
        return getattr(self, 'devices', [])

# --- main.py (continued) ---
def main():
    root = tk.Tk()
    gui = CockpitGUI(root, None)
    net = NetworkManager(gui.uiq, reply_addr, gui.get_devices)
    gui.network_mgr = net
    net.start()
    root.mainloop()

if __name__ == "__main__":
    main()
