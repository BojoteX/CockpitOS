# CockpitOS Label Creator

The Label Creator (`LabelCreator-START.py` / `label_creator/label_creator.py`) creates and manages label sets with built-in TUI editors for all mapping files. It handles the full lifecycle: creating new label sets, selecting aircraft and panels, and editing hardware configuration.

---

## Files

| File | Description |
|------|-------------|
| `label_creator.py` | Main orchestrator — create, browse, edit, delete label sets |
| `ui.py` | ANSI TUI toolkit (menus, pickers, editors, prompts) |
| `aircraft.py` | Aircraft JSON discovery and loading from `_core/aircraft/` |
| `label_set.py` | Label set creation, template copying, generator invocation |
| `panels.py` | Interactive three-level panel selection UI |
| `input_editor.py` | InputMapping.h editor — source, port, bit, hidId fields |
| `led_editor.py` | LEDMapping.h editor — device, info, dimmable, activeLow fields |
| `display_editor.py` | DisplayMapping.cpp editor — renderType, fieldType, min/max |
| `segment_map_editor.py` | SegmentMap.h editor — HT1622 RAM-to-segment mapping |
| `custompins_editor.py` | CustomPins.h editor — pin assignments grouped by device type |
| `latched_editor.py` | LatchedButtons.h editor — toggle list for latched inputs |
| `covergate_editor.py` | CoverGates.h editor — add/edit/delete cover gate entries |

## Usage

Run from the project root:
```
python LabelCreator-START.py
```

## Internal Documentation

See `LLM/` subdirectory for detailed architecture docs:
- `LLM_GUIDE.md` — overview and conventions
- `ARCHITECTURE.md` — module relationships and data flow
- `EDITOR_FEATURES.md` — editor capabilities and field handling
