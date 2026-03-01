# BUG: DCS-BIOS Transport — Oversized Frame Handling

**Status:** Bug 1 open (WiFi IP fragmentation — unfixable in firmware). Bug 2 fixed (uint16_t types). Bug 3 fixed and verified (serial RX buffer overflow).
**Severity:** High — Bug 1 breaks WiFi with oversized streams; Bug 3 broke all serial transports with oversized streams
**Affected transports:** WiFi (Bug 1), all ESP32 (Bug 2), Serial/HWCDC/USBSerial (Bug 3)
**Discovered:** February 2026, across 6 debug sessions

---

## 1. Problem Summary

**Bug 1 — WiFi IP Fragmentation (CONFIRMED — OPEN):** The Apache's DCS-BIOS export stream contains a single UDP frame of **1954 bytes** — larger than the WiFi MTU of 1500 bytes. The ESP32's lwIP stack has **IPv4 reassembly disabled**, so this frame is silently dropped. This kills all module outputs (LEDs, selectors, displays) on WiFi. **Confirmed** by the `--split-frames` PLAY script patch — ARM LED toggles correctly when oversized frames are pre-split. **Cannot be fixed in firmware** — the packet is dropped by lwIP before application code runs.

**Bug 2 — Protocol Parser `unsigned int` Type Mismatch (FIXED):** The DCS-BIOS protocol parser in `Protocol.h` declares `address`, `count`, and `data` as `volatile unsigned int`. The original DCS-BIOS library was designed for AVR Arduino where `unsigned int` is 16-bit. On ESP32, `unsigned int` is **32-bit**, breaking 16-bit overflow/wrap semantics that the protocol depends on. Key impact: `address += 2` at 0xFFFE should wrap to 0x0000 (16-bit) but instead goes to 0x10000 (32-bit), affecting listener flush behavior. **Fixed** by changing types to `volatile uint16_t`.

**Bug 3 — Serial RX Buffer Overflow (FIXED + VERIFIED):** The `SERIAL_RX_BUFFER_SIZE` was 512 bytes — too small for Apache's 1954-byte Frame 1. Socat delivers the full UDP frame as a burst over the COM port, overflowing the buffer and corrupting the protocol parser state. HWCDC and USBSerial both failed with the Apache stream while working fine with shorter streams (F/A-18C). **Fixed** by increasing buffer to 4096 bytes. **Verified working** on both HWCDC and USBSerial with the Apache stream.

---

## 2. Current Transport Status

Status as of Session 6, tested with Apache AH-64D stream (contains 1954-byte oversized frame):

| Transport | Mission Start/Stop | ARM LED ON/OFF | Module Outputs | Status |
|-----------|-------------------|----------------|----------------|--------|
| **USB** (HID Manager) | OK | OK | OK | Working |
| **BLE** (HID Manager) | OK | OK | OK | Working |
| **USBSerial** (Socat) | OK | OK | OK | **Fixed** (Bug 3 — RX buffer 512→4096) |
| **HWCDC** (Socat) | OK | OK | OK | **Fixed** (Bug 3 — RX buffer 512→4096) |
| **WiFi** (direct UDP) | FAIL | FAIL | FAIL | **Broken** (Bug 1 — unfixable in firmware) |
| **WiFi + --split-frames** | OK | OK | OK | Workaround (sender-side frame splitting) |

**Key insight:** USB and BLE work because HID Manager on the PC side receives the full UDP frame (Windows handles IP reassembly) and slices it into 64-byte chunks with protocol-level flow control. Serial transports (USBSerial, HWCDC) now work after increasing the RX buffer to hold the largest DCS-BIOS frame. WiFi remains broken because the ESP32's lwIP stack silently drops oversized UDP packets before application code can see them.

---

## 3. Architecture Context

### 3.1 DCS-BIOS Wire Protocol

DCS-BIOS exports cockpit state over UDP multicast (239.255.50.10:5010). The protocol is a byte stream parsed by a 7-state state machine (`Protocol.cpp`):

- **SYNC marker:** 4 consecutive 0x55 bytes
- **Address/count/data triplets:** 2-byte address, 2-byte count, N data bytes
- **End marker:** Address 0xFFFE, count 0x0000 signals end of consistent frame
- The parser calls `ExportStreamListener::onDcsBiosWrite()` for each address/value pair and `onConsistentData()` when listeners are flushed (via address progression or SYNC boundary)

All protocol values are **16-bit**. The parser was written for AVR where `unsigned int` naturally matches.

### 3.2 Transport Paths

| Transport | PC-side receiver | PC has IP reassembly? | Transfer to ESP32 | ESP32 entry point | Ring buffer? |
|-----------|------------------|-----------------------|--------------------|-------------------|-------------|
| **USB** | HID Manager (Python) | Yes — Windows reassembles | 64-byte HID Output Reports (zero-padded) | `_onOutput()` in HIDDescriptors.h | Yes, 1 slot per report |
| **WiFi** | ESP32 directly joins multicast | **No** — lwIP reassembly disabled | N/A (direct receive) | `AsyncUDP.onPacket()` in WiFiDebug.cpp | Yes, needs 16 slots atomically |
| **Serial/Socat** | Socat captures UDP on PC | Yes — Windows reassembles | Raw bytes over UART at 250kbaud | `Serial.read()` byte-by-byte | No — feeds parser directly |

### 3.3 Ring Buffer Architecture (RingBuffer.cpp)

WiFi and USB both use a SPSC (single-producer, single-consumer) ring buffer to decouple the receive callback from the main `loop()`.

**Buffer configuration (Config.h):**

| Setting | USB value | WiFi value |
|---------|-----------|------------|
| `DCS_UDP_RINGBUF_SIZE` | 32 | 64 |
| `DCS_UDP_PACKET_MAXLEN` | 64 (= HID report size) | 128 |
| `DCS_UDP_MAX_REASSEMBLED` | 4096 | 4096 |
| `MAX_UDP_FRAMES_PER_DRAIN` | 1 | 1 |

### 3.4 Mission Start/Stop Detection Chain

1. `onDcsBiosWrite()` — if address is in aircraft name range (0x0000-0x0017), writes bytes to `aircraftNameBuf`, sets `aircraftNameDirty = true`
2. `onConsistentData()` — fires on frame boundary, calls `commitAnonymousStringField()`
3. `commitAnonymousStringField()` — if dirty, compares buffer to `lastAircraftName`. If changed, calls `onAircraftName(buffer)`
4. `onAircraftName()` — blank name = MISSION STOP, matching name = MISSION START

---

## 4. Stream Analysis

### 4.1 Apache AH-64D Stream (dcsbios_data.json.AH-64)

- 1258 frames per cycle (~42 seconds at 30 FPS)
- Frame 0: **138 bytes** — SYNC + addr=0x0000 (aircraft name "AH-64D_BLK_II") + addr=0x0400 (metadata)
- Frame 1: **1954 bytes** — SYNC + addr=0x8000 (ALL module outputs including CMWS addresses 0x8712-0x875F) + addr=0xFFFE (end marker)
- Only 1 frame out of 1258 exceeds 1500 bytes
- First 10 frame sizes: [138, 1954, 184, 304, 128, 116, 120, 116, 106, 116]

### 4.2 SHORT F/A-18C Stream (dcsbios_data.json.SHORT)

- 1872 frames per cycle
- Maximum frame size: **572 bytes** — zero frames exceed 1400 bytes
- Works on ALL transports

---

## 5. Bug 1: WiFi IP Fragmentation (CONFIRMED — OPEN)

### 5.1 Root Cause

1. PLAY script sends 1954-byte UDP datagram via `sock.sendto()`
2. The OS fragments this into 2 IP packets (1500 + ~482 bytes including headers)
3. ESP32 receives the fragments on its WiFi radio
4. lwIP's `ip4_input()` checks `#if IP_REASSEMBLY` — this is compiled out (disabled)
5. Fragment is freed via `pbuf_free(p)` — silently dropped
6. The complete 1954-byte packet **never reaches** `AsyncUDP.onPacket()`

**Evidence from ESP32 sdkconfig** (`AppData\Local\Arduino15\packages\esp32\tools\esp32-libs\3.3.6\sdkconfig`):
```
# CONFIG_LWIP_IP4_REASSEMBLY is not set
CONFIG_LWIP_IP_REASS_MAX_PBUFS=10
```

The reassembly code exists in lwIP source but is behind `#if IP_REASSEMBLY` guards. ESP32 Arduino v3.3.6 uses **pre-compiled `.a` library files** — the reassembly function `ip4_reass()` was compiled out and cannot be re-enabled without rebuilding the entire ESP32 SDK from source.

### 5.2 Secondary Issue — AsyncUDP pbuf Chain Splitting

Even if IP reassembly were enabled, ESP32's AsyncUDP has another problem. In `AsyncUDP.cpp` (`_udp_recv()`), reassembled chained pbufs are split into separate handler callbacks:

```cpp
// The library breaks the pbuf chain and fires separate callbacks:
this_pb->next = NULL;  // Severs the chain
// AsyncUDPPacket uses pb->len (single pbuf length), not pb->tot_len
```

A reassembled 1954-byte packet would arrive as **two separate** `onPacket()` callbacks. However, since the DCS-BIOS parser is byte-oriented, this would actually work if the bytes arrive in order.

### 5.3 Test Result — CONFIRMED

With `--split-frames` enabled on the PLAY script (interactive mode, chunk size 1400B):

```
[09:27:19] [DCS-BIOS] STREAM UP
[09:27:19] [MISSION START] AH-64D
[09:27:19] [PANEL SYNC] Started...
[09:27:19] PLT_CMWS_ARM 0
[09:27:19] [PANEL SYNC] Finished...
[09:27:29] [STATE UPDATE] PLT_CMWS_ARM = SAFE
[09:27:32] [STATE UPDATE] PLT_CMWS_ARM = ARM      <-- LED ON
[09:28:11] [STATE UPDATE] PLT_CMWS_ARM = SAFE     <-- LED OFF
[09:28:14] [STATE UPDATE] PLT_CMWS_ARM = ARM      <-- LED ON
```

**ALL module outputs flow correctly on WiFi when frames are split below MTU.**

---

## 6. Bug 2: Protocol Parser `unsigned int` Type Mismatch (FIXED)

### 6.1 Root Cause

The DCS-BIOS protocol uses 16-bit values for addresses, counts, and data. The original Arduino library declares these as `unsigned int`, which is **16-bit on AVR** — matching the protocol's design:

```cpp
// Original DCS-BIOS library (Protocol.h) — designed for AVR
volatile unsigned int address;  // 16-bit on AVR
volatile unsigned int count;    // 16-bit on AVR
volatile unsigned int data;     // 16-bit on AVR
```

CockpitOS inherited this declaration verbatim. On ESP32, `unsigned int` is **32-bit**, silently changing the parser's overflow semantics.

**The Arduino RS485 Master** (`ARDUINO_RS485_Master.ino`) correctly uses `uint16_t`:
```cpp
// Arduino Master (line 120-122)
uint16_t ledParserAddress;
uint16_t ledParserCount;
uint16_t ledParserData;
```

### 6.2 Key Behavioral Differences

| Operation | AVR (uint16) | ESP32 (uint32) | Impact |
|-----------|-------------|----------------|--------|
| `address += 2` at 0xFFFE | Wraps to 0x0000 | Goes to 0x10000 | Different listener flush behavior at end-of-frame |
| `count - 1` at 0x0000 | Wraps to 0xFFFF | Goes to 0xFFFFFFFF | Parser stays in underflow state much longer |
| Address comparisons | 16-bit range | 32-bit range | Listener `lastAddressOfInterest < address` behaves differently |

The DCS-BIOS end-of-frame marker is `address=0xFFFE, count=0x0000`. After processing this block, `address += 2` is expected to wrap to 0x0000 on AVR. On ESP32 with 32-bit types, the address escapes the 16-bit protocol space entirely.

### 6.3 The Fix

**Protocol.h** — Changed types to explicit 16-bit (added `#include <stdint.h>`):
```cpp
volatile uint16_t address;
volatile uint16_t count;
volatile uint16_t data;
```

**Protocol.cpp** — Updated casts from `(unsigned int)` to `(uint16_t)`:
```cpp
address = (uint16_t)c;   // was (unsigned int)c
count = (uint16_t)c;     // was (unsigned int)c
data = (uint16_t)c;      // was (unsigned int)c
```

This makes the ESP32 parser behave identically to the AVR parser — matching the protocol's designed 16-bit semantics.

### 6.4 Reverted: count=0 Guard (was incorrectly added in Session 4)

A previous debug session incorrectly added a `count == 0` guard in COUNT_HIGH that skipped to ADDRESS_LOW when count was zero:

```cpp
// WRONG — was added in Session 4, now REVERTED:
if (count == 0)
    state = DCSBIOS_STATE_ADDRESS_LOW;  // skip empty data block
else
    state = DCSBIOS_STATE_DATA_LOW;
```

This was wrong because:
- The DCS-BIOS end marker (address=0xFFFE, count=0x0000) legitimately sends count=0
- The original parser handles this by entering DATA_LOW where the underflowed count is interrupted by SYNC detection within 4 bytes
- The guard blocked this mechanism, preventing the normal end-of-frame flush
- USB mission cycling was CORRECT behavior (matching what the Arduino does), not an artifact of a parser bug

The original unconditional `state = DCSBIOS_STATE_DATA_LOW` has been restored.

---

## 7. CockpitOS Additions to Original Parser

CockpitOS's Protocol.cpp has one intentional addition not present in the original DCS-BIOS library:

**SYNC-boundary flush (`flushRemainingListeners`):**
```cpp
if (sync_byte_count == 4) {
    // CockpitOS addition: ensure all listeners are flushed at frame boundary
    if (processingData) {
        flushRemainingListeners(startESL);
        processingData = false;
    }
    state = DCSBIOS_STATE_ADDRESS_LOW;
    sync_byte_count = 0;
    startESL = ExportStreamListener::firstExportStreamListener;
}
```

This ensures `onConsistentData()` fires for all listeners at every frame boundary, even if address progression didn't reach their range. The original library relies solely on address progression for flushing. This addition is **correct and intentional** — it makes end-of-frame commits deterministic.

---

## 8. Bug 3: Serial Transport — RX Buffer Overflow (FIXED + VERIFIED)

### 8.1 Root Cause

The `SERIAL_RX_BUFFER_SIZE` was 512 bytes. When socat bridges a large DCS-BIOS UDP frame (e.g., Apache Frame 1 at 1954 bytes) to the COM port, the USB CDC driver delivers data faster than the main loop can drain the serial buffer. Once the buffer overflows, bytes are dropped, corrupting the protocol parser's state machine.

**Evidence:**
- F/A-18C stream (max 572-byte frames) **works on all serial transports**
- Apache stream (1954-byte Frame 1) **fails on HWCDC and USBSerial**
- USB/BLE unaffected because HID Manager slices frames into 64-byte reports with built-in USB/BLE flow control

### 8.2 Transport-Specific Behavior

| Serial Transport | Buffer Overflow Behavior | Observed Symptoms |
|------------------|-------------------------|-------------------|
| **HWCDC** (JTAG USB) | Bytes DROPPED — HWCDC may not implement robust NAK-based flow control | Nothing works (no ARM LED, no mission detection) |
| **USBSerial** (TinyUSB OTG) | TinyUSB NAKs host when buffer full — data delayed, not lost, but backpressure disrupts timing | ARM LED works (small delta frames), mission stop/start broken (SYNC timing disrupted) |
| **Plain Serial** (Original ESP32 UART at 250kbaud) | Physical UART overflow — bytes dropped at hardware level | Depends on data rate vs baud rate |

### 8.3 Why USB/BLE Are Immune

HID Manager on the PC side receives full UDP frames (Windows handles IP reassembly) and slices them into 64-byte HID reports or BLE writes. Each 64-byte chunk:
- Fits in a single ring buffer slot with `isLastChunk = true`
- Has built-in protocol-level flow control (USB HID: host waits for ACK; BLE: connection interval pacing)
- Is processed independently — no multi-chunk reassembly needed

### 8.4 The Fix

Increased `SERIAL_RX_BUFFER_SIZE` from 512 to 4096 bytes in `Config.h` (line 87). This gives the serial RX buffer enough room to hold the largest DCS-BIOS frame (Apache: 1954 bytes) with headroom. Cost: ~3.5KB additional RAM (negligible on ESP32).

### 8.5 Verification

Tested on hardware with Apache AH-64D stream via Socat:
- **HWCDC:** Mission start/stop, ARM LED ON/OFF, module outputs — all working
- **USBSerial:** Mission start/stop, ARM LED ON/OFF, module outputs — all working
- **F/A-18C SHORT stream:** Already worked before the fix (max frame 572B < old buffer 512B... just barely, but no single burst exceeded the buffer drain rate)

---

## 9. Proposed Fixes for Bug 1 (WiFi IP Fragmentation)

The PLAY script `--split-frames` is a diagnostic workaround that **confirmed** the root cause. The firmware **cannot** fix IP fragmentation — the packet is silently dropped by lwIP before `AsyncUDP.onPacket()` fires.

**Option A — Sender-Side Frame Splitting (Recommended):**
HID Manager, socat, or the PLAY script splits oversized frames before transmission. The DCS-BIOS parser is byte-oriented, so split sub-frames are transparent. PLAY script `--split-frames` already implements this with protocol-aware splitting.

**Option B — DCS-BIOS Source Fix:**
DCS-BIOS Lua export scripts could be modified to split large export frames into sub-MTU UDP datagrams. Upstream fix, benefits all consumers.

**Option C — Enable lwIP IP Reassembly:**
Requires building ESP32 SDK from source with `CONFIG_LWIP_IP4_REASSEMBLY=y`. Infeasible with Arduino pre-compiled toolchain (attempted Session 3, abandoned).

---

## 10. Attempted Fixes History

| Session | Theory | Change | Result | Status |
|---------|--------|--------|--------|--------|
| 1 | g_prevValues overflow | Moved update after queue check | No improvement | Reverted |
| 1 | One-chunk-per-loop | Modified drain to pop one entry | No improvement | Reverted |
| 3 | Enable IP reassembly | Build ESP32 SDK from source | Infeasible with Arduino toolchain | Abandoned |
| 4 | PLAY --split-frames | Split oversized UDP before send | **Bug 1 confirmed** — ARM LED works | Active (diagnostic) |
| 4 | Protocol.cpp count=0 guard | Skip DATA_LOW when count=0 | **WRONG** — blocked normal end-of-frame | **Reverted** |
| 5 | Protocol.h uint16_t types | Changed unsigned int to uint16_t | Matches AVR behavior | **Applied** |
| 6 | Serial RX buffer overflow | Increased SERIAL_RX_BUFFER_SIZE 512→4096 | **Bug 3 fixed + verified** — HWCDC and USBSerial both work with Apache stream | **Applied** |

---

## 11. Key Files

| File | What to look at |
|------|----------------|
| `lib/DCS-BIOS/src/internal/Protocol.h` | **BUG 2 FIX**: `uint16_t` types for address/count/data |
| `lib/DCS-BIOS/src/internal/Protocol.cpp` | Parser state machine, reverted count=0 guard |
| `lib/DCS-BIOS/src/internal/ExportStreamListener.h` | Listener chain, address masking |
| `src/Core/DCSBIOSBridge.cpp` | DcsBiosSniffer, mission detection, ring buffer drain |
| `src/Core/RingBuffer.cpp` | `dcsUdpRingbufPushChunked()` — atomic capacity check |
| `src/Core/WiFiDebug.cpp` | WiFi receive callback |
| `src/HIDDescriptors.h` | USB HID receive callback |
| `Config.h` | Buffer size defines |
| `Debug Tools/PLAY_DCS_stream.py` | Stream replay with `--split-frames` |
| `Tools/ARDUINO_RS485_Master/ARDUINO_RS485_Master.ino` | Reference — uses `uint16_t` types |
| `Dev/DCS-BIOS-LIBRARY/src/internal/Protocol.cpp.inc` | Original DCS-BIOS parser (AVR) |
