import socket
import os
from datetime import datetime

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Discover our local IP address
def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        # Connect to a non-routable IP, no traffic actually sent
        s.connect(("8.8.8.8", 80))
        return s.getsockname()[0]
    except:
        return "127.0.0.1"

LOCAL_IP = get_local_ip()
PORT = 7778

# Setup socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', PORT))
# sock.bind((LOCAL_IP, PORT))

print(f"⚠️ *** DO NOT run this while DCS is also running as it will prevent commands from reaching DCS ***")
print(f"🔭 Listening for DCS Commands on {LOCAL_IP} port {PORT}...\n")

while True:
    try:
        data, addr = sock.recvfrom(1024)
        sender_ip, sender_port = addr
        msg = data.decode('utf-8', errors='ignore').strip()
        timestamp = datetime.now().strftime('%H:%M:%S')

        print(f"[{timestamp} - {sender_ip}] {msg}")
    except Exception as e:
        print(f"[ERROR] {e}")
