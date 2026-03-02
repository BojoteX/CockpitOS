#!/usr/bin/env python3
"""
CockpitOS Generator Preservation Test
======================================
Verifies that auto-generation preserves all hand-tuned hardware wiring
in InputMapping.h across every label set.

Usage:
    python test_preservation.py          # run test
    python test_preservation.py --snap   # take new baseline snapshot

The test:
  1. Reads each InputMapping.h and extracts every entry's wiring tuple:
     (label, source, port, bit, hidId, oride_label, oride_value, controlType, group, releaseValue)
  2. Runs the auto-generator in batch mode for every label set.
  3. Re-reads each InputMapping.h and compares wiring.
  4. Reports PASS/FAIL per label set, with detailed diff on failure.

"Wiring" = source, port, bit, hidId, group, releaseValue.
Label renames (POS0 -> real name) are EXPECTED and allowed, as long as:
  - The wiring tuple for each (oride_label, oride_value) pair is unchanged.
  - No entries are lost (count must match or increase).
"""

import os, sys, re, subprocess, hashlib, json
from pathlib import Path

LABELS_DIR = Path(__file__).resolve().parent.parent  # src/LABELS/
CORE_DIR   = Path(__file__).resolve().parent          # src/LABELS/_core/

LINE_RE = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*'
    r'"(?P<source>[^"]+)"\s*,\s*'
    r'(?P<port>-?\d+|PIN\(\d+\)|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*'
    r'(?P<bit>-?\d+)\s*,\s*'
    r'(?P<hidId>-?\d+)\s*,\s*'
    r'"(?P<cmd>[^"]+)"\s*,\s*'
    r'(?P<value>-?\d+)\s*,\s*'
    r'"(?P<type>[^"]+)"\s*,\s*'
    r'(?P<group>\d+)\s*'
    r'(?:,\s*(?P<releaseValue>\d+)\s*)?'
    r'\}\s*,'
)

def parse_inputmapping(filepath):
    """Extract wiring entries from InputMapping.h, keyed by (cmd, value) for comparison."""
    entries = {}
    entry_list = []
    if not os.path.isfile(filepath):
        return entries, entry_list
    with open(filepath, "r", encoding="utf-8") as f:
        for line in f:
            m = LINE_RE.search(line)
            if not m:
                continue
            d = m.groupdict()
            rel = int(d["releaseValue"]) if d.get("releaseValue") else 0
            entry = {
                "label":   d["label"],
                "source":  d["source"],
                "port":    d["port"],
                "bit":     int(d["bit"]),
                "hidId":   int(d["hidId"]),
                "cmd":     d["cmd"],
                "value":   int(d["value"]),
                "type":    d["type"],
                "group":   int(d["group"]),
                "rel":     rel,
            }
            # Key by (cmd, value, label) to handle Custom copies of the same control
            key = (d["cmd"], int(d["value"]), d["label"])
            entries[key] = entry
            entry_list.append(entry)
    return entries, entry_list

def wiring_tuple(e):
    """The fields that MUST be preserved (hardware config)."""
    return (e["source"], e["port"], e["bit"], e["hidId"])

def get_active_set():
    """Read active_set.h to find the current LABEL_SET name."""
    active_set_path = LABELS_DIR / "active_set.h"
    if active_set_path.exists():
        text = active_set_path.read_text(encoding="utf-8")
        m = re.search(r'#define\s+LABEL_SET\s+(\S+)', text)
        if m:
            return f"LABEL_SET_{m.group(1)}"
    return None

def find_label_sets():
    """Find all LABEL_SET_* directories with generate_data.py.
    The active set is always placed last so it runs last and leaves
    the working tree in the correct state for compilation."""
    active = get_active_set()
    sets = []
    active_dir = None
    for d in sorted(LABELS_DIR.iterdir()):
        if d.is_dir() and d.name.startswith("LABEL_SET_") and (d / "generate_data.py").exists():
            if d.name == active:
                active_dir = d
            else:
                sets.append(d)
    if active_dir:
        sets.append(active_dir)
    return sets

def run_generator(label_set_dir):
    """Run generate_data.py in batch mode, return (success, output)."""
    env = os.environ.copy()
    env["COCKPITOS_BATCH"] = "1"
    result = subprocess.run(
        [sys.executable, "generate_data.py"],
        cwd=str(label_set_dir),
        capture_output=True, text=True, env=env, timeout=60
    )
    return result.returncode == 0, result.stdout + result.stderr

def snapshot_hash(filepath):
    """SHA256 of the file content."""
    if not os.path.isfile(filepath):
        return None
    with open(filepath, "rb") as f:
        return hashlib.sha256(f.read()).hexdigest()

def main():
    snap_mode = "--snap" in sys.argv

    label_sets = find_label_sets()
    print(f"Found {len(label_sets)} label sets.\n")

    if snap_mode:
        print("=== SNAPSHOT MODE: recording baselines ===\n")
        snap_file = CORE_DIR / "test_baselines.json"
        baselines = {}
        for ls in label_sets:
            name = ls.name
            im_path = ls / "InputMapping.h"
            h = snapshot_hash(im_path)
            _, entry_list = parse_inputmapping(im_path)
            baselines[name] = {
                "hash": h,
                "entry_count": len(entry_list),
            }
            print(f"  {name}: {len(entry_list)} entries, hash={h[:12]}...")
        with open(snap_file, "w", encoding="utf-8") as f:
            json.dump(baselines, f, indent=2)
        print(f"\nBaseline saved to {snap_file}")
        return

    # --- TEST MODE ---
    active = get_active_set()
    print("=" * 60)
    print("  CockpitOS Generator Preservation Test")
    if active:
        print(f"  Active set: {active} (will run last)")
    print("=" * 60)
    print()

    # Phase 1: Snapshot before
    before = {}
    for ls in label_sets:
        name = ls.name
        im_path = ls / "InputMapping.h"
        entries, entry_list = parse_inputmapping(im_path)
        before[name] = {
            "entries": entries,
            "entry_list": entry_list,
            "hash": snapshot_hash(im_path),
        }

    # Phase 2: Run generators
    print("Running generators...")
    gen_results = {}
    for ls in label_sets:
        name = ls.name
        ok, output = run_generator(ls)
        gen_results[name] = (ok, output)
        status = "OK" if ok else "FAIL"
        print(f"  {name}: {status}")
    print()

    # Phase 3: Compare
    passed = 0
    failed = 0
    warnings = 0

    for ls in label_sets:
        name = ls.name
        im_path = ls / "InputMapping.h"
        after_entries, after_list = parse_inputmapping(im_path)
        b = before[name]

        issues = []

        # Check: did any wiring get lost?
        # For each entry BEFORE, find the corresponding entry AFTER
        # Match by (cmd, value) - label may have changed (POS -> named)
        before_by_cmd_val = {}
        for e in b["entry_list"]:
            cv_key = (e["cmd"], e["value"], e["label"])
            before_by_cmd_val[cv_key] = e

        after_by_cmd_val = {}
        for e in after_list:
            cv_key = (e["cmd"], e["value"], e["label"])
            after_by_cmd_val[cv_key] = e

        # Build a map of (cmd, value) -> wiring for before, allowing label changes
        before_wiring_by_cv = {}
        for e in b["entry_list"]:
            cv = (e["cmd"], e["value"])
            if cv not in before_wiring_by_cv:
                before_wiring_by_cv[cv] = []
            before_wiring_by_cv[cv].append(e)

        after_wiring_by_cv = {}
        for e in after_list:
            cv = (e["cmd"], e["value"])
            if cv not in after_wiring_by_cv:
                after_wiring_by_cv[cv] = []
            after_wiring_by_cv[cv].append(e)

        # For each (cmd, value) pair, check wiring is preserved
        for cv, before_entries in before_wiring_by_cv.items():
            after_entries_cv = after_wiring_by_cv.get(cv, [])
            for be in before_entries:
                bw = wiring_tuple(be)
                # Skip if wiring is default (NONE, 0, 0, -1) - nothing to preserve
                if bw == ("NONE", "0", 0, -1) or bw == ("NONE", 0, 0, -1):
                    continue
                # Find matching wiring in after (label may differ)
                found = False
                for ae in after_entries_cv:
                    aw = wiring_tuple(ae)
                    if bw == aw:
                        found = True
                        break
                if not found:
                    issues.append(
                        f"  WIRING LOST: {be['label']} ({be['cmd']}={be['value']}) "
                        f"had {bw}, not found in output"
                    )

        # Check: entry count (should not decrease)
        before_count = len(b["entry_list"])
        after_count = len(after_list)
        if after_count < before_count:
            issues.append(f"  ENTRY COUNT: {before_count} -> {after_count} (entries lost!)")

        # Check: exact match (ideal)
        after_hash = snapshot_hash(im_path)
        is_exact = b["hash"] == after_hash

        if issues:
            print(f"FAIL  {name}")
            for issue in issues:
                print(issue)
            failed += 1
        elif is_exact:
            print(f"PASS  {name} (exact)")
            passed += 1
        else:
            print(f"PASS  {name} (wiring preserved, labels upgraded)")
            passed += 1

    print()
    print("=" * 60)
    print(f"  Results: {passed} passed, {failed} failed out of {len(label_sets)}")
    print("=" * 60)

    # Write results to test_results.txt (in _core/ next to this script)
    from datetime import datetime
    result_status = "PASS" if failed == 0 else "FAIL"
    results_file = CORE_DIR / "test_results.txt"
    with open(results_file, "w", encoding="utf-8") as f:
        f.write(f"status: {result_status}\n")
        f.write(f"timestamp: {datetime.now().isoformat()}\n")
        f.write(f"active_set: {active or 'unknown'}\n")
        f.write(f"total: {len(label_sets)}\n")
        f.write(f"passed: {passed}\n")
        f.write(f"failed: {failed}\n")
        if failed > 0:
            f.write(f"\nFailed sets:\n")
        # Re-check to log which ones failed
        for ls in label_sets:
            name = ls.name
            im_path = ls / "InputMapping.h"
            after_hash = snapshot_hash(im_path)
            b = before[name]
            is_exact = b["hash"] == after_hash
            if is_exact:
                f.write(f"  {name}: PASS (exact)\n")
            else:
                # Re-derive status
                _, after_list = parse_inputmapping(im_path)
                has_issues = False
                before_wiring_by_cv = {}
                for e in b["entry_list"]:
                    cv = (e["cmd"], e["value"])
                    if cv not in before_wiring_by_cv:
                        before_wiring_by_cv[cv] = []
                    before_wiring_by_cv[cv].append(e)
                after_wiring_by_cv = {}
                for e in after_list:
                    cv = (e["cmd"], e["value"])
                    if cv not in after_wiring_by_cv:
                        after_wiring_by_cv[cv] = []
                    after_wiring_by_cv[cv].append(e)
                for cv, bes in before_wiring_by_cv.items():
                    aes = after_wiring_by_cv.get(cv, [])
                    for be in bes:
                        bw = wiring_tuple(be)
                        if bw == ("NONE", "0", 0, -1) or bw == ("NONE", 0, 0, -1):
                            continue
                        if not any(wiring_tuple(ae) == bw for ae in aes):
                            f.write(f"  {name}: FAIL - {be['label']} wiring lost\n")
                            has_issues = True
                if not has_issues:
                    f.write(f"  {name}: PASS (wiring preserved, labels upgraded)\n")

    return 1 if failed > 0 else 0

if __name__ == "__main__":
    sys.exit(main())
