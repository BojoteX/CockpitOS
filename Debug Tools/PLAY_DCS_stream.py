import socket
import json
import time
import binascii
import os
import argparse
from datetime import datetime

# === CONFIGURATION ===
MULTICAST_IP = "239.255.50.10"
UDP_PORT = 5010
STREAMS_DIR = "streams"
FILE_PREFIX = "dcsbios_data.json."

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

# === FUNCTION: Calculate relative age string ===
def get_relative_age(file_path):
    mtime = os.path.getmtime(file_path)
    age_seconds = time.time() - mtime
    
    if age_seconds < 60:
        return f"{int(age_seconds)} seconds old"
    elif age_seconds < 3600:
        minutes = int(age_seconds / 60)
        return f"{minutes} minute{'s' if minutes != 1 else ''} old"
    elif age_seconds < 86400:
        hours = int(age_seconds / 3600)
        return f"{hours} hour{'s' if hours != 1 else ''} old"
    elif age_seconds < 2592000:  # 30 days
        days = int(age_seconds / 86400)
        return f"{days} day{'s' if days != 1 else ''} old"
    else:
        months = int(age_seconds / 2592000)
        return f"{months} month{'s' if months != 1 else ''} old"

# === FUNCTION: Discover available streams ===
def discover_streams(streams_path):
    """Returns list of tuples: (identifier, full_path, display_name, age_string)"""
    streams = []
    
    if not os.path.isdir(streams_path):
        return streams
    
    for filename in os.listdir(streams_path):
        if filename.startswith(FILE_PREFIX):
            identifier = filename[len(FILE_PREFIX):]  # Extract extension/identifier
            full_path = os.path.join(streams_path, filename)
            
            # Special display name for LAST
            if identifier == "LAST":
                display_name = "LAST (Last captured stream)"
            else:
                display_name = identifier
            
            age_string = get_relative_age(full_path)
            streams.append((identifier, full_path, display_name, age_string))
    
    # Sort: LAST first (if present), then alphabetically
    streams.sort(key=lambda x: (x[0] != "LAST", x[0]))
    return streams

# === FUNCTION: Display menu and get selection ===
def select_stream(streams):
    print("\n" + "=" * 50)
    print("       DCS-BIOS Stream Selector")
    print("=" * 50)
    
    for idx, (identifier, path, display_name, age) in enumerate(streams, start=1):
        print(f"  [{idx}] {display_name:<30} ({age})")
    
    print("=" * 50)
    
    while True:
        try:
            choice = input("Select stream number: ").strip()
            choice_num = int(choice)
            if 1 <= choice_num <= len(streams):
                return streams[choice_num - 1]
            else:
                print(f"⚠️  Enter a number between 1 and {len(streams)}")
        except ValueError:
            print("⚠️  Enter a valid number")

# === ARGUMENT PARSING ===
parser = argparse.ArgumentParser(description="DCS-BIOS UDP Replay Tool")
parser.add_argument("--speed", type=float, default=1.0, help="Replay speed multiplier (e.g. 2.0 = 2x faster)")
parser.add_argument("--fps", type=float, help="Override all delays and force fixed FPS (e.g. 60)")
parser.add_argument("--stream", type=str, help="Stream identifier to load directly (bypasses menu)")
args = parser.parse_args()

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# === STREAM SELECTION ===
streams_path = os.path.join(os.getcwd(), STREAMS_DIR)
available_streams = discover_streams(streams_path)

if not available_streams:
    print("\n❌ No stream files found!")
    print(f"   Expected location: {streams_path}")
    print(f"   Expected pattern:  {FILE_PREFIX}<identifier>")
    print("\n💡 Run DCS_stream_capture.py to capture a stream.")
    print("   It will create: dcsbios_data.json.LAST")
    exit(1)

# Determine which stream to use
if args.stream:
    # Direct selection via command line
    match = None
    for stream in available_streams:
        if stream[0].upper() == args.stream.upper():
            match = stream
            break
    if not match:
        print(f"\n❌ Stream '{args.stream}' not found!")
        print("   Available streams:", ", ".join(s[0] for s in available_streams))
        exit(1)
    selected = match
    print(f"\n🎯 Auto-selected stream: {selected[2]} ({selected[3]})")
else:
    # Interactive menu
    selected = select_stream(available_streams)

identifier, INPUT_JSON_FILE, display_name, age = selected

# === SETUP SOCKET ===
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
sock.setsockopt(socket.IPPROTO_IP, socket.IP_MULTICAST_TTL, 1)
local_ip = get_local_ip()

# === LOAD DATA ===
with open(INPUT_JSON_FILE, "r") as f:
    frames = json.load(f)

print(f"\n[INFO] Loaded {len(frames)} frames from {os.path.basename(INPUT_JSON_FILE)}")
print(f"[INFO] Multicast to {MULTICAST_IP}:{UDP_PORT} via {local_ip}")

if args.fps:
    print(f"🔁 Fixed replay at {args.fps:.1f} FPS ({1000/args.fps:.2f}ms/frame)")
else:
    print(f"🔁 Using recorded frame timing scaled by x{args.speed}")

print("⏳ Press Ctrl+C to stop.\n")

# === REPLAY LOOP ===
frame_timestamps = []
accum_time = 0.0
for frame in frames:
    frame_timestamps.append(accum_time)
    accum_time += frame.get("timing", 0) / args.speed if not args.fps else (1.0 / args.fps)

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