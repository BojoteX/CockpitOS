# CockpitOS Critical-Path Code Audit Report

**Date:** 2026-02-27
**Scope:** End-to-end firmware audit — input hot path, output hot path, RS485 master/slave, transport layers, panel init, label set compilation
**Focus:** Bugs, race conditions, memory corruption, CPU/memory inefficiencies affecting real-time performance

---

## CRITICAL Severity (Crash / Data Corruption / Watchdog Reset)

### C1. RS485 Slave: `txLen` overflow and `txFrameBuf` buffer overrun

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Slave.cpp:306` |
| **Impact** | Memory corruption, hung slave TX, silent data loss |

`txLen` is declared as `uint8_t`. When the payload reaches 253 bytes, the total frame is 1 (length) + 1 (msgtype) + 253 (data) + 1 (checksum) = 256 bytes. `txLen++` wraps to 0, causing `uart_ll_write_txfifo()` to be called with `txLen = 0` (no data transmitted). The state machine enters `TX_WAITING_DONE` but the UART never fires TX_DONE, hanging the slave permanently.

Additionally, `txFrameBuf` is only `RS485_TX_BUFFER_SIZE + 4 = 132` bytes. Payloads > 128 bytes write past the buffer end, corrupting adjacent static memory.

**Fix:** Change `txLen` to `uint16_t`. Cap `toSend` at `min(253, RS485_TX_BUFFER_SIZE - 3)` to respect actual buffer capacity. Add `static_assert` to enforce the invariant at compile time.

---

### C2. RS485 Master: `txBytes()` spin-wait with interrupts disabled

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Master.cpp:506-515, 618-619` |
| **Impact** | CPU stall with RX interrupts disabled, lost UART bytes, potential watchdog on large frames |

`txWithEchoPrevention()` disables RX interrupts, then calls `txBytes()` for frames > 128 bytes (UART FIFO size). `txBytes()` spin-waits on `uart_ll_get_txfifo_len() == 0`. At 250kbps, each byte takes 40us. For a 260-byte frame, the spin-wait after FIFO fill is ~5.3ms with interrupts disabled. On single-core chips (S2/C3/C6/H2), the entire CPU is blocked.

**Fix:** Add compile-time guard to ensure broadcast frames always fit in the 128-byte FIFO:
```cpp
static_assert(RS485_RELAY_CHUNK_SIZE + 4 <= 128, "Relay chunk must fit in TX FIFO");
```

---

### C3. Ring buffer race: WiFi AsyncUDP callback vs main loop without memory barriers

| Field | Value |
|-------|-------|
| **File** | `src/Core/RingBuffer.cpp:76-131` |
| **Impact** | Data corruption on RISC-V (C3/C6/H2), torn ring buffer reads |

`dcsUdpRingbufPush()` is called from the AsyncUDP callback (LwIP task, potentially different core), while `dcsUdpRingbufPop()` runs in `loop()`. Head/tail are `volatile uint8_t` but have no memory barriers. On ESP32-C3/C6 RISC-V, store-load reordering can cause the consumer to see updated head before the data is visible, reading garbage.

**Fix:** Use `__atomic_store_n(&head, newVal, __ATOMIC_RELEASE)` on producer side and `__atomic_load_n(&head, __ATOMIC_ACQUIRE)` on consumer side. Or use FreeRTOS `xQueueSend`/`xQueueReceive`.

---

### C4. `onDcsBiosUdpPacket()` reassembly: `reassemblyLen` resets across calls, corrupting chunked frames

| Field | Value |
|-------|-------|
| **File** | `src/Core/DCSBIOSBridge.cpp:784-808` |
| **Impact** | Corrupted DCS-BIOS frames when UDP packets are split across ring buffer drain calls |

`reassemblyLen` is a local variable, reset to 0 each call. When a chunked UDP packet (WiFi path with `DCS_UDP_PACKET_MAXLEN=128`) spans multiple calls to `onDcsBiosUdpPacket()`, the continuation chunks start at offset 0 instead of the correct offset, producing a corrupted reassembled frame fed to the protocol parser.

**Fix:** Make `reassemblyLen` static:
```cpp
static size_t reassemblyLen = 0;
```

---

## HIGH Severity (Performance Degradation on Hot Path)

### H1. RS485 Master: Change queue race between main loop and FreeRTOS task

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Master.cpp:201-248, 303-317` |
| **Impact** | Data corruption, `changeCount` underflow, `broadcastBuffer` overflow |

When `RS485_USE_TASK=1`, `queueChange()` (main loop) and `prepareBroadcastData()` (RS485 task) both access `changeQueue[]`, `changeHead`/`changeTail`/`changeCount` without synchronization. Non-atomic read-modify-write on `changeCount` from both contexts can underflow on RISC-V or with preemption.

**Fix:** Protect with `portENTER_CRITICAL`/`portEXIT_CRITICAL` using a shared spinlock, or use a FreeRTOS stream buffer.

---

### H2. RS485 Master: `MessageBuffer::clear()` non-atomic vs ISR writes

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Master.cpp:163-167, 802, 1051` |
| **Impact** | ISR writes during clear cause buffer state corruption |

`clear()` sets `readPos`, `writePos`, `complete` in three separate stores from loop context. The UART RX ISR can fire between these writes, calling `put()` which uses `getLength()` (depends on both pointers).

**Fix:** Wrap `clear()` in `portDISABLE_INTERRUPTS()`/`portENABLE_INTERRUPTS()`.

---

### H3. RS485 Master: `checkRxTimeout()` modifies ISR-owned state without critical section

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Master.cpp:807-843` |
| **Impact** | State machine corruption, ISR and loop writing `state` concurrently |

On timeout, the loop writes to `messageBuffer`, `state`, and `messageCompleteTime` — all ISR-modified. A late-arriving byte processed by the ISR during the timeout handler corrupts both buffer and state machine.

**Fix:** Disable UART RX interrupt before modifying shared state in timeout handler.

---

### H4. Subscriber dispatch: O(n) strcmp on every DCS-BIOS output update

| Field | Value |
|-------|-------|
| **File** | `src/Core/DCSBIOSBridge.cpp:604-608, 643-647, 661-665, 679-684` |
| **Impact** | ~12,800 strcmp calls per DCS-BIOS frame, measurable CPU waste |

Each `onLedChange`/`onSelectorChange`/`onMetaDataChange`/`onDisplayChange` iterates all subscribers (up to 32 each) with `strcmp()` to match labels. At 30Hz with ~100 outputs per frame, this is significant overhead.

**Fix:** Build a hash table for subscribers at registration time (same pattern as existing `findCmdEntry()`).

---

### H5. `isLatchedButton()`: O(n) linear scan with strcmp on hot path

| Field | Value |
|-------|-------|
| **File** | `Mappings.cpp:380-384` |
| **Impact** | Unnecessary strcmp overhead on every input event |

Called from `HIDManager_setNamedButton()` on every button press/release. Linear scan with `strcmp()` per entry.

**Fix:** Build a hash set at init time, or add a `bool isLatched` flag to InputMapping pre-resolved at build.

---

### H6. Selector hash table generator emits duplicate keys — unreachable SelectorMap entries

| Field | Value |
|-------|-------|
| **File** | `src/LABELS/*/DCSBIOSBridgeData.h` (generator bug in `src/LABELS/_core/generator_core.py`) |
| **Impact** | Incorrect selector position dispatch — first match wins, duplicates silently shadowed |

Multiple label sets have duplicate `(dcsCommand, value)` entries in their `selectorHashTable[]`. The hash lookup returns the first match, making later entries unreachable. Confirmed in 4 label sets:

- `LABEL_SET_FRONT_LEFT_PANEL`: `RWR_DIS_TYPE_SW`, `ECM_MODE_SW` duplicated
- `LABEL_SET_KY58`: `KY58_FILL_SELECT`, `KY58_MODE_SELECT` duplicated
- `LABEL_SET_RS485_WAVESHARE_MANUAL`: `BLEED_AIR_KNOB`, `INS_SW`, `RADAR_SW` duplicated
- `LABEL_SET_TEST_ONLY`: `ENGINE_CRANK_SW` duplicated

**Fix:** The generator must deduplicate entries by `(dcsCommand, value)` composite key before emitting the hash table. Affected file: `src/LABELS/_core/generator_core.py` selector hash table generation.

---

## MEDIUM Severity (Edge Case Bugs)

### M1. RS485 Master: `parseCount` underflow on odd-count DCS-BIOS frames

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Master.cpp:283-293` |

`parseCount` is `uint16_t`, decremented in both data states. An odd count value (from corruption) causes underflow from 0 to 65535 in `PARSE_DATA_HIGH`, locking the parser until the next sync pattern.

**Fix:** Check `parseCount > 0` before decrementing in `PARSE_DATA_HIGH`.

---

### M2. RS485 Slave: Missing RISC-V memory fence on export buffer consumer

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Slave.cpp:253-261` |

`processExportData()` reads ISR-written `exportWritePos`. On RISC-V, `volatile` only prevents compiler reordering. CPU can serve stale values.

**Fix:** Add `__asm__ __volatile__("fence" ::: "memory")` before loop on RISC-V targets.

---

### M3. RS485 Slave: Export buffer overflow ISR reset corrupts consumer

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Slave.cpp:486-490` |

ISR resets both `exportReadPos` and `exportWritePos` to 0 on overflow. Consumer may have cached old positions, reading garbage after reset.

**Fix:** Advance `exportReadPos` to equal `exportWritePos` instead of resetting both.

---

### M4. `static char fallback[16]` in `onSelectorChange` — shared across calls

| Field | Value |
|-------|-------|
| **File** | `src/Core/DCSBIOSBridge.cpp:636` |

Static buffer used for fallback state strings. Any subscriber storing the `stateStr` pointer gets corrupted data on next call.

**Fix:** Remove `static` (16 bytes is fine on stack).

---

### M5. `static char cbuf[10]` reentrancy hazard in HIDManager

| Field | Value |
|-------|-------|
| **File** | `src/Core/HIDManager.cpp:878, 946` |

Two `static char cbuf[10]` buffers used for `snprintf` before `sendCommand()`. The CoverGate reentry guard protects some paths, but not all. Nested calls would corrupt the buffer.

**Fix:** Remove `static` — 10 bytes is safe on stack.

---

### M6. RS485 Master: 260-byte stack allocation in 4096-byte FreeRTOS task

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Master.cpp:680` |

`sendBroadcast()` allocates `uint8_t frame[260]` on stack in the RS485 task (stack = 4096 bytes). Reduces ISR stack safety margin.

**Fix:** Make `frame` a file-static buffer.

---

### M7. `DEBUG_USE_WIFI` without ring buffer bypasses SPSC safety

| Field | Value |
|-------|-------|
| **File** | `src/Core/WiFiDebug.cpp:201`, `Config.h:202-203` |

The `#error` guard only covers `USE_DCSBIOS_WIFI`, not `DEBUG_USE_WIFI`. If debug WiFi is enabled without `DCS_USE_RINGBUFFER`, the async callback directly calls `parseDcsBiosUdpPacket()` from the wrong context.

**Fix:** Add `#error` for `DEBUG_USE_WIFI && !DCS_USE_RINGBUFFER` in Config.h.

---

### M8. Protocol parser: no length validation on `count` field

| Field | Value |
|-------|-------|
| **File** | `lib/DCS-BIOS/src/internal/Protocol.cpp:39-46` |

A corrupted `count` value (up to 65535) keeps the parser in data mode for 131070 bytes, consuming potential sync patterns as data and causing missed frames.

**Fix:** Add maximum count guard: `if (count > 2048) { state = DCSBIOS_STATE_WAIT_FOR_SYNC; break; }`

---

### M9. `pollPCA9555_flat` selector group loop: O(groups x inputs) per chip per poll

| Field | Value |
|-------|-------|
| **File** | `src/Core/InputControl.cpp:594-613` |

The selector group loop iterates `MAX_SELECTOR_GROUPS` (128) groups, and for each group, scans all `numPca9555Inputs` (up to 64). This runs per PCA chip per poll cycle (250 Hz). With 8 PCA chips, worst case is `8 * 128 * 64 = 65,536` iterations per poll. The GPIO selector path already solved this with pre-resolved flat tables and `gpioSelGroupUsed[]` for O(1) skip.

**Fix:** Pre-resolve PCA selectors at init time with per-chip, per-group start/count indices. At minimum, add a `bool pcaSelGroupUsed[MAX_SELECTOR_GROUPS]` to skip empty groups in O(1).

---

### M10. Serial transport reads bytes one at a time through `parseDcsBiosUdpPacket`

| Field | Value |
|-------|-------|
| **File** | `src/Core/DCSBIOSBridge.cpp:1816-1853` |

The serial read path calls `parseDcsBiosUdpPacket(&b, 1)` for each byte. This means function call overhead (including the `for` loop setup) is paid 1472+ times per DCS-BIOS frame instead of once. On RS485 master builds, it also results in 1472+ calls to `RS485Master_feedExportData` with `len=1`.

**Fix:** Batch-read from Serial into a local buffer:
```cpp
uint8_t buf[256];
while (Serial.available() > 0) {
    int n = Serial.readBytes(buf, min((int)Serial.available(), (int)sizeof(buf)));
    if (n > 0) { parseDcsBiosUdpPacket(buf, n); got = true; }
}
```

---

### M11. CT_DISPLAY path does two redundant hash lookups per word

| Field | Value |
|-------|-------|
| **File** | `src/Core/DCSBIOSBridge.cpp:297-312` |

Every display write calls both `findDisplayBufferByLabel()` and `findDisplayFieldByLabel()` for the same label — two separate hash lookups with identical key. For display-heavy panels (IFEI with 40+ display fields), this doubles lookup overhead on the hottest path.

**Fix:** Consolidate into a single lookup by adding `base_addr` and `length` fields to `DisplayBufferEntry`.

---

### M12. Display buffer dirty-flag not used in `onConsistentData` flush

| Field | Value |
|-------|-------|
| **File** | `src/Core/DCSBIOSBridge.cpp:355-363` |

Every frame (30Hz), all registered display buffers are checked via `strncmp` even when nothing changed. The `RegisteredDisplayBuffer` struct has a `dirtyFlag` field but it's not checked here. Only the `AnonymousStringBuffer` path uses dirty flags.

**Fix:** Set `*b.dirtyFlag = true` in the CT_DISPLAY write path when bytes are modified, then gate the `strncmp` on the dirty flag.

---

### M13. Dropped `pendingUpdates` permanently lost — `g_prevValues` already updated

| Field | Value |
|-------|-------|
| **File** | `src/Core/DCSBIOSBridge.cpp:274, 281-285` |

When `pendingUpdates[]` overflows (> `MAX_PENDING_UPDATES = 220`), dropped updates are never retried because `g_prevValues[index]` was already updated, so the change-detection filter suppresses the same value on the next frame.

**Fix:** Only update `g_prevValues[index]` after successfully queuing the update:
```cpp
if (pendingUpdateCount < MAX_PENDING_UPDATES) {
    g_prevValues[index] = val;
    pendingUpdates[pendingUpdateCount++] = {entry->label, val, entry->max_value};
} else {
    pendingUpdateOverflow++; // Don't update g_prevValues — allow retry
}
```

---

### M14. Null pointer dereference in `buildGPIOEncoderStates` on null `source`/`controlType`

| Field | Value |
|-------|-------|
| **File** | `src/Core/InputControl.cpp:256-257` |

`strcmp(mi.source, "GPIO")` called without null check on `mi.source`. Similarly for `mi.controlType` on line 257. A hand-edited label set with null fields would crash. Other build functions already have null guards.

**Fix:** Add null guards: `if (!mi.label || !mi.source || strcmp(mi.source, "GPIO") != 0) continue;`

---

### M15. TFT_Gauges_CabinPressure: unreachable `#elif` warning branch

| Field | Value |
|-------|-------|
| **File** | `src/Panels/TFT_Gauges_CabinPressure.cpp:8, 481-483` |

Line 8's outer guard requires `HAS_ALR67 && (ESP_FAMILY_S3 || ESP_FAMILY_S2) && ENABLE_TFT_GAUGES`. Line 481's `#elif` tests `HAS_ALR67 && ENABLE_TFT_GAUGES` without the ESP family condition. Since the `#elif` is part of the same `#if` block, it can only be reached when the outer guard is false — but its condition is a subset of the outer guard (missing the ESP family check), so it's never true when the outer guard is false. The `#warning "Cabin Pressure Gauge requires ESP32-S2 or ESP32-S3"` on line 482 is dead code. Other TFT gauge files (Battery, RadarAlt) have the same pattern but use the full guard in both branches.

**Fix:** Change line 481 to match the pattern in other TFT gauge files — use the ESP-family-independent define in the `#elif`:
```cpp
#elif defined(HAS_ALR67) || defined(HAS_CABIN_PRESSURE_GAUGE)
#warning "Cabin Pressure Gauge requires ESP32-S2 or ESP32-S3"
#endif
```

---

## Summary Table

| # | Severity | Component | Issue |
|---|----------|-----------|-------|
| C1 | CRITICAL | RS485 Slave | `txLen` uint8_t overflow + txFrameBuf overrun |
| C2 | CRITICAL | RS485 Master | Spin-wait with interrupts disabled |
| C3 | CRITICAL | Ring Buffer | No memory barriers on RISC-V (WiFi SPSC) |
| C4 | CRITICAL | DCSBIOSBridge | Chunked frame reassembly drops partial data across calls |
| H1 | HIGH | RS485 Master | Change queue race (main loop vs task) |
| H2 | HIGH | RS485 Master | `MessageBuffer::clear()` not ISR-safe |
| H3 | HIGH | RS485 Master | Timeout handler vs ISR state corruption |
| H4 | HIGH | DCSBIOSBridge | O(n) strcmp in subscriber dispatch (hot path) |
| H5 | HIGH | Mappings | `isLatchedButton()` linear scan on hot path |
| H6 | HIGH | Label Sets | Selector hash table duplicate keys (generator bug) |
| M1 | MEDIUM | RS485 Master | `parseCount` underflow on odd byte count |
| M2 | MEDIUM | RS485 Slave | Missing RISC-V fence on consumer |
| M3 | MEDIUM | RS485 Slave | Export buffer overflow reset race |
| M4 | MEDIUM | DCSBIOSBridge | Static fallback buffer shared across calls |
| M5 | MEDIUM | HIDManager | Static cbuf reentrancy hazard |
| M6 | MEDIUM | RS485 Master | Large stack allocation in FreeRTOS task |
| M7 | MEDIUM | WiFiDebug | Missing #error guard for DEBUG_USE_WIFI path |
| M8 | MEDIUM | Protocol | No max-count guard in DCS-BIOS parser |
| M9 | MEDIUM | InputControl | PCA selector O(groups x inputs) quadratic scan |
| M10 | MEDIUM | DCSBIOSBridge | Serial path byte-at-a-time processing overhead |
| M11 | MEDIUM | DCSBIOSBridge | CT_DISPLAY double hash lookup per word |
| M12 | MEDIUM | DCSBIOSBridge | Display buffer strncmp every frame without dirty flag |
| M13 | MEDIUM | DCSBIOSBridge | Dropped pendingUpdates permanently lost |
| M14 | MEDIUM | InputControl | Null deref in encoder build on null source/controlType |
| M15 | MEDIUM | TFT Panels | Unreachable #elif warning in CabinPressure gauge |

---

## Positive Observations

- Pre-resolved flat tables for GPIO selectors/momentaries/encoders eliminate strcmp from polling hot paths — well done
- O(1) hash lookups for `findCmdEntry()`, `findHidMappingByDcs()`, `findSelectorByDcsAndValue()`, `findDcsOutputEntries()` — excellent
- `#error` guards for invalid transport combinations are comprehensive
- Mapping validation at init (`initMappings()`) catches most overflow conditions early
- Ring buffer SPSC design is correct for the Xtensa ESP32 family (TSO-like ordering)
- The DCS-BIOS protocol parser in `Protocol.cpp` handles sync robustly with the 4-byte sync detection running in parallel with state machine transitions
- All pre-resolved table builds check their respective MAX limits before writing — no buffer overflows in the input init path
- No watchdog-triggering blocking calls found in the input polling path — I2C timeout is bounded, HC165/TM1637 use microsecond delays
- The `onConsistentData()` batched update pattern (queue during writes, flush at frame boundary) is architecturally sound
