# RS485 + WiFi/USB Coexistence: Root Cause & Solution

## Status: SOLVED — Task Priority Starvation

## TL;DR

RS485 bus dies when sharing a core with WiFi or USB. The root cause is **FreeRTOS
task priority starvation**, NOT ISR contention, NOT FIFO overflow, NOT core isolation.

**Fix: `RS485_TASK_PRIORITY = 23`** (same as WiFi). Round-robin time slicing gives
RS485 guaranteed scheduling. Confirmed flawless with everything on a single core.

---

## The Symptom

| Configuration | Result |
|---|---|
| RS485 (priority 5) + WiFi on same core | **Fails in 30s-2min** |
| RS485 (priority 5) + USB on same core | **Fails in 1-5min** |
| RS485 (priority 5) alone on dedicated core | **100% stable** |
| RS485 (**priority 23**) + WiFi + USB all on same core | **100% stable** |

## What Was WRONG (Previous Theories)

### Theory 1: "40us Kill Window" - DISPROVEN

Claimed any ISR delay >40us causes missed bytes. **Mathematically wrong.**

ESP32-S3 UART has 128-byte hardware FIFO. At 250kbps (40us/byte), the FIFO buffers
5.12ms of data. WiFi's worst-case ISR delay is 50-100us, consuming only 1-3 of 128
FIFO slots. FIFO overflow from ISR delay alone is physically impossible.

### Theory 2: "ISR Level Contention" - INSUFFICIENT

WiFi BB ISR and UART ISR are both Level 1, so they block each other on the same
core. This is true but NOT the primary failure mechanism. ISR delays of 50-100us
don't overflow the FIFO and don't corrupt protocol timing enough to kill the bus.

### Theory 3: "Core Isolation Required" - WRONG

Core isolation appeared to work because it *incidentally* separated the RS485 task
from high-priority WiFi/USB tasks. It was solving the right problem for the wrong
reason.

## The ACTUAL Root Cause: Task Priority Starvation

### Evidence: FreeRTOS Task Dump

```
Task Name       Priority    Core
---------       --------    ----
loopTask             1      Core 1
RS485_Master         5      Core 0   <-- STARVED
tiT (USB)           18      floats
sys_evt             20      floats
esp_timer           22      floats
wifi (WiFi)         23      Core 0   <-- STARVES RS485
```

RS485 task at priority 5 gets **permanently preempted** by:
- WiFi task at priority 23 (sustained processing, not just bursts)
- USB/tiT task at priority 18
- esp_timer at priority 22
- sys_evt at priority 20

### The Death Mechanism

1. WiFi task runs sustained processing on the same core as RS485 task
2. RS485 task at priority 5 cannot preempt WiFi at priority 23
3. `messageBuffer.complete` stays `true` because the task can't drain it
4. Master's `rs485PollLoop()` checks:
   ```cpp
   if (messageBuffer.isEmpty() && !messageBuffer.complete) {
       // Poll next slave
   }
   ```
5. Condition is never true -> no new polls -> bus goes silent -> death spiral

### Why Core Isolation Appeared to Work

When RS485 was on a dedicated core, there were no higher-priority tasks competing
for CPU time on that core. The RS485 task (priority 5) was the *highest* priority
task on its core, so it always ran. It wasn't ISR isolation that helped - it was
**task scheduling isolation**.

## The Solution: Priority 23 + Round-Robin

### Why It Works

FreeRTOS uses round-robin time slicing for tasks at the **same priority level**.
At priority 23 (same as WiFi):
- RS485 gets a guaranteed 1ms time slice every round-robin cycle
- RS485 uses <1% of its slice (~5-10us of actual work, then `vTaskDelayUntil` sleeps)
- WiFi gets 99%+ of CPU time - zero impact on WiFi performance
- USB at priority 18 is below RS485 now - cannot starve it

### Why RS485 at Priority 23 is Safe

The RS485 FreeRTOS task is **extremely lightweight**:
```cpp
static void rs485Task(void* param) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    while (taskRunning) {
        rs485PollLoop();  // ~5-10us of work
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(1));  // sleep for 1ms
    }
}
```

Each iteration: check state, maybe drain messageBuffer, return. Then voluntarily
sleep for 1ms. The task is awake for <0.5% of the time. It cannot starve WiFi
because it doesn't hold the CPU.

### Config Change

In both `RS485Config.h` (master) and `RS485SlaveConfig.h` (slave):
```cpp
#define RS485_TASK_PRIORITY     23
```

## Why WiFi and USB Coexist (But RS485 Couldn't)

WiFi and USB use an ISR -> queue -> task deferral pattern:
- Their ISRs are thin notification layers (~1-5us)
- ISR posts event to FreeRTOS queue
- FreeRTOS task processes the event at task level
- NO timing-critical decisions in the ISR itself

RS485's ISR runs the full protocol state machine with timestamp-dependent
decisions. The ISR itself is fine (FIFO protects it). The problem was always
at the TASK level where the buffer drain happens.

## Diagnostic Journey (Chronological)

1. Blamed WiFi ISR contention ("40us kill window") -> FIFO math disproves it
2. Blamed Level 1 ISR blocking -> true but insufficient to explain failure
3. User's systematic scenario testing proved:
   - ISR on Core 0, task on Core 1 -> works (ISR is fine!)
   - Task on same core as WiFi -> fails regardless of ISR location
4. Task dump revealed priority 5 vs WiFi 23 -> starvation confirmed
5. Raised to priority 23 -> flawless on single core with WiFi + USB

## Remaining Investigation Items

### esp_timer_get_time() IRAM Safety (LATENT BUG?)

Both master and slave ISRs call `esp_timer_get_time()`. This function is only
IRAM-safe if `CONFIG_ESP_TIMER_IN_IRAM` is enabled in sdkconfig. If NOT enabled,
calling it from an IRAM ISR during a flash write = crash (CPU tries to fetch from
disabled cache).

**Status: NOT YET VERIFIED.** Check sdkconfig for this setting. If not enabled,
either enable it or replace with a direct systimer register read.

### Single-Core ESP32 (S2, C3, C6)

Priority 23 solution should work on single-core chips since it's a task-level fix.
Round-robin scheduling works the same regardless of core count. However, single-core
means ALL ISRs share one interrupt controller - if ISR-level contention IS a factor
(secondary to the primary task starvation), it would manifest here.

**Status: NOT YET TESTED.** User identified S2 (single CPU, no DMA) as the real test.

---

*Last updated after confirming priority 23 fix with WiFi + USB + Events + Loop + RS485
all running on Core 0 simultaneously.*
