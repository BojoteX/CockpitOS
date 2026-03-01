# CockpitOS Critical Bugs — Transport Layer

**Scope:** All bugs discovered during the WiFi/Serial/USB transport debug effort (Sessions 1-6, Feb 2026).
**Detailed traces:** `Docs/Bugs/WiFi-Oversized-Frame-Drop.md`

---

## Bug Summary

| # | Bug | Severity | Transport | Status |
|---|-----|----------|-----------|--------|
| 1 | WiFi IP fragmentation — oversized UDP dropped | High | WiFi | Open (unfixable in firmware) |
| 2 | Protocol.h `unsigned int` on ESP32 is 32-bit, not 16-bit | High | All ESP32 | **Fixed** |
| 3 | Serial RX buffer overflow (512B < 1954B frame) | High | Serial/HWCDC/USBSerial | **Fixed** |
| 4 | Ring buffer `reassemblyLen` race condition (cross-core) | High | WiFi | **Fixed** |
| 5 | WiFi `DCS_UDP_PACKET_MAXLEN` too small (128 < frame sizes) | Medium | WiFi | **Fixed** |
| 6 | USB HID zero-padding creates phantom protocol writes | High | USB/BLE | **Fixed** |

---

## Bug 1: WiFi IP Fragmentation (OPEN)

**Root cause:** Apache AH-64D Frame 1 is 1954 bytes. The OS fragments this into two IP packets. ESP32's lwIP has `CONFIG_LWIP_IP4_REASSEMBLY` disabled (compiled out in pre-built Arduino libs). Fragments are silently dropped — the packet never reaches `AsyncUDP.onPacket()`.

**Impact:** All module outputs (LEDs, selectors, displays) fail on WiFi for any aircraft with a frame > 1500 bytes.

**Workaround:** PLAY script `--split-frames` splits oversized UDP datagrams before transmission. For production, HID Manager or DCS-BIOS Lua could split at the source.

**Cannot fix in firmware** — the drop happens in the IP layer before application code runs.

---

## Bug 2: Protocol Parser Type Mismatch (FIXED)

**Root cause:** `Protocol.h` declared `address`, `count`, `data` as `volatile unsigned int`. On AVR (original target), `unsigned int` = 16-bit, matching the DCS-BIOS protocol. On ESP32, `unsigned int` = 32-bit, breaking overflow/wrap semantics.

**Key impact:** `address += 2` at 0xFFFE should wrap to 0x0000 (16-bit) but goes to 0x10000 (32-bit). `count - 1` at 0 gives 0xFFFFFFFF instead of 0xFFFF. Listener flush behavior differs from AVR.

**Fix:** Changed to `volatile uint16_t` in `Protocol.h`, updated casts in `Protocol.cpp`.

**Files:** `lib/DCS-BIOS/src/internal/Protocol.h`, `lib/DCS-BIOS/src/internal/Protocol.cpp`

---

## Bug 3: Serial RX Buffer Overflow (FIXED)

**Root cause:** `SERIAL_RX_BUFFER_SIZE` was 512 bytes. Socat delivers full UDP frames as UART bursts. Apache Frame 1 (1954B) overflows the buffer, corrupting the parser state.

**Impact:** HWCDC and USBSerial fail with Apache stream. F/A-18C (max 572B frames) works fine.

**Fix:** Increased `SERIAL_RX_BUFFER_SIZE` to 4096 in `Config.h`.

**Why USB/BLE are immune:** HID Manager slices frames into 64-byte HID reports with built-in flow control.

---

## Bug 4: Ring Buffer `reassemblyLen` Race Condition (FIXED)

**Root cause:** In `onDcsBiosUdpPacket()` (DCSBIOSBridge.cpp:793), `reassemblyLen` was a **local variable** that reset to 0 on every call.

ESP32 dual-core architecture: WiFi callback (core 0) pushes chunks via `dcsUdpRingbufPushChunked()`, main loop (core 1) drains via `onDcsBiosUdpPacket()`. When a WiFi packet exceeds `DCS_UDP_PACKET_MAXLEN`, it's split into multiple ring buffer entries. The push is sequential — between pushing chunk 1 and chunk 2, the consumer can drain chunk 1 and exit.

**Race sequence (Frame 0, 138B, chunked into 128+10):**
1. Producer pushes chunk 1 (128B, `isLastChunk=false`)
2. Consumer drains, pops chunk 1, copies to `frames[0].data + 0`. `reassemblyLen=128`. Ring empty, exits. No complete frame, no parsing.
3. Producer pushes chunk 2 (10B, `isLastChunk=true`)
4. Consumer drains, **`reassemblyLen=0` (local reset!)**. Pops chunk 2, copies to `frames[0].data + 0` (**overwrites chunk 1 data**). Treats 10B as complete frame. **Corrupt.**

**Observed symptom:** Aircraft name truncated to "AH-64D" instead of "AH-64D_BLK_II". Frame 0 (which carries the aircraft name at addr 0x0000) was the exact frame affected — at 138B, it's the only frame near the 128B `PACKET_MAXLEN` boundary.

**Why BLE is immune:** BLE receives via HID Manager as small HID reports (< 128B). Every entry has `isLastChunk=true`. No multi-chunk reassembly.

**Why USB is immune:** USB HID reports are exactly 64B = `DCS_UDP_PACKET_MAXLEN`. Every entry fits in one slot with `isLastChunk=true`.

**Fix:** Changed `size_t reassemblyLen = 0;` to `static size_t reassemblyLen = 0;` — persists partial reassembly state across drain calls.

**File:** `src/Core/DCSBIOSBridge.cpp` line 793

---

## Bug 5: WiFi `DCS_UDP_PACKET_MAXLEN` Too Small (FIXED)

**Root cause:** WiFi's `DCS_UDP_PACKET_MAXLEN` was 128 bytes. Any incoming UDP packet > 128B triggered `dcsUdpRingbufPushChunked()` to split across multiple ring buffer slots, exposing the cross-core race (Bug 4). With BLE using the same code path but receiving small HID reports that never need splitting, BLE was immune.

**Fix:** Increased WiFi/BLE `DCS_UDP_PACKET_MAXLEN` from 128 to `UDP_MAX_SiZE` (1460) in `Config.h`. Reduced `DCS_UDP_RINGBUF_SIZE` from 64 to 16 to compensate for larger slot size (16 * 1460 = ~23KB vs 64 * 128 = ~8KB).

With `PACKET_MAXLEN=1460`, every WiFi UDP datagram (max 1460B after IP) fits in a single ring buffer slot with `isLastChunk=true`. No multi-chunk splitting. No race condition possible. This is belt-and-suspenders with the `static reassemblyLen` fix (Bug 4).

**File:** `Config.h` lines 161-162

---

## Bug 6: USB HID Zero-Padding Phantom Writes (FIXED)

**Root cause:** HID Manager slices each UDP frame into 64-byte HID Output Reports, **zero-padding the last chunk** to meet the fixed-size USB HID report requirement. Those trailing zeros are valid DCS-BIOS protocol data that the firmware parser interprets as real writes.

**Mechanism — how 0x00 padding creates phantom writes:**

1. After a frame's last real data byte, the parser is in `ADDRESS_LOW`
2. Zero bytes create: `addr=0x0000, count=0x0000`
3. In `DATA_LOW` with count=0, first data byte does `count = count - 1` = **65535** (uint16_t underflow)
4. Parser writes `onDcsBiosWrite(0x0000, 0x0000)` — **zeros out the aircraft name buffer**
5. Continues writing zeros to sequential addresses (0x0002, 0x0004, ...) covering the entire 24-byte aircraft name range
6. Next SYNC triggers `flushRemainingListeners()` — commits zeroed aircraft name — **mission STOP**
7. Next cycle's Frame 0 writes the real aircraft name back — **mission START**

Frame 0 (138 bytes) produces 3 HID reports: [64] [64] [10 real + 54 zero-pad]. Those 54 zero bytes create 25 phantom writes to addresses 0x0000-0x0030, obliterating the aircraft name that Frame 0 itself had just written.

**Impact:** Mission stop/start cycling on every PLAY script frame. Adjacent DCS-BIOS output values corrupted by phantom zero-writes at low addresses.

### Why real DCS worked despite 0x00 padding

In real DCS, each complete export frame contains both the aircraft name data AND an end marker (addr=0xFFFE). The end marker triggers address-progression flush of `DcsBiosSniffer` (registered at 0x0000-0xFFFD, masked to 0xFFFC). This commits the aircraft name with real data **before** padding arrives. After the flush, `startESL` becomes NULL, so all subsequent phantom writes from padding are silently dropped — the dispatch code skips them entirely.

The PLAY script's Frame 0 has **no end marker** (it's in Frame 1), so `DcsBiosSniffer` stays in the dispatch chain during padding, and the phantom writes reach the aircraft name buffer.

### Why 0x55 padding doesn't work

0x55 padding creates SYNC sequences (4x 0x55 = SYNC). But 54 padding bytes / 4 = 13 SYNCs + **2 leftover 0x55 bytes**. Those leftover bytes carry into the next HID report and consume 2 of the next frame's real SYNC bytes. The parser ends up stuck in `WAIT_FOR_SYNC`, dropping the entire following frame. Same data-loss pattern as the Serial RX overflow (Bug 3).

### Fix: 0xFF padding

Changed `PADDING_BYTE` from `0x00` to `0xFF` in HID Manager.

**How 0xFF padding works:**

1. `0xFF 0xFF` → addr = 0xFFFF (valid address, not SYNC)
2. `0xFF 0xFF` → count = 0xFFFF
3. First data word at addr 0xFFFF → address progression: `DcsBiosSniffer` lastAddr (0xFFFC) < 0xFFFF → **flush fires** — aircraft name committed with real data intact
4. `startESL` moves past `DcsBiosSniffer` (becomes NULL)
5. All remaining 0xFF words dispatched to NULL → **silently dropped**
6. Next frame's real SYNC (0x55 x4) detected by parallel sync counter — no alignment issue because the counter is independent of the data state machine

**File:** `HID Manager/HID_Manager.py` — `PADDING_BYTE = 0xFF` (was implicit 0x00)

### DCS-BIOS mission detection reference

Mission detection is driven by the `_ACFT_NAME` field (MetadataStart module, address 0x0000, 24 bytes):
- **Mission start**: DCS-BIOS publishes the aircraft module name in `_ACFT_NAME`
- **Mission stop**: DCS-BIOS publishes one final message with an **empty string** for `_ACFT_NAME`

Source: [DCS-BIOS developer guide](https://github.com/DCS-Skunkworks/dcs-bios/blob/main/Scripts/DCS-BIOS/doc/developerguide.adoc)

This is NOT a stream timeout — it is an active protocol message. In real DCS, all transports detect mission stop/start correctly because DCS-BIOS sends the actual empty-name frame.

---

## Transport Comparison Matrix

| Behavior | USB | BLE | WiFi | Serial |
|----------|-----|-----|------|--------|
| PC-side relay | HID Manager | HID Manager | Direct multicast | Socat |
| Packet size to ESP32 | 64B (HID report) | ~20-64B (BLE HID) | Up to 1460B (UDP) | Byte stream |
| Ring buffer chunking | Never (64=MAXLEN) | Never (< MAXLEN) | Never (after Bug 5 fix) | N/A (no ring buffer) |
| HID padding | 0xFF (after Bug 6 fix) | 0xFF (after Bug 6 fix) | N/A | N/A |
| Mission cycling (PLAY script) | Correct (after Bug 6 fix) | Correct (after Bug 6 fix) | Correct | Correct |
| Mission cycling (real DCS) | Yes (empty `_ACFT_NAME` frame) | Yes (empty `_ACFT_NAME` frame) | Yes (empty `_ACFT_NAME` frame) | Yes (empty `_ACFT_NAME` frame) |
| IP reassembly | Windows (before HID Manager) | Windows (before HID Manager) | No (lwIP disabled) | Windows (before socat) |

---

## Files Modified

| File | Bug(s) | Change |
|------|--------|--------|
| `lib/DCS-BIOS/src/internal/Protocol.h` | 2 | `unsigned int` -> `uint16_t` for address/count/data |
| `lib/DCS-BIOS/src/internal/Protocol.cpp` | 2 | Updated casts, reverted count=0 guard |
| `Config.h` | 3, 5 | `SERIAL_RX_BUFFER_SIZE` 512->4096, WiFi `PACKET_MAXLEN` 128->1460, `RINGBUF_SIZE` 64->16 |
| `src/Core/DCSBIOSBridge.cpp` | 4 | `reassemblyLen` local -> static |
| `HID Manager/HID_Manager.py` | 6 | `PADDING_BYTE = 0xFF` (was implicit 0x00) |
