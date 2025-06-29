import tkinter as tk
from tkinter import ttk, scrolledtext
import queue
import threading
import time
from datetime import datetime
from hid_device import DeviceEntry, list_target_devices, device_reader
from config import stats, global_stats_lock, DEFAULT_REPORT_SIZE, IDLE_TIMEOUT, prev_reconnections

def log_ts():
    return f"[{datetime.now().strftime('%H:%M:%S')}]"

class CockpitGUI:
    def __init__(self, root, network_mgr):
        self.root = root
        root.title("CockpitOS Updater")
        self.uiq = queue.Queue()

        self.network_mgr = network_mgr  # Use the external NetworkManager

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

        self.known_devices = {}  # mapping from devname to DeviceEntry

        self._start_device_thread()
        self._start_stats_thread()
        self.update_ui()

    # --- Device logic ---
    def _start_device_thread(self):
        threading.Thread(target=self._device_monitor, daemon=True).start()

    def _device_monitor(self):
        import hid  # ensure hid is imported here
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
                    # FIX: remove DeviceEntry.create_device(d), use hid.device()
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
            # Compose the log line with timestamp
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