# RS485 Implementation Overview — What We Found and What Needs to Change

## What This Document Is

This is the non-technical starting point for anyone picking up the RS485 work on
CockpitOS. It explains the background, what went wrong, and where to go from here.
The two companion documents contain all the technical depth needed to actually write
the code:

- **`RS485_MASTER_TIMING_DEBUG_TRANSCRIPT.md`** — Master-side root cause analysis,
  AVR vs ESP32 comparison, and the full implementation plan (Solution D).
- **`RS485_SLAVE_PERFORMANCE_ANALYSIS.md`** — Slave-side performance audit, ISR
  blocking analysis, and the TX_DONE refactor with AVR head-to-head comparison.

Read this document first. Then read whichever companion document applies to the
component you are working on.

---

## Background

CockpitOS is a platform for building physical cockpit panels that interface with
DCS World (a flight simulator) through the DCS-BIOS protocol. The cockpit panels
are connected over an RS485 bus: one master device talks to multiple slave devices
using a poll-response protocol at 250,000 baud.

The original DCS-BIOS RS485 implementation runs on AVR microcontrollers (Arduino Mega).
It has been stable and widely used for years. CockpitOS ported this implementation to
the ESP32 platform to take advantage of faster processors, more memory, WiFi, BLE, and
the broader ESP32 ecosystem.

The ESP32 port works. Panels update, switches register, the bus communicates. But during
cross-device testing, we discovered that the port is **fragile** — it functions under
favorable conditions but fails when the hardware mix changes. We also discovered that
even when it works, it is far less efficient than it should be, leaving significant
ESP32 hardware capabilities unused.

---

## The Problem

### What We Observed

We tested four combinations of master and slave devices:

| Master | Slave | Outputs (master to slave) | Inputs (slave to master) |
|--------|-------|---------------------------|--------------------------|
| Waveshare (manual DE pin) | Waveshare (manual DE pin) | Work | Work |
| Lolin S3 Mini (auto-direction) | Lolin S3 Mini (auto-direction) | Work | Work |
| **Waveshare (manual DE pin)** | **Lolin S3 Mini (auto-direction)** | **Work** | **Almost never work** |
| AVR Arduino Mega | Either slave type | Work | Work |

Same-type pairs work fine. The AVR master works with everything. But when a manual
DE-pin master is paired with an auto-direction slave, inputs from the slave are lost.
The user has to press a button many times to get even one event through.

### What This Told Us

The fact that outputs always work but inputs fail means the broadcast path (master
sending data to all slaves) is fine. The problem is in the **poll-response path** —
specifically, the moment between the master finishing its poll transmission and the
slave beginning its response.

The AVR master handles this transition in hardware, releasing the bus within
microseconds of the last transmitted bit. The ESP32 master holds the bus for an
extra ~52 microseconds after transmission completes (a combination of a cooldown
delay and software overhead). During that window, fast auto-direction slaves have
already started responding — onto a bus the master is still driving. The result is
bus contention and data corruption.

This is not a tuning problem. No amount of adjusting warmup or cooldown delay values
fixes it, because the issue is structural: the order of operations in the ESP32 code
puts the bus release **after** delays that should not exist.

### Why the Slave Also Needs Work

While investigating the master, we examined the slave's transmit path and found the
same architectural pattern: a blocking function that holds the CPU captive inside a
hardware interrupt for the entire duration of a response transmission. For a typical
response, this means ~220 microseconds of CPU lockout — during which no other
interrupts can fire, no WiFi packets can be serviced, and no other processing occurs.

The slave gets away with this because nothing time-critical depends on it releasing
the bus quickly (the master waits patiently). But the inefficiency is severe: the
ESP32 has a 128-byte hardware TX FIFO that the current code loads and then
spin-waits on, defeating the entire purpose of the FIFO. The AVR, with only a
single-byte transmit register, is actually 44 times more CPU-efficient for a
typical response because it uses interrupts instead of spin-waiting.

This matters for two practical reasons:

1. **WiFi and BLE cannot coexist** with a slave that blocks interrupts for hundreds
   of microseconds per poll cycle. On single-core ESP32 variants (C3, C6, H2), this
   is a hard blocker. On dual-core variants, it forces careful core pinning that
   should not be necessary.

2. **Scalability.** As the number of slaves on a bus grows and poll frequency
   increases, the cumulative CPU waste from blocking responses becomes significant.

---

## The Fix — General Direction

Both the master and the slave need the same fundamental change: **stop spin-waiting
for transmission to complete, and instead use the ESP32's `UART_INTR_TX_DONE` hardware
interrupt to get notified when the last bit has left the wire.**

This interrupt (bit 14 of the UART interrupt status register) is the exact hardware
equivalent of the AVR's TXC (TX Complete) interrupt — the same mechanism that makes
the AVR implementation work reliably with every device type. It has been verified
available on all ESP32 variants (ESP32, S2, S3, C3, C6, H2) and uses the same
`uart_ll_*` functions already present in the codebase.

### Master

The master fix is the **critical priority**. It resolves the interoperability bug
that causes slave inputs to be lost with mixed hardware types. The change replaces
the blocking `txWithEchoPrevention()` function with a non-blocking pattern:

1. Assert DE, optional warmup, load TX FIFO, arm TX_DONE interrupt, return.
2. When TX_DONE fires: flush echo bytes, deassert DE, enable RX, start timeout.

The cooldown delay is eliminated entirely. The bus is released at the earliest
possible hardware-guaranteed moment — matching the AVR's behavior.

See **`RS485_MASTER_TIMING_DEBUG_TRANSCRIPT.md`**, Section 11 and Appendix A, for
the complete implementation plan, pseudocode, and risk assessment.

### Slave

The slave fix is a **performance and compatibility improvement**. It does not fix a
visible bug, but it transforms the slave from a blocking design that wastes the ESP32's
hardware into one that is architecturally optimal. The change replaces the blocking
`sendResponseISR()` with:

1. Build response in buffer, load TX FIFO in one burst, arm TX_DONE interrupt, return from ISR.
2. When TX_DONE fires (a few hundred microseconds later): flush echo, deassert DE,
   re-enable RX. Total ISR time: ~8 microseconds instead of ~220+.

This makes WiFi and BLE viable on slave devices and gives the ESP32 slave constant-time
O(1) ISR cost regardless of response size — something the AVR cannot achieve due to its
single-byte transmit register.

See **`RS485_SLAVE_PERFORMANCE_ANALYSIS.md`**, Sections 7-9, for the full solution,
expected improvements, and the AVR vs ESP32 comparison.

### Implementation Order

1. **Master first.** It fixes the actual bug. Test with the mixed-hardware combination
   (Waveshare master + Lolin slave) that currently fails.
2. **Slave second.** It is a performance improvement. Test by monitoring ISR duration
   and verifying WiFi stability under poll load.
3. Both changes use the same interrupt mechanism and the same `uart_ll_*` API calls,
   so patterns learned on one side apply directly to the other.

---

## What Success Looks Like

After implementation:

- All four hardware combinations in the test matrix work reliably for both
  outputs and inputs, matching the AVR master's universal compatibility.
- The ESP32 master releases the bus within ~1 microsecond of the last transmitted
  bit, down from ~52 microseconds.
- The ESP32 slave's ISR time drops from ~220 microseconds to ~8 microseconds per
  poll response.
- WiFi and BLE function reliably on slave devices without core-pinning workarounds.
- The ESP32 implementation is not just "as good as" the AVR — it is measurably
  better, leveraging the 128-byte TX/RX FIFOs that the AVR does not have.

---

*Companion documents:*
- `RS485_MASTER_TIMING_DEBUG_TRANSCRIPT.md` — Full master technical analysis
- `RS485_SLAVE_PERFORMANCE_ANALYSIS.md` — Full slave technical analysis
