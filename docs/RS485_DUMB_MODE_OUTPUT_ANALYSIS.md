# RS485 DUMB Mode Output Failure Analysis

## Executive Summary

After a comprehensive analysis of the RS485 implementations across CockpitOS, the ESP32 RS485 Master reference, and the official Arduino DCS-BIOS library, I have identified **critical architectural differences** that explain why outputs (LEDs, etc.) work perfectly in SMART mode and with the Arduino DCS-BIOS library, but **fail in DUMB mode**.

**Root Cause**: The DUMB mode implementation has a **350-byte frame threshold** before broadcasts are triggered, combined with timing issues that cause output updates to be delayed or lost entirely.

---

## 1. Implementation Comparison

### 1.1 ESP32 RS485 Master (Reference - Works)

**Location**: `Tools/ESP32_RS485_Master/ESP32_RS485_Master.ino`

```cpp
// Lines 262-267
if (exportQueueAvailable() > 0) {    // Simple: ANY data available?
    broadcastExportData();            // Broadcast immediately!
    yield();
    return;
}
```

**Key Characteristics**:
- **No frame threshold**: Broadcasts whenever ANY data is available
- **No sync detection needed**: Just streams whatever is in the buffer
- **Simple ring buffer**: 1024 bytes, broadcasts up to 250 bytes per cycle
- **Priority**: Broadcast BEFORE polling - if data available, broadcast it

**Why it works**: Raw bytes flow continuously to slaves. Slaves parse and dispatch to their listeners. No delay, no filtering.

---

### 1.2 CockpitOS SMART Mode (Works)

**Location**: `lib/CUtils/src/internal/RS485Master.cpp` (lines 207-321)

```cpp
// Lines 284-291 - Sync triggers broadcast
if (rs485_syncByteCount >= 4) {
    rs485_parseState = RS485_PARSE_ADDRESS_LOW;
    rs485_syncByteCount = 0;

    if (rs485_frameHasChanges) {
        rs485_broadcastPending = true;  // Broadcast when changes detected
        rs485_frameHasChanges = false;
    }
}

// Lines 868-869 - Broadcast decision
bool canBroadcast = rs485_broadcastPending && rs485_changeCount > 0;
```

**Key Characteristics**:
- **Parses DCS-BIOS protocol**: Full state machine parser
- **Change detection**: Only broadcasts addresses that changed
- **Address filtering**: Uses `findDcsOutputEntries()` to filter relevant addresses
- **Immediate broadcast**: When sync detected with changes, broadcasts immediately

**Why it works**: Changes trigger immediate broadcasts. No frame size threshold.

---

### 1.3 CockpitOS DUMB Mode (FAILS)

**Location**: `lib/CUtils/src/internal/RS485Master.cpp` (lines 327-410)

```cpp
// Lines 334-345 - Sync detection with threshold
static void rs485_bufferRawByte(uint8_t byte) {
    if (byte == 0x55) {
        rs485_syncDetect++;
        if (rs485_syncDetect >= 4) {
            // BUG: Only set frameReady if >= 350 bytes buffered!
            if (rs485_rawCount >= 350) {
                rs485_frameReady = true;
            }
            rs485_syncDetect = 0;
        }
    }
    // ... buffer the byte ...
}

// Lines 871-872 - Broadcast decision
bool canBroadcast = rs485_broadcastPending && rs485_frameReady && rs485_rawCount > 0;
//                                            ^^^^^^^^^^^^^^^^
//                                            REQUIRES frameReady = true!
```

**The Problem**: THREE conditions must be true:
1. `rs485_broadcastPending` - Set after poll cycle completes
2. `rs485_frameReady` - **Only set if rawCount >= 350 bytes AND sync detected**
3. `rs485_rawCount > 0` - Data in buffer

---

## 2. The Frame Threshold Bug (CRITICAL)

### 2.1 Problem Description

The 350-byte threshold in `rs485_bufferRawByte()` at line 341 creates a **delayed broadcast scenario**:

```
Timeline:
─────────────────────────────────────────────────────────────────────
T0: DCS sends Frame 1 (200 bytes) - Contains LED update for address 0x740C
    → Buffered, rawCount = 200
    → Sync detected, but 200 < 350 → frameReady = FALSE
    → Cannot broadcast!

T1: Poll cycle completes
    → broadcastPending = TRUE
    → But frameReady = FALSE
    → canBroadcast = FALSE (no broadcast!)

T2: DCS sends Frame 2 (200 bytes)
    → Buffered, rawCount = 400
    → Sync detected, 400 >= 350 → frameReady = TRUE
    → NOW canBroadcast = TRUE

T3: Broadcast finally happens
    → Slave receives BOTH frames
    → LED finally updates (DELAYED!)
─────────────────────────────────────────────────────────────────────
```

### 2.2 Impact Analysis

| Scenario | Frame Size | frameReady | Result |
|----------|------------|------------|--------|
| Initial aircraft state | ~370 bytes | TRUE | Works (barely) |
| Single LED update | ~10-50 bytes | FALSE | **NEVER BROADCASTS** |
| Incremental update | ~100-200 bytes | FALSE | Delayed until next frame |
| Rapid changes | Multiple small | FALSE | Accumulates, eventually broadcasts |

### 2.3 Why 350 Bytes?

The comment at line 331 says:
```cpp
// Size: 16KB to handle bursty UDP arrivals and full DCS-BIOS frames
```

A full DCS-BIOS export frame for the F/A-18C is approximately 350-400 bytes. The threshold seems intended to wait for "complete frames," but this is **architecturally wrong** because:

1. DCS-BIOS can send partial frames
2. Incremental updates are small
3. The Arduino reference implementation has NO threshold

---

## 3. Comparison with Arduino DCS-BIOS Library

### 3.1 How Arduino Slave Handles Export Data

**Location**: `Tools/DCS-BIOS/src/internal/DcsBiosNgRS485Slave.cpp.inc` (lines 107-116)

```cpp
case RX_WAIT_DATA:
    rxtx_len--;
    if (rx_datatype == RXDATA_DCSBIOS_EXPORT) {
        if (rxtx_len <= parser.incomingDataBuffer.availableForWrite()) {
            parser.processCharISR(c);  // Feed directly to parser!
        }
    }
    if (rxtx_len == 0) {
        state = RX_WAIT_CHECKSUM;
    }
    break;
```

**Key Point**: The Arduino slave feeds EVERY received byte directly to the DCS-BIOS protocol parser. No buffering thresholds, no frame size requirements.

### 3.2 How Arduino Parser Dispatches to Listeners

**Location**: `Tools/DCS-BIOS/src/internal/Protocol.cpp.inc` (lines 82-100)

```cpp
// Skip listeners not interested in this address
while(startESL && startESL->getLastAddressOfInterest() < address) {
    startESL->onConsistentData();
    startESL = startESL->nextExportStreamListener;
}

// Dispatch to interested listeners
if (startESL) {
    ExportStreamListener* el = startESL;
    while(el) {
        if (el->getFirstAddressOfInterest() > address) break;
        if (el->getFirstAddressOfInterest() <= address &&
            el->getLastAddressOfInterest() >= address) {
            el->onDcsBiosWrite(address, data);  // LED callback here!
        }
        el = el->nextExportStreamListener;
    }
}
```

**Why Arduino Works**:
1. Master broadcasts raw bytes (no threshold)
2. Slave receives and immediately parses
3. Parser dispatches to listeners (LEDs, etc.)
4. Listeners update hardware immediately

---

## 4. CockpitOS Data Flow Analysis

### 4.1 Current DUMB Mode Flow (Broken)

```
DCS-BIOS (UDP)
    │
    ▼
parseDcsBiosUdpPacket() ─────────────► DcsBios::parser.processChar()
    │                                          │
    │                                          ▼
    ▼                                  mySniffer.onDcsBiosWrite()
RS485Master_feedExportData()                   │
    │                                          ▼
    ▼                                  LOCAL LEDs/Outputs work!
rs485_bufferRawByte()
    │
    ▼
rawFifo buffer (waits for 350 bytes)
    │
    ▼ (DELAYED OR NEVER)
Broadcast to slaves
    │
    ▼
Slave receives, parses, updates LEDs
```

**The Fix Is Here**: The local CockpitOS master gets export data parsed immediately. **But slaves only get it after the 350-byte threshold is met.**

### 4.2 Expected DUMB Mode Flow (Fixed)

```
DCS-BIOS (UDP)
    │
    ▼
parseDcsBiosUdpPacket() ─────────────► DcsBios::parser.processChar()
    │                                          │
    ▼                                          ▼
RS485Master_feedExportData()           LOCAL LEDs work!
    │
    ▼
rawFifo buffer (NO threshold!)
    │
    ▼ (IMMEDIATE)
Broadcast to slaves whenever data available
    │
    ▼
Slave receives, parses, updates LEDs
```

---

## 5. Why Inputs Work in DUMB Mode

### 5.1 Input Flow (Working Correctly)

```cpp
// RS485Master.cpp lines 468-469
if (rs485_rxLen > 0 && rs485_rxMsgType == 0) {
    rs485_processInputCommand(rs485_rxBuffer, rs485_rxLen);
    // ...
}

// Line 444
sendCommand(label, value, false);
```

Inputs work because:
1. Slave buffers switch/button state locally
2. When polled, slave sends buffered commands
3. Master receives and forwards to DCS-BIOS immediately
4. **No threshold or frame detection needed** - just process immediately

---

## 6. Hypotheses and Recommendations

### Hypothesis 1: Remove the 350-Byte Threshold (PRIMARY FIX)

**Current Code** (line 341):
```cpp
if (rs485_rawCount >= 350) {
    rs485_frameReady = true;
}
```

**Recommended Fix**:
```cpp
if (rs485_rawCount > 0) {  // Or even remove this check entirely
    rs485_frameReady = true;
}
```

**Rationale**: Match ESP32 Master reference behavior - broadcast whenever data is available.

### Hypothesis 2: Remove frameReady Requirement

**Current Code** (line 872):
```cpp
bool canBroadcast = rs485_broadcastPending && rs485_frameReady && rs485_rawCount > 0;
```

**Recommended Fix**:
```cpp
bool canBroadcast = rs485_broadcastPending && rs485_rawCount > 0;
// Or even simpler:
bool canBroadcast = rs485_rawCount > 0;
```

**Rationale**: The `frameReady` flag serves no purpose if we broadcast incrementally.

### Hypothesis 3: Broadcast-First Priority (Like ESP32 Master)

**ESP32 Master Pattern**:
```cpp
// Broadcast if we have data - BEFORE polling
if (exportQueueAvailable() > 0) {
    broadcastExportData();
    yield();
    return;  // Don't poll this iteration
}
// Only poll if no data to broadcast
pollSlave(currentPollAddress);
```

**Current CockpitOS Pattern**:
```cpp
// Only broadcast at specific times (poll cycle complete + frameReady)
if (canBroadcast) {
    rs485_startBroadcast();
} else {
    rs485_startPoll(rs485_currentPollAddr);  // Poll instead
}
```

**Recommendation**: Prioritize broadcasting over polling when data is available.

### Hypothesis 4: Minimum Broadcast Interval Too High

**RS485Config.h** (line 77):
```cpp
#define RS485_MIN_BROADCAST_INTERVAL_MS 20  // Dumb mode
```

This 20ms minimum may cause output updates to queue up. The ESP32 Master has no such interval.

**Recommendation**: Reduce or remove this interval for DUMB mode.

---

## 7. Test Plan to Verify Hypotheses

### Test 1: Disable Frame Threshold
```cpp
// In rs485_bufferRawByte(), change line 341:
// if (rs485_rawCount >= 350) {
if (rs485_rawCount > 4) {  // Just need sync bytes
```
**Expected**: LEDs should start updating on slaves.

### Test 2: Log Broadcast Activity
Add debug logging:
```cpp
static void rs485_bufferRawByte(uint8_t byte) {
    // After sync detection:
    if (rs485_syncDetect >= 4) {
        debugPrintf("[DUMB] Sync detected, rawCount=%d, frameReady=%d\n",
                    rs485_rawCount, rs485_frameReady);
    }
}
```
**Expected**: See why broadcasts aren't happening.

### Test 3: Compare Broadcast Counts
Monitor `rs485_stats.broadcastCount` over 10 seconds in both modes:
- SMART mode: Should see ~10-100 broadcasts
- DUMB mode: Should see similar, but currently seeing fewer

---

## 8. Summary Table

| Aspect | ESP32 Master | CockpitOS SMART | CockpitOS DUMB |
|--------|--------------|-----------------|----------------|
| Broadcast Trigger | Any data in queue | Changes detected + sync | rawCount >= 350 + sync + poll cycle |
| Frame Threshold | None | None | **350 bytes** |
| Parsing | None (relay only) | Full parser | **None** |
| Priority | Broadcast first | After changes | After poll cycle |
| Min Interval | None | 100ms | 20ms |
| **Output Status** | **WORKS** | **WORKS** | **FAILS** |

---

## 9. Recommended Fix Priority

1. **HIGH**: Remove or drastically reduce the 350-byte threshold in `rs485_bufferRawByte()`
2. **HIGH**: Remove `rs485_frameReady` from broadcast decision
3. **MEDIUM**: Match ESP32 Master's broadcast-first priority
4. **LOW**: Review `RS485_MIN_BROADCAST_INTERVAL_MS` setting

---

## 10. Files to Modify

1. **`lib/CUtils/src/internal/RS485Master.cpp`**
   - Line 341: Change threshold from 350 to ~4 or remove
   - Line 872: Remove `rs485_frameReady` requirement

2. **`lib/CUtils/src/RS485Config.h`**
   - Line 77: Consider reducing `RS485_MIN_BROADCAST_INTERVAL_MS`

---

## 11. Conclusion

The DUMB mode implementation was designed with an incorrect assumption that DCS-BIOS frames are always ~350+ bytes. In reality, incremental updates can be much smaller, and the threshold prevents these updates from ever being broadcast.

The Arduino DCS-BIOS library and ESP32 Master reference both use a simple approach: **broadcast whenever data is available**. This is the correct approach for DUMB mode.

The ironic situation is:
- **SMART mode works** because it triggers broadcasts on detected changes (no size threshold)
- **DUMB mode fails** because it waits for a frame size that may never be reached

The fix is straightforward: remove the frame threshold requirement in DUMB mode to match the behavior of working implementations.

---

*Document generated: 2026-01-30*
*Analysis by: Claude Code Assistant*
