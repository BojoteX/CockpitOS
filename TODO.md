# CockpitOS — Pending Items

---

## VERY HIGH PRIORITY

### YouTube Video Tutorials
- Short-form videos (YouTube Shorts)
- Full walkthrough: zero to hero in 30 minutes
- Complex panel build with custom front panel (long-form video)

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
- DCS-BIOS installer — not needed, DCS-specific task left to the user
- Compiler Misc Options expansion — not needed, settings are stable defaults that don't need user-facing controls
- Version/build/date tracking — dropped

*Last updated 2026-02-19.*
