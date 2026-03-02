# Known Issues & Upstream Warnings

Critical issues from upstream dependencies that affect CockpitOS. Review before modifying generators or updating DCS-BIOS.

---

## DCS-BIOS v0.11.1 - Positions Array Breaking Change

**Date:** 2026-02-28 (v0.11.1 release)
**PRs:** [#1571](https://github.com/DCS-Skunkworks/dcs-bios/pull/1571), [#1572](https://github.com/DCS-Skunkworks/dcs-bios/pull/1572)
**Issue filed:** [#1596](https://github.com/DCS-Skunkworks/dcs-bios/issues/1596)

### What changed

DCS-BIOS moved selector position names from the `description` string to a new `positions` array at the control level:

```
v0.10:  "description": "Mode Selector Switch, MAN/OFF/AUTO"
v0.11+: "description": "Mode Selector Switch", "positions": ["MAN", "OFF", "AUTO"]
```

37 aircraft modules were migrated in PR #1572. Charlie (charliefoxtwo) noted ~1300 more controls will be backfilled in future releases.

### Impact on generator_core.py

The generator derives selector label suffixes (e.g., `MODE_SELECTOR_SW_AUTO`) from position names. Without them, it falls back to generic `POS0`, `POS1`, `POS2`. Since the label-keyed preservation mechanism matches by **full label name**, a rename from `_AUTO` to `_POS0` (or vice versa) causes a label mismatch, which **destroys all hand-tuned hardware wiring** (source, port, bit, hidId reset to NONE/0/0/-1).

### Fixes applied (2026-03-01)

Two changes to `src/LABELS/_core/generator_core.py`:

1. **Step 4.0 - Positions array reader** (before the slash-split): Reads the `positions` array from v0.11+ JSON first, falls back to slash-split for v0.10 and earlier. The array is reversed to match DCS value order (same convention as the existing slash-split).

2. **POS-fallback in merge logic**: When a new label (e.g., `CABIN_PRESS_SW_NORM`) is not found in the existing InputMapping.h, the merge tries the POS-equivalent (`CABIN_PRESS_SW_POS2`) for the same control+value. If found, wiring is preserved and the label is upgraded to the new name.

### Ongoing risk

Every time DCS-BIOS backfills more controls with `positions` arrays, previously-POS-labeled entries will get renamed. The POS-fallback handles this transparently, but only if the generator code is not refactored in a way that removes it. **Do not remove the POS-fallback logic** until all DCS-BIOS controls have been migrated.

### Firmware references to POS labels

Some firmware files reference POS-style labels directly:
- `src/Panels/WingFold.cpp` - hardcodes `WING_FOLD_PULL_POS0`, `WING_FOLD_PULL_POS1`
- `src/LABELS/LABEL_SET_LEFT_PANEL_CONTROLLER/CoverGates.h` - references `GAIN_SWITCH_POS0`, `GAIN_SWITCH_POS1`

If DCS-BIOS adds `positions` arrays for these controls, the generator will rename the labels but the hardcoded references will break. These must be updated manually when that happens.

---
