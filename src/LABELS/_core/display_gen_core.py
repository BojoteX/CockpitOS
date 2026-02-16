#!/usr/bin/env python3
"""
CockpitOS Display Generator - Core Module
==========================================
Called from LABEL_SET_XXX/display_gen.py wrappers.
CWD is set by the wrapper BEFORE this module is imported.
"""
import os, sys, json, re

# When launched from the compiler tool, skip all interactive pauses.
_BATCH = os.environ.get("COCKPITOS_BATCH") == "1"

def _pause(msg="\nPress <ENTER> to exit..."):
    if not _BATCH:
        input(msg)

def run():

    # --------------------------------
    # CONFIG (adjust as needed)
    # --------------------------------
    DEV_MAPPING_CPP = "DisplayMapping.cpp"
    DEV_MAPPING_H   = "DisplayMapping.h"
    PANELS_SELECTED_FILE = "selected_panels.txt"
    PANELS_REFERENCE_FILE = "panels.txt"

    # Extract all display device prefixes for this JSON file (built once, globally)
    def extract_real_device_prefixes(data):
        prefixes = set()
        for category, controls in data.items():
            for key, item in controls.items():
                if item.get('control_type', '').lower().strip() != 'display':
                    continue
                label = item.get('identifier', key)
                prefix = label.split('_')[0] if '_' in label else label
                prefixes.add(prefix)
        return prefixes

    def merge_metadata_jsons(data, metadata_dir="METADATA"):
        """Merge all JSONs from the given subdir into the loaded data (in place)."""
        meta_path = os.path.join(os.getcwd(), metadata_dir)
        if not os.path.isdir(meta_path):
            return
        for fname in sorted(os.listdir(meta_path)):
            if not fname.lower().endswith(".json"):
                continue
            fpath = os.path.join(meta_path, fname)
            try:
                with open(fpath, "r", encoding="utf-8") as f:
                    meta_data = json.load(f)
                for category, controls in meta_data.items():
                    if category not in data:
                        data[category] = {}
                    for label, item in controls.items():
                        # Don't overwrite existing labels (aircraft takes precedence)
                        if label not in data[category]:
                            data[category][label] = item
            except Exception as e:
                print(f"Warning: could not merge {fname}: {e}")

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

    # Python equivalent of the C++ constexpr labelHash() in CUtils.h
    # Must produce identical values for hash table placement to match runtime probing
    def label_hash(s, mod):
        h = 0
        for c in reversed(s):
            h = (ord(c) + 31 * h) & 0xFFFF  # uint16_t truncation at each step
        return h % mod

    # --------------------------------
    # LOAD JSON, PANEL FILTERING
    # --------------------------------
    # --- Auto-detect valid panel JSON file (same as first script) ---
    json_files = [f for f in os.listdir() if f.lower().endswith('.json')]
    valid_jsons = []
    for fname in json_files:
        try:
            with open(fname, "r", encoding="utf-8") as f:
                data_try = json.load(f)
            if is_valid_panel_json(data_try):
                valid_jsons.append((fname, data_try))
        except Exception:
            continue  # skip bad files

    if len(valid_jsons) == 0:
        print("ERROR: No valid panel-style JSON file found in current directory.")
        _pause()
        sys.exit(1)
    if len(valid_jsons) > 1:
        print(f"ERROR: Multiple valid panel-style JSON files found: {[f[0] for f in valid_jsons]}")
        _pause()
        sys.exit(1)

    JSON_FILE, data = valid_jsons[0]

    # We do NOT included prefixes from MERGED data (from METADATA dir) we only do so for actual aircraft JSON file
    REAL_DEVICE_PREFIXES = extract_real_device_prefixes(data)
    merge_metadata_jsons(data)

    if not os.path.isfile(PANELS_SELECTED_FILE):
        # Warn + create template if needed
        print(f"WARNING: '{PANELS_SELECTED_FILE}' does not exist.")
        sys.exit(1)

    all_panels = load_panel_list("panels.txt")
    selected_panels = load_panel_list(PANELS_SELECTED_FILE)
    target_objects = selected_panels

    # --------------------------------
    # SUPERTYPE DEVICE PREFIX LOGIC: Find all device prefixes (for DisplayMapping.h)
    # --------------------------------
    all_device_prefixes = set()
    for panel, controls in data.items():
        for key, item in controls.items():
            if item.get('control_type','').lower().strip() != "display":
                continue
            label = item.get('identifier', key)
            prefix = label.split('_')[0] if '_' in label else label
            if prefix in REAL_DEVICE_PREFIXES:
                all_device_prefixes.add(prefix)

    # --------------------------------
    # PRESERVATION: parse DisplayMapping.cpp fieldDefs[] (if exists)
    # --------------------------------
    preservation_file = None
    if os.path.exists(DEV_MAPPING_CPP):
        preservation_file = DEV_MAPPING_CPP
    elif os.path.exists(DEV_MAPPING_CPP + ".DISABLED"):
        preservation_file = DEV_MAPPING_CPP + ".DISABLED"

    existing_defs = {}
    inside = False
    if preservation_file:
        with open(preservation_file, encoding="utf-8") as fcpp:
            for line in fcpp:
                if "const DisplayFieldDefLabel fieldDefs[]" in line:
                    inside = True
                    continue
                if inside and "};" in line:
                    inside = False
                    break
                if inside and "{" in line:
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

            # Only add to prefixes_seen if this is a real device
            if prefix in REAL_DEVICE_PREFIXES:
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
            preserved_line = existing_defs[label]
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

        # --- Panel-agnostic dispatcher (always!) ---
        render_func = f"render{prefix}Dispatcher"
        clear_func  = "nullptr"

        # Set sensible defaults for all fields:
        field_type  = "FIELD_STRING"
        render_type = "FIELD_RENDER_7SEG"

        # (Optional) override for label/texture types:
        if label.upper().endswith("_TEXTURE") or label.upper().endswith("_LABEL"):
            field_type = "FIELD_LABEL"
            render_type = "FIELD_RENDER_LABEL"
            numDigits, segsPerDigit = 0, 0

        line = (
            f'{{ "{label}", {map_field}, {numDigits}, {segsPerDigit}, 0, 0, {field_type}, 0, &{device_var}, {device_enum}, {render_func}, {clear_func}, {render_type} }}'
        )
        field_defs.append(line)

    # --------------------------------
    # WRITE DisplayMapping.cpp
    # --------------------------------

    # Extra Fields for DisplayMapping.cpp *** REMEMBER TO DOUBLE ESCAPE ***
    if field_defs:
        DISPLAYMAPPING_CPP_EXTRA = '''

    const DisplayFieldDefLabel* findFieldByLabel(const char* label) {
        for (size_t i = 0; i < numFieldDefs; ++i)
            if (strcmp(fieldDefs[i].label, label) == 0)
                return &fieldDefs[i];
        return nullptr;
    }

    '''
    else:
        DISPLAYMAPPING_CPP_EXTRA = '''

    const DisplayFieldDefLabel* findFieldByLabel(const char* /*label*/) {
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
            h = label_hash(label, desired_size)
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
        fcpp.write("// --- labelHash-based lookup (uses shared labelHash from CUtils.h) ---\n")
        fcpp.write("const DisplayFieldDefLabel* findFieldDefByLabel(const char* label) {\n")
        fcpp.write(f"    uint16_t startH = labelHash(label) % FIELD_HASH_SIZE;\n")
        fcpp.write("    for (uint16_t i = 0; i < FIELD_HASH_SIZE; ++i) {\n")
        fcpp.write("        uint16_t idx = (startH + i >= FIELD_HASH_SIZE) ? (startH + i - FIELD_HASH_SIZE) : (startH + i);\n")
        fcpp.write("        const auto& entry = fieldDefHashTable[idx];\n")
        fcpp.write("        if (!entry.label) return nullptr;\n")
        fcpp.write("        if (strcmp(entry.label, label) == 0) return entry.def;\n")
        fcpp.write("    }\n")
        fcpp.write("    return nullptr;\n")
        fcpp.write("}\n")

        # Extra fields for .cpp file
        fcpp.write(DISPLAYMAPPING_CPP_EXTRA)

    # --------------------------------
    # WRITE DisplayMapping.h (SUPERTYPE: use all_device_prefixes)
    # --------------------------------
    with open(DEV_MAPPING_H, "w", encoding="utf-8") as fout:
        fout.write("// DisplayMapping.h - Auto-generated\n#pragma once\n\n")

        # This line added NEW on 8/24/25
        fout.write("#include \"../../../lib/CUtils/src/CUtils.h\"\n\n")

        # fout.write("class HUDDisplay;\n")
        # fout.write("class IFEIDisplay;\n")
        # fout.write("class UFCDisplay;\n")

        # Emit feature defines for present segment maps
        for prefix in sorted(all_device_prefixes):
            segmap_filename = f"{prefix}_SegmentMap.h"
            if os.path.exists(segmap_filename):
                fout.write(f'#define HAS_{prefix}_SEGMENT_MAP\n')

        # Emit segment map includes for present files only
        for prefix in sorted(all_device_prefixes):
            segmap_filename = f"{prefix}_SegmentMap.h"
            if os.path.exists(segmap_filename):
                fout.write(f'#include "{segmap_filename}"\n')

        fout.write("\n// Enums for FieldType, DisplayDeviceType\n")
        fout.write("enum FieldType { FIELD_LABEL, FIELD_STRING, FIELD_NUMERIC, FIELD_MIXED, FIELD_BARGRAPH };\n")
        fout.write("enum DisplayDeviceType { " + ', '.join(f"DISPLAY_{p}" for p in sorted(all_device_prefixes)) + " };\n\n")
        # Device class forward declarations
        for prefix in sorted(all_device_prefixes):
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
        for prefix in sorted(all_device_prefixes):
            var = prefix.lower()
            fout.write(f"extern {prefix}Display {var};\n")
        # --- Emit dispatcher function externs (these are required if any entry uses them)
        for prefix in sorted(all_device_prefixes):
            fout.write(f"extern void render{prefix}Dispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);\n")
            fout.write(f"extern void clear{prefix}Dispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);\n")
        fout.write("// Function pointers (renderFunc, clearFunc) are nullptr in all generated records unless otherwise preserved.\n")

    print(f"Generated {DEV_MAPPING_CPP} and {DEV_MAPPING_H} for devices: {', '.join(sorted(all_device_prefixes))}")

if __name__ == "__main__":
    run()
