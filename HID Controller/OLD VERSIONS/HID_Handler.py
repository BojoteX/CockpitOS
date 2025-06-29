import sys
import socket
import struct
import time
import threading
import configparser
import os
import queue

try:
    import tkinter as tk
    from tkinter import ttk, scrolledtext
except ImportError:
    print("\n[ERROR] Missing required module: 'tkinter'")
    print("To install on Ubuntu/Debian: sudo apt install python3-tk\n")
    sys.exit(1)

REQUIRED_MODULES = [
    ('psutil', None),
    ('hid', 'hidapi'),
]

for mod, pip_hint in REQUIRED_MODULES:
    try:
        globals()[mod] = __import__(mod)
    except ImportError:
        pip_hint = pip_hint if pip_hint else mod
        print(f"\n[ERROR] Missing required module: '{mod}'")
        print(f"To install, run: pip install {pip_hint}\n")
        sys.exit(1)

os.chdir(os.path.dirname(os.path.abspath(__file__)))

DEFAULT_REPORT_SIZE = 64
DEFAULT_MULTICAST_IP = "239.255.50.10"
DEFAULT_UDP_PORT = 5010
HANDSHAKE_REQ = b"DCSBIOS-HANDSHAKE"
FEATURE_REPORT_ID = 0
IDLE_TIMEOUT = 2.0
MAX_DEVICES = 20
LOCKFILE = "cockpitos_dashboard.lock"

cleanup_event = threading.Event()
prev_reconnections = {}

def read_vid_pid_from_ini(filename="settings.ini"):
    config = configparser.ConfigParser()
    # If the file doesn't exist, create it with defaults
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

# Read VID & PID from settings.ini or set defaults if not present
USB_VID, USB_PID = read_vid_pid_from_ini()

# Global stats for the stream
global_frame_count_total = 0
global_frame_count_window = 0
global_bytes = 0
global_start_time = time.time()
global_stats_lock = threading.Lock()

# UDP global state (must be accessed with care)
reply_addr = None

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
            dev.send_feature_report(bytes([FEATURE_REPORT_ID]) + payload)
        except Exception as e:
            if uiq: uiq.put(('handshake', device_name, f"SET FEATURE exception: {e}"))
        try:
            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
            msg = d.rstrip(b'\x00')
            if msg == HANDSHAKE_REQ:
                return True
        except Exception as e:
            if uiq: uiq.put(('handshake', device_name, f"GET FEATURE exception: {e}"))
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

class CockpitGUI:
    def __init__(self, root):
        self.root = root
        root.title("CockpitOS Updater")
        self.uiq = queue.Queue()

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

        # --- FIXED COLUMNS: name, status, reconnections
        self.tree = ttk.Treeview(self.devices_frame, columns=('name','status','reconn'), show='headings')

        self.tree.column('name', width=220, anchor='w')
        self.tree.column('status', width=90, anchor='center')
        self.tree.column('reconn', width=120, anchor='center')

        # Add tag for 'READY' status
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

        self.udp_rx_sock = None
        self.udp_tx_sock = None

        self.known_devices = {}  # mapping from devname to DeviceEntry

        self._start_device_thread()
        self._start_udp_rx_thread()
        self._start_udp_tx_sock()
        self._start_stats_thread()
        self.update_ui()

    # --- Device logic ---
    def _start_device_thread(self):
        threading.Thread(target=self._device_monitor, daemon=True).start()

    # --- UDP RX (multicast) ---
    def _start_udp_rx_thread(self):
        threading.Thread(target=self._udp_rx_processor, daemon=True).start()

    def _udp_rx_processor(self):
        global reply_addr, global_frame_count_total, global_frame_count_window, global_bytes
        self.udp_rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.udp_rx_sock.bind(('', DEFAULT_UDP_PORT))
        mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
        self.udp_rx_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
        while True:
            try:
                data, addr = self.udp_rx_sock.recvfrom(4096)
                # Set reply_addr for UDP TX

                # if addr and addr[0] != '0.0.0.0':
                    # reply_addr = addr[0]

                if addr and addr[0] != '0.0.0.0':
                    if reply_addr != addr[0]:
                        reply_addr = addr[0]
                        self.uiq.put(('data_source', None, reply_addr))

                with global_stats_lock:
                    global_frame_count_total += 1
                    global_frame_count_window += 1
                    global_bytes += len(data)
                # Forward to all ready devices
                for entry in getattr(self, 'devices', []):
                    if entry.handshaked and not entry.disconnected:
                        offset = 0
                        while offset < len(data):
                            chunk = data[offset:offset + DEFAULT_REPORT_SIZE]
                            report = bytearray([0]) + chunk
                            report += bytes((DEFAULT_REPORT_SIZE + 1) - len(report))
                            try:
                                entry.dev.write(report)
                            except Exception as e:
                                self.uiq.put(('log', entry.name, f"Error sending UDP->HID: {e}"))
                            offset += DEFAULT_REPORT_SIZE
            except Exception as e:
                if "WinError 10054" not in str(e):
                    self.uiq.put(('log', "UDP", f"UDP RX processor error: {e}"))
                time.sleep(1)

    # --- UDP TX (unicast) ---
    def _start_udp_tx_sock(self):
        self.udp_tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        # No bind needed for sending

    def udp_send_report(self, msg, port=7778):
        global reply_addr
        if self.udp_tx_sock is not None and reply_addr:
            try:
                self.udp_tx_sock.sendto(msg.encode(), (reply_addr, port))
            except Exception as e:
                self.uiq.put(('log', "UDP", f"[UDP SEND ERROR] {e}"))
        elif not reply_addr:
            self.uiq.put(('log', "UDP", "UDP TX: reply_addr not set, cannot send."))

    # --- Device monitor logic ---
    def _device_monitor(self):
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
            self.udp_send_report(msg)
        device_reader(entry, self.uiq, udp_send)

    # --- Global stats updater thread ---
    def _start_stats_thread(self):
        threading.Thread(target=self._stats_updater, daemon=True).start()

    def _stats_updater(self):
        global global_frame_count_total, global_frame_count_window, global_bytes, global_start_time
        while True:
            time.sleep(1)
            with global_stats_lock:
                duration = time.time() - global_start_time
                hz = global_frame_count_window / duration if duration > 0 else 0
                kbps = (global_bytes / 1024) / duration if duration > 0 else 0
                stats = {
                    'frames': global_frame_count_total,
                    'hz': f"{hz:.1f}",
                    'bw': f"{kbps:.1f}"
                }
                self.uiq.put(('globalstats', stats))
                global_frame_count_window = 0
                global_bytes = 0
                global_start_time = time.time()

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
            # Find matching entry for reconnections
            entry = None
            for dev in getattr(self, 'devices', []):
                if dev.name == devname:
                    entry = dev
                    break
            reconns = entry.reconnections if entry else 0

            # Assign a tag based on status for coloring
            tag = ()
            status_lower = str(data).lower()
            if 'ready' in status_lower:
                tag = ('ready',)
            elif 'wait' in status_lower or 'handshake' in status_lower:
                tag = ('wait',)
            elif 'off' in status_lower or 'disconn' in status_lower:
                tag = ('off',)

            # Insert or update row in the devices Treeview
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
            self.log_text.insert('end', f"[{devname}] {data}\n")
            self.log_text.see('end')
            self.log_text['state'] = 'disabled'

        elif typ == 'globalstats':
            stats = data
            for k in self.stats_vars:
                self.stats_vars[k].set(stats.get(k, "0"))

def device_reader(entry, uiq, udp_send):
    dev = entry.dev
    global reply_addr  # Add this line so Python knows you mean the global variable
    try:
        # Wait for handshake as before
        while not entry.handshaked and not entry.disconnected:
            entry.handshaked = try_fifo_handshake(dev, uiq=uiq, device_name=entry.name)
            if entry.handshaked:
                uiq.put(('status', entry.name, "READY"))
                uiq.put(('log', entry.name, "Handshake complete, ready to process input."))
                entry.status = "READY"

        # === NEW: Wait for reply_addr to be set, log only once ===
        if reply_addr is None and not entry.disconnected:
            uiq.put(('log', entry.name, "Waiting for DCS mission start..."))
        while reply_addr is None and not entry.disconnected:
            time.sleep(0.2)
        if not entry.disconnected:
            uiq.put(('log', entry.name, f"DCS detected on {reply_addr} — Starting normal operation."))

        # Optional: Drain device OUT buffer before normal loop
        for _ in range(10):
            resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
            d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
            if not any(d):
                break
            # Optionally log buffer drain: uiq.put(('log', entry.name, f"Draining buffer: {d!r}"))

        # Now enter normal HID polling and UDP sending
        while not entry.disconnected and entry.handshaked:
            try:
                data = dev.read(DEFAULT_REPORT_SIZE, timeout_ms=0)
                if data:
                    resp = dev.get_feature_report(FEATURE_REPORT_ID, DEFAULT_REPORT_SIZE + 1)
                    d = bytes(resp[1:]) if len(resp) > DEFAULT_REPORT_SIZE else bytes(resp)
                    msg = d.rstrip(b'\x00').decode(errors="replace").strip()
                    if msg and msg != HANDSHAKE_REQ.decode():
                        uiq.put(('log', entry.name, f"IN: {msg}"))
                        udp_send(msg + "\r\n")
            except Exception:
                entry.disconnected = True
                uiq.put(('status', entry.name, "DISCONNECTED"))
                break
    except (OSError, IOError):
        entry.disconnected = True
        uiq.put(('status', entry.name, "DISCONNECTED"))

def main():
    root = tk.Tk()
    app = CockpitGUI(root)
    root.mainloop()

if __name__ == "__main__":
    main()
