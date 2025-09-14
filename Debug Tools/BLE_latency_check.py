#!/usr/bin/env python3
# BLE_latency_check_OUT.py — Input->Output RTT echo using Output reports only
# pip install hid     (hidapi)
import os, sys, time, argparse, hid, re

os.chdir(os.path.dirname(os.path.abspath(__file__)))

MAGIC       = 0xA5A55A5A
REPORT_ID   = 0        # your HID has no explicit IDs; Windows needs a leading 0 byte
REPORT_LEN  = 64

def read_pid_from_header():
    path = os.path.join(os.path.dirname(__file__), "../src/LABELS/active_set.h")
    try:
        with open(path, "r", encoding="utf-8") as f:
            first_line = f.readline().strip()
        m = re.match(r"//\s*PID:0x([0-9A-Fa-f]+)", first_line)
        if m:
            return int(m.group(1), 16)
    except Exception as e:
        pass
    return 0xB5D3  # fallback default if file missing or malformed

def find_dev(vid, pid, product_substr):
    cand = []
    for d in hid.enumerate():
        if vid and d['vendor_id'] != vid: continue
        if pid and d.get('product_id',0) != pid: continue
        if product_substr and product_substr.lower() not in (d.get('product_string') or '').lower():
            continue
        cand.append(d)
    if not cand: return None
    return cand[0]

def open_dev(vid, pid, product):
    info = find_dev(vid, pid, product)
    if not info: raise RuntimeError("Device not found")
    dev = hid.device(); dev.open_path(info['path'])
    dev.set_nonblocking(0)  # blocking read on IN reports
    return dev, info

def build_out(seq, t0_us):
    p = bytearray(64)
    p[0:4]  = (MAGIC).to_bytes(4, 'little')
    p[4:8]  = (seq).to_bytes(4, 'little')
    p[8:16] = (t0_us).to_bytes(8, 'little')
    return bytes(p)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--vid", type=lambda s:int(s,0), default=0xCAFE)
    ap.add_argument("--pid", type=lambda s:int(s,0), default=read_pid_from_header())
    ap.add_argument("--product", type=str, default="")
    ap.add_argument("--hz", type=int, default=10)
    args = ap.parse_args()

    dev, info = open_dev(args.vid, args.pid, args.product)
    print(f"Opened: VID=0x{info['vendor_id']:04X} PID=0x{info.get('product_id',0):04X} Name={info.get('product_string')}")

    period = 1.0 / max(1, args.hz)
    seq = 0
    try:
        while True:
            seq = (seq + 1) & 0xFFFFFFFF
            t0_us = time.perf_counter_ns() // 1000
            payload = build_out(seq, t0_us)
            packet  = bytes([REPORT_ID]) + payload   # Windows needs leading ReportID=0
            dev.write(packet)

            data = dev.read(64)                     # blocking
            if not data: return
            if len(data) == 65 and data[0] == 0:    # strip Report ID if present
                data = data[1:]

            rtt_us = int.from_bytes(data[36:40], 'little')
            if rtt_us == 0:
                rtt_us = int.from_bytes(data[0:4], 'little')

            if rtt_us:
                print(f"[LATENCY] RTT {rtt_us/1000.0:.3f} ms")

            t_spare = period - (time.perf_counter_ns()/1e9 - (t0_us/1e6))
            if t_spare > 0:
                time.sleep(t_spare)
    finally:
        try: dev.close()
        except: pass

if __name__ == "__main__":
    main()