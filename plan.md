# Plan: Fix pendingUpdates ordering bug in DCSBIOSBridge.cpp

## The One Real Finding

**M2: `g_prevValues` updated before `pendingUpdates` queue check**
- File: `src/Core/DCSBIOSBridge.cpp`, lines 272-295
- Bug: `g_prevValues[index] = val` runs at line 274, before the queue-full check at lines 281/290. If the queue overflows, the dropped update is permanently suppressed because `g_prevValues` already matches the new value.
- Impact: LEDs/gauges stuck at wrong state until the sim value changes again. Only triggers when pendingUpdateCount >= 220 on a single frame (unlikely with current label sets but possible with multiple large panels).
- Fix: Move `g_prevValues[index] = val` inside the successful-queue branch, for both CT_LED/CT_GAUGE/CT_ANALOG and CT_SELECTOR cases.

## Changes

### `src/Core/DCSBIOSBridge.cpp` (lines 272-295)

Move `g_prevValues` update from before the switch to inside each successful queue path:

**Before:**
```cpp
if (entry->controlType != CT_DISPLAY) {
    if (index >= DcsOutputTableSize || g_prevValues[index] == val) continue;
    g_prevValues[index] = val;            // <-- updates before queue check
}

switch (entry->controlType) {
    case CT_GAUGE:
    case CT_LED:
    case CT_ANALOG:
        if (pendingUpdateCount < MAX_PENDING_UPDATES) {
            pendingUpdates[pendingUpdateCount++] = {entry->label, val, entry->max_value};
        } else {
            pendingUpdateOverflow++;
        }
        break;
    case CT_SELECTOR:
        onSelectorChange(entry->label, val);
        if (pendingUpdateCount < MAX_PENDING_UPDATES) {
            pendingUpdates[pendingUpdateCount++] = {entry->label, val, entry->max_value};
        } else {
            pendingUpdateOverflow++;
        }
        break;
```

**After:**
```cpp
if (entry->controlType != CT_DISPLAY) {
    if (index >= DcsOutputTableSize || g_prevValues[index] == val) continue;
    // g_prevValues update moved into successful queue paths below
}

switch (entry->controlType) {
    case CT_GAUGE:
    case CT_LED:
    case CT_ANALOG:
        if (pendingUpdateCount < MAX_PENDING_UPDATES) {
            g_prevValues[index] = val;
            pendingUpdates[pendingUpdateCount++] = {entry->label, val, entry->max_value};
        } else {
            pendingUpdateOverflow++;
        }
        break;
    case CT_SELECTOR:
        onSelectorChange(entry->label, val);
        if (pendingUpdateCount < MAX_PENDING_UPDATES) {
            g_prevValues[index] = val;
            pendingUpdates[pendingUpdateCount++] = {entry->label, val, entry->max_value};
        } else {
            pendingUpdateOverflow++;
        }
        break;
```

This ensures dropped updates are retried on the next frame (g_prevValues still shows the old value, so the change detection fires again).

## What was retracted and why (for future Claude reviews)

### False positive categories to avoid:

1. **Config-bounded bugs**: Always check actual default config values. The developer sizes buffers intentionally (e.g., RS485_RELAY_CHUNK_SIZE=124 to fit FIFO).
2. **Hardware-model bugs**: Check which ESP32 variants actually support the feature. WiFi ring buffer is irrelevant on single-core RISC-V because there's no cross-core reordering.
3. **Small-N linear scans**: MAX_LATCHED_BUTTONS=16, MAX_*_SUBSCRIPTIONS=32. Linear scans of <=32 short strings at human input rates are negligible. Don't flag these.
4. **Static buffers consumed immediately**: If a static buffer is written by snprintf and consumed by the very next function call with no reentry path, it's safe. Don't flag.
5. **Error-path races**: Timeout handlers modifying ISR state during error recovery is correct behavior — we're already handling a fault.
6. **Self-correcting parser issues**: Protocol parsers with sync detection recover automatically. A corrupted count costs one frame (~33ms), not a crash.
7. **Dead code in hash tables**: Generator putting extra entries that are always shadowed by correct entries is wasted space, not a bug.
8. **ISR races on single-core**: On single-core chips, ISRs preempt the main context but never run concurrently. Volatile is sufficient for SPSC patterns.

### The 25→1 scorecard:
- 4 CRITICAL: All false positives (config already prevents, hardware doesn't support, design intentional)
- 6 HIGH: All false positives (negligible N, error paths, dead code)
- 15 MEDIUM: 14 false positives, 1 valid (pendingUpdates ordering)
