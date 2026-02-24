# CockpitOS Bug Report

**Date:** 2026-02-24
**Branch:** `claude/review-codebase-bugs-3sBAD`
**Label Set Compiled:** MAIN
**Build Result:** PASS (0 CockpitOS warnings, 71 framework-only warnings)
**Flash:** 1,051,719 bytes (50%) | **RAM:** 103,296 bytes (31%)

---

## Critical

### C1. PCA9555_cachedPortStates indexed by raw I2C address -- out-of-bounds write

**File:** `lib/CUtils/src/internal/PCA9555.cpp:101-102, 197-199, 362-363, 432-436`
**Also:** `lib/CUtils/src/CUtils.cpp:36`, `lib/CUtils/src/CUtils.h:113`

`PCA9555_cachedPortStates` is declared as `uint8_t [8][2]` and the comment says "indexed by (address - 0x20)". But the code indexes it with the **raw** I2C address:

```cpp
PCA9555_cachedPortStates[addr][0] = 0xFF;  // addr is 0x20-0x27 (32-39)!
```

PCA9555 I2C addresses are 0x20-0x27. Using `addr` (32-39) as an index into an 8-element array writes 24-31 elements past the end. This corrupts adjacent static variables (`pcaLabelBuf`, `pcaConfigCache`, etc.) on every PCA write.

**Impact:** Silent memory corruption affecting any build that uses PCA9555 I/O expanders. Could manifest as wrong LED states, phantom button presses, or random crashes depending on linker layout.

**Fix:** Use `addr - 0x20` as the index everywhere, or validate at the callsite:
```cpp
PCA9555_cachedPortStates[addr - 0x20][port] |= (1 << bit);
```

---

### C2. HIDManager per-pin arrays sized 40, but GPIO pins can reach 48 on ESP32-S3

**File:** `src/Core/HIDManager.cpp:379-382, 756-761`

```cpp
static int lastFiltered[40] = { 0 };     // line 379
static int lastOutput[40] = { -1 };      // line 380
static unsigned int stabCount[40] = { 0 }; // line 381
static bool stabilized[40] = { false };   // line 382
// ...
static bool g_bootstrapped[64] = { false }; // line 729 -- different size!
```

All five arrays are indexed by `pin` (GPIO number). On ESP32-S3, valid analog-capable GPIOs go up to 20, but other GPIOs go up to 48. The 40-element arrays overflow for any pin >= 40, while `g_bootstrapped[64]` would pass the bootstrap check and allow the writes.

**Impact:** Silent memory corruption. Writing past the end of `lastFiltered[40]` corrupts `lastOutput`, `stabCount`, `stabilized`, or other adjacent statics. Any label set with analog inputs on GPIO >= 40 triggers this.

**Fix:** Unify all five arrays to the same size (64), or add a bounds check:
```cpp
if (pin >= 64) return;  // or use a constant MAX_ANALOG_PINS
```

---

## Important

### I1. buildHIDGroupBitmasks missing bounds check on group ID

**File:** `src/Core/HIDManager.cpp:386-389`

```cpp
void buildHIDGroupBitmasks() {
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        if (m.group > 0 && m.hidId > 0) {
            groupBitmask[m.group] |= (1UL << (m.hidId - 1));  // no bounds check!
```

`groupBitmask` is sized `MAX_GROUPS` (128). If any InputMapping has `group >= 128`, this writes past the array. `flushBufferedDcsCommands()` checks and calls `abort()`, but `buildHIDGroupBitmasks` runs first during setup without a guard.

**Impact:** Memory corruption during setup if an auto-generated InputMapping has a group ID >= 128.

**Fix:** Add `if (m.group >= MAX_GROUPS) continue;` before the write.

---

### I2. HIDManager_resetAllAxes only resets 40 slots, does not reset g_bootstrapped

**File:** `src/Core/HIDManager.cpp:579-589`

```cpp
void HIDManager_resetAllAxes() {
    axCalibInit();
    for (int i = 0; i < 40; ++i) {   // should be 64
        stabCount[i] = 0;
        stabilized[i] = false;
        lastOutput[i] = -1;
    }
    // g_bootstrapped[] and lastFiltered[] are NOT reset
}
```

After a mission restart, pins 40-63 retain stale state, and the EMA filter seed is never re-applied.

**Fix:** Reset all five arrays with unified size. Include `g_bootstrapped` and `lastFiltered`.

---

### I3. Config.h: ESP32-P4 missing from DEVICE_HAS_WIFI guard

**File:** `Config.h:293-299`

The comment says "ESP32-P4: no Wi-Fi / no BT" but only ESP32-H2 is excluded from `DEVICE_HAS_WIFI`:

```cpp
#if (defined(ESP_FAMILY_H2))        // Missing: || defined(ESP_FAMILY_P4)
    #define DEVICE_HAS_WIFI 0
#else
    #define DEVICE_HAS_WIFI 1       // P4 incorrectly gets WiFi=1
#endif
```

**Impact:** On ESP32-P4, WiFi debug code compiles and likely crashes at runtime (no WiFi radio). The `USE_DCSBIOS_WIFI` guard catches the transport case but not `DEBUG_USE_WIFI`.

**Fix:** Add `|| defined(ESP_FAMILY_P4)` to the guard.

---

### I4. CockpitOS.ino: checkHealth() bare spin loop starves watchdog

**File:** `CockpitOS.ino:207`

```cpp
if (getMaxUsedGroup() >= MAX_GROUPS) {
    debugPrintf("FATAL: group ID >= MAX_GROUPS...");
    while (true);  // bare spin, no delay()
}
```

This blocks the main task without feeding the watchdog, creating an infinite boot loop. The startup watchdog task would eventually trigger bootloader mode after 30 seconds, but diagnosis is difficult.

**Impact:** Device enters hard boot loop with brief flashing error message. Same pattern at line 268 for WiFi+HID conflict.

**Fix:** Replace `while (true);` with `while (true) { delay(1000); }` to feed watchdog, or call `enterBootloaderMode()` directly.

---

### I5. TFT gauge panels: race condition between DCS-BIOS callbacks and draw task

**File:** All TFT gauge panels (Battery, BrakePress, CabinPressure, CabPressTest, HydPress, RadarAlt)

DCS-BIOS callbacks write `volatile` state variables (e.g., `angleU`, `gaugeDirty`, `currentLightingMode`) on Core 1, while the draw task reads them on Core 0. `volatile` prevents compiler reordering but does NOT provide memory ordering on dual-core ESP32-S3.

The `gaugeDirty` flag is cleared in the draw task but set concurrently by callbacks. Without a memory barrier, the `false` write can clobber a concurrent `true` write, losing an update.

**Impact:** Occasional visual glitches (missed needle updates, wrong background for one frame). Not a crash, but observable.

**Fix:** Use `portMEMORY_BARRIER()` after grouped writes in callbacks and before reads in draw task, or use a single atomic bitmask for dirty flags.

---

### I6. TFT_Gauges_RadarAlt: vTaskDelete before waitDMADone

**File:** `src/Panels/TFT_Gauges_RadarAlt.cpp:569`

`RadarAlt_deinit()` calls `vTaskDelete(tftTaskHandle)` BEFORE `waitDMADone()`, unlike all other gauge panels which correctly wait for DMA first. This kills the draw task while a DMA transfer may be in progress.

**Impact:** Calling `RadarAlt_deinit()` can corrupt SPI bus state, potentially locking up the SPI peripheral.

**Fix:** Swap the order -- call `waitDMADone()` before `vTaskDelete()`.

---

### I7. IFEIPanel.cpp: copyN can overflow tempL_base/tempR_base buffers

**File:** `src/Panels/IFEIPanel.cpp:678, 684`

`tempL_base` is `char[4]` (line 344). `copyN(tempL_base, value, def.numDigits + 1)` copies `numDigits + 1` bytes. If `numDigits` is 3, this copies 4 bytes -- exact fit. If any auto-generated field has `numDigits > 3`, this overflows.

**Impact:** Stack buffer overflow corrupting adjacent `volatile bool` flags, causing wrong display behavior.

**Fix:** Add bounds check: `if (def.numDigits + 1 <= sizeof(tempL_base)) copyN(...);`

---

### I8. logExpanderState signature mismatch between header and implementation

**File:** `lib/CUtils/src/CUtils.h:127` vs `lib/CUtils/src/internal/PCA9555.cpp:690`

Header declares: `void logExpanderState(uint8_t p0, uint8_t p1);`
Implementation has: `void logExpanderState(uint8_t p0, uint8_t p1, char* buffer, size_t buflen);`

**Impact:** Any external code calling the 2-parameter version gets a linker error. The header provides a dead declaration.

**Fix:** Update the header to match the implementation signature.

---

### I9. pcaConfigCache partial initialization

**File:** `lib/CUtils/src/internal/PCA9555.cpp:2`

```cpp
static uint8_t pcaConfigCache[128][2] = { {0xFF, 0xFF} };
```

Only initializes `[0][0]` and `[0][1]` to 0xFF. All other 127 entries default to 0x00 (C++ zero-init). Intent is clearly all entries = 0xFF (all pins as input).

**Impact:** `logPCA9555State` incorrectly reports all pins as outputs for PCA addresses > 0 until the cache is updated. Misleading debug output.

**Fix:** `memset(pcaConfigCache, 0xFF, sizeof(pcaConfigCache));` in an init function.

---

### I10. HT1622 commitBurst holds critical section for ~2.5ms

**File:** `lib/CUtils/src/internal/HT1622.cpp:187-203`

`commitBurst()` wraps its entire 64-nibble bit-bang transfer in `portENTER_CRITICAL / portEXIT_CRITICAL`, disabling ALL interrupts for ~2.5ms. With 2+ HT1622 displays committing in the same cycle, total interrupt-disabled time can reach 5+ms.

**Impact:** Dropped RS485 bytes, potential TWDT trigger on multi-display setups. `commitPartial()` already does incremental updates but `commitBurst` doesn't use it.

**Fix:** Route `commit()` through `commitPartial()`, or break `commitBurst` into chunks with interrupt windows.

---

### I11. RS485 Master broadcastBuffer overflow potential in SMART mode

**File:** `lib/CUtils/src/internal/RS485Master.cpp:637-654`

`broadcastBuffer` is 256 bytes. The loop guard `broadcastLen < RS485_MAX_BROADCAST_CHUNK` only checks at loop start. Each iteration writes 10 bytes, so the actual write can reach `RS485_MAX_BROADCAST_CHUNK + 9`. With default chunk size of 64 this is safe, but there's no compile-time guard against misconfiguration.

**Impact:** Overflow if `RS485_MAX_BROADCAST_CHUNK` is set above 246.

**Fix:** Add `if (broadcastLen + 10 > sizeof(broadcastBuffer)) break;` inside the loop. Add `static_assert(RS485_MAX_BROADCAST_CHUNK + 10 <= 256)`.

---

### I12. HID descriptor declares 16 axes but only 8 have Usage IDs

**File:** `src/CustomDescriptors/BidireccionalNew.h:16-38`

The HID descriptor has `Report Count 16` but only 8 Usage IDs (X, Y, Z, Rx, Ry, Rz, Dial, Slider). The other 8 are commented out. The code (`HIDAxis` enum, `report.axes[16]`) supports all 16, but the host OS can only meaningfully interpret the first 8.

**Impact:** Axes 8-15 (AXIS_SLIDER1 through AXIS_SLIDER8) are present in the USB report but invisible to most host drivers. Label sets using >8 analog axes will silently lose them.

**Fix:** Either uncomment the remaining 8 Usage IDs in the descriptor, or limit `HID_AXIS_COUNT` to 8.

---

### I13. WingFold.cpp: PCA address hardcoded to 0x26

**File:** `src/Panels/WingFold.cpp:68-69`

```cpp
g_addr = 0x26;  // hardcoded
g_port = 0;     // hardcoded
```

`resolveWingFoldFromMappings()` finds bit positions from InputMappings but never extracts the PCA address from `m.source` (e.g., `"PCA_0x24"`).

**Impact:** WingFold panel only works on PCA address 0x26. Any other address configuration reads from the wrong I2C device.

**Fix:** Parse the address from `m.source` like `_collectPcaAddresses` does in `Mappings.cpp`.

---

## Minor

### M1. lastValSelector partial initialization

**File:** `src/Core/InputControl.cpp:450`

```cpp
int16_t lastValSelector[MAX_SELECTOR_GROUPS][MAX_PCAS] = { { -1 } };
```

Only `[0][0]` is initialized to -1. All other entries are 0. If a selector's `oride_value` is 0, the first non-forced poll won't detect it as a change.

**Fix:** `memset(lastValSelector, 0xFF, sizeof(lastValSelector));` in `buildPCA9555ResolvedInputs()`.

---

### M2. Static char buffers in sendDCSBIOSCommand and HIDManager

**File:** `src/Core/DCSBIOSBridge.cpp:1585`, `src/Core/HIDManager.cpp:373, 875, 943, 1143`

Multiple functions use `static char buf[10]` for formatting DCS command values. While safe in the current single-threaded flow (data is consumed before return), the `static` keyword is unnecessary and creates a latent reentrancy risk.

**Fix:** Replace `static char buf[10]` with `char buf[10]` (stack-local). The 10-byte cost is negligible.

---

### M3. Pointer comparison instead of strcmp in queueDeferredRelease

**File:** `src/Core/HIDManager.cpp:355`

```cpp
if (s_pendingRelease[i].active && s_pendingRelease[i].label == label) {
```

Uses pointer identity instead of `strcmp`. Works because all labels come from static InputMapping table entries, but would break with dynamically-sourced labels.

**Fix:** Use `strcmp(s_pendingRelease[i].label, label) == 0`.

---

### M4. frameCounter is uint64_t, non-atomic on 32-bit ESP32

**File:** `src/Core/DCSBIOSBridge.cpp:181`

`uint64_t frameCounter` requires two 32-bit reads. If read from a different core/task, a torn value is possible.

**Fix:** Use `uint32_t` (sufficient for counting frames) which is atomic on ESP32.

---

### M5. Config.h: DEBUG_PERFORMANCE redefinition without #undef

**File:** `Config.h:57, 307-309`

`DEBUG_PERFORMANCE` is defined on line 57, then conditionally redefined on line 308 without `#undef`. If the user sets line 57 to 0 but `VERBOSE_PERFORMANCE_ONLY` to 1, the compiler emits a redefinition warning.

**Fix:** Add `#undef DEBUG_PERFORMANCE` before the re-`#define` on line 308.

---

### M6. Generic.cpp: hc165Bits dead variable

**File:** `src/Panels/Generic.cpp:35`

`hc165Bits` is initialized in `Generic_init()` but never read in the loop (replaced by local `newBits`).

**Fix:** Remove the dead variable.

---

### M7. CockpitOS.ino: Duplicate #include "src/HIDManager.h"

**File:** `CockpitOS.ino:18, 20`

Same file included twice. Harmless due to `#pragma once` but indicates copy-paste.

**Fix:** Remove the duplicate on line 20.

---

### M8. parseHexByte guard too permissive

**File:** `lib/CUtils/src/CUtils.cpp:185-188`

```cpp
if (!s || strlen(s) < 3) return 0;
return (hexNib(s[2]) << 4) | hexNib(s[3]);  // reads s[3] on 3-char string
```

The guard accepts length-3 strings (e.g., "0xA") but reads `s[3]` which is the null terminator. `hexNib('\0')` returns 0 so "0xA" returns 0xA0 instead of 0x0A.

**Fix:** Change guard to `strlen(s) < 4`.

---

### M9. RS485 Slave skipRemaining overflow for length >= 254

**File:** `lib/CUtils/src/internal/RS485Slave.cpp:537`

`skipRemaining = c + 2` with both as `uint8_t`. If `c >= 254`, the result wraps. Unlikely with typical RS485 payload sizes but possible with malformed packets.

**Fix:** Use `uint16_t` for `skipRemaining`, or cap: `skipRemaining = min((uint16_t)(c + 2), (uint16_t)255);`

---

### M10. RS485 Master txByte blocks indefinitely

**File:** `lib/CUtils/src/internal/RS485Master.cpp:507`

```cpp
while (uart_ll_get_txfifo_len(RS485_UART_DEV) == 0) {}
```

Spins with no timeout. A UART hardware fault would hang the system and trigger watchdog.

**Fix:** Add a timeout counter or `delayMicroseconds(1)` with a max iteration count.

---

### M11. displayedIndexes buffer overflow in LED test menu

**File:** `lib/CUtils/src/CUtils.cpp:26, 102`

```cpp
static int displayedIndexes[128];
// ...
displayedIndexes[displayedCount++] = i;  // no bounds check
```

If a label set has >128 LEDs, this overflows. Development-only feature (`TEST_LEDS=1`).

**Fix:** Add `if (displayedCount >= 128) break;`.

---

### M12. GN1640 uses global pins, only one device supported

**File:** `lib/CUtils/src/internal/GN1640.cpp:16, 227-230`

`GN1640_detect()` overwrites global `gn1640_clkPin` and `gn1640_dioPin`. If called during hardware probing with wrong pins, all subsequent GN1640 commands go to wrong GPIOs.

**Fix:** `GN1640_detect` should save and restore global pins, or use local variables for probing.

---

### M13. Cumulative blocking delays in setup() can approach watchdog timeout

**File:** `Mappings.cpp:260-294`, `src/Panels/IFEIPanel.cpp:421-446`, `src/Panels/ServoTest.cpp:50-53`

`initializeLEDs()` has 1s `delay()` per device type (TM1637, GN1640, WS2812, each PCA, GPIOs). IFEI lamp test adds 3s. ServoTest adds 4s. With many devices, total can approach the 30s startup watchdog timeout.

**Fix:** Reduce flash/sweep durations to 250-500ms each.

---

### M14. axCalibLoad unsigned underflow in debug log

**File:** `src/Core/HIDManager.cpp:128`

`axMax[i] - axMin[i]` underflows if NVS data is corrupted and `max < min`. Cosmetic (debug output only).

**Fix:** Guard: `axMax[i] > axMin[i] ? axMax[i] - axMin[i] : 0`.

---

### M15. RingBuffer.h: DcsRawUsbOutRingMsg uses DCS_UDP_PACKET_MAXLEN for data field

**File:** `src/RingBuffer.h:27`

The outgoing USB ring message struct uses `DCS_UDP_PACKET_MAXLEN` (which is 1472 in BLE mode) for its data field, but the clamp in push uses `DCS_USB_PACKET_MAXLEN` (64). In BLE mode, each of the 32 ring slots wastes ~1400 bytes.

**Fix:** Use `DCS_USB_PACKET_MAXLEN` for the outgoing struct's data field.

---

## Generator / Toolchain

### G1. generator_core.py: sys.exit(1) on successful completion

**File:** `src/LABELS/_core/generator_core.py:1613`

```python
sys.exit(1)  # exits with error code even on success
```

The CI works around this with `|| true` + `.last_run` file check. Any tool that checks the exit code will think the generator failed.

**Fix:** Change to `sys.exit(0)`.

---

## Compile Results

- **Build:** PASS
- **CockpitOS code warnings:** 0
- **Framework warnings:** 71 (all from ESP-IDF, TinyUSB, Arduino libraries -- not actionable)
- **Flash usage:** 1,051,719 / 2,097,152 bytes (50%)
- **RAM usage:** 103,296 / 327,680 bytes (31%)
