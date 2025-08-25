#!/usr/bin/env python3
import os, sys, json, re
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# --------------------------------
# CONFIG (adjust as needed)
# --------------------------------
JSON_FILE = "FA-18C_hornet.json"          # or auto-detect
DEV_MAPPING_CPP = "DisplayMapping.cpp"
DEV_MAPPING_H   = "DisplayMapping.h"
PANELS_SELECTED_FILE = "selected_panels.txt"
PANELS_REFERENCE_FILE = "panels.txt"

def is_valid_panel_json(data):
    if not isinstance(data, dict) or not data:
        return False
    for k, v in data.items():
        if not isinstance(v, dict):
            return False
    return True

def load_panel_list(filename):
    panels = set()
    with open(filename, "r", encoding="utf-8") as f:
        for line in f:
            name = line.strip()
            if name and not name.startswith("#"):
                panels.add(name)
    return panels

def next_prime(n):
    def is_prime(n):
        if n < 2: return False
        if n % 2 == 0: return n == 2
        for i in range(3, int(n**0.5)+1, 2):
            if n % i == 0: return False
        return True
    while not is_prime(n):
        n += 1
    return n

def djb2_hash(s, mod):
    h = 5381
    for c in s:
        h = ((h << 5) + h) + ord(c)
    return h % mod

# --------------------------------
# LOAD JSON, PANEL FILTERING
# --------------------------------
with open(JSON_FILE, "r", encoding="utf-8") as f:
    data = json.load(f)
if not is_valid_panel_json(data):
    print("ERROR: Not a valid panel JSON file")
    sys.exit(1)

if not os.path.isfile(PANELS_SELECTED_FILE):
    # Warn + create template if needed
    print(f"WARNING: '{PANELS_SELECTED_FILE}' does not exist.")
    sys.exit(1)

all_panels = load_panel_list("panels.txt")
selected_panels = load_panel_list(PANELS_SELECTED_FILE)
target_objects = selected_panels

# --------------------------------
# PRESERVATION: parse DisplayMapping.cpp fieldDefs[] (if exists)
# --------------------------------
existing_defs = {}
inside = False
if os.path.exists(DEV_MAPPING_CPP):
    with open(DEV_MAPPING_CPP, encoding="utf-8") as fcpp:
        for line in fcpp:
            if "const DisplayFieldDefLabel fieldDefs[]" in line:
                inside = True
                continue
            if inside and "};" in line:
                inside = False
                break
            if inside and "{" in line:
                # This is a field initializer line (may have comments, etc.)
                # Extract label (first quoted string)
                m = re.match(r'\s*\{\s*"([^"]+)"', line)
                if m:
                    label = m.group(1)
                    existing_defs[label] = line.rstrip('\n')

# --------------------------------
# BUILD fieldDefs[] ENTRIES FOR SELECTED PANELS
# --------------------------------
field_defs = []
prefixes_seen = set()
display_fields = []

for panel, controls in data.items():
    if panel not in target_objects:
        continue
    for key, item in controls.items():
        if item.get('control_type','').lower().strip() != "display":
            continue
        label = item.get('identifier', key)
        prefix = label.split('_')[0] if '_' in label else label
        prefixes_seen.add(prefix)
        max_length = 0
        if "outputs" in item and item["outputs"]:
            max_length = item["outputs"][0].get("max_length", 0)
        display_fields.append((label, prefix, max_length))

# Order by label (for deterministic output)
display_fields.sort(key=lambda x: x[0])

# --- Parse all SegmentMap headers for declared map variables ---
segmentmap_vars = {}  # key: varname, value: dims (e.g., '[3][7]', '[14]', or None)
var_decl_re = re.compile(
    r'const\s+SegmentMap\s+([A-Z0-9_]+)\s*((\[[^\]]+\])*)\s*=', re.MULTILINE
)
for prefix in prefixes_seen:
    map_filename = f"{prefix}_SegmentMap.h"
    if not os.path.exists(map_filename):
        continue
    with open(map_filename, "r", encoding="utf-8") as fmap:
        text = fmap.read()
        for m in var_decl_re.finditer(text):
            varname = m.group(1)
            dims = m.group(2).strip() if m.group(2) else None
            segmentmap_vars[varname] = dims

def get_map_pointer_and_dims(label):
    base_upper = label.upper()
    # Strict: LABEL/TEXTURE fields only match _LABEL, everything else _MAP
    if base_upper.endswith("_TEXTURE") or base_upper.endswith("_LABEL"):
        cand = f"{base_upper}_LABEL"
    else:
        cand = f"{base_upper}_MAP"
    if cand in segmentmap_vars:
        dims = segmentmap_vars[cand]
        # Pointer style: &NAME[0][0] for 2D, NAME for 1D, &NAME for struct
        if dims:
            counts = re.findall(r'\[(\d+)\]', dims)
            if len(counts) == 2:
                # e.g., [3][7]
                return f"&{cand}[0][0]", int(counts[0]), int(counts[1])
            elif len(counts) == 1:
                # e.g., [14]
                return f"{cand}", int(counts[0]), 0
            else:
                # Scalar
                return f"&{cand}", 0, 0
        else:
            # No dimensions = scalar (struct or single)
            return f"&{cand}", 0, 0
    return "nullptr", 0, 0

field_defs = []
for label, prefix, max_length in display_fields:
    if label in existing_defs:
        # Only preserve if it's NOT a nullptr map
        # Parse out the map field from the preserved line
        preserved_line = existing_defs[label]
        # Try to extract the map field (2nd argument)
        m = re.match(r'\s*\{\s*"[^"]+",\s*([^\s,]+),', preserved_line)
        preserved_map = m.group(1) if m else None
        if preserved_map and preserved_map != "nullptr":
            field_defs.append(preserved_line)
        continue

    device_var = prefix.lower()
    device_enum = f"DISPLAY_{prefix}"

    # Get strict map pointer and dimensions
    map_field, numDigits, segsPerDigit = get_map_pointer_and_dims(label)

    # --- ONLY emit if there is a valid mapping! ---
    if map_field == "nullptr":
        continue  # skip entry if no SegmentMap available

    render_func = f"render{prefix}Dispatcher"
    clear_func  = "nullptr"  # (Or f"clear{prefix}Dispatcher" if you want to prefill, but 'nullptr' is typical.)
  
    line = (
        f'{{ "{label}", {map_field}, {numDigits}, {segsPerDigit}, 0, 0, {field_type}, 0, &{device_var}, {device_enum}, {render_func}, {clear_func}, {render_type} }}'
    )

    field_defs.append(line)

# --------------------------------
# WRITE DisplayMapping.cpp
# --------------------------------

# Extra Fields for DisplayMapping.cpp *** REMEMBER TO DOUBLE ESCAPE *** 
DISPLAYMAPPING_CPP_EXTRA = '''

const DisplayFieldDefLabel* findFieldByLabel(const char* label) {
    for (size_t i = 0; i < numFieldDefs; ++i)
        if (strcmp(fieldDefs[i].label, label) == 0)
            return &fieldDefs[i];
    return nullptr;
}

'''

with open(DEV_MAPPING_CPP, "w", encoding="utf-8") as fcpp:
    fcpp.write("// DisplayMapping.cpp - Defines the display fields\n")
    fcpp.write('#include "../../Globals.h"\n#include "DisplayMapping.h"\n\n')
    fcpp.write("const DisplayFieldDefLabel fieldDefs[] = {\n")

    # Correctly preserve commas after each line if hand editing
    for i, line in enumerate(field_defs):
        is_last = (i == len(field_defs) - 1)
        # Remove any trailing comma from the preserved/generated line
        line_out = line.rstrip()
        if line_out.endswith(','):
            line_out = line_out.rstrip(',')
        # Only add comma if not last entry
        if not is_last:
            fcpp.write(f"{line_out},\n")
        else:
            fcpp.write(f"{line_out}\n")
    fcpp.write("};\n")

    fcpp.write("size_t numFieldDefs = sizeof(fieldDefs) / sizeof(fieldDefs[0]);\n")
    fcpp.write("FieldState fieldStates[sizeof(fieldDefs) / sizeof(fieldDefs[0])];\n\n")

    # --- Generate hash table for fieldDefs[] ---
    # Only include labels that actually have an entry in field_defs (in output order)
    labels = []
    for i, line in enumerate(field_defs):
        m = re.match(r'\s*\{\s*"([^"]+)"', line)
        if m:
            labels.append(m.group(1))

    desired_size = next_prime(len(labels) * 2)
    hash_table = ["{nullptr, nullptr}"] * desired_size
    for idx, label in enumerate(labels):
        h = djb2_hash(label, desired_size)
        for probe in range(desired_size):
            if hash_table[h] == "{nullptr, nullptr}":
                hash_table[h] = f'{{"{label}", &fieldDefs[{idx}]}}'
                break
            h = (h + 1) % desired_size
        else:
            print(f"❌ Hash table full for fieldDefs", file=sys.stderr)
            sys.exit(1)

    fcpp.write("// --- Hash Table for fieldDefs[] ---\n")
    fcpp.write(f"struct FieldDefHashEntry {{ const char* label; const DisplayFieldDefLabel* def; }};\n")
    fcpp.write(f"static const size_t FIELD_HASH_SIZE = {desired_size};\n")
    fcpp.write(f"static const FieldDefHashEntry fieldDefHashTable[FIELD_HASH_SIZE] = {{\n")
    for entry in hash_table:
        fcpp.write(f"    {entry},\n")
    fcpp.write("};\n\n")
    fcpp.write("// --- djb2-based hash function and lookup ---\n")
    fcpp.write("constexpr uint16_t djb2_hash(const char* s, size_t mod) {\n")
    fcpp.write("    uint32_t h = 5381;\n")
    fcpp.write("    while (*s) h = ((h << 5) + h) + (uint8_t)(*s++);\n")
    fcpp.write("    return h % mod;\n")
    fcpp.write("}\n\n")
    fcpp.write("const DisplayFieldDefLabel* findFieldDefByLabel(const char* label) {\n")
    fcpp.write("    uint16_t startH = djb2_hash(label, FIELD_HASH_SIZE);\n")
    fcpp.write("    for (uint16_t i = 0; i < FIELD_HASH_SIZE; ++i) {\n")
    fcpp.write("        uint16_t idx = (startH + i >= FIELD_HASH_SIZE) ? (startH + i - FIELD_HASH_SIZE) : (startH + i);\n")
    fcpp.write("        const auto& entry = fieldDefHashTable[idx];\n")
    fcpp.write("        if (!entry.label) continue;\n")
    fcpp.write("        if (strcmp(entry.label, label) == 0) return entry.def;\n")
    fcpp.write("    }\n")
    fcpp.write("    return nullptr;\n")
    fcpp.write("}\n")

    # Extra fields for .cpp file
    fcpp.write(DISPLAYMAPPING_CPP_EXTRA)

# --------------------------------
# WRITE DisplayMapping.h
# Only emit devices actually seen in .cpp's fieldDefs[]
# --------------------------------
with open(DEV_MAPPING_H, "w", encoding="utf-8") as fout:
    fout.write("// DisplayMapping.h - Auto-generated\n#pragma once\n\n")
    
    # Segment map includes but ONLY for present devices
    for prefix in sorted(prefixes_seen):
        fout.write(f'#include "{prefix}_SegmentMap.h"\n')

    fout.write("\n// Enums for FieldType, DisplayDeviceType\n")
    fout.write("enum FieldType { FIELD_LABEL, FIELD_STRING, FIELD_NUMERIC, FIELD_MIXED, FIELD_BARGRAPH };\n")
    fout.write("enum DisplayDeviceType { " + ', '.join(f"DISPLAY_{p}" for p in sorted(prefixes_seen)) + " };\n\n")
    # Device class forward declarations
    for prefix in sorted(prefixes_seen):
        fout.write(f"class {prefix}Display;\n")
    fout.write("\n")
    fout.write("enum FieldRenderType : uint8_t {\n")
    fout.write("    FIELD_RENDER_7SEG,\n")
    fout.write("    FIELD_RENDER_7SEG_SHARED,\n")
    fout.write("    FIELD_RENDER_LABEL,\n")
    fout.write("    FIELD_RENDER_BINGO,\n")
    fout.write("    FIELD_RENDER_BARGRAPH,\n")
    fout.write("    FIELD_RENDER_FUEL,\n")
    fout.write("    FIELD_RENDER_RPM,\n")
    fout.write("    FIELD_RENDER_ALPHA_NUM_FUEL,\n")
    fout.write("    FIELD_RENDER_CUSTOM,\n")
    fout.write("};\n\n")
    fout.write("// Structure for a field definition\n")
    fout.write("struct DisplayFieldDefLabel {\n")
    fout.write("    const char* label;\n")
    fout.write("    const SegmentMap* segMap;\n")
    fout.write("    uint8_t numDigits;\n")
    fout.write("    uint8_t segsPerDigit;\n")
    fout.write("    int minValue;\n")
    fout.write("    int maxValue;\n")
    fout.write("    FieldType type;\n")
    fout.write("    uint8_t barCount;\n")
    fout.write("    void* driver;\n")
    fout.write("    DisplayDeviceType deviceType;\n")
    fout.write("    void (*renderFunc)(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);\n")
    fout.write("    void (*clearFunc)(void*, const SegmentMap*, const DisplayFieldDefLabel&);\n")
    fout.write("    FieldRenderType renderType;\n")
    fout.write("};\n\n")
    fout.write("struct FieldState {\n")
    fout.write("    char lastValue[8];\n")
    fout.write("};\n\n")

    fout.write("const DisplayFieldDefLabel* findFieldByLabel(const char* label);\n")
    fout.write("const DisplayFieldDefLabel* findFieldDefByLabel(const char* label);\n\n")

    fout.write("extern const DisplayFieldDefLabel fieldDefs[];\n")
    fout.write("extern size_t numFieldDefs;\n")
    fout.write("extern FieldState fieldStates[];\n")

    # Emit externs per detected device
    for prefix in sorted(prefixes_seen):
        var = prefix.lower()
        fout.write(f"extern {prefix}Display {var};\n")
    # --- Emit dispatcher function externs (these are required if any entry uses them)
    for prefix in sorted(prefixes_seen):
        fout.write(f"extern void render{prefix}Dispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);\n")
        fout.write(f"extern void clear{prefix}Dispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);\n")
    fout.write("// Function pointers (renderFunc, clearFunc) are nullptr in all generated records unless otherwise preserved.\n")

print(f"Generated {DEV_MAPPING_CPP} and {DEV_MAPPING_H} for devices: {', '.join(sorted(prefixes_seen))}")

input("\nPress <ENTER> to exit...")

