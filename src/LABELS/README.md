# CockpitOS Label Sets

This directory controls **which panels and controls are compiled into your firmware** for a specific aircraft, instrument set, or cockpit build. Each label set is an independent configuration enabling multiple hardware-specific or aircraft-specific firmware targets.

---

## Directory Structure

```
LABELS/
├── _core/                          Generator modules and aircraft JSON definitions
│   ├── aircraft/                   Aircraft JSON source files (FA-18C_hornet.json, etc.)
│   ├── generator_core.py           Core generation engine
│   ├── display_gen_core.py         Display mapping generator
│   └── reset_core.py              Label set reset/initialization
├── active_set.h                    Points to the currently active label set
├── LABEL_SET_MAIN/                 Example label set
├── LABEL_SET_ALR67/
├── LABEL_SET_IFEI/
└── (etc; each set is a subdirectory)
```

---

## What a Label Set Contains

Each `LABEL_SET_*` directory holds a complete build configuration:

| File | Generated | User-Editable | Purpose |
|------|:---------:|:-------------:|---------|
| `selected_panels.txt` | Yes | Yes | Which panels to include in this build |
| `panels.txt` | Yes | No | All available panels from the aircraft definition |
| `aircraft.txt` | Yes | No | Which aircraft JSON this set targets |
| `InputMapping.h` | Yes | Yes | Input definitions (buttons, switches, encoders) |
| `LEDMapping.h` | Yes | Yes | Output definitions (LEDs, gauges, indicators) |
| `DisplayMapping.cpp` / `.h` | Yes | Yes | Display field-to-segment-map linkage |
| `*_SegmentMap.h` | Manual | Yes | HT1622 RAM-to-segment mapping (if displays used) |
| `DCSBIOSBridgeData.h` | Yes | No | Auto-generated hash tables for DCS-BIOS |
| `CT_Display.h` / `.cpp` | Yes | No | Compiled display type definitions |
| `CustomPins.h` | Yes | Yes | Pin assignments and feature enables |
| `LabelSetConfig.h` | Yes | Yes | Device name, HID settings |
| `LatchedButtons.h` | Yes | Yes | Latched button declarations |
| `CoverGates.h` | Yes | Yes | Cover gate definitions |
| `METADATA/` | Manual | Yes | JSON overlays for custom extensions |

Files marked `.DISABLED` are excluded from compilation but preserved for reference.

---

## Workflow

Use the **Label Creator** tool (`LabelCreator-START.py`) to create, edit, and manage label sets. It provides built-in editors for InputMapping.h, LEDMapping.h, DisplayMapping.cpp, SegmentMap.h, CustomPins.h, LatchedButtons.h, and CoverGates.h.

The generators (`generate_data.py`, `reset_data.py`, `display_gen.py`) inside each label set call into the shared `_core/` modules. **Do not run generators manually** — use the Label Creator or Compiler Tool which invoke them safely.

---

## METADATA Directory

Each label set can include a `METADATA/` subdirectory containing JSON overlays:
- `CommonData.json` — shared data merged into all panels
- `MetadataStart.json` / `MetadataEnd.json` — prepended/appended to panel definitions
- `Custom.json` — custom extensions and overrides

These are merged into the main aircraft definition at generation time, allowing per-label-set customization without modifying the base aircraft JSON.

---

## Naming Conventions

- Label set directories must start with `LABEL_SET_`.
- The active set is selected via `active_set.h` (managed by the Compiler Tool).

---

## Best Practices

- **Never** manually edit auto-generated files except in user-marked safe zones.
- Keep `selected_panels.txt` in sync with what you actually want in your firmware.
- Use METADATA overlays for per-label-set customizations.
- Use the Label Creator's built-in editors rather than editing mapping files by hand.
