# hid_debug.py — minimal HID feature test for CockpitOS devices
# Usage: python hid_debug.py [--vid 0xCAFE --pid 0xCAF3]
import argparse, sys, time, hid

HANDSHAKE_REQ = b"DCSBIOS-HANDSHAKE"
FEATURE_REPORT_ID = 0       # matches your manager and descriptor
FEATURE_SIZE = 64           # matches your descriptor (64-byte Feature) :contentReference[oaicite:0]{index=0}

def hexline(b: bytes) -> str:
    return ' '.join(f'{x:02X}' for x in b)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--vid', type=lambda x:int(x,0), default=0xCAFE)
    ap.add_argument('--pid', type=lambda x:int(x,0), default=0xCAF3)
    args = ap.parse_args()

    devs = [d for d in hid.enumerate() if d['vendor_id']==args.vid and d['product_id']==args.pid]
    if not devs:
        print("No matching HID devices.")
        sys.exit(1)

    print(f"Found {len(devs)} device(s):")
    for i,d in enumerate(devs):
        print(f"[{i}] path={d.get('path')!r} prod={d.get('product_string')} serial={d.get('serial_number')}")

    # Open the first one (likely your BLE unit if USB panels are unplugged)
    d0 = devs[0]
    h = hid.device()
    h.open_path(d0['path'])
    h.set_nonblocking(True)
    print("\nOpened.")

    # 1) Raw GET_FEATURE (should succeed and return 65 bytes: [ID=0] + 64)
    try:
        data = h.get_feature_report(FEATURE_REPORT_ID, FEATURE_SIZE+1)
        print(f"GET_FEATURE ok ({len(data)} bytes): {hexline(bytes(data))}")
    except Exception as e:
        print(f"GET_FEATURE error: {e!r}")

    # 2) SET_FEATURE with handshake payload (ID byte + 64)
    payload = HANDSHAKE_REQ.ljust(FEATURE_SIZE, b'\x00')
    try:
        sent = h.send_feature_report(bytes([FEATURE_REPORT_ID]) + payload)
        print(f"SET_FEATURE sent {sent} bytes")
    except Exception as e:
        print(f"SET_FEATURE error: {e!r}")

    # 3) GET_FEATURE again (expect device to echo READY/zeros per your onRead logic)
    time.sleep(0.2)
    try:
        data = h.get_feature_report(FEATURE_REPORT_ID, FEATURE_SIZE+1)
        print(f"GET_FEATURE(again) ({len(data)} bytes): {hexline(bytes(data))}")
    except Exception as e:
        print(f"GET_FEATURE(again) error: {e!r}")

    # 4) Try a blocking INPUT read once (gamepad IN report)
    try:
        print("Waiting up to 1s for an input report...")
        pkt = h.read(FEATURE_SIZE, timeout_ms=1000)
        if pkt:
            print(f"INPUT report ({len(pkt)}): {hexline(bytes(pkt))}")
        else:
            print("No INPUT report in 1s (ok if idle).")
    except Exception as e:
        print(f"READ error: {e!r}")

    h.close()

if __name__ == "__main__":
    main()
