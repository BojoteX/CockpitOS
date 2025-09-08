import socket
import os
import argparse
from datetime import datetime
import shutil

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

LOG_FILE = "logs/udpLogger.log"
PREV_LOG = "logs/prevLog.log"

# Rotate logs: Move last log to prevLog if it exists
if os.path.exists(LOG_FILE):
    if os.path.exists(PREV_LOG):
        os.remove(PREV_LOG)
    shutil.copy2(LOG_FILE, PREV_LOG)
    with open(LOG_FILE, 'w'): pass  # Clear current log

# Discover our local IP address
def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return "127.0.0.1"

# Argument parsing
parser = argparse.ArgumentParser(description="UDP Logger with optional IP filter")
parser.add_argument("--ip", help="Only log messages from this IP address", default=None)
args = parser.parse_args()

LOCAL_IP = get_local_ip()
PORT = 4210

# Setup socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(('0.0.0.0', PORT))

def log_write(line):
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(line + "\n")

header1 = f"🔭 Listening for ESP32S2 Debug/Info messages on {LOCAL_IP} port {PORT}..."
header2 = f"🎯 Filtering to only show messages from IP: {args.ip}" if args.ip else ""
print(header1)
if header2: print(header2)
print()
log_write(header1)
if header2: log_write(header2)
log_write("")

while True:
    try:
        data, addr = sock.recvfrom(1024)
        sender_ip, sender_port = addr
        if args.ip and sender_ip != args.ip:
            continue  # Skip unmatched IPs

        msg = data.decode('utf-8', errors='ignore').strip()
        timestamp = datetime.now().strftime('%H:%M:%S')
        line = f"[{timestamp} - {sender_ip}] {msg}"
        print(line)
        log_write(line)

    except Exception as e:
        errline = f"[ERROR] {e}"
        print(errline)
        log_write(errline)
