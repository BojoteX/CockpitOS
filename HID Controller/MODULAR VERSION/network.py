import socket
import struct
import threading
import time
import hid
from config import DEFAULT_REPORT_SIZE, DEFAULT_MULTICAST_IP, DEFAULT_UDP_PORT, stats, global_stats_lock

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

                # Set reply_addr for UDP TX
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
