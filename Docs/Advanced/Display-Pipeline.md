# Display Pipeline

Deep dive into the CockpitOS display system architecture. This document covers the data flow from DCS-BIOS export stream to physical display hardware, the HT1622 shadow RAM system, SegmentMap architecture, DisplayMapping, and the TFT gauge pipeline.

---

## 1. Display Data Flow

```
DCS World
   |
   | (UDP / USB / Serial / RS485)
   v
DCS-BIOS Export Stream (binary: address + value pairs)
   |
   v
DcsBiosSniffer::onDcsBiosWrite()
   |
   +-- CT_DISPLAY entries: write bytes into registered display buffers
   |
   v
DcsBiosSniffer::onConsistentData()  (end-of-frame)
   |
   +-- Compare buffer vs. last: if changed, call onDisplayChange()
   |
   v
onDisplayChange(label, value)
   |
   +-- renderField(label, value)  -->  look up DisplayFieldDefLabel
   |                                   call renderFunc() with driver + segMap
   |
   +-- Dispatch to subscribers (subscribeToDisplayChange callbacks)
   |
   v
Panel-specific render function (e.g. renderIFEIDispatcher)
   |
   +-- Write segment patterns into shadow RAM buffer
   |
   v
Display commit (disp_loop hook)
   |
   +-- Compare shadow RAM vs. last committed: write only changed bytes
   |
   v
Physical Display Hardware (HT1622 via SPI, TFT via SPI+DMA)
```

### Frame Timing

DCS-BIOS sends export frames at approximately 30 Hz. Each frame contains address/value pairs for all active controls. The `onDcsBiosWrite()` callback fires for every address that matches an entry in `DcsOutputTable`. The `onConsistentData()` callback fires once at the end of each complete frame.

Display buffers accumulate byte-level writes throughout the frame. At `onConsistentData()`, each buffer is compared against its previous state. If changed, `onDisplayChange()` fires, which calls `renderField()` to push the new value through the render pipeline.

---

## 2. HT1622 Segment LCD Pipeline

The HT1622 is a segment LCD controller with 256 segments arranged as 64 addresses x 4 bits. CockpitOS uses a shadow RAM architecture to minimize SPI traffic.

### Shadow RAM

Each `IFEIDisplay` instance maintains two 64-byte arrays per HT1622 chip:

```cpp
uint8_t ramShadow[IFEI_MAX_CHIPS][64];   // Current desired state
uint8_t lastShadow[IFEI_MAX_CHIPS][64];  // Last committed state
```

All render functions write to `ramShadow`. The `commit()` function compares `ramShadow` against `lastShadow` and only writes the bytes that differ to the HT1622 hardware.

### Character Rendering

Characters are rendered through font lookup tables:

- **7-segment font** (`seg7_ascii[128]`): Maps ASCII characters to 7-bit segment patterns. Supports digits 0-9, letters A-Z (uppercase and lowercase), and common symbols.
- **14-segment font** (`seg14_ascii[128]`): Maps ASCII characters to 14-bit segment patterns. Full alphanumeric support including punctuation.

```cpp
// Example: rendering 'A' on a 7-segment display
// seg7_ascii['A'] = 0x77 = bits 0,1,2,4,5,6 = segments a,b,c,e,f,g
```

### RAM Commit

The commit function runs at `IFEI_DISPLAY_REFRESH_RATE_HZ` (250 Hz default):

```cpp
void IFEIDisplay::commit(bool force) {
    constexpr uint32_t MIN_COMMIT_INTERVAL_MS = 1000 / IFEI_DISPLAY_REFRESH_RATE_HZ;

    uint32_t now = millis();
    if (!force && (now - lastCommitTimeMs < MIN_COMMIT_INTERVAL_MS)) return;
    lastCommitTimeMs = now;

    for (int chip = 0; chip < IFEI_MAX_CHIPS; ++chip) {
        if (!_chips[chip]) continue;
        if (memcmp(ramShadow[chip], lastShadow[chip], 64) != 0)
            _chips[chip]->commit(ramShadow[chip], lastShadow[chip], 64);
    }
}
```

The HT1622 driver's `commit()` method performs a byte-by-byte comparison and writes only changed bytes via SPI. The `commitPartial()` method can update a specific address range for even finer granularity.

### Commit Regions

The `buildCommitRegions()` function pre-computes per-field address ranges at init time. This enables `commitNextRegion()` to update individual display fields without scanning all 64 bytes:

```cpp
struct CommitRegion {
    const char* label;   // field name for debugging
    uint8_t chip;        // which HT1622 chip (0 or 1)
    uint8_t addrStart;   // first RAM address
    uint8_t addrEnd;     // last RAM address
};
```

---

## 3. SegmentMap Architecture

### What Is a SegmentMap?

A `SegmentMap` entry maps a single display segment to a physical HT1622 RAM location:

```cpp
struct SegmentMap {
    uint8_t addr;    // HT1622 RAM address (0-63)
    uint8_t bit;     // Bit within that address (0-3)
    uint8_t ledID;   // Which HT1622 chip (0 or 1)
};
```

### File Organization

Each physical display has its own SegmentMap header file using a prefix naming convention:

```
src/LABELS/LABEL_SET_IFEI/IFEI_SegmentMap.h
src/LABELS/LABEL_SET_NEWIFEI/IFEI_SegmentMap.h
```

### Map Structures by Field Type

**7-segment fields** use `[digits][7]` arrays. Each digit has 7 segments (a-g):

```cpp
const SegmentMap IFEI_RPM_L_MAP[3][7] = {
    // Digit 0 (hundreds): only segment 'a' is real, rest are 0xFF (unused)
    { {0,0,0}, {0xFF,0xFF,0}, ... },
    // Digit 1 (tens): all 7 segments mapped
    { {2,3,0}, {2,2,0}, {2,1,0}, {2,0,0}, {0,1,0}, {0,3,0}, {0,2,0} },
    // Digit 2 (ones): all 7 segments mapped
    { {6,3,0}, {6,2,0}, {6,1,0}, {6,0,0}, {4,1,0}, {4,3,0}, {4,2,0} }
};
```

**14-segment fields** (starburst) use `[digits][14]` arrays for full alphanumeric rendering.

**Fuel fields** use a mixed format `[6][14]` where digits 0-3 are 7-segment, digit 4 is a single segment, and digit 5 is 14-segment starburst.

**Bargraph fields** use a flat `[1][11]` array where each entry is one bar segment (e.g., nozzle position indicator).

**Label fields** use a single `SegmentMap` entry for on/off indicators.

### Sentinel Values

An address of `0xFF` and bit of `0xFF` mark unused segments. The render functions skip these entries:

```cpp
if (seg.addr == 0xFF || seg.bit == 0xFF) continue;
```

---

## 4. DisplayMapping.cpp Structure

### fieldDefs Array

The `DisplayMapping.cpp` file (generated per label set) contains the `fieldDefs` array that maps DCS-BIOS control names to display fields:

```cpp
// Conceptual structure (actual field names may vary)
const DisplayFieldDefLabel fieldDefs[] = {
    {
        "IFEI_RPM_L",              // DCS-BIOS label
        &IFEI_RPM_L_MAP[0][0],    // pointer to SegmentMap array
        0,                          // base address (from DCS-BIOS)
        0,                          // mask
        0,                          // shift
        100,                        // max_value
        FIELD_NUMERIC,              // field type
        3,                          // numDigits
        &ifei,                      // driver pointer (IFEIDisplay*)
        DISPLAY_IFEI,               // display type
        renderIFEIDispatcher,       // render function
        clearIFEIDispatcher,        // clear function
        FIELD_RENDER_RPM            // render type
    },
    // ... more fields ...
};
```

### Render Types

| Render Type | Description |
|-------------|-------------|
| `FIELD_RENDER_7SEG` | Standard 7-segment numeric/alpha rendering |
| `FIELD_RENDER_7SEG_SHARED` | 7-segment with overlay priority (TEMP/SP/CODES) |
| `FIELD_RENDER_RPM` | 3-digit RPM with hundreds digit as single segment |
| `FIELD_RENDER_BINGO` | 5-digit bingo fuel with mixed 7-seg and single segments |
| `FIELD_RENDER_FUEL` | 6-digit fuel with mixed 7-seg + single + 14-seg |
| `FIELD_RENDER_ALPHA_NUM_FUEL` | Alphanumeric fuel variant |
| `FIELD_RENDER_BARGRAPH` | Bar graph / pointer position |
| `FIELD_RENDER_LABEL` | Single-segment on/off label |
| `FIELD_RENDER_CUSTOM` | Custom handler |

### Label Creator Integration

The `fieldDefs` array is generated by the Label Creator's Display Editor. When you regenerate mappings, user-customized entries are preserved. The generator only adds or updates entries that match known DCS-BIOS controls.

---

## 5. Display Field Registration

### registerDisplayBuffer()

Each display field's character buffer is registered with the DCS-BIOS bridge at startup:

```cpp
bool registerDisplayBuffer(
    const char* label,    // DCS-BIOS field name
    char* buf,            // character buffer (payload + NUL)
    uint8_t len,          // payload length
    bool* dirtyFlag,      // pointer to dirty flag
    char* last            // previous-value buffer for change detection
);
```

Buffers are stored in `registeredBuffers[]` with a capacity of `MAX_REGISTERED_DISPLAY_BUFFERS` (64 default). Registration happens automatically during `DCSBIOSBridge_postSetup()` from the `CT_DisplayBuffers[]` array generated by the Label Creator.

### CT_Display.h / CT_Display.cpp

These auto-generated files (per label set) define the display buffer entries:

```
src/LABELS/LABEL_SET_IFEI/CT_Display.h
src/LABELS/LABEL_SET_IFEI/CT_Display.cpp
```

They allocate static character buffers and dirty flags for each display field, then populate the `CT_DisplayBuffers[]` array that gets auto-registered at boot.

---

## 6. The RAM Walker Tool

The RAM Walker is a utility for discovering the HT1622 RAM-to-segment mapping of new or unknown displays.

### How It Works

1. The tool iterates through every HT1622 RAM address (0-63) and every bit (0-3).
2. At each step, it lights exactly one segment by setting one bit in RAM.
3. You observe which physical segment lights up on the display and record it.
4. After walking all 256 possible segments across the chip(s), you have a complete mapping.

### Creating a SegmentMap

From your observations, you build the SegmentMap arrays that map logical display positions (digit N, segment S) to physical HT1622 locations (address A, bit B, chip C).

The segment ordering within a 7-segment digit follows:

```
Index:  0    1    2    3    4    5    6
Name:   a    b    c    d    e    f    g
        ---
       |   |
        ---
       |   |
        ---
```

For 14-segment (starburst) displays, there are additional diagonal and horizontal segments.

---

## 7. Render Functions

### renderField()

The top-level dispatcher that looks up a display field by label and calls its registered render function:

```cpp
void renderField(const char* label, const char* value);
```

### Panel-Specific Render Dispatchers

Each display panel provides a render dispatcher that handles its specific render types. For example, the IFEI dispatcher:

```cpp
void renderIFEIDispatcher(void* drv, const SegmentMap* segMap,
                           const char* value, const DisplayFieldDefLabel& def)
{
    IFEIDisplay* display = reinterpret_cast<IFEIDisplay*>(drv);

    if (!isMissionRunning()) return;

    switch (def.renderType) {
        case FIELD_RENDER_7SEG:
            display->addAsciiString7SegToShadow(value, segMap, def.numDigits);
            break;
        case FIELD_RENDER_RPM:
            display->addRpmStringToShadow(value, ...);
            break;
        case FIELD_RENDER_BARGRAPH:
            display->addPointerBarToShadow(percent, segMap, 11);
            break;
        // ... more types ...
    }
}
```

### Clear Functions

Each render type has a corresponding clear function that blanks the field:

```cpp
void clearIFEIDispatcher(void* drv, const SegmentMap* segMap,
                          const DisplayFieldDefLabel& def)
{
    switch (def.renderType) {
        case FIELD_RENDER_7SEG:
            display->clear7SegFromShadow(segMap, def.numDigits);
            break;
        // ... more types ...
    }
}
```

### Shared Field Overlay System

The IFEI panel implements a priority-based overlay system for fields that share physical segments:

- **TEMP_L / SP**: The SP (setpoint) field overlays TEMP_L. When SP has a non-blank value, it takes priority. When SP becomes blank, TEMP_L is restored from its cached base value.
- **TEMP_R / CODES**: Same overlay logic as TEMP_L/SP.
- **FUEL_UP / IFEI_T**: The T (time) field overlays FUEL_UP. While T-mode is active, both fuel fields and temp fields are suppressed.
- **FUEL_DOWN / TIME_SET_MODE**: Time-set overlays fuel-down during T-mode.

```
Priority (highest first):
  1. SP / CODES overlay (active when non-blank)
  2. IFEI_T / TIME_SET overlay (fuel mode)
  3. Base TEMP_L / TEMP_R values
  4. Base FUEL_UP / FUEL_DOWN values
```

---

## 8. TFT Gauge Pipeline

TFT gauges (Battery, Brake Pressure, Hydraulic Pressure, Radar Altimeter, Cabin Pressure) use a fundamentally different rendering approach from segment LCDs.

### Architecture Overview

```
DCS-BIOS value change
   |
   v
subscribeToLedChange() callback
   |
   +-- Update volatile angle/position variable
   +-- Set gaugeDirty = true
   |
   v
FreeRTOS render task (5ms loop)
   |
   +-- Check dirty flag and timing
   +-- Compute dirty rectangle (old needle + new needle AABB union)
   +-- Restore background in dirty region (PSRAM cache -> frame sprite)
   +-- Composite needle sprites with rotation
   +-- Flush dirty region to display via DMA
```

### Key Components

**Background Cache (PSRAM):** Full 240x240 images stored in PSRAM for both Day and NVG modes. Instant restoration of background under moving needles.

**Frame Sprite (PSRAM):** A LovyanGFX sprite the size of the screen used as the composition target.

**Needle Sprites (Internal RAM):** Small sprites for each needle, rendered with `pushRotateZoom()` for smooth rotation.

**DMA Bounce Buffers (Internal RAM):** Two stripe buffers for double-buffered DMA transfer to the display. Ping-pong pattern prevents tearing.

### Dirty-Rect Optimization

Instead of redrawing the entire 240x240 screen every frame, the system computes the axis-aligned bounding box (AABB) of both the old and new needle positions and only updates that region:

```cpp
Rect uOld = rotatedAABB(cx, cy, w, h, px, py, oldAngle);
Rect uNew = rotatedAABB(cx, cy, w, h, px, py, newAngle);
Rect dirty = rectUnion(uOld, uNew);
```

This reduces SPI bandwidth by 80-95% for typical needle movements.

### Day/NVG Modes

Gauges monitor the `CONSOLES_DIMMER` DCS-BIOS value. When it crosses `NVG_THRESHOLD`, the gauge switches background image and needle assets. The transition triggers a `needsFullFlush = true` for a complete screen repaint.

### Timing

- Render interval: `GAUGE_DRAW_MIN_INTERVAL_MS` (13ms = ~77 fps max)
- Task delay: 5ms (`vTaskDelay(pdMS_TO_TICKS(5))`)
- SPI clock: 80 MHz

For detailed TFT gauge wiring and setup, see [Hardware/TFT-Gauges.md](../Hardware/TFT-Gauges.md).

---

## See Also

- [Hardware/Displays.md](../Hardware/Displays.md) -- Display hardware wiring
- [Hardware/TFT-Gauges.md](../Hardware/TFT-Gauges.md) -- TFT gauge setup
- [Advanced/Custom-Panels.md](Custom-Panels.md) -- Panel architecture
- [Advanced/FreeRTOS-Tasks.md](FreeRTOS-Tasks.md) -- Task management for TFT gauges
- [Reference/Config.md](../Reference/Config.md) -- Display-related configuration constants
