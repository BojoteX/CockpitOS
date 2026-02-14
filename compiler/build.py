"""
CockpitOS — Build, upload, and clean.

Compilation via arduino-cli, firmware upload, port scanning, and cleanup.
"""

import subprocess
import os
import sys
import re
import json
import time
import shutil
from pathlib import Path

from ui import (
    CYAN, DIM, RED, YELLOW, RESET,
    header, info, warn, error, cprint,
    pick, pick_live, big_success, big_fail,
    Spinner,
)
from config import (
    SKETCH_DIR, read_current_transport, read_current_role,
    transport_label, role_label, config_get,
)
from labels import read_active_label_set
import boards
from boards import run_cli

# -----------------------------------------------------------------------------
# Paths
# -----------------------------------------------------------------------------
def _resolve_arduino_data():
    """Resolve the Arduino data directory. Fails fast if LOCALAPPDATA is missing."""
    local = os.environ.get("LOCALAPPDATA")
    if not local:
        # Fallback: use the user's home directory
        local = str(Path.home() / "AppData" / "Local")
    return Path(local) / "Arduino15" / "CockpitOS"

_ARDUINO_DATA = _resolve_arduino_data()
BUILD_DIR     = _ARDUINO_DATA / "build"
CACHE_DIR     = _ARDUINO_DATA / "cache"
OUTPUT_DIR    = SKETCH_DIR / "compiler" / "output"

# -----------------------------------------------------------------------------
# Firmware naming (ALL CAPS)
# -----------------------------------------------------------------------------
def build_firmware_name():
    """Build a descriptive firmware filename from current config.

    Convention (ALL CAPS):
      Standalone:   LABEL_TRANSPORT.bin               e.g. MAIN_USB.bin
      RS485 Master: RS485_MASTER_LABEL_TRANSPORT.bin   e.g. RS485_MASTER_MAIN_USB.bin
      RS485 Slave:  RS485_SLAVE_LABEL.bin              e.g. RS485_SLAVE_MAIN.bin
    """
    label = (read_active_label_set() or "UNKNOWN").upper()
    role = read_current_role()
    transport = read_current_transport()

    transport_names = {
        "serial": "SERIAL", "usb": "USB", "wifi": "WIFI", "ble": "BLE",
    }

    if role == "slave":
        return f"RS485_SLAVE_{label}"
    elif role == "master":
        t = transport_names.get(transport, (transport or "UNKNOWN").upper())
        return f"RS485_MASTER_{label}_{t}"
    else:
        t = transport_names.get(transport, (transport or "UNKNOWN").upper())
        return f"{label}_{t}"


# -----------------------------------------------------------------------------
# Compile
# -----------------------------------------------------------------------------
def compile_sketch(fqbn, options, verbose=False, export_bin=False, board_name=None):
    header("Compiling CockpitOS")

    if options:
        opts_str = ",".join(f"{k}={v}" for k, v in options.items() if v is not None)
        full_fqbn = f"{fqbn}:{opts_str}" if opts_str else fqbn
    else:
        full_fqbn = fqbn

    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    CACHE_DIR.mkdir(parents=True, exist_ok=True)

    # Show build summary
    active_ls = read_active_label_set() or "?"
    transport = read_current_transport() or "?"
    role = read_current_role() or "?"

    fw_name = build_firmware_name()
    target = board_name or fqbn
    info(f"Firmware:  {CYAN}{fw_name}.bin{RESET}")
    info(f"Label Set: {CYAN}{active_ls}{RESET}")
    info(f"Transport: {CYAN}{transport_label(transport)}{RESET}")
    info(f"Role:      {CYAN}{role_label(role)}{RESET}")
    info(f"Board:     {CYAN}{target}{RESET}")
    print()

    cmd = [
        str(boards.ARDUINO_CLI), "compile",
        "--fqbn", full_fqbn,
        "--build-path", str(BUILD_DIR),
        "--build-cache-path", str(CACHE_DIR),
        str(SKETCH_DIR),
    ]

    if verbose:
        cmd.append("--verbose")

    if export_bin:
        cmd.extend(["--output-dir", str(SKETCH_DIR / "Binaries")])

    start = time.time()
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
        bufsize=1,
    )

    output_lines = []
    warning_count = 0
    error_count = 0

    if verbose:
        cprint(DIM, "  -- Compiler output -----------------------------------------------")
        print()
        for line in proc.stdout:
            output_lines.append(line)
            stripped = line.rstrip()
            if not stripped:
                continue
            if "error:" in stripped.lower():
                cprint(RED, f"  {stripped}")
                error_count += 1
            elif "warning:" in stripped.lower():
                cprint(YELLOW, f"  {stripped}")
                warning_count += 1
            else:
                print(f"  {DIM}{stripped}{RESET}")
        print()
        cprint(DIM, "  -------------------------------------------------------------")
    else:
        spinner = Spinner("Compiling")
        spinner.start()
        for line in proc.stdout:
            output_lines.append(line)
            if "error:" in line.lower():
                error_count += 1
            elif "warning:" in line.lower():
                warning_count += 1
        spinner.stop()

    proc.wait()
    elapsed = time.time() - start

    if proc.returncode == 0:
        # Parse size info — extract just the percentages
        program_pct = None
        memory_pct = None
        for line in output_lines:
            if "Sketch uses" in line:
                m = re.search(r'\((\d+)%\)', line)
                if m:
                    program_pct = m.group(1)
            elif "Global variables" in line:
                m = re.search(r'\((\d+)%\)', line)
                if m:
                    memory_pct = m.group(1)

        # Find the .bin, rename to descriptive ALL-CAPS name, copy to output/
        fw_name = build_firmware_name()
        for f in BUILD_DIR.iterdir():
            if f.suffix == ".bin" and "merged" not in f.name and f.name.startswith("CockpitOS"):
                build_dest = BUILD_DIR / f"{fw_name}.bin"
                shutil.copy2(f, build_dest)

                OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
                output_dest = OUTPUT_DIR / f"{fw_name}.bin"
                shutil.copy2(f, output_dest)
                break

        details = []
        if program_pct:
            details.append(f"Program space: {program_pct}%")
        if memory_pct:
            details.append(f"Dynamic memory: {memory_pct}%")
        details.append(f"Binary: {fw_name}.bin")
        if warning_count:
            details.append(f"Warnings: {warning_count}")

        big_success("BUILD SUCCESSFUL", details)
        return True
    else:
        error_lines = [l.rstrip() for l in output_lines if "error:" in l.lower()]
        details = [f"Exit code {proc.returncode}  ({elapsed:.1f}s)"]
        details.extend(error_lines[-10:])

        big_fail("BUILD FAILED", details)
        return False


# -----------------------------------------------------------------------------
# Upload
# -----------------------------------------------------------------------------
def scan_ports():
    """Scan for connected devices via arduino-cli. Returns [(display, address)]."""
    r = run_cli("board", "list", "--format", "json")
    if not r or r.returncode != 0:
        return []
    try:
        data = json.loads(r.stdout)
        entries = data.get("detected_ports", [])
        ports = []
        for entry in entries:
            p = entry.get("port", {})
            addr = p.get("address", "")
            proto_label = p.get("protocol_label", "")
            vid = p.get("properties", {}).get("vid", "")
            board_name = ""
            if entry.get("matching_boards"):
                board_name = entry["matching_boards"][0].get("name", "")

            if addr:
                parts = [addr]
                if board_name:
                    parts.append(board_name)
                elif proto_label:
                    parts.append(proto_label)
                if vid:
                    parts.append(f"VID:{vid}")
                display = " - ".join(parts)
                ports.append((display, addr))
        # Sort by COM port number (COM1, COM2, COM22) so the list is stable
        def _port_sort_key(item):
            addr = item[1]
            import re as _re
            m = _re.search(r'(\d+)', addr)
            return (int(m.group(1)) if m else 0, addr)
        ports.sort(key=_port_sort_key)
        return ports
    except (json.JSONDecodeError, KeyError):
        return []


def upload_sketch(fqbn, options, port=None, board_name=None):
    header("Upload to Device")

    if not port:
        info("Scanning for devices...")
        initial_ports = scan_ports()
        port = pick_live(
            "Select port:",
            initial_ports,
            scan_fn=scan_ports,
            interval=2.0,
            empty_message="No devices found — plug in your board...",
        )
        if port is None:
            return False

    if not port:
        error("No port specified.")
        return False

    if options:
        opts_str = ",".join(f"{k}={v}" for k, v in options.items() if v is not None)
        full_fqbn = f"{fqbn}:{opts_str}" if opts_str else fqbn
    else:
        full_fqbn = fqbn

    target = board_name or fqbn
    info(f"Board: {CYAN}{target}{RESET}")
    info(f"Port:  {port}")
    print()

    cmd = [
        str(boards.ARDUINO_CLI), "upload",
        "--fqbn", full_fqbn,
        "--port", port,
        "--input-dir", str(BUILD_DIR),
    ]

    start = time.time()
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        encoding="utf-8",
        errors="replace",
    )
    hash_verified = False
    for line in proc.stdout:
        stripped = line.rstrip()
        if stripped:
            print(f"  {stripped}")
        if "Hash of data verified" in line:
            hash_verified = True
    proc.wait()
    elapsed = time.time() - start

    if proc.returncode == 0:
        big_success("UPLOAD SUCCESSFUL", [f"Port: {port}", f"Time: {elapsed:.1f}s"])
        return True
    elif hash_verified:
        # Flash succeeded but esptool errored during post-reset reconnect.
        # Common on S2 boards (native USB re-enumerates after reset).
        warn("esptool reported an error after flashing, but data was verified.")
        big_success("UPLOAD SUCCESSFUL", [f"Port: {port}", f"Time: {elapsed:.1f}s",
                                          "Post-reset serial error ignored (S2 native USB)"])
        return True
    else:
        big_fail("UPLOAD FAILED", [f"Exit code {proc.returncode}", f"Port: {port}"])
        return False


# -----------------------------------------------------------------------------
# Clean
# -----------------------------------------------------------------------------
def clean_build():
    header("Clean Build")
    removed = 0
    for d in [BUILD_DIR, CACHE_DIR]:
        if d.exists():
            shutil.rmtree(d)
            info(f"Removed: {d}")
            removed += 1
    if OUTPUT_DIR.exists():
        for f in OUTPUT_DIR.iterdir():
            if f.suffix == ".bin":
                f.unlink()
                info(f"Removed: {f.name}")
                removed += 1
    if removed:
        info("Build directories cleaned.")
    else:
        info("Nothing to clean.")
