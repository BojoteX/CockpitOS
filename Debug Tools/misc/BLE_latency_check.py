#!/usr/bin/env python3
"""
BLE_latency_check.py — Device-initiated RTT measurement for BLE HID gamepads

PROTOCOL FLOW:
==============
1. User presses physical button on BLE device (LAT_TEST_PIN)
2. Device captures T0 = esp_timer_get_time(), sets lat_t0_sent
3. Device queues Feature Report with [MAGIC|seq|T0] (optional debug data)
4. Device sends Input Report with button state (trigger for host)
5. Host receives Input Report, detects button press (byte 32, bit 0)
6. Host echoes MAGIC via Output Report AS FAST AS POSSIBLE
7. Device receives echo, calculates RTT = now - lat_t0_sent
8. Device embeds RTT in Input Report bytes [36:39], sends to host
9. Host extracts RTT and displays result

WHAT RTT MEASURES:
==================
  RTT = T_echo_received - T_button_press
      = BLE_TX_latency + Host_processing + BLE_RX_latency

Since BLE is roughly symmetric:
  One-way latency ≈ RTT / 2

REQUIREMENTS:
=============
  pip install hid  (hidapi)
  Firmware must have MEASURE_LATENCY_TEST = 1
"""

import os
import sys
import argparse
import re
import statistics

try:
    import hid
except ImportError:
    print("ERROR: 'hid' module not found. Install with: pip install hid")
    sys.exit(1)

# Protocol constants (must match firmware)
MAGIC = 0xA5A55A5A
REPORT_ID = 0       # No explicit Report ID in descriptor; Windows needs leading 0
REPORT_LEN = 64

# Input Report layout (from BidireccionalNew.h):
#   [0:31]  - 16x 16-bit axes (32 bytes)
#   [32:35] - 32 buttons (4 bytes) — bit 0 of byte 32 = latency test button
#   [36:63] - Padding (28 bytes) — bytes [36:39] = RTT result (uint32_t, µs)


def read_pid_from_header():
    """Try to read PID from active_set.h for auto-detection."""
    path = os.path.join(os.path.dirname(__file__), "../src/LABELS/active_set.h")
    try:
        with open(path, "r", encoding="utf-8") as f:
            first_line = f.readline().strip()
        m = re.match(r"//\s*PID:0x([0-9A-Fa-f]+)", first_line)
        if m:
            return int(m.group(1), 16)
    except Exception:
        pass
    return 0xB5D3  # Fallback default


def find_dev(vid, pid, product_substr):
    """Find matching HID device."""
    for d in hid.enumerate():
        if vid and d['vendor_id'] != vid:
            continue
        if pid and d.get('product_id', 0) != pid:
            continue
        if product_substr and product_substr.lower() not in (d.get('product_string') or '').lower():
            continue
        return d
    return None


def open_dev(vid, pid, product):
    """Open HID device, return (device, info) tuple."""
    info = find_dev(vid, pid, product)
    if not info:
        raise RuntimeError(f"Device not found (VID=0x{vid:04X}, PID=0x{pid:04X})")
    dev = hid.device()
    dev.open_path(info['path'])
    dev.set_nonblocking(False)  # Blocking reads — we wait for device events
    return dev, info


def strip_report_id(data):
    """
    Remove leading Report ID byte if present.
    Windows HID adds a leading byte (Report ID) even when descriptor has none.
    """
    if data and len(data) == 65:
        return bytes(data[1:])
    return bytes(data) if data else b''


def main():
    ap = argparse.ArgumentParser(
        description="BLE HID latency tester (device-initiated RTT measurement)",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
USAGE:
  1. Ensure firmware has MEASURE_LATENCY_TEST = 1
  2. Connect BLE device to this computer
  3. Run this script
  4. Press the button on your device repeatedly
  5. Press Ctrl+C to stop and see statistics
        """
    )
    ap.add_argument("--vid", type=lambda s: int(s, 0), default=0xCAFE,
                    help="Vendor ID (default: 0xCAFE)")
    ap.add_argument("--pid", type=lambda s: int(s, 0), default=None,
                    help="Product ID (default: auto-detect from active_set.h)")
    ap.add_argument("--product", type=str, default="",
                    help="Product name substring filter")
    ap.add_argument("--debug", action="store_true",
                    help="Enable debug output")
    args = ap.parse_args()

    # Auto-detect PID if not specified
    if args.pid is None:
        args.pid = read_pid_from_header()

    try:
        dev, info = open_dev(args.vid, args.pid, args.product)
    except RuntimeError as e:
        print(f"ERROR: {e}")
        print("\nAvailable HID devices:")
        for d in hid.enumerate():
            print(f"  VID=0x{d['vendor_id']:04X} PID=0x{d.get('product_id',0):04X} "
                  f"Name={d.get('product_string', 'N/A')}")
        sys.exit(1)

    print(f"Opened: VID=0x{info['vendor_id']:04X} PID=0x{info.get('product_id',0):04X} "
          f"Name={info.get('product_string', 'N/A')}")
    print("=" * 65)
    print(" DEVICE-INITIATED LATENCY TEST")
    print(" Press the latency test button on your BLE device.")
    print(" The device measures RTT; this script just echoes the trigger.")
    print(" Press Ctrl+C to stop and show statistics.")
    print("=" * 65)

    samples = []
    pending_echo = False  # True after we send echo, waiting for RTT response
    last_button_state = False

    try:
        while True:
            # Blocking read for Input Report
            raw = dev.read(65)
            if not raw:
                continue

            data = strip_report_id(raw)
            if len(data) < 40:  # Need at least 40 bytes to read RTT field
                if args.debug:
                    print(f"  [DEBUG] Short report: {len(data)} bytes")
                continue

            # Extract RTT from bytes [36:39] (uint32_t, microseconds)
            rtt_us = int.from_bytes(data[36:40], 'little')

            # Extract button state (byte 32, bit 0)
            button_pressed = (data[32] & 0x01) != 0

            if args.debug:
                print(f"  [DEBUG] btn={int(button_pressed)} rtt_us={rtt_us} pending={pending_echo}")

            # CASE 1: RTT response received
            if rtt_us != 0:
                rtt_ms = rtt_us / 1000.0
                one_way_ms = rtt_ms / 2.0
                samples.append(rtt_ms)
                pending_echo = False
                print(f"[LATENCY] RTT = {rtt_ms:7.3f} ms  (one-way ≈ {one_way_ms:6.3f} ms)  "
                      f"[n={len(samples)}]")
                continue

            # CASE 2: Button press detected (rising edge) — send echo
            # Only echo on transition from released→pressed, and not already waiting
            if button_pressed and not last_button_state and not pending_echo:
                # Build Output Report with MAGIC (echo trigger)
                # Format: [ReportID=0] + [MAGIC (4 bytes)] + [padding (60 bytes)]
                echo_payload = MAGIC.to_bytes(4, 'little') + bytes(60)
                echo_packet = bytes([REPORT_ID]) + echo_payload
                
                try:
                    dev.write(echo_packet)
                    pending_echo = True
                    if args.debug:
                        print("  [DEBUG] Echo sent")
                except Exception as e:
                    print(f"  [ERROR] Failed to send echo: {e}")

            # CASE 3: Button release or other state — just update tracking
            last_button_state = button_pressed

    except KeyboardInterrupt:
        print("\n")
    finally:
        try:
            dev.close()
        except Exception:
            pass

    # Print statistics
    if samples:
        print("=" * 65)
        print(f" STATISTICS ({len(samples)} samples)")
        print("=" * 65)
        mean_rtt = statistics.mean(samples)
        median_rtt = statistics.median(samples)
        min_rtt = min(samples)
        max_rtt = max(samples)
        stdev_rtt = statistics.stdev(samples) if len(samples) > 1 else 0.0

        print(f"  Mean RTT:     {mean_rtt:7.3f} ms  (one-way ≈ {mean_rtt/2:6.3f} ms)")
        print(f"  Median RTT:   {median_rtt:7.3f} ms  (one-way ≈ {median_rtt/2:6.3f} ms)")
        print(f"  Std Dev:      {stdev_rtt:7.3f} ms")
        print(f"  Min RTT:      {min_rtt:7.3f} ms")
        print(f"  Max RTT:      {max_rtt:7.3f} ms")
        print(f"  Range:        {max_rtt - min_rtt:7.3f} ms")
        print("=" * 65)

        # Interpretation guide
        print("\n INTERPRETATION:")
        if mean_rtt < 15:
            print("  ✓ Excellent — competitive gaming grade (<15ms RTT)")
        elif mean_rtt < 30:
            print("  ✓ Good — suitable for most gaming (<30ms RTT)")
        elif mean_rtt < 50:
            print("  ~ Acceptable — casual gaming (<50ms RTT)")
        else:
            print("  ✗ High latency — may affect gameplay (>50ms RTT)")
        
        print(f"\n  Note: One-way latency (button→host) ≈ {mean_rtt/2:.1f}ms")
        print("        BLE connection interval also affects jitter.")
    else:
        print("No samples collected. Did you press the latency test button?")


if __name__ == "__main__":
    main()
