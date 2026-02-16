# Session Changelog — 2026-02-15
## Panel_ST77916 Review Implementation + Inversion Fix

### Context
A formal 25-finding code review was performed by another LLM on two files. 12 findings were accepted for implementation, 13 were deferred or rejected. After implementing the 12 findings, a color inversion bug was discovered and fixed. The regular (non-QSPI) SPI TFT panels are now experiencing hard freezes during setup — they never reach loop(). This changelog documents every change made so another LLM can diagnose the regression.

### Files Modified
1. `src/Panels/includes/Panel_ST77916.hpp` — Custom QSPI panel driver (inherits Panel_AMOLED)
2. `src/Panels/TFT_Gauges_CabPressTest.cpp` — Cabin pressure gauge (dual SPI/QSPI support)

### Important Architecture Notes
- `TFT_Gauges_CabPressTest.cpp` supports TWO bus modes selected by `#define CABIN_PRESSURE_USE_QSPI`:
  - `CABIN_PRESSURE_USE_QSPI == 1`: Uses `Panel_ST77916` (inherits `Panel_AMOLED`) for QSPI
  - `CABIN_PRESSURE_USE_QSPI == 0`: Uses `Panel_ST77961` (inherits `Panel_LCD`) for standard SPI
- Other TFT gauge files (e.g. `TFT_Gauges_CabinPressure.cpp`, `TFT_Gauges_RadarAlt.cpp`, etc.) use standard SPI with `Panel_ST77961` and were NOT directly modified in this session
- However, if any of those other gauge files share code patterns, compilation units, or global state with `TFT_Gauges_CabPressTest.cpp`, they could be affected indirectly

---

## Changes to Panel_ST77916.hpp (QSPI-only file)

All changes below are inside `struct Panel_ST77916 : public Panel_AMOLED` and only affect QSPI builds. They should have ZERO impact on standard SPI panels since this file is only included when `CABIN_PRESSURE_USE_QSPI == 1`.

### Change 1 — Header doc comment updated (Finding F-01)
- **What**: Updated doc comment (lines 35-41) to state that `writeImage()` is intentionally NOT overridden
- **Old text**: listed writeImage as one of the overrides
- **New text**: explains writeImage is not overridden because the gauge pipeline uses setAddrWindow + pushPixelsDMA
- **Impact**: Comment only, no behavioral change

### Change 2 — "FOR IMPLEMENTERS" section updated (Finding F-01)
- **What**: Changed step 5 (lines 71-72) from listing writeImage as a required override to saying it's usually unnecessary
- **Impact**: Comment only, no behavioral change

### Change 3 — Removed writeImage() override (Finding F-01)
- **What**: Deleted the entire 80-line `writeImage()` method override and replaced with a comment block explaining why it's intentionally not overridden
- **Lines removed**: Previously lines 192-302 (the full writeImage method body)
- **Lines added**: 7-line comment block (lines 186-194) explaining the intentional omission
- **Why**: Dead code — gauge pipeline never calls writeImage
- **Impact on SPI panels**: NONE — this method only existed inside Panel_ST77916

### Change 4 — Added COLMOD (0x3A) to init sequence (Finding F-02)
- **What**: Added `0x3A, 1, 0x55` (COLMOD = RGB565) to the vendor init command list in `getInitCommands()`, before the SLPOUT/DISPON block
- **Location**: Line 602-603 in getInitCommands() static array
- **Why**: The init sequence was missing the pixel format command; the display was relying on power-on defaults
- **Impact on SPI panels**: NONE — getInitCommands() only runs for Panel_ST77916

### Change 5 — Fixed setColorDepth() to send COLMOD command to hardware (Finding F-02)
- **What**: Added code inside `setColorDepth()` override to send COLMOD (0x3A) command to the display via the bus when `_bus` is available
- **Old behavior**: Only updated internal `_write_depth`/`_read_depth` state, never communicated to hardware
- **New behavior**: After updating state, if `_bus != nullptr`, sends `startWrite()`, `write_cmd(0x3A)`, `_bus->writeData(colmod, 8)`, `endWrite()`
- **Impact on SPI panels**: NONE — only inside Panel_ST77916

### Change 6 — Added _cfg.rgb_order = true to constructor (Finding F-03)
- **What**: Added `_cfg.rgb_order = true;` in Panel_ST77916 constructor
- **Why**: Makes panel self-describing (Waveshare ESP32-S3-LCD-1.85 uses BGR subpixel order)
- **Impact on SPI panels**: NONE — only inside Panel_ST77916 constructor

### Change 7 — Changed _cfg.invert to _invert in constructor (Finding F-03 + inversion bugfix)
- **What**: Originally added `_cfg.invert = true;` per F-03. Then changed it to `_invert = true;` to fix the inverted colors bug.
- **Current code** (lines 135-140):
  ```cpp
  // Inversion: Panel_AMOLED::setInvert() ignores _cfg.invert (unlike
  // Panel_LCD which XORs).  Instead, LGFXBase::init_impl() calls
  // setInvert(getInvert()) which reads the _invert member directly.
  // So we set _invert = true here.  The actual INVON command is sent
  // by init_impl AFTER getInitCommands() runs — NOT in the init sequence.
  _invert = true;
  ```
- **Why**: Panel_AMOLED::setInvert() does NOT use `_cfg.invert` at all (unlike Panel_LCD which XORs `invert ^ _cfg.invert`). LGFXBase::init_impl() calls `invertDisplay(_panel->getInvert())` which reads the `_invert` member directly. If `_invert` is false (default), init_impl sends INVOFF, undoing any INVON from the init sequence.
- **Impact on SPI panels**: NONE — only inside Panel_ST77916 constructor

### Change 8 — Removed INVON (0x21) from init sequence (inversion bugfix)
- **What**: Removed the `0x21, 0,  // INVON` line from getInitCommands()
- **Current code** (lines 605-609):
  ```cpp
  // NOTE: No INVON here — inversion is handled by LGFXBase::init_impl()
  // which calls setInvert(getInvert()) after this sequence completes.
  // We set _invert = true in the constructor so INVON is sent once, correctly.
  // Putting INVON here would cause a double-invert (our INVON + init_impl's INVOFF).
  ```
- **Why**: Double-inversion. The init sequence sent INVON, then init_impl called setInvert(false) (because _invert was false by default) which sent INVOFF, undoing it. Now _invert = true in constructor → init_impl sends setInvert(true) → INVON once, correctly.
- **Impact on SPI panels**: NONE — only inside Panel_ST77916::getInitCommands()

### Change 9 — Improved RASET pre-clear comment (Finding F-04)
- **What**: Expanded comment (lines 588-592) explaining why RASET is set to a single row before the RAMCLACT clear
- **Impact**: Comment only

### Change 10 — Added setWindow bounds comment (Finding F-05)
- **What**: Added comment (lines 153-154) noting _width/_height are post-rotation logical dimensions
- **Impact**: Comment only

---

## Changes to TFT_Gauges_CabPressTest.cpp

**IMPORTANT**: Some changes below are in SHARED code paths (outside `#if CABIN_PRESSURE_USE_QSPI` guards) and affect BOTH SPI and QSPI builds.

### Change 11 — Fixed duplicate preprocessor condition (Finding F-09)
- **Line**: 28
- **Old**: `#if defined(HAS_CABIN_PRESSURE_GAUGE) || defined(HAS_CABIN_PRESSURE_GAUGE)`
- **New**: `#if defined(HAS_CABIN_PRESSURE_GAUGE)`
- **Why**: Duplicate — same macro on both sides of `||`
- **Impact**: No behavioral change (true||true == true), but this line controls `REGISTER_PANEL` macro invocation
- **SHARED CODE**: Yes, but logically identical

### Change 12 — Changed freq_read from 16000000 to 0 (Finding F-14)
- **Line**: 180
- **Old**: `cfg.freq_read = 16000000;`
- **New**: `cfg.freq_read = 0;         // No read-back; QSPI is write-only on this bus`
- **Why**: QSPI bus doesn't support read-back
- **SHARED CODE**: **YES — THIS IS IN THE SHARED BUS CONFIG BLOCK**, outside any `#if CABIN_PRESSURE_USE_QSPI` guard. It applies to BOTH SPI and QSPI builds of this file.
- **POTENTIAL RISK**: If LovyanGFX uses freq_read internally for anything during init (timing, dummy clocks, panel identification, etc.), setting it to 0 could cause issues on SPI panels. The panel config does set `pcfg.readable = false` (line 207), but freq_read=0 vs freq_read=16000000 may affect bus initialization behavior.
- **NOTE**: This change ONLY affects TFT_Gauges_CabPressTest.cpp. Other gauge files that have their own LGFX_Device classes with their own bus configs are NOT affected.

### Change 13 — Removed commented-out Rect struct (Finding F-23)
- **Lines removed**: Previously lines 273-277 (a 5-line commented-out struct)
- **What was removed**:
  ```cpp
  /*
  struct Rect {
      int16_t x = 0, y = 0, w = 0, h = 0;
  };
  */
  ```
- **Why**: Dead commented-out code
- **Impact**: None — was already commented out

### Change 14 — Simplified CABPRESS_CPU_CORE define (Finding F-24)
- **Lines**: 42-43
- **Old**:
  ```cpp
  #if defined(ESP_FAMILY_S3)
  #define CABPRESS_CPU_CORE 0
  #else
  #define CABPRESS_CPU_CORE 0
  #endif
  ```
- **New**:
  ```cpp
  // Select core (currently pinned to core 0 for all variants)
  #define CABPRESS_CPU_CORE 0
  ```
- **Why**: Both branches had identical value
- **Impact**: None — same value either way

### Change 15 — Added needsFullFlush to stateChanged check (Finding F-25)
- **Line**: 412
- **Old**: `const bool stateChanged = gaugeDirty || (u != lastDrawnAngleU);`
- **New**: `const bool stateChanged = gaugeDirty || needsFullFlush || (u != lastDrawnAngleU);`
- **Why**: A pending full flush (from lighting mode change) could be skipped if angle hadn't changed and gaugeDirty was already cleared
- **SHARED CODE**: Yes — this is in `CabinPressureGauge_draw()` which runs for both SPI and QSPI
- **Risk**: Low — this is a correctness improvement. Prevents early return when a full repaint is pending. Should not cause hangs.

### Change 16 — Added cooperative shutdown TODO comment (Finding F-15)
- **Lines**: 610-613
- **What**: Added TODO comment above `vTaskDelete` in `CabinPressureGauge_deinit()`
- **Impact**: Comment only, no behavioral change

### Change 17 — Improved QSPI bandwidth comment (Finding F-16/F-20)
- **Line**: 152
- **Old**: `// 40 MHz for QSPI (conservative; try 80 MHz once confirmed working)`
- **New**: `// 40 MHz QSPI → 160 Mbit/s (4 data lines). Try 80 MHz → 320 Mbit/s once stable.`
- **Impact**: Comment only, inside `#if CABIN_PRESSURE_USE_QSPI`

### Change 18 — Updated debug printf to show Mbit/s (Finding F-16/F-20)
- **Lines**: 554-557
- **Old format**: `"QSPI mode @ %u MHz"`
- **New format**: `"QSPI @ %u MHz, %u Mbit/s"` with added `(unsigned)(freq_write / 1000000u * 4u)` argument
- **Impact**: Inside `#if CABIN_PRESSURE_USE_QSPI`, debug output only

### Change 19 — Made pcfg.invert/pcfg.rgb_order conditional on bus mode (inversion bugfix)
- **Lines**: 219-231
- **Old**:
  ```cpp
  pcfg.bus_shared     = shared_bus;
  pcfg.invert         = true;
  pcfg.rgb_order      = true;
  _panel.config(pcfg);
  ```
- **New**:
  ```cpp
  pcfg.bus_shared     = shared_bus;
  #if CABIN_PRESSURE_USE_QSPI
              // QSPI (Panel_ST77916 → Panel_AMOLED): invert and rgb_order are set
              // in the Panel_ST77916 constructor.  Panel_AMOLED ignores _cfg.invert —
              // inversion is controlled via _invert member + init_impl()'s setInvert().
  #else
              // SPI (Panel_ST77961 → Panel_LCD): Panel_LCD XORs _cfg.invert in setInvert()
              pcfg.invert         = true;
              pcfg.rgb_order      = true;
  #endif
  _panel.config(pcfg);
  ```
- **Why**: For Panel_AMOLED derivatives, _cfg.invert is ignored; inversion is controlled via the _invert member. Setting _cfg.invert was a misleading no-op.
- **Impact on SPI panels**: The `#else` branch preserves the original behavior exactly — SPI still gets `pcfg.invert = true` and `pcfg.rgb_order = true`.
- **HOWEVER**: There is a subtle issue. The old code set these values unconditionally BEFORE `_panel.config(pcfg)`. The new code still sets them before `_panel.config(pcfg)` in the `#else` branch. BUT — if for some reason `CABIN_PRESSURE_USE_QSPI` is defined as 1 on a build that is actually using SPI hardware, the invert/rgb_order would NOT be set, potentially causing display issues.

---

## Changes NOT Made by This Session (user/linter modifications noted)

The system reported that both files were modified by the user or a linter. Notable differences from the system-reminder diffs:

### In TFT_Gauges_CabPressTest.cpp:
- Line 304: Array initializers changed from `{ -px, (float)w - px, ... }` to `{ (float)-px, (float)w - px, ... }` — explicit float casts added. This was NOT done by this session; likely a linter fix for implicit conversion warnings.
- Line 626: `#else` changed to `#elif defined(HAS_CABIN_PRESSURE_GAUGE) && defined(ENABLE_TFT_GAUGES) && (ENABLE_TFT_GAUGES == 1)` — the fallback warning condition was tightened. This was NOT done by this session.

---

## Summary: Changes That Could Affect SPI Panels

| Change # | Description | In shared code? | Risk Level |
|----------|-------------|-----------------|------------|
| 12 | freq_read = 16000000 → 0 | **YES** | **HIGH** — could affect SPI bus init |
| 15 | needsFullFlush added to stateChanged | YES | LOW — correctness fix |
| 19 | pcfg.invert/rgb_order made conditional | Partially (#else preserves SPI) | LOW |
| 11 | Duplicate preprocessor fixed | YES but logically identical | NONE |
| 14 | CABPRESS_CPU_CORE simplified | YES but same value | NONE |
| All others | QSPI-only or comments | NO | NONE |

### Most Likely Cause of SPI Regression: Change 12 (freq_read = 0)
Setting `cfg.freq_read = 0` in the shared bus config could cause LovyanGFX's SPI bus initialization to behave differently. Even though `pcfg.readable = false` is set, the bus layer may still use freq_read during setup (e.g., for SPI mode configuration, dummy clock generation, or bus speed negotiation). A value of 0 could cause a division-by-zero, an infinite timeout, or prevent the SPI peripheral from initializing correctly, leading to a hard freeze during `tft.init()`.

### Recommended First Fix to Try
Revert Change 12: set `cfg.freq_read` back to `16000000`, or make it conditional:
```cpp
#if CABIN_PRESSURE_USE_QSPI
    cfg.freq_read = 0;          // QSPI is write-only
#else
    cfg.freq_read = 16000000;   // SPI read-back clock
#endif
```

### Other Possible Causes
- If the freezing panels are NOT using `TFT_Gauges_CabPressTest.cpp` at all (they use different gauge files like `TFT_Gauges_CabinPressure.cpp`, `TFT_Gauges_RadarAlt.cpp`, etc.), then NONE of the changes in this session should be responsible, and the issue is elsewhere (compilation order, shared headers, global state, etc.).
- The user/linter change to line 626 (`#else` → `#elif ...`) could also affect compilation — if the condition is not met, the `#warning` is suppressed and the `#endif` matching could shift.
