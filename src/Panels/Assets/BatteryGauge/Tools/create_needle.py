import os
import struct

# --- CONFIG ---
BMP_FILE = "output_needle_nvg.bmp"
HEADER_FILE = "batNeedle.h"
ARRAY_NAME = "batNeedle"
EXPECTED_WIDTH = 15
EXPECTED_HEIGHT = 88
EXPECTED_BPP = 16
PIXELS_PER_LINE = 15
TRANSPARENT_MASK = 0x0120  # Value used to indicate transparency (e.g. replace black)

def rgb565_from_bytes(b1, b2):
    """Little-endian bytes → RGB565 uint16"""
    return b2 << 8 | b1

def write_header_file(pixels, output_path):
    with open(output_path, 'w') as f:
        f.write('#include <TFT_eSPI.h>\n')
        f.write(f"// 'Needle', {EXPECTED_WIDTH}x{EXPECTED_HEIGHT} Pivot point 7x84\n")
        f.write('// Image produced in .png format (I used paint.net but any suitable editor is fine - something supporting layers is useful)\n')
        f.write('// Image then converted into code format using Icd-image-converter, again alternatives are fine\n')
        f.write('// Image needs to be in R5G6B5 color format, at a block size of 16 bit and in Little-Endian Byte order\n')
        f.write('// Once the code is imported into the Tab, use the Edit-Find drop down to replace the background color with the one for TRANSPARENT - 0x0120\n')
        f.write(f'const uint16_t {ARRAY_NAME} [] = {{\n')

        for i in range(0, len(pixels), PIXELS_PER_LINE):
            line = ', '.join(f'0x{pixels[i + j]:04X}' for j in range(min(PIXELS_PER_LINE, len(pixels) - i)))
            f.write(f'    {line},\n')

        f.write('};\n')
        f.write(f'const size_t {ARRAY_NAME}Len = sizeof({ARRAY_NAME}) / sizeof({ARRAY_NAME}[0]);\n')
    print(f"✅ Header file written to: {output_path}")

def main():
    print(f"🔍 Validating: {BMP_FILE}")

    if not os.path.exists(BMP_FILE):
        print("❌ File not found.")
        return

    with open(BMP_FILE, 'rb') as f:
        header = f.read(54)

        if header[0:2] != b'BM':
            print("❌ Not a BMP file.")
            return

        width, height = struct.unpack('<ii', header[18:26])
        bpp = struct.unpack('<H', header[28:30])[0]
        compression = struct.unpack('<I', header[30:34])[0]
        offset = struct.unpack('<I', header[10:14])[0]

        if (width, abs(height)) != (EXPECTED_WIDTH, EXPECTED_HEIGHT):
            print(f"❌ Image size is {width}x{height}, expected {EXPECTED_WIDTH}x{EXPECTED_HEIGHT}.")
            return

        if bpp != EXPECTED_BPP:
            print(f"❌ Expected 16bpp, found {bpp}bpp.")
            return

        if compression not in (0, 3):  # BI_RGB or BI_BITFIELDS
            print(f"❌ Unsupported BMP compression: {compression}")
            return

        # BMP row stride is padded to 4-byte alignment
        row_stride = (EXPECTED_WIDTH * 2 + 3) & ~3

        # Read and decode rows with padding accounted for
        f.seek(offset)
        rows = []
        for _ in range(EXPECTED_HEIGHT):
            row_data = f.read(row_stride)
            row = [rgb565_from_bytes(row_data[i], row_data[i+1]) for i in range(0, EXPECTED_WIDTH * 2, 2)]
            rows.append(row)

        rows.reverse()  # BMP stores pixel rows bottom-up

        # Replace pure black (0x0000) with transparent marker
        fixed_pixels = [
            TRANSPARENT_MASK if px == 0x0000 else px
            for row in rows for px in row
        ]

        write_header_file(fixed_pixels, HEADER_FILE)

if __name__ == "__main__":
    main()
