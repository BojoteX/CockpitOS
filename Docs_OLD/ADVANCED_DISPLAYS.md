# CockpitOS — Advanced Displays & Custom Panel Programming

> **From data stream to lit segments.**  
> This guide covers HT1622 segment displays (like the F/A-18C IFEI), the RAM walker methodology, display field registration, and the callback/subscription system for programming custom panels.

---

## Table of Contents

1. [Understanding the Display Pipeline](#1-understanding-the-display-pipeline)
2. [HT1622 Segment LCD Basics](#2-ht1622-segment-lcd-basics)
3. [The RAM Walker Process](#3-the-ram-walker-process)
4. [SegmentMap Structure — The Critical File](#4-segmentmap-structure--the-critical-file)
5. [Font Tables — 7-Segment and 14-Segment](#5-font-tables--7-segment-and-14-segment)
6. [Display Field Registration](#6-display-field-registration)
7. [The DCS-BIOS Callback System](#7-the-dcs-bios-callback-system)
8. [Subscription System for Custom Logic](#8-subscription-system-for-custom-logic)
9. [Programming Your Own Panel](#9-programming-your-own-panel)
10. [IFEI Implementation Deep Dive](#10-ifei-implementation-deep-dive)
11. [TFT Gauge Overview](#11-tft-gauge-overview)
12. [Debugging Displays](#12-debugging-displays)
13. [Quick Reference](#13-quick-reference)

---

## 1. Understanding the Display Pipeline

Before diving into specifics, let's understand how data flows from DCS World to your physical display:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        DISPLAY DATA PIPELINE                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌───────────┐      ┌───────────┐      ┌───────────┐      ┌───────────┐   │
│   │           │      │           │      │           │      │           │   │
│   │ DCS World │─────►│ DCS-BIOS  │─────►│ CockpitOS │─────►│ Hardware  │   │
│   │           │      │  Protocol │      │  Firmware │      │  Display  │   │
│   │           │      │           │      │           │      │           │   │
│   └───────────┘      └───────────┘      └───────────┘      └───────────┘   │
│                                                                             │
│   1. LUA exports      2. Binary stream    3. Parse →        4. GPIO bit-   │
│      cockpit data        addr/value          callbacks →       bang or     │
│                          pairs               shadow RAM        SPI DMA     │
│                                              → render                      │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   KEY COMPONENTS IN COCKPITOS:                                              │
│                                                                             │
│   • DCSBIOSBridge.cpp  — Parses stream, dispatches callbacks                │
│   • DisplayMapping.cpp — Field definitions, render functions                │
│   • SegmentMap.h       — Physical RAM→segment mapping (per display)         │
│   • IFEIPanel.cpp      — Panel-specific logic, shadow RAM, commit           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.1 Control Types That Reach Displays

The DCS-BIOS protocol defines several control types. For displays, we care about:

| Control Type | C++ Enum | Description |
|--------------|----------|-------------|
| `display` | `CT_DISPLAY` | String fields (IFEI digits, UFC scratchpad) |
| `led` | `CT_LED` | Single-bit indicators |
| `selector` | `CT_SELECTOR` | Multi-position switches (feedback) |
| `metadata` | `CT_METADATA` | Internal state tracking |
| `analog_gauge` | `CT_GAUGE` | Servo-driven needles |
| `analog_dial` | `CT_ANALOG` | Variable brightness/position |

---

## 2. HT1622 Segment LCD Basics

The HT1622 is a dedicated LCD segment driver IC commonly used in cockpit instrument replicas.

### 2.1 How the HT1622 Works

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        HT1622 ARCHITECTURE                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   The HT1622 contains 256 bits of RAM organized as 64 addresses × 4 bits:   │
│                                                                             │
│   Address 0:  [ bit3 | bit2 | bit1 | bit0 ]  ← 4 segments                   │
│   Address 1:  [ bit3 | bit2 | bit1 | bit0 ]                                 │
│   Address 2:  [ bit3 | bit2 | bit1 | bit0 ]                                 │
│      ...                                                                    │
│   Address 63: [ bit3 | bit2 | bit1 | bit0 ]                                 │
│                                                                             │
│   Each bit controls ONE segment on the LCD glass.                           │
│   Setting bit = 1 turns that segment ON (dark/visible).                     │
│   Setting bit = 0 turns that segment OFF (transparent).                     │
│                                                                             │
│   COMMUNICATION: 3-wire serial (CS, WR, DATA)                               │
│   - CS (Chip Select): LOW to start communication                            │
│   - WR (Write Clock): Data sampled on rising edge                           │
│   - DATA: Serial data line                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Wiring an HT1622

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        HT1622 WIRING                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│                        ESP32                         HT1622 Module          │
│                     ┌─────────┐                    ┌─────────────┐          │
│                     │         │                    │             │          │
│                     │  GPIO 5 ├────────────────────┤ CS          │          │
│                     │   (CS)  │                    │             │          │
│                     │         │                    │             │          │
│                     │  GPIO 6 ├────────────────────┤ WR          │          │
│                     │   (WR)  │                    │             │          │
│                     │         │                    │             │          │
│                     │  GPIO 7 ├────────────────────┤ DATA        │          │
│                     │  (DATA) │                    │             │          │
│                     │         │                    │             │          │
│                     │   3.3V  ├────────────────────┤ VCC         │          │
│                     │   GND   ├────────────────────┤ GND         │          │
│                     │         │                    │             │          │
│                     └─────────┘                    └─────────────┘          │
│                                                                             │
│   For the IFEI, TWO HT1622 chips are used (one per half of the display):    │
│                                                                             │
│   Chip 0: CS0_PIN, WR0_PIN, DATA0_PIN                                       │
│   Chip 1: CS1_PIN, WR1_PIN, DATA1_PIN                                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.3 Shadow RAM Concept

CockpitOS maintains a "shadow RAM" — a local copy of the HT1622's RAM. This allows:

1. **Efficient updates**: Only write changed nibbles to hardware
2. **Atomic rendering**: Build the complete frame before committing
3. **Multi-field coordination**: Multiple display fields can write to shadow before flush

```cpp
// From IFEIPanel.cpp
uint8_t ramShadow[IFEI_MAX_CHIPS][64];   // Local copy of HT1622 RAM
uint8_t lastShadow[IFEI_MAX_CHIPS][64];  // Last committed state (for diff)
```

The `commit()` function compares `ramShadow` with `lastShadow` and only sends changed nibbles to the hardware.

---

## 3. The RAM Walker Process

**The RAM Walker is the critical first step when mapping a new HT1622 display.** It systematically lights each segment to discover which RAM address/bit controls which physical segment.

### 3.1 Why You Need It

Every LCD manufacturer wires segments differently. There's no standard. What appears as "digit 1, segment A" might be at RAM address 23, bit 2 on one display and RAM address 7, bit 0 on another.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     THE RAM WALKER PROBLEM                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   WHAT YOU SEE ON THE LCD:           WHAT'S IN THE HT1622 RAM:              │
│                                                                             │
│        ┌───────┐                     Address 0: 0b0000                      │
│        │   A   │  ← Segment A        Address 1: 0b0000                      │
│       ┌┴┐   ┌──┤                     Address 2: 0b0010 ← This bit!          │
│       │F│   │B │                     Address 3: 0b0000                      │
│       └┬┘   └──┤                     ...                                    │
│        │   G   │                                                            │
│       ┌┴┐   ┌──┤                     The LCD glass routing is arbitrary.    │
│       │E│   │C │                     You MUST discover it empirically.      │
│       └┬┘   └──┤                                                            │
│        │   D   │                                                            │
│        └───────┘                                                            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 RAM Walker Algorithm

The RAM Walker iterates through every address (0-63) and every bit (0-3), turning on ONE segment at a time:

```cpp
// Pseudocode for RAM Walker
for (uint8_t addr = 0; addr < 64; addr++) {
    for (uint8_t bit = 0; bit < 4; bit++) {
        // Clear all segments
        ht1622.clear();
        
        // Set only this one segment
        ht1622.writeNibble(addr, 1 << bit);
        
        // Wait for human to observe and record
        delay(500);
        
        // Print what we're showing
        Serial.printf("Address: %d, Bit: %d\n", addr, bit);
    }
}
```

### 3.3 Recording the Mapping

As you run the RAM Walker, you must record what you see:

```
Address  Bit  What Lit Up
-------  ---  -----------
0        0    Nothing
0        1    Nothing
0        2    Clock digit 1, segment G (middle)
0        3    Clock digit 1, segment A (top)
1        0    Clock digit 1, segment B (top-right)
1        1    Clock digit 1, segment C (bottom-right)
...
```

### 3.4 Tools Directory — RAM Walker and Helpers

CockpitOS includes essential tools in the `/Tools/` directory for display development:

```
Tools/
├── IFEI_RAM_WALKER/           ← Walk HT1622 segments systematically
│   └── IFEI_RAM_WALKER.ino    ← Upload this, observe, record
├── HT1622_AllOn/              ← Light all segments (wiring test)
├── HT1622_Nibble_Test/        ← Test individual nibbles
├── Segment_Pattern_Gen/       ← Generate font table patterns
└── Serial_Monitor_Parser/     ← Parse RAM walker output
```

### 3.5 RAM Walker Sketch Example

Here's what a typical RAM Walker sketch looks like:

```cpp
// Tools/IFEI_RAM_WALKER/IFEI_RAM_WALKER.ino (simplified)

#include "HT1622.h"

// Your HT1622 pins
#define CS_PIN   5
#define WR_PIN   6
#define DATA_PIN 7

HT1622 chip(CS_PIN, WR_PIN, DATA_PIN);

void setup() {
    Serial.begin(115200);
    chip.init();
    chip.clear();
    
    Serial.println("=== HT1622 RAM WALKER ===");
    Serial.println("Watch the display, record which segment lights.");
    Serial.println("Press any key to advance...");
}

void loop() {
    for (uint8_t addr = 0; addr < 64; addr++) {
        for (uint8_t bit = 0; bit < 4; bit++) {
            // Clear everything first
            chip.clear();
            
            // Light only this one segment
            chip.writeNibble(addr, 1 << bit);
            
            // Tell the user what we're showing
            Serial.printf("Address: %2d, Bit: %d  ", addr, bit);
            Serial.println("(press Enter to continue)");
            
            // Wait for user input
            while (!Serial.available()) delay(10);
            while (Serial.available()) Serial.read();
        }
    }
    
    Serial.println("\n=== WALK COMPLETE ===");
    while (true) delay(1000);
}
```

### 3.6 Recording Your Mapping

Create a spreadsheet or text file as you walk:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ADDR  │  BIT  │  PHYSICAL LOCATION           │  NOTES                     │
├────────┼───────┼──────────────────────────────┼────────────────────────────┤
│   0    │   0   │  Nothing                     │                            │
│   0    │   1   │  Nothing                     │                            │
│   0    │   2   │  Digit 1, segment G (middle) │  Clock hours, 10s place    │
│   0    │   3   │  Digit 1, segment A (top)    │  Clock hours, 10s place    │
│   1    │   0   │  Digit 1, segment B (top-R)  │  Clock hours, 10s place    │
│   1    │   1   │  Digit 1, segment C (bot-R)  │  Clock hours, 10s place    │
│   1    │   2   │  Digit 1, segment D (bottom) │  Clock hours, 10s place    │
│   1    │   3   │  Digit 1, segment E (bot-L)  │  Clock hours, 10s place    │
│   2    │   0   │  Digit 1, segment F (top-L)  │  Clock hours, 10s place    │
│  ...   │  ...  │  ...                         │  ...                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.7 Converting RAM Walker Output to SegmentMap

After walking, you must REORDER to match font table order:

```cpp
// RAW RAM WALKER OUTPUT (physical order discovered):
// Digit 1: A=addr0,bit3  B=addr1,bit0  C=addr1,bit1  D=addr1,bit2
//          E=addr1,bit3  F=addr2,bit0  G=addr0,bit2

// FONT TABLE ORDER REQUIRED:
// [0]=TOP(A), [1]=TOP-R(B), [2]=BOT-R(C), [3]=BOT(D), 
// [4]=BOT-L(E), [5]=TOP-L(F), [6]=MID(G)

// CORRECT SEGMENTMAP (reordered to match font):
const SegmentMap CLOCK_H_10S[7] = {
    {0, 3, 0},  // [0] = A (TOP)       at addr 0, bit 3
    {1, 0, 0},  // [1] = B (TOP-RIGHT) at addr 1, bit 0
    {1, 1, 0},  // [2] = C (BOT-RIGHT) at addr 1, bit 1
    {1, 2, 0},  // [3] = D (BOTTOM)    at addr 1, bit 2
    {1, 3, 0},  // [4] = E (BOT-LEFT)  at addr 1, bit 3
    {2, 0, 0},  // [5] = F (TOP-LEFT)  at addr 2, bit 0
    {0, 2, 0},  // [6] = G (MIDDLE)    at addr 0, bit 2
};
```

---

## 4. SegmentMap Structure — The Critical File

The `SegmentMap.h` file is **the single most important file** for any HT1622 display. Get it wrong, and your digits will be scrambled garbage.

### 4.1 The SegmentMap Structure

```cpp
// From CUtils.h
struct SegmentMap {
    uint8_t addr;   // HT1622 RAM address (0-63)
    uint8_t bit;    // Bit within that address (0-3)
    uint8_t ledID;  // Which HT1622 chip (0 or 1 for IFEI)
};
```

### 4.2 Array Organization

A SegmentMap array is organized as `[digit][segment]`:

```cpp
// Example: 3-digit, 7-segment display
// [0]=TOP, [1]=TOP-RIGHT, [2]=BOTTOM-RIGHT, [3]=BOTTOM, 
// [4]=BOTTOM-LEFT, [5]=TOP-LEFT, [6]=MIDDLE
const SegmentMap RPM_MAP[3][7] = {
    // Digit 0 (100s place)
    { {12,3,0}, {10,3,0}, {10,1,0}, {12,0,0}, {12,1,0}, {12,2,0}, {10,2,0} },
    // Digit 1 (10s place)
    { {8,3,0}, {6,3,0}, {6,1,0}, {8,0,0}, {8,1,0}, {8,2,0}, {6,2,0} },
    // Digit 2 (1s place)
    { {4,3,0}, {2,3,0}, {2,1,0}, {4,0,0}, {4,1,0}, {4,2,0}, {2,2,0} },
};
```

### 4.3 The Segment Order Problem

**This is where everyone fails.** The segment order in your array MUST match your font table order.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     SEGMENT ORDER MAPPING                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   STANDARD 7-SEGMENT LAYOUT:           COCKPITOS FONT ORDER:                │
│                                                                             │
│          ┌─── A ───┐                   Index 0 = TOP (A)                    │
│          │         │                   Index 1 = TOP-RIGHT (B)              │
│          F         B                   Index 2 = BOTTOM-RIGHT (C)           │
│          │         │                   Index 3 = BOTTOM (D)                 │
│          ├─── G ───┤                   Index 4 = BOTTOM-LEFT (E)            │
│          │         │                   Index 5 = TOP-LEFT (F)               │
│          E         C                   Index 6 = MIDDLE (G)                 │
│          │         │                                                        │
│          └─── D ───┘                                                        │
│                                                                             │
│   YOUR SEGMENTMAP ARRAY MUST FOLLOW THIS ORDER:                             │
│   { {A_addr,A_bit,chip}, {B_addr,B_bit,chip}, {C_addr,C_bit,chip}, ... }    │
│                                                                             │
│   If your RAM walker discovered:                                            │
│     TOP    at addr=5, bit=2                                                 │
│     TOP-R  at addr=3, bit=0                                                 │
│     BOT-R  at addr=3, bit=1                                                 │
│     ...                                                                     │
│                                                                             │
│   Then your array MUST be:                                                  │
│   { {5,2,0}, {3,0,0}, {3,1,0}, ... }  ← In font table order!                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.4 Invalid Segments

Use `{0xFF, 0xFF, chip}` for segments that don't exist (e.g., when a 14-segment array has unused positions):

```cpp
// 14-segment array with only 7 segments used
{ {35,3,1}, {33,3,1}, {33,1,1}, {35,0,1}, {35,1,1}, {35,2,1}, {33,2,1},
  {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, 
  {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },
```

### 4.5 The ledID (Chip ID) Warning

```
⚠️ ABSOLUTE WARNING FROM SegmentMap.txt:

**EVERY SegmentMap entry has a CHIP_ID (aka ledID).**
THIS IS *NOT* OPTIONAL. If you set the wrong ledID, your digits/segments
will appear on the wrong physical chip, the wrong display, or not at all.

BE OBSESSIVE: CHECK THE ledID FOR EVERY ENTRY.
```

---

## 5. Font Tables — 7-Segment and 14-Segment

Font tables define which segments to light for each ASCII character.

### 5.1 7-Segment Font Table

```cpp
// From IFEIPanel.cpp — seg7_ascii[128]
// Each byte represents which segments to light for that ASCII character
// Bit 0 = TOP, Bit 1 = TOP-RIGHT, Bit 2 = BOT-RIGHT, etc.

uint8_t const seg7_ascii[128] = {
    // ... (control characters = 0x00)
    0x00, //  32  ' ' (space)
    // ...
    0x3F, //  48  '0'  = segments A,B,C,D,E,F (all except G)
    0x06, //  49  '1'  = segments B,C
    0x5B, //  50  '2'  = segments A,B,D,E,G
    0x4F, //  51  '3'  = segments A,B,C,D,G
    0x66, //  52  '4'  = segments B,C,F,G
    0x6D, //  53  '5'  = segments A,C,D,F,G
    0x7D, //  54  '6'  = segments A,C,D,E,F,G
    0x07, //  55  '7'  = segments A,B,C
    0x7F, //  56  '8'  = all segments
    0x6F, //  57  '9'  = segments A,B,C,D,F,G
    // ...
    0x77, //  65  'A'  = segments A,B,C,E,F,G
    0x7C, //  66  'B'  = segments C,D,E,F,G (lowercase b)
    0x39, //  67  'C'  = segments A,D,E,F
    // ...
};
```

### 5.2 14-Segment Font Table

For starburst/alphanumeric displays:

```cpp
// From IFEIPanel.cpp — seg14_ascii[128]
// 16-bit values for 14 segments

uint16_t const seg14_ascii[128] = {
    // ...
    0x3847, //  65  'A'
    0x02D7, //  66  'B'
    0x28C0, //  67  'C'
    0x02D5, //  68  'D'
    0x38C0, //  69  'E'
    0x3840, //  70  'F'
    // ...
};
```

### 5.3 How Rendering Uses Font Tables

When you render "123" to a 3-digit display:

```cpp
void addAsciiString7SegToShadow(const char* str, const SegmentMap* map, int numDigits) {
    for (int d = 0; d < numDigits; d++) {
        char c = str[d];
        uint8_t segBits = seg7_ascii[(uint8_t)c];  // Lookup which segments
        
        for (int s = 0; s < 7; s++) {
            const SegmentMap& seg = map[d * 7 + s];
            if (segBits & (1 << s))
                ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);   // Turn ON
            else
                ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);  // Turn OFF
        }
    }
}
```

---

## 6. Display Field Registration

Display fields connect DCS-BIOS labels to physical display areas.

### 6.1 DisplayFieldDefLabel Structure

```cpp
// From DisplayMapping.h
struct DisplayFieldDefLabel {
    const char*      label;         // DCS-BIOS label (e.g., "IFEI_RPM_L")
    const SegmentMap* segMap;       // Pointer to segment map
    uint8_t          numDigits;     // How many digits
    uint8_t          segsPerDigit;  // Segments per digit (7 or 14)
    uint16_t         minValue;      // Minimum value (for validation)
    uint16_t         maxValue;      // Maximum value
    FieldType        type;          // FIELD_7SEG, FIELD_BARGRAPH, etc.
    uint8_t          barCount;      // Number of bars (for bargraphs)
    void*            driver;        // Pointer to display driver
    DisplayDevice    deviceType;    // DISPLAY_IFEI, DISPLAY_UFC, etc.
    FieldRenderType  renderType;    // How to render
    void (*renderFunc)(...);        // Custom render function
    void (*clearFunc)(...);         // Custom clear function
};
```

### 6.2 Registering Fields in DisplayMapping.cpp

```cpp
// Auto-generated and manually configured
static DisplayFieldDefLabel fieldDefs[] = {
    {
        "IFEI_RPM_L",                    // DCS-BIOS label
        (const SegmentMap*)IFEI_RPM_L_MAP, // Segment map
        3,                                // 3 digits
        7,                                // 7 segments each
        0,                                // min value
        999,                              // max value
        FIELD_7SEG,                       // Field type
        0,                                // bar count (N/A)
        &ifei,                            // Display driver
        DISPLAY_IFEI,                     // Device type
        FIELD_RENDER_7SEG,                // Render type
        renderIFEIDispatcher,             // Render function
        clearIFEIDispatcher               // Clear function
    },
    // ... more fields
};
```

### 6.3 The renderField() Function

When DCS-BIOS sends display data, `renderField()` is called:

```cpp
void renderField(const char* label, const char* value) {
    // Find the field definition by label (O(1) hash lookup)
    const DisplayFieldDefLabel* def = findFieldDefByLabel(label);
    if (!def) return;
    
    // Call the render function
    if (def->renderFunc) {
        def->renderFunc(def->driver, def->segMap, *def, value);
    }
}
```

---

## 7. The DCS-BIOS Callback System

CockpitOS receives all cockpit data through callbacks. Understanding these is essential for custom panels.

### 7.1 Callback Types

```cpp
// From DCSBIOSBridge.h

// Called when an LED/indicator value changes
void onLedChange(const char* label, uint16_t value, uint16_t max_value);

// Called when a selector position changes
void onSelectorChange(const char* label, unsigned int newValue);

// Called when a metadata field changes
void onMetaDataChange(const char* label, unsigned int value);

// Called when a display string changes
void onDisplayChange(const char* label, const char* value);

// Called when aircraft name is received (mission start)
void onAircraftName(const char* str);
```

### 7.2 How Callbacks Are Triggered

The DCS-BIOS parser dispatches to callbacks based on control type:

```cpp
// Simplified from DCSBIOSBridge.cpp
void dispatchByControlType(const DcsOutputEntry* entry, uint16_t value) {
    switch (entry->controlType) {
        case CT_LED:
            onLedChange(entry->label, value, entry->max_value);
            break;
            
        case CT_SELECTOR:
            onSelectorChange(entry->label, value);
            break;
            
        case CT_DISPLAY:
            // String fields are assembled, then:
            onDisplayChange(entry->label, stringBuffer);
            break;
            
        case CT_METADATA:
            onMetaDataChange(entry->label, value);
            break;
    }
}
```

### 7.3 The Frame Counter

CockpitOS tracks stream health using a frame counter:

```cpp
// Each callback increments this
frameCounter++;

// After enough frames, we know the stream is valid
bool simReady() {
    return frameCounter >= MINIMUM_FRAMES_FOR_READY;
}
```

---

## 8. Subscription System for Custom Logic

Beyond the automatic dispatch, you can subscribe to specific labels for custom handling. This is the heart of programming custom panels.

### 8.1 What You Have Access To

When programming CockpitOS, you have access to **everything** coming from DCS World via WiFi/USB/Serial. Here's what's available:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     DATA AVAILABLE IN COCKPITOS                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   DATA TYPE        │  CALLBACK                 │  WHAT YOU GET              │
│  ──────────────────┼───────────────────────────┼────────────────────────────│
│   LED/Indicators   │  onLedChange()            │  label, value (0-max)      │
│   Selectors        │  onSelectorChange()       │  label, position (0-N)     │
│   Display Strings  │  onDisplayChange()        │  label, string value       │
│   Metadata         │  onMetaDataChange()       │  label, integer value      │
│   Analog Gauges    │  (via onLedChange)        │  label, 0-65535 range      │
│   Aircraft Name    │  onAircraftName()         │  aircraft module string    │
│                                                                             │
│   EVERY piece of cockpit data exported by DCS-BIOS is available!            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 8.2 The Four Subscription Functions

```cpp
// ═══════════════════════════════════════════════════════════════════════════
// SUBSCRIBE TO DISPLAY STRING CHANGES
// ═══════════════════════════════════════════════════════════════════════════
// Use for: IFEI digits, UFC scratchpad, any text display
bool subscribeToDisplayChange(
    const char* label,                                    // e.g., "IFEI_FUEL_UP"
    void (*callback)(const char* label, const char* value) // Your handler
);

// Example callback:
void onFuelUpChange(const char* label, const char* value) {
    // value = "12345" (current fuel quantity as string)
    int fuel = atoi(value);
    if (fuel < 1000) triggerLowFuelWarning();
}

// ═══════════════════════════════════════════════════════════════════════════
// SUBSCRIBE TO SELECTOR POSITION CHANGES
// ═══════════════════════════════════════════════════════════════════════════
// Use for: Switch positions, dial settings, mode changes
bool subscribeToSelectorChange(
    const char* label,                                    // e.g., "MASTER_ARM_SW"
    void (*callback)(const char* label, uint16_t value)   // Your handler
);

// Example callback:
void onMasterArmChange(const char* label, uint16_t value) {
    // value = 0 (SAFE), 1 (ARM)
    if (value == 1) flashMasterArmLED();
}

// ═══════════════════════════════════════════════════════════════════════════
// SUBSCRIBE TO METADATA CHANGES
// ═══════════════════════════════════════════════════════════════════════════
// Use for: Internal state, brightness values, external lights, custom tracking
bool subscribeToMetadataChange(
    const char* label,                                    // e.g., "IFEI_BRIGHTNESS"
    void (*callback)(const char* label, uint16_t value)   // Your handler
);

// Example callback:
void onBrightnessChange(const char* label, uint16_t value) {
    // value = 0-65535 (brightness level)
    setDisplayBrightness(map(value, 0, 65535, 0, 100));
}

// ═══════════════════════════════════════════════════════════════════════════
// SUBSCRIBE TO LED/INDICATOR CHANGES
// ═══════════════════════════════════════════════════════════════════════════
// Use for: Warning lights, annunciators, any on/off or dimmable indicator
bool subscribeToLedChange(
    const char* label,                                    // e.g., "MASTER_CAUTION_LT"
    void (*callback)(const char* label, uint16_t value, uint16_t max_value)
);

// Example callback:
void onMasterCautionChange(const char* label, uint16_t value, uint16_t max_value) {
    // value = 0 (off) or 1 (on) for binary
    // For dimmable: value/max_value = brightness ratio
    bool isOn = (value > 0);
    uint8_t brightness = (max_value > 0) ? map(value, 0, max_value, 0, 255) : 255;
}
```

### 8.3 How to Register Subscriptions

Always register subscriptions in your panel's `init()` function:

```cpp
// MyPanel.cpp

void MyPanel_init() {
    // Register for display strings
    subscribeToDisplayChange("IFEI_RPM_L", onRpmLeftChange);
    subscribeToDisplayChange("IFEI_FUEL_UP", onFuelUpChange);
    
    // Register for selector positions
    subscribeToSelectorChange("MASTER_ARM_SW", onMasterArmChange);
    subscribeToSelectorChange("FUEL_DUMP_SW", onFuelDumpChange);
    
    // Register for metadata (brightness, external state)
    subscribeToMetadataChange("IFEI_DISP_INT_LT", onIfeiBrightnessChange);
    subscribeToMetadataChange("NVG_MODE", onNvgModeChange);
    
    // Register for LED/indicator changes
    subscribeToLedChange("MASTER_CAUTION_LT", onMasterCautionChange);
    subscribeToLedChange("FIRE_LEFT_LT", onFireLeftChange);
}
```

### 8.4 Complete Programming Example — Custom CMWS Display

Here's a real-world example from the TFT CMWS display:

```cpp
// TFT_Display_CMWS.cpp (simplified)

// Static state variables (no heap!)
static bool g_dirty = false;
static bool g_dispBrt = false, g_dispDim = false;
static bool g_readyBrt = false, g_readyDim = false;
static char g_flareCount[8] = " 00";

// Callback functions
static void onDispenseBrt(const char*, uint16_t v) { 
    g_dispBrt = (v != 0); 
    g_dirty = true; 
}

static void onDispenseDim(const char*, uint16_t v) { 
    g_dispDim = (v != 0); 
    g_dirty = true; 
}

static void onFlareCount(const char*, const char* value) {
    if (!value) return;
    // Copy to static buffer, mark dirty
    strncpy(g_flareCount, value, sizeof(g_flareCount) - 1);
    g_dirty = true;
}

void CMWSDisplay_init() {
    // Initialize TFT hardware...
    
    // Subscribe to ALL relevant DCS-BIOS fields
    subscribeToMetadataChange("PLT_CMWS_D_BRT_L", onDispenseBrt);
    subscribeToMetadataChange("PLT_CMWS_D_DIM_L", onDispenseDim);
    subscribeToMetadataChange("PLT_CMWS_R_BRT_L", onReadyBrt);
    subscribeToMetadataChange("PLT_CMWS_R_DIM_L", onReadyDim);
    
    subscribeToDisplayChange("PLT_CMWS_FLARE_COUNT", onFlareCount);
    subscribeToDisplayChange("PLT_CMWS_CHAFF_COUNT", onChaffCount);
    
    subscribeToLedChange("PLT_CMWS_LAMP", onLampChange);
}

void CMWSDisplay_loop() {
    if (!g_dirty) return;  // Nothing changed
    
    // Redraw only what changed
    redrawDisplay();
    g_dirty = false;
}
```

### 8.5 Querying Current State

You can query the last known value of any tracked control:

```cpp
// Get last selector value
uint16_t value = getLastKnownState("MASTER_ARM_SW");

// Check if a cover is open
bool coverOpen = isCoverOpen("EJECTION_SEAT_SAFE_COVER");

// Convenience macros
#define isToggleOn(label)       isCoverOpen(label)
#define setCoverState(label, v) sendDCSBIOSCommand(label, v ? 1 : 0, true)
#define setToggleState(label, v) setCoverState(label, v)
```

### 8.6 Sending Commands Back to DCS

You can send commands to DCS-BIOS from your panel code:

```cpp
// Send a command with a value
sendDCSBIOSCommand("MASTER_ARM_SW", 1, false);  // Set to ARM

// Force send (bypass throttling)
sendDCSBIOSCommand("EJECTION_HANDLE", 1, true);  // true = force

// Using the macro helpers
setToggleState("MASTER_ARM_SW", true);
setCoverState("EJECTION_SEAT_SAFE_COVER", true);
```

### 8.7 Helper Functions Available

```cpp
// Mission state
bool isMissionRunning();           // Is a mission active?
uint32_t msSinceMissionStart();    // Milliseconds since mission start
bool simReady();                   // Is the stream established?

// Force resync
void forceResync();                // Request full state resync

// String utilities
bool isFieldBlank(const char* s);  // Check if display field is blank
int strToIntFast(const char* s);   // Fast string-to-int conversion
```

---

## 9. Programming Your Own Panel

Let's walk through creating a custom display panel from scratch.

### 9.1 Panel File Structure

```cpp
// MyPanel.cpp

#include "../Globals.h"
#include "../DCSBIOSBridge.h"
#include "includes/MyPanel.h"

// Register the panel with the system
REGISTER_PANEL(MyPanel, MyPanel_init, MyPanel_loop, 
               MyDisplay_init, MyDisplay_loop, MyDisplay_commit, 100);

// Static state (no heap!)
static HT1622 chip0(CS_PIN, WR_PIN, DATA_PIN);
static uint8_t ramShadow[64];
static uint8_t lastShadow[64];

void MyPanel_init() {
    chip0.init();
    chip0.clear();
    
    // Subscribe to relevant DCS-BIOS fields
    subscribeToDisplayChange("MY_FIELD", onMyFieldChange);
}

void MyPanel_loop() {
    // Periodic tasks (button polling, etc.)
}

void MyDisplay_init() {
    memset(ramShadow, 0, 64);
    memset(lastShadow, 0xFF, 64);  // Force initial commit
}

void MyDisplay_loop() {
    // Called each display refresh cycle
}

void MyDisplay_commit() {
    chip0.commit(ramShadow, lastShadow, 64);
}

static void onMyFieldChange(const char* label, const char* value) {
    // Render value to ramShadow
    renderToShadow(value);
}
```

### 9.2 Creating Your SegmentMap

After running the RAM Walker and documenting your mapping:

```cpp
// MyPanel_SegmentMap.h

#pragma once
#include "../../../lib/CUtils/src/CUtils.h"

// 3-digit, 7-segment display
// Segment order: [0]=TOP, [1]=TOP-RIGHT, [2]=BOT-RIGHT, [3]=BOT, 
//                [4]=BOT-LEFT, [5]=TOP-LEFT, [6]=MIDDLE
const SegmentMap MY_DISPLAY_MAP[3][7] = {
    // Digit 0 (leftmost)
    { {5,2,0}, {3,0,0}, {3,1,0}, {5,0,0}, {5,1,0}, {7,0,0}, {3,2,0} },
    // Digit 1 (middle)
    { {9,2,0}, {7,3,0}, {7,1,0}, {9,0,0}, {9,1,0}, {11,0,0}, {7,2,0} },
    // Digit 2 (rightmost)
    { {13,2,0}, {11,3,0}, {11,1,0}, {13,0,0}, {13,1,0}, {15,0,0}, {11,2,0} },
};
```

### 9.3 Writing to Shadow RAM

```cpp
void renderToShadow(const char* value) {
    int len = strlen(value);
    
    for (int d = 0; d < 3; d++) {
        int charIdx = d - (3 - len);  // Right-align
        uint8_t segBits = 0;
        
        if (charIdx >= 0 && charIdx < len) {
            char c = value[charIdx];
            segBits = seg7_ascii[(uint8_t)c];
        }
        
        for (int s = 0; s < 7; s++) {
            const SegmentMap& seg = MY_DISPLAY_MAP[d][s];
            if (seg.addr == 0xFF) continue;  // Skip invalid segments
            
            if (segBits & (1 << s))
                ramShadow[seg.addr] |= (1 << seg.bit);
            else
                ramShadow[seg.addr] &= ~(1 << seg.bit);
        }
    }
}
```

### 9.4 Committing to Hardware

```cpp
void MyDisplay_commit() {
    // Only write changed nibbles (efficient)
    chip0.commit(ramShadow, lastShadow, 64);
    
    // Or for partial updates (even more efficient):
    // chip0.commitPartial(ramShadow, lastShadow, startAddr, endAddr);
}
```

---

## 10. IFEI Implementation Deep Dive

The F/A-18C IFEI is the reference implementation for HT1622 displays in CockpitOS. Understanding how it was developed will help you create your own segment displays.

### 10.1 How the IFEI Was Developed

The IFEI development followed this exact process:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     IFEI DEVELOPMENT TIMELINE                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   STEP 1: HARDWARE IDENTIFICATION                                           │
│   ─────────────────────────────────                                         │
│   • Identified the display uses TWO HT1622 driver ICs                       │
│   • Each chip drives one half of the display (left/right)                   │
│   • Traced the CS/WR/DATA lines to ESP32 GPIO pins                          │
│                                                                             │
│   STEP 2: RAM WALKER EXECUTION                                              │
│   ────────────────────────────────                                          │
│   • Used Tools/IFEI_RAM_WALKER sketch                                       │
│   • Walked through all 64 addresses × 4 bits for EACH chip                  │
│   • Recorded which physical segment lit for each addr/bit combination       │
│   • This took several hours of careful observation and note-taking          │
│                                                                             │
│   STEP 3: SEGMENT MAP CREATION                                              │
│   ────────────────────────────────                                          │
│   • Organized raw RAM walker data into logical digit groups                 │
│   • CRITICAL: Reordered segments to match font table order                  │
│   • Created IFEI_SegmentMap.h with all field definitions                    │
│                                                                             │
│   STEP 4: FONT TABLE VERIFICATION                                           │
│   ────────────────────────────────                                          │
│   • Tested each digit 0-9 to verify segment order                           │
│   • Fixed any scrambled digits by adjusting SegmentMap order                │
│   • Added 14-segment patterns for alphanumeric fuel display                 │
│                                                                             │
│   STEP 5: DISPLAY LOGIC IMPLEMENTATION                                      │
│   ────────────────────────────────────                                      │
│   • Created IFEIDisplay class with write helpers                            │
│   • Implemented overlay logic (SP/CODES sharing TEMP fields)                │
│   • Added region-based commits for efficiency                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.2 IFEI Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        IFEI DISPLAY LAYOUT                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │  CHIP 0 (Left Half)              │  CHIP 1 (Right Half)             │   │
│   │                                  │                                  │   │
│   │  RPM L    [###]                  │  [###]    RPM R                  │   │
│   │  TEMP L   [###]                  │  [###]    TEMP R                 │   │
│   │  FF L     [###]                  │  [###]    FF R                   │   │
│   │  NOZ L    [###]                  │  [###]    NOZ R                  │   │
│   │  OIL L    [###]                  │  [###]    OIL R                  │   │
│   │                                  │                                  │   │
│   │  FUEL UP  [######]               │  [######] FUEL DOWN              │   │
│   │  BINGO    [#####]                │                                  │   │
│   │                                  │                                  │   │
│   │  CLOCK    HH:MM:SS               │  TIMER   HH:MM:SS                │   │
│   │                                  │                                  │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│   Each chip has independent CS/WR/DATA lines.                               │
│   The ledID in SegmentMap determines which chip receives the write.         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 10.3 The IFEIDisplay Class API

The `IFEIDisplay` class provides all the methods you need to write to the display. Here's the complete API:

```cpp
// From IFEIPanel.h
class IFEIDisplay {
public:
    IFEIDisplay(HT1622* chips[IFEI_MAX_CHIPS]);

    // ═══════════════════════════════════════════════════════════════════════
    // BASIC METHODS
    // ═══════════════════════════════════════════════════════════════════════
    
    void commit(bool force = false);    // Flush shadow RAM to hardware
    void clear();                       // Clear all segments
    void buildCommitRegions();          // Pre-calculate commit regions
    void commitNextRegion();            // Commit one region (round-robin)
    
    // Access the shadow RAM directly
    inline uint8_t (*getRamShadow())[64] { return ramShadow; }
    
    // Read what's currently displayed (reverse lookup)
    void readRegionFromShadow(const SegmentMap* map, int numDigits, 
                               int segsPerDigit, uint8_t ramShadow[IFEI_MAX_CHIPS][64], 
                               char* out, size_t outSize);

    // ═══════════════════════════════════════════════════════════════════════
    // REFRESH & RESET
    // ═══════════════════════════════════════════════════════════════════════
    
    void blankBuffersAndDirty();        // Clear buffers, mark dirty
    void invalidateHardwareCache();     // Force full redraw on next commit

    // ═══════════════════════════════════════════════════════════════════════
    // WRITE HELPERS — These write to shadow RAM
    // ═══════════════════════════════════════════════════════════════════════
    
    // Standard 7-segment string (any ASCII)
    void addAsciiString7SegToShadow(const char* str, const SegmentMap* map, int numDigits);
    
    // RPM-style: single "1" segment + 2 full 7-seg digits
    void addRpmStringToShadow(const char* str, const SegmentMap map[3][7]);
    
    // BINGO: 3 full 7-seg + 2 single-segment digits
    void addBingoStringToShadow(const char* str, const SegmentMap map[5][7]);
    
    // Fuel display: 4 × 7-seg + 1 single + 1 × 14-seg starburst
    void addFuelStringToShadow(const char* str, const SegmentMap map[6][14]);
    void addAlphaNumFuelStringToShadow(const char* str, const SegmentMap map[6][14]);
    
    // Bargraph pointer (e.g., nozzle position)
    void addPointerBarToShadow(int percent, const SegmentMap* barMap, int numBars);
    
    // Single-segment label (e.g., "L" or "R" indicator)
    void addLabelToShadow(const SegmentMap& label, const char* value);

    // ═══════════════════════════════════════════════════════════════════════
    // CLEAR HELPERS — These clear segments from shadow RAM
    // ═══════════════════════════════════════════════════════════════════════
    
    void clear7SegFromShadow(const SegmentMap* map, int numDigits);
    void clearBingoFromShadow(const SegmentMap map[5][7]);
    void clearFuelFromShadow(const SegmentMap map[6][14]);
    void clearLabelFromShadow(const SegmentMap* segMap);
    void clearBarFromShadow(const SegmentMap* barMap, int numBars);

private:
    HT1622* _chips[IFEI_MAX_CHIPS];
    uint8_t ramShadow[IFEI_MAX_CHIPS][64];  // Per-chip shadow RAM
    uint8_t lastShadow[IFEI_MAX_CHIPS][64]; // Last committed (for diff)
};

// Global instance
extern IFEIDisplay ifei;
```

### 10.4 Example: Writing to the IFEI

```cpp
// Write "123" to RPM Left display
ifei.addAsciiString7SegToShadow("123", (const SegmentMap*)IFEI_RPM_L_MAP, 3);

// Write fuel quantity
ifei.addAlphaNumFuelStringToShadow("12345", IFEI_FUEL_UP_MAP);

// Set nozzle position to 75%
ifei.addPointerBarToShadow(75, &IFEI_NOZZLE_L_MAP[0][0], 11);

// Don't forget to commit!
ifei.commit();
```

### 10.5 Key IFEI Fields

| Field | Type | Digits | Special Notes |
|-------|------|--------|---------------|
| IFEI_RPM_L/R | 7-seg | 3 | Standard numeric |
| IFEI_TEMP_L/R | 7-seg | 3 | Overlay logic for SP mode |
| IFEI_FF_L/R | 7-seg | 4 | Fuel flow |
| IFEI_FUEL_UP/DOWN | Mixed | 6 | 7-seg + 14-seg starburst |
| IFEI_BINGO | 7-seg | 5 | 3 digits + 2 single segments |
| IFEI_CLOCK_H/M/S | 7-seg | 2 each | Time display |
| Bargraphs | Pointer | 11 | Single-bar indicator |

### 10.3 IFEI Overlay Logic

The IFEI has complex overlay behavior where certain fields share physical display space:

```cpp
// From IFEIPanel.cpp
static volatile bool tempL_overlay = false, tempR_overlay = false;
static volatile bool sp_active = false;
static volatile bool fuel_mode_active = false;

// When SP (Set Fuel) mode is active, TEMP fields show SP values
// When SP exits, restore TEMP base values
```

### 10.4 Region-Based Commits

For efficiency, the IFEI uses region-based commits instead of full-frame commits:

```cpp
void IFEIDisplay::buildCommitRegions() {
    // Build a list of regions, each covering one display field's RAM range
    for (each field) {
        CommitRegion region = {
            .label = field.label,
            .chip = field.chipId,
            .addrStart = field.minAddr,
            .addrEnd = field.maxAddr
        };
        commitRegions[numCommitRegions++] = region;
    }
}

void IFEIDisplay::commitNextRegion() {
    // Commit one region per call (round-robin)
    const CommitRegion& region = commitRegions[currentRegion];
    chip->commitPartial(ramShadow, lastShadow, region.addrStart, region.addrEnd);
    currentRegion = (currentRegion + 1) % numCommitRegions;
}
```

---

## 11. TFT Gauge Overview

While this guide focuses on HT1622 segment displays, CockpitOS also supports TFT gauges. Here's a brief overview:

### 11.1 TFT vs Segment Displays

| Aspect | HT1622 Segment | TFT Gauge |
|--------|----------------|-----------|
| Resolution | 256 segments max | 240×240+ pixels |
| Update method | Bit manipulation | Sprite composition |
| Memory | ~64 bytes shadow | PSRAM for sprites |
| Library | Custom driver | LovyanGFX |
| Complexity | Medium | High |
| Power | Very low | Higher |

### 11.2 TFT Architecture (Brief)

```cpp
// TFT gauges use:
// 1. Background cache in PSRAM (day/night variants)
// 2. Sprite composition for needle/elements
// 3. Dirty-rect tracking to minimize redraws
// 4. DMA for non-blocking screen updates

// The pattern:
void TFT_draw(uint16_t value) {
    float angle = mapValueToAngle(value);
    Rect dirty = computeNeedleBounds(lastAngle, angle);
    blitBackground(dirty);
    drawNeedle(angle);
    flushRegion(dirty);
    lastAngle = angle;
}
```

### 11.3 Adapting Existing TFT Gauges

To create a new TFT gauge, copy an existing implementation (e.g., `TFT_Gauges_HydPress.cpp`) and modify:

1. Pin definitions
2. Background images
3. Needle geometry
4. Value-to-angle mapping
5. DCS-BIOS field subscription

---

## 12. Debugging Displays

### 12.1 Common Issues

```
┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  Scrambled digits (8 looks like 0, characters garbled)                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  CAUSE: SegmentMap order doesn't match font table order.                    │
│                                                                             │
│  FIX: Verify your SegmentMap indices follow:                                │
│       [0]=TOP, [1]=TOP-RIGHT, [2]=BOT-RIGHT, [3]=BOT,                        │
│       [4]=BOT-LEFT, [5]=TOP-LEFT, [6]=MIDDLE                                 │
│                                                                             │
│       Re-run RAM Walker if needed and REORDER your array.                   │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  Segments appear on wrong display half / ghost segments                     │
├─────────────────────────────────────────────────────────────────────────────┤
│  CAUSE: Wrong ledID (chip ID) in SegmentMap entries.                        │
│                                                                             │
│  FIX: Verify every entry has the correct chip ID:                           │
│       - ledID = 0 for chip 0 (usually left half)                            │
│       - ledID = 1 for chip 1 (usually right half)                           │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│  ISSUE                                                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│  Display doesn't update / stuck on one value                                │
├─────────────────────────────────────────────────────────────────────────────┤
│  CHECKS:                                                                    │
│  1. Is commit() being called in the display loop?                           │
│  2. Is lastShadow being properly initialized to 0xFF?                       │
│  3. Is the field registered in DisplayMapping.cpp?                          │
│  4. Is the DCS-BIOS label correct? (check JSON file)                        │
│  5. Enable DEBUG and check serial output for callbacks                      │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 12.2 Debug Flags

In `Config.h`:

```cpp
#define DEBUG                    1   // Enable debug output
#define DEBUG_LISTENERS_AT_STARTUP 1 // List all registered listeners
#define DEBUG_PERFORMANCE        1   // Profile display render time
```

### 12.3 Forcing a Display Test

```cpp
// In your panel's init, force all segments on:
void MyDisplay_init() {
    chip0.allSegmentsOn();
    delay(2000);  // Visual test
    chip0.clear();
}
```

---

## 13. Quick Reference

### 13.1 Callback Functions

| Callback | Signature | When Called |
|----------|-----------|-------------|
| `onLedChange` | `(label, value, max_value)` | LED/indicator changes |
| `onSelectorChange` | `(label, value)` | Selector position changes |
| `onMetaDataChange` | `(label, value)` | Metadata field changes |
| `onDisplayChange` | `(label, value)` | String display changes |
| `onAircraftName` | `(str)` | Mission start |

### 13.2 Subscription Functions

```cpp
subscribeToDisplayChange(label, callback);
subscribeToSelectorChange(label, callback);
subscribeToMetadataChange(label, callback);
subscribeToLedChange(label, callback);
```

### 13.3 Display Helper Functions

```cpp
// Check if string is blank
bool isFieldBlank(const char* s);

// Check mission state
bool isMissionRunning();
uint32_t msSinceMissionStart();
bool simReady();

// Force resync
void forceResync();
```

### 13.4 HT1622 API

```cpp
HT1622 chip(cs_pin, wr_pin, data_pin);
chip.init();
chip.clear();
chip.allSegmentsOn();
chip.allSegmentsOff();
chip.commit(shadow, lastShadow, length);
chip.commitPartial(shadow, lastShadow, startAddr, endAddr);
chip.writeNibble(addr, nibble);
```

### 13.5 SegmentMap Structure

```cpp
struct SegmentMap {
    uint8_t addr;   // 0-63
    uint8_t bit;    // 0-3
    uint8_t ledID;  // Chip index
};

// Invalid segment marker
{ 0xFF, 0xFF, chipId }
```

---

## What's Next?

1. **Run the RAM Walker** on your display hardware
2. **Document your segment mapping** carefully
3. **Create your SegmentMap.h** following font table order
4. **Register your fields** in DisplayMapping.cpp
5. **Subscribe to DCS-BIOS labels** in your panel init
6. **Test with replay mode** before connecting to DCS

---

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                                                                             │
│    Remember: The #1 cause of broken displays is segment order mismatch.     │
│                                                                             │
│    Trust the process:                                                       │
│    RAM Walker → Document → Reorder to font order → Test                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

*Guide version: 1.0 | Last updated: January 2025*  
*Reference implementation: IFEI (F/A-18C Hornet)*
