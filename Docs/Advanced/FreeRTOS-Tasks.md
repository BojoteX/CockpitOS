# FreeRTOS Tasks

Deep dive into FreeRTOS task usage in CockpitOS. This document covers the task creation patterns used by TFT gauges, the IFEI display, and the RS485 bus driver, including core pinning, stack sizing, priority selection, memory allocation strategies, volatile data sharing between tasks and ISRs, and cleanup.

---

## 1. Overview

CockpitOS runs on ESP32 chips that use FreeRTOS as their real-time operating system. The Arduino `loop()` function runs on core 1 (on dual-core chips) at priority 1, but certain subsystems benefit from running in their own dedicated tasks:

- **TFT Gauges** -- render needle animations at ~77 fps independent of the main loop.
- **IFEI Display** (optional) -- commit HT1622 shadow RAM at 250 Hz.
- **RS485 Master/Slave** -- maintain deterministic bus timing regardless of main loop load.

Each task is created during the panel or driver `init()` function and runs until the system is shut down or the panel is deinitialized.

---

## 2. Task Creation Patterns

### Dual-Core: xTaskCreatePinnedToCore

On dual-core chips (ESP32, ESP32-S3), tasks are pinned to a specific core:

```cpp
xTaskCreatePinnedToCore(
    BatteryGauge_task,      // Task function
    "BatteryGaugeTask",     // Name (for debugging)
    4096,                   // Stack size in bytes
    nullptr,                // Parameter (unused)
    2,                      // Priority
    &tftTaskHandle,         // Task handle (for deletion)
    BATT_CPU_CORE           // Core affinity (0 or 1)
);
```

### Single-Core: xTaskCreate

On single-core chips (ESP32-S2, ESP32-C3, ESP32-C6, ESP32-H2), core affinity is not applicable:

```cpp
xTaskCreate(
    rs485Task,              // Task function
    "RS485M",               // Name
    RS485_TASK_STACK_SIZE,  // Stack size
    nullptr,                // Parameter
    RS485_TASK_PRIORITY,    // Priority
    &rs485TaskHandle        // Task handle
);
```

### Compile-Time Selection

CockpitOS uses preprocessor guards to select the correct API:

```cpp
#if defined(CONFIG_IDF_TARGET_ESP32S2) || defined(CONFIG_IDF_TARGET_ESP32C3) || \
    defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32H2)
    // Single-core: no affinity
    xTaskCreate(taskFunc, "name", stackSize, nullptr, priority, &handle);
#else
    // Dual-core: pin to specified core
    xTaskCreatePinnedToCore(taskFunc, "name", stackSize, nullptr, priority,
                            &handle, TASK_CORE);
#endif
```

---

## 3. TFT Gauge Task Pattern

All TFT gauge panels (Battery, Brake Pressure, Cabin Pressure, Hydraulic Pressure, Radar Altimeter) follow the same task pattern.

### Task Function

```cpp
static void BatteryGauge_task(void*) {
    for (;;) {
        BatteryGauge_draw(false, false);
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
```

The task runs an infinite loop that:
1. Calls the draw function (which checks dirty flags and timing internally).
2. Delays for 5ms, yielding the CPU to other tasks.

### Dirty Flag Communication

DCS-BIOS callbacks (running in the main loop or DCS-BIOS task context) set volatile flags that the gauge task reads:

```cpp
// Shared state (volatile for cross-task visibility)
static volatile int16_t angleU = U_MIN, angleE = E_MAX;
static volatile bool    gaugeDirty = false;
static volatile bool    needsFullFlush = true;
static volatile uint8_t currentLightingMode = 0;

// DCS-BIOS callback (main loop context)
static void onBatVoltUChange(const char*, uint16_t v, uint16_t) {
    int16_t a = map(v, 0, 65535, U_MIN, U_MAX);
    if (a != angleU) { angleU = a; gaugeDirty = true; }
}

// Draw function (task context)
static void BatteryGauge_draw(bool force, bool blocking) {
    const bool stateChanged = gaugeDirty
        || (u != lastDrawnAngleU)
        || (e != lastDrawnAngleE)
        || needsFullFlush;
    if (!stateChanged) return;

    gaugeDirty = false;
    // ... render and flush ...
    needsFullFlush = false;
}
```

The `volatile` keyword ensures that writes from one task context are visible to reads from another. For simple flag-and-value patterns like this, `volatile` is sufficient -- no mutex or critical section is needed because:
- The DCS-BIOS callback only writes angle values and sets `gaugeDirty = true`.
- The draw function only reads angles and clears `gaugeDirty = false`.
- A torn read on a 16-bit angle value is harmless (the next draw cycle corrects it).

### Draw Interval Throttling

The draw function enforces a minimum interval between draws:

```cpp
#define GAUGE_DRAW_MIN_INTERVAL_MS 13  // ~77 fps max

if (!force && !needsFullFlush && (now - lastDrawTime < GAUGE_DRAW_MIN_INTERVAL_MS))
    return;
```

Combined with the 5ms task delay, the effective draw rate is approximately 77 fps maximum (limited by `GAUGE_DRAW_MIN_INTERVAL_MS`), but the task wakes up every 5ms to check for dirty flags.

### Enable/Disable at Compile Time

Each gauge has its own enable macro:

```cpp
#define RUN_GAUGE_AS_TASK 1              // Battery
#define RUN_BRAKE_PRESSURE_GAUGE_AS_TASK 1
#define RUN_CABIN_PRESSURE_GAUGE_AS_TASK 1
#define RUN_HYD_PRESSURE_GAUGE_AS_TASK 1
#define RUN_RADARALT_AS_TASK 1
```

When set to 0, the draw function is called from the panel's `disp_loop` hook instead, running in the main loop.

---

## 4. IFEI Display Task

The IFEI panel can optionally run its HT1622 commit cycle in a dedicated task:

```cpp
#define RUN_IFEI_DISPLAY_AS_TASK 0  // Default: main loop
#define IFEI_DISPLAY_REFRESH_RATE_HZ 250

#if RUN_IFEI_DISPLAY_AS_TASK
xTaskCreate(IFEIDisplayTask, "IFEIDisplay", 4096, NULL, 1, NULL);
#endif
```

When enabled, the task calls `commit()` at up to 250 Hz. When disabled (the default), `commit()` is called from `IFEIDisplay_loop()` which runs as the panel's `disp_loop` hook.

The IFEI task does not store a task handle because the display is never deinitialized at runtime.

---

## 5. RS485 Task Pattern

The RS485 master and slave use a different task pattern optimized for precise timing.

### vTaskDelayUntil (Precise Timing)

```cpp
static void rs485Task(void* param) {
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (taskRunning) {
        rs485PollLoop();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(RS485_TASK_TICK_INTERVAL));
    }

    vTaskDelete(nullptr);
}
```

Unlike the TFT gauge tasks which use `vTaskDelay()` (relative delay), the RS485 task uses `vTaskDelayUntil()` (absolute delay). This provides consistent 1ms tick intervals regardless of how long the poll loop takes to execute.

### Graceful Shutdown

The RS485 task uses a `taskRunning` flag for clean shutdown:

```cpp
static volatile bool taskRunning = false;

// In RS485Master_stop():
taskRunning = false;
vTaskDelay(pdMS_TO_TICKS(100));  // Wait for task to exit
vQueueDelete(cmdQueue);
rs485TaskHandle = nullptr;
```

The task checks `taskRunning` at the top of each iteration and calls `vTaskDelete(nullptr)` to delete itself when the flag becomes false.

### Command Queue (Master Only)

In task mode, the master cannot call `sendCommand()` directly from its task (it must run in main loop context). A FreeRTOS queue bridges the gap:

```cpp
struct SlaveCommand { char label[48]; char value[16]; };
static QueueHandle_t cmdQueue;

// Created during init:
cmdQueue = xQueueCreate(RS485_CMD_QUEUE_SIZE, sizeof(SlaveCommand));

// In task context (producer):
xQueueSend(cmdQueue, &cmd, 0);  // Non-blocking

// In main loop (consumer):
while (xQueueReceive(cmdQueue, &cmd, 0) == pdTRUE) {
    sendCommand(cmd.label, cmd.value, false);
}
```

---

## 6. Memory Allocation

### PSRAM vs Internal RAM

TFT gauges allocate large buffers in PSRAM (external SPI RAM) and small DMA buffers in internal RAM:

```cpp
// Background cache -- PSRAM (240x240x2 = 115,200 bytes each)
bgCache[0] = (uint16_t*)heap_caps_aligned_alloc(
    16, FRAME_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

// DMA bounce buffers -- Internal RAM (must be DMA-capable)
dmaBounce[0] = (uint16_t*)heap_caps_aligned_alloc(
    32, STRIPE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
```

**Why the separation:**
- DMA transfers require memory in internal RAM (ESP32 DMA controllers cannot access PSRAM directly on most chips).
- Background images and frame sprites are large and accessed by CPU only, so PSRAM is appropriate.
- Alignment (16 or 32 bytes) ensures cache-line and DMA burst alignment.

### LovyanGFX Sprites

The frame sprite is allocated in PSRAM:

```cpp
frameSpr.setColorDepth(COLOR_DEPTH_BATT);  // 16-bit
frameSpr.setPsram(true);                    // Allocate in PSRAM
frameSpr.createSprite(SCREEN_W, SCREEN_H); // 240x240 = 115,200 bytes
```

Needle sprites are small enough for internal RAM:

```cpp
needleU_spr.setColorDepth(COLOR_DEPTH_BATT);
needleU_spr.createSprite(NEEDLE_W, NEEDLE_H);  // 15x88 = 2,640 bytes
```

### Allocation Failure Handling

If a critical allocation fails, the gauge halts with an infinite delay to prevent undefined behavior:

```cpp
if (!dmaBounce[0] || !dmaBounce[1]) {
    debugPrintf("dmaBounce alloc failed (%u bytes each)\n", (unsigned)STRIPE_BYTES);
    while (1) vTaskDelay(pdMS_TO_TICKS(1000));
}
```

---

## 7. Core Pinning Strategy

### Dual-Core Assignment

On ESP32-S3 (dual-core), tasks are distributed across cores:

| Task | Core | Priority | Rationale |
|------|------|----------|-----------|
| Arduino `loop()` | 1 | 1 | Default Arduino core |
| TFT Gauge tasks | 0 | 2 | Keep rendering off the main loop core |
| IFEI Display | any | 1 | Low priority, SPI-bound |
| RS485 Master | 1 | 24 | Must match WiFi priority for round-robin |
| RS485 Slave | 0 | 5 | Export data processing |
| WiFi | 0/1 | 23 | System task |
| USB/tinyUSB | 0 | 18 | System task |

### Priority Selection Guidelines

- **Priority 1-2** -- display rendering tasks. They are latency-tolerant (a missed frame is invisible).
- **Priority 5** -- slave-side housekeeping (export buffer draining). Not time-critical because RX/TX is ISR-driven.
- **Priority 24** -- master-side poll loop. Must be high enough to avoid starvation by WiFi (23) and USB (18). Uses `vTaskDelayUntil()` with 1ms ticks, consuming less than 1% CPU.

### Single-Core Considerations

On single-core chips, all tasks share one core. Priority becomes critical:
- RS485 at priority 24 ensures it always gets CPU time.
- TFT gauges at priority 2 may occasionally miss frames when the bus is busy.
- The FreeRTOS tick rate (1ms default) provides round-robin time slicing between equal-priority tasks.

---

## 8. Stack Sizing

### Current Stack Sizes

| Task | Stack Size | Contents |
|------|-----------|----------|
| TFT Gauges | 4096 bytes | Draw function locals, dirty-rect math, DMA wait |
| IFEI Display | 4096 bytes | Shadow RAM comparison, SPI commit |
| RS485 Master | 4096 bytes | Poll loop, message parsing, command formatting |
| RS485 Slave | 4096 bytes | Export buffer drain, status printing |

### Stack Usage Estimation

FreeRTOS tracks high-water mark for each task. Use `uxTaskGetStackHighWaterMark(taskHandle)` to check how close a task is to overflowing. The Performance Monitor (`PerfMonitor.cpp`) can print a detailed task list with stack free values.

### When to Increase Stack

- Adding debug printing to a task function (especially `debugPrintf` with format strings).
- Using local arrays or string buffers larger than ~256 bytes.
- Calling functions that allocate on the stack (e.g., LovyanGFX sprite operations).

---

## 9. Task Cleanup

### TFT Gauge Deinit

```cpp
void BatteryGauge_deinit() {
    waitDMADone();  // Ensure no in-flight DMA transfer

    if (tftTaskHandle) {
        vTaskDelete(tftTaskHandle);
        tftTaskHandle = nullptr;
    }

    // Free sprites
    needleU_spr.deleteSprite();
    needleE_spr.deleteSprite();
    frameSpr.deleteSprite();

    // Free PSRAM caches
    if (bgCache[0]) { heap_caps_free(bgCache[0]); bgCache[0] = nullptr; }
    if (bgCache[1]) { heap_caps_free(bgCache[1]); bgCache[1] = nullptr; }

    // Free DMA bounce buffers
    if (dmaBounce[0]) { heap_caps_free(dmaBounce[0]); dmaBounce[0] = nullptr; }
    if (dmaBounce[1]) { heap_caps_free(dmaBounce[1]); dmaBounce[1] = nullptr; }
}
```

**Cleanup order matters:**
1. Wait for DMA to finish -- prevents writing to freed memory.
2. Delete the task -- stops the draw loop.
3. Free sprites -- releases LovyanGFX-managed memory.
4. Free raw allocations -- releases PSRAM and internal RAM.

### RS485 Shutdown

```cpp
void RS485Master_stop() {
    // Stop task first
    taskRunning = false;
    vTaskDelay(pdMS_TO_TICKS(100));  // Give task time to exit

    // Delete command queue
    if (cmdQueue) { vQueueDelete(cmdQueue); cmdQueue = nullptr; }
    rs485TaskHandle = nullptr;

    // Free ISR
    if (intrHandle) { esp_intr_free(intrHandle); intrHandle = nullptr; }

    initialized = false;
}
```

The 100ms delay after setting `taskRunning = false` gives the task time to exit its loop and call `vTaskDelete(nullptr)`. The ISR is freed after the task to prevent the task from accessing hardware after the ISR is gone.

---

## 10. Common Patterns Summary

### Creating a New Task

```cpp
// 1. Define enable macro
#define RUN_MY_FEATURE_AS_TASK 1

// 2. Declare shared state as volatile
static volatile bool myDirtyFlag = false;
static volatile int16_t myValue = 0;
static TaskHandle_t myTaskHandle = nullptr;

// 3. Write the task function
static void myTask(void*) {
    for (;;) {
        if (myDirtyFlag) {
            myDirtyFlag = false;
            // ... do work ...
        }
        vTaskDelay(pdMS_TO_TICKS(5));  // or vTaskDelayUntil for precise timing
    }
}

// 4. Create during init
#if RUN_MY_FEATURE_AS_TASK
xTaskCreatePinnedToCore(myTask, "MyTask", 4096, nullptr, 2, &myTaskHandle, 0);
#endif

// 5. Fallback to main loop when not a task
void myFeature_loop() {
    #if !RUN_MY_FEATURE_AS_TASK
    // Same work as the task, called from panel loop hook
    if (myDirtyFlag) {
        myDirtyFlag = false;
        // ... do work ...
    }
    #endif
}

// 6. Clean up
void myFeature_deinit() {
    if (myTaskHandle) { vTaskDelete(myTaskHandle); myTaskHandle = nullptr; }
}
```

### Checklist

- Use `volatile` for all data shared between tasks or between a task and an ISR.
- Use `heap_caps_aligned_alloc()` for DMA buffers (must be in internal RAM).
- Use `MALLOC_CAP_SPIRAM` for large non-DMA buffers.
- Pin rendering tasks to core 0 on dual-core chips to keep core 1 free for the main loop.
- Use `vTaskDelay()` for render loops (periodic, tolerant of jitter).
- Use `vTaskDelayUntil()` for protocol tasks (precise timing required).
- Always delete tasks before freeing their shared memory.
- Store task handles so you can delete them during deinit.

---

## See Also

- [Advanced/Display-Pipeline.md](Display-Pipeline.md) -- TFT gauge rendering architecture
- [Advanced/RS485-Deep-Dive.md](RS485-Deep-Dive.md) -- RS485 task integration details
- [Advanced/Custom-Panels.md](Custom-Panels.md) -- Panel lifecycle hooks
- [Reference/Config.md](../Reference/Config.md) -- Task-related configuration constants
