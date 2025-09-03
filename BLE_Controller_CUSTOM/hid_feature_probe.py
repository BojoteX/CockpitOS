# hid_feature_probe.py
import hid, time
VID, PID = 0xCAFE, 0xCAF3
for d in hid.enumerate(VID, PID):
    print("path:", d['path'], "prod:", d.get('product_string'), "serial:", d.get('serial_number'))
    h = hid.device(); h.open_path(d['path'])
    print("Opened.")
    # GET_FEATURE (ID 0, buffer includes ID byte)
    buf = bytearray(65); buf[0] = 0x00
    try:
        n = h.get_feature_report(0, 65)
        print("GET_FEATURE ok, len:", len(n))
        print(n)
    except OSError as e:
        print("GET_FEATURE error:", e)
    # SET_FEATURE handshake
    payload = b"DCSBIOS-HANDSHAKE".ljust(64, b'\x00')
    try:
        sent = h.send_feature_report(bytes([0])+payload)
        print("SET_FEATURE sent:", sent)
    except OSError as e:
        print("SET_FEATURE error:", e)
    h.close()
    break
