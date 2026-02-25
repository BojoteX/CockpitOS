# CockpitOS — Pending Items

---

## VERY HIGH PRIORITY

### YouTube Video Tutorials
- Short-form videos (YouTube Shorts)
- Full walkthrough: zero to hero in 30 minutes
- Complex panel build with custom front panel (long-form video)

### Misc
- Place holder, no idea whats NEXT

---

## Bug Fixes

### Generator exit code returns failure on success
- **File:** `src/LABELS/_core/generator_core.py` line 1613
- **Symptom:** `sys.exit(1)` is called unconditionally after a successful run. In batch mode (`COCKPITOS_BATCH=1`), CI interprets a successful generation as a failure.
- **Fix:** Change `sys.exit(1)` to `sys.exit(0)` at end of successful `run()` path.
- **Severity:** Minor — only affects CI/batch workflows, not interactive use (the `_pause()` before it is already correctly skipped in batch mode).

---

## Code Quality Enhancements

### HID axis arrays use hardcoded [40] instead of HID_AXIS_COUNT
- **File:** `src/Core/HIDManager.cpp` lines 379-382, 584
- **Symptom:** Four filter/state arrays are declared as `[40]` while `HID_AXIS_COUNT` is 16. The reset loop also iterates to 40. No crash or overrun since arrays are oversized, but wastes 96 bytes of static RAM and the reset loop does 24 extra no-op iterations.
- **Fix:** Replace `[40]` with `[HID_AXIS_COUNT]` in declarations; use `HID_AXIS_COUNT` as the loop bound in the reset loop.
- **Severity:** Negligible — consistency improvement, no runtime impact.

---

## DONE — CI/CD Automated Build Testing

GitHub Actions workflow (`.github/workflows/ci-build.yml`) compiles all 16 label sets against ESP32-S3 on every push to `dev` and daily at 06:00 UTC. Produces per-label-set `warnings.log` and `errors.log` as downloadable artifacts. Warnings are classified into CockpitOS code vs. framework/SDK, matching the compiler tool's format.

---

## Completed

All previous engineering TODO items have been resolved:

- Per-label-set Latched Buttons and CoverGates — done
- CustomPins.h TUI editor — done
- HID Manager dependency installer — done
- RS485 (all items: core isolation, timing, slave performance, HID passthrough) — done
- DCS-BIOS feature gap analysis — confirmed complete, no missing features
- Performance optimization — at peak efficiency
- Magnetic switch support — implemented from design document
- SPI TFT regression (freq_read) — fixed
- DCS-BIOS installer — done (Setup tool downloads, installs, patches Export.lua, compiles LUAs to JSONs)
- DCS-BIOS aircraft selector filtering — done (filterable picker with type-to-filter, viewport scrolling)
- Compiler Misc Options expansion — not needed, settings are stable defaults that don't need user-facing controls
- Version/build/date tracking — dropped
- Custom.json TUI editor — done (custom_editor.py: create/edit/delete custom controls, type-to-filter control browser, Tab toggle selected/all panels, full input/output editing)
- METADATA seeding — done (Setup copies metadata JSONs to _core/metadata/, label_set.create_label_set() seeds METADATA/ dir with metadata JSONs + empty Custom.json, wipe cleans metadata)
- Generator group ID fix for Custom controls — done (selector_entries 7th field carries JSON key; PATCH block and alloc_group key on ident so custom copies get separate group IDs)
- Generator output/LED dedup for Custom controls — done (custom copies skipped from output_entries since they share the same DCS-BIOS address; output editing removed from custom_editor.py)

*Last updated 2026-02-21.*
