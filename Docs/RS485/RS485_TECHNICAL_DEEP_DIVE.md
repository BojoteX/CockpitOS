# DCS-BIOS RS485 on ESP32: A Bare-Metal Implementation

## The Challenge

DCS-BIOS RS485 has lived on AVR for years. It works. But the AVR architecture has hard physical limits — a single-byte UART buffer, no DMA, no FIFO, and ISR-per-byte transmission. Every byte transmitted requires a separate interrupt. A 10-byte slave response means 11 ISR entries just to get it on the wire.

CockpitOS implements the DCS-BIOS RS485 protocol on ESP32 from the ground up — bare-metal, ISR-driven, direct hardware register access — achieving O(1) constant-time transmission regardless of payload size, sub-microsecond bus turnaround, and 100% wire compatibility with every existing AVR device on the bus.

---

## Architecture: No Driver, No Abstraction, No Overhead

CockpitOS bypasses the ESP-IDF UART driver entirely. The driver is built for general-purpose serial — it installs its own ISR, manages internal ring buffers, and adds 15-30us of overhead per interrupt. For a protocol that needs sub-microsecond bus turnaround to maintain interoperability with AVR devices, that overhead is unacceptable.

Instead, the implementation goes direct:

- `periph_module_enable()` powers the UART peripheral — no driver installation
- Baud rate, stop bits, parity configured through direct register writes
- Custom ISR registered via `esp_intr_alloc()` with `ESP_INTR_FLAG_IRAM` — executes from internal RAM, immune to flash cache misses
- RX FIFO threshold set to **1 byte** — the ISR fires on the very first arriving byte, not after a configurable watermark

The state machine runs **inside** the ISR. No buffering. No deferred processing. No task notification overhead. Byte arrives, state transitions, done. This matches the AVR's approach — but with hardware that can do dramatically more per interrupt.

---

## The 128-Byte FIFO Advantage

This is the fundamental architectural difference.

**AVR**: Single-byte UART Data Register (UDR). To transmit N bytes, the CPU enters the UDRE ISR N times (one byte per entry), then one final TXC interrupt when complete. A 50-byte slave response = 51 interrupts.

**ESP32**: 128-byte hardware TX FIFO. The entire response is burst-loaded in a single operation — `uart_ll_write_txfifo()` copies directly into the peripheral's FIFO memory. Hardware shifts bytes out autonomously. One `UART_INTR_TX_DONE` interrupt fires when the last stop bit leaves the shift register. A 50-byte slave response = 2 interrupts (one RX to detect the poll, one TX_DONE when finished).

The slave ISR cost is O(1) constant time — approximately 8us regardless of whether the response is 1 byte or 128 bytes. The CPU loads the FIFO, arms the interrupt, and returns. Hardware does the rest.

---

## TX_DONE: Releasing the Bus at the Hardware Limit

Half-duplex RS485 requires precise bus turnaround. The transmitter must release DE (Driver Enable) the instant the last bit leaves the wire — not a microsecond later, because a fast slave (or master) may already be responding.

AVR achieves this with the TXC (Transmit Complete) interrupt, which fires after the last stop bit of the last byte. CockpitOS uses the equivalent: `UART_INTR_TX_DONE`, which fires when the last bit (including stop bit) exits the shift register.

Inside the TX_DONE ISR:
1. Flush the RX FIFO — discard all echo bytes from the half-duplex loopback in a single operation (not byte-by-byte like AVR)
2. Deassert DE — bus released, ~1us from last bit
3. Re-enable RX interrupts — ready for incoming data
4. Transition state machine

Bus turnaround: **~1us**. Matches AVR. On auto-direction transceivers, the hardware switches in nanoseconds — the protocol doesn't even notice.

---

## Preemption-Safe Interrupt Ordering

On single-core ESP32 (S2), FreeRTOS tick preemption can interrupt any code at any instruction boundary. The RS485 task shares priority with other system tasks. When a tick fires, the scheduler switches away mid-function — mid-register-write if necessary.

This creates a critical ordering requirement for the TX path. The ESP32 UART interrupt architecture:

| Register | Function |
|----------|----------|
| `int_raw` | Hardware sets bits here autonomously (TX_DONE, RX_FULL, etc.) |
| `int_ena` | Software enable mask |
| `int_st` | `int_raw AND int_ena` — this triggers the ISR |
| `int_clr` | Write-only — clears bits in `int_raw` |

The TX sequence must be ordered to guarantee the TX_DONE event **cannot be lost** regardless of where preemption occurs:

```
1. state = TX_WAITING_DONE       // ISR ready to handle TX_DONE
2. Clear TX_DONE in int_raw      // Erase stale event from PREVIOUS cycle
3. Load TX FIFO                  // Hardware begins autonomous transmission
4. Enable TX_DONE in int_ena     // Arm the interrupt
```

**Why this ordering is bulletproof:**

Step 2 (clear) happens before step 3 (FIFO load). It can only erase stale bits from the previous transmission — there is no current event to lose because the FIFO isn't loaded yet.

After step 3, the hardware transmits autonomously and accumulates TX_DONE in `int_raw` regardless of what the CPU is doing — even if preempted for milliseconds.

Step 4 (enable) writes `int_ena`. At this point `int_st = int_raw AND int_ena` is guaranteed non-zero. The ISR fires immediately if the hardware already finished, or fires naturally when it does finish.

**Preemption between steps 3 and 4?** The event accumulates in `int_raw`. When the task resumes and writes `int_ena`, the ISR fires instantly.

**Preemption between steps 2 and 3?** No event exists to lose. FIFO isn't loaded. When the task resumes, it loads the FIFO and the sequence continues normally.

**There is no window where the event can be set and then erased.** It is structurally impossible.

This same ordering is applied identically in both the master's `txWithEchoPrevention()` and the slave's `sendResponseISR()`.

**Verified: 11+ hours continuous operation, 25 million polls, 100.0% response rate, single-core ESP32-S2.**

---

## Slave Response: Zero-Delay Warmup

The AVR slave burns a phantom 0x00 byte (~40us at 250kbps) with DE deasserted before responding — a timing spacer so the master can finish its TX-to-RX turnaround.

CockpitOS eliminates this dead time. When polled, the slave:

1. **Asserts DE immediately** — transceiver begins settling (MAX485 typical: 200ns)
2. **Builds the response frame** in a static buffer — this loop runs 0.5-15us at 240MHz depending on payload size
3. **Loads the FIFO** — single burst operation
4. **Arms TX_DONE** — returns to caller

The frame build loop (step 2) provides natural transceiver settling time — 2.5x to 75x the MAX485's settling requirement, with zero wasted cycles. No `ets_delay_us()` blocking, no spin-waiting, no dead time. Every microsecond of the "warmup" is doing useful work.

---

## RISC-V Memory Coherency

ESP32-S2 and S3 use Xtensa cores where `portDISABLE_INTERRUPTS()` (the RSIL instruction) has implicit memory barrier semantics — all pending stores complete before the instruction retires.

ESP32-C3, C6, and H2 use RISC-V cores. `portDISABLE_INTERRUPTS()` on RISC-V does **not** imply a memory fence. Disabling interrupts prevents preemption but does not guarantee store visibility.

This matters in two places:

1. **FIFO reads**: After reading bytes from the RX FIFO, the RISC-V core may cache the FIFO length register value. Without a fence, the next `uart_ll_get_rxfifo_len()` call could return a stale count, causing duplicate or missed byte reads.

2. **txBuffer coherency**: The main loop writes commands into `txBuffer` (ring buffer). The ISR reads from it when polled. Without a fence, the ISR could read stale data — a command that was written by the task but not yet visible to the ISR's memory view.

Both are handled with explicit `fence` instructions:

```cpp
#ifdef __riscv
    __asm__ __volatile__("fence" ::: "memory");
#endif
```

On Xtensa builds, this compiles to nothing. On RISC-V, it ensures all prior stores are globally visible before the ISR reads from shared memory.

---

## Master: Two Operating Modes

### Relay Mode (Default)

Raw byte pump. The DCS-BIOS export stream from UDP/WiFi enters a 512-byte ring buffer and goes straight to the RS485 bus — no parsing, no filtering, no transformation. Bytes in, bytes out.

This mode exists for one reason: **interoperability**. A CockpitOS master in relay mode is a drop-in replacement for an AVR master. Every existing AVR slave works without configuration. Every DCS aircraft works. Every address range works. Plug in and go.

### Smart Mode (Optional)

The master parses the DCS-BIOS export stream in real-time, extracts address/value pairs, and cross-references them against a table of which slaves need which addresses (`DcsOutputTable`). Only relevant data gets broadcast to the bus.

Optional change detection takes it further — if an address hasn't changed value since the last broadcast, it doesn't get sent again.

**Bandwidth reduction: 100x to 1000x** on a typical panel. A full DCS-BIOS export frame is ~8KB. A panel with 20 gauges and switches might care about 40 bytes of it. Smart mode sends those 40 bytes. Change detection sends only the ones that actually changed this frame — often zero.

Smart mode is **off by default**. Relay mode is the safe, compatible, zero-configuration starting point.

---

## Poll Timeout: Two-Tier Recovery

The master uses two timeout thresholds to handle unresponsive or partially-responsive slaves:

| Timeout | Duration | Trigger | Action |
|---------|----------|---------|--------|
| **Poll timeout** | 1ms | No response byte at all | Mark slave offline, send timeout marker, move to next slave |
| **Mid-message timeout** | 5ms | Partial response then silence | Inject `\n` to realign DCS-BIOS parser, mark complete, continue |

The mid-message timeout keeps the DCS-BIOS command parser aligned. If a slave sends 5 bytes of a command then dies, the parser would be stuck waiting for the delimiter. The injected `\n` closes the partial message cleanly and the bus continues operating.

---

## Interoperability Matrix

| Configuration | Status |
|--------------|--------|
| CockpitOS Master + AVR Slaves | Works out of the box (relay mode) |
| AVR Master + CockpitOS Slave | Works (`RS485_ARDUINO_COMPAT=1` for 0x72 checksum) |
| CockpitOS Master + CockpitOS Slaves | Works (can enable smart mode) |
| Mixed bus (AVR + ESP32 slaves) | Works (relay mode, same protocol on wire) |
| Any DCS aircraft | Works (relay mode passes all addresses) |

The protocol is byte-identical on the wire. An oscilloscope cannot distinguish a CockpitOS frame from an AVR frame. Up to 127 slave addresses on a single bus.

---

## WiFi Debug: Live Bus Monitoring

Since CockpitOS supports WiFi as a transport/debug channel, the RS485 bus can be monitored in real-time from a browser or terminal:

- **As master**: See every slave response — what commands each slave is sending, response timing, error rates
- **As slave**: See what's being queued for the master — export data arriving, commands waiting to send, buffer utilization

No logic analyzer. No USB-serial adapter. No second computer. Connect to WiFi, open the debug stream, watch the bus live.

---

## Performance

| Metric | Value |
|--------|-------|
| Master main loop | 30us |
| Slave main loop | 20us |
| Slave ISR response time | ~8us (O(1) constant) |
| Bus turnaround | ~1us |
| TX FIFO burst | Up to 128 bytes, single operation |
| Baud rate | 250kbps (DCS-BIOS standard) |
| Max slaves | 127 |
| Platforms | ESP32, S2, S3, C3, C6, H2 (single and dual core) |

**Endurance test**: 11+ hours, 25 million polls, 100.0% response rate, zero data loss — on a single-core ESP32-S2 running FreeRTOS with USB, WiFi, and RS485 all sharing one CPU.

A DCS frame at 60fps is 16.6ms. The master loop runs ~550 times per DCS frame. The slave loop runs ~830 times. An encoder pulse at maximum human rotation speed is roughly 1ms. The slave samples it 50 times per pulse.

No input is missed. Ever.

---

## What's Next

- HID aggregation mode — slaves send DCS-BIOS commands to the master over the bus as usual, but the master can optionally map those inputs to USB HID buttons and axes, presenting the entire RS485 bus as a single unified USB controller to the PC
- Per-slave export filtering tables for smart mode bandwidth optimization
- Diagnostic counters accessible via WiFi for field debugging without physical access

---

*CockpitOS RS485 — Master v2.4, Slave v3.3*
*Bare-metal UART, ISR-driven, preemption-safe, AVR-compatible*
