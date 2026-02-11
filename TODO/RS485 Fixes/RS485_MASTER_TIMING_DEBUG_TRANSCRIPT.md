# RS485 Master Timing Debug Transcript

## Document Purpose

This document is a comprehensive technical analysis of a timing-related interoperability
issue discovered in the ESP32 port of the DCS-BIOS RS485 Master implementation. It is
intended to serve as a persistent knowledge base for future debugging sessions. It contains:
the exact symptoms observed, the hardware configurations tested, a line-by-line comparison
of the AVR (original) and ESP32 (ported) master implementations, the identified root cause,
and proposed solutions.

**Date:** February 2025
**Codebase:**
- AVR Original: `DCS-BIOS/src/internal/DcsBiosNgRS485Master.h` + `.cpp.inc`
- ESP32 Port: `lib/src/internal/RS485Master.cpp` (v2.2)
- ESP32 Config: `lib/src/RS485Config.h`

---

## Table of Contents

1. [Problem Statement](#1-problem-statement)
2. [Hardware Configurations Tested](#2-hardware-configurations-tested)
3. [Test Matrix and Results](#3-test-matrix-and-results)
4. [The RS485 Poll-Response Protocol](#4-the-rs485-poll-response-protocol)
5. [AVR Master: Complete TX-to-RX Transition Analysis](#5-avr-master-complete-tx-to-rx-transition-analysis)
6. [ESP32 Master: Complete TX-to-RX Transition Analysis](#6-esp32-master-complete-tx-to-rx-transition-analysis)
7. [Side-by-Side Timeline Comparison](#7-side-by-side-timeline-comparison)
8. [Root Cause Analysis](#8-root-cause-analysis)
9. [Why Warmup/Cooldown Tuning Cannot Fix This](#9-why-warmupcooldown-tuning-cannot-fix-this)
10. [Secondary Differences Between AVR and ESP32](#10-secondary-differences-between-avr-and-esp32)
11. [Proposed Solutions](#11-proposed-solutions)

---

## 1. Problem Statement

### Symptom

When an ESP32-based RS485 master (running on a Waveshare Industrial CAN-RS485 board with
**manual direction control** via a DE pin) is paired with an ESP32-based RS485 slave (running
on a Lolin S3 Mini with an **auto-direction** TTL/RS485 board), the following behavior is
observed:

- **Outputs (master-to-slave broadcasts) work flawlessly.** The slave receives DCS-BIOS
  export data and updates its local state without any issues.
- **Inputs (slave-to-master responses) almost never work.** The slave's response to a poll
  (containing switch/encoder commands) is lost. The user must press a button many times to
  register even a single input event.

### Why This Matters

The original AVR DCS-BIOS RS485 master (Arduino Mega 2560) works perfectly with **any**
slave device -- AVR, ESP32, auto-direction, manual-direction. The ESP32 master was written
as a direct port of the AVR logic, yet it fails in mixed-hardware configurations. This
means the ESP32 port introduced a fundamental behavioral change that breaks interoperability.

### Key Insight

The fact that outputs always work but inputs almost never work tells us:

- The **transmit path** (master TX) is healthy -- frames are well-formed and correctly received.
- The **receive path** (master RX, catching slave responses after a poll) is broken.
- Specifically, the **TX-to-RX turnaround** -- the transition from "I just finished sending
  a poll" to "I am now listening for the slave's response" -- is where the problem lives.

---

## 2. Hardware Configurations Tested

### Device A: Waveshare Industrial CAN-RS485

- **Platform:** ESP32-S3
- **RS485 transceiver:** Integrated on-board, **manual direction control**
- **DE pin:** GPIO21 (directly controls the RS485 transceiver's Driver Enable)
- **Behavior:** When DE is HIGH, the transceiver drives the bus (TX mode). When DE is LOW,
  the transceiver listens (RX mode). The bus direction is controlled **entirely by firmware**
  via this GPIO pin.
- **Timing sensitivity:** Very sensitive to warmup/cooldown values. Optimal values found
  by trial: ~30-50us for both warmup and cooldown. Deviations cause corrupted data or
  missed packets.

### Device B: Lolin S3 Mini + Auto-Direction TTL/RS485 Board

- **Platform:** ESP32-S3
- **RS485 transceiver:** External auto-direction module
- **DE pin:** Not applicable (set to -1 in config). The board automatically detects data
  direction from the TX line activity.
- **Behavior:** The transceiver switches between TX and RX mode automatically based on
  whether data is being driven on the TX pin. No firmware-controlled DE pin needed.
- **Timing sensitivity:** Very tolerant. Works with warmup/cooldown values of 0, or any
  reasonable value. The auto-direction hardware handles turnaround internally.

### Device C: Arduino Mega 2560 (AVR)

- **Platform:** ATmega2560
- **RS485 transceiver:** External MAX485 with manual DE pin
- **DE pin:** Various (PORTE pin 4/5, PORTG pin 5 for UART1/2/3)
- **Behavior:** DE controlled by firmware via direct port register writes.
- **Timing sensitivity:** None. The AVR implementation works perfectly with any slave type.

---

## 3. Test Matrix and Results

```
+-------------------+------------------------+---------------------+-------------------+
| Master            | Slave                  | Outputs (Broadcast) | Inputs (Poll-Rsp) |
+-------------------+------------------------+---------------------+-------------------+
| Lolin (auto-dir)  | Lolin (auto-dir)       | PERFECT             | PERFECT           |
| Waveshare (DE)    | Waveshare (DE)         | PERFECT             | PERFECT           |
| Waveshare (DE)    | Lolin (auto-dir)       | PERFECT             | ALMOST NEVER      |
| Lolin (auto-dir)  | Waveshare (DE)         | Not tested          | Not tested        |
| AVR Mega (DE)     | Lolin (auto-dir)       | PERFECT             | PERFECT           |
| AVR Mega (DE)     | Waveshare (DE)         | PERFECT             | PERFECT           |
| AVR Mega (DE)     | ESP32 slave (any type) | PERFECT             | PERFECT           |
+-------------------+------------------------+---------------------+-------------------+
```

### Warmup/Cooldown Values Tested (ESP32 Master, Waveshare)

The following combinations were tested for the Waveshare master with a Lolin slave:

| Warmup (us) | Cooldown (us) | Outputs | Inputs | Notes |
|-------------|---------------|---------|--------|-------|
| 50          | 50            | OK      | Fail   | Default config |
| 0           | 0             | Degraded| Fail   | Outputs also suffer without warmup |
| 25          | 25            | OK      | Fail   | |
| 0           | 50            | OK      | Fail   | |
| 50          | 0             | OK      | Fail   | |
| 0           | 10            | OK      | Fail   | |
| 0           | 25            | OK      | Fail   | |
| 25          | 50            | OK      | Fail   | |
| 50          | 25            | OK      | Fail   | |

**Conclusion:** No combination of warmup/cooldown values fixes the input problem.
Some values degrade outputs, but **inputs never work regardless of the values chosen.**
This proves the issue is NOT simply about the warmup/cooldown delay magnitudes. There
is a structural/architectural problem in how the ESP32 master handles the TX-to-RX
transition.

---

## 4. The RS485 Poll-Response Protocol

RS485 is a half-duplex bus. Only one device can transmit at a time. The master controls
the bus by polling slaves sequentially. The protocol for a single poll cycle is:

```
TIME ──────────────────────────────────────────────────────────────────>

     Master TX                    Slave TX
     ─────────                    ────────
     [Addr][0x00][0x00]           [Length][MsgType][Data...][Checksum]
     ←── poll frame ──→ ←─ gap ─→ ←──── response frame ────────────→
                         │
                         └─ TX-to-RX turnaround: master must release
                            the bus and enable its receiver BEFORE
                            the slave begins transmitting.
```

At 250,000 baud (8N1), one byte = 10 bits = **40 microseconds**.

The **critical timing window** is the gap between the master's last poll byte leaving the
wire and the slave's first response byte arriving. During this window, the master must:

1. Detect that its TX is physically complete (last bit has left the shift register)
2. Release the bus (DE pin LOW for manual-direction transceivers)
3. Enable its receiver
4. Start listening for the slave's response

If ANY of these steps are delayed, the slave's response can be:
- **Corrupted** (bus contention -- both master and slave driving simultaneously)
- **Missed entirely** (slave transmits while master's receiver is not yet enabled)
- **Flushed** (response arrives in the RX FIFO but gets discarded by a FIFO reset)

---

## 5. AVR Master: Complete TX-to-RX Transition Analysis

### Source Files

- `DCS-BIOS/src/internal/DcsBiosNgRS485Master.h` (lines 113-168)
- `DCS-BIOS/src/internal/DcsBiosNgRS485Master.cpp.inc` (lines 69-212)

### Key Hardware Functions

The AVR implementation has three critical inline functions that control the UART and DE pin
via direct register manipulation:

```cpp
// DcsBiosNgRS485Master.h, line 137
void set_txen() {
    *ucsrb &= ~((1<<RXEN0) | (1<<RXCIE0));   // Disable RX hardware + RX interrupt
    *txen_port |= txen_pin_mask;               // DE pin HIGH (bus in TX mode)
    *ucsrb |= (1<<TXEN0) | (1<<TXCIE0);       // Enable TX hardware + TXC interrupt
}

// DcsBiosNgRS485Master.h, line 138
void clear_txen() {
    *ucsrb &= ~((1<<TXEN0) | (1<<TXCIE0));    // Disable TX hardware + TXC interrupt
    *txen_port &= ~txen_pin_mask;              // DE pin LOW (bus in RX mode)
    *ucsrb |= (1<<RXEN0) | (1<<RXCIE0);       // Enable RX hardware + RX interrupt
}

// DcsBiosNgRS485Master.h, line 139
void tx_byte(uint8_t c) {
    set_txen();              // Ensure TX mode
    *udr = c;                // Write byte to UART data register
    *ucsra |= (1<<TXC0);    // Clear TXC flag (write 1 to clear)
}
```

### The AVR Poll Cycle: Step by Step

**Step 1: Initiate poll (main loop)**

```cpp
// DcsBiosNgRS485Master.cpp.inc, line 79-86
advancePollAddress();
noInterrupts();                    // Disable global interrupts
tx_byte(poll_address);             // → set_txen() + write byte 1 to UDR
state = POLL_ADDRESS_SENT;
set_udrie();                       // Enable UDRE (Data Register Empty) interrupt
interrupts();                      // Re-enable global interrupts
```

At this point:
- RX hardware is OFF (RXEN0 cleared by `set_txen()`)
- RX interrupt is OFF (RXCIE0 cleared)
- TX hardware is ON (TXEN0 set)
- TXC interrupt is ON (TXCIE0 set)
- DE pin is HIGH (bus in TX mode)
- Byte 1 (slave address) is being shifted out

**Step 2: UDRE ISR fires -- send byte 2 (message type)**

```cpp
// DcsBiosNgRS485Master.cpp.inc, line 118-121
case POLL_ADDRESS_SENT:
    state = POLL_MSGTYPE_SENT;
    tx_byte(0x0);                  // Write byte 2 (MsgType=0) to UDR
    break;
```

The UDRE interrupt fires when UDR is empty and can accept the next byte. The hardware
is double-buffered: UDR acts as a transmit buffer, and the shift register handles the
actual bit-by-bit transmission. Writing to UDR while the shift register is busy means
the next byte will begin transmitting immediately when the current byte finishes.

**Step 3: UDRE ISR fires -- send byte 3 (data length)**

```cpp
// DcsBiosNgRS485Master.cpp.inc, line 123-127
case POLL_MSGTYPE_SENT:
    state = POLL_DATALENGTH_SENT;
    tx_byte(0);                    // Write byte 3 (Length=0) to UDR
    clear_udrie();                 // No more bytes to send -- disable UDRE interrupt
    break;
```

After this, byte 3 is in UDR or the shift register. The UDRE interrupt is disabled
because there are no more bytes to send.

**Step 4: TXC ISR fires -- THE CRITICAL MOMENT**

The TXC (Transmit Complete) interrupt fires when:
- The TX shift register has finished shifting out the **last bit** of the last byte
- The UDR transmit buffer is empty (no more bytes pending)

This is a **hardware guarantee** that all data has physically left the chip. The TXC
interrupt is the earliest possible moment the firmware can know that transmission is
complete.

```cpp
// DcsBiosNgRS485Master.cpp.inc, line 159-163
case POLL_DATALENGTH_SENT:
    rx_start_time = micros();      // Start RX timeout timer
    state = RX_WAIT_DATALENGTH;    // Transition to RX state
    clear_txen();                  // ← THIS IS THE CRITICAL CALL
    break;
```

**What `clear_txen()` does in a single uninterruptible sequence:**

```
STEP  REGISTER OPERATION              EFFECT
────  ──────────────────────────────  ──────────────────────────────────
 1    UCSRB &= ~(TXEN0 | TXCIE0)     TX hardware disabled, TXC ISR disabled
 2    TXEN_PORT &= ~pin_mask          DE pin goes LOW → transceiver switches to RX
 3    UCSRB |= (RXEN0 | RXCIE0)      RX hardware enabled, RX ISR enabled
```

Because this happens inside an ISR (interrupts are already disabled by hardware),
these three register writes execute as an **atomic sequence**. There is no possibility
of another interrupt preempting between steps 1, 2, and 3.

**The result:** At the exact clock cycle when the last bit of the poll frame leaves
the shift register, the AVR:
- Releases the bus (DE LOW)
- Enables its receiver
- Is ready to catch the slave's response

The total latency from "last bit on wire" to "receiver ready" is approximately
**3-5 AVR clock cycles** (at 16 MHz = ~200-300 nanoseconds). This is effectively
instantaneous for RS485 purposes.

**Step 5: RX ISR fires when slave responds**

```cpp
// DcsBiosNgRS485Master.cpp.inc, line 180-188
case RX_WAIT_DATALENGTH:
    rxtx_len = data;
    slave_present[poll_address] = true;
    if (rxtx_len > 0) {
        state = RX_WAIT_MSGTYPE;
    } else {
        state = IDLE;              // Empty response (slave had no data)
    }
    break;
```

Note: `rx_start_time` is NOT updated here. The AVR's mid-message timeout (5ms) runs
from the TXC ISR, not from the last received byte.

---

## 6. ESP32 Master: Complete TX-to-RX Transition Analysis

### Source Files

- `lib/src/internal/RS485Master.cpp` (lines 516-547, 628-640)
- `lib/src/RS485Config.h`

### The ESP32 Poll Cycle: Step by Step

**Step 1: Initiate poll (poll loop)**

```cpp
// RS485Master.cpp, line 991-993
uint8_t nextAddr = advancePollAddress();
sendPoll(nextAddr);
lastPollUs = now;
```

**Step 2: sendPoll() -- the entry point**

```cpp
// RS485Master.cpp, line 628-640
static void sendPoll(uint8_t addr) {
    uint8_t frame[3] = { addr, MSGTYPE_DCSBIOS, 0 };
    currentPollAddr = addr;
    txWithEchoPrevention(frame, 3);        // ← ALL TX happens here (BLOCKING)
    rxStartTime = esp_timer_get_time();    // Start timeout AFTER tx returns
    state = MasterState::RX_WAIT_DATALENGTH;
    statPolls++;
}
```

**Step 3: txWithEchoPrevention() -- THE CRITICAL FUNCTION**

This is where the ESP32 implementation fundamentally diverges from the AVR. Here is
the function with detailed annotations:

```cpp
// RS485Master.cpp, line 516-547
static void txWithEchoPrevention(const uint8_t* data, size_t len) {

    // ──── PHASE 1: PREPARE FOR TX ────────────────────────────────
    disableRxInt();                          // [A] Disable RX FIFO interrupt
                                             //     (UART RX hardware stays ON!)

    deAssert();                              // [B] DE pin HIGH → bus in TX mode

    ets_delay_us(RS485_TX_WARMUP_DELAY_US);  // [C] *** BLOCKING 50us WARMUP ***
                                             //     CPU spins in a busy-wait loop.
                                             //     Nothing else can happen.
                                             //     Purpose: let transceiver settle
                                             //     before data appears on bus.

    // ──── PHASE 2: TRANSMIT DATA ─────────────────────────────────
    txBytes(data, len);                      // [D] Write all 3 bytes to TX FIFO
                                             //     Each byte: spin-wait for FIFO
                                             //     space, then write. Blocking.

    while (!txIdle()) {}                     // [E] *** BLOCKING WAIT ***
                                             //     Spin-wait until the UART's TX
                                             //     shift register is empty (all
                                             //     bits have physically left).
                                             //     Equivalent to AVR's TXC flag.

    // ──── PHASE 3: TRANSITION TO RX ─────────────────────────────
    //
    // AT THIS POINT: The last bit has left the shift register.
    // The equivalent AVR moment is when the TXC ISR fires.
    // The AVR does clear_txen() HERE -- atomically releasing the
    // bus and enabling the receiver. But the ESP32 does NOT.
    // Instead, it does this:
    //

    ets_delay_us(RS485_TX_COOLDOWN_DELAY_US); // [F] *** BLOCKING 50us COOLDOWN ***
                                              //     DE pin is STILL HIGH.
                                              //     Bus is STILL in TX mode.
                                              //     Transceiver is STILL driving.
                                              //     For 50 MICROSECONDS.

    flushRxFifo();                            // [G] Reset the RX FIFO.
                                              //     Discards ALL bytes currently
                                              //     in the receive FIFO.
                                              //     Purpose: discard echo bytes
                                              //     (ESP32 UART RX hardware was
                                              //     ON this whole time, capturing
                                              //     the TX bytes as echo).
                                              //     SIDE EFFECT: Also discards any
                                              //     legitimate slave response bytes
                                              //     that arrived in the FIFO!

    deDeassert();                             // [H] DE pin LOW → bus released.
                                              //     Transceiver switches to RX.
                                              //     But RX interrupt is still OFF.

    enableRxInt();                            // [I] Clear pending RX interrupt flags
                                              //     and re-enable the RX FIFO
                                              //     interrupt. NOW the master can
                                              //     receive bytes.
}
```

**The total time from "last bit on wire" (step E) to "receiver ready" (step I):**

```
Step [F]:  50us cooldown delay (ets_delay_us)
Step [G]:  ~1-2us FIFO flush (register write)
Step [H]:  ~0.1us DE pin toggle (GPIO register write)
Step [I]:  ~0.5us interrupt mask operations

TOTAL:     ~52us of dead time where the master CANNOT receive
```

Compare to AVR: ~0.2us (3-5 clock cycles).

**The ESP32 is ~250x slower at the TX-to-RX turnaround than the AVR.**

---

## 7. Side-by-Side Timeline Comparison

This diagram shows a complete poll cycle with precise timing. At 250,000 baud, one byte
(10 bits: start + 8 data + stop) takes exactly **40 microseconds**.

```
TIME (microseconds)        AVR MASTER                    ESP32 MASTER
═══════════════════════════════════════════════════════════════════════

t=0                        noInterrupts()                disableRxInt()
                           set_txen():                   deAssert() [DE HIGH]
                             RX OFF, DE HIGH, TX ON      ets_delay_us(50) ← WARMUP

t=0 to t=50                                              ... CPU spinning ...

t=50 (AVR t=0)             Byte 1 enters shift register  txByte(frame[0]) ← addr
                           (via UDR write in tx_byte)     txByte(frame[1]) ← msgtype
                                                          txByte(frame[2]) ← length
                                                          (all written to TX FIFO)

t=50 to t=90               Byte 1 on wire (40us)         Byte 1 on wire (40us)
(AVR t=0 to t=40)

t=90 to t=130              Byte 2 on wire (UDRE ISR      Byte 2 on wire (from FIFO,
(AVR t=40 to t=80)         loads next byte)              automatic hardware shift)

t=130 to t=170             Byte 3 on wire (UDRE ISR      Byte 3 on wire (from FIFO,
(AVR t=80 to t=120)        loads last byte, disables     automatic hardware shift)
                           UDRE)

t=170                      *** LAST BIT LEAVES WIRE ***  *** txIdle() returns true ***
(AVR t=120)

                    ┌──── AVR: TXC ISR FIRES ─────────   ESP32: begins cooldown...
                    │
                    │  IMMEDIATELY in the ISR:
                    │    rx_start_time = micros()         ets_delay_us(50) ← COOLDOWN
                    │    state = RX_WAIT_DATALENGTH       DE pin STILL HIGH!
                    │    clear_txen():                    Bus STILL in TX mode!
                    │      TX OFF                         Transceiver STILL driving!
                    │      DE LOW ← bus released
                    │      RX ON ← receiver ready
                    │
                    │  *** AVR RECEIVER IS READY ***

t=170.2             │  AVR: listening for slave          ESP32: still in cooldown
(AVR t=120.2)       │       response                    (DE HIGH, not listening)

                    │  <-- AVR gap: ~0.2us -->

t=220                                                    flushRxFifo() ← FIFO RESET
(AVR t=170)                                              deDeassert() [DE LOW]
                                                         enableRxInt()
                                                         rxStartTime = now
                                                         state = RX_WAIT_DATALENGTH

                                                         *** ESP32 RECEIVER IS READY ***

                                                         <-- ESP32 gap: ~52us -->
═══════════════════════════════════════════════════════════════════════

THE GAP: From "last bit on wire" to "receiver ready":
  AVR:    ~0.2 microseconds (effectively zero)
  ESP32:  ~52 microseconds (enough to miss 1.3 complete bytes at 250kbaud)
```

---

## 8. Root Cause Analysis

### The Failure Scenario

Here is exactly what happens when the ESP32 master (Waveshare, manual DE) polls an
auto-direction slave (Lolin):

```
t=170us   Master's last poll byte finishes transmitting.
          txIdle() returns true.

t=170us   ESP32 begins ets_delay_us(50) cooldown.
          DE pin is STILL HIGH.
          The Waveshare's RS485 transceiver is STILL driving the bus.

t=180us   The Lolin slave (auto-direction) has received the complete
          3-byte poll frame. Its ISR immediately processes the frame
          and enters sendResponseISR().

t=185us   The Lolin slave's auto-direction transceiver switches to TX
          and begins sending the response (e.g., a single 0x00 byte
          meaning "no data").

t=185us   *** BUS CONTENTION ***
to        The master's transceiver is driving the bus (DE HIGH) while
t=220us   the slave's transceiver is also driving the bus.
          The bus voltage is indeterminate. The slave's response is
          corrupted or lost.

t=220us   Master finally deasserts DE (bus released).
          Master flushes RX FIFO (discarding anything captured).
          Master enables RX interrupt.
          Master starts listening.

t=220us+  The slave's response byte has ALREADY been transmitted (it
          only took 40us: t=185 to t=225). If it was corrupted by
          bus contention, the master receives garbage or nothing.
          If the master's flushRxFifo() happened to discard it, it's gone.
          Either way: the response is lost.
```

### Why Auto-Direction Slaves Are Particularly Affected

An auto-direction RS485 module does not wait for a DE signal -- it automatically switches
to TX mode the instant it sees data on its TX input line. This means the slave's transceiver
begins driving the bus within nanoseconds of the slave's UART starting to shift out the
first response byte. There is no "negotiation" or "wait period."

A manual-direction slave (like another Waveshare) has its own DE pin control with warmup
delays. This adds a delay before the slave starts driving the bus, which may coincidentally
be enough time for the master's cooldown to finish. That's why two Waveshare devices work
together -- the slave's warmup delay (50us) effectively waits out the master's cooldown.

### Why the AVR Master Works With Everything

The AVR releases the bus within ~200 nanoseconds of the last bit leaving the wire. Even the
fastest auto-direction slave takes at least a few microseconds to process the poll in its ISR
and begin transmitting. The AVR is always listening before the slave starts responding. There
is **never** bus contention.

### The Fundamental Architectural Flaw

The ESP32 port replaced the AVR's **interrupt-driven, zero-delay TX-to-RX transition** with
a **blocking, sequential procedure** that includes artificial delays. Specifically:

1. **The cooldown delay happens WHILE DE is still asserted.** In the AVR, DE is deasserted
   at the exact moment TX completes. There is no concept of a "cooldown" because there is no
   need for one -- the TXC interrupt IS the hardware's signal that it's safe to release.

2. **The RX FIFO flush happens AFTER the cooldown but BEFORE DE deassert.** This means any
   bytes that were somehow captured during the cooldown (even partially) are discarded.
   Worse, if DE is released before the flush due to code reordering, early response bytes
   get flushed too.

3. **The RX interrupt is re-enabled AFTER everything else.** There is a cascading sequence
   of operations (cooldown → flush → DE release → interrupt enable) where each step adds
   latency and each gap is a window for data loss.

---

## 9. Why Warmup/Cooldown Tuning Cannot Fix This

The user tested every reasonable combination of warmup and cooldown values. None fixed
the input issue. Here is why:

### Reducing cooldown to 0 does not help

Even with `RS485_TX_COOLDOWN_DELAY_US = 0`, the ESP32 still executes:

```cpp
// With cooldown = 0, this code still runs:
flushRxFifo();          // Still flushes the FIFO
deDeassert();           // Still deasserts DE
enableRxInt();          // Still re-enables interrupts
```

These operations take ~2-5 microseconds. During that time, DE is still HIGH (the
`flushRxFifo()` call happens before `deDeassert()`). A fast slave can begin responding
within 5-10 microseconds of seeing the complete poll frame. This creates a narrow but
real bus contention window.

More importantly, the `flushRxFifo()` call will discard any bytes that arrived during
the `while (!txIdle()) {}` spin or the brief period after. The UART RX hardware was
never disabled (only the interrupt was disabled), so it captures echo bytes AND any
early response bytes indiscriminately.

### Increasing cooldown makes it worse

A longer cooldown gives the slave MORE time to start responding while DE is still HIGH,
making bus contention more likely and longer in duration.

### The ordering is wrong, not the magnitude

The problem is **structural**: the ESP32 code does cooldown-then-flush-then-release-then-listen.
It needs to do release-then-listen AT the moment TX completes, with NO intervening delays.
No amount of delay tuning can fix an incorrect ordering of operations.

---

## 10. Secondary Differences Between AVR and ESP32

Beyond the critical TX-to-RX transition issue, several other behavioral differences exist:

### 10.1 Blocking TX vs. Interrupt-Driven TX

**AVR:** The three poll bytes are sent via the UDRE interrupt chain. The main loop writes
byte 1 and enables UDRE. UDRE ISR fires to load byte 2, then byte 3. Between bytes, the
main loop can check timeouts and perform other work.

**ESP32:** All three bytes are written to the TX FIFO in a tight loop (`txBytes()`), then
the CPU spin-waits for `txIdle()`. The calling context (FreeRTOS task or main loop) is
completely blocked for ~120us (3 bytes at 40us each) plus warmup/cooldown.

**Impact:** On the ESP32, nothing else can happen during TX. On the AVR, the CPU is free
between bytes. This doesn't directly cause the input issue but reduces overall throughput.

### 10.2 RX Hardware Enable vs. RX Interrupt Disable

**AVR's `set_txen()`:** Disables `RXEN0` (the UART receiver hardware itself) and `RXCIE0`
(the RX interrupt). The receiver is **physically turned off.** No bytes are captured during
TX. There are no echo bytes to deal with.

**ESP32's `disableRxInt()`:** Disables only the RX FIFO interrupt mask. The UART RX hardware
continues running and captures bytes into the FIFO. During TX, the FIFO fills with echo copies
of the transmitted bytes. This is why the ESP32 needs `flushRxFifo()` -- to discard these echo
bytes.

**Impact:** The AVR never needs to flush because it never captures echoes. The ESP32's flush
is necessary but introduces the risk of discarding legitimate response bytes that arrive
during the flush window. The ESP32 UART does not support disabling the RX hardware (RXEN
equivalent) independently of the UART module itself; the RX pin is always active when the
UART is enabled.

### 10.3 Timeout Reset Behavior

**AVR:** `rx_start_time` is set once in the TXC ISR and **never updated** during the RX
state machine. The 5ms mid-message timeout runs from TX completion.

```cpp
// DcsBiosNgRS485Master.cpp.inc, line 160
rx_start_time = micros();    // Set in TXC ISR, never touched again
```

**ESP32:** `rxStartTime` is reset on **every** received byte:

```cpp
// RS485Master.cpp, line 418, 425, 433, 441
rxStartTime = esp_timer_get_time();    // Reset on every byte
```

**Impact:** The ESP32 is more forgiving of slow slaves (each byte resets the 5ms clock).
The AVR enforces a strict 5ms total from TX completion. This is a behavioral difference
but not a cause of the input issue.

### 10.4 Immediate Forwarding to PC

**AVR:** Each received data byte in `RX_WAIT_DATA` triggers `uart0.set_udrie()`, which
immediately starts draining the message buffer to the PC via UART0. Data flows to the PC
byte-by-byte as it arrives from the slave.

**ESP32:** Data bytes accumulate in `messageBuffer`. Only after `messageBuffer.complete` is
set (checksum received) and on the NEXT iteration of `rs485PollLoop()` is
`processCompletedMessage()` called. This adds latency to the PC-side update but does not
affect the RS485 bus protocol.

### 10.5 TXC Interrupt vs. Polling Loop

**AVR:** Uses the hardware TXC interrupt to detect TX completion. The ISR fires at the exact
clock cycle the last bit leaves the shift register. Zero polling latency.

**ESP32:** Uses `while (!txIdle()) {}` to poll the UART status register. The loop iterates
at CPU speed (240MHz on ESP32-S3), so the detection latency is typically < 100 nanoseconds.
This is not a significant difference, but it does mean the ESP32 detects TX completion in
a non-ISR context, allowing subsequent operations to be interrupted by higher-priority ISRs
(a minor concern for timing-critical code).

---

## 11. Proposed Solutions

### Solution A: ESP32 TXC Interrupt-Driven Transition (Replicates AVR Exactly)

**Concept:** Use the ESP32 UART's TX-done interrupt (`UART_INTR_TX_DONE`) to replicate the
AVR's TXC ISR behavior. When the last byte finishes transmitting, an ISR fires and
immediately deasserts DE and enables RX -- exactly like the AVR.

**How it works:**

The ESP32 UART hardware has a `UART_INTR_TX_DONE` interrupt that fires when the TX FIFO
is empty and the shift register has finished. This is functionally identical to the AVR's
TXC interrupt.

```
Implementation sketch:
1. Write all poll bytes to TX FIFO
2. Enable UART_INTR_TX_DONE interrupt
3. In the TX_DONE ISR:
   a. Flush RX FIFO (discard echo bytes captured during TX)
   b. DE pin LOW (release bus) — use gpio_ll_set_level for speed
   c. Clear and enable RX FIFO interrupt
   d. Set rxStartTime
   e. Set state = RX_WAIT_DATALENGTH
4. Return from ISR — receiver is now active
```

**Advantages:**
- Matches AVR behavior exactly
- Near-zero latency from TX completion to RX ready (~0.5us for ISR entry + register ops)
- No blocking delays in the critical path
- Warmup delay can still be used before TX starts (this is fine -- it's before data goes out)
- Eliminates the cooldown delay entirely
- No bus contention possible (bus is released the instant TX finishes)

**Concerns:**
- Still needs to handle echo bytes in the FIFO. The flush inside the ISR must happen BEFORE
  enabling RX interrupts. Since the UART RX hardware is always active on ESP32, echo bytes
  accumulate during TX and must be cleared. But this flush happens while DE is still HIGH
  (the ISR fires when the last bit finishes, and DE hasn't been released yet), so no
  legitimate slave response bytes can be in the FIFO yet. This is the key insight: **the flush
  is safe inside the TX_DONE ISR because the bus is still in TX mode at that instant.**

**Estimated turnaround time:** ~1-2 microseconds (ISR entry latency on ESP32).

### Solution B: Reorder the Transition Sequence (Minimal Change)

**Concept:** Without adding a new interrupt, reorder the operations inside
`txWithEchoPrevention()` to eliminate the cooldown delay and move DE deassert to
immediately after `txIdle()`.

```
Current (broken) order:         Proposed order:
  txIdle() detected               txIdle() detected
  cooldown delay (50us)            flushRxFifo()      ← echo bytes only (DE still HIGH)
  flushRxFifo()                    deDeassert()        ← bus released IMMEDIATELY
  deDeassert()                     enableRxInt()       ← receiver ready
  enableRxInt()                    (no cooldown needed)
```

**How it works:**

```
1. After txIdle() returns true, the shift register is empty. DE is still HIGH.
2. Flush the RX FIFO immediately. Since DE is still HIGH, the transceiver is in TX
   mode. No external bytes can enter the RX pin. The FIFO contains ONLY echo bytes.
   This flush is safe.
3. Deassert DE (bus released). The transceiver switches to RX.
4. Enable RX interrupts. The receiver is now active.
5. No cooldown delay is needed because:
   - The transceiver does not need time to "cool down" after TX. The last bit is already
     out. The transceiver can switch direction instantly.
   - The only reason for a cooldown would be if the transceiver needs time to release the
     bus after DE goes low. But DE goes low in step 3, and the transceiver's release time
     is specified by the hardware (typically < 1us for MAX485-type transceivers).
```

**Advantages:**
- Minimal code change (reorder lines, remove cooldown delay)
- No new interrupt handler needed
- Still handles echo bytes correctly
- Turnaround time reduced from ~52us to ~2-3us (FIFO flush + GPIO write + interrupt enable)

**Concerns:**
- Still blocking/polling (not interrupt-driven like Solution A)
- The turnaround is not as fast as Solution A (~2-3us vs ~1us) because the code must poll
  `txIdle()`, then execute the transition sequence in normal context (not ISR), which could
  be preempted by a higher-priority interrupt

### Solution C: Use the ESP32 UART's Built-In RS485 Hardware Mode

**Concept:** The ESP32 UART peripheral has a built-in RS485 mode that can automatically
control the DE pin based on TX activity. This would delegate the entire TX/RX turnaround
to hardware, eliminating all software timing concerns.

**How it works:**

The ESP32 UART has an `RS485` mode where:
- DE is automatically asserted when the transmitter is active
- DE is automatically deasserted when the transmitter goes idle
- An optional turnaround delay (in bit-periods) can be configured

This is configured via `uart_set_mode(UART_NUM, UART_MODE_RS485_HALF_DUPLEX)` and
related `uart_ll_*` functions.

**Advantages:**
- Zero software involvement in the TX/RX transition
- Hardware-guaranteed timing (no ISR latency, no preemption concerns)
- Eliminates all echo prevention code (hardware handles it)
- Potentially sub-microsecond turnaround

**Concerns:**
- Requires investigation of whether the ESP32's RS485 hardware mode properly handles
  DE timing for the DCS-BIOS protocol
- May not be available on all ESP32 variants or may have different behavior
- Less control over the exact DE pin timing (hardware-managed)
- The UART's RS485 mode may add its own turnaround delay that needs to be measured
- Requires testing to verify compatibility with the protocol's frame structure

### Solution D: Hybrid Approach (Recommended)

**Concept:** Combine the best aspects of Solutions A and B:

1. **For manual-DE devices:** Use `UART_INTR_TX_DONE` interrupt to achieve near-zero
   turnaround. This exactly matches the AVR's behavior and guarantees interoperability
   with all slave types.

2. **For auto-direction devices:** Keep the current approach but with a reordered sequence
   (Solution B) and minimal/zero delays. Auto-direction devices are already tolerant of
   timing variations.

3. **Warmup delay preserved:** The warmup delay (before TX starts) is still useful for
   manual-DE transceivers to ensure the bus is stable before data appears. This delay
   is harmless -- it happens before any data is on the wire.

4. **Cooldown delay eliminated:** The cooldown delay is never needed. The transceiver
   switches direction in hardware within nanoseconds. The "cooldown" concept was a
   misunderstanding of what the transceiver needs.

**Implementation approach:**

```
Poll transmission flow (hybrid):

  [Normal context]
    disableRxInt()
    deAssert()                    // DE HIGH
    ets_delay_us(warmup)          // Optional: let transceiver settle
    txBytes(frame, len)           // Write all bytes to FIFO
    enable UART_INTR_TX_DONE      // Arm the TX-complete interrupt

  [TX_DONE ISR fires when last bit exits]
    flushRxFifo()                 // Safe: DE still HIGH, only echoes in FIFO
    deDeassert()                  // DE LOW: bus released
    enableRxInt()                 // RX active immediately
    rxStartTime = now             // Start timeout
    state = RX_WAIT_DATALENGTH    // Ready for slave response
```

**Advantages:**
- AVR-equivalent turnaround (~1us) for manual-DE devices
- Works perfectly with all slave types (auto-direction, manual-direction, AVR, ESP32)
- Warmup still protects against bus instability at TX start
- No unnecessary delays in the critical RX-ready path
- Clean separation: TX initiation in normal context, TX completion in ISR
- The main loop / FreeRTOS task is free during TX (not blocked by spin-wait)

**This is the recommended approach because it:**
- Eliminates the root cause (delayed DE deassert after TX)
- Matches the proven AVR architecture (interrupt-driven TX completion)
- Preserves useful features (warmup delay, echo prevention)
- Does not require the ESP32's RS485 hardware mode (which adds unknowns)
- Can be conditionally compiled: use TX_DONE ISR for DE-pin devices, keep the simpler
  blocking approach for auto-direction devices where timing doesn't matter

### Solution E: Full Non-Blocking ISR Architecture (Maximum Performance)

**Concept:** Go beyond just fixing the turnaround and make the entire TX path non-blocking,
exactly like the AVR's UDRE + TXC interrupt chain. This would make the ESP32 master
strictly superior to the AVR in every measurable way.

**How it works:**

```
Interrupts used:
  UART_INTR_TXFIFO_EMPTY  ← Equivalent to AVR's UDRE (Data Register Empty)
  UART_INTR_TX_DONE       ← Equivalent to AVR's TXC (Transmit Complete)
  UART_INTR_RXFIFO_FULL   ← Already used for RX

TX flow:
  1. Normal context: Load first byte(s) into TX FIFO
  2. TXFIFO_EMPTY ISR: Load next byte(s) from a tx buffer
  3. When last byte loaded: Disable TXFIFO_EMPTY, wait for TX_DONE
  4. TX_DONE ISR: Flush RX FIFO, deassert DE, enable RX interrupt

RX flow (unchanged):
  5. RXFIFO_FULL ISR: Process byte through state machine (processRxByte)
```

**Advantages:**
- CPU is NEVER blocked during TX -- free to process other tasks
- Near-zero TX-to-RX turnaround via TX_DONE ISR
- Matches and exceeds AVR performance:
  - AVR: CPU is free between bytes (UDRE-driven) but blocked during byte shift
  - ESP32: CPU is ALWAYS free (FIFO handles byte buffering)
- Can interleave broadcast preparation with active TX
- Maximum bus utilization: no wasted CPU cycles
- The ESP32's TX FIFO (128 bytes) can hold an entire broadcast frame, meaning a single
  FIFO load can trigger a complete broadcast without any ISR involvement during TX

**Concerns:**
- More complex implementation (two new ISR handlers for TX)
- Requires careful state machine management across TX and RX ISRs
- The TX_DONE interrupt behavior may vary slightly between ESP32 variants
- Overkill for the immediate problem (the simpler Solution D fixes the root cause)

**Recommended as a Phase 2 optimization** after Solution D has been validated.

---

## Summary of Findings

| Aspect | AVR Master | ESP32 Master | Impact |
|--------|-----------|-------------|--------|
| TX completion detection | TXC hardware interrupt | `while (!txIdle()) {}` poll | Minor (~0.1us difference) |
| TX-to-RX turnaround | ~0.2us (atomic in ISR) | ~52us (blocking sequential) | **ROOT CAUSE** |
| DE deassert timing | Instant at TX complete | 50us AFTER TX complete | **Bus contention** |
| RX FIFO echo handling | Not needed (RX OFF) | Flush after TX (may lose data) | **Data loss risk** |
| RX enable timing | Atomic with DE deassert | Separate step, after flush | Additional delay |
| Blocking during TX | No (ISR-driven) | Yes (entire function blocks) | Throughput loss |
| Warmup delay | None (not needed) | 50us before TX | OK (not harmful) |
| Cooldown delay | None (not needed) | 50us after TX | **Harmful** |

**Root Cause:** The ESP32 master holds the DE pin HIGH (bus in TX mode) for ~50 microseconds
after the last byte finishes transmitting. During this window, fast-responding slaves
(especially auto-direction devices) begin transmitting their response onto a bus that is
still being driven by the master. This causes bus contention and data corruption/loss.

**Fix:** Eliminate the cooldown delay and deassert DE immediately upon TX completion, ideally
via a TX_DONE hardware interrupt (Solution D) to match the AVR's proven zero-latency approach.

---

## Appendix A: Final Hardware Audit — Confirming Solution Feasibility

### UART_INTR_TX_DONE: Verified Available on All ESP32 Variants

A direct inspection of the ESP-IDF HAL headers on this machine confirmed that
`UART_INTR_TX_DONE` (bit 14 of the UART interrupt status register) is defined
identically across all ESP32 chip variants:

| Chip        | Header Location                                                    | Value          |
|-------------|---------------------------------------------------------------------|---------------|
| ESP32       | `.../esp32/include/hal/uart_ll.h`                                  | `(0x1 << 14)` |
| ESP32-S3    | `.../esp32s3/include/hal/uart_ll.h`                                | `(0x1 << 14)` |
| ESP32-C3    | `.../esp32c3/include/hal/uart_ll.h`                                | `(0x1 << 14)` |
| ESP32-C6    | `.../esp32c6/include/hal/uart_ll.h`                                | `(0x1 << 14)` |
| ESP32-H2    | `.../esp32h2/include/hal/uart_ll.h`                                | `(0x1 << 14)` |
| ESP32-S2    | `.../esp32s2/include/hal/uart_ll.h`                                | `(0x1 << 14)` |

This interrupt can be enabled, disabled, and cleared using the same `uart_ll_*`
functions already used in the codebase for the RX interrupt:

```cpp
uart_ll_ena_intr_mask(hw, UART_INTR_TX_DONE);    // Enable
uart_ll_disable_intr_mask(hw, UART_INTR_TX_DONE); // Disable
uart_ll_clr_intsts_mask(hw, UART_INTR_TX_DONE);   // Clear pending
```

These are all `FORCE_INLINE_ATTR` single-register-write functions. They execute in
one CPU cycle. No function call overhead.

### uart_ll_is_tx_idle() — Confirmed Implementation

```cpp
// ESP32-S3 uart_ll.h, line 886-889
FORCE_INLINE_ATTR bool uart_ll_is_tx_idle(uart_dev_t *hw)
{
    return ((hw->status.txfifo_cnt == 0) && (hw->fsm_status.st_utx_out == 0));
}
```

This checks two conditions:
1. TX FIFO count is zero (no bytes pending)
2. TX FSM state is 0 (IDLE — the shift register is not active)

**`UART_INTR_TX_DONE` fires when BOTH of these conditions become true.** This is the
hardware equivalent of the AVR's TXC interrupt.

### What Happens Inside the ISR

Since the codebase already uses `esp_intr_alloc()` with `ESP_INTR_FLAG_IRAM` for the
RX interrupt, the same interrupt source can handle TX_DONE. The ESP32 UART has a single
interrupt source per UART peripheral — the ISR receives ALL enabled UART interrupts.
The existing `rxISR()` function already checks the interrupt status bits to determine
which event triggered it. Adding TX_DONE handling is a matter of checking one more bit:

```
Existing ISR flow:
  1. Check UART_INTR_RXFIFO_FULL → process RX bytes
  2. Clear RXFIFO_FULL flag

Extended ISR flow:
  1. Check UART_INTR_RXFIFO_FULL → process RX bytes
  2. Check UART_INTR_TX_DONE → do TX-complete transition
  3. Clear all handled flags
```

Since RX and TX never overlap in the master (it waits for a response before sending
the next poll), there is no conflict. The ISR handles whichever event fired.

### Why This Makes the ESP32 As Good Or Better Than the AVR

With the TX_DONE interrupt approach, the ESP32 master would have:

1. **TX-to-RX turnaround: ~1us** (ISR entry latency on ESP32-S3 at 240MHz).
   The AVR achieves ~0.2us at 16MHz. Both are far below the 40us byte time at
   250kbaud, so both are effectively instantaneous for protocol purposes.

2. **TX FIFO advantage:** The ESP32 has a 128-byte TX FIFO. An entire poll frame (3 bytes)
   or even a full broadcast frame (up to ~260 bytes) can be loaded into the FIFO in one
   burst. The hardware shifts out bytes automatically. The CPU is free immediately after
   the FIFO load. The AVR has no FIFO — it must load one byte at a time via the UDRE ISR.

3. **RX FIFO advantage:** The ESP32 has a 128-byte RX FIFO. Even if the ISR is slightly
   delayed (e.g., by another interrupt), incoming bytes accumulate safely in the FIFO.
   The AVR has a single-byte receive buffer — if the ISR is delayed by even one byte
   time (40us), data is lost (overrun).

4. **Non-blocking TX:** By loading the TX FIFO and arming the TX_DONE interrupt, the CPU
   is free during the entire transmission. The AVR blocks the CPU between UDRE ISR
   invocations (it must service each byte individually). The ESP32 can process broadcast
   data preparation, check timeouts, or handle other tasks during TX.

5. **Deterministic timing:** The TX_DONE ISR fires at a hardware-guaranteed moment. Unlike
   the current `while (!txIdle()) {}` polling, which can be delayed by FreeRTOS context
   switches or other interrupts, the ISR is triggered by the hardware and executes within
   the ISR latency window (typically < 1us on ESP32-S3).

### Recommended Implementation Plan (Solution D — Confirmed Feasible)

Based on the hardware audit, the recommended implementation is:

```
PHASE 1: Fix the root cause
═══════════════════════════

1. Modify the shared UART ISR (rxISR) to also handle UART_INTR_TX_DONE:

   static void IRAM_ATTR rxISR(void* arg) {
       uint32_t status = uart_ll_get_intsts_mask(RS485_UART_DEV);

       // Handle TX_DONE first (higher priority — bus turnaround)
       if (status & UART_INTR_TX_DONE) {
           flushRxFifo();                    // Discard echo bytes (DE still HIGH)
           deDeassert();                     // DE LOW — release bus
           enableRxInt();                    // RX active immediately
           rxStartTime = esp_timer_get_time();
           state = MasterState::RX_WAIT_DATALENGTH;
           uart_ll_disable_intr_mask(RS485_UART_DEV, UART_INTR_TX_DONE);
           uart_ll_clr_intsts_mask(RS485_UART_DEV, UART_INTR_TX_DONE);
       }

       // Handle RX bytes (existing logic, unchanged)
       if (status & (UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT)) {
           while (uart_ll_get_rxfifo_len(RS485_UART_DEV) > 0) {
               uint8_t c;
               uart_ll_read_rxfifo(RS485_UART_DEV, &c, 1);
               RS485_RISCV_FENCE();
               processRxByte(c);
           }
           uart_ll_clr_intsts_mask(RS485_UART_DEV,
               UART_INTR_RXFIFO_FULL | UART_INTR_RXFIFO_TOUT);
       }
   }

2. Modify sendPoll() to be non-blocking:

   static void sendPoll(uint8_t addr) {
       uint8_t frame[3] = { addr, MSGTYPE_DCSBIOS, 0 };
       currentPollAddr = addr;

       disableRxInt();
       deAssert();                                  // DE HIGH
       #if RS485_TX_WARMUP_DELAY_US > 0
       ets_delay_us(RS485_TX_WARMUP_DELAY_US);      // Optional warmup
       #endif
       txBytes(frame, 3);                           // Load TX FIFO

       // Arm TX_DONE interrupt — ISR will handle the turnaround
       uart_ll_clr_intsts_mask(RS485_UART_DEV, UART_INTR_TX_DONE);
       uart_ll_ena_intr_mask(RS485_UART_DEV, UART_INTR_TX_DONE);

       // State set to a TRANSMITTING state; ISR will transition to RX_WAIT
       // Or: keep state as IDLE and let the ISR set RX_WAIT_DATALENGTH
       statPolls++;
   }

3. Same pattern for sendBroadcast() and sendTimeoutZero().

4. The cooldown delay is COMPLETELY ELIMINATED. It was never needed —
   the transceiver switches direction in hardware within nanoseconds of
   DE going LOW. The "cooldown" was a cargo-cult addition that actively
   harmed the protocol.

5. The warmup delay is PRESERVED but ONLY for DE-pin devices. It ensures
   the transceiver is stable before data appears. For auto-direction
   devices, the warmup can be reduced to 0-15us or eliminated.


PHASE 2: Performance optimization (future)
══════════════════════════════════════════

6. Make sendPoll() fully non-blocking: after loading the TX FIFO and arming
   the ISR, return immediately. The calling context can do other work.
   The ISR handles the turnaround when TX completes.

7. For broadcasts: load the entire frame into the 128-byte TX FIFO in one
   shot (if it fits). Arm TX_DONE. Return. The hardware shifts out all
   bytes automatically. No CPU involvement during transmission.

8. This makes the ESP32 strictly superior to the AVR:
   - Faster TX-to-RX turnaround (ISR-driven, < 1us)
   - CPU free during TX (FIFO handles byte shifting)
   - RX FIFO prevents data loss during ISR latency
   - Higher bus throughput (less dead time per cycle)
```

### Risk Assessment

| Risk | Likelihood | Mitigation |
|------|-----------|------------|
| TX_DONE fires too early (before last bit) | Very low — hardware guarantees | Verify with oscilloscope |
| TX_DONE and RXFIFO_FULL fire simultaneously | Low — master never receives during TX | ISR handles both, TX_DONE first |
| ISR latency > 40us (one byte time) | Very low at 240MHz | ESP_INTR_FLAG_LEVEL1 ensures fast dispatch |
| Warmup removal breaks same-type pairs | None — warmup is preserved | Only cooldown is removed |
| ESP32 variant differences in TX_DONE | None — verified identical across all variants | See table above |

---

## Appendix B: ESP-IDF RS485 Hardware Mode (Solution C) — Not Recommended

The ESP-IDF documentation states that `UART_MODE_RS485_HALF_DUPLEX` provides automatic
RTS (DE) pin management: the driver asserts RTS when TX starts and deasserts when TX
finishes. However:

1. **It requires `uart_driver_install()`**, which conflicts with the bare-metal approach
   used in this codebase. The codebase deliberately avoids the UART driver to achieve
   lowest latency via `uart_ll_*` direct register access.

2. **The ESP-IDF docs explicitly state:** "The ESP32 UART controllers themselves do not
   support half-duplex communication as they cannot provide automatic control of the RTS
   pin." The automatic DE control is done in the UART **driver** (software), not in
   hardware. This means it goes through the driver's ISR, adding latency.

3. **RTS pin is not the same as a custom DE pin.** The hardware RTS is fixed to specific
   GPIO pins. Using a custom DE pin (like GPIO21 on the Waveshare) requires software
   control anyway.

4. **The driver's DE control has documented latency:** "Since the switching is made in the
   interrupt handler, comparing to DTR line, some latency is expected on RTS line."

For these reasons, Solution C (ESP-IDF RS485 mode) is not recommended. Solution D
(TX_DONE interrupt with bare-metal UART) achieves the same result with less latency
and more control, using the existing codebase architecture.

---

*End of transcript.*
