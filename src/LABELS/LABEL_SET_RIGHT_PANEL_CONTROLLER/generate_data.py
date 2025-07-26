#!/usr/bin/env python3
import os, sys, json
import subprocess
from collections import defaultdict
os.chdir(os.path.dirname(os.path.abspath(__file__)))

# Get our current working LABEL SET directory
current_dir = os.path.abspath(os.path.dirname(__file__))
current_label_set = os.path.basename(current_dir)

print(f"Current LABEL SET: {current_label_set}")

# -------- CONFIGURATION --------
PROCESS_ALL     = False
OUTPUT_HEADER   = "DCSBIOSBridgeData.h"
INPUT_REFERENCE = "InputMapping.h"
LED_REFERENCE   = "LEDMapping.h"
KNOWN_DEVICES   = {
    "GPIO",
    "GAUGE",
    "PCA9555",
    "TM1637",
    "GN1640T",
    "WS2812",
    "NONE",
}

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

# --- Auto-detect valid panel JSON file ---
json_files = [f for f in os.listdir() if f.lower().endswith('.json')]
valid_jsons = []
for fname in json_files:
    try:
        with open(fname, "r", encoding="utf-8") as f:
            data = json.load(f)
        if is_valid_panel_json(data):
            valid_jsons.append((fname, data))
    except Exception:
        continue  # skip bad files

if len(valid_jsons) == 0:
    print("ERROR: No valid panel-style JSON file found in current directory.")
    input("\nPress <ENTER> to exit...")
    sys.exit(1)
if len(valid_jsons) > 1:
    print(f"ERROR: Multiple valid panel-style JSON files found: {[f[0] for f in valid_jsons]}")
    input("\nPress <ENTER> to exit...")
    sys.exit(1)

JSON_FILE, data = valid_jsons[0]
merge_metadata_jsons(data)

# At the top, after valid_jsons[0] is set
JSON_FILE, data = valid_jsons[0]
ACFT_NAME = os.path.splitext(os.path.basename(JSON_FILE))[0]

# Always (re-)generate panels.txt from current data
with open("panels.txt", "w", encoding="utf-8") as f:
    for panel in data.keys():
        f.write(panel + "\n")

PANELS_REFERENCE_FILE = "panels.txt"
PANELS_SELECTED_FILE = "selected_panels.txt"

if PROCESS_ALL:
    target_objects = set(data.keys())
else:
    # Warn + create template if selected_panels.txt does not exist
    if not os.path.isfile(PANELS_SELECTED_FILE):
        print(f"WARNING: '{PANELS_SELECTED_FILE}' does not exist. This usually means this is your first run.")
        print(f"Creating '{PANELS_SELECTED_FILE}' template...")

        with open(PANELS_SELECTED_FILE, "w", encoding="utf-8") as sf:
            sf.write("# selected_panels.txt - List panels to include, one per line.\n")
            sf.write("# To enable a panel, remove the leading '#'. Only panels listed in panels.txt are valid.\n")
            sf.write("# Example:\n")
            sf.write("# Master Arm Panel\n")
            sf.write("# Caution Light Panel\n")
            sf.write("#\n# To update the list of available panels, just re-run this script.\n\n")
            for panel in data.keys():
                sf.write(f"# {panel}\n")
        print(f"\nNow open '{PANELS_SELECTED_FILE}', uncomment the panels you want, and re-run this script.")
        input("\nPress <ENTER> to exit...")
        sys.exit(0)

    # Always reload the reference (panels.txt just regenerated)
    all_panels = load_panel_list(PANELS_REFERENCE_FILE)
    selected_panels = load_panel_list(PANELS_SELECTED_FILE)
    if not selected_panels:
        print("ERROR: No panels selected in selected_panels.txt.")
        input("\nPress <ENTER> to exit...")
        sys.exit(1)
    invalid_panels = selected_panels - all_panels
    if invalid_panels:
        print("\nERROR: The following panels in 'selected_panels.txt' are NOT present in 'panels.txt':")
        for panel in sorted(invalid_panels):
            print(f"  - {panel}")
        input("\nPress <ENTER> to exit...")
        sys.exit(1)
    target_objects = selected_panels

if PROCESS_ALL:
    print(f"\n⚠️  [WARNING] PROCESS_ALL is set to True! ALL panels in the JSON ({JSON_FILE}) will be processed.")
    print("    This mode is for PERFORMANCE TESTING or dev only.")
    print("    To process only panels listed in selected_panels.txt, set PROCESS_ALL = False in your script.\n")
else:
    print("\n[INFO] The current target_objects (panel(s) to be processed) are:")
    for panel in sorted(target_objects):
        print(f"  - {panel}")
    print("")
input("\nPress <ENTER> to continue and generate the files...")

# ——— HELPERS ———
def is_prime(n):
    if n < 2: return False
    if n % 2 == 0: return n == 2
    for i in range(3, int(n**0.5)+1, 2):
        if n % i == 0: return False
    return True

def next_prime(n):
    while not is_prime(n):
        n += 1
    return n

def djb2_hash(s, mod):
    h = 5381
    for c in s:
        h = ((h << 5) + h) + ord(c)
    return h % mod

# -------- LOAD JSON -------- You need to load the Hornet Json file in order to generate the file.
# with open(JSON_FILE, encoding='utf-8') as f:
    # data = json.load(f)

# -------- BUILD OUTPUT_ENTRIES (filtered) -------- THE LOGIC HERE IS SACRED, YOU DO NOT TOUCH NOT EVEN A COMMENT
control_type_map = {
    'led': 'CT_LED',
    'limited_dial': 'CT_ANALOG',
    'analog_gauge': 'CT_GAUGE',
    'selector': 'CT_SELECTOR',
    'toggle_switch': 'CT_SELECTOR',
    'action': 'CT_SELECTOR',
    'emergency_parking_brake': 'CT_SELECTOR',
    'display': 'CT_DISPLAY',
    'metadata': 'CT_METADATA'
}

output_entries = []
for panel, controls in data.items():
    if not PROCESS_ALL and panel not in target_objects:
        continue
    for key, item in controls.items():
        ctype_raw = item.get('control_type','').lower().strip()
        if ctype_raw not in control_type_map:
            continue
        outs = item.get('outputs', [])

        # ---- BEGIN PATCH for display string fields ----
        if ctype_raw == "display" and outs and "address" in outs[0] and "max_length" in outs[0]:
            base_addr = outs[0]['address']
            length = outs[0]['max_length']
            label = item.get('identifier', key)
            for i in range(length):
                output_entries.append({
                    'label':     label,
                    'addr':      base_addr + i,
                    'mask':      0xFF,
                    'shift':     0,
                    'max_value': length,
                    'controlType': 'CT_DISPLAY'
                })
            continue  # Skip default logic for these entries
        # ---- END PATCH for display string fields ----

        if not outs:
            continue
        out = outs[0]
        if not all(k in out for k in ('address','mask','shift_by')):
            continue
        output_entries.append({
            'label':     item.get('identifier', key),
            'addr':      out['address'],
            'mask':      out['mask'],
            'shift':     out['shift_by'],
            'max_value': out.get('max_value', 1),
            'controlType': control_type_map[ctype_raw]
        })

# -------- BUILD SELECTOR_ENTRIES (filtered) -------- THE LOGIC HERE IS SACRED, YOU DO NOT TOUCH NOT EVEN A COMMENT
selector_entries = []
groupCounter = 0
for panel, controls in data.items():
    if not PROCESS_ALL and panel not in target_objects:
        continue
    for ident, item in controls.items():
        ctype       = item.get('control_type','').lower().strip()
        api_variant = item.get('api_variant','').strip()
        lid         = ident.lower()
        desc_lower  = item.get('description','').lower()

        # skip analog gauges (but allow knobs)
        # if ctype in ('limited_dial','analog_dial','analog_gauge'):
        if ctype == 'analog_gauge':
            continue

        # find max_value
        max_val = None
        for inp in item.get('inputs', []):
            if inp.get('interface') == 'set_state' and 'max_value' in inp:
                max_val = inp['max_value']
                break
        if max_val is None or max_val < 0:
            continue

        if 'button' in desc_lower:
            selector_entries.append((ident, ident, 1, 'momentary', 0, "PRESS"))
            continue

        if lid.endswith('_cover') or lid.startswith('cover_') or '_cover_' in lid:
            selector_entries.append((ident, ident, 1, 'momentary', 0, "OPEN"))
            continue

        if api_variant == 'momentary_last_position':
            selector_entries.append((ident, ident, 1, 'momentary', 0, "PRESS"))
            continue

        # 4) discrete selectors: split labels
        count     = max_val + 1
        raw_desc  = item.get('description','')
        label_src = raw_desc.split(',',1)[1].strip() if ',' in raw_desc else raw_desc
        labels    = []
        useSlash  = False
        currentGroup = 0

        # 4.1) slash‑split
        if '/' in label_src:
            parts = [s.strip() for s in label_src.split('/')]
            if len(parts) == count:
                labels = parts
                useSlash = True
                groupCounter += 1
                currentGroup = groupCounter

        # 4.2) strict BTN/SW → PRESS/RELEASE
        if not labels and count == 2:
            if (lid.endswith('_btn') or lid.startswith('btn_') or '_btn_' in lid
                    or lid.endswith('_sw') or lid.startswith('sw_') or '_sw_' in lid):
                labels = ['PRESS','RELEASE']

        # 4.3) fallback → POS0, POS1, …
        if len(labels) != count:
            labels = [f"POS{i}" for i in range(count)]

        # 5) append (with reversed value and group for slash‑split selectors)
        if ctype in ('limited_dial', 'analog_dial'):
            # Analog input: use single label with type 'analog' and no group
            selector_entries.append((ident, ident, max_val, 'analog', 0, "LEVEL"))
        else:
            # 5) append as discrete selector/momentary
            for i, lab in enumerate(labels):
                clean = lab.upper().replace(' ','_')
                if useSlash:
                    val = (count - 1) - i
                    selector_entries.append((f"{ident}_{clean}", ident, val, ctype, currentGroup, clean))
                else:
                    selector_entries.append((f"{ident}_{clean}", ident, i, ctype, 0, clean))

# -------- WRITE HEADER FOR OUTPUT AND SELECTORS -------- 

with open(OUTPUT_HEADER, 'w', encoding='utf-8') as f:
    f.write("// Auto-generated DCSBIOS Bridge Data (JSON‑only) - DO NOT EDIT\n")
    f.write("#pragma once\n\n")

    # Store the Aicraft name
    f.write(f'#define DCSBIOS_ACFT_NAME "{ACFT_NAME}"\n')
    f.write(f'static constexpr const char* DCSBIOS_AIRCRAFT_NAME = "{ACFT_NAME}";\n\n')

    # Outputs
    f.write("enum ControlType : uint8_t {\n")
    f.write("  CT_LED,\n  CT_ANALOG,\n  CT_GAUGE,\n  CT_SELECTOR,\n  CT_DISPLAY,\n  CT_METADATA\n};\n\n")

    f.write("// --- Command History Tracking Struct ---\n")
    f.write("struct CommandHistoryEntry {\n")
    f.write("    const char*     label;\n")
    f.write("    uint16_t        lastValue;\n")
    f.write("    unsigned long   lastSendTime;\n")
    f.write("    bool            isSelector;\n")
    f.write("    uint16_t        group;\n")
    f.write("    uint16_t        pendingValue;\n")
    f.write("    unsigned long   lastChangeTime;\n")
    f.write("    bool            hasPending;\n")
    f.write("    uint8_t         lastReport[GAMEPAD_REPORT_SIZE];\n")
    f.write("    uint8_t         pendingReport[GAMEPAD_REPORT_SIZE];\n")
    f.write("    unsigned long   lastHidSendTime;\n")
    f.write("};\n\n")

    f.write("struct DcsOutputEntry { uint16_t addr, mask; uint8_t shift; uint16_t max_value; const char* label; ControlType controlType; };\n")
    f.write("static const DcsOutputEntry DcsOutputTable[] = {\n")
    for e in output_entries:
        f.write(f'    {{0x{e["addr"]:04X},0x{e["mask"]:04X},{e["shift"]},{e["max_value"]},"{e["label"]}",{e["controlType"]}}},\n')
    f.write("};\nstatic const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);\n\n")

    # Address→entries static flat table
    addr_map = defaultdict(list)
    for i, e in enumerate(output_entries):
        addr_map[e['addr']].append(i)

    f.write("// Static flat address-to-output entry lookup\n")
    f.write("struct AddressEntry {\n")
    f.write("  uint16_t addr;\n")

    # dynamically determine max entry count per address
    max_entries_per_addr = max(len(idxs) for idxs in addr_map.values())
    f.write(f"  const DcsOutputEntry* entries[{max_entries_per_addr}]; // max entries per address\n")
    f.write("  uint8_t count;\n")
    f.write("};\n\n")

    f.write("static const AddressEntry dcsAddressTable[] = {\n")
    for addr, idxs in addr_map.items():
        ptrs = ', '.join(f"&DcsOutputTable[{i}]" for i in idxs)
        count = len(idxs)
        f.write(f"  {{ 0x{addr:04X}, {{ {ptrs} }}, {count} }},\n")
    f.write("};\n\n")

    # Generate hash table for address → AddressEntry*
    address_entries = list(addr_map.items())
    desired_addr_count = len(address_entries) * 2
    ADDR_TABLE_SIZE = next_prime(max(desired_addr_count, 53))

    addr_hash_table = ["{0xFFFF, nullptr}"] * ADDR_TABLE_SIZE
    for idx, (addr, _) in enumerate(address_entries):
        h = addr % ADDR_TABLE_SIZE
        for probe in range(ADDR_TABLE_SIZE):
            if addr_hash_table[h] == "{0xFFFF, nullptr}":
                addr_hash_table[h] = f"{{ 0x{addr:04X}, &dcsAddressTable[{idx}] }}"
                break
            h = (h + 1) % ADDR_TABLE_SIZE
        else:
            print(f"❌ Address hash table full! ADDR_TABLE_SIZE={ADDR_TABLE_SIZE} too small", file=sys.stderr)
            sys.exit(1)

    # Emit hash table and lookup
    f.write("// Address hash entry\n")
    f.write("struct DcsAddressHashEntry {\n")
    f.write("  uint16_t addr;\n")
    f.write("  const AddressEntry* entry;\n")
    f.write("};\n\n")

    f.write(f"static const DcsAddressHashEntry dcsAddressHashTable[{ADDR_TABLE_SIZE}] = {{\n")
    for entry in addr_hash_table:
        f.write(f"  {entry},\n")
    f.write("};\n\n")

    f.write("// Simple address hash (modulo)\n")
    f.write("constexpr uint16_t addrHash(uint16_t addr) {\n")
    f.write(f"  return addr % {ADDR_TABLE_SIZE};\n")
    f.write("}\n\n")

    f.write("inline const AddressEntry* findDcsOutputEntries(uint16_t addr) {\n")
    f.write(f"  uint16_t startH = addrHash(addr);\n")
    f.write(f"  for (uint16_t i = 0; i < {ADDR_TABLE_SIZE}; ++i) {{\n")
    f.write(f"    uint16_t idx = (startH + i >= {ADDR_TABLE_SIZE}) ? (startH + i - {ADDR_TABLE_SIZE}) : (startH + i);\n")
    f.write("    const auto& entry = dcsAddressHashTable[idx];\n")
    f.write("    if (entry.addr == 0xFFFF) continue;\n")
    f.write("    if (entry.addr == addr) return entry.entry;\n")
    f.write("  }\n")
    f.write("  return nullptr;\n")
    f.write("}\n\n")

    # Selectors: add group field
    f.write("struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };\n")
    f.write("static const SelectorEntry SelectorMap[] = {\n")
    for full, cmd, val, ct, grp, lab in selector_entries:
        f.write(f'    {{ "{full}","{cmd}",{val},"{ct}",{grp},"{lab}" }},\n')

    f.write("};\nstatic const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);\n")

    # Build a flat list of all unique oride_labels and mark selectors with group > 0
    command_tracking = {}
    for full, cmd, val, ct, grp, lab in selector_entries:
        if cmd not in command_tracking:
            command_tracking[cmd] = (grp > 0, grp)

    # ——— EMIT THE UNIFIED COMMAND HISTORY TABLE WITH GROUP + BUFFER FIELDS + HID REPORT CACHE ———
    f.write("\n// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)\n")


    f.write("static CommandHistoryEntry commandHistory[] = {\n")
    for label, (is_selector, grp) in sorted(command_tracking.items()):
        # initialize all numeric fields to zero, booleans to false, arrays to {0}
        f.write(
            '    {{ "{label}", 0, 0, {sel}, {g}, 0,   0, false, '
            '{{0}}, {{0}}, 0 }},\n'.format(
                label=label,
                sel="true" if is_selector else "false",
                g=grp
            )
        )
    f.write("};\n")
    f.write("static const size_t commandHistorySize = sizeof(commandHistory)/sizeof(CommandHistoryEntry);\n")

    # --- START PATCH for displayFields[] ---

    display_field_defs = []
    for panel, controls in data.items():
        if not PROCESS_ALL and panel not in target_objects:
            continue
        for key, item in controls.items():
            ctype_raw = item.get('control_type','').lower().strip()
            if ctype_raw == 'display':
                outs = item.get('outputs', [])
                if outs and 'address' in outs[0] and 'max_length' in outs[0]:
                    display_field_defs.append({
                        'panel': panel.replace('"', '\\"'),  # Escape quotes for C++
                        'label': item.get('identifier', key),
                        'base_addr': outs[0]['address'],
                        'length': outs[0]['max_length']
                    })

    # Only emit the struct and array ONCE, after the loop!
    f.write("\n// --- Auto-generated display string field grouping ---\n")
    f.write("struct DisplayFieldDef {\n")
    f.write("    const char* panel;\n")
    f.write("    const char* label;\n")
    f.write("    uint16_t base_addr;\n")
    f.write("    uint8_t  length;\n")
    f.write("};\n")

    # Only emit array if you have any fields, but always emit array definition (even if empty)
    if display_field_defs:
        f.write("static const DisplayFieldDef displayFields[] = {\n")
        for d in display_field_defs:
            f.write('    { "%s", "%s", 0x%04X, %d },\n' % (d['panel'], d['label'], d['base_addr'], d['length']))
        f.write("};\n")
    else:
        f.write("static const DisplayFieldDef displayFields[] = {\n};\n")

    f.write("static constexpr size_t numDisplayFields = sizeof(displayFields)/sizeof(displayFields[0]);\n\n")

    # --- END  ---

    # --- Emit hash table for displayFieldsByLabel (label→DisplayFieldDef*) ---
    labels = [d['label'] for d in display_field_defs]
    desired_size = next_prime(len(labels) * 2)
    label_hash_table = ["{nullptr, nullptr}"] * desired_size

    for idx, label in enumerate(labels):
        h = djb2_hash(label, desired_size)
        for probe in range(desired_size):
            if label_hash_table[h] == "{nullptr, nullptr}":
                label_hash_table[h] = f'{{"{label}", &displayFields[{idx}]}}'
                break
            h = (h + 1) % desired_size
        else:
            print(f"❌ Hash table full for displayFieldsByLabel", file=sys.stderr)
            sys.exit(1)

    f.write(f"struct DisplayFieldHashEntry {{ const char* label; const DisplayFieldDef* def; }};\n")
    f.write(f"static const DisplayFieldHashEntry displayFieldsByLabel[{desired_size}] = {{\n")
    for entry in label_hash_table:
        f.write(f"  {entry},\n")
    f.write("};\n\n")

    f.write("// Shared recursive hash implementation for display label lookup\n")
    f.write("constexpr uint16_t labelHash(const char* s);\n\n")

    f.write("inline const DisplayFieldDef* findDisplayFieldByLabel(const char* label) {\n")
    f.write(f"  uint16_t startH = labelHash(label) % {desired_size};\n")
    f.write(f"  for (uint16_t i = 0; i < {desired_size}; ++i) {{\n")
    f.write(f"    uint16_t idx = (startH + i >= {desired_size}) ? (startH + i - {desired_size}) : (startH + i);\n")
    f.write("    const auto& entry = displayFieldsByLabel[idx];\n")
    f.write("    if (!entry.label) continue;\n")
    f.write("    if (strcmp(entry.label, label) == 0) return entry.def;\n")
    f.write("  }\n")
    f.write("  return nullptr;\n")
    f.write("}\n\n")

    # --- Auto-generated tracked metadata fields ---
    metadata_entries = []
    for panel, controls in data.items():
        if not PROCESS_ALL and panel not in target_objects:
            continue
        for key, item in controls.items():
            ctype_raw = item.get('control_type','').lower().strip()
            if ctype_raw == 'metadata':
                metadata_entries.append({
                    'label': item.get('identifier', key),
                    'panel': panel,
                    'desc': item.get('description', '')
                })

    if metadata_entries:
        f.write("\n// --- Auto-generated tracked metadata fields ---\n")
        f.write("struct MetadataState {\n")
        f.write("    const char* label;\n")
        f.write("    uint16_t    value;\n")
        f.write("};\n")
        f.write("static MetadataState metadataStates[] = {\n")
        for entry in metadata_entries:
            comment = entry['desc'].replace('\"', '\\\"')
            f.write(f'    {{ "{entry["label"]}", 0 }}, // {entry["panel"]}: {comment}\n')
        f.write("};\n")
        f.write("static const size_t numMetadataStates = sizeof(metadataStates)/sizeof(metadataStates[0]);\n\n")

        # --- Hash table for instant lookup ---
        def djb2_hash(s, mod):
            h = 5381
            for c in s:
                h = ((h << 5) + h) + ord(c)
            return h % mod

        desired_size = next_prime(len(metadata_entries) * 2)
        hash_table = ["{nullptr, nullptr}"] * desired_size

        for idx, entry in enumerate(metadata_entries):
            label = entry['label']
            h = djb2_hash(label, desired_size)
            for probe in range(desired_size):
                if hash_table[h] == "{nullptr, nullptr}":
                    hash_table[h] = f'{{"{label}", &metadataStates[{idx}]}}'
                    break
                h = (h + 1) % desired_size
            else:
                print(f"❌ Hash table full for metadataStates", file=sys.stderr)
                sys.exit(1)

        f.write(f"struct MetadataHashEntry {{ const char* label; MetadataState* state; }};\n")
        f.write(f"static MetadataHashEntry metadataHashTable[{desired_size}] = {{\n")
        for entry in hash_table:
            f.write(f"  {entry},\n")
        f.write("};\n\n")
        f.write("constexpr uint16_t metadataHash(const char* s) { return labelHash(s); }\n\n")
        f.write("inline MetadataState* findMetadataState(const char* label) {\n")
        f.write(f"    uint16_t startH = metadataHash(label) % {desired_size};\n")
        f.write(f"    for (uint16_t i = 0; i < {desired_size}; ++i) {{\n")
        f.write(f"        uint16_t idx = (startH + i >= {desired_size}) ? (startH + i - {desired_size}) : (startH + i);\n")
        f.write( "        const auto& entry = metadataHashTable[idx];\n")
        f.write( "        if (!entry.label) continue;\n")
        f.write( "        if (strcmp(entry.label, label) == 0) return entry.state;\n")
        f.write( "    }\n")
        f.write( "    return nullptr;\n")
        f.write("}\n")
    else:
        f.write("// No tracked metadata fields found\n")
        f.write("struct MetadataState { const char* label; uint16_t value; };\n")
        f.write("static MetadataState metadataStates[] = {};\n")
        f.write("static const size_t numMetadataStates = 0;\n")
        f.write("inline MetadataState* findMetadataState(const char*) { return nullptr; }\n")

print(f"[✓] Generated {OUTPUT_HEADER} with "
      f"{len(output_entries)} outputs,  "
      f"{len(selector_entries)} selectors.")

# /////////////////////////////////////// EXTRA FIELDS //////////////////////////////////////////////

# Extra Fields for CT_Display.h *** REMEMBER TO DOUBLE ESCAPE *** 
CT_DISPLAY_H_EXTRA = '''

// --- Forward declarations for opaque driver pointer types ---
typedef void* DisplayDriverPtr;

// --- Forward declaration for field def struct (real type in DisplayMapping.h) ---
struct DisplayFieldDefLabel;
struct FieldState;

void renderField(const char* label, const char* value, const DisplayFieldDefLabel* defOverride = nullptr, FieldState* stateOverride = nullptr);

'''

# Extra Fields for CT_Display.cpp *** REMEMBER TO DOUBLE ESCAPE *** 
CT_DISPLAY_CPP_EXTRA = '''

void renderField(const char* label, const char* strValue, const DisplayFieldDefLabel* defOverride, FieldState* stateOverride) {
    const DisplayFieldDefLabel* def = defOverride ? defOverride : findFieldDefByLabel(label);

    if (!def) return;

    if (!def->renderFunc) {
        debugPrintf("[DISPLAY] No renderFunc for label '%s', skipping\\n", def->label);
        return;
    }

    FieldState* state = stateOverride;
    int idx = 0;

    if (!stateOverride) {
        idx = def - fieldDefs;
        state = &fieldStates[idx];
    }

    bool valid = true;
    if (def->type == FIELD_NUMERIC) {
        int value = strToIntFast(strValue);
        valid = (value >= def->minValue && value <= def->maxValue);
    }

    const char* toShow = valid ? strValue : state->lastValue;

    if (memcmp(toShow, state->lastValue, sizeof(state->lastValue)) == 0)
        return;

    memcpy(state->lastValue, toShow, sizeof(state->lastValue));
    if (def->clearFunc) def->clearFunc(def->driver, def->segMap, *def);

    // Call the render function with the appropriate parameters
    def->renderFunc(def->driver, def->segMap, toShow, *def);
}

'''


# /////////////////////////////////////// EXTRA FIELDS //////////////////////////////////////////////

# Build a global list of all display field labels/lengths
all_display_labels = [(d['label'], d['length']) for d in display_field_defs]

with open("CT_Display.h", "w", encoding="utf-8") as fout:
    fout.write("// Auto-generated — DO NOT EDIT\n")
    fout.write("#pragma once\n\n")
    fout.write("// Buffers and dirty flags for all display fields (global)\n")
    for label, length in all_display_labels:
        buf_var = label.lower()
        fout.write(f"extern char {buf_var}[{length+1}];\n")
        fout.write(f"extern bool {buf_var}_dirty;\n")
        fout.write(f"extern char last_{buf_var}[{length+1}];\n")
    fout.write("\nextern DisplayBufferEntry CT_DisplayBuffers[];\n")
    fout.write("extern const size_t numCTDisplayBuffers;\n")
    fout.write("extern const DisplayBufferHashEntry CT_DisplayBufferHash[];\n")
    fout.write("const DisplayBufferEntry* findDisplayBufferByLabel(const char* label);\n")

    # Extra fields for .h file
    fout.write(CT_DISPLAY_H_EXTRA)


with open("CT_Display.cpp", "w", encoding="utf-8") as fcpp:
    fcpp.write('#include "../../Globals.h"\n')
    fcpp.write('#include "CT_Display.h"\n')
    fcpp.write('#include "DisplayMapping.h"\n\n')

    for label, length in all_display_labels:
        buf_var = label.lower()
        fcpp.write(f"char {buf_var}[{length+1}] = {{}};\n")
        fcpp.write(f"bool {buf_var}_dirty = false;\n")
        fcpp.write(f"char last_{buf_var}[{length+1}] = {{}};\n")
    fcpp.write("\nDisplayBufferEntry CT_DisplayBuffers[] = {\n")
    for label, length in all_display_labels:
        buf_var = label.lower()
        fcpp.write(f'    {{ "{label}", {buf_var}, {length}, &{buf_var}_dirty, last_{buf_var} }},\n')
    fcpp.write("};\n")
    fcpp.write("const size_t numCTDisplayBuffers = sizeof(CT_DisplayBuffers)/sizeof(CT_DisplayBuffers[0]);\n")

    # Hash table for fast label->buffer lookup (just like before)
    desired_size = next_prime(len(all_display_labels) * 2)
    hash_table = ["{nullptr, nullptr}"] * desired_size
    for idx, (label, _) in enumerate(all_display_labels):
        h = djb2_hash(label, desired_size)
        for probe in range(desired_size):
            if hash_table[h] == "{nullptr, nullptr}":
                hash_table[h] = f'{{"{label}", &CT_DisplayBuffers[{idx}]}}'
                break
            h = (h + 1) % desired_size
        else:
            print(f"❌ Hash table full for CT_DisplayBufferHash", file=sys.stderr)
            sys.exit(1)
    fcpp.write(f"\nconst DisplayBufferHashEntry CT_DisplayBufferHash[{desired_size}] = {{\n")
    for entry in hash_table:
        fcpp.write(f"    {entry},\n")
    fcpp.write("};\n\n")

    # Fast hash lookup function (O(1), collision-resilient)
    fcpp.write("const DisplayBufferEntry* findDisplayBufferByLabel(const char* label) {\n")
    fcpp.write(f"    uint16_t startH = labelHash(label) % {desired_size};\n")
    fcpp.write(f"    for (uint16_t i = 0; i < {desired_size}; ++i) {{\n")
    fcpp.write(f"        uint16_t idx = (startH + i >= {desired_size}) ? (startH + i - {desired_size}) : (startH + i);\n")
    fcpp.write( "        const auto& entry = CT_DisplayBufferHash[idx];\n")
    fcpp.write( "        if (!entry.label) continue;\n")
    fcpp.write( "        if (strcmp(entry.label, label) == 0) return entry.entry;\n")
    fcpp.write( "    }\n")
    fcpp.write( "    return nullptr;\n")
    fcpp.write("}\n")

    # Extra fields for .cpp file
    fcpp.write(CT_DISPLAY_CPP_EXTRA)

import re
# --- BUILD panel-to-display map from JSON ---
display_panel_map = {}
for panel, controls in data.items():
    for key, item in controls.items():
        ctype_raw = item.get('control_type','').lower().strip()
        if ctype_raw == 'display':
            outs = item.get('outputs', [])
            if outs and 'address' in outs[0] and 'max_length' in outs[0]:
                label = item.get('identifier', key)
                length = outs[0]['max_length']
                # Use the prefix before first '_' if present, else panel name
                m = re.match(r'^([A-Z0-9]+)_', label)
                prefix = m.group(1) if m else panel.replace(' ', '_').replace('/', '_')
                if prefix not in display_panel_map:
                    display_panel_map[prefix] = []
                display_panel_map[prefix].append((label, length))

# -------- WRITE InputMapping.h from selector_entries above (label-keyed preserve) --------

# 1) load existing entries, keyed by label
existing_map = {}
line_re = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*'        # capture the label
    r'"(?P<source>[^"]+)"\s*,\s*'
    r'(?P<port>-?\d+)\s*,\s*'
    r'(?P<bit>-?\d+)\s*,\s*'
    r'(?P<hidId>-?\d+)\s*,\s*'
    r'"(?P<cmd>[^"]+)"\s*,\s*'
    r'(?P<value>-?\d+)\s*,\s*'
    r'"(?P<type>[^"]+)"\s*,\s*'
    r'(?P<group>\d+)\s*\}\s*,'
)
if os.path.exists(INPUT_REFERENCE):
    with open(INPUT_REFERENCE, "r", encoding="utf-8") as fin:
        for line in fin:
            m = line_re.search(line)
            if not m:
                continue
            d = m.groupdict()
            existing_map[d["label"]] = {
                "label":       d["label"],
                "source":      d["source"],
                "port":        int(d["port"]),
                "bit":         int(d["bit"]),
                "hidId":       int(d["hidId"]),
                "oride_label": d["cmd"],
                "oride_value": int(d["value"]),
                "controlType": d["type"],
                "group":       int(d["group"])
            }

# --- TEMP: Strip lab for InputMapping merge ---
selector_entries_inputmap = [
    (full, cmd, val, ct, grp) for (full, cmd, val, ct, grp, lab) in selector_entries
]

# 2) merge into a new list, preserving any user edits by label
merged = []
for full, cmd, val, ct, grp in selector_entries_inputmap:
    if full in existing_map:
        e = existing_map[full]
        merged.append((
            e["label"], e["source"], e["port"], e["bit"],
            e["hidId"], e["oride_label"], e["oride_value"],
            e["controlType"], e["group"]
        ))
    else:
        # brand-new entry: use defaults
        merged.append((full, "PCA_0x00", 0, 0, -1, cmd, val, ct, grp))

# 3) write out merged list
with open(INPUT_REFERENCE, "w", encoding="utf-8") as f2:
    input_labels = [e[0] for e in merged]
    f2.write("// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL RECORDS, DO NOT ADD OR DELETE THEM HERE\n")
    f2.write("#pragma once\n\n")
    f2.write("struct InputMapping {\n")
    f2.write("    const char* label;        // Unique selector label\n")
    f2.write("    const char* source;       // Hardware source identifier\n")
    f2.write("    int8_t     port;         // Port index\n")
    f2.write("    int8_t     bit;          // Bit position\n")
    f2.write("    int8_t      hidId;        // HID usage ID\n")
    f2.write("    const char* oride_label;  // Override command label (dcsCommand)\n")
    f2.write("    uint16_t    oride_value;  // Override command value (value)\n")
    f2.write("    const char* controlType;  // Control type, e.g., \"selector\"\n")
    f2.write("    uint16_t    group;        // Group ID for exclusive selectors\n")
    f2.write("};\n\n")
    f2.write('//  label                       source     port bit hidId  DCSCommand           value   Type        group\n')

    if not merged:
        f2.write("static const InputMapping InputMappings[] = {\n};\n")
    else:
        f2.write("static const InputMapping InputMappings[] = {\n")
        max_label = max(len(e[0]) for e in merged)
        max_cmd   = max(len(e[5]) for e in merged)
        max_type  = max(len(e[7]) for e in merged)

        for lbl, src, port, bit, hid, cmd, val, typ, gp in merged:
            lblf = f'"{lbl}"'.ljust(max_label+2)
            cmdf = f'"{cmd}"'.ljust(max_cmd+2)
            ctf  = f'"{typ}"'.ljust(max_type+2)
            # Format override value safely
            val_str = f"0xFFFF" if val > 32767 else f"{val:>5}"
            f2.write(f'    {{ {lblf}, "{src}" , {port:>2} , {bit:>2} , {hid:>3} , '
                 f'{cmdf}, {val_str} , {ctf}, {gp:>2} }},\n')

        f2.write("};\n")

    f2.write("static const size_t InputMappingSize = sizeof(InputMappings)/sizeof(InputMappings[0]);\n\n")

    # --- TrackedSelectorLabels[] auto-gen ---
    tracked_labels = set()
    for lbl, src, port, bit, hid, oride_label, oride_value, typ, group in merged:
        if typ == "selector" and int(group) > 0:
            tracked_labels.add(oride_label)
    if tracked_labels:
        f2.write("// Auto-generated: selector DCS labels with group > 0 (panel sync)\n")
        f2.write("static const char* const TrackedSelectorLabels[] = {\n")
        for lbl in sorted(tracked_labels):
            f2.write(f'    "{lbl}",\n')
        f2.write("};\n")
        f2.write("static const size_t TrackedSelectorLabelsCount = sizeof(TrackedSelectorLabels)/sizeof(TrackedSelectorLabels[0]);\n\n")
    else:
        f2.write("static const char* const TrackedSelectorLabels[] = {};\n")
        f2.write("static const size_t TrackedSelectorLabelsCount = 0;\n\n")

    # 4) Generate static hash table for InputMappings[]
    desired = len(input_labels) * 2
    INPUT_TABLE_SIZE = next_prime(max(desired, 53))

    input_hash_table = ["{nullptr, nullptr}"] * INPUT_TABLE_SIZE

    for idx, label in enumerate(input_labels):
        h = djb2_hash(label, INPUT_TABLE_SIZE)
        for probe in range(INPUT_TABLE_SIZE):
            if input_hash_table[h] == "{nullptr, nullptr}":
                input_hash_table[h] = f'{{"{label}", &InputMappings[{idx}]}}'
                break
            h = (h + 1) % INPUT_TABLE_SIZE
        else:
            print(f"❌ Input hash table full! TABLE_SIZE={INPUT_TABLE_SIZE} too small", file=sys.stderr)
            sys.exit(1)

    f2.write(f"\n// Static hash lookup table for InputMappings[]\n")
    f2.write("struct InputHashEntry { const char* label; const InputMapping* mapping; };\n")
    f2.write(f"static const InputHashEntry inputHashTable[{INPUT_TABLE_SIZE}] = {{\n")
    for entry in input_hash_table:
        f2.write(f"  {entry},\n")
    f2.write("};\n\n")

    f2.write("// Shared recursive hash implementation for display label lookup\n")
    f2.write("constexpr uint16_t labelHash(const char* s);\n")
    f2.write("\n\n")

    # f2.write("// Shared recursive hash implementation\n")
    # f2.write("constexpr uint16_t labelHash(const char* s) {\n")
    # f2.write("  return *s ? static_cast<uint16_t>(*s) + 31 * labelHash(s + 1) : 0;\n")
    # f2.write("}\n\n")

    f2.write("// Preserve existing signature\n")
    f2.write("constexpr uint16_t inputHash(const char* s) { return labelHash(s); }\n\n")

    f2.write("inline const InputMapping* findInputByLabel(const char* label) {\n")
    f2.write(f"  uint16_t startH = inputHash(label) % {INPUT_TABLE_SIZE};\n")
    f2.write(f"  for (uint16_t i = 0; i < {INPUT_TABLE_SIZE}; ++i) {{\n")
    f2.write(f"    uint16_t idx = (startH + i >= {INPUT_TABLE_SIZE}) ? (startH + i - {INPUT_TABLE_SIZE}) : (startH + i);\n")
    f2.write("    const auto& entry = inputHashTable[idx];\n")
    f2.write("    if (!entry.label) continue;\n")
    f2.write("    if (strcmp(entry.label, label) == 0) return entry.mapping;\n")
    f2.write("  }\n")
    f2.write("  return nullptr;\n")
    f2.write("}\n")

print(f"[✓] Generated {INPUT_REFERENCE} with {len(merged)} entries.")

# --------------------------- Generate our LEDMappings.h file -----------------------------------------------

def generate_comment(device, info_type, info_values):
    info = [x.strip() for x in info_values.split(",")]
    if device == "GPIO" and len(info) >= 1:
        return f"// GPIO {info[0]}"
    if device == "GAUGE" and len(info) >= 1:
        return f"// GAUGE GPIO {info[0]}"
    if device == "PCA9555" and len(info) >= 3:
        return f"// PCA {info[0]} Port {info[1]} Bit {info[2]}"
    if device == "TM1637" and len(info) >= 4:
        return f"// TM1637 CLK {info[0]} DIO {info[1]} Seg {info[2]} Bit {info[3]}"
    if device == "GN1640T" and len(info) >= 3:
        return f"// GN1640 Addr {info[0]} Col {info[1]} Row {info[2]}"
    if device == "WS2812" and len(info) >= 1:
        return f"// WS2812 Index {info[0]}"
    return "// No Info"

# ——— 1) PARSE existing LEDControl_Lookup.h (if any) ———
existing = {}
if os.path.exists(LED_REFERENCE):
    with open(LED_REFERENCE, "r", encoding="utf-8") as f:
        txt = f.read()

    entry_re = re.compile(
        r'\{\s*"(?P<label>[^"]+)"\s*,\s*DEVICE_(?P<device>[A-Z0-9_]+)\s*,'
        r'\s*\{\s*\.(?P<info_type>[a-zA-Z0-9_]+)\s*=\s*\{\s*(?P<info_values>[^}]+)\}\s*\}'
        r'\s*,\s*(?P<dimmable>true|false)\s*,\s*(?P<activeLow>true|false)\s*\}',
        re.MULTILINE
    )

    for m in entry_re.finditer(txt):
        d = m.groupdict()
        existing[d["label"]] = {
            "device":      d["device"],
            "info_type":   d["info_type"],
            "info_values": d["info_values"].strip(),
            "dimmable":    d["dimmable"],
            "activeLow":   d["activeLow"],
        }


# ——— 2) PARSE DcsOutputTable for all labels (only CT_LED, CT_GAUGE) ———
if not os.path.exists(OUTPUT_HEADER):
    print(f"❌ Cannot find `{OUTPUT_HEADER}` – adjust OUTPUT_HEADER path.", file=sys.stderr)
    sys.exit(1)

with open(OUTPUT_HEADER, "r", encoding="utf-8") as f:
    dcs = f.read()

dcs_re = re.compile(
    r'\{\s*0x[0-9A-Fa-f]+\s*,\s*0x[0-9A-Fa-f]+\s*,\s*\d+\s*,\s*\d+\s*,\s*"([^"]+)"\s*,\s*(CT_\w+)\s*\}'
)

labels = []
for m in dcs_re.finditer(dcs):
    label = m.group(1)
    control_type = m.group(2)
    if control_type in ("CT_LED", "CT_GAUGE", "CT_ANALOG"):
        labels.append(label)

# Duplicate label detection
if len(labels) != len(set(labels)):
    print("⚠️ WARNING: Duplicate labels detected in DCS table!", file=sys.stderr)

print(f"✅ Found {len(labels)} LED/Analog labels in DcsOutputTable ({OUTPUT_HEADER})")

# ——— 3) Compute table size dynamically (load ≤ 50%) ———
desired = len(labels) * 2
TABLE_SIZE = next_prime(max(desired, 53))
print(f"🔢 Using TABLE_SIZE = {TABLE_SIZE} (next prime ≥ {desired})")

# ——— 4) Build the new panelLEDs[] lines ———
# for clean alignment, find max label width
max_label_len = max(len(label) for label in labels) if labels else 0

final_panel = []
for label in labels:
    padded_label = f'"{label}"'.ljust(max_label_len+2)
    if label in existing:
        e = existing[label]
        dev = e["device"]
        if dev not in KNOWN_DEVICES:
            print(f"⚠️  WARNING: DEVICE_{dev} on `{label}` not in KNOWN_DEVICES")
        line = (
            f'  {{ {padded_label}, DEVICE_{dev.ljust(8)}, '
            f'{{.{e["info_type"]} = {{{e["info_values"]}}}}}, '
            f'{e["dimmable"]}, {e["activeLow"]} }}, {generate_comment(dev, e["info_type"], e["info_values"])}'
        )
    else:
        line = (
            f'  {{ {padded_label}, DEVICE_NONE    , '
            f'{{.gpioInfo = {{0}}}}, false, false }}'
        )
    final_panel.append(line)

# ——— 5) Build & probe the hash table ———
hash_table = ["{nullptr, nullptr}"] * TABLE_SIZE
for idx, label in enumerate(labels):
    h = djb2_hash(label, TABLE_SIZE)
    for probe in range(TABLE_SIZE):
        if hash_table[h] == "{nullptr, nullptr}":
            hash_table[h] = f'{{"{label}", &panelLEDs[{idx}]}}'
            break
        h = (h + 1) % TABLE_SIZE
    else:
        print(f"❌ Hash table full! TABLE_SIZE={TABLE_SIZE} too small", file=sys.stderr)
        sys.exit(1)

# ——— 6) Emit the new LEDMapping.h ———
with open(LED_REFERENCE, "w", encoding="utf-8") as out:
    out.write("// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL LED/GAUGE RECORDS, DO NOT ADD OR DELETE THEM HERE\n")
    out.write("#pragma once\n\n")

    out.write("// Embedded LEDMapping structure and enums\n")
    out.write("enum LEDDeviceType {\n")
    for dev in KNOWN_DEVICES:
        out.write(f"  DEVICE_{dev},\n")
    out.write("};\n\n")

    out.write("struct LEDMapping {\n")
    out.write("  const char* label;\n")
    out.write("  LEDDeviceType deviceType;\n")
    out.write("  union {\n")
    out.write("    struct { uint8_t gpio; } gpioInfo;\n")
    # out.write("    struct { uint8_t gauge; } gaugeInfo;\n")
    out.write("    struct { uint8_t gpio; uint16_t minPulse; uint16_t maxPulse; uint16_t period; } gaugeInfo;\n")
    out.write("    struct { uint8_t address; uint8_t port; uint8_t bit; } pcaInfo;\n")
    out.write("    struct { uint8_t clkPin; uint8_t dioPin; uint8_t segment; uint8_t bit; } tm1637Info;\n")
    out.write("    struct { uint8_t address; uint8_t column; uint8_t row; } gn1640Info;\n")
    out.write("    struct { uint8_t index; } ws2812Info;\n")
    out.write("  } info;\n")
    out.write("  bool dimmable;\n")
    out.write("  bool activeLow;\n")
    out.write("};\n\n")

    out.write("// Auto-generated panelLEDs array\n")
    out.write("static const LEDMapping panelLEDs[] = {\n")
    out.write(",\n".join(final_panel))
    out.write("\n};\n\n")

    out.write("static constexpr uint16_t panelLEDsCount = sizeof(panelLEDs)/sizeof(panelLEDs[0]);\n\n")

    out.write("// Auto-generated hash table\n")
    out.write("struct LEDHashEntry { const char* label; const LEDMapping* led; };\n")
    out.write(f"static const LEDHashEntry ledHashTable[{TABLE_SIZE}] = {{\n")
    for entry in hash_table:
        out.write(f"  {entry},\n")
    out.write("};\n\n")

    out.write("// Reuse shared recursive hash implementation\n")
    out.write("constexpr uint16_t ledHash(const char* s) { return labelHash(s); }\n\n")

    out.write("// findLED lookup\n")
    out.write("inline const LEDMapping* findLED(const char* label) {\n")
    out.write(f"  uint16_t startH = ledHash(label) % {TABLE_SIZE};\n")
    out.write(f"  for (uint16_t i = 0; i < {TABLE_SIZE}; ++i) {{\n")
    out.write(f"    uint16_t idx = (startH + i >= {TABLE_SIZE}) ? (startH + i - {TABLE_SIZE}) : (startH + i);\n")
    out.write("    const auto& entry = ledHashTable[idx];\n")
    out.write("    if (!entry.label) continue;\n")
    out.write("    if (strcmp(entry.label, label) == 0) return entry.led;\n")
    out.write("  }\n")
    out.write("  return nullptr;\n")
    out.write("}\n")

print(f"✅ {LED_REFERENCE} fully regenerated with dynamic enum from KNOWN_DEVICES.")

# Cleaner so that only one set of .cpp files exists
WHITELIST = ['CT_Display.cpp', 'DisplayMapping.cpp']
def print_and_disable_cpp_files():
    current_dir = os.path.abspath(os.path.dirname(__file__))
    current_dir_name = os.path.basename(current_dir)
    parent_dir = os.path.dirname(current_dir)
    parent_dir_name = os.path.basename(parent_dir)

    # CLEANUP: In current dir only, remove *.cpp.DISABLED only if *.cpp exists
    for f in os.listdir(current_dir):
        if f.endswith('.cpp.DISABLED'):
            cpp_name = f[:-9]  # remove '.DISABLED'
            if cpp_name.endswith('.cpp') and cpp_name in os.listdir(current_dir):
                try:
                    os.remove(os.path.join(current_dir, f))
                    # print(f"Cleaned: {f} (since {cpp_name} exists)")
                except Exception as e:
                    print(f"Error cleaning {f}: {e}")

    # SAFETY CHECK: must be inside a LABELS directory
    if parent_dir_name != "LABELS":
        print("ERROR: This script must be run from a subdirectory of 'LABELS'!")
        input("\nPress <ENTER> to exit...")
        return

    for entry in os.listdir(parent_dir):
        entry_path = os.path.join(parent_dir, entry)
        if os.path.isdir(entry_path) and entry != current_dir_name:
            cpp_files = [f for f in os.listdir(entry_path)
                         if os.path.isfile(os.path.join(entry_path, f)) 
                         and f.endswith('.cpp') 
                         and f in WHITELIST]
            if cpp_files:
                for f in cpp_files:
                    src = os.path.join(entry_path, f)
                    dst = src + ".DISABLED"
                    # If .DISABLED exists, remove it first
                    if os.path.exists(dst):
                        try:
                            os.remove(dst)
                        except Exception as e:
                            print(f"    Could not remove existing {os.path.basename(dst)}: {e}")
                            continue
                    try:
                        os.rename(src, dst)
                    except Exception as e:
                        print(f"    {f} -> ERROR: {e}")


# --- CALL DISPLAY GENERATOR SCRIPT ---
try:
    script_dir = os.path.dirname(os.path.abspath(__file__))
    display_gen_path = os.path.join(script_dir, "display_gen.py")
    print("\n[INFO] Generating DisplayMapping.cpp/h using display_gen.py ...")
    subprocess.run([sys.executable, display_gen_path], cwd=script_dir, check=True)
except Exception as e:
    print(f"❌ Error running display_gen.py: {e}")
    input("\nPress <ENTER> to exit...")
    sys.exit(1)

print_and_disable_cpp_files()
print(f"✅ Renamed .cpp files to cpp.DISABLE in the inactive LABEL SETS to avoid linker conflicts")