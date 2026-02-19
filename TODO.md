# CockpitOS — Pending Implementation Items

> Items identified during the Docs_v2 documentation restructure.
> These are features that remain **manual** and should be automated.

---

## ~~HIGH PRIORITY — Per-Label-Set Configuration Gaps~~ DONE

### 1. ~~Move Latched Buttons to Per-Label-Set~~ DONE
Moved `kLatchedButtons[]` from `Mappings.cpp` into per-label-set `LatchedButtons.h`. Extracted `CoverGateDef` struct/enum to `src/Core/CoverGateDef.h`. `LabelSetSelect.h` uses `__has_include` with empty fallback for label sets that don't have the file. Generator creates empty defaults on first run. Label Creator has a "Latched Buttons" toggle-list editor (`latched_editor.py`).

---

### 2. ~~Move CoverGate Definitions to Per-Label-Set~~ DONE
Moved `kCoverGates[]` from `Mappings.cpp` into per-label-set `CoverGates.h`. Same `__has_include` fallback pattern. Label Creator has a "Cover Gates" list editor with add/edit/delete (`covergate_editor.py`). Reset tool wipes both files. LABEL_SET_MAIN populated with the original data from Mappings.cpp.

---

## MEDIUM PRIORITY — Setup Tool Additions

### 3. DCS-BIOS Installer
**Current state:** Users must manually download DCS-BIOS from GitHub and extract to `Saved Games\DCS\Scripts\DCS-BIOS\`.

**Solution:** Add to Setup Tool — auto-detect DCS install path, download latest release, extract.

---

### 4. ~~HID Manager Dependency Installer~~ DONE
Added as Step 4 in `Setup-START.py` `action_setup()` flow. Auto-detects missing pip packages and installs them. Also handled by "Reset to recommended versions".

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

## ~~LABEL CREATOR — CustomPins.h Editor~~ DONE

### 6. ~~Intelligent CustomPins.h Editor~~ DONE
Replaced the "open in Notepad" approach with a TUI editor (`custompins_editor.py`). Scans InputMapping.h and LEDMapping.h to detect which hardware devices are in use (PCA9555, HC165, TM1637, WS2812, etc.), then presents a grouped pin configuration screen with contextual warnings when required pins are missing. Known groups: Feature Enables, I2C Bus, HC165, WS2812, TM1637, Mode Switch, RS485. Unknown defines preserved verbatim. Auto-suggests HC165_BITS from detected input count. Feature enables warn when detection mismatches setting. TFT gauge / IFEI / ALR-67 pin groups deferred to v2.

---

*Last updated during CustomPins.h editor implementation.*
