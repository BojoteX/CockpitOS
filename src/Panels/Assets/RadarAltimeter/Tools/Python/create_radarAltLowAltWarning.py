import os
from PIL import Image

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# --- CONFIG ---
PNG_FILE = "radarAltLowAltWarning.png"
HEADER_FILE = "radarAltLowAltWarning.h"
ARRAY_NAME = "radarAltLowAltWarning"
EXPECTED_WIDTH = 34
EXPECTED_HEIGHT = 34
PIXELS_PER_LINE = 15

TRANSPARENT_MASK = 0x0120  # Value used to indicate transparency (RGB565)
SWAP_ENDIAN = True         # <--- Set to True for little-endian (MCU order), False for "PC order"
SWAP_ENDIAN_TRANSPARENT = True         # <--- Set to True for little-endian (MCU order), False for "PC order"

def swap16(x):
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF)

def rgb565_from_rgb(r, g, b):
    """Convert 8-bit RGB to 16-bit RGB565"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def write_header_file(pixels, output_path):
    with open(output_path, 'w') as f:
        f.write(f"// 'Radar Altimeter Low Alt Warning', {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}\n")
        f.write(f"// This array is generated from PNG (RGB, 16bpp RGB565 {'LE' if SWAP_ENDIAN else 'BE'} order)\n")
        f.write(f'const uint16_t {ARRAY_NAME} [] = {{\n')
        for i in range(0, len(pixels), PIXELS_PER_LINE):
            line = ', '.join(f'0x{pixels[i + j]:04X}' for j in range(min(PIXELS_PER_LINE, len(pixels) - i)))
            f.write(f'    {line},\n')
        f.write('};\n')
        f.write(f'const size_t {ARRAY_NAME}Len = sizeof({ARRAY_NAME}) / sizeof({ARRAY_NAME}[0]);\n')
    print(f"✅ Header file written to: {output_path}")

def main():
    print(f"🔍 Validating: {PNG_FILE}")
    if not os.path.exists(PNG_FILE):
        print("❌ File not found.")
        return

    im = Image.open(PNG_FILE).convert("RGBA")  # RGBA for alpha check
    width, height = im.size

    if (width, height) != (EXPECTED_WIDTH, EXPECTED_HEIGHT):
        print(f"❌ Image size is {width}x{height}, expected {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}.")
        return

    # Swap transparent color constant if needed
    transparent_val = TRANSPARENT_MASK
    if SWAP_ENDIAN_TRANSPARENT:
        transparent_val = swap16(TRANSPARENT_MASK)

    pixels = []
    for y in range(EXPECTED_HEIGHT):
        for x in range(EXPECTED_WIDTH):
            r, g, b, a = im.getpixel((x, y))
            if a == 0 or (r == 0 and g == 0 and b == 0):  # allow true alpha or "pure black"
                val = transparent_val
            else:
                val = rgb565_from_rgb(r, g, b)
                if SWAP_ENDIAN:
                    val = swap16(val)
            pixels.append(val)

    write_header_file(pixels, HEADER_FILE)

if __name__ == "__main__":
    main()
