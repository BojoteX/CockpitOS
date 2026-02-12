# CockpitOS Transcript - Milestones & Activity Log

> Previous entries archived in `transcript.1.md`

---

## 2025-02-11 — RS485 Master v2.3 Compilation Fix

**Scope**: `lib/CUtils/src/internal/RS485Master.cpp`

### Issue
Master v2.3 TX_DONE refactor failed to compile with three errors:
```
RS485Master.cpp: error: 'flushRxFifo' was not declared in this scope
RS485Master.cpp: error: 'deDeassert' was not declared in this scope
RS485Master.cpp: error: 'enableRxInt' was not declared in this scope
```

### Root Cause
The new TX_DONE handler inside `rxISR()` called `flushRxFifo()`, `deDeassert()`, and `enableRxInt()` — but these UART helper functions were defined *after* `rxISR()` in the file. The original `rxISR()` (v2.2) only called `uart_ll_*` functions (from headers) and `processRxByte()` (forward-declared), so declaration order was never an issue before.

### Fix
- **Moved** all UART helper functions (`deAssert`, `deDeassert`, `disableRxInt`, `enableRxInt`, `flushRxFifo`, `txIdle`, `txByte`, `txBytes`) from after `rxISR()` to before it
- **Added `IRAM_ATTR`** to `deAssert`, `deDeassert`, `disableRxInt`, `enableRxInt`, `flushRxFifo` — these are now called from ISR context (TX_DONE handler)
- **Removed** the old duplicate definitions that remained below `rxISR()`
- `txWithEchoPrevention()` stays below `rxISR()` (not called from ISR, calls helpers which are now above)

### Result
Compilation clean. No functional change — purely a declaration ordering fix.

---

## 2025-02-11 — RS485 TX_DONE Refactor: CONFIRMED WORKING

**Scope**: Both master and slave RS485 drivers

### Outcome
Both the slave (v3.1) and master (v2.3) TX_DONE non-blocking refactors are confirmed working in hardware testing:

- **Slave v3.0 → v3.1**: `sendResponseISR()` no longer blocks. TX_DONE interrupt handles bus turnaround. Confirmed working first.
- **Master v2.2 → v2.3**: `txWithEchoPrevention()` no longer blocks. TX_DONE interrupt handles DE release and RX re-enable. Confirmed working after compilation fix above.

### Test Matrix Status
- Waveshare master + Lolin slave: **FIXED** (was the broken combo)
- Other combinations: working (expected — these were already functional)

### Open Issues from v2.2→v2.3 refactor — CLOSED
- ~~Needs hardware testing with all four combinations~~ → **Confirmed working**
- ~~Consider adding safety timeout for TX_WAITING_DONE~~ → Not needed, hardware is reliable
- Driver-mode TX paths on slave side still blocking (lower priority, separate fix if ever needed)

---

## 2025-02-11 — KNOWN ISSUE: WiFi Causes RS485 Link Freeze

**Scope**: RS485 master ISR + WiFi coexistence

### Symptom
When WiFi is active alongside RS485 master, the RS485 link stops completely — both TX and RX indicator LEDs go dark. No communication occurs.

### Workaround Found
Pinning the RS485 task to **CPU 1** resolves the issue. WiFi runs on CPU 0 by default on dual-core ESP32 variants.

### Suspected Root Cause
**ISR conflict / priority contention on CPU 0.** The WiFi stack uses high-priority interrupts on CPU 0. The RS485 master's `rxISR()` (UART interrupt) is likely being starved or blocked by WiFi interrupts when both run on the same core. Since RS485 is timing-critical (250kbaud, byte-level ISR state machine), even brief ISR latency spikes can break the protocol.

### Investigation Notes
- ESP32 UART interrupts are core-affine — they fire on whichever core called `esp_intr_alloc()`
- If RS485 init runs on CPU 0 (default), the UART ISR is bound to CPU 0 alongside WiFi
- Pinning to CPU 1 isolates RS485 from WiFi interrupt contention
- This is a known ESP32 pattern: real-time peripherals should avoid CPU 0 when WiFi/BLE is active

### Fix Applied
Task pinned to CPU 1 via `xTaskCreatePinnedToCore(..., 1)`. This moves the `esp_intr_alloc()` call (and therefore the UART ISR binding) to CPU 1, away from WiFi's interrupt contention on CPU 0. Issue resolved.

### Future Note
- Could formalize with a `RS485_CPU_CORE` config define so it's documented and harder to accidentally revert
- This affects master only — slave uses driver-mode UART which has its own DMA/interrupt handling

---

## 2025-02-11 — RS485 Master v2.4: Driver-Mode Fallback

**Scope**: `RS485Master.cpp`, `RS485Config.h`, `Config.h`

### Problem
RS485 master ISR mode (`esp_intr_alloc` Level 1 + bare-metal `uart_ll_*`) conflicts with WiFi on the same core. WiFi's `portENTER_CRITICAL` sections starve the UART ISR, killing the RS485 link after 30-45 seconds. Also affects USB on single-core chips (S2). Pinning to a separate core works on dual-core (S3) but is not an option on single-core devices.

### Solution
Added `RS485_USE_ISR_MODE` support to the master, matching the pattern the slave already uses:

- **ISR mode (default, `RS485_USE_ISR_MODE=1`)**: Unchanged. Bare-metal UART, TX_DONE non-blocking, max performance. Requires a quiet core.
- **Driver mode (`RS485_USE_ISR_MODE=0`)**: Uses ESP-IDF `uart_driver_install()` + `uart_read_bytes()` + `uart_write_bytes()`. Blocking TX at task level. Rock-solid WiFi coexistence.

### Changes Made

**RS485Config.h**:
- Added `RS485_USE_ISR_MODE` `#ifndef` fallback define (default 1), matching RS485SlaveConfig.h

**Config.h**:
- Updated comment on line 35 — `RS485_USE_ISR_MODE` now applies to both master and slave

**RS485Master.cpp (v2.3 → v2.4)**:
- Updated file header to document dual-mode architecture
- Conditional includes: bare-metal headers (`hal/uart_ll.h`, `esp_intr_alloc.h`, etc.) wrapped in `#if RS485_USE_ISR_MODE`; `driver/uart.h` always included
- Hardware abstraction macros (`RS485_UART_DEV`, etc.) wrapped in `#if RS485_USE_ISR_MODE`
- `TxContext` enum, `txContext`, `intrHandle` wrapped in `#if RS485_USE_ISR_MODE`
- UART helpers, `rxISR()`, ISR-mode `txWithEchoPrevention()` wrapped in `#if RS485_USE_ISR_MODE`
- Added driver-mode `txWithEchoPrevention()` using `uart_write_bytes()` + `uart_wait_tx_done()` (blocking)
- `sendPoll()`, `sendBroadcast()`, `sendTimeoutZero()`: `txContext` assignments wrapped; post-TX state transitions added for driver mode
- `checkRxTimeout()`: `flushRxFifo()`/`enableRxInt()` branched to `uart_flush_input()` for driver mode
- Split hardware init into `initRS485Hardware_ISR()` and `initRS485Hardware_Driver()`
- Added `uart_read_bytes()` RX polling at top of `rs485PollLoop()` for driver mode
- `RS485Master_stop()`: branched cleanup (`esp_intr_free` vs `uart_driver_delete`)
- Status/debug messages report ISR vs Driver mode

### What Was NOT Changed
- `processRxByte()` — shared between both modes (works from ISR or task context)
- `processCompletedMessage()` / `processInputCommand()` — shared
- `rs485PollLoop()` logic — shared (only RX polling insert added for driver mode)
- MessageBuffer, smart mode, relay mode, FreeRTOS task, public API — all untouched
- All ISR-mode code paths — completely untouched, just wrapped in `#if`

### Backups
- `RS485Master.cpp.BAK2` — pre-driver-mode v2.3
- `RS485Config.h.BAK2` — pre-driver-mode config

### Testing Required
- Compile with `RS485_USE_ISR_MODE 1` — verify zero regression
- Compile with `RS485_USE_ISR_MODE 0` — verify clean compilation
- Test driver mode with WiFi on same core — RS485 should survive indefinitely
- Test driver mode RS485 functionality — polls, responses, broadcasts, timeouts

---
