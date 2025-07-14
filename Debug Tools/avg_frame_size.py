import json
import os

# Always operate in script's directory (per your workflow)
os.chdir(os.path.dirname(__file__) or ".")

with open("dcsbios_data.json", "r") as f:
    data = json.load(f)

# Detect structure: list of frames or dict with "frames"
if isinstance(data, dict) and "frames" in data:
    frames = data["frames"]
elif isinstance(data, list):
    frames = data
else:
    raise Exception("Unrecognized JSON structure")

# Calculate frame sizes
frame_sizes = [len(frame.get("data", [])) for frame in frames if isinstance(frame, dict) and "data" in frame]

if len(frame_sizes) < 3:
    print("Not enough frames to discard min and max!")
    exit(1)

# Discard the largest and smallest
frame_sizes.sort()
trimmed_sizes = frame_sizes[1:-1]  # Discard first (min) and last (max)

avg_size = sum(trimmed_sizes) / len(trimmed_sizes)

print(f"Frame count (after trim): {len(trimmed_sizes)}")
print(f"Average frame size: {avg_size:.2f} bytes")

input("\nPress <ENTER> to exit...")
