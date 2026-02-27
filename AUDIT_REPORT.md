# CockpitOS Critical-Path Code Audit Report

**Date:** 2026-02-27 (revised after validation pass)
**Scope:** End-to-end firmware audit — input hot path, output hot path, RS485 master/slave, transport layers, panel init, label set compilation
**Focus:** Bugs, race conditions, memory corruption, CPU/memory inefficiencies affecting real-time performance

---

## Revision Notice

The initial audit reported 4 CRITICAL, 6 HIGH, and 15 MEDIUM findings. After a thorough re-validation pass — reading all project documentation (`Docs/`, `Docs/LLM/`, config headers), checking actual default config values, tracing every code path with dry runs, and verifying hardware capabilities per ESP32 variant — the vast majority of findings were **retracted**. The developer already accounted for most of the flagged conditions through config guards, buffer sizing, and architectural choices documented in code comments.

**Final validated finding count: 0 CRITICAL, 0 HIGH, 4 MEDIUM (real), 7 LOW (cosmetic/defensive)**

---

## Retracted Findings (with explanation)

### ~~C1. RS485 Slave `txLen` uint8_t overflow~~ — RETRACTED

**Why wrong:** `RS485_TX_BUFFER_SIZE = 128` (default). `queueCommand()` caps `txCount_val` at 128. `sendResponseISR()` caps `toSend = min(txCount_val, 253)` → max 128. Frame = 1+1+128+1 = 131 bytes. `txLen` reaches max 131, fits `uint8_t`. `txFrameBuf` is 132 bytes — data fits. The overflow only occurs if someone changes `RS485_TX_BUFFER_SIZE` to >= 253, which is not the default and would be unusual.

### ~~C2. RS485 Master `txBytes()` spin-wait~~ — RETRACTED

**Why wrong:** The developer **already designed around this**. `RS485_RELAY_CHUNK_SIZE = 124` (comment at `RS485Config.h:102-103`: "124 = 128 FIFO depth - 3 (header) - 1 (checksum) → fits in one FIFO load, avoiding the spin-wait txBytes path for oversized broadcasts"). SMART mode: `RS485_MAX_BROADCAST_CHUNK = 64`. Polls = 3 bytes. ALL frames are <= 128 bytes with default config. The `txBytes()` spin-wait path is **dead code**. Also, `disableRxInt()` only disables RX interrupts, not all system interrupts.

### ~~C3. Ring buffer RISC-V memory barrier race~~ — RETRACTED

**Why wrong:** The WiFi ring buffer (`RingBuffer.cpp`) is only active when `USE_DCSBIOS_WIFI=1`. WiFi-capable ESP32 RISC-V variants: C3 (single-core), C6 (WiFi runs on HP core only). On single-core or single-application-core, volatile prevents compiler reordering, and in-order execution prevents CPU reordering. On dual-core Xtensa (S3), TSO memory ordering makes volatile sufficient for SPSC. The developer is clearly aware of RISC-V fence requirements — the RS485 code uses `RS485_RISCV_FENCE()` and explicit `__riscv` fences where needed.

### ~~C4. `reassemblyLen` local variable in `onDcsBiosUdpPacket`~~ — RETRACTED

**Why wrong:** `dcsUdpRingbufPushChunked()` is **all-or-nothing** — it checks `dcsUdpRingbufAvailable() < needed` before pushing ANY chunks, and pushes all chunks in a tight for loop (~1-2us). The drain loop always finds all chunks of a packet in the ring buffer. The `break` at `MAX_UDP_FRAMES_PER_DRAIN` only fires after `isLastChunk=true` (when `reassemblyLen = 0`). Making `reassemblyLen` static would actually be WORSE — stale state from a previous call could corrupt the next packet.

### ~~H2. `MessageBuffer::clear()` vs ISR~~ — RETRACTED

**Why wrong:** `clear()` is only called when: (1) `messageBuffer.complete = true` → state is IDLE → ISR ignores bytes, or (2) mid-message timeout → error recovery path where losing a partial byte from a corrupted message is harmless.

### ~~H3. `checkRxTimeout()` modifies ISR state~~ — RETRACTED

**Why wrong:** Same reasoning as H2. The timeout handler runs during an error condition (slave went silent). On single-core, the ISR preempts the task, not vice versa. After timeout sets `state = IDLE`, the ISR stops processing bytes. Any ISR byte during the timeout window is from a dead slave — losing it is correct behavior.

### ~~H4. Subscriber dispatch O(n) strcmp~~ — RETRACTED

**Why wrong:** Max 32 subscribers per type (LED, selector, metadata, display). Typical registration: 1-5 subscribers. Even worst-case 32 x 100 outputs/frame = 3200 strcmp of short strings = ~50us per frame at 240MHz. That's 0.15% CPU at 30Hz. Negligible.

### ~~H5. `isLatchedButton()` linear scan~~ — RETRACTED

**Why wrong:** `MAX_LATCHED_BUTTONS = 16`. Called once per physical button event (human input rate ~10Hz). 16 strcmp per button press = ~1us. The documentation (`Docs/Advanced/Latched-Buttons.md:65`) explicitly states: "The linear scan is acceptable because `MAX_LATCHED_BUTTONS` is 16 and the check only involves short string comparisons."

### ~~M2. RS485 Slave RISC-V fence on export buffer~~ — RETRACTED

**Why wrong:** ESP32-C3/C6/H2 are in-order RISC-V cores. The ISR runs on the same core as the consumer task. On single-core in-order, stores from the ISR are visible in program order. volatile prevents compiler reordering, which is sufficient.

### ~~M3. Export buffer overflow ISR reset~~ — RETRACTED

**Why wrong:** The ISR resets both `exportReadPos` and `exportWritePos` to 0 atomically from the ISR's perspective (no preemption within ISR). The consumer reads volatile positions on each loop iteration. After reset, `exportReadPos == exportWritePos`, so the consumer loop exits cleanly. The ISR also resets to `RX_SYNC`, meaning old data was partial/corrupted anyway.

### ~~M4. Static `fallback[16]` in `onSelectorChange`~~ — RETRACTED

**Why wrong:** The static buffer is consumed immediately by `debugPrintf` on line 649. Subscriber callbacks receive `(label, value)` — an integer, not the `stateStr` pointer. No code stores a reference to `fallback`. No reentry path exists (sequential dispatch from protocol parser).

### ~~M5. Static `cbuf[10]` in HIDManager~~ — RETRACTED

**Why wrong:** Both instances are consumed immediately by `sendCommand()`, which formats and sends the DCS-BIOS command and returns. No reentry into HIDManager between `snprintf` and `sendCommand`. The static qualifier is harmless.

### ~~M6. 260-byte stack allocation in RS485 task~~ — RETRACTED

**Why wrong:** 260 bytes in a 4096-byte task stack is 6.3% utilization. ESP32 ISRs use a **separate** interrupt stack, not the task stack. Plenty of headroom for the call chain.

### ~~M7. `DEBUG_USE_WIFI` ring buffer bypass~~ — RETRACTED

**Why wrong:** The DCS-BIOS multicast listener is inside `#if USE_DCSBIOS_WIFI` (WiFiDebug.cpp:178). `DEBUG_USE_WIFI` only enables debug output over WiFi — it does NOT set up the DCS-BIOS data listener. The `DCS_USE_RINGBUFFER` code path is exclusively within the `USE_DCSBIOS_WIFI` block, which already has the `#error` guard.

---

## Validated Findings — MEDIUM

### M1. PCA9555 selector group loop: O(groups x inputs) per chip per poll

| Field | Value |
|-------|-------|
| **File** | `src/Core/InputControl.cpp:594-613` |
| **Impact** | Measurable CPU waste with 4+ PCA chips |

The selector group loop iterates `MAX_SELECTOR_GROUPS` (128) groups, and for each group, scans all `numPca9555Inputs` (up to 64). The GPIO selector path already solved this with pre-resolved flat tables and `gpioSelGroupUsed[]` for O(1) empty-group skip. With 1-2 PCA chips and typical input counts, overhead is ~1% CPU. With 4+ chips, it becomes significant (~6%).

**Fix:** Add `bool pcaSelGroupUsed[MAX_SELECTOR_GROUPS]` to skip empty groups in O(1), matching the GPIO pattern.

---

### M2. Dropped `pendingUpdates` permanently lost — `g_prevValues` already updated

| Field | Value |
|-------|-------|
| **File** | `src/Core/DCSBIOSBridge.cpp:274, 281-285` |
| **Impact** | LEDs/gauges stuck at wrong state until next sim value change |

When `pendingUpdates[]` overflows (> `MAX_PENDING_UPDATES = 220`), dropped updates are never retried because `g_prevValues[index]` was already updated at line 274. The change-detection filter suppresses the same value on subsequent frames. The value stays wrong until DCS-BIOS sends a different value for that address. This is most likely to trigger on the first full export frame after connecting (all addresses sent at once).

**Fix:** Only update `g_prevValues[index]` after successfully queuing:
```cpp
if (pendingUpdateCount < MAX_PENDING_UPDATES) {
    g_prevValues[index] = val;
    pendingUpdates[pendingUpdateCount++] = {entry->label, val, entry->max_value};
} else {
    pendingUpdateOverflow++;
}
```

---

### M3. Selector hash table generator emits duplicate `(dcsCommand, value)` keys

| Field | Value |
|-------|-------|
| **File** | `src/LABELS/*/DCSBIOSBridgeData.h` (generator bug in `src/LABELS/_core/generator_core.py`) |
| **Impact** | Wrong position label in debug output and subscriber callbacks (cosmetic) |

Confirmed duplicate `(dcsCommand, value)` entries in at least 2 label sets:
- `LABEL_SET_KY58`: `KY58_FILL_SELECT` value 1, `KY58_MODE_SELECT` values 0 and 1
- `LABEL_SET_TEST_ONLY`: `ENGINE_CRANK_SW` values 0 and 2

The hash lookup (`findSelectorByDcsAndValue`) returns the first match, making later entries unreachable. Impact is cosmetic — affects position label display (e.g., "NORM" vs "OFF"), not actual DCS-BIOS command routing.

**Fix:** Generator should deduplicate entries by `(dcsCommand, value)` composite key before emitting the hash table.

---

### M4. RS485 Master change queue race between main loop and FreeRTOS task

| Field | Value |
|-------|-------|
| **File** | `lib/CUtils/src/internal/RS485Master.cpp:221-231, 634-654` |
| **Impact** | Off-by-one in change count, causing stale value broadcast or skipped change |

When `RS485_USE_TASK=1` and `RS485_SMART_MODE=1`, `queueChange()` (main loop via `RS485Master_feedExportData`) and `prepareBroadcastData()` (RS485 task) both modify `changeCount` without synchronization. A FreeRTOS tick preempting the main loop during the 3-instruction `changeCount` update window can cause a lost increment/decrement. Consequence is an off-by-one: either a stale queue entry is broadcast (benign — corrected next frame) or a change is skipped (corrected on next sim change).

**Fix:** Wrap the `changeCount` modifications in `portENTER_CRITICAL`/`portEXIT_CRITICAL` with a shared spinlock, or switch to a FreeRTOS stream buffer.

---

## Validated Findings — LOW (Defensive Coding / Marginal)

These are technically correct observations but have negligible practical impact. Included for completeness.

### L1. Protocol parser: no max-count guard

| File | `lib/DCS-BIOS/src/internal/Protocol.cpp:39-46` |
|------|-------|

A corrupted `count` value keeps the parser in data mode, but the 4-byte sync detection runs in parallel and resyncs within one frame (~33ms). Self-correcting.

### L2. RS485 Master `parseCount` underflow on odd byte count

| File | `lib/CUtils/src/internal/RS485Master.cpp:283-293` |
|------|-------|

Odd count from corruption causes underflow, but sync detection resyncs within one frame. Requires bus-level corruption.

### L3. Serial transport byte-at-a-time processing

| File | `src/Core/DCSBIOSBridge.cpp:1816-1818` |
|------|-------|

Function call overhead is ~0.07% CPU at 115200 baud. The parser processes bytes individually regardless of batching.

### L4. CT_DISPLAY double hash lookup

| File | `src/Core/DCSBIOSBridge.cpp:297-312` |
|------|-------|

Two hash lookups for same label per display word. Adds microseconds per frame. Negligible.

### L5. Display buffer strncmp without dirty flag

| File | `src/Core/DCSBIOSBridge.cpp:355-363` |
|------|-------|

strncmp of small buffers (8-16 bytes) at 30Hz is sub-microsecond total.

### L6. Null guard missing in `buildGPIOEncoderStates`

| File | `src/Core/InputControl.cpp:256-257` |
|------|-------|

`strcmp(mi.source, "GPIO")` without null check on `mi.source`. Can't happen with auto-generated mappings, but adding `!mi.source ||` is good defensive coding.

### L7. TFT_Gauges_CabinPressure: unreachable `#elif` warning

| File | `src/Panels/TFT_Gauges_CabinPressure.cpp:8, 481-483` |
|------|-------|

Dead code path — the `#warning` never fires. Other TFT gauge files have the correct pattern.

---

## Summary Table

| # | Severity | Component | Status | Issue |
|---|----------|-----------|--------|-------|
| ~~C1~~ | ~~CRITICAL~~ | RS485 Slave | RETRACTED | txLen can't overflow with default config (128) |
| ~~C2~~ | ~~CRITICAL~~ | RS485 Master | RETRACTED | txBytes() is dead code — config sized to avoid it |
| ~~C3~~ | ~~CRITICAL~~ | Ring Buffer | RETRACTED | RISC-V WiFi chips are single-core; Xtensa is TSO |
| ~~C4~~ | ~~CRITICAL~~ | DCSBIOSBridge | RETRACTED | PushChunked is all-or-nothing; local var is correct |
| ~~H1-H5~~ | ~~HIGH~~ | Various | RETRACTED | See retraction details above |
| ~~H6~~ | ~~HIGH~~ | Label Sets | DOWNGRADED | → M3 (cosmetic impact only) |
| M1 | MEDIUM | InputControl | VALID | PCA selector O(groups x inputs) quadratic scan |
| M2 | MEDIUM | DCSBIOSBridge | VALID | Dropped pendingUpdates permanently lost |
| M3 | MEDIUM | Label Sets | VALID | Selector hash duplicate keys (generator bug) |
| M4 | MEDIUM | RS485 Master | VALID | Change queue race (off-by-one consequence) |
| L1-L7 | LOW | Various | VALID | Defensive coding / marginal performance |

---

## Positive Observations

These hold up after the re-validation and deserve emphasis:

- **Config-as-defense:** `RS485_RELAY_CHUNK_SIZE = 124` was specifically calculated to avoid the FIFO spin-wait path. `RS485_TX_BUFFER_SIZE = 128` keeps slave responses well within uint8_t. These aren't accidents — the comments prove intentional design.
- **RISC-V awareness:** The RS485 code uses `RS485_RISCV_FENCE()`, `#ifdef __riscv` fences in the slave, and documents the rationale in comments. The developer understands the memory model differences.
- **TX_DONE non-blocking pattern:** Both master and slave use the `state → clear → FIFO → enable` ordering with detailed comments explaining the race it prevents. This is expert-level bare-metal UART programming.
- **Pre-resolved flat tables** for GPIO selectors/momentaries/encoders eliminate strcmp from polling hot paths
- **O(1) hash lookups** for `findCmdEntry()`, `findHidMappingByDcs()`, `findSelectorByDcsAndValue()`, `findDcsOutputEntries()`
- **`#error` guards** for invalid transport combinations are comprehensive
- **All-or-nothing ring buffer push** (`PushChunked`) prevents partial-packet consumption
- **FreeRTOS xQueue** for cross-context command passing in RS485 master task mode
- **`portDISABLE_INTERRUPTS`** correctly used in slave `queueCommand` for ISR-safe ring buffer access
- The `onConsistentData()` batched update pattern (queue during writes, flush at frame boundary) is architecturally sound
