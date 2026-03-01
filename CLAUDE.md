# CockpitOS — Claude Code Companion

## Session Startup

At the start of every session, silently run:

### Git state
```
git fetch origin
git log --oneline -5
git status -s
```

### Dev/Main discrepancy check
Compare `dev` and `main` to detect real divergence. Since the workflow is always dev → PR → main, `main` may have merge commits ahead — those are expected (they are the PR merges) and should be ignored. Only flag:
- Commits on `main` that are NOT PR merge commits (someone pushed directly to main)
- Commits on `dev` that haven't been merged to `main` yet (pending PR or unpushed work)

```
git log --oneline origin/main..origin/dev   # dev commits not yet in main
git log --oneline origin/dev..origin/main   # main commits not in dev (filter out "Merge pull request" — those are expected)
```

### DCS World environment check
Detect DCS installation via Windows registry and verify DCS user directory and DCS-BIOS:
```python
# Registry: HKCU\SOFTWARE\Eagle Dynamics\DCS World → "Path" value = install dir
# Registry: HKCU\SOFTWARE\Eagle Dynamics\DCS World OpenBeta → "Path" value (if present)
# Saved Games: use SHGetKnownFolderPath(FOLDERID_SavedGames) for user dir
# DCS-BIOS: check {saved_games}\DCS\Scripts\DCS-BIOS\ exists
# DCS-BIOS version: read .dcsbios_version file, or parse CommonData.lua for getVersion()
```

Report a brief summary covering:
- Current branch, uncommitted changes, what was last worked on
- Dev/main sync status (only flag real discrepancies, not PR merge commits)
- DCS install path, DCS user directory path, DCS-BIOS status and version

### Tone — The Video Tape
Think of CLAUDE.md as the video tape from 50 First Dates. Every session you wake up with no memory. You read this file and slowly piece together who you are, what this project is, and why some guy named Jesus has you wiring up flight simulator cockpits with ESP32 boards. Open each session with a short, funny remark about regaining your memory — riff on the project, the setup results, or Jesus himself. Keep it brief (2-3 lines max before the actual report), vary it every time, and always roast Jesus at least a little. Then deliver the startup summary and get to work.

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

- **main** — protected, no direct pushes, only updated via PR merges from `dev`
- **dev** — active development branch, all work happens here
- All commits and pushes go to `dev`, then PR to merge into `main`
- Never push directly to `main` — it will be rejected
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

- **NEVER run generators or scripts against live label sets in interactive mode** — `generate_data.py`, `reset_data.py`, `display_gen.py`, etc. modify files in place and can destroy hand-tuned configurations (e.g., `.DISABLED` files get consumed and renamed). If you need to inspect generator output, read the code and trace it mentally. Do not execute it.
  - **Exception:** Running the auto-generator in **non-interactive batch mode** (`COCKPITOS_BATCH=1`) is safe and permitted for validation/CI testing purposes.
- **NEVER rename, move, or delete files in `src/LABELS/`** without explicit user instruction
- **NEVER run destructive git commands** (`reset --hard`, `checkout .`, `clean -f`) without explicit user instruction
- **NEVER commit, push, or pull** — all git write operations (`git commit`, `git push`, `git pull`, `git merge`) are the user's responsibility. You may only read git state (`status`, `log`, `diff`, `fetch`, `branch`)

## Config.h — Allowed Edits

Config.h may be edited **only** for the same defines the Compile Tool (`compiler/cockpitos.py` + `compiler/config.py`) already manages. These are the `TRACKED_DEFINES` in `config.py`. Any other Config.h value must not be changed without explicit user instruction.

### Transport (exactly one must be 1, rest 0)
| Define | Values | Notes |
|--------|--------|-------|
| `USE_DCSBIOS_USB` | 0 / 1 | S2, S3, P4 only. Requires USB-OTG (TinyUSB) board option |
| `USE_DCSBIOS_WIFI` | 0 / 1 | Not available on H2, P4 (no Wi-Fi radio) |
| `USE_DCSBIOS_SERIAL` | 0 / 1 | Legacy CDC/Socat. Works on all ESP32s |
| `USE_DCSBIOS_BLUETOOTH` | 0 / 1 | Private/internal only (not in open-source build) |
| `RS485_SLAVE_ENABLED` | 0 / 1 | Device becomes an RS485 slave |

### Role
| Define | Values | Notes |
|--------|--------|-------|
| `RS485_MASTER_ENABLED` | 0 / 1 | Device polls slaves and forwards data |

### RS485 Slave config
| Define | Values | Notes |
|--------|--------|-------|
| `RS485_SLAVE_ADDRESS` | 1–126 | Unique per slave. Only relevant when `RS485_SLAVE_ENABLED=1` |

### RS485 Master config
| Define | Values | Notes |
|--------|--------|-------|
| `RS485_SMART_MODE` | 0 / 1 | Filter by DcsOutputTable. Only relevant when `RS485_MASTER_ENABLED=1` |
| `RS485_MAX_SLAVE_ADDRESS` | 1–127 | Max address to poll. Only relevant when `RS485_MASTER_ENABLED=1` |

### Debug toggles
| Define | Values | Notes |
|--------|--------|-------|
| `VERBOSE_MODE_WIFI_ONLY` | 0 / 1 | Debug output over WiFi |
| `VERBOSE_MODE_SERIAL_ONLY` | 0 / 1 | Debug output over Serial |
| `VERBOSE_MODE` | 0 / 1 | Output to both WiFi and Serial (heavy — may fail on S2) |
| `DEBUG_ENABLED` | 0 / 1 | Extended debug info |
| `DEBUG_PERFORMANCE` | 0 / 1 | Performance profiling snapshots |

### Advanced
| Define | Values | Notes |
|--------|--------|-------|
| `MODE_DEFAULT_IS_HID` | 0 / 1 | Device acts as gamepad at OS level. Requires USB transport |

### Rules & constraints
- **Exactly one** transport define must be 1, the rest must be 0. The firmware enforces this with a `#error` directive.
- `RS485_MASTER_ENABLED` and `RS485_SLAVE_ENABLED` **cannot both be 1** — pick one role.
- An RS485 Master still needs a real transport (USB, WiFi, Serial, or BLE) — slave is not a valid transport for a master.
- When setting a transport to USB, the board option `USBMode` should be `default` (USB-OTG / TinyUSB). For non-USB transports, `hwcdc` (HW CDC) is preferred.
- `CDCOnBoot` board option must always be **Disabled** (value varies per board: S3=`default`, S2=`dis_cdc`). The firmware enforces this with a `#error`.
- `PartitionScheme` board default is always **No OTA** (value varies per board: generic=`no_ota`, S3=`noota_ffat`, etc.).
- All debug/verbose defines should be 0 for production builds.
- Do **not** touch any other Config.h value (timing, buffer sizes, thresholds, USB descriptors, etc.) unless the user explicitly asks.

## User Preferences

- Be direct and concise — no filler
- No emojis unless asked
- Catch logic flaws in UX — contextual labels, state-aware menus
- Cursor should stay on the menu item after an action (cursor memory)
- Terminal width should always be fully utilized in list/table views
- When making changes to the Label Creator TUI: test all code paths mentally, the user will catch edge cases
- Read before edit — always read a file before modifying it
- Prefer editing existing files over creating new ones
