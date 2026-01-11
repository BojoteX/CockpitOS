import socket
import time
import json
import os 

os.chdir(os.path.dirname(os.path.abspath(__file__)))

# === CONFIGURATION ===
IP_ADDRESS = "0.0.0.0"
UDP_PORT = 5010
BUFFER_SIZE = 4096
GROUP = "239.255.50.10"
OUTPUT_FILE = "dcsbios_data.json.LAST"

print(f"Listening on UDP {GROUP}:{UDP_PORT}, saving to {OUTPUT_FILE}")

# === SETUP SOCKET ===
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((IP_ADDRESS, UDP_PORT))
sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, socket.inet_aton(GROUP) + socket.inet_aton(IP_ADDRESS))
sock.settimeout(0.5)

log_data = []
buffer = bytearray()
last_time = time.time()
first_packet = True

def extract_frames(data, timestamp):
    SYNC = b'\x55\x55\x55\x55'
    frames = []
    parts = data.split(SYNC)
    if parts and parts[0] == b'':
        parts = parts[1:]
    for part in parts:
        if part:
            frame = SYNC + part
            frames.append({
                "timing": round(timestamp, 5),
                "data": frame.hex().upper()
            })
    return frames

try:
    print("Logging started. Press Ctrl+C to stop.")
    while True:
        try:
            data, _ = sock.recvfrom(BUFFER_SIZE)
            now = time.time()
            elapsed = 2.0 if first_packet else now - last_time
            last_time = now
            first_packet = False

            buffer.extend(data)
            new_frames = extract_frames(buffer, elapsed)
            buffer.clear()
            log_data.extend(new_frames)

            # Write every 10 frames
            if len(log_data) % 10 == 0:
                with open(OUTPUT_FILE, "w") as f:
                    json.dump(log_data, f, indent=2)

        except socket.timeout:
            continue

except KeyboardInterrupt:
    print("\nLogging interrupted by user.")

finally:
    with open(OUTPUT_FILE, "w") as f:
        json.dump(log_data, f, indent=2)
    print(f"✅ Saved {len(log_data)} frames to {OUTPUT_FILE}")
