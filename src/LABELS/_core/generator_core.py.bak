#!/usr/bin/env python3
"""
CockpitOS Generator - Core Module
==================================
Called from LABEL_SET_XXX/generate_data.py wrappers.
CWD is set by the wrapper BEFORE this module is imported.
"""
import os, sys, json, re, hashlib
import subprocess
from collections import defaultdict

# Ensure stdout/stderr can handle Unicode (✓, ✅, 🔢, etc.) even when
# piped or redirected on Windows (where the default codec may be cp1252).
if sys.stdout.encoding and sys.stdout.encoding.lower().replace("-", "") != "utf8":
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

# When launched from the compiler tool, skip all interactive pauses.
_BATCH = os.environ.get("COCKPITOS_BATCH") == "1"

def _pause(msg="\nPress <ENTER> to exit..."):
    if not _BATCH:
        input(msg)

def run():

    # Get our current working LABEL SET directory
    current_dir = os.path.abspath(os.getcwd())
    current_label_set = os.path.basename(current_dir)

    # Always extract _ls_name (stripped LABEL_SET_ prefix or as-is)
    _ls_name = current_label_set[len("LABEL_SET_"):] if current_label_set.startswith("LABEL_SET_") else current_label_set

    print(f"Current LABEL SET: {current_label_set}")

    # -------- CONFIGURATION --------
    FIXED_STEP_INCDEC_THRESHOLD = 2 # Selectors with more than FIXED_STEP_INCDEC_THRESHOLD + 1 positions get an INC and DEC pseudo label (to use only 2 GPIOs)
    PROCESS_ALL     = False
    OUTPUT_HEADER   = "DCSBIOSBridgeData.h"
    INPUT_REFERENCE = "InputMapping.h"
    LED_REFERENCE   = "LEDMapping.h"
    KNOWN_DEVICES   = [
        "GPIO",
        "GAUGE",
        "PCA9555",
        "TM1637",
        "GN1640T",
        "WS2812",
        "NONE",
    ]

    # Types that should behave like selectors in INPUTS
    NORMALIZE_TO_SELECTOR = {
        'action',
        'toggle_switch',
        'emergency_parking_brake',
        'radio',
        'mission_computer_switch',
        '3Pos_2Command_Switch_OpenClose',
        'electrically_held_switch',
    }

    # All Control-types dealt with in OUTPUTs (assuming they have an output)
    control_type_map = {
        'led': 'CT_LED',
        'limited_dial': 'CT_ANALOG',
        'analog_dial': 'CT_ANALOG',
        'analog_gauge': 'CT_GAUGE',
        'selector': 'CT_SELECTOR',
        'fixed_step_dial': 'CT_SELECTOR',
        'toggle_switch': 'CT_SELECTOR',
        'action': 'CT_SELECTOR',
        'emergency_parking_brake': 'CT_SELECTOR',
        'radio': 'CT_SELECTOR',
        'mission_computer_switch': 'CT_SELECTOR',
        'display': 'CT_DISPLAY',
        'metadata': 'CT_METADATA',
        '3Pos_2Command_Switch_OpenClose': 'CT_SELECTOR',
        'electrically_held_switch': 'CT_SELECTOR',
        'variable_step_dial': 'CT_ANALOG',
    }

    # --- Helper: detect if a control needs INC/DEC aliases ---

    def parse_int(val): return int(val, 0) # auto-detects hex ('0xFFFF') or decimal

    def needs_fixed_step_incdec(item, threshold):
        """
        Returns True if this control exposes BOTH:
          - a set_state input with max_value > threshold
          - a fixed_step input
        """
        has_set_state = False
        has_fixed_step = False
        max_val = None

        for inp in item.get('inputs', []):
            iface = inp.get('interface')
            if iface == 'set_state' and 'max_value' in inp:
                max_val = inp['max_value']
                if max_val > threshold:
                    has_set_state = True
            if iface == 'fixed_step':
                has_fixed_step = True

        return has_set_state and has_fixed_step

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

    # --- Load aircraft JSON (centralized in _core/aircraft/) ---
    # Import load_aircraft_json eagerly (pure I/O, cross-platform).
    # Import select_aircraft lazily — it uses msvcrt (Windows-only TUI).
    from aircraft_selector import load_aircraft_json
    core_dir = os.path.dirname(os.path.abspath(__file__))
    JSON_FILE = None
    data = None
    ACFT_NAME = None

    # 1. Try aircraft.txt → load from _core/aircraft/
    if os.path.isfile("aircraft.txt"):
        acft_name = open("aircraft.txt", "r", encoding="utf-8").read().strip()
        JSON_FILE, data = load_aircraft_json(core_dir, acft_name)
        if data:
            ACFT_NAME = acft_name
        else:
            print(f"WARNING: aircraft.txt references '{acft_name}' but JSON not found in _core/aircraft/")

    # 2. Fallback: check for legacy local JSON (backward compat during migration)
    if data is None:
        json_files = [f for f in os.listdir() if f.lower().endswith('.json') and "METADATA" not in f]
        for fname in json_files:
            try:
                with open(fname, "r", encoding="utf-8") as f:
                    data = json.load(f)
                if is_valid_panel_json(data):
                    JSON_FILE = fname
                    ACFT_NAME = os.path.splitext(fname)[0]
                    print(f"NOTE: Using legacy local JSON '{fname}'. Run reset_data.py to migrate to centralized aircraft selection.")
                    break
            except Exception:
                continue

    # 3. No aircraft.txt and no local JSON → interactive selector
    if data is None:
        print("No aircraft configured for this LABEL_SET.")
        from aircraft_selector import select_aircraft
        selected = select_aircraft(core_dir)
        if selected:
            JSON_FILE, data = load_aircraft_json(core_dir, selected)
            if data:
                ACFT_NAME = selected
                with open("aircraft.txt", "w", encoding="utf-8") as f:
                    f.write(selected + "\n")
                print(f"aircraft.txt written: {selected}")
        if data is None:
            print("ERROR: No aircraft selected. Cannot proceed.")
            _pause()
            sys.exit(1)

    merge_metadata_jsons(data)

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
            _pause()
            sys.exit(0)

        # Always reload the reference (panels.txt just regenerated)
        all_panels = load_panel_list(PANELS_REFERENCE_FILE)
        selected_panels = load_panel_list(PANELS_SELECTED_FILE)
        if not selected_panels:
            print("ERROR: No panels selected in selected_panels.txt.")
            _pause()
            sys.exit(1)
        invalid_panels = selected_panels - all_panels
        if invalid_panels:
            print("\nERROR: The following panels in 'selected_panels.txt' are NOT present in 'panels.txt':")
            for panel in sorted(invalid_panels):
                print(f"  - {panel}")
            _pause()
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
    _pause("\nPress <ENTER> to continue and generate the files...")

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

    # Python equivalent of the C++ constexpr labelHash() in CUtils.h
    # Must produce identical values for hash table placement to match runtime probing
    def label_hash(s, mod):
        h = 0
        for c in reversed(s):
            h = (ord(c) + 31 * h) & 0xFFFF  # uint16_t truncation at each step
        return h % mod

    # -------- LOAD JSON -------- You need to load the Hornet Json file in order to generate the file.
    # with open(JSON_FILE, encoding='utf-8') as f:
        # data = json.load(f)

    # -------- BUILD OUTPUT_ENTRIES (filtered) -------- THE LOGIC HERE IS SACRED, YOU DO NOT TOUCH NOT EVEN A COMMENT

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

            orig_ident  = item.get('identifier', ident)   
            ctype       = item.get('control_type','').lower().strip()

            # Added a list at the top of the generator for easy modification
            # ctype       = 'selector' if ctype == 'action' else ctype
            if ctype in NORMALIZE_TO_SELECTOR:
                ctype = 'selector'

            api_variant = item.get('api_variant','').strip()
            lid         = ident.lower()
            desc_lower  = item.get('description','').lower()

            # does this control expose fixed_step?
            has_fixed_step = any(inp.get('interface') == 'fixed_step' for inp in item.get('inputs', []))


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

            # Fallback for analog knobs that only expose variable_step
            if max_val is None and ctype in ('limited_dial', 'analog_dial'):
                for inp in item.get('inputs', []):
                    if inp.get('interface') == 'variable_step':
                        # max_val = inp.get('suggested_step', 1)
                        max_val = 1 # We do this so only two records are created POS0 and POS1
                        ctype = 'variable_step'
                        break
                if max_val is None:
                    max_val = 1

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
                # parts = [s.strip() for s in label_src.split('/')]
                parts = [s.strip() for s in label_src.split('/')][::-1]
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

            # if ctype in ('limited_dial', 'analog_dial'):
                # Analog input: use single label with type 'analog' and no group
                # selector_entries.append((ident, ident, max_val, 'analog', 0, "LEVEL"))

            if ctype in ('limited_dial', 'analog_dial'):
                # Potentiometer path
                selector_entries.append((ident, orig_ident, max_val, 'analog', 0, "LEVEL"))
                # Always expose encoder-friendly -3200 / + 3200 pair
                selector_entries.append((f"{ident}_DEC", orig_ident, 0, 'variable_step', 0, "DEC"))
                selector_entries.append((f"{ident}_INC", orig_ident, 1, 'variable_step', 0, "INC"))

            else:
                # 5) append as discrete selector/momentary
                for i, lab in enumerate(labels):
                    clean = lab.upper().replace(' ','_')
                    if useSlash:
                        val = i  # ascending, same as POS0..POSN
                        selector_entries.append((f"{ident}_{clean}", orig_ident, val, ctype, currentGroup, clean))
                    else:
                        selector_entries.append((f"{ident}_{clean}", orig_ident, i, ctype, 0, clean))

                # --- Extra: add INC/DEC aliases for large selectors with fixed_step ---
                if needs_fixed_step_incdec(item, FIXED_STEP_INCDEC_THRESHOLD):
                    selector_entries.append((f"{ident}_DEC", orig_ident, 0, 'fixed_step', 0, "DEC"))
                    selector_entries.append((f"{ident}_INC", orig_ident, 1,  'fixed_step', 0, "INC"))

    # -------- PATCH: Assign missing group IDs for exclusive selectors --------
    from collections import defaultdict
    label_to_indices = defaultdict(list)
    for idx, (full, cmd, val, ct, grp, lab) in enumerate(selector_entries):
        if ct == 'selector':
            label_to_indices[cmd].append(idx)

    next_group_id = groupCounter + 1
    for cmd, indices in label_to_indices.items():
        # If more than one entry and all groups are 0, assign a new unique group
        if len(indices) > 1 and all(selector_entries[i][4] == 0 for i in indices):
            for i in indices:
                entry = list(selector_entries[i])
                entry[4] = next_group_id
                selector_entries[i] = tuple(entry)
            next_group_id += 1
    # -------- END PATCH --------



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

        # f.write("    if (entry.addr == 0xFFFF) continue;\n") // Removed as per suggestion

        f.write("    if (entry.addr == 0xFFFF) return nullptr;\n")
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

        # --- Generate hash table for SelectorMap[] keyed by (dcsCommand, value) ---
        # This replaces the O(n) linear scan in onSelectorChange() with O(1) lookup.
        # Composite key: labelHash(dcsCommand) ^ (value * 7919), same pattern as hidDcsHashTable.
        selector_hash_keys = []  # list of (dcsCommand, value, index_into_SelectorMap)
        for idx, (full, cmd, val, ct, grp, lab) in enumerate(selector_entries):
            selector_hash_keys.append((cmd, val, idx))

        desired_sel_hash = len(selector_hash_keys) * 2
        SEL_HASH_SIZE = next_prime(max(desired_sel_hash, 53))

        def label_hash_raw(s):
            """Raw labelHash matching C++ uint16_t — no modulo applied."""
            h = 0
            for c in reversed(s):
                h = (ord(c) + 31 * h) & 0xFFFF
            return h

        sel_hash_table = ["{nullptr, 0, nullptr}"] * SEL_HASH_SIZE
        for cmd, val, idx in selector_hash_keys:
            h = (label_hash_raw(cmd) ^ (val * 7919)) % SEL_HASH_SIZE
            for probe in range(SEL_HASH_SIZE):
                if sel_hash_table[h] == "{nullptr, 0, nullptr}":
                    sel_hash_table[h] = f'{{"{cmd}", {val}, &SelectorMap[{idx}]}}'
                    break
                h = (h + 1) % SEL_HASH_SIZE
            else:
                print(f"❌ Selector hash table full! SEL_HASH_SIZE={SEL_HASH_SIZE} too small", file=sys.stderr)
                sys.exit(1)

        f.write("\n// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)\n")
        f.write("struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };\n")
        f.write(f"static const SelectorHashEntry selectorHashTable[{SEL_HASH_SIZE}] = {{\n")
        for entry in sel_hash_table:
            f.write(f"  {entry},\n")
        f.write("};\n\n")

        f.write("// Composite hash: labelHash(dcsCommand) ^ (value * 7919)\n")
        f.write("inline const SelectorEntry* findSelectorByDcsAndValue(const char* dcsCommand, uint16_t value) {\n")
        f.write(f"  uint16_t startH = (labelHash(dcsCommand) ^ (value * 7919u)) % {SEL_HASH_SIZE};\n")
        f.write(f"  for (uint16_t i = 0; i < {SEL_HASH_SIZE}; ++i) {{\n")
        f.write(f"    uint16_t idx = (startH + i >= {SEL_HASH_SIZE}) ? (startH + i - {SEL_HASH_SIZE}) : (startH + i);\n")
        f.write("    const auto& entry = selectorHashTable[idx];\n")
        f.write("    if (!entry.dcsCommand) return nullptr;\n")
        f.write("    if (entry.value == value && strcmp(entry.dcsCommand, dcsCommand) == 0) return entry.entry;\n")
        f.write("  }\n")
        f.write("  return nullptr;\n")
        f.write("}\n\n")

        # Build a flat list of all unique oride_labels and mark selectors with group > 0
        command_tracking = {}

        # for full, cmd, val, ct, grp, lab in selector_entries:
            # if cmd not in command_tracking:
                # command_tracking[cmd] = (grp > 0, grp)

        for full, cmd, val, ct, grp, lab in selector_entries:
            if cmd in command_tracking:
                is_sel, old_grp = command_tracking[cmd]
                command_tracking[cmd] = ((is_sel or grp > 0), max(old_grp, grp))
            else:
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

        # --- findCmdEntry() is built at runtime in DCSBIOSBridge.cpp (same TU as commandHistory[]) ---
        # We only emit commandHistorySize above; the hash table lives in DCSBIOSBridge.cpp to avoid
        # static-in-header split-brain across translation units.

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
            h = label_hash(label, desired_size)
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
        f.write("    if (!entry.label) return nullptr;\n")
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
            desired_size = next_prime(len(metadata_entries) * 2)
            hash_table = ["{nullptr, nullptr}"] * desired_size

            for idx, entry in enumerate(metadata_entries):
                label = entry['label']
                h = label_hash(label, desired_size)
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
            f.write( "        if (!entry.label) return nullptr;\n")
            f.write( "        if (strcmp(entry.label, label) == 0) return entry.state;\n")
            f.write( "    }\n")
            f.write( "    return nullptr;\n")
            f.write("}\n")
        else:
            f.write("// No tracked metadata fields found\n")
            f.write("struct MetadataState { const char* label; uint16_t value; };\n")
            f.write("static MetadataState metadataStates[] __attribute__((unused)) = {};\n")
            f.write("static const size_t numMetadataStates __attribute__((unused)) = 0;\n")
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

    void renderField(const char* label, const char* strValue,
                     const DisplayFieldDefLabel* defOverride,
                     FieldState* stateOverride) {
        const DisplayFieldDefLabel* def = defOverride ? defOverride : findFieldDefByLabel(label);
        if (!def) return;
        if (!def->renderFunc) { debugPrintf("[DISPLAY] No renderFunc for label '%s', skipping\\n", def->label); return; }

        FieldState* state = stateOverride;
        if (!state) {
            int idx = def - fieldDefs;
            state = &fieldStates[idx];
        }

        bool valid = true;
        if (def->type == FIELD_NUMERIC) {
            const int value = strToIntFast(strValue);
            valid = (value >= def->minValue && value <= def->maxValue);
        }
        const char* toShow = valid ? strValue : state->lastValue;

        // bound compare/copy by field length, clamp to cache size
        uint8_t need = def->numDigits ? def->numDigits : 1;
        if (need > sizeof(state->lastValue)) need = sizeof(state->lastValue);

        if (memcmp(toShow, state->lastValue, need) == 0) return;

        memcpy(state->lastValue, toShow, need);
        if (def->clearFunc) def->clearFunc(def->driver, def->segMap, *def);
        def->renderFunc(def->driver, def->segMap, toShow, *def);
    }

    '''


    # /////////////////////////////////////// EXTRA FIELDS //////////////////////////////////////////////

    # Build a global list of all display field labels/lengths
    all_display_labels = [(d['label'], d['length']) for d in display_field_defs]

    with open("CT_Display.h", "w", encoding="utf-8") as fout:
        fout.write("// Auto-generated — DO NOT EDIT\n")
        fout.write("#pragma once\n\n")

        # This line added NEW on 8/24/25
        fout.write("#include \"../../../lib/CUtils/src/CUtils.h\"\n\n")

        # fout.write("struct DisplayBufferEntry;\n")
        # fout.write("struct DisplayBufferHashEntry;\n")

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
            h = label_hash(label, desired_size)
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
        fcpp.write( "        if (!entry.label) return nullptr;\n")
        fcpp.write( "        if (strcmp(entry.label, label) == 0) return entry.entry;\n")
        fcpp.write( "    }\n")
        fcpp.write( "    return nullptr;\n")
        fcpp.write("}\n")

        # Extra fields for .cpp file
        fcpp.write(CT_DISPLAY_CPP_EXTRA)

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

        # r'(?P<port>-?\d+)\s*,\s*'

    line_re = re.compile(
        r'\{\s*"(?P<label>[^"]+)"\s*,\s*'        # capture the label
        r'"(?P<source>[^"]+)"\s*,\s*'
        r'(?P<port>-?\d+|PIN\(\d+\)|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*'
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
                    # "port":        int(d["port"]),
                    "port": d["port"],   # Store as string always
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

    # --- GROUP PRESERVATION/ALLOCATION (no collisions) ---
    existing_max_group = 0
    existing_group_by_cmd = {}
    for e in existing_map.values():
        g = int(e["group"])
        if g > 0:
            existing_max_group = max(existing_max_group, g)
            # lock group id for this DCS command label
            existing_group_by_cmd[e["oride_label"]] = g

    next_group_box = [existing_max_group + 1]   # mutable counter
    new_group_by_cmd = {}                       # for new commands this run

    def alloc_group(cmd: str, proposed_grp: int) -> int:
        # keep analog / momentary ungrouped
        if proposed_grp <= 0:
            return 0
        # if this command already has a group in the preserved file, reuse it
        if cmd in existing_group_by_cmd:
            return existing_group_by_cmd[cmd]
        # if we already allocated a new id for this command in this run, reuse it
        if cmd in new_group_by_cmd:
            return new_group_by_cmd[cmd]
        # brand‑new grouped command → allocate next unique id
        g = next_group_box[0]
        new_group_by_cmd[cmd] = g
        next_group_box[0] += 1
        return g

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
            final_grp = alloc_group(cmd, grp)  # <- ensure unique, stable group id
            merged.append((full, "NONE", 0, 0, -1, cmd, val, ct, final_grp))

    # 3) write out merged list
    with open(INPUT_REFERENCE, "w", encoding="utf-8") as f2:
        input_labels = [e[0] for e in merged]
        f2.write("// THIS FILE IS AUTO-GENERATED; ONLY EDIT INDIVIDUAL RECORDS, DO NOT ADD OR DELETE THEM HERE\n")
        f2.write("// You can use a PIN(X) macro where X is an S2 PIN to AUTO-CONVERT to its equivalent position in an S3 device.\n")
        f2.write("// So, PIN(4) will always be PIN 4 if you compile with an S2 but will get automatically converted to 5 if you compile the firmware on an S3.\n")
        f2.write("// This is to easily use S2 or S3 devices on same backplane/hardware physically connected to specific PINs\n")

        f2.write("#pragma once\n\n")
        f2.write("struct InputMapping {\n")
        f2.write("    const char* label;        // Unique selector label, auto-generated.\n")
        f2.write("    const char* source;      // Hardware source identifier. (e.g PCA_0x26, HC165, GPIO, NONE etc)\n")
        f2.write("    int8_t     port;           // Port index (For PCA use port 0 or 1, HC165 does not use port. For GPIO use PIN and use -1 when sharing GPIOs to differentiate HIGH/LOW)\n")
        f2.write("    int8_t     bit;            // Bit position is used for PCA & HC165. GPIO also uses it but ONLY for one-hot selectors (GPIO assigned for each position) in such cases set as -1\n")
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
                # val_str = f"0xFFFF" if val > 32767 else f"{val:>5}"
                val_str = f"{val:>5}"
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
            h = label_hash(label, INPUT_TABLE_SIZE)
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
        f2.write("    if (!entry.label) return nullptr;\n")
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


    # ——— 2) PARSE DcsOutputTable for all labels (CT_LED, CT_GAUGE, CT_ANALOG, CT_SELECTOR) ———
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
        # if control_type in ("CT_LED", "CT_GAUGE", "CT_ANALOG"):
        if control_type in ("CT_LED", "CT_GAUGE", "CT_ANALOG", "CT_SELECTOR"):
            labels.append(label)

    # Duplicate label detection
    if len(labels) != len(set(labels)):
        print("⚠️ WARNING: Duplicate labels detected in DCS table!", file=sys.stderr)

    print(f"✅ Found {len(labels)} LED/Analog/Selector labels in DcsOutputTable ({OUTPUT_HEADER})")

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
        h = label_hash(label, TABLE_SIZE)
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
        out.write("    struct { int8_t gpio; } gpioInfo;\n")
        # out.write("    struct { uint8_t gauge; } gaugeInfo;\n")
        out.write("    struct { uint8_t gpio; uint16_t minPulse; uint16_t maxPulse; uint16_t period; } gaugeInfo;\n")
        out.write("    struct { uint8_t address; uint8_t port; uint8_t bit; } pcaInfo;\n")
        out.write("    struct { uint8_t clkPin; uint8_t dioPin; uint8_t segment; uint8_t bit; } tm1637Info;\n")
        out.write("    struct { uint8_t address; uint8_t column; uint8_t row; } gn1640Info;\n")
        # out.write("    struct { uint8_t index; } ws2812Info;\n")
        out.write("    struct { uint8_t index; uint8_t pin; uint8_t defR; uint8_t defG; uint8_t defB; uint8_t defBright; } ws2812Info;\n")
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
        out.write("    if (!entry.label) return nullptr;\n")
        out.write("    if (strcmp(entry.label, label) == 0) return entry.led;\n")
        out.write("  }\n")
        out.write("  return nullptr;\n")
        out.write("}\n")

    print(f"✅ {LED_REFERENCE} fully regenerated with dynamic enum from KNOWN_DEVICES.")

    # Cleaner so that only one set of .cpp files exists
    WHITELIST = ['CT_Display.cpp', 'DisplayMapping.cpp']
    def print_and_disable_cpp_files():
        current_dir = os.path.abspath(os.getcwd())
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
            _pause()
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
        script_dir = os.path.abspath(os.getcwd())
        display_gen_path = os.path.join(script_dir, "display_gen.py")
        print("\n[INFO] Generating DisplayMapping.cpp/h using display_gen.py ...")
        subprocess.run([sys.executable, display_gen_path], cwd=script_dir, check=True)
    except Exception as e:
        print(f"❌ Error running display_gen.py: {e}")
        _pause()
        sys.exit(1)

    print_and_disable_cpp_files()
    print(f"✅ Renamed .cpp files to cpp.DISABLE in the inactive LABEL SETS to avoid linker conflicts")

    # --- Begin LabelSetConfig.h generation (preserve user edits) ---

    PID_MIN = 0x1000
    PID_MAX = 0xFFFE
    PID_SPACE = PID_MAX - PID_MIN + 1

    def pid_from_name(name: str) -> int:
        h = hashlib.sha1(name.encode("utf-8")).digest()
        raw16 = int.from_bytes(h[:2], "big") & 0xFFFF
        return PID_MIN + (raw16 % PID_SPACE)

    def extract_define(txt: str, key: str) -> str | None:
        # Matches: #define KEY "value"
        m = re.search(rf'#define\s+{key}\s+"([^"]+)"', txt)
        if m: return m.group(1)
        # Fallback: #define KEY value   (stops before whitespace/comment)
        m = re.search(rf'#define\s+{key}\s+([^\s/]+)', txt)
        return m.group(1) if m else None

    def extract_define_int01(txt: str, key: str, default: int) -> int:
        v = extract_define(txt, key)
        if v is None: return default
        v = v.strip().lower()
        return 1 if v in ("1", "true", "yes", "on") else 0

    labelsetconfig_filename = "LabelSetConfig.h"

    # Base names from directory
    _dir_name = os.path.basename(os.path.abspath(os.getcwd()))
    _ls_name = _dir_name[len("LABEL_SET_"):] if _dir_name.startswith("LABEL_SET_") else _dir_name

    # Defaults
    _has_hid_mode_selector = 0

    # Read existing file to preserve user edits
    existing_fullname = None
    if os.path.exists(labelsetconfig_filename):
        with open(labelsetconfig_filename, "r", encoding="utf-8") as _f:
            _txt = _f.read()
        existing_fullname         = extract_define(_txt, "LABEL_SET_FULLNAME")
        _has_hid_mode_selector    = extract_define_int01(_txt, "HAS_HID_MODE_SELECTOR", _has_hid_mode_selector)

    # Name used to derive PID
    name_for_pid = existing_fullname if existing_fullname else f"CockpitOS Panel {_ls_name}"
    _pid = pid_from_name(name_for_pid)

    # FULLNAME to write
    _label_set_fullname = existing_fullname if existing_fullname else f"CockpitOS Panel {_ls_name}"

    _lines = [
        '#if __has_include("./CustomPins.h")',
        '#include "./CustomPins.h"',
        '#endif',
        '',
        f'#define LABEL_SET_NAME        "{_ls_name}"',
        f'#define HAS_HID_MODE_SELECTOR {_has_hid_mode_selector}',
        f'#define LABEL_SET_FULLNAME    "{_label_set_fullname}" // You can safely change this',
        f'#define HAS_{_ls_name}',
        f'#define AUTOGEN_USB_PID       0x{_pid:04X} // DO NOT EDIT THIS',
        ''
    ]

    with open(labelsetconfig_filename, "w", encoding="utf-8") as _f:
        _f.write("\n".join(_lines))

    print(f"[✓] Created {labelsetconfig_filename} with AUTOGEN_USB_PID=0x{_pid:04X} (from '{name_for_pid}')")
    # --- End of LabelSetConfig.h generation ---

    # --- LatchedButtons.h (create if missing, never overwrite) ---
    if not os.path.exists("LatchedButtons.h"):
        with open("LatchedButtons.h", "w", encoding="utf-8") as _f:
            _f.write("// LatchedButtons.h -- Per-label-set latched button configuration\n")
            _f.write("// Buttons listed here toggle ON/OFF instead of acting as momentary press/release.\n")
            _f.write("// Edit via the Label Creator tool or manually.\n\n")
            _f.write("#pragma once\n\n")
            _f.write("static const char* kLatchedButtons[] = {\n")
            _f.write("    // ...add labels of buttons that should latch\n")
            _f.write("};\n")
            _f.write("static const unsigned kLatchedButtonCount = sizeof(kLatchedButtons)/sizeof(kLatchedButtons[0]);\n")
        print("[+] Created LatchedButtons.h (empty)")
    else:
        print("[=] LatchedButtons.h exists, preserving")

    # --- CoverGates.h (create if missing, never overwrite) ---
    if not os.path.exists("CoverGates.h"):
        with open("CoverGates.h", "w", encoding="utf-8") as _f:
            _f.write("// CoverGates.h -- Per-label-set cover gate configuration\n")
            _f.write("// Defines selectors/buttons that are physically guarded by a cover.\n")
            _f.write("// Edit via the Label Creator tool or manually.\n\n")
            _f.write("#pragma once\n\n")
            _f.write("static const CoverGateDef kCoverGates[] = {\n")
            _f.write('    // { "ACTION", "RELEASE_OR_nullptr", "COVER", CoverGateKind::Selector, delay_ms, close_delay_ms },\n')
            _f.write("};\n")
            _f.write("static const unsigned kCoverGateCount = sizeof(kCoverGates) / sizeof(kCoverGates[0]);\n")
        print("[+] Created CoverGates.h (empty)")
    else:
        print("[=] CoverGates.h exists, preserving")

    # Set the active set
    # --- Emit active.set.h one directory up ---
    active_set_path = os.path.join(os.path.dirname(current_dir), "active_set.h")
    with open(active_set_path, "w", encoding="utf-8") as f:
        f.write(f"// PID:0x{_pid:04X} \n\n")
        f.write("#pragma once\n\n")
        f.write(f"#define LABEL_SET {_ls_name}\n")
    print(f"[✓] Created ../active_set.h with LABEL_SET {_ls_name}")

    # --- Call generate_panelkind.py from root ---
    root_dir = os.path.abspath(os.path.join(current_dir, "..", "..", ".."))
    panelkind_script = os.path.join(root_dir, "generate_panelkind.py")

    if os.path.exists(panelkind_script):
        print("\n" + "=" * 60)
        print("Running generate_panelkind.py to update PanelKind.h...")
        print("=" * 60)
        result = subprocess.run([sys.executable, panelkind_script], cwd=root_dir)
        if result.returncode != 0:
            print("⚠️ Warning: generate_panelkind.py returned non-zero exit code")
    else:
        print(f"⚠️ Warning: Could not find {panelkind_script}")
    # --- End generate_panelkind.py call ---

    # Write a completion timestamp so the compiler tool can verify
    # that generate_data.py ran to the end without errors.
    with open(".last_run", "w", encoding="utf-8") as _ts:
        from datetime import datetime
        _ts.write(datetime.now().isoformat())

    # Press ENTER to exit (skipped in batch mode)
    _pause()
    sys.exit(1)

if __name__ == "__main__":
    run()
