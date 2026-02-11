# CockpitOS Performance Audit — Complete Findings
**Date:** 2026-02-10
**Branch:** dev (commit e77ddaa)
**Scope:** FULL CODEBASE — every subsystem reviewed

---

## RESULT: Only 2 meaningful improvements found across the entire codebase

After reviewing every subsystem, driver, transport layer, panel implementation, and runtime hot path in CockpitOS, only two issues were identified that would produce a real-world performance improvement. Everything else is already well-optimized.

---

## Finding 1: `findCmdEntry()` — O(N) Linear Search → Hash Table

**Severity:** HIGH
**Status:** TODO (code already has comment: "O(N) for now, can swap to hash later")

### What

`findCmdEntry()` in `src/Core/DCSBIOSBridge.cpp` line 928 does a linear O(N) scan through `commandHistory[]` using `strcmp()`.

```cpp
// CURRENT CODE — src/Core/DCSBIOSBridge.cpp lines 928-935
CommandHistoryEntry* findCmdEntry(const char* label) {
    for (size_t i = 0; i < commandHistorySize; ++i) {
        if (strcmp(commandHistory[i].label, label) == 0) {
            return &commandHistory[i];
        }
    }
    return nullptr;
}
```

### Why it matters

Called on multiple hot paths at high frequency:
- `onSelectorChange()` (DCSBIOSBridge.cpp line 586) — every DCS selector update @ 30Hz
- `sendDCSBIOSCommand()` (DCSBIOSBridge.cpp line 1454) — every outgoing command
- `getLastKnownState()` (DCSBIOSBridge.cpp line 913) — selector validation queries
- `HIDManager.cpp` (lines 533, 643, 730, 748) — panel polling @ 250Hz
- `flushBufferedDcsCommands()` (DCSBIOSBridge.cpp lines 955-1019) — multiple O(N) passes per flush @ 250Hz

With 21 entries (FA-18C), this produces thousands of unnecessary `strcmp` calls/sec.

### How to fix — Step by step

#### Step 1: Modify the auto-generator

**File:** `src/LABELS/_core/generator_core.py`

After the existing `commandHistory[]` array generation block, add hash table generation:

1. Calculate hash table size: `next_prime(len(unique_commands) * 2)` (same helper already exists in the file)
2. Build hash table entries using `djb2_hash` (same hash function already used for other label tables)
3. Emit a new struct and table:
```cpp
struct CmdHistoryHashEntry { const char* label; CommandHistoryEntry* entry; };
static CmdHistoryHashEntry cmdHistoryHashTable[PRIME_SIZE] = { ... };
```
4. Emit an inline lookup function with **correct** early termination:
```cpp
inline CommandHistoryEntry* findCmdEntry(const char* label) {
    uint16_t startH = labelHash(label) % TABLE_SIZE;
    for (uint16_t i = 0; i < TABLE_SIZE; ++i) {
        uint16_t idx = (startH + i >= TABLE_SIZE) ? (startH + i - TABLE_SIZE) : (startH + i);
        const auto& entry = cmdHistoryHashTable[idx];
        if (!entry.label) return nullptr;  // <-- CORRECT: early termination
        if (strcmp(entry.label, label) == 0) return entry.entry;
    }
    return nullptr;
}
```

**Reference pattern to follow:** Look at `InputMapping.h` (generated) — the `inputHashTable[]` and `findInputByLabel()` are the exact template. Copy the structure, just change the types.

#### Step 2: Remove the manual function

**File:** `src/Core/DCSBIOSBridge.cpp`

Delete lines 925-935 (the `// Lookup helper` comment and the manual `findCmdEntry()` function). The auto-generated inline version in `DCSBIOSBridgeData.h` will replace it. Also remove or update the forward declaration if one exists.

#### Step 3: Regenerate all label sets

Run `generate_data.py` in every `LABEL_SET_*` directory to regenerate the headers with the new hash table.

---

## Finding 2: Null-Skip Bug in Label Hash Table Probing

**Severity:** MEDIUM (correctness + performance)
**Status:** BUG — causes O(N) full-table scan on every cache miss

### What

The label-based hash tables use `continue` when encountering a null slot during linear probing. This is **incorrect** for open-addressing — a null slot means the probe chain has ended and the key does not exist.

```cpp
// BROKEN (current) — skips nulls, keeps scanning entire table on miss:
if (!entry.label) continue;

// CORRECT (address hash already does this right):
if (entry.addr == 0xFFFF) return nullptr;  // empty slot = end of chain
```

### Why it matters

For any label NOT in the table, the lookup must scan the **entire** table before returning `nullptr`. Table sizes range from 2 to 97 entries. A single miss on a 97-entry table = up to 97 `strcmp` calls instead of 1-2 probes.

This happens when:
- A DCS label has no matching input/LED mapping (normal for many controls)
- `CT_DISPLAY` dispatch calls `findDisplayBufferByLabel()` + `findDisplayFieldByLabel()` for every changed display byte at 30Hz
- Any label mismatch during debug/diagnostic queries

### How to fix — Step by step

#### Step 1: Fix the generator templates

**File:** `src/LABELS/_core/generator_core.py`

Search for every occurrence of:
```python
f.write("    if (!entry.label) continue;\n")
```

Replace with:
```python
f.write("    if (!entry.label) return nullptr;\n")
```

There should be instances in the generation code for these lookup functions:
- `findInputByLabel()` — generates into `InputMapping.h`
- `findLED()` — generates into `LEDMapping.h`
- `findDisplayFieldByLabel()` — generates into `DCSBIOSBridgeData.h`

#### Step 2: Also check the display generator

**File:** `src/LABELS/_core/display_gen_core.py`

Same search-and-replace for:
- `findDisplayBufferByLabel()` — generates into `CT_Display.cpp`
- `findFieldDefByLabel()` — generates into `DisplayMapping.cpp`

#### Step 3: Regenerate all label sets

Run `generate_data.py` in every `LABEL_SET_*` directory.

#### Why this is safe

The generator places entries using linear probing from the hash position. A null slot guarantees no entry in this probe chain was placed beyond it. This is the fundamental invariant of open-addressing hash tables.

---

## Full Audit Coverage — Everything Else Is Already Optimized

The following subsystems were fully reviewed and found to be **clean** (no meaningful improvements possible):

### Core Runtime
| File | Lines | Verdict |
|------|-------|---------|
| `src/Core/DCSBIOSBridge.cpp` | ~1808 | Findings #1 and #2 only; address hash O(1), g_prevValues dedup, ring buffer drain all optimal |
| `src/Core/InputControl.cpp` | ~1152 | Pre-resolved flat tables for PCA/HC165/TM1637/Matrix; GPIO selectors acceptable at current scale |
| `src/Core/LEDControl.cpp` | ~217 | Hash-based dispatch via findLED(), boolean-gated driver ticks, zero waste |
| `src/Core/HIDManager.cpp` | ~900+ | memcmp dedup for BLE/HID, hash-based button dispatch, 5ms USB timeout correct |
| `src/Core/RingBuffer.cpp` | ~274 | SPSC ring buffer, volatile head/tail, all-or-nothing chunked push, stats tracking |
| `src/Core/WiFiDebug.cpp` | ~230 | AsyncUDP multicast, interrupt-context push to ring buffer |
| `src/Core/Private/BLEManager.cpp` | ~1142 | Critical-section snapshot, dirty flag retry, TOCTOU fix correct |

### DCS-BIOS Protocol
| File | Lines | Verdict |
|------|-------|---------|
| `lib/DCS-BIOS/src/internal/Protocol.cpp` | ~107 | 7-state FSM, one branch + one assignment per byte, textbook optimal |
| `lib/DCS-BIOS/src/internal/ExportStreamListener.h` | ~45 | Sorted linked list, only 1 listener = O(1) dispatch |

### Hardware Drivers (lib/CUtils)
| File | Lines | Verdict |
|------|-------|---------|
| `HT1622.cpp` | ~326 | gpio_set_level (direct register), partial commit, critical sections for BLE coexistence |
| `TM1637.cpp` | ~230 | needsUpdate batching, protocol-bound at 3μs/bit |
| `GN1640.cpp` | ~243 | Shadow buffer with per-row dirty tracking, 2ms debounce |
| `PCA9555.cpp` | ~743 | Minimum I2C transactions, cached port states avoid read-modify-write round trips |
| `HC165.cpp` | ~92 | Protocol-bound shift register read, no faster approach possible |
| `GPIO.cpp` | ~107 | Thin passthrough to digitalWrite/analogWrite |
| `AnalogG.cpp` | ~53 | Software servo pulse, rate-limited, blocking by design |
| `MatrixRotary.cpp` | ~16 | Strobe-and-read, protocol-bound |
| `WS2812.cpp` | ~448 | RMT hardware-accelerated DMA, zero CPU during wire TX, static buffers |

### Panel Implementations
| File | Lines | Verdict |
|------|-------|---------|
| `src/Panels/IFEIPanel.cpp` | ~1445 | Shadow buffer diff, rate-limited commit, strcmp only on infrequent mode transitions |
| `src/Panels/TFT_Gauges_RadarAlt.cpp` | ~643 | Dirty-rect AABB, double-buffered DMA striping, PSRAM background cache, dedicated FreeRTOS task |

### Transport / USB
| File | Lines | Verdict |
|------|-------|---------|
| `src/HIDDescriptors.h` | ~111 | GPDevice class, ring buffer drain on GET_FEATURE |
| `src/CustomDescriptors/BidireccionalNew.h` | ~82 | 64-byte reports, no report IDs on input, static_assert on size |
| `HID Manager/HID_Manager.py` | ~878 | Per-device reader+writer threads, no locks on HID I/O, 4MB socket buffer |

### Auto-Generator Pipeline
| File | Lines | Verdict |
|------|-------|---------|
| `src/LABELS/_core/generator_core.py` | ~1399 | Findings #1 and #2 originate here; djb2 hash, ~50% load factor, prime-sized tables all correct |

---

## NOT Worth Optimizing (Evaluated and Dismissed)

- **SelectorMap linear scan** (`DCSBIOSBridge.cpp` line 603): O(40) but only on physical switch flips (<5/sec)
- **`pollGPIOSelectors()` runtime strcmp** (`InputControl.cpp`): Multiple passes through InputMappings[], but most groups are empty (early continue). Only 2-4 active GPIO groups in practice
- **`lookup7SegToChar()` / `lookup14SegToChar()`** (`IFEIPanel.cpp`): O(128) linear scan but only called from DEBUG path
- **`findFieldDefByLabel()` in T-mode transitions** (`IFEIPanel.cpp`): Up to 7 lookups but only on pilot pressing T button (once per sortie)
- **`findHidMappingByDcs()`** (`HIDManager.cpp` line 341): O(41) linear scan, only called from flush (infrequent)
- **`TM1637_findByPins()`** (`LEDControl.cpp`): Linear scan through max 8 devices
- **`AnalogG_set()`**: Linear scan through max `MAX_GAUGES` entries
- **All hardware driver timing**: Protocol-bound (I2C bus speed, SPI timing specs, WS2812 signal timing) — the chip datasheets define the speed floor, not the code
