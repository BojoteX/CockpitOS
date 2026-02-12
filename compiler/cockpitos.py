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
import msvcrt
from pathlib import Path

# ---------------------------------------------------------------------------
# Module imports
# ---------------------------------------------------------------------------
from ui import (
    CYAN, BOLD, DIM, GREEN, RESET, HIDE_CUR, SHOW_CUR,
    cls, cprint, header, info, success, warn, error,
    menu_pick, pick, toggle_pick, confirm, text_input, _RESET_SENTINEL,
)
from config import (
    CONFIG_H, TRACKED_DEFINES,
    load_config_snapshot, config_get, config_set,
    read_current_transport, read_current_role,
    transport_label, role_label,
    configure_transport_and_role,
    SKETCH_DIR, CREDENTIALS_DIR, WIFI_H,
    safe_write_file,
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
        info(f"  RS485 Smart Mode:  {smart}")
        info(f"  Max Slave Address: {max_addr}")
    elif role == "slave":
        addr = config_get("RS485_SLAVE_ADDRESS") or "?"
        info(f"  Slave Address: {addr}")

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
        if fqbn is None:
            return False
        prefs["fqbn"] = fqbn
        opts = configure_options(fqbn, prefs)
        if opts is None:
            return False
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


def do_quick_compile(prefs, verbose=False):
    """Quick compile — reuse current label set, skip generate_data.py."""
    active = read_active_label_set()
    if not active:
        warn("No active label set. Use full COMPILE first.")
        return False

    if not prefs.get("fqbn"):
        warn("No board configured yet.")
        fqbn = select_board(prefs)
        if fqbn is None:
            return False
        prefs["fqbn"] = fqbn
        opts = configure_options(fqbn, prefs)
        if opts is None:
            return False
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
        error("Cannot compile \u2014 fix the fatal issue(s) above first.")
        return False
    if any(lv == WARNING for lv, _ in issues):
        if not confirm("Warnings detected. Continue compiling anyway?", default_yes=True):
            return False

    info(f"Quick compile \u2014 reusing label set: {CYAN}{active}{RESET}")
    info(f"{DIM}Skipping label set selection and generate_data.py{RESET}")
    print()

    return compile_sketch(fqbn, opts, verbose=verbose, board_name=prefs.get("board_name"))


# -----------------------------------------------------------------------------
# Misc Options sub-menu
# -----------------------------------------------------------------------------
def misc_options(prefs):
    """Sub-menu for Wi-Fi credentials and debug toggles."""
    while True:
        header("Misc Options")
        choice = pick("Select:", [
            ("Wi-Fi Credentials",       "wifi"),
            ("Debug / Verbose Toggles", "debug"),
            ("Back",                    "back"),
        ])
        if choice is None or choice == "back":
            return
        elif choice == "wifi":
            configure_wifi_credentials()
            input(f"\n  {DIM}Press Enter to continue...{RESET}")
        elif choice == "debug":
            configure_debug_toggles()
            input(f"\n  {DIM}Press Enter to continue...{RESET}")


def configure_wifi_credentials():
    """Create or update .credentials/wifi.h with SSID and password."""
    header("Wi-Fi Credentials")

    # -- Wi-Fi requirements --------------------------------------------------
    default_ssid = "TestNetwork"
    default_pass = "TestingOnly"

    info(f"{DIM}ESP32 Wi-Fi requirements:{RESET}")
    info(f"{DIM}  \u2022 2.4 GHz band only (5 GHz is NOT supported){RESET}")
    info(f"{DIM}  \u2022 WPA2-PSK (AES/CCMP) security{RESET}")
    info(f"{DIM}  \u2022 Works on S2, S3, C3 (not H2/P4 \u2014 no radio){RESET}")
    print()

    # -- Read current values if wifi.h exists --------------------------------
    current_ssid = None
    current_pass = None
    has_file = WIFI_H.exists()

    if has_file:
        import re
        text = WIFI_H.read_text(encoding="utf-8")
        m_ssid = re.search(r'#define\s+WIFI_SSID\s+"([^"]*)"', text)
        m_pass = re.search(r'#define\s+WIFI_PASS\s+"([^"]*)"', text)
        if m_ssid:
            current_ssid = m_ssid.group(1)
        if m_pass:
            current_pass = m_pass.group(1)

        info(f"Current SSID: {CYAN}{current_ssid or '(empty)'}{RESET}")
        info(f"Current PASS: {CYAN}{'*' * min(len(current_pass), 8) if current_pass else '(empty)'}{RESET}")
    else:
        info(f"{DIM}No wifi.h found \u2014 using defaults: "
             f"\"{default_ssid}\" / \"{default_pass}\"{RESET}")

    print()
    info(f"{DIM}Enter to keep current  \u00b7  Esc to cancel  \u00b7  Ctrl+D to reset defaults{RESET}")
    print()

    # -- SSID prompt ---------------------------------------------------------
    ssid = text_input("SSID", default=current_ssid)
    if ssid is None:                       # ESC pressed
        return
    if ssid is _RESET_SENTINEL:            # Ctrl+D — reset to defaults
        if has_file:
            WIFI_H.unlink()
            success(f"Deleted wifi.h \u2014 reset to defaults "
                    f"(\"{default_ssid}\" / \"{default_pass}\")")
        else:
            info("Already using defaults.")
        return
    if not ssid:
        ssid = default_ssid

    # -- Password prompt -----------------------------------------------------
    password = text_input("Password", default=current_pass, mask=True)
    if password is None:                   # ESC pressed
        return
    if password is _RESET_SENTINEL:        # Ctrl+D — reset to defaults
        if has_file:
            WIFI_H.unlink()
            success(f"Deleted wifi.h \u2014 reset to defaults "
                    f"(\"{default_ssid}\" / \"{default_pass}\")")
        else:
            info("Already using defaults.")
        return
    if not password:
        password = default_pass

    print()
    CREDENTIALS_DIR.mkdir(parents=True, exist_ok=True)

    content = (
        "// Auto-generated by CockpitOS Management Tool\n"
        "// Do NOT commit this file to version control.\n"
        f'#define WIFI_SSID "{ssid}"\n'
        f'#define WIFI_PASS "{password}"\n'
    )

    if safe_write_file(WIFI_H, content):
        success(f"Saved: .credentials/wifi.h")
        info(f"  SSID: {CYAN}{ssid}{RESET}")
    else:
        error("Failed to write wifi.h!")


def configure_debug_toggles():
    """Toggle debug/verbose defines in Config.h."""
    header("Debug / Verbose Toggles")

    toggles = [
        ("Verbose output over WiFi",    "VERBOSE_MODE_WIFI_ONLY"),
        ("Verbose output over Serial",  "VERBOSE_MODE_SERIAL_ONLY"),
        ("Extended debug info",          "DEBUG_ENABLED"),
        ("Performance profiling",        "DEBUG_PERFORMANCE"),
    ]

    items = []
    for label, key in toggles:
        val = config_get(key) or "0"
        items.append((label, key, val != "0"))

    result = toggle_pick("Toggle options:", items)
    if result is None:
        return

    changes = []
    for label, key in toggles:
        new_val = "1" if result[key] else "0"
        old_val = config_get(key) or "0"
        if new_val != old_val:
            config_set(key, new_val)
            changes.append(f"{key}: {old_val} -> {new_val}")

    print()
    if changes:
        success("Config.h updated:")
        for c in changes:
            info(f"  {c}")
    else:
        info("No changes.")


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
        role_str = role_label(role)
        transport_str = transport_label(transport) if transport else "?"
        print()
        print(f"     \U0001f5a5\ufe0f {CYAN}{friendly_board}{RESET}")
        if role == "slave":
            print(f"     \U0001f517 {CYAN}RS485 Slave{RESET}")
        else:
            print(f"     \U0001f517 {CYAN}{role_str}{RESET}  {DIM}\u00b7{RESET}  {CYAN}{transport_str}{RESET}")

        # -- Debug status line ---------------------------------------------------
        via_wifi   = config_get("VERBOSE_MODE_WIFI_ONLY") == "1"
        via_serial = config_get("VERBOSE_MODE_SERIAL_ONLY") == "1"
        extended   = config_get("DEBUG_ENABLED") == "1"
        perf       = config_get("DEBUG_PERFORMANCE") == "1"
        debug_on   = via_wifi or via_serial

        if debug_on:
            label = "Debug (Extended)" if extended else "Debug"
            if via_wifi and via_serial:
                via = "WiFi + Serial"
            elif via_wifi:
                via = "WiFi"
            else:
                via = "Serial"
            perf_str = f"  {DIM}(w/ Performance Profiler){RESET}" if perf else ""
            print(f"     \U0001f41b {GREEN}{label} ENABLED{RESET} via {CYAN}{via}{RESET}{perf_str}")
        else:
            print(f"     \U0001f41b {DIM}Debug is DISABLED (see Misc Options to enable){RESET}")

        print()

        choice = menu_pick([
            ("Board / Options",      "board",     "normal"),
            ("Role / Transport",     "transport", "normal"),
            ("---",),
            ("COMPILE",              "compile",   "action"),
            *([("QUICK COMPILE", "qcompile", "action", "(" + active_ls + ")")] if active_ls != "(none)" else []),
            ("UPLOAD",               "upload",    "action"),
            ("---",),
            ("Misc Options",         "misc",      "normal"),
            ("Clear cache/build",    "clean",     "dim"),
            ("Exit",                 "exit",      "dim"),
        ])

        if choice is None:         # ESC on main menu — just redraw
            continue

        if choice == "exit":
            cprint(DIM, "  Bye!")
            break

        elif choice == "transport":
            result = configure_transport_and_role(prefs, board_has_dual_usb, preferred_usb_mode)
            if result is None:
                continue            # ESC pressed, back to menu
            save_prefs(prefs)
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "board":
            fqbn = select_board(prefs)
            if fqbn is None:
                continue            # ESC pressed
            prefs["fqbn"] = fqbn
            opts = configure_options(fqbn, prefs)
            if opts is None:
                continue            # ESC pressed
            prefs["options"] = opts
            save_prefs(prefs)
            success("Board configuration saved.")
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "compile":
            compile_ok = do_compile(prefs)
            if compile_ok:
                # Offer direct path to upload
                print(f"\n  {DIM}Press Enter to upload, Esc for menu...{RESET}")
                sys.stdout.write(HIDE_CUR)
                sys.stdout.flush()
                while True:
                    ch = msvcrt.getwch()
                    if ch == "\r":       # Enter → upload
                        sys.stdout.write(SHOW_CUR)
                        sys.stdout.flush()
                        if prefs.get("fqbn") and BUILD_DIR.exists():
                            upload_sketch(prefs["fqbn"], prefs.get("options", {}),
                                          board_name=prefs.get("board_name"))
                        else:
                            warn("Board not configured or no build found.")
                        input(f"\n  {DIM}Press Enter to continue...{RESET}")
                        break
                    elif ch == "\x1b":   # ESC → back to menu
                        sys.stdout.write(SHOW_CUR)
                        sys.stdout.flush()
                        break
            else:
                input(f"\n  {DIM}Press Enter to continue...{RESET}")

        elif choice == "qcompile":
            compile_ok = do_quick_compile(prefs)
            if compile_ok:
                print(f"\n  {DIM}Press Enter to upload, Esc for menu...{RESET}")
                sys.stdout.write(HIDE_CUR)
                sys.stdout.flush()
                while True:
                    ch = msvcrt.getwch()
                    if ch == "\r":
                        sys.stdout.write(SHOW_CUR)
                        sys.stdout.flush()
                        if prefs.get("fqbn") and BUILD_DIR.exists():
                            upload_sketch(prefs["fqbn"], prefs.get("options", {}),
                                          board_name=prefs.get("board_name"))
                        else:
                            warn("Board not configured or no build found.")
                        input(f"\n  {DIM}Press Enter to continue...{RESET}")
                        break
                    elif ch == "\x1b":
                        sys.stdout.write(SHOW_CUR)
                        sys.stdout.flush()
                        break
            else:
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

        elif choice == "misc":
            misc_options(prefs)

        elif choice == "clean":
            clean_build()
            input(f"\n  {DIM}Press Enter to continue...{RESET}")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n{DIM}  Interrupted.{RESET}")
        sys.exit(0)
