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
import re
import json
import msvcrt
import ctypes
import subprocess
from pathlib import Path

# ---------------------------------------------------------------------------
# Module imports
# ---------------------------------------------------------------------------
from ui import (
    CYAN, BOLD, DIM, GREEN, YELLOW, RESET, HIDE_CUR, SHOW_CUR,
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
    board_has_dual_usb, preferred_usb_mode,
    validate_config_vs_board, FATAL, WARNING,
    select_board, configure_options,
    resolve_arduino_cli,
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
    load_config_snapshot()  # Re-read Config.h — may have changed externally
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

    fresh = load_prefs()
    show_detail = fresh.get("show_detailed_warnings", False)
    return compile_sketch(fqbn, opts, verbose=verbose, board_name=prefs.get("board_name"),
                          show_detailed_warnings=show_detail)


# -----------------------------------------------------------------------------
# Misc Options sub-menu
# -----------------------------------------------------------------------------
def misc_options(prefs):
    """Sub-menu for Wi-Fi credentials, debug toggles, and advanced settings."""
    while True:
        header("Misc Options")
        choice = pick("Select:", [
            ("Wi-Fi Credentials",       "wifi"),
            ("Debug / Verbose Toggles", "debug"),
            ("Advanced Settings",       "advanced"),
            ("Clear cache/build",       "clean"),
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
        elif choice == "advanced":
            configure_advanced_settings()
            input(f"\n  {DIM}Press Enter to continue...{RESET}")
        elif choice == "clean":
            clean_build()
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
    load_config_snapshot()  # Re-read Config.h — may have changed externally
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
    failed = []
    for label, key in toggles:
        new_val = "1" if result[key] else "0"
        old_val = config_get(key) or "0"
        if new_val != old_val:
            if config_set(key, new_val):
                changes.append(f"{key}: {old_val} -> {new_val}")
            else:
                failed.append(key)

    print()
    if failed:
        error(f"Failed to write Config.h for: {', '.join(failed)}")
        error("Config.h may be locked by another program or read-only.")
    if changes:
        success("Config.h updated:")
        for c in changes:
            info(f"  {c}")
    elif not failed:
        info("No changes.")


def configure_advanced_settings():
    """Advanced settings — Config.h toggles and compiler preferences."""
    load_config_snapshot()  # Re-read Config.h — may have changed externally
    header("Advanced Settings")

    transport = read_current_transport()
    role      = read_current_role()

    if role == "slave":
        print()
        warn("HID mode does not apply to slaves. Slaves are RS485-only devices.")
        warn("HID mode is configured on the master or standalone device.")
        print()

    if role != "slave" and transport not in ("usb", "ble"):
        print()
        error("HID mode requires USB or BLE transport. "
              "Current config will fail to compile if HID is enabled.")
        print()

    # --- Config.h toggles ---
    config_toggles = [
        ("HID mode by default",  "MODE_DEFAULT_IS_HID"),
    ]

    config_items = []
    for label, key in config_toggles:
        val = config_get(key) or "0"
        config_items.append((label, key, val != "0"))

    # --- Compiler preference toggles ---
    prefs = load_prefs()
    show_detail = prefs.get("show_detailed_warnings", False)

    all_items = config_items + [
        ("Show detailed warnings", "show_detailed_warnings", show_detail),
    ]

    result = toggle_pick("Toggle options:", all_items)
    if result is None:
        return

    # Apply Config.h changes
    changes = []
    failed = []
    for label, key in config_toggles:
        new_val = "1" if result[key] else "0"
        old_val = config_get(key) or "0"
        if new_val != old_val:
            if config_set(key, new_val):
                changes.append(f"{key}: {old_val} -> {new_val}")
            else:
                failed.append(key)

    # Apply compiler preference changes
    new_detail = result.get("show_detailed_warnings", False)
    if new_detail != show_detail:
        prefs["show_detailed_warnings"] = new_detail
        save_prefs(prefs)
        state_str = "ON" if new_detail else "OFF"
        changes.append(f"Detailed warnings: {state_str}")

    print()
    if failed:
        error(f"Failed to write Config.h for: {', '.join(failed)}")
        error("Config.h may be locked by another program or read-only.")
    if changes:
        success("Settings updated:")
        for c in changes:
            info(f"  {c}")
    elif not failed:
        info("No changes.")


# =============================================================================
#  Tool switcher — launch sibling CockpitOS tools
# =============================================================================
_PROJECT_ROOT = Path(__file__).resolve().parent.parent
_lock_path = None          # set in main(), cleaned up before exec

def _launch_tool(script_name):
    """Replace the current process with a sibling CockpitOS tool.

    Uses os.execl so there is no nesting — the old tool is gone,
    the new one takes over the same console window.
    """
    script = _PROJECT_ROOT / script_name
    if not script.exists():
        error(f"{script_name} not found.")
        return
    # Clean up our lock file before we disappear
    if _lock_path:
        _lock_path.unlink(missing_ok=True)
    cls()
    os.execl(sys.executable, sys.executable, str(script))


# =============================================================================
#  Main menu
# =============================================================================
def _bring_existing_to_front():
    """Find an existing CockpitOS tool window and bring it to the foreground."""
    user32 = ctypes.windll.user32
    hwnd = user32.FindWindowW(None, "CockpitOS Management Tool")
    if hwnd:
        SW_RESTORE = 9
        user32.ShowWindow(hwnd, SW_RESTORE)
        user32.SetForegroundWindow(hwnd)
        return True
    return False

def main():
    import platform
    if platform.system() != "Windows":
        print("\n  CockpitOS Management Tool requires Windows.")
        print("  Linux and macOS are not supported.\n")
        sys.exit(1)

    os.system("")  # Enable ANSI on Windows

    # --- Single-instance guard (lock file) ---
    global _lock_path
    lock_path = Path(__file__).parent / ".cockpitos.lock"
    _lock_path = lock_path
    def _is_cockpitos_process(pid):
        """Check if a PID belongs to a running Python process (not a recycled PID)."""
        try:
            kernel32 = ctypes.windll.kernel32
            psapi = ctypes.windll.psapi
            handle = kernel32.OpenProcess(0x0400 | 0x0010, False, pid)  # PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
            if not handle:
                return False
            try:
                buf = ctypes.create_unicode_buffer(260)
                if psapi.GetModuleFileNameExW(handle, None, buf, 260):
                    return "python" in buf.value.lower()
                return False
            finally:
                kernel32.CloseHandle(handle)
        except Exception:
            return False

    try:
        # Exclusive create — fails if file already exists
        lock_fd = os.open(str(lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, str(os.getpid()).encode())
        os.close(lock_fd)
    except FileExistsError:
        # Check if the existing process is actually CockpitOS (not a recycled PID)
        still_running = False
        try:
            old_pid = int(lock_path.read_text().strip())
            if _is_cockpitos_process(old_pid):
                # Confirmed Python process — check if it has our window title
                if _bring_existing_to_front():
                    still_running = True
        except (ValueError, OSError):
            pass

        if still_running:
            print("\n  CockpitOS tool is already running. Switching to existing window.")
            sys.exit(0)

        # Stale lock (crash, recycled PID, or no window found) — reclaim it
        lock_path.unlink(missing_ok=True)
        lock_fd = os.open(str(lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
        os.write(lock_fd, str(os.getpid()).encode())
        os.close(lock_fd)

    # Clean up lock on exit
    import atexit
    atexit.register(lambda: lock_path.unlink(missing_ok=True))

    # Set window title so other instances can find us
    ctypes.windll.kernel32.SetConsoleTitleW("CockpitOS Management Tool")

    prefs = load_prefs()

    # Resolve arduino-cli path (registry → common dirs → PATH → user prompt)
    cli = resolve_arduino_cli(prefs)
    if not cli:
        import boards
        print()
        warn("Arduino IDE not found automatically.")
        info("Searched: Windows registry, common install paths, system PATH.")
        print()
        info("Please enter the path to your Arduino IDE folder")
        info(f"{DIM}e.g. C:\\Program Files\\Arduino IDE{RESET}")
        raw = input("\n  Arduino IDE path: ").strip().strip('"')
        if raw:
            candidate = Path(raw) / boards._CLI_RELATIVE
            if candidate.exists():
                boards.ARDUINO_CLI = candidate
                save_prefs(prefs)
                success(f"Arduino CLI found.")
            else:
                error(f"arduino-cli.exe not found at expected location.")
                error("Make sure you entered the Arduino IDE root folder.")
                sys.exit(1)
        else:
            error("Cannot proceed without arduino-cli.")
            sys.exit(1)
    else:
        save_prefs(prefs)  # Cache the resolved path if newly found

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
        # Reload prefs AND Config.h from disk every iteration.
        # Config.h is the source of truth — it may change externally
        # (git, editor, AI agent) while the tool is running.
        prefs = load_prefs()
        load_config_snapshot()
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
        cprint(CYAN + BOLD, "              Compile Tool")
        print()
        role_str = role_label(role)
        transport_str = transport_label(transport) if transport else "?"
        print()

        # -- Read Wi-Fi SSID once (used by transport and/or debug lines) --------
        via_wifi   = config_get("VERBOSE_MODE_WIFI_ONLY") == "1"
        via_serial = config_get("VERBOSE_MODE_SERIAL_ONLY") == "1"
        needs_wifi = transport == "wifi" or via_wifi
        wifi_ssid = None
        if needs_wifi:
            if WIFI_H.exists():
                m = re.search(r'#define\s+WIFI_SSID\s+"([^"]*)"',
                              WIFI_H.read_text(encoding="utf-8"))
                if m:
                    wifi_ssid = m.group(1)
            if not wifi_ssid:
                wifi_ssid = "TestNetwork"
        ssid_shown = False   # only show SSID once

        # -- Board line ----------------------------------------------------------
        show_detail = prefs.get("show_detailed_warnings", False)
        detail_str = f"  {YELLOW}(w/ verbose warnings){RESET}" if show_detail else ""
        print(f"     \U0001f5a5\ufe0f {CYAN}{friendly_board}{RESET}{detail_str}")

        # -- Role / Transport line -----------------------------------------------
        if role == "slave":
            addr = config_get("RS485_SLAVE_ADDRESS") or "?"
            print(f"     \U0001f517 {CYAN}RS485 Slave #{addr}{RESET}")
        else:
            transport_parts = f"{CYAN}{role_str}{RESET}  {DIM}\u00b7{RESET}  {CYAN}{transport_str}{RESET}"
            if transport == "wifi":
                transport_parts += f"  {DIM}\u00b7  SSID:{RESET} {CYAN}{wifi_ssid}{RESET}"
                ssid_shown = True
            print(f"     \U0001f517 {transport_parts}")

        # -- HID mode line -------------------------------------------------------
        hid_on = config_get("MODE_DEFAULT_IS_HID") == "1"
        if hid_on:
            print(f"     \U0001f3ae\ufe0f {GREEN}HID Mode ON{RESET}  {DIM}(device acts as a gamepad in Windows){RESET}")

        # -- Debug status line ---------------------------------------------------
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
            # Append SSID to debug line if WiFi debug is on but SSID wasn't shown on transport line
            ssid_str = ""
            if via_wifi and not ssid_shown:
                ssid_str = f"  {DIM}\u00b7  SSID:{RESET} {CYAN}{wifi_ssid}{RESET}"
            print(f"     \U0001f41b {GREEN}{label} ENABLED{RESET} via {CYAN}{via}{RESET}{ssid_str}{perf_str}")
        else:
            print(f"     \U0001f41b {DIM}Debug is DISABLED (see Misc Options to enable){RESET}")

        print()

        choice = menu_pick([
            ("Board / Options",      "board",     "normal"),
            ("Role / Transport",     "transport", "normal"),
            ("Misc Options",         "misc",      "normal"),
            ("",),
            ("action_bar", [
                ("Compile", "compile"),
                ("Upload", "upload"),
            ]),
            ("",),
            ("---", "Switch Tool"),
            ("Label Creator",        "label",     "normal"),
            ("Environment Setup",    "setup",     "normal"),
            ("",),
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

        elif choice == "label":
            _launch_tool("LabelCreator-START.py")

        elif choice == "setup":
            _launch_tool("Setup-START.py")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n{DIM}  Interrupted.{RESET}")
        sys.exit(0)
