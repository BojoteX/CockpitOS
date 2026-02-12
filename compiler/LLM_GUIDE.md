# CockpitOS Management Tool — LLM Reference Guide

> This file is for LLMs working on this codebase. It maps every module,
> function, constant, and design decision so you can modify or extend
> the tool without reading every source file first.

## Quick Context

CockpitOS is an ESP32 firmware for DCS World flight simulator cockpit panels.
It uses DCS-BIOS to bridge cockpit controls to the sim. This **management tool**
is a Python TUI that configures, compiles, and uploads the firmware.

**Entry point:** `CockpitOS-START.py` (project root) — adds `compiler/` to
`sys.path` and calls `cockpitos.main()`.

---

## File Layout

```
CockpitOS/
├── CockpitOS-START.py              Root launcher (thin, calls cockpitos.main)
├── Config.h                        Firmware configuration (#define values)
├── compiler/
│   ├── cockpitos.py                Main menu, prefs, orchestration
│   ├── ui.py                       Terminal UI toolkit (zero domain knowledge)
│   ├── config.py                   Config.h engine + file safety + transport/role
│   ├── boards.py                   Arduino CLI + board intelligence + validation
│   ├── build.py                    Compile + upload + clean + firmware naming
│   ├── labels.py                   Label set discovery + generate_data.py
│   ├── compiler_prefs.json         User preferences (board, FQBN, options)
│   ├── backups/                    Timestamped Config.h backups (auto-created)
│   ├── output/                     Final .bin files after compile (auto-created)
│   ├── TODO                        Improvement plan with task status
│   └── LLM_GUIDE.md               THIS FILE
├── src/
│   ├── LABELS/                     Label sets (per-aircraft panel configs)
│   │   ├── active_set.h            Currently selected label set
│   │   ├── LABEL_SET_MAIN/         Example label set
│   │   └── ...
│   └── Panels/                     Panel hardware definitions
└── Docs/                           User documentation (.md files)
```

---

## Dependency Graph (no circular imports)

```
cockpitos.py ──> build.py ──> boards.py ──> config.py ──> ui.py
             ──> boards.py       └──> labels.py     └──> (nothing)
             ──> labels.py       └──> config.py
             ──> config.py
```

**Rule:** `ui.py` imports NOTHING from our modules. Every other module imports
`ui`. Only `cockpitos.py` imports from all modules.

**One exception:** `config.py` does a late import of `boards` inside
`configure_transport_and_role()` to avoid a circular dependency.

---

## Module Details

### ui.py (413 lines) — Terminal UI Toolkit

Pure presentation. Zero CockpitOS knowledge. Reusable in any TUI project.

**Constants:**
`CYAN`, `GREEN`, `YELLOW`, `RED`, `BOLD`, `DIM`, `RESET`, `REV`,
`HIDE_CUR`, `SHOW_CUR`, `ERASE_LN` — ANSI escape codes.

**Functions:**
| Function | Purpose |
|---|---|
| `_enable_ansi()` | Enable ANSI on Windows 10+ (called at import time) |
| `_w(text)` | Write raw text to stdout, no newline, flush |
| `cls()` | Clear terminal screen |
| `cprint(color, text)` | Print with color, handles UnicodeEncodeError |
| `header(text)` | Clears screen, prints centered cyan banner |
| `success(text)` | Green `[OK]` message |
| `warn(text)` | Yellow `[!]` message |
| `error(text)` | Red `[X]` message |
| `info(text)` | Indented plain text |
| `big_success(title, details)` | Large green banner with detail lines |
| `big_fail(title, details)` | Large red banner with detail lines |
| `pick(prompt, options, default)` | Arrow-key single-select picker. `options` = `[(label, value), ...]` |
| `menu_pick(items)` | Full-screen menu with separators. Items: `("label", "value", "style")` or `("---",)`. Styles: `action`/`normal`/`dim` |
| `pick_filterable(prompt, options, default)` | Arrow-key picker with type-to-filter for long lists |
| `confirm(prompt, default_yes)` | Y/n prompt, returns bool |

**Class: `Spinner`**
Background-threaded animated spinner with elapsed time display.
Methods: `start()`, `stop()`. Usage: `s = Spinner("Compiling"); s.start(); ...; s.stop()`

---

### config.py (533 lines) — Config.h Engine

Owns ALL interaction with Config.h. File safety, tracked registry, snapshot,
transport/role logic, backup/restore.

**Key Constants:**
| Name | Purpose |
|---|---|
| `SKETCH_DIR` | CockpitOS root directory (Path). Other modules import this. |
| `CONFIG_H` | Path to Config.h |
| `BACKUP_DIR` | Path to `compiler/backups/` |
| `ALLOWED_FILES` | Whitelist dict — only these files can be written by the tool |
| `MAX_BACKUPS` | Keep last 10 backups |
| `TRACKED_DEFINES` | Dict of all 17 Config.h `#define` names the tool manages, with default values |
| `TRANSPORT_DEFINES` | List of 5 transport-related define names |
| `TRANSPORT_KEY` | Maps friendly names (`"wifi"`, `"usb"`, etc.) to define names |
| `TRANSPORTS` | List of `(display_label, key)` tuples for the transport picker |
| `_config_snapshot` | In-memory dict populated by `load_config_snapshot()` on startup |

**File Safety Functions:**
| Function | Purpose |
|---|---|
| `_is_allowed_file(path)` | Check whitelist |
| `_prune_backups(prefix)` | Delete old backups beyond MAX_BACKUPS |
| `backup_file(path)` | Timestamped copy, once-per-session guard |
| `safe_write_file(path, content)` | Atomic write: .tmp then `os.replace()` |
| `list_backups(filename)` | List backups newest first: `[(path, timestamp, size)]` |
| `diff_backup_vs_current(backup, current)` | Compare #defines between two files |

**Config.h Read/Write (low-level — don't call directly):**
| Function | Purpose |
|---|---|
| `read_config_define(name)` | Regex read of `#define` from Config.h file |
| `write_config_define(name, value)` | Regex substitution + backup + atomic write |

**Snapshot (use these instead):**
| Function | Purpose |
|---|---|
| `load_config_snapshot()` | Read all tracked defines into memory. Returns list of missing names. |
| `config_get(name)` | Read from snapshot (fast). Falls back to file if snapshot not loaded. |
| `config_set(name, value)` | Write to Config.h AND update snapshot. Returns bool. |

**Transport/Role Queries:**
| Function | Purpose |
|---|---|
| `read_current_transport()` | Returns: `"wifi"`, `"usb"`, `"serial"`, `"ble"`, `"slave"`, or `None` |
| `read_current_role()` | Returns: `"standalone"`, `"master"`, or `"slave"` |
| `transport_label(t)` | Friendly name: `"wifi"` -> `"WiFi"` |
| `role_label(r)` | Friendly name: `"master"` -> `"RS485 Master"` |

**Interactive:**
| Function | Purpose |
|---|---|
| `restore_config()` | Interactive backup picker + diff + confirm + restore |
| `configure_transport_and_role(prefs, board_has_dual_usb_fn, preferred_usb_mode_fn)` | 3-step wizard: Master? -> Transport -> Slave addr. Updates Config.h + prefs. Board functions passed in to avoid circular import. |

---

### boards.py (283 lines) — Board Management

Arduino CLI interaction, board intelligence, cross-validation.

**Key Constants:**
| Name | Purpose |
|---|---|
| `ARDUINO_CLI` | Path to `arduino-cli.exe` |
| `COCKPITOS_DEFAULTS` | Dict of 12 non-negotiable board option defaults (CDCOnBoot=off, NoOTA, 240MHz, etc.) |
| `FATAL` / `WARNING` | String constants for validation levels |

**Functions:**
| Function | Purpose |
|---|---|
| `run_cli(*args)` | Subprocess wrapper for arduino-cli |
| `get_board_options(fqbn)` | Parse `board details` output into `{key: {name, values, default}}` |
| `board_has_dual_usb(prefs)` | True if board has USBMode option with 2+ values (S3, P4) |
| `preferred_usb_mode(transport)` | `"usb"` -> `"default"` (OTG), else `"hwcdc"` (CDC) |
| `validate_config_vs_board(prefs)` | Returns `[(level, message)]`. Checks: CDC on boot, USB mode mismatch |
| `get_all_boards()` | Query all installed boards: `[(name, fqbn)]` |
| `select_board(prefs)` | Interactive filterable board picker |
| `configure_options(fqbn, prefs)` | Quick (auto defaults) or Full (manual) board option config |

**Validation Rules:**
- **FATAL:** CDC On Boot enabled -> blocks compile
- **FATAL:** USB transport + HW CDC on dual-USB board -> blocks compile
- **WARNING:** Non-USB transport + USB-OTG on dual-USB board -> warns

---

### build.py (324 lines) — Compile, Upload, Clean

**Key Constants:**
| Name | Purpose |
|---|---|
| `BUILD_DIR` | `AppData/Local/Arduino15/CockpitOS/build/` |
| `CACHE_DIR` | `AppData/Local/Arduino15/CockpitOS/cache/` |
| `OUTPUT_DIR` | `compiler/output/` — user-accessible final binaries |

**Functions:**
| Function | Purpose |
|---|---|
| `build_firmware_name()` | ALL CAPS naming: `MAIN_WIFI.bin`, `RS485_MASTER_MAIN_USB.bin`, `RS485_SLAVE_MAIN.bin` |
| `compile_sketch(fqbn, options, verbose, export_bin)` | Full compile with spinner or verbose output. Copies .bin to output/. |
| `scan_ports()` | List connected devices: `[(display, address)]` |
| `upload_sketch(fqbn, options, port)` | Interactive upload with port picker |
| `clean_build()` | Delete build, cache, and output .bin files |

---

### labels.py (109 lines) — Label Set Management

**Key Constants:**
| Name | Purpose |
|---|---|
| `LABELS_DIR` | `src/LABELS/` |
| `ACTIVE_SET` | `src/LABELS/active_set.h` |

**Functions:**
| Function | Purpose |
|---|---|
| `discover_label_sets()` | Find `LABEL_SET_*` dirs: `[(name, path, has_generate_data)]` |
| `read_active_label_set()` | Parse `active_set.h` for current label set name |
| `select_label_set(prefs)` | Interactive picker with current highlighted |
| `run_generate_data(label_set_name)` | Launch `generate_data.py` in new console, verify output |

---

### cockpitos.py (313 lines) — Entry Point & Orchestration

**Key Constants:**
| Name | Purpose |
|---|---|
| `PREFS_FILE` | `compiler/compiler_prefs.json` |

**Functions:**
| Function | Purpose |
|---|---|
| `load_prefs()` | Read JSON prefs file |
| `save_prefs(prefs)` | Write JSON prefs file |
| `show_config(prefs)` | Display full summary: label set, transport, role, RS485 details, board, options, debug flags, validation issues |
| `do_compile(prefs, verbose)` | Full compile flow: label set -> board check -> validation gate -> generate_data -> compile |
| `main()` | Main menu loop. Checks ARDUINO_CLI exists, loads prefs, loads Config.h snapshot, runs menu. |

**Main Menu Options:**
`COMPILE`, `UPLOAD`, `Transport / Role`, `Board / Board Options`,
`Show config`, `Restore Config.h`, `Clean build`, `Exit`

---

## Key Concepts for LLMs

### Config.h is the Source of Truth
- 17 tracked `#define` values in `TRACKED_DEFINES`
- Loaded into `_config_snapshot` on startup via `load_config_snapshot()`
- Read via `config_get()`, write via `config_set()` — never use raw functions
- User can hand-edit Config.h in Arduino IDE; next launch picks up changes

### Two Configuration Domains
1. **Config.h** — firmware defines (transport, role, RS485, debug)
2. **Board Options** — arduino-cli FQBN flags (USB mode, partition, CDC, etc.)
- These interact! USB transport requires USB-OTG mode on dual-USB boards
- `validate_config_vs_board()` enforces the rules

### File Safety Guarantees
- `ALLOWED_FILES` whitelist — tool can ONLY write files listed here
- `backup_file()` — timestamped copy before first modification per session
- `safe_write_file()` — atomic write via temp + `os.replace()`
- Config.h can NEVER be corrupted by the tool

### Transport Model
- Exactly ONE of 5 transport defines must be `1`, rest `0`
- RS485 Slave IS a transport (zeroes out the other four)
- RS485 Master is a ROLE that requires a real transport (WiFi/USB/Serial/BLE)
- Standalone just picks a transport directly

### compiler_prefs.json
Stores: `fqbn`, `board_name`, `options` (board flags), `transport`, `role`.
Does NOT mirror Config.h values — those live in Config.h only.

---

## How to Extend

### Add a new tracked Config.h define
1. Add to `TRACKED_DEFINES` in `config.py`
2. Use `config_get("NEW_DEFINE")` / `config_set("NEW_DEFINE", value)` anywhere

### Add a new file the tool can modify
1. Add to `ALLOWED_FILES` in `config.py`
2. Use `backup_file()` + `safe_write_file()` for all writes

### Add a new cross-validation rule
1. Add check in `validate_config_vs_board()` in `boards.py`
2. Return `(FATAL, msg)` or `(WARNING, msg)` tuple

### Add a new menu option
1. Add item to `menu_pick()` call in `cockpitos.py` `main()`
2. Add `elif choice == "new_option":` handler below

### Add a new module
1. Create `compiler/new_module.py`
2. Import from `ui` and/or `config` (never create circular deps)
3. Import in `cockpitos.py` to wire into the main flow
