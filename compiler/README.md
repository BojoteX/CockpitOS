# CockpitOS Compiler Tool

The Compiler Tool (`CockpitOS-START.py` / `compiler/cockpitos.py`) compiles and uploads CockpitOS firmware to ESP32 boards. It provides a TUI (Terminal User Interface) that automates the entire build process — no Arduino IDE needed.

---

## Files

| File | Description |
|------|-------------|
| `cockpitos.py` | Main entry point — menu loop, compilation flow orchestration |
| `ui.py` | ANSI TUI toolkit (menus, pickers, spinners, prompts) |
| `build.py` | Compilation via arduino-cli, firmware upload, port scanning |
| `boards.py` | Board discovery, option configuration, dual-USB detection |
| `config.py` | Config.h engine — tracked variable registry, transport/role logic |
| `labels.py` | Label set discovery, selection, and generator invocation |
| `compiler_prefs.json` | Persistent preferences (FQBN, board options, transport settings) |
| `arduino-cli/` | Bundled arduino-cli executable |

## Usage

Run from the project root:
```
python CockpitOS-START.py
```

The tool handles board detection, label set selection, Config.h configuration, compilation, and upload in a guided workflow.
