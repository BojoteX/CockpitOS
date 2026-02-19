# CockpitOS — Pending Implementation Items

> Items identified during the Docs_v2 documentation restructure.
> These are features that remain **manual** and should be automated.

---

## HIGH PRIORITY — Per-Label-Set Configuration Gaps

### 1. Move Latched Buttons to Per-Label-Set
**Current state:** `kLatchedButtons[]` is hardcoded in `Mappings.cpp:40-50` — shared across ALL label sets.

**Problem:** Every label set uses the same latched button list, even when the buttons are irrelevant to that aircraft/panel. Users must manually edit C++ to add/remove latched buttons.

**Solution:**
- Move `kLatchedButtons[]` into a per-label-set file (e.g., `LatchConfig.h` or extend `LabelSetConfig.h`)
- Add a **Latched Buttons editor** to the Label Creator — user toggles which controls should be latched from the InputMapping list
- Generator could pre-populate defaults from aircraft JSON if a `"latched": true` flag exists
- `Mappings.cpp` would `#include` the label-set-specific file instead of hardcoding

**Files affected:**
- `Mappings.cpp` — remove hardcoded `kLatchedButtons[]`, include per-label-set file
- `Mappings.h` — keep extern declarations, adjust includes
- `label_creator/label_creator.py` — add "Latched Buttons" menu item
- `_core/generator_core.py` — generate default latched list from JSON (optional)
- Each label set folder — new generated file

---

### 2. Move CoverGate Definitions to Per-Label-Set
**Current state:** `kCoverGates[]` is hardcoded in `Mappings.cpp:24-37` — shared across ALL label sets.

**Problem:** All panels share the same CoverGate definitions regardless of aircraft. Users must manually write C++ structs with timing values.

**Solution:**
- Move `kCoverGates[]` into a per-label-set file (e.g., `CoverGateConfig.h`)
- Add a **CoverGate editor** to the Label Creator:
  - Select action control (from InputMapping list)
  - Select release control (for Selector kind, from InputMapping list)
  - Select cover control (from InputMapping list)
  - Choose kind: Selector, ButtonMomentary, ButtonLatched
  - Set delay_ms and close_delay_ms (with sensible defaults)
- Generator could pre-populate known cover/action pairs from aircraft JSON
- `Mappings.cpp` would `#include` the label-set-specific file

**Files affected:**
- `Mappings.cpp` — remove hardcoded `kCoverGates[]`, include per-label-set file
- `Mappings.h` — keep `CoverGateDef` struct and extern declarations
- `label_creator/label_creator.py` — add "CoverGate" menu item
- `_core/generator_core.py` — generate default CoverGate config (optional)
- Each label set folder — new generated file

---

## MEDIUM PRIORITY — Setup Tool Additions

### 3. DCS-BIOS Installer
**Current state:** Users must manually download DCS-BIOS from GitHub and extract to `Saved Games\DCS\Scripts\DCS-BIOS\`.

**Solution:** Add to Setup Tool — auto-detect DCS install path, download latest release, extract.

---

### 4. HID Manager Dependency Installer
**Current state:** Users must manually `pip install hidapi filelock windows-curses ifaddr`.

**Solution:** Add to Setup Tool — detect missing deps, run pip install.

---

### 5. Expand Compiler Tool Misc Options
**Current state:** Several commonly-tuned Config.h settings require manual editing.

**Settings to add:**
- `POLLING_RATE_HZ` (125 / 250 / 500)
- `DISPLAY_REFRESH_RATE_HZ` (30 / 60)
- `TEST_LEDS` (on/off)
- `IS_REPLAY` (on/off)
- Hardware debug flags (`DEBUG_ENABLED_FOR_PCA_ONLY`, `HC165`, `TM1637`)
- Axis calibration thresholds (deadzone, min/max defaults)

---

## LOW PRIORITY — Nice to Have

### 6. Socat Setup for Serial Transport
**Current state:** Users must manually install socat from `Serial Manager/socat/`.

**Solution:** Add to Setup Tool when Serial transport is selected.

---

*Last updated during Docs_v2 documentation restructure.*
