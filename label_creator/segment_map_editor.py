"""
CockpitOS Label Creator -- Segment Map Manager.

Creates and manages {PREFIX}_SegmentMap.h files that define the physical
mapping between logical display fields and hardware RAM addresses / bits /
chip IDs for segmented-display driver chips (HT1622, etc.).

A segment map is a PREREQUISITE for the display generator — without one,
display fields for that device prefix are silently skipped.
"""

import os
import re
import sys
import time
import msvcrt
import threading
from pathlib import Path

import ui
import aircraft

# ---------------------------------------------------------------------------
# ANSI constants (mirror ui.py / other editors)
# ---------------------------------------------------------------------------
CYAN     = "\033[96m"
GREEN    = "\033[92m"
YELLOW   = "\033[93m"
RED      = "\033[91m"
BOLD     = "\033[1m"
DIM      = "\033[2m"
RESET    = "\033[0m"
REV      = "\033[7m"
HIDE_CUR = "\033[?25l"
SHOW_CUR = "\033[?25h"
ERASE_LN = "\033[2K"

_SEL_BG  = "\033[48;5;236m"    # subtle dark-gray row highlight
_SECTION_SEP = f"\n  {DIM}\u2500\u2500 Configure \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500{RESET}\n"

# ---------------------------------------------------------------------------
# Segment order templates (used when creating new entries)
# ---------------------------------------------------------------------------
SEG7_ORDER = "[0]=TOP, [1]=TOP-RIGHT, [2]=BOTTOM-RIGHT, [3]=BOTTOM, [4]=BOTTOM-LEFT, [5]=TOP-LEFT, [6]=MIDDLE"
SEG14_ORDER = ("[0]=TOP, [1]=TOP-RIGHT, [2]=BOTTOM-RIGHT, [3]=BOTTOM, "
               "[4]=BOTTOM-LEFT, [5]=TOP-LEFT, [6]=MIDDLE, "
               "[7]=TOP-LEFT-DIAG, [8]=TOP-MID, [9]=TOP-RIGHT-DIAG, "
               "[10]=BOTTOM-LEFT-DIAG, [11]=BOTTOM-MID, "
               "[12]=BOTTOM-RIGHT-DIAG, [13]=DECIMAL")

SEGMENT_PRESETS = {
    7:  ("7-segment", SEG7_ORDER),
    14: ("14-segment", SEG14_ORDER),
}

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
LABELS_DIR = Path(__file__).resolve().parent.parent / "src" / "LABELS"
CORE_DIR   = LABELS_DIR / "_core"


def _w(text):
    sys.stdout.write(text)
    sys.stdout.flush()


def _get_terminal_size():
    try:
        size = os.get_terminal_size()
        return size.columns, size.lines
    except Exception:
        return 120, 30


# ═══════════════════════════════════════════════════════════════════════════
# Segment Map Parser
# ═══════════════════════════════════════════════════════════════════════════
_VAR_RE = re.compile(
    r'const\s+SegmentMap\s+(?P<name>[A-Z0-9_]+)'
    r'(?P<dims>(?:\[\d+\])*)\s*=\s*'
)


def parse_segment_map(filepath):
    """Parse a {PREFIX}_SegmentMap.h file into structured entries.

    Returns list of dicts with keys:
      name          - variable name (e.g. "IFEI_RPM_L_MAP")
      kind          - "MAP" or "LABEL"
      dims          - (digits, segs) tuple for MAP, None for LABEL
      entries       - For MAP: list of lists of (addr, bit, chipID) tuples
                      For LABEL: single (addr, bit, chipID) tuple
      line_start    - first line index of the declaration
      line_end      - last line index (inclusive) of the declaration
      comment_above - list of comment line strings immediately above
    """
    if not os.path.isfile(filepath):
        return []

    with open(filepath, "r", encoding="utf-8") as f:
        raw = f.readlines()

    results = []
    i = 0
    while i < len(raw):
        m = _VAR_RE.search(raw[i])
        if not m:
            i += 1
            continue

        name = m.group("name")
        dims_str = m.group("dims").strip()

        # Collect comment lines immediately above
        comments = []
        j = i - 1
        while j >= 0 and raw[j].strip().startswith("//"):
            comments.insert(0, raw[j].rstrip())
            j -= 1

        # Find the closing semicolon
        end = i
        while end < len(raw) and ";" not in raw[end]:
            end += 1

        # Parse dimension sizes
        dim_nums = [int(x) for x in re.findall(r'\[(\d+)\]', dims_str)]

        if len(dim_nums) == 2:
            # MAP: [digits][segs]
            digits, segs = dim_nums
            entries = _parse_2d_entries(raw, i, end, digits, segs)
            results.append({
                "name": name,
                "kind": "MAP",
                "dims": (digits, segs),
                "entries": entries,
                "line_start": i,
                "line_end": end,
                "comment_above": comments,
            })
        elif len(dim_nums) == 1:
            # 1D array — treat as MAP with 1 "row"
            segs = dim_nums[0]
            entries = _parse_2d_entries(raw, i, end, 1, segs)
            results.append({
                "name": name,
                "kind": "MAP",
                "dims": (1, segs),
                "entries": entries,
                "line_start": i,
                "line_end": end,
                "comment_above": comments,
            })
        else:
            # Scalar (single struct) — LABEL
            triplet = _parse_single_entry(raw, i, end)
            results.append({
                "name": name,
                "kind": "LABEL",
                "dims": None,
                "entries": triplet,
                "line_start": i,
                "line_end": end,
                "comment_above": comments,
            })

        i = end + 1

    return results


def _parse_2d_entries(raw, start, end, digits, segs):
    """Extract a 2D array of (addr, bit, chipID) tuples from source lines."""
    # Join all lines of the declaration
    text = "".join(raw[start:end + 1])
    # Find the outer braces of the initializer
    outer_start = text.index("{", text.index("=")) + 1
    outer_end = text.rindex("}")

    inner = text[outer_start:outer_end]
    # Split on row braces
    rows = []
    depth = 0
    current = ""
    for ch in inner:
        if ch == "{":
            depth += 1
            if depth == 1:
                current = ""
                continue
        elif ch == "}":
            depth -= 1
            if depth == 0:
                rows.append(current)
                current = ""
                continue
        if depth >= 1:
            current += ch

    entries = []
    for row_str in rows:
        segs_list = _parse_triplets(row_str)
        entries.append(segs_list)

    return entries


def _parse_triplets(text):
    """Parse comma/brace groups like {0,1,0}, {2,3,0} into (addr, bit, chip) tuples."""
    results = []
    # Match each {...} group
    for m in re.finditer(r'\{([^}]+)\}', text):
        parts = [x.strip() for x in m.group(1).split(",")]
        if len(parts) >= 3:
            addr = int(parts[0], 0)  # handles 0xFF
            bit = int(parts[1], 0)
            chip = int(parts[2], 0)
            results.append((addr, bit, chip))
    return results


def _parse_single_entry(raw, start, end):
    """Parse a scalar SegmentMap = {addr, bit, chip}; declaration."""
    text = "".join(raw[start:end + 1])
    m = re.search(r'=\s*\{([^}]+)\}', text)
    if m:
        parts = [x.strip() for x in m.group(1).split(",")]
        if len(parts) >= 3:
            return (int(parts[0], 0), int(parts[1], 0), int(parts[2], 0))
    return (0, 0, 0)


# ═══════════════════════════════════════════════════════════════════════════
# Segment Map Writer
# ═══════════════════════════════════════════════════════════════════════════
def write_segment_map(filepath, entries):
    """Write a complete {PREFIX}_SegmentMap.h file from structured entries."""
    lines = ["#pragma once\n", "\n"]

    for entry in entries:
        # Comment block above
        for c in entry.get("comment_above", []):
            lines.append(c + "\n")

        name = entry["name"]
        kind = entry["kind"]

        if kind == "LABEL":
            addr, bit, chip = entry["entries"]
            lines.append(f"const SegmentMap {name} = {{{addr}, {bit}, {chip}}};\n")
        else:
            digits, segs = entry["dims"]
            if digits == 1 and not name.endswith("_MAP"):
                # 1D array
                lines.append(f"const SegmentMap {name}[{segs}] = {{\n")
            else:
                lines.append(f"const SegmentMap {name}[{digits}][{segs}] = {{\n")

            for d_idx, row in enumerate(entry["entries"]):
                parts = []
                for addr, bit, chip in row:
                    if addr == 0xFF and bit == 0xFF:
                        parts.append(f"{{0xFF,0xFF,{chip}}}")
                    else:
                        parts.append(f"{{{addr},{bit},{chip}}}")
                row_str = "    { " + ", ".join(parts) + " }"
                if d_idx < len(entry["entries"]) - 1:
                    row_str += ","
                lines.append(row_str + "\n")

            lines.append("};\n")

        lines.append("\n")

    with open(filepath, "w", encoding="utf-8") as f:
        f.writelines(lines)


def _append_entry_to_file(filepath, entry):
    """Append a single entry to an existing segment map file."""
    lines = []

    # Comment above
    for c in entry.get("comment_above", []):
        lines.append(c + "\n")

    name = entry["name"]
    kind = entry["kind"]

    if kind == "LABEL":
        addr, bit, chip = entry["entries"]
        lines.append(f"const SegmentMap {name} = {{{addr}, {bit}, {chip}}};\n")
    else:
        digits, segs = entry["dims"]
        lines.append(f"const SegmentMap {name}[{digits}][{segs}] = {{\n")
        for d_idx, row in enumerate(entry["entries"]):
            parts = []
            for addr, bit, chip in row:
                if addr == 0xFF and bit == 0xFF:
                    parts.append(f"{{0xFF,0xFF,{chip}}}")
                else:
                    parts.append(f"{{{addr},{bit},{chip}}}")
            row_str = "    { " + ", ".join(parts) + " }"
            if d_idx < len(entry["entries"]) - 1:
                row_str += ","
            lines.append(row_str + "\n")
        lines.append("};\n")

    lines.append("\n")

    with open(filepath, "a", encoding="utf-8") as f:
        f.writelines(lines)


# ═══════════════════════════════════════════════════════════════════════════
# Device prefix scanner — reads aircraft JSON for display fields
# ═══════════════════════════════════════════════════════════════════════════
def scan_display_prefixes(ls_dir, ac_name):
    """Scan aircraft JSON for display device prefixes.

    Returns list of dicts sorted by prefix:
      prefix      - "IFEI", "HUD", etc.
      fields      - list of field label strings for this prefix
      has_segmap  - bool
      segmap_path - Path or None
      entry_count - int (entries if file exists, else 0)
    """
    ls_path = Path(ls_dir) if not isinstance(ls_dir, Path) else ls_dir

    try:
        data = aircraft.load_aircraft_json(ac_name)
    except Exception:
        return []

    # Collect display fields grouped by prefix
    prefix_fields = {}
    for category, controls in data.items():
        if not isinstance(controls, dict):
            continue
        for key, item in controls.items():
            if not isinstance(item, dict):
                continue
            if item.get("control_type", "").lower().strip() != "display":
                continue
            label = item.get("identifier", key)
            prefix = label.split("_")[0] if "_" in label else label
            prefix_fields.setdefault(prefix, []).append(label)

    results = []
    for prefix in sorted(prefix_fields):
        segmap_name = f"{prefix}_SegmentMap.h"
        segmap_path = ls_path / segmap_name
        has = segmap_path.is_file()
        count = 0
        if has:
            try:
                entries = parse_segment_map(str(segmap_path))
                count = len(entries)
            except Exception:
                count = 0
        results.append({
            "prefix": prefix,
            "fields": sorted(prefix_fields[prefix]),
            "has_segmap": has,
            "segmap_path": segmap_path if has else None,
            "entry_count": count,
        })

    return results


def segment_map_caption(ls_dir, ac_name):
    """Return e.g. '1 of 3 devices' for the menu caption."""
    ls_path = Path(ls_dir) if not isinstance(ls_dir, Path) else ls_dir
    prefixes = scan_display_prefixes(ls_path, ac_name)
    if not prefixes:
        return "no displays"
    have = sum(1 for p in prefixes if p["has_segmap"])
    total = len(prefixes)
    return f"{have} of {total} devices"


# ═══════════════════════════════════════════════════════════════════════════
# Validation helpers
# ═══════════════════════════════════════════════════════════════════════════
def _validate_address(addr_str):
    """Validate and return an integer address (0-63 or 0xFF)."""
    s = addr_str.strip()
    if not s:
        return None
    try:
        val = int(s, 0)
    except ValueError:
        return None
    if val == 0xFF:
        return 0xFF
    if 0 <= val <= 63:
        return val
    return None


def _validate_bit(bit_str):
    """Validate and return a bit value (0-7 or 0xFF)."""
    s = bit_str.strip()
    if not s:
        return None
    try:
        val = int(s, 0)
    except ValueError:
        return None
    if val == 0xFF:
        return 0xFF
    if 0 <= val <= 7:
        return val
    return None


def _validate_chip(chip_str):
    """Validate and return chip ID (0-7)."""
    s = chip_str.strip()
    if not s:
        return None
    try:
        val = int(s)
    except ValueError:
        return None
    if 0 <= val <= 7:
        return val
    return None


def _find_duplicate_segments(entries):
    """Find duplicate (addr, bit, chip) in a list of parsed entries.

    Returns list of duplicate triplet strings for display.
    """
    seen = set()
    dupes = []
    for entry in entries:
        if entry["kind"] == "LABEL":
            t = entry["entries"]
            if t[0] == 0xFF:
                continue
            key = t
            if key in seen:
                dupes.append(f"({t[0]}, {t[1]}, {t[2]})")
            seen.add(key)
        else:
            for row in entry["entries"]:
                for t in row:
                    if t[0] == 0xFF:
                        continue
                    key = t
                    if key in seen:
                        dupes.append(f"({t[0]}, {t[1]}, {t[2]})")
                    seen.add(key)
    return dupes


# ═══════════════════════════════════════════════════════════════════════════
# TUI — Create new entry
# ═══════════════════════════════════════════════════════════════════════════
def _prompt_new_entry(prefix, existing_fields, all_fields):
    """Guide user through creating a new segment map entry.

    Returns a dict ready for write/append, or None if cancelled.
    """
    # Show available (unmapped) fields
    mapped_bases = set()
    for name in existing_fields:
        # Strip prefix and _MAP or _LABEL suffix to get the base field
        base = name
        if base.startswith(prefix + "_"):
            base = base[len(prefix) + 1:]
        base = re.sub(r'_(?:MAP|LABEL|TEXTURE_LABEL)$', '', base)
        mapped_bases.add(base)

    # Build list of unmapped fields from the JSON
    unmapped = []
    for label in all_fields:
        base = label
        if base.startswith(prefix + "_"):
            base = base[len(prefix) + 1:]
        if base not in mapped_bases:
            unmapped.append((label, label))

    if unmapped:
        ui.info(f"{DIM}Fields without a segment map entry:{RESET}")
        field_choice = ui.pick("Map which field?", unmapped)
        if field_choice is None:
            return None
    else:
        ui.info(f"{DIM}All fields have segment map entries.{RESET}")
        ui.info(f"{DIM}You can still add a custom entry.{RESET}")
        field_choice = ui.text_input("Variable base name (without prefix)")
        if not field_choice:
            return None
        field_choice = f"{prefix}_{field_choice.strip().upper()}"

    # Auto-determine entry type from field name convention.
    # The generator (display_gen_core.py → get_map_pointer_and_dims) uses:
    #   field ending in _TEXTURE or _LABEL  →  {FIELD}_LABEL  (single indicator)
    #   everything else                     →  {FIELD}_MAP    (multi-digit)
    # We enforce the same rule so the generated code can find the entry.
    base_name = field_choice.upper()
    if not base_name.startswith(prefix + "_"):
        base_name = f"{prefix}_{base_name}"

    if base_name.endswith("_TEXTURE") or base_name.endswith("_LABEL"):
        kind = "LABEL"
    else:
        kind = "MAP"

    print(_SECTION_SEP)
    ui.info(
        f"{DIM}Auto-detected type: {CYAN}{kind}{RESET}"
        f"{DIM}  (field {'ends with _TEXTURE/_LABEL' if kind == 'LABEL' else 'is a multi-digit display'}){RESET}"
    )

    if kind == "LABEL":
        return _prompt_label_entry(base_name)
    else:
        return _prompt_map_entry(base_name)


def _prompt_label_entry(base_name):
    """Prompt for a single LABEL entry: {addr, bit, chip}."""
    # Determine variable name
    if not base_name.endswith("_LABEL") and not base_name.endswith("_TEXTURE_LABEL"):
        if "TEXTURE" in base_name.upper():
            var_name = f"{base_name}_LABEL"
        else:
            var_name = f"{base_name}_LABEL"
    else:
        var_name = base_name

    print()
    ui.info(f"{DIM}Enter the hardware address for: {CYAN}{var_name}{RESET}")
    ui.info(f"{DIM}Format: RAM address (0-63), bit (0-7), chip ID (0-7){RESET}")
    print()

    addr_s = ui.text_input("RAM address (0-63)")
    if addr_s is None:
        return None
    addr = _validate_address(addr_s)
    if addr is None:
        ui.error("Invalid address. Must be 0-63 or 0xFF.")
        return None

    bit_s = ui.text_input("Bit (0-7)")
    if bit_s is None:
        return None
    bit = _validate_bit(bit_s)
    if bit is None:
        ui.error("Invalid bit. Must be 0-7 or 0xFF.")
        return None

    chip_s = ui.text_input("Chip ID (0-7)", default="0")
    if chip_s is None:
        return None
    chip = _validate_chip(chip_s)
    if chip is None:
        ui.error("Invalid chip ID. Must be 0-7.")
        return None

    # Build section comment
    section = base_name.replace(f"{base_name.split('_')[0]}_", "")
    comment = f"// ================== {section} =================="

    return {
        "name": var_name,
        "kind": "LABEL",
        "dims": None,
        "entries": (addr, bit, chip),
        "line_start": 0,
        "line_end": 0,
        "comment_above": [comment],
    }


def _prompt_map_entry(base_name):
    """Prompt for a MAP entry with full digit × segment grid."""
    var_name = f"{base_name}_MAP"

    # Number of digits
    print()
    digits_s = ui.text_input("Number of digits", default="3")
    if digits_s is None:
        return None
    try:
        digits = int(digits_s)
        if digits < 1 or digits > 32:
            raise ValueError
    except ValueError:
        ui.error("Invalid. Must be 1-32.")
        return None

    # Segments per digit
    seg_opts = [
        ("7  (standard 7-segment)",      "7"),
        ("14 (alphanumeric 14-segment)",  "14"),
        ("Custom number",                 "custom"),
    ]
    seg_choice = ui.pick("Segments per digit:", seg_opts, default="7")
    if seg_choice is None:
        return None

    if seg_choice == "custom":
        segs_s = ui.text_input("Segments per digit")
        if segs_s is None:
            return None
        try:
            segs = int(segs_s)
            if segs < 1 or segs > 32:
                raise ValueError
        except ValueError:
            ui.error("Invalid. Must be 1-32.")
            return None
    else:
        segs = int(seg_choice)

    # Get segment order info
    seg_preset = SEGMENT_PRESETS.get(segs)
    if seg_preset:
        seg_label, seg_order = seg_preset
        print()
        ui.info(f"{DIM}Segment order ({seg_label}): {seg_order}{RESET}")
    else:
        seg_order = None

    # Chip ID (same for all segments in this map)
    chip_s = ui.text_input("Chip ID for all segments (0-7)", default="0")
    if chip_s is None:
        return None
    default_chip = _validate_chip(chip_s)
    if default_chip is None:
        ui.error("Invalid chip ID.")
        return None

    # Prompt for each digit's segments
    print()
    ui.info(f"{DIM}Enter segments for each digit.{RESET}")
    ui.info(f"{DIM}Format per segment: RAM_ADDR,BIT  (or 'x' for unused 0xFF,0xFF){RESET}")
    ui.info(f"{DIM}You can also enter all segments for a digit on one line:{RESET}")
    ui.info(f"{DIM}  e.g.  2,3 2,2 2,1 2,0 0,1 0,3 0,2{RESET}")
    print()

    all_rows = []
    for d in range(digits):
        row_label = f"Digit {d + 1}/{digits}"
        ui.info(f"{CYAN}{BOLD}{row_label}{RESET}")

        # Try single-line entry first
        line_input = ui.text_input(
            f"  All {segs} segments (space-separated addr,bit or 'x')"
        )
        if line_input is None:
            return None

        if line_input == ui._RESET_SENTINEL:
            return None

        row = _parse_segment_line(line_input, segs, default_chip)
        if row is not None:
            all_rows.append(row)
            continue

        # Fall back to per-segment entry
        row = []
        for s in range(segs):
            if seg_preset:
                seg_name = seg_order.split(", ")[s] if s < len(seg_order.split(", ")) else f"[{s}]"
            else:
                seg_name = f"[{s}]"
            seg_input = ui.text_input(f"  Segment {seg_name}")
            if seg_input is None:
                return None
            seg_input = seg_input.strip()
            if seg_input.lower() == "x":
                row.append((0xFF, 0xFF, default_chip))
            else:
                parts = seg_input.split(",")
                if len(parts) >= 2:
                    a = _validate_address(parts[0])
                    b = _validate_bit(parts[1])
                    if a is not None and b is not None:
                        row.append((a, b, default_chip))
                    else:
                        ui.error("Invalid. Using 0xFF placeholder.")
                        row.append((0xFF, 0xFF, default_chip))
                else:
                    ui.error("Expected addr,bit. Using 0xFF placeholder.")
                    row.append((0xFF, 0xFF, default_chip))
        all_rows.append(row)

    # Build entry
    section = base_name.replace(f"{base_name.split('_')[0]}_", "")
    seg_comment = f"// {seg_order}" if seg_order else ""

    comments = [f"// ================== {section} =================="]
    if seg_comment:
        comments.append(seg_comment)

    return {
        "name": var_name,
        "kind": "MAP",
        "dims": (digits, segs),
        "entries": all_rows,
        "line_start": 0,
        "line_end": 0,
        "comment_above": comments,
    }


def _parse_segment_line(text, expected_segs, default_chip):
    """Try to parse a single-line segment entry like '2,3 2,2 2,1 x x x x'.

    Returns list of (addr, bit, chip) tuples, or None if parsing fails.
    """
    text = text.strip()
    if not text:
        return None

    parts = text.split()
    if len(parts) != expected_segs:
        return None

    row = []
    for part in parts:
        part = part.strip()
        if part.lower() == "x":
            row.append((0xFF, 0xFF, default_chip))
        else:
            ab = part.split(",")
            if len(ab) >= 2:
                a = _validate_address(ab[0])
                b = _validate_bit(ab[1])
                c = _validate_chip(ab[2]) if len(ab) >= 3 else default_chip
                if a is not None and b is not None and c is not None:
                    row.append((a, b, c))
                else:
                    return None
            else:
                return None
    return row


# ═══════════════════════════════════════════════════════════════════════════
# TUI — Edit existing entry
# ═══════════════════════════════════════════════════════════════════════════
def _edit_existing_entry(entry):
    """Edit an existing segment map entry in-place. Returns True if modified."""
    name = entry["name"]
    kind = entry["kind"]

    ui.header(f"Edit: {name}")
    print()

    if kind == "LABEL":
        addr, bit, chip = entry["entries"]
        ui.info(f"  Current: {CYAN}{{{addr}, {bit}, {chip}}}{RESET}")
        print()

        new_addr_s = ui.text_input("RAM address (0-63)", default=str(addr))
        if new_addr_s is None:
            return False
        new_addr = _validate_address(new_addr_s)
        if new_addr is None:
            ui.error("Invalid address.")
            return False

        new_bit_s = ui.text_input("Bit (0-7)", default=str(bit))
        if new_bit_s is None:
            return False
        new_bit = _validate_bit(new_bit_s)
        if new_bit is None:
            ui.error("Invalid bit.")
            return False

        new_chip_s = ui.text_input("Chip ID", default=str(chip))
        if new_chip_s is None:
            return False
        new_chip = _validate_chip(new_chip_s)
        if new_chip is None:
            ui.error("Invalid chip ID.")
            return False

        entry["entries"] = (new_addr, new_bit, new_chip)
        return True

    # MAP entry
    digits, segs = entry["dims"]
    ui.info(f"  Dimensions: {CYAN}[{digits}][{segs}]{RESET}")
    print()

    # Show current grid
    for d_idx, row in enumerate(entry["entries"]):
        parts = []
        for addr, bit, chip in row:
            if addr == 0xFF and bit == 0xFF:
                parts.append(f"{DIM}  x  {RESET}")
            else:
                parts.append(f"{addr},{bit},{chip}")
        ui.info(f"  Digit {d_idx}: " + "  ".join(parts))

    print()
    edit_opts = [
        ("Edit a specific digit", "digit"),
        ("Re-enter all digits",   "all"),
        ("Cancel",                "cancel"),
    ]
    action = ui.pick("What to edit?", edit_opts)
    if action is None or action == "cancel":
        return False

    if action == "digit":
        digit_opts = [(f"Digit {d}", str(d)) for d in range(digits)]
        d_choice = ui.pick("Which digit?", digit_opts)
        if d_choice is None:
            return False
        d_idx = int(d_choice)

        # Get default chip from existing data
        existing_chips = set()
        for a, b, c in entry["entries"][d_idx]:
            if a != 0xFF:
                existing_chips.add(c)
        default_chip = existing_chips.pop() if existing_chips else 0

        ui.info(f"{DIM}Enter {segs} segments (space-separated addr,bit or 'x'):{RESET}")
        line = ui.text_input(f"Digit {d_idx}")
        if line is None:
            return False
        row = _parse_segment_line(line, segs, default_chip)
        if row is None:
            ui.error(f"Expected {segs} entries.")
            return False
        entry["entries"][d_idx] = row
        return True

    if action == "all":
        # Get default chip from existing data
        existing_chips = set()
        for row in entry["entries"]:
            for a, b, c in row:
                if a != 0xFF:
                    existing_chips.add(c)
        default_chip = existing_chips.pop() if existing_chips else 0

        new_rows = []
        for d in range(digits):
            ui.info(f"{CYAN}Digit {d + 1}/{digits}{RESET}")
            line = ui.text_input(
                f"  All {segs} segments (space-separated addr,bit or 'x')"
            )
            if line is None:
                return False
            row = _parse_segment_line(line, segs, default_chip)
            if row is None:
                ui.error(f"Expected {segs} entries. Aborting.")
                return False
            new_rows.append(row)
        entry["entries"] = new_rows
        return True

    return False


# ═══════════════════════════════════════════════════════════════════════════
# TUI — Entry detail view (shown on Space)
# ═══════════════════════════════════════════════════════════════════════════
def _show_entry_detail(entry):
    """Full-screen detail view of a single segment map entry.

    Scrollable with Up/Down. Any other key exits back to the list.
    """
    name = entry["name"]
    kind = entry["kind"]

    lines = []
    lines.append(f"  {CYAN}{BOLD}{name}{RESET}")
    lines.append("")

    if kind == "LABEL":
        a, b, c = entry["entries"]
        lines.append(f"  {DIM}Type:{RESET}     Label (single indicator)")
        lines.append(f"  {DIM}Address:{RESET}  {a}")
        lines.append(f"  {DIM}Bit:{RESET}      {b}")
        lines.append(f"  {DIM}Chip ID:{RESET}  {c}")
    else:
        digits, segs = entry["dims"]
        preset = SEGMENT_PRESETS.get(segs)
        type_label = preset[0] if preset else f"{segs}-segment"
        seg_order = preset[1] if preset else None

        lines.append(f"  {DIM}Type:{RESET}     MAP ({type_label})")
        lines.append(f"  {DIM}Digits:{RESET}   {digits}")
        lines.append(f"  {DIM}Segments:{RESET} {segs}")
        if seg_order:
            lines.append(f"  {DIM}Order:{RESET}    {seg_order}")

        # Collect chip IDs
        chips = set()
        for row in entry["entries"]:
            for a, b, c in row:
                if a != 0xFF:
                    chips.add(c)
        chip_str = ", ".join(str(c) for c in sorted(chips)) if chips else "?"
        lines.append(f"  {DIM}Chip(s):{RESET}  {chip_str}")

        lines.append("")
        lines.append(f"  {BOLD}Segment Grid:{RESET}")
        lines.append("")

        # Header row with segment indices
        hdr = f"  {'':>8}"
        for s in range(segs):
            hdr += f"  {DIM}[{s}]{RESET}    "
        lines.append(hdr)

        # Data rows per digit
        for d_idx, row in enumerate(entry["entries"]):
            row_str = f"  {CYAN}Digit {d_idx}{RESET} "
            for a, b, c in row:
                if a == 0xFF and b == 0xFF:
                    row_str += f"  {DIM}  --  {RESET} "
                else:
                    row_str += f"  {GREEN}{a:>2},{b},{c}{RESET}  "
            lines.append(row_str)

    lines.append("")

    # Comments above the entry
    if entry.get("comment_above"):
        lines.append(f"  {BOLD}Source comments:{RESET}")
        for c in entry["comment_above"]:
            lines.append(f"  {DIM}{c}{RESET}")
        lines.append("")

    lines.append(f"  {DIM}\u2191\u2193=scroll  any other key=back{RESET}")

    # -- Scrollable display --
    cols, term_rows = _get_terminal_size()
    view_height = term_rows - 4  # header + footer margins
    scroll = 0
    total = len(lines)

    def _draw_detail():
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)
        header_w = cols - 4
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  Segment Map Detail{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        end = min(scroll + view_height, total)
        for i in range(scroll, end):
            _w(lines[i] + "\n")
        # Pad remaining lines
        for _ in range(view_height - (end - scroll)):
            _w("\n")

    _draw_detail()

    while True:
        ch = msvcrt.getwch()
        if ch in ("\xe0", "\x00"):
            ch2 = msvcrt.getwch()
            if ch2 == "H" and scroll > 0:        # Up
                scroll -= 1
                _draw_detail()
            elif ch2 == "P" and scroll < total - view_height:  # Down
                scroll += 1
                _draw_detail()
            else:
                # Any other special key exits
                _w(SHOW_CUR)
                return
        else:
            _w(SHOW_CUR)
            return


# ═══════════════════════════════════════════════════════════════════════════
# TUI — Scrollable entry list for an existing segment map
# ═══════════════════════════════════════════════════════════════════════════
def _entry_dims_str(entry):
    """Format dimensions for list display."""
    if entry["kind"] == "LABEL":
        return "single"
    d, s = entry["dims"]
    return f"[{d}][{s}]"


def _entry_type_str(entry):
    """Format type for list display."""
    if entry["kind"] == "LABEL":
        return "label"
    segs = entry["dims"][1]
    preset = SEGMENT_PRESETS.get(segs)
    return preset[0] if preset else f"{segs}-seg"


def _entry_chip_str(entry):
    """Format chip ID(s) for list display."""
    if entry["kind"] == "LABEL":
        return str(entry["entries"][2])
    chips = set()
    for row in entry["entries"]:
        for a, b, c in row:
            if a != 0xFF:
                chips.add(c)
    return ",".join(str(c) for c in sorted(chips)) if chips else "?"


def _show_segment_map_entries(filepath, prefix, all_fields, ls_name="", ac_name=""):
    """Scrollable list of entries in a segment map file.

    Full-screen TUI with arrow keys, scroll bar, boundary flash.
    Space = show detail view, Enter/Right = edit, A = add, Esc/Left = back.

    Returns True if the file was modified (caller may want to regenerate).
    """
    file_modified = False

    while True:
        entries = parse_segment_map(filepath)
        total = len(entries)

        if total == 0:
            parts = [ls_name, ac_name]
            ctx = ("  (" + ", ".join(p for p in parts if p) + ")"
                   if any(parts) else "")
            ui.header(f"{prefix}_SegmentMap.h{ctx}")
            print()
            ui.info(f"{DIM}No entries found in this segment map.{RESET}")
            print()
            ui.info(f"Press {CYAN}A{RESET} to add a new entry, or {DIM}Esc{RESET} to go back.")
            while True:
                ch = msvcrt.getwch()
                if ch.lower() == "a":
                    existing_names = [e["name"] for e in entries]
                    _w(SHOW_CUR)
                    new_entry = _prompt_new_entry(prefix, existing_names, all_fields)
                    if new_entry is not None:
                        _append_entry_to_file(filepath, new_entry)
                        file_modified = True
                        ui.success(f"Added {new_entry['name']}")
                        time.sleep(0.6)
                    break  # re-enter outer loop to re-parse
                elif ch == "\x1b":
                    return file_modified
            continue

        idx = 0
        scroll = 0

        cols, rows = _get_terminal_size()
        # Reserve: 3 header + 1 col-header + list + 1 blank + 1 footer = 6 fixed
        list_height = rows - 6
        if list_height < 5:
            list_height = 5

        _SCROLL_BLOCK = "\u2588"
        _SCROLL_LIGHT = "\u2591"
        needs_scroll = total > list_height

        _flash_timer  = [None]
        _flash_active = [False]

        def _scroll_bar_positions():
            if not needs_scroll:
                return (0, list_height)
            thumb = max(1, round(list_height * list_height / total))
            max_scroll = total - list_height
            if max_scroll <= 0:
                top = 0
            else:
                top = round(scroll * (list_height - thumb) / max_scroll)
            return (top, top + thumb)

        # Column widths
        name_w   = 36
        dims_w   = 8
        type_w   = 12
        chip_w   = 6
        # Fixed: prefix(3) + 4 gaps(2 each) + scrollbar(1) + margin(2)
        fixed_cols = 3 + 8 + 2 + 1
        remaining = cols - fixed_cols - dims_w - type_w - chip_w
        name_w = max(remaining, 20)

        def _render_row(i, is_highlighted, scroll_char):
            e = entries[i]
            nm   = e["name"][:name_w]
            dims = _entry_dims_str(e)[:dims_w]
            tp   = _entry_type_str(e)[:type_w]
            ch   = _entry_chip_str(e)[:chip_w]

            if is_highlighted and _flash_active[0]:
                return (
                    f"{_SEL_BG} {YELLOW}> {nm:<{name_w}}  "
                    f"{dims:<{dims_w}}  {tp:<{type_w}}  {ch:<{chip_w}}"
                    f" {scroll_char}{RESET}"
                )
            elif is_highlighted:
                return (
                    f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} "
                    f"{GREEN}{nm:<{name_w}}{RESET}{_SEL_BG}  "
                    f"{GREEN}{dims:<{dims_w}}{RESET}{_SEL_BG}  "
                    f"{GREEN}{tp:<{type_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{ch:<{chip_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}"
                )
            return (
                f"   {GREEN}{nm:<{name_w}}{RESET}  "
                f"{GREEN}{dims:<{dims_w}}{RESET}  "
                f"{GREEN}{tp:<{type_w}}{RESET}  "
                f"{DIM}{ch:<{chip_w}}{RESET}"
                f" {DIM}{scroll_char}{RESET}"
            )

        def _clamp_scroll():
            nonlocal scroll
            if idx < scroll:
                scroll = idx
            if idx >= scroll + list_height:
                scroll = idx - list_height + 1

        def _flash_row():
            if _flash_timer[0]:
                _flash_timer[0].cancel()
            _flash_active[0] = True
            _draw()
            def _restore():
                _flash_active[0] = False
                _draw()
            _flash_timer[0] = threading.Timer(0.15, _restore)
            _flash_timer[0].daemon = True
            _flash_timer[0].start()

        header_w = cols - 4

        def _draw():
            os.system("cls" if os.name == "nt" else "clear")
            _w(HIDE_CUR)

            # Header  — matches LED / Display editor pattern
            parts = [ls_name, ac_name]
            ctx = ("  (" + ", ".join(p for p in parts if p) + ")"
                   if any(parts) else "")
            title = f"{prefix}_SegmentMap.h{ctx}"
            counter = f"{total} entries"
            spacing = header_w - len(title) - len(counter) - 4
            if spacing < 1:
                spacing = 1
            _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
            _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}"
               f"{GREEN}{counter}{RESET}\n")
            _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

            # Column header
            _w(f" {BOLD}  {'Name':<{name_w}}  {'Dims':<{dims_w}}  "
               f"{'Type':<{type_w}}  {'Chip':<{chip_w}}{RESET}\n")

            # Scroll bar
            thumb_start, thumb_end = _scroll_bar_positions()

            # Rows
            for row in range(list_height):
                ri = scroll + row
                if needs_scroll:
                    sc = (_SCROLL_BLOCK
                          if thumb_start <= row < thumb_end
                          else _SCROLL_LIGHT)
                else:
                    sc = " "
                if ri < total:
                    _w(_render_row(ri, ri == idx, sc) + "\n")
                else:
                    _w(ERASE_LN + "\n")

            # Footer
            _w(f"\n  {DIM}\u2191\u2193=move  Space=detail  "
               f"\u2192/Enter=edit  A=add  \u2190/Esc=back{RESET}")

        _clamp_scroll()
        _draw()

        try:
            while True:
                ch = msvcrt.getwch()

                if ch in ("\xe0", "\x00"):
                    ch2 = msvcrt.getwch()
                    old = idx
                    if ch2 == "H":          # Up
                        if idx > 0:
                            idx -= 1
                        else:
                            _flash_row()
                    elif ch2 == "P":        # Down
                        if idx < total - 1:
                            idx += 1
                        else:
                            _flash_row()
                    elif ch2 == "M":        # Right — edit
                        _w(SHOW_CUR)
                        e = entries[idx]
                        if _edit_existing_entry(e):
                            all_entries = parse_segment_map(filepath)
                            for ae in all_entries:
                                if ae["name"] == e["name"]:
                                    ae["entries"] = e["entries"]
                                    break
                            write_segment_map(filepath, all_entries)
                            file_modified = True
                            ui.success(f"Updated {e['name']}")
                            time.sleep(0.6)
                        # Re-parse & redraw
                        break  # break inner, re-enter outer while
                    elif ch2 == "K":        # Left — back
                        if _flash_timer[0]:
                            _flash_timer[0].cancel()
                        _w(SHOW_CUR)
                        return file_modified
                    else:
                        continue
                    if old != idx:
                        _clamp_scroll()
                        _draw()

                elif ch == " ":             # Space — detail view
                    _show_entry_detail(entries[idx])
                    _clamp_scroll()
                    _draw()

                elif ch == "\r":            # Enter — edit
                    _w(SHOW_CUR)
                    e = entries[idx]
                    if _edit_existing_entry(e):
                        all_entries = parse_segment_map(filepath)
                        for ae in all_entries:
                            if ae["name"] == e["name"]:
                                ae["entries"] = e["entries"]
                                break
                        write_segment_map(filepath, all_entries)
                        file_modified = True
                        ui.success(f"Updated {e['name']}")
                        time.sleep(0.6)
                    break  # re-parse & redraw

                elif ch.lower() == "a":     # A — add new entry
                    _w(SHOW_CUR)
                    existing_names = [e["name"] for e in entries]
                    new_entry = _prompt_new_entry(
                        prefix, existing_names, all_fields)
                    if new_entry is not None:
                        _append_entry_to_file(filepath, new_entry)
                        file_modified = True
                        ui.success(f"Added {new_entry['name']}")
                        time.sleep(0.6)
                    break  # re-parse & redraw

                elif ch == "\x1b":          # Esc — back
                    if _flash_timer[0]:
                        _flash_timer[0].cancel()
                    _w(SHOW_CUR)
                    return file_modified

        except KeyboardInterrupt:
            if _flash_timer[0]:
                _flash_timer[0].cancel()
            _w(SHOW_CUR)
            raise


# ═══════════════════════════════════════════════════════════════════════════
# TUI — Create new segment map file
# ═══════════════════════════════════════════════════════════════════════════
def _create_new_segment_map(ls_dir, prefix, all_fields, ls_name="", ac_name=""):
    """Guide user through creating a brand new segment map file.

    Returns True if a file was created.
    """
    filepath = os.path.join(str(ls_dir), f"{prefix}_SegmentMap.h")

    ui.header(f"Create {prefix}_SegmentMap.h")
    print()
    ui.info(f"This will create a new segment map for the {CYAN}{prefix}{RESET} display device.")
    ui.info(f"{DIM}The segment map defines how logical display fields map to{RESET}")
    ui.info(f"{DIM}physical RAM addresses, bits, and chip IDs on your hardware.{RESET}")
    print()
    ui.info(f"{YELLOW}{BOLD}Important:{RESET} {DIM}Segment order MUST match your font tables!{RESET}")
    ui.info(f"{DIM}See SegmentMap.txt in your label set for detailed instructions.{RESET}")
    print()

    ok = ui.confirm(f"Create {prefix}_SegmentMap.h?")
    if not ok:
        return False

    # Create the file with pragma once header
    with open(filepath, "w", encoding="utf-8") as f:
        f.write("#pragma once\n\n")

    # Immediately start adding entries
    ui.success(f"Created {prefix}_SegmentMap.h")
    time.sleep(0.5)

    return _show_segment_map_entries(filepath, prefix, all_fields, ls_name, ac_name)


# ═══════════════════════════════════════════════════════════════════════════
# Main entry point
# ═══════════════════════════════════════════════════════════════════════════
def manage_segment_maps(ls_dir, ls_name, ac_name):
    """Top-level entry: show device type picker, then manage that device's segment map.

    Returns True if any segment map was created or modified.
    """
    ls_path = Path(ls_dir) if not isinstance(ls_dir, Path) else ls_dir
    modified = False

    while True:
        prefixes = scan_display_prefixes(ls_path, ac_name)

        if not prefixes:
            ui.header(f"Segment Maps \u2014 {ls_name}")
            print()
            ui.warn("No display fields found for this aircraft.")
            ui.info(f"{DIM}This aircraft has no CT_DISPLAY controls.{RESET}")
            input(f"\n  {DIM}Press Enter to continue...{RESET}")
            return modified

        ui.header(f"Segment Maps \u2014 {ls_name}")
        print()
        ui.info(f"  Aircraft: {CYAN}{ac_name}{RESET}")
        print()

        # Build picker options
        options = []
        descriptions = {}
        for p in prefixes:
            prefix = p["prefix"]
            if p["has_segmap"]:
                label = f"\u2713 {prefix}"
                cap = f"({p['entry_count']} entries)"
                desc_lines = [
                    f"{GREEN}{BOLD}{prefix}_SegmentMap.h{RESET} {DIM}\u2014 exists{RESET}",
                    f"{DIM}{p['entry_count']} segment map entries defined.{RESET}",
                    f"{DIM}{len(p['fields'])} display fields in aircraft JSON.{RESET}",
                ]
            else:
                label = f"  {prefix}"
                cap = "(no segment map)"
                desc_lines = [
                    f"{YELLOW}{BOLD}{prefix}_SegmentMap.h{RESET} {DIM}\u2014 missing{RESET}",
                    f"{DIM}No segment map file exists yet. Display fields{RESET}",
                    f"{DIM}for this device will be skipped during generation.{RESET}",
                    f"{DIM}{len(p['fields'])} display fields need mapping.{RESET}",
                ]
            options.append((f"{label}  {DIM}{cap}{RESET}", prefix))
            descriptions[prefix] = desc_lines

        # Ctrl+D delete handler — double confirmation
        def _on_delete(prefix_val):
            # Find the prefix info
            info = None
            for p in prefixes:
                if p["prefix"] == prefix_val:
                    info = p
                    break
            if info is None or not info["has_segmap"]:
                ui.warn("No segment map to delete for this device.")
                input(f"\n  {DIM}Press Enter to continue...{RESET}")
                return False
            fname = f"{prefix_val}_SegmentMap.h"
            fpath = info["segmap_path"]
            print()
            ui.warn(f"Delete {BOLD}{fname}{RESET}?")
            ui.info(f"{DIM}This will permanently remove the segment map file")
            ui.info(f"and all {info['entry_count']} entries inside it.{RESET}")
            print()
            ok1 = ui.confirm(f"Delete {fname}?", default_yes=False)
            if not ok1:
                ui.info("Cancelled.")
                time.sleep(0.4)
                return False
            ok2 = ui.confirm("Are you absolutely sure?", default_yes=False)
            if not ok2:
                ui.info("Cancelled.")
                time.sleep(0.4)
                return False
            os.remove(str(fpath))
            ui.success(f"Deleted {fname}")
            time.sleep(0.6)
            return True

        choice = ui.pick("Which display device?", options,
                         descriptions=descriptions, on_delete=_on_delete)
        if choice is None:
            return modified
        if choice == ui._DELETE_SENTINEL:
            # Always re-draw; only mark modified if a file was actually removed
            modified = True
            continue

        # Find the selected prefix info
        sel = None
        for p in prefixes:
            if p["prefix"] == choice:
                sel = p
                break

        if sel is None:
            continue

        if sel["has_segmap"]:
            result = _show_segment_map_entries(
                str(sel["segmap_path"]), sel["prefix"], sel["fields"],
                ls_name, ac_name
            )
            if result:
                modified = True
        else:
            result = _create_new_segment_map(
                ls_path, sel["prefix"], sel["fields"],
                ls_name, ac_name
            )
            if result:
                modified = True
