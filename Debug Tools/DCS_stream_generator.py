import socket
import json
import time
import binascii
import os
import argparse

# === CONFIGURATION ===
MULTICAST_IP = "239.255.50.10"
UDP_PORT = 5010
INPUT_JSON_FILE = "dcsbios_data.json"

# === FUNCTION: Detect local IP ===
def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except Exception:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip

# === ARGUMENT PARSING ===
parser = argparse.ArgumentParser(description="DCS-BIOS UDP Replay Tool")
parser.add_argument("--speed", type=float, default=1.0, help="Replay speed multiplier (e.g. 2.0 = 2x faster)")
parser.add_argument("--fps", type=float, help="Override all delays and force fixed FPS (e.g. 60)")
args = parser.parse_args()

os.chdir(os.path.dirname(os.path.abspath(__file__)))

# === SETUP SOCKET ===
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)

# Set multicast TTL
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)

# Set multicast interface dynamically
local_ip = get_local_ip()
# sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_IF, socket.inet_aton(local_ip))

# === LOAD DATA ===
with open(INPUT_JSON_FILE, "r") as f:
    frames = json.load(f)

print(f"[INFO] Loaded {len(frames)} frames from {INPUT_JSON_FILE}")
print(f"[INFO] Multicast to {MULTICAST_IP}:{UDP_PORT} via {local_ip}")
if args.fps:
    print(f"🔁 Fixed replay at {args.fps:.1f} FPS ({1000/args.fps:.2f}ms/frame)")
else:
    print(f"🔁 Using recorded frame timing scaled by x{args.speed}")
print("⏳ Press Ctrl+C to stop.\n")

# === REPLAY LOOP ===
# Precompute absolute time offsets from start of stream
frame_timestamps = []
accum_time = 0.0
for frame in frames:
    frame_timestamps.append(accum_time)
    accum_time += frame.get("timing", 0) / args.speed if not args.fps else (1.0 / args.fps)

# Start absolute timer
stream_start_time = time.perf_counter()
cycle = 0

while True:
    for i, frame in enumerate(frames):
        hex_data = frame.get("data", "")
        if not hex_data:
            continue

        target_time = stream_start_time + frame_timestamps[i] + (cycle * accum_time)
        now = time.perf_counter()
        wait_time = target_time - now
        if wait_time > 0:
            time.sleep(wait_time)

        try:
            binary_data = binascii.unhexlify(hex_data)
            sock.sendto(binary_data, (MULTICAST_IP, UDP_PORT))
        except Exception as e:
            print(f"⚠️ Error sending frame: {e}")

    cycle += 1
    print(f"✅ Completed cycle {cycle}")

sock.close()
