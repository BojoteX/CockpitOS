# RS485 DUMB Mode Output Failure: Low Address Analysis

## The Actual Problem (From Log Evidence)

Every broadcast only contains **LOW addresses near 0x0000**:

```
55 55 55 55 00 00 02 00  → Sync + Address 0x0000
55 55 55 55 02 00 04 00  → Sync + Address 0x0002
55 55 55 55 06 00 04 00  → Sync + Address 0x0006
55 55 55 55 0A 00 04 00  → Sync + Address 0x000A
55 55 55 55 0E 00 04 00  → Sync + Address 0x000E
55 55 55 55 12 00 04 00  → Sync + Address 0x0012
```

These are addresses 0x0000-0x0012 (aircraft name, metadata). **LED addresses like 0x740C are NEVER broadcast.**

Stats from log: `Overflow=1784` - significant data loss.

---

## Why This Happens

### DCS-BIOS Frame Structure

DCS-BIOS exports data sequentially from LOW addresses to HIGH addresses:

```
Frame: [Sync 55555555] [Addr 0x0000 data] [Addr 0x0002 data] ... [Addr 0x740C data] ... [Addr 0x75XX data] [Sync]
       ↑                                                         ↑
       Start of frame                                            LEDs are HERE (end of frame)
```

A complete F/A-18C frame is ~350-400 bytes covering addresses 0x0000 → ~0x7500.

### The Fragmentation Problem

1. **DCS-BIOS sends complete frames** at ~30-50 fps
2. **Master buffers into circular FIFO** (16KB)
3. **Each broadcast is only 80-120 bytes** (from the logs)
4. **Broadcasts always start from new sync** (low addresses)
5. **Before high addresses can be broadcast, new frame arrives**
6. **Cycle repeats - high addresses are NEVER reached**

```
Timeline:
─────────────────────────────────────────────────────────────────────
T0: Frame 1 arrives (400 bytes: 0x0000 → 0x7500)
    Buffer: [Frame1: 0x0000...0x7500]

T1: Broadcast 100 bytes (0x0000 → ~0x0030)
    Buffer: [remaining ~300 bytes of Frame1]

T2: Frame 2 arrives while we're polling slaves
    Buffer: [partial Frame1] + [Frame2: 0x0000...0x7500]
    Buffer OVERFLOWS - oldest data (high addresses of Frame1) LOST!

T3: Next broadcast finds SYNC of Frame2
    Broadcasts 100 bytes starting from 0x0000 AGAIN
    ↑
    HIGH ADDRESSES NEVER SENT!
─────────────────────────────────────────────────────────────────────
```

---

## Listener Behavior Comparison

### Arduino DCS-BIOS Library (Works)

**`Tools/DCS-BIOS/src/internal/DcsBiosNgRS485Slave.cpp.inc` lines 107-116:**

```cpp
case RX_WAIT_DATA:
    rxtx_len--;
    if (rx_datatype == RXDATA_DCSBIOS_EXPORT) {
        parser.processCharISR(c);  // ← BYTE-BY-BYTE, IMMEDIATE processing
    }
    if (rxtx_len == 0) {
        state = RX_WAIT_CHECKSUM;
    }
```

**Key behavior:**
- Each byte is passed to parser **immediately as it arrives**
- Parser processes addresses in order (low to high)
- Each listener (LED, display, etc.) registered for its specific address range
- When address reaches listener's range → `onDcsBiosWrite()` called
- **Listeners get called during byte stream, not after complete frame**

**`Tools/DCS-BIOS/src/internal/Protocol.cpp.inc` lines 85-100:**

```cpp
// Skip listeners whose address range is BELOW current address
while(startESL && startESL->getLastAddressOfInterest() < address) {
    startESL->onConsistentData();
    startESL = startESL->nextExportStreamListener;
}
// Call listeners whose range INCLUDES current address
if (startESL) {
    ExportStreamListener* el = startESL;
    while(el) {
        if (el->getFirstAddressOfInterest() > address) break;
        if (el->getFirstAddressOfInterest() <= address &&
            el->getLastAddressOfInterest() >= address) {
            el->onDcsBiosWrite(address, data);  // ← LED callback here
        }
        el = el->nextExportStreamListener;
    }
}
```

**Listeners are sorted by address range and processed in order as addresses arrive.**

### CockpitOS (What's Different)

**`src/Core/DCSBIOSBridge.cpp` line 200-202:**

```cpp
DcsBiosSniffer():
    DcsBios::ExportStreamListener(0x0000, 0xFFFD),  // ← Registers for ENTIRE range
```

**Local CockpitOS behavior (WORKS):**
- `parseDcsBiosUdpPacket()` calls `DcsBios::parser.processChar()` for each byte
- Parser calls `mySniffer.onDcsBiosWrite()` as addresses are processed
- LEDs update locally on the master ✓

**RS485 DUMB mode broadcast (FAILS):**
- Raw bytes buffered
- Broadcasts fragmented (only beginning of frames)
- Slaves never receive high addresses
- Slave listeners for high addresses never called ✗

---

## Why the Slave's Protocol Parser Never Sees LED Addresses

**CockpitOS RS485 Slave (`lib/CUtils/src/internal/RS485Slave.cpp` line 237):**

```cpp
static void rs485s_processExportData() {
    // Feed to CockpitOS DCS-BIOS parser
    parseDcsBiosUdpPacket(rs485s_rxBuffer, rs485s_rxLen);
}
```

The slave's parser IS the same as the master's. It WOULD work correctly... **IF it received the high addresses.**

But since every broadcast starts from address 0x0000-0x0012, the slave parser:
1. Processes addresses 0x0000, 0x0002, 0x0006...
2. Calls listeners for aircraft name, metadata
3. **NEVER receives addresses like 0x740C (LEDs)**
4. LED listeners never triggered

---

## Buffer Management Issue

**`RS485Master.cpp` lines 353-357:**

```cpp
// Keep buffer bounded (~512 bytes = full frame + margin)
// Discard oldest if full
while (rs485_rawCount >= 512) {
    rs485_rawTail = (rs485_rawTail + 1) % RS485_RAW_FIFO_SIZE;
    rs485_rawCount--;
    rs485_fifoOverflows++;  // ← 1784 overflows logged!
}
```

When buffer hits 512 bytes, **OLDEST data is discarded**.

DCS-BIOS sends low addresses first, high addresses last. When new frames arrive faster than we broadcast:
- Low addresses (new data) preserved
- High addresses (older data) discarded
- Result: Only low addresses ever broadcast

---

## Contrast with ESP32 RS485 Master (Reference - Works)

**`Tools/ESP32_RS485_Master/ESP32_RS485_Master.ino`:**

The ESP32 Master broadcasts **continuously** and **prioritizes broadcasting over polling**:

```cpp
// Broadcast if we have data - BEFORE polling
if (exportQueueAvailable() > 0) {
    broadcastExportData();
    yield();
    return;  // Don't poll this iteration - keep broadcasting
}
```

This ensures:
1. All data gets broadcast before new data can overwrite it
2. Complete frames reach slaves
3. High addresses are transmitted

---

## Summary

| Aspect | Arduino/ESP32 Master | CockpitOS DUMB Mode |
|--------|---------------------|---------------------|
| Broadcast size | Continuous stream | Fragmented ~80-120 bytes |
| Starting address | Sequential (all addresses) | Always 0x0000 (restart) |
| High addresses | ✓ Transmitted | ✗ Lost to overflow |
| Buffer behavior | Drain before overflow | Overflow discards oldest |
| LED updates | ✓ Work | ✗ Never reach slaves |

---

## The Real Question

Why does DUMB mode fragment broadcasts to only ~80-120 bytes, and why does it restart from sync (low addresses) instead of continuing where it left off?

The answer lies in `rs485_prepareExportData()` which:
1. Scans for sync pattern (55 55 55 55)
2. Starts sending from that point
3. Only sends a limited amount before returning to polling

By the time the next broadcast opportunity comes, buffer has new frame data starting from low addresses, so it finds sync again and starts over.

---

*Document generated: 2026-01-30*
*This analysis focuses on the low address issue as identified from log evidence.*
