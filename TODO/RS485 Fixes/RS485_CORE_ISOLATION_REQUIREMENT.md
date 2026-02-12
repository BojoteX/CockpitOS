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

---

# COUNTER-ARGUMENT: The 40µs Theory Doesn't Hold Up

## Status: NEEDS VERIFICATION — Requesting 3rd-party review

The empirical results above are real (core isolation fixes it). But the *explanation*
of why has a fatal math problem, and deeper research reveals a more nuanced picture.

## The FIFO Problem — The 40µs Claim is Mathematically Wrong

The ESP32-S3 UART has a **128-byte hardware FIFO**. The document claims a 40µs ISR
delay causes a "missed byte." This is wrong:

| Fact | Value |
|---|---|
| Baud rate | 250,000 bps (8N1 = 10 bits/byte) |
| Time per byte | 40µs |
| UART RX FIFO depth | 128 bytes (hardware, per ESP32-S3 TRM) |
| FIFO buffer time before overflow | 128 × 40µs = **5,120µs (5.12ms)** |
| WiFi BB ISR worst case | 50–100µs |

The FIFO threshold is set to 1 (`uart_ll_set_rxfifo_full_thr(uartHw, 1)`), but this
only controls **when the interrupt fires**, NOT the FIFO capacity. If the ISR is
delayed by 100µs, only ~2-3 bytes accumulate — the other 125 FIFO slots are empty.
**The hardware buffers bytes regardless of whether the ISR has fired.**

A 50–100µs WiFi ISR consumes **1–3 of 128 FIFO slots**. The FIFO has ~50x more
headroom than needed. FIFO overflow from WiFi ISR latency alone is physically
impossible at 250kbps.

The document's reasoning applies to AVR (1-byte shift register, no FIFO). It does
not apply to ESP32.

## So Why Does Core Isolation Fix It?

Core isolation works empirically. The question is: what is the REAL mechanism?
After deep research into ESP-IDF internals, here is what we know for certain:

### DISPROVEN: Flash Cache Isolation Between Cores

An earlier version of this counter-argument proposed that core isolation works
because each core has independent cache lines. **This is wrong for ESP32-S3.**

- ESP32-S3 has a **shared, unified cache** (unlike original ESP32 which has
  `SOC_IDCACHE_PER_CORE`)
- When ANY core triggers a flash write, ESP-IDF's `spi_flash_disable_interrupts_
  caches_and_other_cpu()` explicitly stalls the OTHER core via IPC
- The other core enters `spi_flash_op_block_func()`: disables non-IRAM interrupts,
  then **busy-waits** until the flash operation completes
- Core isolation does NOT protect against flash cache disable — both cores are
  frozen regardless of which core initiated the write
- Source: `components/spi_flash/cache_utils.c` in ESP-IDF

### What Actually Kills RS485 — Three Mechanisms

After verifying with ESP-IDF source code, the actual threats are:

---

**Mechanism 1: WiFi BB ISR — Same-Level Interrupt Blocking (CONFIRMED)**

The WiFi baseband ISR runs at **interrupt level 1** — the same level as the RS485
UART ISR (allocated with `ESP_INTR_FLAG_LEVEL1`). On Xtensa, same-level interrupts
**cannot preempt each other**. When the WiFi BB ISR is executing (50–100µs), the
UART ISR is blocked.

At 250kbps, 50–100µs of ISR delay = 1–3 bytes accumulate in FIFO. This does NOT
overflow the 128-byte FIFO. **But it delays the state machine.**

The state machine runs byte-by-byte IN the ISR. If the ISR is delayed 100µs, the
state machine falls behind by 2-3 bytes. Those bytes are safe in the FIFO and will
be processed when the ISR runs. **This alone should not cause failure.**

However, if the WiFi BB ISR fires repeatedly (bursty WiFi traffic, scanning, beacon
processing), the cumulative effect could delay the ISR enough that:
- The slave misses its response window (master times out)
- The `esp_timer_get_time()` timestamp is stale, causing false sync timeouts
- Multiple protocol timeouts cascade into a death spiral

**This is the most likely primary cause.** Core isolation eliminates WiFi BB ISR
from Core 0 entirely, removing the interrupt contention. The failure isn't byte
loss — it's **protocol timing corruption** from ISR latency jitter.

---

**Mechanism 2: `esp_timer_get_time()` May Not Be IRAM-Safe (LATENT BUG)**

Both ISRs call `esp_timer_get_time()` (Slave line 383, Master line 543).
This function is only IRAM-safe if `CONFIG_ESP_TIMER_IN_IRAM` is enabled:

```c
// In esp_timer_impl.h:
#if CONFIG_ESP_TIMER_IN_IRAM
#define ESP_TIMER_IRAM_ATTR IRAM_ATTR
#else
#define ESP_TIMER_IRAM_ATTR           // <-- empty! Lives in flash!
#endif
```

**`CONFIG_ESP_TIMER_IN_IRAM` is NOT enabled by default.** If it's not enabled in
this project's sdkconfig, then `esp_timer_get_time()` lives in flash. Calling it
from an IRAM ISR during a flash write = CPU hangs trying to fetch code from
disabled cache.

Flash writes happen from WiFi NVS, OTA, and other subsystems. Both cores are
stalled during flash writes regardless of core isolation. If this function is NOT
in IRAM, core isolation is masking the symptom (fewer flash writes when WiFi is
on another core? less likely to coincide?) but NOT fixing the root cause.

**This is a latent bug regardless of core isolation.**

---

**Mechanism 3: USB-OTG — Both ISR AND Task (CORRECTED)**

The original document says USB-OTG is a FreeRTOS task. The counter-argument said
tasks can't preempt ISRs. Both are incomplete.

USB-OTG has **two components**:

1. **Hardware ISR** (`dcd_int_handler` in TinyUSB's `dcd_dwc2.c`): A real hardware
   interrupt that fires on USB events. Registered via `esp_intr_alloc()` with
   flags = 0 (NOT `ESP_INTR_FLAG_IRAM`). This means:
   - It IS a Level 1 interrupt that contends with the UART ISR on the same core
   - It is NOT IRAM-safe (disabled during flash writes — not a threat there)
   - It floats to whichever core runs USB init

2. **FreeRTOS task** (`usb_device_task`): Created with `xTaskCreate()` (no core
   pinning) at `configMAX_PRIORITIES - 1`. Processes deferred USB events. Can
   preempt the RS485 FreeRTOS drain task, causing export buffer backpressure.

**Both components can cause problems:**
- The USB ISR contends at interrupt level (same as WiFi BB ISR issue)
- The USB task can starve the RS485 export buffer drain task
- Neither is pinned to a core — they float to wherever the scheduler puts them

---

### Why `delay(1)` in loop() Is Not the Problem

The main loop runs on Core 1 (Arduino `loopTask`). The `delay(1)` yields to the
FreeRTOS scheduler for 1ms. This is on a different core from the RS485 ISR and
does not affect interrupt servicing on Core 0.

However, `RS485Slave_loop()` and `RS485Master_loop()` are called FROM `loop()` on
Core 1. If these functions interact with shared data (export buffer, change queue),
the cross-core access patterns matter. But this is task-level, not ISR-level.

### Why WiFi and USB-OTG Don't "Protect" Against This

You're right — they don't and can't. WiFi's job is radio communication. USB-OTG's
job is USB protocol handling. Neither has any awareness of UART peripherals.

The ESP-IDF interrupt allocator assigns interrupts to the core that calls
`esp_intr_alloc()`. WiFi ISRs land on whichever core runs `esp_wifi_init()`.
USB ISRs land on whichever core runs `USB.begin()`. There is no coordination
between subsystems — each grabs interrupt slots independently.

The only "protection" is architectural: keep competing interrupt sources on
separate cores. This is exactly what the original document recommends, and it's
correct — just for the wrong reason.

## Revised Assessment — What's Wrong in the Original Document

| Original Claim | Verdict | Correct Version |
|---|---|---|
| "40µs kill window" | **WRONG** | FIFO gives 5.12ms. Byte loss needs >5ms ISR delay. |
| "ANY interrupt >40µs causes missed byte" | **WRONG** | Bytes are safe in FIFO. The threat is protocol timing, not byte loss. |
| "Hardware interrupt controller limitation" | **PARTIALLY RIGHT** | Same-level ISRs can't preempt each other — that part is correct. But the consequence is timing jitter, not byte loss. |
| "USB-OTG preempts everything" | **WRONG** | It has a Level 1 ISR (contends with UART) AND a max-priority task (starves drain task). Neither "preempts everything." |
| "Core isolation is the only solution" | **WRONG** | It's the simplest solution. Others exist (DMA, higher interrupt level, IRAM audit). |
| "No code optimization changes this" | **WRONG** | Verifying IRAM safety, using DMA, or raising interrupt level would all help. |
| "This is NOT a software bug" | **PROBABLY WRONG** | `esp_timer_get_time()` may not be in IRAM. That's a software configuration bug. |
| Core isolation fixes it empirically | **CORRECT** | The test results are valid and trustworthy. |

## What's Wrong in the Counter-Argument (Self-Correction)

| Counter-Argument Claim | Verdict |
|---|---|
| "ESP32-S3 has independent cache per core" | **WRONG.** ESP32-S3 has shared cache. Flash cache disable stalls BOTH cores via IPC. Only original ESP32 has `SOC_IDCACHE_PER_CORE`. |
| "Flash cache theory explains core isolation" | **WRONG.** Since both cores are stalled during flash writes, core isolation cannot help with flash-related issues. |

## Revised Theory — Why Core Isolation Actually Works

The fix works because of **interrupt-level contention on Xtensa**, not FIFO overflow
or cache isolation:

1. WiFi BB ISR and UART ISR are both Level 1. On the same core, they block each
   other. WiFi BB ISR runs for 50–100µs, during which the UART ISR cannot fire.

2. The UART ISR runs the protocol state machine. Delayed ISR execution means
   stale timestamps from `esp_timer_get_time()`, missed response windows, and
   false sync timeouts. The protocol desynchronizes and enters a death spiral.

3. USB-OTG adds a second Level 1 ISR on whatever core it lands on, plus a
   max-priority task that can starve the RS485 buffer drain.

4. Core isolation moves ALL competing Level 1 ISR sources (WiFi BB, USB-OTG) and
   high-priority tasks off the RS485 core. The UART ISR runs with minimal latency.

**The death spiral is not byte loss. It's protocol timing corruption.**

## Recommended Diagnostic Steps

Before accepting core isolation as the permanent architecture:

1. **Add `UART_INTR_RXFIFO_OVF` to the ISR interrupt mask** — increment a counter
   when it fires. If this counter is ZERO during failures, FIFO overflow is
   definitively ruled out and the byte-loss theory is dead.

2. **Check `CONFIG_ESP_TIMER_IN_IRAM` in sdkconfig** — if it's not set, this is a
   latent crash bug during ANY flash write, regardless of core isolation. Fix:
   enable it, or replace `esp_timer_get_time()` with a direct systimer register
   read (guaranteed IRAM-safe since it's a memory-mapped register).

3. **Verify all ISR code paths are in IRAM** — compile with `-ffunction-sections`,
   check the linker map for any ISR-reachable function in `.flash.text` instead of
   `.iram0.text`. Focus on `uart_ll_*` inlining at the current optimization level.

4. **Add protocol-level diagnostics** — count sync timeout events, missed response
   windows, and false re-syncs. If these spike when WiFi is on the same core (but
   RXFIFO_OVF stays at zero), the "timing corruption" theory is confirmed.

5. **Pin USB-OTG to Core 1 explicitly** — in Arduino-ESP32, `USB.begin()` should
   be called from a Core 1 context to ensure both the USB ISR and task land there.
   This may fix the USB-OTG interference without full core isolation.

## Conclusion

Core isolation is a **correct and valid engineering solution**. The empirical results
are trustworthy. The recommendation to use it is sound.

But the explanation of WHY matters for three reasons:

1. **Single-core variants (C3, C6, H2)**: If the real problem is Level 1 ISR
   contention, these chips cannot use core isolation. The fix would be interrupt
   level separation (UART at Level 3+) or DMA.

2. **The `esp_timer_get_time()` IRAM question**: This is a potential crash bug
   that core isolation does not fix. During flash writes, BOTH cores are stalled.
   If this function isn't in IRAM, a flash write during active RS485 traffic will
   crash regardless of core assignment.

3. **USB-OTG can be pinned**: Unlike WiFi (whose ISR is buried in a binary blob),
   USB-OTG initialization can be controlled. Pinning it to Core 1 may be sufficient
   to fix the USB-OTG interference specifically, without requiring full core isolation.
