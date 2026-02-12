# RS485 CORE ISOLATION REQUIREMENT

## Status: MANDATORY — Not Optional

## The Rule
**RS485 ISR must have COMPLETE core isolation. Nothing else runs on its core.**

## What Was Proven

| Configuration | Result |
|---|---|
| RS485 alone on Core 0, loop+events on Core 1, WiFi OFF | **100% stable all night** |
| RS485 on Core 0 + WiFi on Core 0 | **Fails in 30s–2min** |
| RS485 on same core as loop/events, WiFi OFF | **Also shuts down** |

## Why — The 40µs Kill Window

At 250kbps, one byte = ~40µs. The RXFIFO_TOUT safety net catches the FIFO drain race
in most cases, but ANY interrupt or task that holds the CPU for >40µs on the RS485 core
can cause a missed byte → state machine desync → death spiral (both TX/RX LEDs go dark).

## What Can Kill RS485 (besides WiFi)

- **WiFi BB ISR**: 50–100µs interrupt-disabled windows. Most aggressive offender.
- **USB-OTG**: Runs at `configMAX_PRIORITIES - 1` (highest priority), floats with
  `tskNO_AFFINITY`. When it lands on the RS485 core, it preempts everything.
- **Arduino event loop**: `loopTask` processes events, timers, callbacks. Any callback
  taking >40µs delays the ISR.
- **GPIO ISR service**: Button/encoder GPIO interrupts share the interrupt controller.
- **System timer ISRs**: ESP-IDF timers, `esp_timer` high-priority callbacks.

## WiFi Was the Loudest, Not the Only

WiFi was blamed initially because it's the most aggressive (50–100µs BB ISR). But
USB-OTG at max priority, or a burst of event processing, causes the same failure —
just less frequently, which is why it takes longer to manifest without WiFi.

## Correct Architecture

```
Core 0: RS485 ISR ONLY (master or slave)
         - UART RX interrupt (RXFIFO_FULL + RXFIFO_TOUT)
         - UART TX_DONE interrupt
         - RS485 FreeRTOS task (export buffer drain only)
         - NOTHING ELSE

Core 1: Everything else
         - Arduino loop()
         - Events (WiFiEvents, GPIO, timers)
         - WiFi stack
         - USB-OTG (floats but mostly here since loop is here)
         - All application logic
```

## Key Takeaway

This is NOT a software bug. It's a hardware interrupt controller limitation.
Two interrupt sources on the same core share one interrupt controller. If one
holds interrupts disabled for >40µs, the other misses its window. No amount
of code optimization changes this — core isolation is the only solution.
