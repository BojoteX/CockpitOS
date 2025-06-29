import json
import struct
import os
os.chdir(os.path.dirname(os.path.abspath(__file__)))

INPUT_JSON = "dcsbios_data.json"
OUTPUT_HEADER = "DcsbiosReplayData.h"

def main():
    with open(INPUT_JSON, "r") as f:
        json_data = json.load(f)

    binary_buffer = bytearray()

    for entry in json_data:
        try:
            delay = float(entry["timing"])
            data = bytes.fromhex(entry["data"])
            length = len(data)
            binary_buffer += struct.pack('<fH', delay, length)
            binary_buffer += data
        except Exception as e:
            print(f"Skipping entry due to error: {e}")
            continue

    with open(OUTPUT_HEADER, "w") as f:
        f.write("// Auto-generated binary replay blob\n")
        f.write("#pragma once\n")
        f.write(f"const size_t dcsbiosReplayLength = {len(binary_buffer)};\n")
        f.write("const uint8_t dcsbiosReplayData[] = {\n")

        # Emit data in readable chunks
        for i in range(0, len(binary_buffer), 12):
            chunk = binary_buffer[i:i+12]
            hex_line = ", ".join(f"0x{b:02X}" for b in chunk)
            f.write(f"  {hex_line},\n")

        f.write("};\n")

if __name__ == "__main__":
    main()
