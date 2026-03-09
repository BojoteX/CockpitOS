"""Convert VLW font files to PROGMEM C header arrays for LovyanGFX loadFont()."""
import sys
import os

def convert(vlw_path, header_path, array_name):
    with open(vlw_path, 'rb') as f:
        data = f.read()
    size = len(data)
    with open(header_path, 'w') as out:
        out.write(f"// Auto-generated from {os.path.basename(vlw_path)} ({size} bytes)\n")
        out.write("#pragma once\n")
        out.write("#include <pgmspace.h>\n\n")
        out.write(f"const uint8_t {array_name}[] PROGMEM = {{\n")
        for i in range(0, size, 16):
            chunk = data[i:i+16]
            hex_vals = ', '.join(f'0x{b:02X}' for b in chunk)
            out.write(f"    {hex_vals},\n")
        out.write("};\n")
    print(f"  {os.path.basename(vlw_path)} -> {os.path.basename(header_path)} ({size} bytes, array: {array_name})")

if __name__ == '__main__':
    fonts_dir = os.path.join(os.environ.get('TEMP', os.environ.get('TMP', '/tmp')), 'ifei_fonts')
    out_dir = os.path.join(os.path.dirname(os.path.dirname(__file__)), 'src', 'Panels', 'Assets', 'Fonts')
    os.makedirs(out_dir, exist_ok=True)
    conversions = [
        ('IFEI-Data-36.vlw',   'IFEI_Data_36.h',   'IFEI_Data_36'),
        ('IFEI-Data-32.vlw',   'IFEI_Data_32.h',   'IFEI_Data_32'),
        ('IFEI-Labels-16.vlw', 'IFEI_Labels_16.h',  'IFEI_Labels_16'),
    ]
    for vlw_name, header_name, array_name in conversions:
        convert(os.path.join(fonts_dir, vlw_name), os.path.join(out_dir, header_name), array_name)
    print("Done.")
