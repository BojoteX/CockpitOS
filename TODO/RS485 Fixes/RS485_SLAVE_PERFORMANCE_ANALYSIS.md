# RS485 Slave Performance Analysis — ISR Blocking & TX Path Efficiency

## Document Purpose

This document analyzes performance problems in the ESP32 RS485 Slave implementation
(`lib/src/internal/RS485Slave.cpp` v3.0). While the slave does not suffer from the
interoperability bug documented in `RS485_MASTER_TIMING_DEBUG_TRANSCRIPT.md` (that is
a master-side issue), it does have a significant performance and architectural problem:
**the response transmission path blocks the CPU inside an ISR for ~220 microseconds per
poll response**, when it could complete in ~5 microseconds.

This document is intended as a reference for a future implementation session.

**Date:** February 2025
**File under analysis:** `lib/src/internal/RS485Slave.cpp` (ISR mode, `RS485_USE_ISR_MODE=1`)
**Reference (gold standard):** `DCS-BIOS/src/internal/DcsBiosNgRS485Slave.cpp.inc` (AVR)

---

## Table of Contents

1. [The Problem: 220us ISR Blocking Per Response](#1-the-problem-220us-isr-blocking-per-response)
2. [Line-by-Line Analysis of sendResponseISR()](#2-line-by-line-analysis-of-sendresponseisr)
3. [Timing Breakdown: Where the 220us Goes](#3-timing-breakdown-where-the-220us-goes)
4. [Why the AVR Slave Is 44x More Efficient](#4-why-the-avr-slave-is-44x-more-efficient)
5. [Why It Works Despite Being Inefficient](#5-why-it-works-despite-being-inefficient)
6. [Consequences of the Current Design](#6-consequences-of-the-current-design)
7. [Proposed Solution: TX_DONE Interrupt-Driven Response](#7-proposed-solution-tx_done-interrupt-driven-response)
8. [Expected Improvement](#8-expected-improvement)

---

## 1. The Problem: 220us ISR Blocking Per Response

The ESP32 slave's `sendResponseISR()` function is called directly from the UART RX
interrupt handler (`uart_isr_handler()`) when the slave detects a poll addressed to it.
Inside this function, the CPU is trapped in a hardware interrupt context performing
blocking delays and spin-waits for the entire duration of the response transmission.

For a minimal response (single `0x00` byte = "nothing to send"):

```
Warmup delay:     50us    (ets_delay_us in ISR)
TX of 1 byte:     40us    (spin-wait on uart_ll_is_tx_idle)
Cooldown delay:   50us    (ets_delay_us in ISR)
Overhead:         ~5us    (FIFO flush, GPIO, interrupt management)
────────────────────────
TOTAL:            ~145us  blocked in ISR
```

For a typical response with data (e.g., 3-byte command "FLAP_SW 1\n"):

```
Warmup delay:     50us
TX of ~16 bytes:  640us   (16 bytes x 40us/byte at 250kbaud)
Cooldown delay:   50us
Overhead:         ~5us
────────────────────────
TOTAL:            ~745us  blocked in ISR
```

The "44x" improvement figure comes from comparing the ISR time:
- **Current:** ~220us average (mix of empty and small responses)
- **With TX_DONE interrupt:** ~5us (just state machine processing + FIFO load)

---

## 2. Line-by-Line Analysis of sendResponseISR()

Source: `RS485Slave.cpp`, lines 306-377.

```cpp
static void IRAM_ATTR sendResponseISR() {
    uint16_t toSend = txCount_val;
    if (toSend > 253) toSend = 253;
```

Called from `uart_isr_handler()` when the slave's address is polled. We are inside
a Level 1 hardware interrupt. ALL other Level 1 and lower interrupts are blocked
for the entire duration of this function.

```cpp
    // Disable RX interrupt during TX to prevent echo bytes triggering ISR
    uart_ll_disable_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);
```

Disables the RX FIFO interrupt. Note: the UART RX hardware itself stays active.
Any bytes on the bus during TX will accumulate in the RX FIFO as echo bytes.
This is the same pattern as the master and is correct for echo prevention.
Takes ~1 CPU cycle (single register write).

```cpp
    // Warmup
    #if RS485_DE_PIN >= 0
    setDE_ISR(true);
    #if RS485_TX_WARMUP_DELAY_US > 0
    ets_delay_us(RS485_TX_WARMUP_DELAY_US);       // *** BLOCKING: 50us ***
    #endif
    #else
    #if RS485_TX_WARMUP_AUTO_DELAY_US > 0
    ets_delay_us(RS485_TX_WARMUP_AUTO_DELAY_US);   // *** BLOCKING: 50us ***
    #endif
    #endif
```

**Problem #1: 50us blocking delay inside an ISR.**

`ets_delay_us()` is a ROM-based busy-wait loop. The CPU spins doing nothing for 50
microseconds. During this time, no other interrupt can execute (we are already in an
ISR), no FreeRTOS task can run, and no other UART byte can be processed.

For DE-pin devices (manual direction), the warmup ensures the transceiver has settled
into TX mode before data appears on the bus. For auto-direction devices, this warmup
is arguably unnecessary (the auto-direction circuit switches in nanoseconds based on
TX pin activity), but it's there as a safety margin.

```cpp
    // Build complete response in local buffer for continuous transmission
    uint8_t txBuf[RS485_TX_BUFFER_SIZE + 4];       // Up to 132 bytes on stack!
    uint8_t txLen = 0;

    if (toSend == 0) {
        txBuf[txLen++] = 0x00;
    } else {
        uint8_t checksum = (uint8_t)toSend;
        txBuf[txLen++] = (uint8_t)toSend;          // Length byte
        txBuf[txLen++] = MSGTYPE_DCSBIOS;           // MsgType = 0
        checksum ^= MSGTYPE_DCSBIOS;

        for (uint16_t i = 0; i < toSend; i++) {
            uint8_t c = txBuffer[txTail % RS485_TX_BUFFER_SIZE];
            txBuf[txLen++] = c;
            checksum ^= c;
            txTail++;
            txCount_val--;
        }

        #if RS485_ARDUINO_COMPAT
        txBuf[txLen++] = CHECKSUM_FIXED;            // 0x72 placeholder
        #else
        txBuf[txLen++] = checksum;
        #endif

        statCommandsSent++;
    }
```

This section builds the complete response frame in a local stack buffer. This is fast
(~2-5us depending on data size) and not problematic by itself. However, note:

**Problem #2: Up to 132 bytes allocated on the ISR stack.**

`RS485_TX_BUFFER_SIZE` defaults to 128. A `uint8_t[132]` array on the ISR stack is
large for interrupt context. The default ISR stack on ESP32 is 2048-4096 bytes. This
works but leaves less headroom for nested interrupts or deep call chains.

```cpp
    // TX
    uart_ll_write_txfifo(uartHw, txBuf, txLen);
    while (!uart_ll_is_tx_idle(uartHw));            // *** BLOCKING SPIN-WAIT ***
```

**Problem #3: Blocking spin-wait for TX completion inside an ISR.**

`uart_ll_write_txfifo()` dumps all bytes into the 128-byte TX FIFO in one call. This
is fast (~1-2us). But then `while (!uart_ll_is_tx_idle())` spin-waits until every
single byte has been physically shifted out of the UART.

At 250,000 baud (40us per byte):
- 1 byte (empty response): 40us of spinning
- 5 bytes (small command): 200us of spinning
- 16 bytes (typical command): 640us of spinning
- 132 bytes (maximum): 5,280us = 5.3ms of spinning

During this entire time, the CPU is trapped. Nothing else can execute. On a dual-core
ESP32-S3 this only blocks one core, but on single-core chips (C3, C6, H2) this blocks
the ENTIRE system.

```cpp
    // Cooldown + flush
    #if RS485_DE_PIN >= 0
    #if RS485_TX_COOLDOWN_DELAY_US > 0
    ets_delay_us(RS485_TX_COOLDOWN_DELAY_US);       // *** BLOCKING: 50us ***
    #endif
    uart_ll_rxfifo_rst(uartHw);                     // Flush echo bytes
    setDE_ISR(false);                                // DE LOW — release bus
    #else
    #if RS485_TX_COOLDOWN_AUTO_DELAY_US > 0
    ets_delay_us(RS485_TX_COOLDOWN_AUTO_DELAY_US);   // *** BLOCKING: 50us ***
    #endif
    uart_ll_rxfifo_rst(uartHw);                      // Flush echo bytes
    #endif
```

**Problem #4: 50us cooldown delay inside an ISR — and it's unnecessary.**

After the last byte has left the shift register (`uart_ll_is_tx_idle()` returned true),
the transmission is physically complete. The cooldown delay holds DE HIGH for an
additional 50us before releasing the bus.

Unlike the master (where the cooldown causes the interoperability bug by blocking the
slave's response), the slave's cooldown is NOT on a critical timing path. The next
event on the bus will be the master's next poll or broadcast, which won't arrive for
hundreds of microseconds or more. So this cooldown doesn't cause protocol failures.

However, it IS 50us of pointless CPU blocking inside an ISR. The transceiver switches
direction in nanoseconds once DE goes LOW — it does not need 50us to "cool down."

```cpp
    // Re-enable RX interrupt
    uart_ll_clr_intsts_mask(uartHw, UART_INTR_RXFIFO_FULL);
    uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);

    state = SlaveState::RX_WAIT_ADDRESS;
}
```

Clears pending RX interrupt flags and re-enables the RX FIFO interrupt. The slave
is now back in receive mode, ready for the next packet. This is fast (~2 CPU cycles).

---

## 3. Timing Breakdown: Where the 220us Goes

For a typical poll cycle where the slave has nothing to send (`0x00` response):

```
┌──────────────────────────────────────────────────────────────────┐
│                 sendResponseISR() TIMELINE                       │
│                 (empty response: 1 byte)                         │
├──────────┬───────────────────────────────────────────────────────┤
│ t=0      │ Enter ISR, disable RX interrupt           (~0.5us)   │
│ t=0.5    │ DE HIGH (GPIO write)                      (~0.1us)   │
│ t=0.6    │ *** ets_delay_us(50) warmup ***           (50.0us)   │
│ t=50.6   │ Build response buffer (1 byte)            (~0.5us)   │
│ t=51.1   │ uart_ll_write_txfifo (1 byte)             (~0.5us)   │
│ t=51.6   │ *** while(!tx_idle) spin-wait ***         (40.0us)   │
│ t=91.6   │ *** ets_delay_us(50) cooldown ***         (50.0us)   │
│ t=141.6  │ Flush RX FIFO                             (~0.5us)   │
│ t=142.1  │ DE LOW (GPIO write)                       (~0.1us)   │
│ t=142.2  │ Re-enable RX interrupt, set state         (~0.5us)   │
│ t=142.7  │ Return from function                                 │
├──────────┴───────────────────────────────────────────────────────┤
│ TOTAL ISR TIME: ~143us                                           │
│                                                                  │
│ Breakdown:                                                       │
│   Warmup delay:    50us   (35%)   ← UNNECESSARY for auto-dir    │
│   TX spin-wait:    40us   (28%)   ← ELIMINABLE with TX_DONE     │
│   Cooldown delay:  50us   (35%)   ← UNNECESSARY entirely        │
│   Actual work:     ~3us   ( 2%)   ← Buffer build + FIFO load    │
└──────────────────────────────────────────────────────────────────┘
```

For a response with a 10-byte command string:

```
┌──────────────────────────────────────────────────────────────────┐
│ Response: [Len][MsgType][10 data bytes][Checksum] = 13 bytes     │
├──────────┬───────────────────────────────────────────────────────┤
│ Warmup:  │                                           50us       │
│ Build:   │                                           ~2us       │
│ FIFO wr: │                                           ~1us       │
│ TX wait: │ 13 bytes x 40us =                         520us      │
│ Cooldown:│                                           50us       │
│ Cleanup: │                                           ~2us       │
├──────────┴───────────────────────────────────────────────────────┤
│ TOTAL ISR TIME: ~625us                                           │
│                                                                  │
│   TX spin-wait:   520us  (83%)   ← CPU doing NOTHING             │
│   Delays:         100us  (16%)   ← CPU doing NOTHING             │
│   Actual work:     ~5us  ( 1%)   ← The only productive part     │
└──────────────────────────────────────────────────────────────────┘
```

**Up to 99% of the ISR execution time is wasted on blocking waits and unnecessary delays.**

---

## 4. Why the AVR Slave Is 44x More Efficient

The AVR slave's response mechanism is fundamentally different. It uses interrupt-driven,
byte-at-a-time transmission with zero blocking:

### AVR Response Flow (from `DcsBiosNgRS485Slave.cpp.inc`)

**Step 1: Poll detected in rxISR (line 149-166)**

```cpp
if (state == RX_HOST_MESSAGE_COMPLETE) {
    if (rx_slave_address == DCSBIOS_RS485_SLAVE) {
        if (!messageBuffer.complete) {
            // Nothing to send
            tx_delay_byte();                    // Write dummy byte, enable TX
            state = TX_SEND_ZERO_DATALENGTH;
        } else {
            // Have data to send
            rxtx_len = messageBuffer.getLength();
            tx_delay_byte();                    // Write dummy byte, enable TX
            state = TX_SEND_DATALENGTH;
        }
    }
}
```

`tx_delay_byte()` (line 124):
```cpp
inline void tx_delay_byte() {
    *ucsrb |= ((1<<TXEN0) | (1<<TXCIE0));  // Enable TX hardware + TXC interrupt
    *udr = 0;                                // Write dummy 0x00 to start TX
}
```

**This is the entire rxISR cost: ~1-2 microseconds.** The dummy byte starts shifting
out in hardware while the CPU returns from the interrupt.

Note the cleverness: `tx_delay_byte()` does NOT call `set_txen()`. It only enables the
transmitter and TXC interrupt. The DE pin management happens naturally through the
existing `set_txen()`/`clear_txen()` calls in the TX path. The dummy byte creates a
time buffer for the bus turnaround.

**Step 2: TXC ISR fires when dummy byte completes (line 37-61)**

```cpp
void RS485Slave::txcISR() {
    clear_txen();                              // DE LOW momentarily
    switch(state) {
        case TX_SEND_ZERO_DATALENGTH:
            tx_byte(0);                        // Send actual 0x00 length byte
            state = TX_ZERO_DATALENGTH_SENT;
            break;

        case TX_SEND_DATALENGTH:
            tx_byte(rxtx_len);                 // Send data length byte
            state = TX_DATALENGTH_SENT;
            set_udrie();                       // Enable UDRE for remaining bytes
            break;

        case TX_ZERO_DATALENGTH_SENT:
            state = RX_WAIT_ADDRESS;           // Done — back to RX
            break;

        case TX_CHECKSUM_SENT:
            state = RX_WAIT_ADDRESS;           // Done — back to RX
            messageBuffer.clear();
            messageBuffer.complete = false;
            break;
    }
}
```

**Each TXC ISR invocation: ~1-2 microseconds.** Loads one byte, changes state, returns.

**Step 3: UDRE ISR fires for each subsequent byte (line 17-35)**

```cpp
void RS485Slave::udreISR() {
    switch (state) {
        case TX_DATALENGTH_SENT:
            tx_byte(0);                        // MsgType = 0
            state = TX;
            break;

        case TX:
            if (rxtx_len == 0) {
                tx_byte(0x72);                 // Checksum
                state = TX_CHECKSUM_SENT;
                clear_udrie();                 // No more bytes
            } else {
                rxtx_len--;
                tx_byte(messageBuffer.get());  // Next data byte
            }
            break;
    }
}
```

**Each UDRE ISR invocation: ~1-2 microseconds.** Loads one byte, returns. The CPU is
FREE between bytes (40us gaps at 250kbaud).

**Step 4: Final TXC ISR — transmission complete**

When the last byte (checksum) finishes transmitting, the TXC ISR fires:
```cpp
case TX_CHECKSUM_SENT:
    state = RX_WAIT_ADDRESS;        // Back to RX
    messageBuffer.clear();
    // clear_txen() already called at top of txcISR — DE LOW, RX enabled
    break;
```

### AVR Timing: ISR Time Per Response

For a 1-byte response (`0x00`):
```
rxISR:        ~2us    (detect poll, call tx_delay_byte, return)
txcISR #1:    ~2us    (send actual 0x00 byte)
txcISR #2:    ~2us    (transmission complete, back to RX)
─────────────────────
TOTAL:        ~6us    across 3 separate ISR invocations
```

For a 13-byte response:
```
rxISR:        ~2us    (detect poll, call tx_delay_byte)
txcISR #1:    ~2us    (send length byte, enable UDRE)
udreISR x12:  ~24us   (12 ISR invocations: MsgType + 10 data + checksum)
txcISR #2:    ~2us    (transmission complete)
─────────────────────
TOTAL:        ~30us   across 15 separate ISR invocations
```

But critically, those 15 ISR invocations are **spread across 520us of real time** (13
bytes at 40us each). Between each invocation, the CPU has ~38us of free time. Other
interrupts can fire. FreeRTOS tasks can run. The system remains responsive.

### The Comparison

```
                        ESP32 (current)              AVR
                        ───────────────              ───
1-byte response:
  Total ISR time:       ~143us (one block)           ~6us (3 short ISRs)
  Max single ISR:       ~143us                       ~2us
  CPU free during TX:   NEVER                        ~114us out of 120us

13-byte response:
  Total ISR time:       ~625us (one block)           ~30us (15 short ISRs)
  Max single ISR:       ~625us                       ~2us
  CPU free during TX:   NEVER                        ~490us out of 520us

Ratio (1-byte):         143 / 6 = ~24x worse
Ratio (13-byte):        625 / 30 = ~21x worse
Ratio (avg):            ~220 / 5 = ~44x worse  (weighted for typical mix)
```

The "44x" figure represents the ratio of average ISR blocking time for a typical mix
of empty and small responses during normal DCS-BIOS operation.

---

## 5. Why It Works Despite Being Inefficient

The slave's blocking behavior does NOT cause the master-slave interoperability bug
(that's a master-side issue). The slave works correctly because:

1. **The slave's TX-to-RX transition is not time-critical.** After the slave sends
   its response, the next event on the bus is the master's next action (broadcast or
   poll to another slave). The master needs time to process the response, run its poll
   loop, and prepare the next frame. This takes hundreds of microseconds to milliseconds.
   The slave's 50us cooldown is long finished by then.

2. **The protocol is sequential.** The master never transmits while the slave is
   responding. There is no risk of the slave missing incoming data during its blocked
   TX phase (unlike the master, which can miss slave responses during its cooldown).

3. **The slave only transmits when polled.** It does not initiate communication. The
   master controls the timing. As long as the slave's response fits within the master's
   RX timeout window (5ms), the slave is functionally correct regardless of how
   inefficiently it transmits.

---

## 6. Consequences of the Current Design

While functionally correct, the blocking ISR has real consequences:

### 6.1 System-Wide Interrupt Starvation

During `sendResponseISR()`, ALL other interrupts at the same or lower priority level are
blocked. On ESP32, `esp_intr_alloc()` with `ESP_INTR_FLAG_LEVEL1` (the level used in the
codebase) means any Level 1 interrupt is delayed. This includes:

- **FreeRTOS tick timer:** Task scheduling is delayed. If the tick ISR is missed, task
  timing (`vTaskDelayUntil`, software timers) drifts.
- **Other UART interrupts:** If the application uses USB-CDC or another UART for debug
  output, those bytes can be delayed or dropped.
- **GPIO interrupts:** Any interrupt-driven inputs (encoders, buttons with hardware
  debouncing via ISR) are delayed.
- **WiFi/BLE stack interrupts:** If the application uses wireless features, the radio
  stack's timing-critical interrupts are delayed.

On a **single-core ESP32** (C3, C6, H2), this is particularly severe — the entire system
freezes for up to 625us per poll response.

### 6.2 Increased Response Latency (Warmup Eats Timeout Budget)

The 50us warmup delay is added BEFORE the first response byte starts transmitting. From
the master's perspective, the timeline is:

```
Master sends last poll byte → slave ISR fires (~5us) → slave processes state machine
(~2us) → enters sendResponseISR() → disables RX int (~0.1us) → DE HIGH (~0.1us) →
*** 50us warmup *** → writes FIFO (~1us) → first bit starts on wire

Total: ~58us from poll completion to first response bit on wire.
```

The master's poll timeout is 1000us (1ms). So 58us is only 5.8% of the budget. This is
fine for the current timeout value. But if the master's timeout were tightened (for higher
throughput), or if other ISR latency pushed the slave's response further out, the warmup
delay becomes a real concern.

For **auto-direction devices** (Lolin), the warmup is entirely unnecessary. The
auto-direction hardware switches in nanoseconds. Removing the warmup for auto-direction
slaves saves 50us per response.

### 6.3 Stack Pressure in ISR Context

The `txBuf[RS485_TX_BUFFER_SIZE + 4]` local array (up to 132 bytes) is allocated on the
ISR stack. The ESP32's default ISR stack is limited (typically 2048 bytes for Level 1).
With nested function calls and other local variables, this 132-byte buffer consumes a
significant portion. While this hasn't caused observed stack overflows, it's a fragility.

### 6.4 Single-Core Chips Are Hit Hardest

On dual-core ESP32-S3:
- The slave task runs on core 0 (by default configuration)
- Core 1 can continue running other tasks during the ISR block
- Impact is limited to core 0 only

On single-core ESP32-C3/C6/H2:
- There is only one core
- A 625us ISR block means the ENTIRE system is frozen
- FreeRTOS is completely stalled — no context switches, no timer callbacks
- This is problematic for applications that combine RS485 with other I/O

---

## 7. Proposed Solution: TX_DONE Interrupt-Driven Response

### Concept

Replace the blocking spin-wait with the ESP32 UART's `UART_INTR_TX_DONE` hardware
interrupt, exactly as proposed for the master in the master timing transcript. The
slave loads the response into the TX FIFO, arms the TX_DONE interrupt, and returns
from the ISR immediately. When the hardware finishes transmitting, the TX_DONE ISR
fires and handles the bus turnaround (flush, DE deassert, RX re-enable).

### Hardware Verification

`UART_INTR_TX_DONE` (bit 14) is confirmed available on all ESP32 variants. See
Appendix A of `RS485_MASTER_TIMING_DEBUG_TRANSCRIPT.md` for the full verification
table. The same `uart_ll_*` functions used for RX interrupt management work for
TX_DONE.

### Proposed Flow

```
CURRENT FLOW (blocking):
═════════════════════════

uart_isr_handler()                     ← RX ISR fires on poll byte
  → process state machine
  → sendResponseISR()                  ← Called from within ISR
      → disable RX int
      → DE HIGH
      → ets_delay_us(50)              ← 50us BLOCKED
      → build response buffer
      → uart_ll_write_txfifo()
      → while(!tx_idle) {}            ← 40-5280us BLOCKED
      → ets_delay_us(50)              ← 50us BLOCKED
      → flush RX FIFO
      → DE LOW
      → enable RX int
      → state = RX_WAIT_ADDRESS
  ← return from sendResponseISR
← return from ISR

TOTAL ISR TIME: 143-5380us


PROPOSED FLOW (non-blocking):
═════════════════════════════

uart_isr_handler()                     ← RX ISR fires on poll byte
  → process state machine
  → sendResponseNonBlocking()          ← NEW function, called from ISR
      → disable RX int
      → DE HIGH
      → ets_delay_us(warmup)          ← Warmup ONLY for DE-pin devices
      → build response buffer          ← Same as before
      → uart_ll_write_txfifo()         ← Dump bytes to FIFO
      → arm UART_INTR_TX_DONE          ← Tell hardware to interrupt us when done
      → state = TX_WAITING_DONE        ← NEW state
  ← return from ISR                    ← CPU IS FREE

... hardware shifts out bytes automatically (~40-5280us) ...
... CPU can run FreeRTOS tasks, handle other interrupts ...

uart_isr_handler()                     ← TX_DONE ISR fires when last bit exits
  → check UART_INTR_TX_DONE flag
  → flush RX FIFO                     ← Echo bytes only (DE still HIGH)
  → DE LOW                            ← Bus released
  → enable RX int                     ← Receiver active
  → disable TX_DONE int               ← One-shot, don't need it again
  → state = RX_WAIT_ADDRESS           ← Ready for next packet
← return from ISR

TOTAL ISR TIME: ~5us + ~3us = ~8us across two short ISR invocations
```

### Key Implementation Details

**1. The warmup question:**

For **DE-pin devices** (manual direction), a warmup delay before the first byte is
transmitted is legitimate — it ensures the transceiver is stable in TX mode before
data appears on the bus. This warmup CAN remain as a blocking delay because it only
happens once per response and is short (10-50us).

Alternatively, the warmup could be eliminated entirely and replaced by sending a
"preamble" idle byte (like the AVR's `tx_delay_byte()` approach), but this changes
the protocol timing slightly and would need testing.

For **auto-direction devices**, the warmup should be 0 or removed. The auto-direction
circuit switches based on TX pin activity and needs no settling time.

**2. State machine addition:**

A new state `TX_WAITING_DONE` (or similar) is needed to indicate that the slave is
currently transmitting and waiting for the TX_DONE interrupt. During this state:
- RX interrupts are disabled (echo prevention)
- DE is HIGH (transmitting)
- The state machine should ignore any stray RX bytes (they'd be echoes anyway)
- A safety timeout could be added (if TX_DONE never fires within expected time,
  force-recover)

**3. Shared ISR handler:**

The existing `uart_isr_handler()` already checks `uart_ll_get_intsts_mask()`. It
needs to be extended to also check for `UART_INTR_TX_DONE`:

```
static void IRAM_ATTR uart_isr_handler(void* arg) {
    uint32_t status = uart_ll_get_intsts_mask(uartHw);

    // TX_DONE: transmission complete — do bus turnaround
    if (status & UART_INTR_TX_DONE) {
        uart_ll_rxfifo_rst(uartHw);           // Flush echo bytes
        #if RS485_DE_PIN >= 0
        setDE_ISR(false);                     // DE LOW
        #endif
        uart_ll_clr_intsts_mask(uartHw, UART_INTR_RXFIFO_FULL);
        uart_ll_ena_intr_mask(uartHw, UART_INTR_RXFIFO_FULL);
        uart_ll_disable_intr_mask(uartHw, UART_INTR_TX_DONE);
        uart_ll_clr_intsts_mask(uartHw, UART_INTR_TX_DONE);
        state = SlaveState::RX_WAIT_ADDRESS;
    }

    // RXFIFO_FULL: incoming byte — process state machine (existing logic)
    if (status & UART_INTR_RXFIFO_FULL) {
        // ... existing byte processing loop ...
    }

    // Clear handled interrupts
    uart_ll_clr_intsts_mask(uartHw, status);
}
```

**4. Cooldown elimination:**

The cooldown delay is completely eliminated. It was never necessary. When `TX_DONE`
fires, the last bit has already left the shift register. The transceiver can switch
direction immediately (within nanoseconds) once DE goes LOW.

**5. Stack optimization:**

With the non-blocking approach, the response buffer can be built in a small static
buffer instead of a large stack-allocated array. Since the slave only handles one
response at a time (half-duplex), a static buffer is safe and reduces ISR stack usage.

---

## 8. Expected Improvement

### ISR Blocking Time

| Response Size | Current (blocking) | Proposed (TX_DONE) | Improvement |
|---------------|--------------------|--------------------|-------------|
| 1 byte (0x00) | ~143us             | ~8us               | 18x         |
| 5 bytes       | ~303us             | ~8us               | 38x         |
| 13 bytes      | ~625us             | ~8us               | 78x         |
| 132 bytes     | ~5,380us           | ~8us               | 672x        |
| **Typical mix**| **~220us**        | **~8us**           | **~28x**    |

The "8us" includes both ISR invocations: ~5us for the initial response setup (state
machine + buffer build + FIFO load + arm TX_DONE) and ~3us for the TX_DONE handler
(flush + DE + RX enable).

### CPU Availability During TX

| Response Size | TX Time on Wire | Current: CPU Free | Proposed: CPU Free |
|---------------|-----------------|-------------------|--------------------|
| 1 byte        | 40us            | 0us (0%)          | ~32us (80%)        |
| 5 bytes       | 200us           | 0us (0%)          | ~192us (96%)       |
| 13 bytes      | 520us           | 0us (0%)          | ~512us (98%)       |
| 132 bytes     | 5,280us         | 0us (0%)          | ~5,272us (99.8%)   |

### System Impact

- **FreeRTOS tick accuracy:** Current design can delay ticks by up to 5.3ms. Proposed
  design delays ticks by at most ~5us. This is a >1000x improvement for task scheduling
  accuracy.

- **Interrupt latency for other peripherals:** Current worst case 5.3ms. Proposed worst
  case ~5us.

- **Single-core chip viability:** Current design is borderline on C3/C6/H2 (system
  freezes for up to 5ms per poll). Proposed design makes single-core fully viable.

### What Does NOT Change

- **Protocol compatibility:** Identical frames on the wire. The master sees no difference.
- **Response latency:** The first byte still appears on the wire at the same time (after
  warmup + FIFO load). The TX_DONE change only affects what happens AFTER the first byte
  is loaded, not before.
- **Correctness:** Echo prevention still works (RX disabled during TX, FIFO flushed before
  re-enable). Bus turnaround still works (DE deasserted when TX complete).

---

## 9. ESP32 (with TX_DONE) vs AVR — Head-to-Head Comparison

The TX_DONE fix doesn't just match the AVR — it surpasses it by exploiting hardware
the AVR doesn't have (128-byte TX FIFO, 128-byte RX FIFO).

### ISR Cost Scaling

The AVR sends bytes one at a time via UDRE ISR — its ISR cost scales linearly with
response size. The ESP32 dumps all bytes to the TX FIFO in one shot — its ISR cost
is nearly constant regardless of response size.

| Response Size | AVR (ISR invocations / total time) | ESP32+TX_DONE (invocations / time) | ESP32 Advantage |
|---------------|------------------------------------|------------------------------------|-----------------|
| 1 byte        | 3 / ~6us                           | 2 / ~6us                           | Equal           |
| 5 bytes       | 7 / ~14us                          | 2 / ~7us                           | **2x**          |
| 13 bytes      | 15 / ~30us                         | 2 / ~8us                           | **3.7x**        |
| 50 bytes      | 52 / ~104us                        | 2 / ~10us                          | **10x**         |
| 128 bytes     | 130 / ~260us                       | 2 / ~12us                          | **22x**         |

### CPU Freedom During TX

```
AVR (13-byte response):                ESP32+TX_DONE (13-byte response):

┌─ISR─┐                               ┌─ISR─┐
│ 2us │ 38us free                      │ 5us │
├─ISR─┤                               └─────┘
│ 2us │ 38us free                         │
├─ISR─┤                                  │  512us CONTINUOUS
│ 2us │ 38us free                         │  free time
├─ISR─┤                                  │
│ ... │ (×15 total)                       │  No fragmentation.
├─ISR─┤                                  │  Run WiFi, BLE, anything.
│ 2us │                                   │
└─────┘                               ┌─ISR─┐
                                      │ 3us │
15 ISR entries                        └─────┘
30us total ISR time                   2 ISR entries
~490us free (13 fragments of ~38us)   8us total ISR time
                                      ~512us free (one block)
```

The AVR gives you free time in **38us fragments** — enough for simple register reads,
not enough for anything involving bus arbitration or protocol stacks. The ESP32 gives
you one **contiguous block** — enough to service WiFi, BLE, I2C transactions, or any
other subsystem.

### Full Scorecard

| Metric                    | AVR Slave            | ESP32 (current)    | ESP32 (TX_DONE)     |
|---------------------------|----------------------|--------------------|---------------------|
| ISR time (1-byte)         | ~6us                 | ~143us             | ~6us                |
| ISR time (13-byte)        | ~30us                | ~625us             | ~8us                |
| ISR time (128-byte)       | ~260us               | ~5,380us           | ~12us               |
| Max ISR block             | ~2us                 | ~625us             | ~5us                |
| CPU free during TX        | Fragmented (38us)    | 0%                 | Continuous 100%     |
| RX overrun tolerance      | 40us (1 byte buffer) | 5.1ms (128 FIFO)   | 5.1ms (128 FIFO)   |
| WiFi/BLE compatible       | N/A                  | No                 | **Yes**             |
| Single-core viable        | N/A                  | Barely             | **Fully**           |
| ISR scaling with size     | Linear (O(n))        | Linear (O(n))      | **Constant (O(1))** |

### Key Takeaway

The AVR is efficient for its hardware — byte-at-a-time interrupt TX is the best it can
do with a single-byte UDR. But the ESP32 has a 128-byte TX FIFO that the current code
ignores (it loads the FIFO then spin-waits for it to drain). The TX_DONE fix lets the
hardware do what it was designed for: shift out bytes autonomously while the CPU is free.

The result is **constant-time O(1) ISR cost** regardless of response size — something
the AVR cannot architecturally achieve.

---

*End of document.*
