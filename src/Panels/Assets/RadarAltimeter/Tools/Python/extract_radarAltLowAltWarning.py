import os
import re
from PIL import Image

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# --- CONFIGURATION ---
INPUT_HEADER = 'radarAltLowAltWarning.h'
OUTPUT_IMAGE = 'radarAltLowAltWarning.png'
IMAGE_WIDTH = 45
IMAGE_HEIGHT = 45

TRANSPARENT_COLOR = 0x0120  # RGB565 value that marks transparent
SWAP_ENDIAN = False           # <--- Swap pixel data?
SWAP_TRANSPARENT_COLOR = False  # <--- Swap transparent color constant?

# --- BYTE SWAP FUNCTION ---
def swap16(x):
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF)

# --- SWAP TRANSPARENT COLOR IF NEEDED ---
if SWAP_TRANSPARENT_COLOR:
    TRANSPARENT_COLOR = swap16(TRANSPARENT_COLOR)

# --- PARSE HEADER FILE ---
with open(INPUT_HEADER, 'r') as f:
    contents = f.read()

pattern = re.compile(r'const\s+uint16_t\s+\w+\s*\[\s*\]\s*=\s*\{([^}]+)\};', re.DOTALL)
match = pattern.search(contents)
if not match:
    raise ValueError("Could not find image array in header.")

hex_values = match.group(1)
pixels = []

for value in re.findall(r'0x[0-9A-Fa-f]+', hex_values):
    pixel = int(value, 16)
    if SWAP_ENDIAN:
        pixel = swap16(pixel)
    pixels.append(pixel)

if len(pixels) != IMAGE_WIDTH * IMAGE_HEIGHT:
    raise ValueError(f"Expected {IMAGE_WIDTH * IMAGE_HEIGHT} pixels, got {len(pixels)}")

def r5g6b5_to_rgba(pixel):
    r = ((pixel >> 11) & 0x1F) << 3
    g = ((pixel >> 5) & 0x3F) << 2
    b = (pixel & 0x1F) << 3
    a = 0 if pixel == TRANSPARENT_COLOR else 255
    return (r, g, b, a)

rgba_pixels = [r5g6b5_to_rgba(p) for p in pixels]

img = Image.new("RGBA", (IMAGE_WIDTH, IMAGE_HEIGHT))
img.putdata(rgba_pixels)
img.save(OUTPUT_IMAGE)

print(f"✅ Needle image saved as {OUTPUT_IMAGE} ({IMAGE_WIDTH}x{IMAGE_HEIGHT}, swap_endian={SWAP_ENDIAN}, swap_transparent_color={SWAP_TRANSPARENT_COLOR})")
