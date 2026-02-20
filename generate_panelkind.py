#!/usr/bin/env python3
"""
generate_panelkind.py — Auto-generate PanelKind.h from src/Panels/*.cpp

This script scans all .cpp files in src/Panels/ and generates an enum class
with one entry per panel. The PanelKind identifier is determined by:

  1. If the file contains "// PANEL_KIND: XXX" in the first 30 lines, use XXX
  2. Otherwise, use the filename without .cpp extension

Additionally, permanent entries for auto-detected PCA panels are always included
(Brain, ECM, MasterARM) since these are runtime-detected via I²C scan and don't
have REGISTER_PANEL macros.

IMPORTANT: Place this script in the CockpitOS root directory.
           You can run it from anywhere once it's there.

Usage:
    python generate_panelkind.py

Output:
    Creates/overwrites: <root>/src/Generated/PanelKind.h
"""

import os
import sys
import re
from pathlib import Path

# Ensure stdout/stderr can handle Unicode (→, ✓, etc.) even when
# piped or redirected on Windows (where the default codec may be cp1252).
if sys.stdout.encoding and sys.stdout.encoding.lower().replace("-", "") != "utf8":
    import io
    sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding="utf-8", errors="replace")
    sys.stderr = io.TextIOWrapper(sys.stderr.buffer, encoding="utf-8", errors="replace")

# Change to the directory where THIS script is located (should be CockpitOS root)
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(SCRIPT_DIR)

# Configuration (paths relative to script location)
PANELS_DIR = "src/Panels"
OUTPUT_FILE = "src/Generated/PanelKind.h"

# Verify we're in the right place
if not os.path.isdir(PANELS_DIR):
    print(f"ERROR: Cannot find '{PANELS_DIR}' directory.")
    print(f"       Script location: {SCRIPT_DIR}")
    print()
    print("This script must be placed in the CockpitOS ROOT directory.")
    print("Expected structure:")
    print("  CockpitOS/")
    print("    ├── generate_panelkind.py   <-- script goes HERE")
    print("    └── src/")
    print("        └── Panels/")
    print("            ├── Generic.cpp")
    print("            ├── IFEIPanel.cpp")
    print("            └── ...")
    sys.exit(1)
METADATA_PATTERN = re.compile(r'^\s*//\s*PANEL_KIND:\s*(\w+)', re.IGNORECASE)
SCAN_LINES = 30  # Only scan first N lines for metadata

# =============================================================================
# PERMANENT ENTRIES — Runtime-detected panels (no REGISTER_PANEL)
# PCA9555 panels are now fully data-driven from InputMapping/LEDMapping,
# so Brain, ECM, MasterARM are no longer needed here.
# Format: (identifier, comment)
# =============================================================================
PERMANENT_PCA_ENTRIES = [
    ("AnalogGauge", "Auto-detected when DEVICE_GAUGE in LEDMapping"),
]

def find_panel_kind(filepath: Path) -> tuple[str, str]:
    """
    Extract PanelKind identifier from a .cpp file.
    
    Returns:
        (identifier, source) where source is 'metadata' or 'filename'
    """
    filename_id = filepath.stem  # filename without .cpp
    
    try:
        with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
            for i, line in enumerate(f):
                if i >= SCAN_LINES:
                    break
                match = METADATA_PATTERN.match(line)
                if match:
                    return (match.group(1), 'metadata')
    except Exception as e:
        print(f"  Warning: Could not read {filepath}: {e}")
    
    return (filename_id, 'filename')


def scan_panels(panels_dir: Path) -> list[tuple[str, str, str]]:
    """
    Scan all .cpp files in the panels directory.
    
    Returns:
        List of (identifier, source, filename) tuples, sorted by identifier
    """
    results = []
    
    if not panels_dir.exists():
        print(f"Error: Panels directory not found: {panels_dir}")
        sys.exit(1)
    
    cpp_files = sorted(panels_dir.glob("*.cpp"))
    
    if not cpp_files:
        print(f"Warning: No .cpp files found in {panels_dir}")
        return results
    
    print(f"Scanning {len(cpp_files)} panel files...")
    
    for cpp_file in cpp_files:
        identifier, source = find_panel_kind(cpp_file)
        results.append((identifier, source, cpp_file.name))
        
        # Show what we found
        if source == 'metadata':
            print(f"  {cpp_file.name:40} → {identifier:20} (from PANEL_KIND)")
        else:
            print(f"  {cpp_file.name:40} → {identifier:20} (from filename)")
    
    # Sort by identifier for consistent output
    results.sort(key=lambda x: x[0])
    
    return results


def check_duplicates(panels: list[tuple[str, str, str]], permanent: list[tuple[str, str]]) -> bool:
    """Check for duplicate PanelKind identifiers."""
    seen = {}
    has_duplicates = False
    
    # Check permanent entries first
    for identifier, comment in permanent:
        seen[identifier] = f"(permanent: {comment})"
    
    # Check scanned panels
    for identifier, source, filename in panels:
        if identifier in seen:
            print(f"ERROR: Duplicate PanelKind '{identifier}':")
            print(f"       - {seen[identifier]}")
            print(f"       - {filename}")
            has_duplicates = True
        else:
            seen[identifier] = filename
    
    return has_duplicates


def generate_header(panels: list[tuple[str, str, str]], permanent: list[tuple[str, str]], output_path: Path) -> None:
    """Generate the PanelKind.h header file."""
    
    # Ensure output directory exists
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    lines = [
        "// =============================================================================",
        "// PanelKind.h — AUTO-GENERATED by generate_panelkind.py",
        "// DO NOT EDIT MANUALLY — Changes will be overwritten!",
        "//",
        "// To change a panel's PanelKind identifier, add this comment to the .cpp file:",
        "//     // PANEL_KIND: YourIdentifier",
        "//",
        "// If no PANEL_KIND comment is present, the filename (minus .cpp) is used.",
        "// =============================================================================",
        "",
        "#pragma once",
        "#include <stdint.h>",
        "",
        "enum class PanelKind : uint8_t {",
        "",
        "    // ===== Auto-detected PCA panels (permanent, no REGISTER_PANEL) =====",
    ]
    
    # Add permanent PCA entries first
    for identifier, comment in permanent:
        lines.append(f"    {identifier},{' ' * max(1, 24 - len(identifier))}// {comment}")
    
    lines.append("")
    lines.append("    // ===== Compiled panels (auto-generated from src/Panels/*.cpp) =====")
    
    # Add each scanned panel entry with source comment
    for identifier, source, filename in panels:
        comment = f"from {filename}" if source == 'filename' else f"PANEL_KIND in {filename}"
        lines.append(f"    {identifier},{' ' * max(1, 24 - len(identifier))}// {comment}")
    
    # Add COUNT sentinel
    lines.append("")
    lines.append(f"    COUNT{' ' * 20}// Must always be last")
    lines.append("};")
    lines.append("")
    
    # Add static_assert for mask size
    lines.append("// Ensure we don't exceed bitmask capacity")
    lines.append("static_assert(static_cast<uint8_t>(PanelKind::COUNT) <= 32,")
    lines.append('              "PanelKind exceeds 32 entries — switch activeMask to uint64_t");')
    lines.append("")
    
    # Write file
    with open(output_path, 'w', encoding='utf-8', newline='\n') as f:
        f.write('\n'.join(lines))
    
    total_count = len(permanent) + len(panels)
    print(f"\nGenerated: {output_path}")
    print(f"  → {len(permanent)} permanent PCA entries")
    print(f"  → {len(panels)} compiled panel entries")
    print(f"  → {total_count} total + COUNT")


def main():
    root = Path(SCRIPT_DIR)
    panels_dir = root / PANELS_DIR
    output_path = root / OUTPUT_FILE
    
    print("=" * 60)
    print("CockpitOS PanelKind Generator")
    print("=" * 60)
    print(f"Root:   {root}")
    print(f"Panels: {panels_dir}")
    print(f"Output: {output_path}")
    print()
    
    # Show permanent entries
    print(f"Permanent PCA entries ({len(PERMANENT_PCA_ENTRIES)}):")
    for identifier, comment in PERMANENT_PCA_ENTRIES:
        print(f"  {identifier:24} — {comment}")
    print()
    
    # Scan panels
    panels = scan_panels(panels_dir)
    
    if not panels:
        print("No panel .cpp files found. Exiting.")
        sys.exit(1)
    
    # Check for duplicates (including permanent entries)
    if check_duplicates(panels, PERMANENT_PCA_ENTRIES):
        print("\nAborting due to duplicate identifiers.")
        sys.exit(1)
    
    # Generate header
    generate_header(panels, PERMANENT_PCA_ENTRIES, output_path)
    
    print("\nDone!")


if __name__ == "__main__":
    main()
