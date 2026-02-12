#!/usr/bin/env python3
"""
CockpitOS Management Tool — Interactive build, configure, and deploy.

Entry point. Thin orchestration layer:
  - Main menu loop
  - Preferences persistence
  - Compile flow (label set → validation → generate → compile)

Modules:
  ui.py       Terminal UI toolkit (zero CockpitOS knowledge)
  config.py   Config.h engine — tracked registry, read/write, transport/role
  boards.py   Arduino CLI wrappers — board discovery, options, validation
  build.py    Compile + upload + clean
  labels.py   Label set discovery, selection, generate_data
"""

import os
import sys
import json
from pathlib import Path

# ---------------------------------------------------------------------------
# Module imports
# ---------------------------------------------------------------------------
from ui import (
    CYAN, BOLD, DIM, RESET,
    cls, cprint, header, info, success, warn, error,
    menu_pick, confirm,
)
from config import (
    CONFIG_H, TRACKED_DEFINES,
    load_config_snapshot, config_get,
    read_current_transport, read_current_role,
    transport_label, role_label,
    configure_transport_and_role,
    SKETCH_DIR,
)
from boards import (
    ARDUINO_CLI, COCKPITOS_DEFAULTS,
    board_has_dual_usb, preferred_usb_mode,
    validate_config_vs_board, FATAL, WARNING,
    select_board, configure_options,
)
from build import (
    BUILD_DIR,
    compile_sketch, upload_sketch, clean_build,
)
from labels import (
    read_active_label_set, select_label_set, run_generate_data,
)

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
PREFS_FILE = SKETCH_DIR / "compiler" / "compiler_prefs.json"

# -----------------------------------------------------------------------------
# Preferences persistence
# -----------------------------------------------------------------------------
def load_prefs():
    if PREFS_FILE.exists():
        try:
            return json.loads(PREFS_FILE.read_text(encoding="utf-8"))
        except Exception:
            pass
    return {}

def save_prefs(prefs):
    PREFS_FILE.parent.mkdir(parents=True, exist_ok=True)
    PREFS_FILE.write_text(json.dumps(prefs, indent=2), encoding="utf-8")


# -----------------------------------------------------------------------------
# Show full config summary
# -----------------------------------------------------------------------------
def show_config(prefs):
    header("Current Configuration")
    print()

    # Label Set
    active_ls = read_active_label_set()
    info(f"Label Set:   {CYAN}{active_ls or '(not set)'}{RESET}")

    # Transport & Role from Config.h
    transport = read_current_transport()
    role = read_current_role()
    info(f"Transport:   {CYAN}{transport_label(transport) if transport else '(unknown)'}{RESET}")
    info(f"Role:        {CYAN}{role_label(role) if role else '(unknown)'}{RESET}")

    # RS485 details if applicable
    if role == "master":
        smart = config_get("RS485_SMART_MODE") or "0"
        max_addr = config_get("RS485_MAX_SLAVE_ADDRESS") or "127"
        use_task = config_get("RS485_USE_TASK") or "?"
        task_core = config_get("RS485_TASK_CORE") or "?"
        info(f"  RS485 Smart Mode:  {smart}")
        info(f"  Max Slave Address: {max_addr}")
        info(f"  Use Task: {use_task}  Core: {task_core}")
    elif role == "slave":
        addr = config_get("RS485_SLAVE_ADDRESS") or "?"
        isr = config_get("RS485_USE_ISR_MODE") or "?"
        info(f"  Slave Address: {addr}")
        info(f"  ISR Mode: {isr}")

    # RS485 pins
    if role in ("master", "slave"):
        tx = config_get("RS485_TX_PIN") or "?"
        rx = config_get("RS485_RX_PIN") or "?"
        de = config_get("RS485_DE_PIN") or "?"
        info(f"  Pins: TX={tx}  RX={rx}  DE={de}")

    print()

    # Board
    fqbn = prefs.get("fqbn", "(not set)")
    friendly = prefs.get("board_name")
    board_str = f"{friendly} ({fqbn})" if friendly else fqbn
    info(f"Board:       {CYAN}{board_str}{RESET}")

    dual_usb = board_has_dual_usb(prefs)
    if dual_usb:
        info(f"  {DIM}Dual USB board (USB-OTG + HW CDC){RESET}")

    opts = prefs.get("options", {})
    if opts:
        key_opts = ["PartitionScheme", "FlashSize", "FlashMode", "USBMode", "CPUFreq", "PSRAM"]
        for k in key_opts:
            if k in opts:
                info(f"  {k} = {opts[k]}")

    # Debug flags
    debug = config_get("DEBUG_ENABLED") or "0"
    verbose = config_get("VERBOSE_MODE") or "0"
    if debug != "0" or verbose != "0":
        print()
        warn(f"Debug flags active: DEBUG_ENABLED={debug}, VERBOSE_MODE={verbose}")

    # Cross-validation
    issues = validate_config_vs_board(prefs)
    if issues:
        print()
        for level, msg in issues:
            if level == FATAL:
                error(f"[FATAL] {msg}")
            else:
                warn(f"[WARNING] {msg}")


# -----------------------------------------------------------------------------
# Compile flow
# -----------------------------------------------------------------------------
def do_compile(prefs, verbose=False):
    """Compile flow. Always asks for label set (defaults to current), then builds."""

    label_set = select_label_set(prefs)
    if not label_set:
        return False

    if not prefs.get("fqbn"):
        warn("No board configured yet.")
        fqbn = select_board(prefs)
        prefs["fqbn"] = fqbn
        opts = configure_options(fqbn, prefs)
        prefs["options"] = opts
        save_prefs(prefs)
    else:
        fqbn = prefs["fqbn"]
        opts = prefs.get("options", {})

    # Pre-compile cross-validation
    issues = validate_config_vs_board(prefs)
    has_fatal = False
    for level, msg in issues:
        if level == FATAL:
            error(f"[FATAL] {msg}")
            has_fatal = True
        else:
            warn(f"[WARNING] {msg}")
    if has_fatal:
        error("Cannot compile — fix the fatal issue(s) above first.")
        return False
    if any(lv == WARNING for lv, _ in issues):
        if not confirm("Warnings detected. Continue compiling anyway?", default_yes=True):
            return False

    # Run generate_data.py
    ok = run_generate_data(label_set)
    if not ok:
        if not confirm("generate_data.py had issues. Continue compiling anyway?", default_yes=False):
            return False

    return compile_sketch(fqbn, opts, verbose=verbose, board_name=prefs.get("board_name"))


# =============================================================================
#  Main menu
# =============================================================================
def main():
    os.system("")  # Enable ANSI on Windows

    if not ARDUINO_CLI.exists():
        error(f"arduino-cli not found at: {ARDUINO_CLI}")
        error("Update the ARDUINO_CLI path in boards.py.")
        sys.exit(1)

    prefs = load_prefs()

    # Load Config.h snapshot — source of truth for all tracked defines
    if CONFIG_H.exists():
        missing = load_config_snapshot()
        if missing:
            warn(f"Config.h is missing {len(missing)} tracked define(s):")
            for m in missing:
                info(f"  {m} (using default: {TRACKED_DEFINES[m]})")
            info(f"Your Config.h may be outdated. Defaults applied in memory.")
            input(f"\n  {DIM}Press Enter to continue...{RESET}")
    else:
        error("Config.h not found! The tool cannot operate without it.")
        sys.exit(1)

    while True:
        # Read live state for display
        active_ls = read_active_label_set() or "(none)"
        transport = read_current_transport()
        role = read_current_role()
        friendly_board = prefs.get("board_name", prefs.get("fqbn", "(not configured)"))

        cls()
        print()
        cprint(CYAN + BOLD, "        ____            _          _ _    ___  ____")
        cprint(CYAN + BOLD, "       / ___|___   ___ | | ___ __ (_) |_ / _ \\/ ___|")
        cprint(CYAN + BOLD, "      | |   / _ \\ / __|| |/ / '_ \\| | __| | | \\___ \\")
        cprint(CYAN + BOLD, "      | |__| (_) | (__ |   <| |_) | | |_| |_| |___) |")
        cprint(CYAN + BOLD, "       \\____\\___/ \\___|_|\\_\\ .__/|_|\\__|\\___/|____/")
        cprint(CYAN + BOLD, "                            |_|")
        print()
        if role == "slave":
            print(f"     Label Set: {CYAN}{active_ls}{RESET}  |  "
                  f"Mode: {CYAN}RS485 Slave{RESET}")
        else:
            print(f"     Label Set: {CYAN}{active_ls}{RESET}  |  "
                  f"{CYAN}{role_label(role)}{RESET}  |  "
                  f"Transport: {CYAN}{transport_label(transport)}{RESET}")
        print(f"     Board: {CYAN}{friendly_board}{RESET}")
        print()

        choice = menu_pick([
            ("COMPILE",              "compile",   "action"),
            ("UPLOAD",               "upload",    "action"),
            ("---",),
            ("Transport / Role",     "transport", "normal"),
            ("Board / Board Options","board",     "normal"),
            ("---",),
            ("Show config",          "config",    "dim"),
            ("Clear cache/build",    "clean",     "dim"),
            ("Exit",                 "exit",      "dim"),
        ])

        if choice == "exit":
            cprint(DIM, "  Bye!")
            break

        elif choice == "transport":
            configure_transport_and_role(prefs, board_has_dual_usb, preferred_usb_mode)
            save_prefs(prefs)
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "board":
            fqbn = select_board(prefs)
            prefs["fqbn"] = fqbn
            opts = configure_options(fqbn, prefs)
            prefs["options"] = opts
            save_prefs(prefs)
            success("Board configuration saved.")
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "compile":
            do_compile(prefs)
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "upload":
            if not prefs.get("fqbn"):
                warn("No board configured. Set up Board first.")
                input(f"\n  {DIM}Press Enter to continue...{RESET}")
                continue
            if not BUILD_DIR.exists():
                warn("No build found. Compile first.")
                input(f"\n  {DIM}Press Enter to continue...{RESET}")
                continue
            upload_sketch(prefs["fqbn"], prefs.get("options", {}), board_name=prefs.get("board_name"))
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "config":
            show_config(prefs)
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "clean":
            clean_build()
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n{DIM}  Interrupted.{RESET}")
        sys.exit(0)
