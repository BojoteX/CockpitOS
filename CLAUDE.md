# CockpitOS — Claude Code Companion

## Session Startup

At the start of every session, silently run:
```
git fetch origin
git log --oneline HEAD...origin/main
git diff --stat origin/main
git log --oneline -5
git status -s
```
Report a brief summary: current branch, commits ahead/behind remote, uncommitted changes, and what was last worked on. This keeps us always aware of project state without me having to ask.

## What Is CockpitOS

ESP32 firmware (C++/Arduino) for DCS World flight simulator cockpit panels. Physical buttons, switches, LEDs, displays, and gauges connect to ESP32 boards and communicate with DCS World through DCS-BIOS (a LUA export protocol over UDP).

Three Python TUI tools automate the entire workflow — no Arduino IDE or manual file editing needed:
- **Setup-START.py** — installs ESP32 core + libraries via bundled arduino-cli
- **CockpitOS-START.py** → `compiler/cockpitos.py` — compiles and uploads firmware
- **LabelCreator-START.py** → `label_creator/label_creator.py` — creates/edits label sets with built-in editors for InputMapping.h, LEDMapping.h, DisplayMapping.cpp, SegmentMap.h, CustomPins.h, LatchedButtons.h, CoverGates.h

All tools are Windows-only, Python 3.12+, ANSI TUI, and switch between each other via `os.execl()`.

## Where to Find Information

| Topic | Location |
|-------|----------|
| **Documentation (current)** | `Docs/` — structured docs (Getting-Started, Tools, Hardware, How-To, Reference, Advanced, LLM) |
| **LLM master reference** | `Docs/LLM/CockpitOS-LLM-Reference.md` — start here for any CockpitOS question |
| **Config.h reference** | `Docs/Reference/Config.md` |
| **Label Creator internals** | `label_creator/LLM/` — three files: LLM_GUIDE.md, ARCHITECTURE.md, EDITOR_FEATURES.md |
| **Pending work items** | `TODO.md` (root) + `TODO/` directory (older items, RS485 fixes, perf, DCS-BIOS) |
| **Session changelogs** | `TODO/SESSION_CHANGELOG_*.md` |

## Project Structure (Key Paths)

```
CockpitOS.ino           Entry point
Config.h                Master config (transport, debug, timing)
Mappings.cpp/h          Panel init/loop orchestration, PCA auto-detection, isLatchedButton()
src/Core/               Core firmware (HIDManager, DCSBIOSBridge, CoverGate, InputControl, LEDControl)
src/Panels/             Panel implementations (Generic.cpp handles most; custom panels for complex logic)
src/LABELS/             Label sets — one folder per panel/aircraft
src/LABELS/active_set.h Points to the active label set
src/LABELS/_core/       Generator modules (generator_core.py, display_gen_core.py, reset_core.py)
compiler/               Compiler tool source (cockpitos.py, ui.py)
label_creator/          Label Creator source (label_creator.py, ui.py, input_editor.py, led_editor.py, display_editor.py, segment_map_editor.py, custompins_editor.py, latched_editor.py, covergate_editor.py)
HID Manager/            PC-side USB HID bridge
Debug Tools/            UDP console, stream recorder/player, command testers
```

## Git Workflow

- **main** — stable, pushes go through PRs from **dev**
- **dev** — active development branch
- Work on `dev`, PR to `main`
- Commit messages: short imperative, describe the "what" not the "how"

## Coding Conventions

### Python (tools)
- ANSI escape codes for all TUI output — no curses, no external TUI libraries
- `ui.py` in each tool is the TUI toolkit (menus, pickers, editors, prompts)
- `msvcrt` for keyboard input (Windows-only)
- `os.execl()` for tool switching (replaces current process)
- Row highlighting: `_SEL_BG = "\033[48;5;236m"` (dark gray)
- Warning style: `"\033[43m\033[30m"` (yellow bg + black text, no bold)
- Terminal-responsive layouts: use `os.get_terminal_size()` for dynamic column widths

### C++ (firmware)
- Arduino framework on ESP32
- Static memory allocation — no `new`/`malloc` in loop paths
- Non-blocking I/O — never block in `loop()` functions (watchdog resets at ~3s)
- `CUtils` API for hardware access, not raw Arduino GPIO calls
- `REGISTER_PANEL()` macro for panel registration
- Label sets are self-contained folders — core firmware should rarely need modification
- `#if defined(HAS_*)` guards for conditional panel compilation

## Label Set Structure

Each folder in `src/LABELS/LABEL_SET_*/` contains:
- `InputMapping.h` — input definitions (source: GPIO, PCA, HC165, TM1637, MATRIX, NONE)
- `LEDMapping.h` — output definitions (device: GPIO, PCA9555, WS2812, TM1637, GN1640T, GAUGE)
- `DisplayMapping.cpp/h` — display field → segment map linkage
- `*_SegmentMap.h` — HT1622 RAM-to-segment mapping
- `CustomPins.h` — pin assignments, feature enables
- `LabelSetConfig.h` — device name, HID settings
- `DCSBIOSBridgeData.h` — auto-generated hash tables
- `LatchedButtons.h` — per-label-set latched button declarations
- `CoverGates.h` — per-label-set cover gate definitions
- `selected_panels.txt` — which aircraft panels are included

## Known Gaps (TODO.md)

These are the remaining manual steps that should be automated:

1. **DCS-BIOS installer** — not automated by Setup Tool
2. **HID Manager deps** — pip installs not automated
3. **Compiler Misc Options** — some Config.h settings (polling rate, TEST_LEDS, IS_REPLAY, axis tuning) still require manual editing

## Changelog

Maintain `CHANGELOG.md` in the project root. This is a user-facing file — keep it brief, scannable, and only for significant changes. Do not log internal refactors, minor fixes, or CI tweaks.

Rules:
- Group entries under version headers (`## v1.1.5`, etc.) with a date
- Use short bullet points — one line per item, plain language, no jargon
- Categories: **Added**, **Improved**, **Fixed** (only include categories that have entries)
- Only add an entry when a change is meaningful to end users: new features, new hardware support, notable UX improvements, or important bug fixes
- Do not add entries for: code cleanup, doc edits, CI changes, internal restructuring
- When committing a significant change, update the changelog in the same commit
- Newest version goes at the top of the file

## Hard Rules — Do NOT

- **NEVER run generators or scripts against live label sets** — `generate_data.py`, `reset_data.py`, `display_gen.py`, etc. modify files in place and can destroy hand-tuned configurations (e.g., `.DISABLED` files get consumed and renamed). If you need to inspect generator output, read the code and trace it mentally. Do not execute it.
- **NEVER rename, move, or delete files in `src/LABELS/`** without explicit user instruction
- **NEVER run destructive git commands** (`reset --hard`, `checkout .`, `clean -f`) without explicit user instruction

## User Preferences

- Be direct and concise — no filler
- No emojis unless asked
- Catch logic flaws in UX — contextual labels, state-aware menus
- Cursor should stay on the menu item after an action (cursor memory)
- Terminal width should always be fully utilized in list/table views
- When making changes to the Label Creator TUI: test all code paths mentally, the user will catch edge cases
- Read before edit — always read a file before modifying it
- Prefer editing existing files over creating new ones
