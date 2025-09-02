# --- bt_hid_manager.py ---
# Exact functional replica for BLE HID (HID-over-GATT). USB code removed.

# ===== dependencies and lock logic =====
import sys, re, os

REQUIRED_MODULES = {
    "bleak":  "bleak",
    "filelock": "filelock",
    "tkinter": None,
}

missing = []
for mod, pip_name in REQUIRED_MODULES.items():
    if mod == "tkinter":
        try:
            import tkinter  # noqa
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
        root.geometry("560x320"); root.resizable(False, False)
        lbl = tk.Label(root, text="Missing required modules for CockpitController HID Handler:", font=("Arial", 12, "bold"), pady=10); lbl.pack()
        text = scrolledtext.ScrolledText(root, width=68, height=10, font=("Consolas", 10)); text.pack(padx=12, pady=(0,10))
        text.insert("1.0", msg); text.config(state='normal'); text.focus()
        def close(): root.destroy(); sys.exit(1)
        btn = tk.Button(root, text="Close", command=close, width=18); btn.pack(pady=10)
        root.protocol("WM_DELETE_WINDOW", close); root.mainloop()
    except Exception:
        print(msg)
    sys.exit(1)

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

# ===== config.py =====
import configparser
import time
import threading
import ipaddress

DEFAULT_REPORT_SIZE = 64
DEFAULT_MULTICAST_IP = "239.255.50.10"
DEFAULT_UDP_PORT = 5010
HANDSHAKE_REQ = b"DCSBIOS-HANDSHAKE"
FEATURE_REPORT_ID = 0  # preserved constant; unused in BLE path but kept for parity
IDLE_TIMEOUT = 2.0
MAX_DEVICES = 20
LOCKFILE = "cockpitos_dashboard.lock"

def is_bt_serial(s: str) -> bool:
    return bool(re.fullmatch(r'(?i)(?:[0-9a-f]{2}[:-]){5}[0-9a-f]{2}|[0-9a-f]{12}|bx-[0-9a-f]{16}', s))

def read_settings_from_ini(filename="settings.ini"):
    config = configparser.ConfigParser()
    if not os.path.isfile(filename):
        config['USB'] = {'VID': '0xCAFE', 'PID': '0xCAF3'}  # retained for UI parity
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
            and not ip_obj.is_loopback
        )
    except Exception:
        return False

USB_VID, USB_PID, STORED_DCS_IP = read_settings_from_ini()  # kept for UI
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

# ===== BT HID (BLE GATT) =====
import asyncio
from bleak import BleakScanner, BleakClient
from bleak.backends.device import BLEDevice
import sys
WIN = sys.platform.startswith("win")
if WIN:
    try:
        from winrt.windows.devices.bluetooth import BluetoothLEDevice
        from winrt.windows.devices.enumeration import DeviceInformation
    except Exception:
        WIN = False  # fall back to scanning only if winrt not available

HID_SVC      = "00001812-0000-1000-8000-00805f9b34fb"
REPORT_CHAR  = "00002a4d-0000-1000-8000-00805f9b34fb"
REPORT_REF   = "00002908-0000-1000-8000-00805f9b34fb"
BT_NAME_MATCH = "cockpitos"  # case-insensitive substring match

async def _scan_once(timeout=2.0):
    devs = await BleakScanner.discover(timeout=timeout)
    out = []
    for d in devs:
        name = (d.name or "")
        if name and BT_NAME_MATCH in name.lower():
            out.append({'address': d.address, 'serial_number': d.address, 'product_string': name})
    return out

async def _winrt_paired():
    # enumerate paired BLE devices even if not advertising
    out = []
    selector = BluetoothLEDevice.get_device_selector_from_pairing_state(True)
    infos = await DeviceInformation.find_all_async_aqs_filter(selector)
    for info in infos:
        name = (info.name or "")
        if name and BT_NAME_MATCH in name.lower():
            # Bleak on Windows accepts the DeviceInformation.Id string as address
            out.append({'address': str(info.id), 'serial_number': str(info.id), 'product_string': name})
    return out

def list_target_devices():
    # try advertising scan first
    try:
        found = asyncio.run(_scan_once())
    except RuntimeError:
        loop = asyncio.new_event_loop()
        try:
            found = loop.run_until_complete(_scan_once())
        finally:
            loop.close()

    # if none and on Windows with winrt, enumerate paired devices
    if not found and WIN:
        try:
            found = asyncio.run(_winrt_paired())
        except RuntimeError:
            loop = asyncio.new_event_loop()
            try:
                found = loop.run_until_complete(_winrt_paired())
            finally:
                loop.close()
    return found


class DeviceEntry:
    def __init__(self, dev_info):
        self.info = dev_info
        serial  = (dev_info.get('serial_number')  or '')
        product = (dev_info.get('product_string') or '')
        self.name = (product or serial) if is_bt_serial(serial) else (serial or product)
        self.status = "WAIT HANDSHAKE"
        self.last_sent = time.time()
        self.disconnected = False
        self.handshaked = False
        self.reconnections = 0

        # BLE session fields
        self.client: BleakClient | None = None
        self.loop: asyncio.AbstractEventLoop | None = None
        self.inp_char = None
        self.out_char = None
        self.feat_char = None
        self.tx_queue: asyncio.Queue | None = None

    def check(self):
        now = time.time()
        if self.disconnected:
            self.status = "OFF"
        elif (now - self.last_sent) > IDLE_TIMEOUT:
            self.status = "IDLE"
        else:
            self.status = "RECV"

def _enqueue_output(entry: DeviceEntry, chunk: bytes):
    if entry.loop and entry.tx_queue:
        if len(chunk) < DEFAULT_REPORT_SIZE:
            chunk = chunk + b'\x00' * (DEFAULT_REPORT_SIZE - len(chunk))
        asyncio.run_coroutine_threadsafe(entry.tx_queue.put(chunk[:DEFAULT_REPORT_SIZE]), entry.loop)

async def _discover_reports(client: BleakClient):
    await client.get_services()
    inp = out = feat = None
    for s in client.services:
        if str(s.uuid).lower() != HID_SVC:
            continue
        for ch in s.characteristics:
            if str(ch.uuid).lower() != REPORT_CHAR:
                continue
            rr_desc = next((d for d in ch.descriptors if str(d.uuid).lower() == REPORT_REF), None)
            if not rr_desc:
                continue
            rr = await client.read_gatt_descriptor(rr_desc.handle)
            if len(rr) < 2:
                continue
            rid, rtype = rr[0], rr[1]
            if rid != 0x00:
                continue  # no Report IDs in your design
            if rtype == 0x01:
                inp = ch
            elif rtype == 0x02:
                out = ch
            elif rtype == 0x03:
                feat = ch
    return inp, out, feat

async def _device_session(entry: DeviceEntry, uiq, udp_send):




    

    addr = entry.info['address']
    name = entry.info.get('product_string') or ""

    if WIN and addr.startswith("BluetoothLE#"):
        # Build a BLEDevice using WinRT so Bleak can connect even if not advertising
        ble_win = await BluetoothLEDevice.from_id_async(addr)
        ble_dev = BLEDevice(address=addr, name=name, details=ble_win, rssi=0)
        entry.client = BleakClient(ble_dev, timeout=10.0)
    else:
        entry.client = BleakClient(addr, timeout=10.0)

    try:
        await entry.client.connect()








        try:
            await entry.client.pair(protection_level=2)
        except Exception:
            pass

        inp, out, feat = await _discover_reports(entry.client)
        if not (inp and out and feat):
            raise RuntimeError("HID report chars not found")

        # Handshake loop (Feature FIFO echo semantics)
        payload = HANDSHAKE_REQ.ljust(DEFAULT_REPORT_SIZE, b'\x00')
        attempts = 0
        while True:
            try:
                d = await entry.client.read_gatt_char(feat)
                if d.rstrip(b"\x00") == HANDSHAKE_REQ:
                    entry.handshaked = True
                    break
            except Exception as e:
                uiq.put(('handshake', entry.name, f"GET FEATURE exception: {e}"))
                await asyncio.sleep(0.2)
                continue
            try:
                await entry.client.write_gatt_char(feat, payload, response=True)
            except Exception as e:
                uiq.put(('handshake', entry.name, f"SET FEATURE exception: {e}"))
                await asyncio.sleep(0.2)
                continue
            try:
                d = await entry.client.read_gatt_char(feat)
                if d.rstrip(b"\x00") == HANDSHAKE_REQ:
                    entry.handshaked = True
                    break
            except Exception as e:
                uiq.put(('handshake', entry.name, f"GET FEATURE exception: {e}"))
                await asyncio.sleep(0.2)
                continue
            attempts += 1
            if attempts % 10 == 0:
                uiq.put(('handshake', entry.name, "Waiting for handshake..."))
            await asyncio.sleep(0.2)

        uiq.put(('status', entry.name, "READY"))
        uiq.put(('log', entry.name, "Handshake complete, ready to process input."))
        entry.status = "READY"

        # Backlog clear
        cleared = False
        for _ in range(100):
            try:
                d = await entry.client.read_gatt_char(feat)
                if not any(d):
                    cleared = True
                    break
            except Exception:
                entry.disconnected = True
                uiq.put(('status', entry.name, "DISCONNECTED")); return
        if not cleared:
            entry.disconnected = True
            uiq.put(('status', entry.name, "DISCONNECTED (backlog timeout)")); return

        entry.inp_char, entry.out_char, entry.feat_char = inp, out, feat
        entry.loop = asyncio.get_running_loop()
        entry.tx_queue = asyncio.Queue()
        input_queue: asyncio.Queue[bytes] = asyncio.Queue()

        def _on_input(_, data: bytearray):
            entry.last_sent = time.time()
            # push raw 64B input; identical to USB read trigger
            entry.loop.call_soon_threadsafe(input_queue.put_nowait, bytes(data))

        await entry.client.start_notify(inp, _on_input)

        while not entry.disconnected and entry.handshaked:
            data = await input_queue.get()  # blocking like dev.read()
            if data:
                # drain Feature FIFO into UDP
                while not entry.disconnected:
                    try:
                        d = await entry.client.read_gatt_char(feat)
                        msg = d.rstrip(b"\x00").decode(errors="replace").strip()
                        if not msg or msg == HANDSHAKE_REQ.decode():
                            break
                        uiq.put(('log', entry.name, f"IN: {msg}"))
                        udp_send(msg + "\n")
                    except Exception:
                        entry.disconnected = True
                        uiq.put(('status', entry.name, "DISCONNECTED")); return
            # flush any pending host->device output chunks
            try:
                while True:
                    chunk = entry.tx_queue.get_nowait()
                    await entry.client.write_gatt_char(out, chunk, response=False)
            except asyncio.QueueEmpty:
                pass
            await asyncio.sleep(0)

    finally:
        try:
            if entry.client and entry.client.is_connected:
                await entry.client.disconnect()
        except Exception:
            pass

def device_reader(entry: DeviceEntry, uiq, udp_send):
    try:
        asyncio.run(_device_session(entry, uiq, udp_send))
    finally:
        entry.disconnected = True

# ===== network.py =====
import socket, struct

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
        if self.udp_rx_sock: self.udp_rx_sock.close()
        if self.udp_tx_sock: self.udp_tx_sock.close()

    def _start_udp_tx_sock(self):
        self.udp_tx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

    def _udp_rx_processor(self):
        self._ip_committed = False
        try:
            self.udp_rx_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
            self.udp_rx_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self.udp_rx_sock.bind(('', DEFAULT_UDP_PORT))
            mreq = struct.pack("=4sl", socket.inet_aton(DEFAULT_MULTICAST_IP), socket.INADDR_ANY)
            self.udp_rx_sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)
            while self._running.is_set():
                data, addr = self.udp_rx_sock.recvfrom(4096)
                if (not self._ip_committed and addr and is_valid_ipv4(addr[0]) and self.reply_addr[0] != addr[0]):
                    self.reply_addr[0] = addr[0]; write_settings_dcs_ip(addr[0])
                    self._ip_committed = True; self.uiq.put(('data_source', None, addr[0]))
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
                            # BLE: write 64 bytes to Output report (no Report ID)
                            try:
                                _enqueue_output(entry, chunk)
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

# ===== gui.py =====
from tkinter import ttk, scrolledtext
import queue
import threading
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

        ttk.Label(root, text="Event Log:").pack(anchor='w', padx=10)
        from tkinter import scrolledtext
        self.log_text = scrolledtext.ScrolledText(root, width=90, height=15, state='disabled')
        self.log_text.pack(fill='both', expand=True, padx=10, pady=(0,10))


        ttk.Label(self.stats_frame, text="Frames:").grid(row=0, column=0, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['frames']).grid(row=0, column=1, padx=5)
        ttk.Label(self.stats_frame, text="Hz:").grid(row=0, column=2, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['hz']).grid(row=0, column=3, padx=5)
        ttk.Label(self.stats_frame, text="kB/s:").grid(row=0, column=4, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['bw']).grid(row=0, column=5, padx=5)
        ttk.Label(self.stats_frame, text="Avg UDP Frame size (Bytes):").grid(row=0, column=6, padx=5)
        ttk.Label(self.stats_frame, textvariable=self.stats_vars['avgudp']).grid(row=0, column=7, padx=5)
        self.data_source_label = ttk.Label(self.stats_frame, textvariable=self.data_source_var, foreground="blue")
        self.data_source_label.grid(row=0, column=8, padx=12)

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
        self.known_devices = {}
        self.devices = []

        self.statusbar = ttk.Label(root, text="Initializing...", anchor='w')
        self.statusbar.pack(fill='x', side='bottom')

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
                # reconnection count per your logic
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
                    entry = DeviceEntry(d)
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
                time.sleep(2); continue
            time.sleep(1)

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
            s = data.lower()
            if 'ready' in s: tag = ('ready',)
            elif 'wait' in s or 'handshake' in s: tag = ('wait',)
            elif 'off' in s or 'disconn' in s: tag = ('off',)
            if devname not in self.device_nodes:
                idx = self.tree.insert('', 'end', values=(devname, data, reconns), tags=tag)
                self.device_nodes[devname] = idx
            else:
                idx = self.device_nodes[devname]
                if self.tree.exists(idx):
                    vals = list(self.tree.item(idx)['values'])
                    vals[1] = data; vals[2] = reconns
                    self.tree.item(idx, values=vals, tags=tag)
                else:
                    del self.device_nodes[devname]
        elif typ in ('log', 'handshake'):
            devname, data = rest
            self.log_text['state'] = 'normal'
            line = f"{log_ts()} [{devname}] {data}\n"
            self.log_text.insert('end', line); self.log_text.see('end')
            self.log_text['state'] = 'disabled'
        elif typ == 'globalstats':
            for k, v in rest[0].items(): self.stats_vars[k].set(v)
        elif typ == 'remove_device':
            devname = rest[0]
            if devname in self.device_nodes:
                idx = self.device_nodes[devname]
                if self.tree.exists(idx): self.tree.delete(idx)
                del self.device_nodes[devname]

    def get_devices(self):
        with self.devices_lock:
            return list(self.devices)

# ===== main =====
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
