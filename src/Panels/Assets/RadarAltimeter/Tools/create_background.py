import os
from PIL import Image

# Change to script directory
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# --- CONFIG ---
PNG_FILE = "cabinPressBackground.png"
HEADER_FILE = "cabinPressBackground.h"
ARRAY_NAME = "cabinPressBackground"
EXPECTED_WIDTH = 360
EXPECTED_HEIGHT = 360
PIXELS_PER_LINE = 8
SWAP_ENDIAN = False    # <--- Set True for MCU/LCD little-endian, False for PC-style

def swap16(x):
    return ((x & 0xFF) << 8) | ((x >> 8) & 0xFF)

def rgb565_from_rgb(r, g, b):
    """Convert 8-bit RGB to 16-bit RGB565"""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def write_header_file(pixels, output_path):
    with open(output_path, 'w') as f:
        f.write('// \'cabin pressure\', 360x360px\n')
        f.write(f'// This array is generated from PNG (RGB, 16bpp RGB565 {"LE" if SWAP_ENDIAN else "BE"} order)\n')
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

    im = Image.open(PNG_FILE).convert("RGB")  # Ignore alpha for backgrounds
    width, height = im.size

    if (width, height) != (EXPECTED_WIDTH, EXPECTED_HEIGHT):
        print(f"❌ Image size is {width}x{height}, expected {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}.")
        return

    pixels = []
    for y in range(EXPECTED_HEIGHT):
        for x in range(EXPECTED_WIDTH):
            r, g, b = im.getpixel((x, y))
            val = rgb565_from_rgb(r, g, b)
            if SWAP_ENDIAN:
                val = swap16(val)
            pixels.append(val)

    write_header_file(pixels, HEADER_FILE)

if __name__ == "__main__":
    main()
