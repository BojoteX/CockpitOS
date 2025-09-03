# bleak_feature_probe.py
import asyncio, struct
from bleak import BleakClient, BleakScanner

UUID_REPORT   = "00002a4d-0000-1000-8000-00805f9b34fb"
UUID_REP_REF  = "00002908-0000-1000-8000-00805f9b34fb"  # Report Reference desc

TARGET_NAME = "BojoteX NavX"

async def main():
    devs = await BleakScanner.discover()
    d = next((x for x in devs if x.name == TARGET_NAME), None)
    if not d: 
        print("device not found"); return
    async with BleakClient(d) as c:
        svcs = await c.get_services()
        feat_char = None
        for ch in svcs.characteristics:
            if ch.uuid.lower() == UUID_REPORT and "read" in ch.properties:
                # check its 0x2908 Report Reference
                for desc in ch.descriptors:
                    if desc.uuid.lower() == UUID_REP_REF:
                        rr = await c.read_gatt_descriptor(desc.handle)
                        if len(rr) >= 2 and rr[1] == 0x03:  # type = Feature
                            feat_char = ch; break
            if feat_char: break
        if not feat_char:
            print("Feature report char not found"); return
        v = await c.read_gatt_char(feat_char)
        print("READ feature len:", len(v))
        payload = b"DCSBIOS-HANDSHAKE".ljust(64, b'\x00')
        await c.write_gatt_char(feat_char, payload, response=True)
        v2 = await c.read_gatt_char(feat_char)
        print("READ-after-write len:", len(v2))
asyncio.run(main())
