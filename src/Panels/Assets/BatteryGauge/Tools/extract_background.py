import re
import struct
from PIL import Image

# --- CONFIGURATION ---
INPUT_HEADER = 'batBackgroundNVG.h'  # Your header file
OUTPUT_IMAGE = 'output_image.bmp'
IMAGE_WIDTH = 240
IMAGE_HEIGHT = 240

# --- PARSE HEADER FILE ---
with open(INPUT_HEADER, 'r') as f:
    contents = f.read()

# Extract the uint16_t array
pattern = re.compile(r'const\s+uint16_t\s+\w+\s*\[\s*\]\s*=\s*\{([^}]+)\};', re.DOTALL)
match = pattern.search(contents)

if not match:
    raise ValueError("Could not find image array in header.")

hex_values = match.group(1)
pixels = []

for value in re.findall(r'0x[0-9A-Fa-f]+', hex_values):
    pixel = int(value, 16)
    pixels.append(pixel)

if len(pixels) != IMAGE_WIDTH * IMAGE_HEIGHT:
    raise ValueError(f"Expected {IMAGE_WIDTH * IMAGE_HEIGHT} pixels, got {len(pixels)}")

# --- CONVERT R5G6B5 TO RGB ---
def r5g6b5_to_rgb888(pixel):
    r = ((pixel >> 11) & 0x1F) << 3
    g = ((pixel >> 5) & 0x3F) << 2
    b = (pixel & 0x1F) << 3
    return (r, g, b)

rgb_pixels = [r5g6b5_to_rgb888(p) for p in pixels]

# --- CREATE IMAGE ---
img = Image.new("RGB", (IMAGE_WIDTH, IMAGE_HEIGHT))
img.putdata(rgb_pixels)
img.save(OUTPUT_IMAGE)

print(f"✅ Image saved as {OUTPUT_IMAGE}")
