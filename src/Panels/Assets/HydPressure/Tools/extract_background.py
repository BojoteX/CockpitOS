import os
import re
from PIL import Image

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# --- CONFIGURATION ---
INPUT_HEADER = 'hydPressBackground.h'
OUTPUT_IMAGE = 'hydPressBackground.png'
IMAGE_WIDTH = 240
IMAGE_HEIGHT = 240
SWAP_ENDIAN = False  # <--- Set to True to swap, False for no swap

# --- PARSE HEADER FILE ---
with open(INPUT_HEADER, 'r') as f:
    contents = f.read()

pattern = re.compile(r'const\s+uint16_t\s+\w+\s*\[\s*\]\s*=\s*\{([^}]+)\};', re.DOTALL)
match = pattern.search(contents)
if not match:
    raise ValueError("Could not find image array in header.")

hex_values = match.group(1)
pixels = []

def swap16(x):
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF)

for value in re.findall(r'0x[0-9A-Fa-f]+', hex_values):
    pixel = int(value, 16)
    if SWAP_ENDIAN:
        pixel = swap16(pixel)
    pixels.append(pixel)

if len(pixels) != IMAGE_WIDTH * IMAGE_HEIGHT:
    raise ValueError(f"Expected {IMAGE_WIDTH * IMAGE_HEIGHT} pixels, got {len(pixels)}")

def r5g6b5_to_rgb888(pixel):
    r = ((pixel >> 11) & 0x1F) << 3
    g = ((pixel >> 5) & 0x3F) << 2
    b = (pixel & 0x1F) << 3
    return (r, g, b)

rgb_pixels = [r5g6b5_to_rgb888(p) for p in pixels]
img = Image.new("RGB", (IMAGE_WIDTH, IMAGE_HEIGHT))
img.putdata(rgb_pixels)
img.save(OUTPUT_IMAGE)

print(f"✅ Image saved as {OUTPUT_IMAGE} (swap_endian={SWAP_ENDIAN})")
