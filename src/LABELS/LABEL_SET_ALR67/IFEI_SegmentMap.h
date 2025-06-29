/*
================================================================================
SEGMENT MAP FILE (IFEI_SegmentMap.h, etc.)
================================================================================

**DO NOT SKIP THIS BLOCK. IF YOU IGNORE THIS, YOUR DISPLAY OUTPUT WILL BE BROKEN.**

WHAT THIS FILE IS:
------------------
This file defines the *PHYSICAL mapping* between logical display fields (digits, bargraphs, indicators)
and the SPECIFIC RAM addresses, bits, and ***chip IDs (ledID)*** for each segment in the display hardware.

ABSOLUTELY EVERY SegmentMap[] array (e.g. myMap[3][7]) defines the segment order for each digit.
**THIS ORDER IS *ALMOST NEVER STANDARD* AND MUST BE DOCUMENTED BELOW.**

***THE SEGMENT ORDER IN EACH ARRAY _MUST_ MATCH THE SOFTWARE FONT TABLE ORDER.***

EXACT SEGMENT ORDER:
--------------------
The ONLY correct order is the one your FONT TABLES expect. 
EXAMPLE for standard 7-segment displays:
  // For each digit [d][s]:
  // [0]=TOP, [1]=TOP-RIGHT, [2]=BOTTOM-RIGHT, [3]=BOTTOM, [4]=BOTTOM-LEFT, [5]=TOP-LEFT, [6]=MIDDLE

If your RAM walker discovered them in a DIFFERENT order (and it almost always does), ***YOU MUST REORDER THEM HERE***.
**DO NOT** copy the physical RAM walker order directly! You MUST create a mapping that matches your logical font order.

WHY THIS MATTERS:
-----------------
- If your SegmentMap order does NOT match your font tables, you WILL get scrambled digits, missing segments, or unreadable junk. "8" will look like "0", "0" will look like a 'C', etc.
- ALL digit/letter patterns in your font tables MUST map to the segments in the same index order in your SegmentMap.
- Hardware wiring is *never* consistent between displays. If you do not remap, your code WILL be broken.

ABSOLUTE WARNING ON ledID (CHIP ID):
-------------------------------------
**EVERY SegmentMap entry has a CHIP_ID (aka ledID).**
THIS IS *NOT* OPTIONAL. If you set the wrong ledID, your digits/segments will appear on the wrong physical chip, the wrong display, or not at all.
BE OBSESSIVE: CHECK THE ledID FOR EVERY ENTRY. 
**If you want two fields (e.g. clock and timer) to show on the same physical HT1622, THEY MUST HAVE THE SAME ledID!**
If you change wiring or want to move a field to a different display, update ledID everywhere in the array.

STRUCTURE:
----------
- Each entry:   `{ RAM_ADDR, BIT, CHIP_ID }`
- [digit][segment]: e.g., `[3][7]` = 3 digits, 7 segments per digit, but segment order is **custom!**
- YOU MUST DOCUMENT THE MEANING OF EACH [x][y] INDEX ABOVE EVERY MAP.
- **If you do not, anyone (including LLMs) will fuck this up. Do not trust auto-generation.**

HOW TO BUILD/VERIFY:
--------------------
1. Use your RAM walker (or manual test code) to discover the *physical* on-screen order of each segment for each digit. This means: cycle every segment in hardware, WRITE DOWN THE EXACT SEQUENCE as they appear.
2. ***Map EVERY discovered segment to the correct logical position in your font order!***
3. For every map: Add a comment block above it that lists what each index means, e.g. [0]=TOP, [1]=TOP-RIGHT, etc.
4. When writing/using font tables (digits or alpha), ***make sure the index order matches this mapping***.
5. **Do NOT skip ledID. Do NOT skip comments.**

NEVER DO THIS (Bad Examples):
-----------------------------
- Never paste RAM walker output directly as your SegmentMap! This WILL SCRAMBLE YOUR DIGITS.
- Never ignore chip ID (ledID)! If it's wrong, nothing will show, or you'll get ghosting/crossed-up panels.

TYPICAL USE CASE:
-----------------
- Calling `renderField("IFEI_SP", "SP ")` uses this map and the matching pattern for "S" and "P" in the DOCUMENTED ORDER and on the correct CHIP/ledID.

================================================================================
EXAMPLE — HOW TO DOCUMENT ORDER (REQUIRED!):

// 7-segment (logical font order!)
// [0]=TOP, [1]=TOP-RIGHT, [2]=BOTTOM-RIGHT, [3]=BOTTOM, [4]=BOTTOM-LEFT, [5]=TOP-LEFT, [6]=MIDDLE
const SegmentMap myMap[3][7] = {
  { ... }, // Digit 1 (100s)
  { ... }, // Digit 2 (10s)
  { ... }, // Digit 3 (1s)
};
// If your hardware order is different, MAP IT TO THIS LOGIC. DO NOT SKIP!

================================================================================
*FINAL WARNING:* If you get scrambled digits, missing bars, or ghost segments, YOU HAVE IGNORED THIS BLOCK..
================================================================================
*/

#pragma once

// -- LEFT ENGINE RPM --
const SegmentMap leftRpmMap[3][7] = {
    // 100s
    { {0,0,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0} },
    // 10s
    { {2,3,0}, {2,2,0}, {2,1,0}, {2,0,0}, {0,1,0}, {0,3,0}, {0,2,0} },
    // 1s
    { {6,3,0}, {6,2,0}, {6,1,0}, {6,0,0}, {4,1,0}, {4,3,0}, {4,2,0} }
};

// RPM Label (RPM)
const SegmentMap rpmLabel = {5, 0, 0};

// -- RIGHT ENGINE RPM --
const SegmentMap rightRpmMap[3][7] = {
    // 100s
    { {1,0,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0} },
    // 10s
    { {3,3,0}, {3,2,0}, {3,1,0}, {3,0,0}, {1,1,0}, {1,3,0}, {1,2,0} },
    // 1s
    { {7,3,0}, {7,2,0}, {7,1,0}, {7,0,0}, {5,1,0}, {5,3,0}, {5,2,0} }
};

// -- FUEL FLOW LEFT --
const SegmentMap fuelFlowLeftMap[3][7] = {
    { {9,0,0}, {10,3,0}, {10,1,0}, {8,1,0}, {8,2,0}, {8,3,0}, {10,2,0} },
    { {13,0,0}, {14,3,0}, {14,1,0}, {12,1,0}, {12,2,0}, {12,3,0}, {14,2,0} },
    { {17,0,0}, {18,3,0}, {18,1,0}, {16,1,0}, {16,2,0}, {16,3,0}, {18,2,0} }
};

// Fuel Flow Label (FFx100)
const SegmentMap fuelFlowLabel = {16, 0, 0};

// -- FUEL FLOW RIGHT --
const SegmentMap fuelFlowRightMap[3][7] = {
    { {21,0,0}, {22,3,0}, {22,1,0}, {20,1,0}, {20,2,0}, {20,3,0}, {22,2,0} },
    { {25,0,0}, {26,3,0}, {26,1,0}, {24,1,0}, {24,2,0}, {24,3,0}, {26,2,0} },
    { {29,0,0}, {30,3,0}, {30,1,0}, {28,1,0}, {28,2,0}, {28,3,0}, {30,2,0} }
};

// -- (SP) TEMP LEFT (Used by SP TEST MODE) --
const SegmentMap spTempLeftMap[3][7] = {
    { {11,3,0}, {11,2,0}, {11,1,0}, {11,0,0}, {9,1,0}, {9,3,0}, {9,2,0} },
    { {15,3,0}, {15,2,0}, {15,1,0}, {15,0,0}, {13,1,0}, {13,3,0}, {13,2,0} },
    { {19,3,0}, {19,2,0}, {19,1,0}, {19,0,0}, {17,1,0}, {17,3,0}, {17,2,0} }
};

// -- TEMP LEFT --
const SegmentMap tempLeftMap[3][7] = {
    { {11,3,0}, {11,2,0}, {11,1,0}, {11,0,0}, {9,1,0}, {9,3,0}, {9,2,0} },
    { {15,3,0}, {15,2,0}, {15,1,0}, {15,0,0}, {13,1,0}, {13,3,0}, {13,2,0} },
    { {19,3,0}, {19,2,0}, {19,1,0}, {19,0,0}, {17,1,0}, {17,3,0}, {17,2,0} }
};

// Temperature Label (TEMP)
const SegmentMap tempLabel = {18, 0, 0};

// -- (CODES) TEMP RIGHT --
const SegmentMap codesTempRightMap[3][7] = {
    { {23,3,0}, {23,2,0}, {23,1,0}, {23,0,0}, {21,1,0}, {21,3,0}, {21,2,0} },
    { {27,3,0}, {27,2,0}, {27,1,0}, {27,0,0}, {25,1,0}, {25,3,0}, {25,2,0} },
    { {31,3,0}, {31,2,0}, {31,1,0}, {31,0,0}, {29,1,0}, {29,3,0}, {29,2,0} }
};

// -- TEMP RIGHT --
const SegmentMap tempRightMap[3][7] = {
    { {23,3,0}, {23,2,0}, {23,1,0}, {23,0,0}, {21,1,0}, {21,3,0}, {21,2,0} },
    { {27,3,0}, {27,2,0}, {27,1,0}, {27,0,0}, {25,1,0}, {25,3,0}, {25,2,0} },
    { {31,3,0}, {31,2,0}, {31,1,0}, {31,0,0}, {29,1,0}, {29,3,0}, {29,2,0} }
};

// Nozzle Label+arc (NOZ)
const SegmentMap nozLabel = {37, 3, 0};

// -- NOZZLE BARGRAPH LEFT (11 segments, 0–100%) --
const SegmentMap nozBarLeftMap[11] = {
    {14,0,0}, {12,0,0}, {10,0,0}, {42,0,0}, {42,1,0}, {42,2,0}, {42,3,0}, {43,0,0}, {43,1,0}, {43,2,0}, {43,3,0}
};

// -- NOZZLE BARGRAPH RIGHT (11 segments, 0–100%) --
const SegmentMap nozBarRightMap[11] = {
    {20,0,0}, {24,0,0}, {26,0,0}, {32,0,0}, {32,1,0}, {32,2,0}, {32,3,0}, {33,0,0}, {33,1,0}, {33,2,0}, {33,3,0}
};

// -- NOZZLE POINTER LABEL LEFT (1 or 0) 
// const SegmentMap leftNozLabelPointerMap[1] = { {0xFF, 0xFF, 0xFF} };
const SegmentMap leftNozLabelPointerMap[1] = { {43,3,0} };

// -- NOZZLE POINTER LABEL RIGHT (1 or 0)
// const SegmentMap rightNozLabelPointerMap[1] = { {0xFF, 0xFF, 0xFF} };
const SegmentMap rightNozLabelPointerMap[1] = { {33,3,0} };

const SegmentMap oilLeftMap[3][7] = {
    // 100s digit (only one segment, rest are unused)
    { {40,3,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0} },
    // 10s digit (TOP, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM, BOTTOM_LEFT, TOP_LEFT, MIDDLE)
    { {38,3,0}, {38,2,0}, {38,1,0}, {38,0,0}, {40,0,0}, {40,2,0}, {40,1,0} },
    // 1s digit (TOP, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM, BOTTOM_LEFT, TOP_LEFT, MIDDLE)
    { {34,3,0}, {34,2,0}, {34,1,0}, {34,0,0}, {36,0,0}, {36,2,0}, {36,1,0} }
};

// OIL Label (use your provided segment info)
const SegmentMap oilLabel = {36, 3, 0};  // RAM addr 36, bit 3, ledID 0

const SegmentMap oilRightMap[3][7] = {
    // 100s digit (only one segment, rest unused)
    { {41,3,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0}, {0xFF,0xFF,0} },
    // 10s digit (TOP, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM, BOTTOM_LEFT, TOP_LEFT, MIDDLE)
    { {39,3,0}, {39,2,0}, {39,1,0}, {39,0,0}, {41,0,0}, {41,2,0}, {41,1,0} },
    // 1s digit (TOP, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM, BOTTOM_LEFT, TOP_LEFT, MIDDLE)
    { {35,3,0}, {35,2,0}, {35,1,0}, {35,0,0}, {37,0,0}, {37,2,0}, {37,1,0} }
};


// Starburst (14-Seg) order Mapping. See Seg-14 image for tweaking
//
// [0]  = Bottom Right
// [1]  = Middle Right
// [2]  = Top Right
// [3]  = Inner Bottom Right
// [4]  = Inner Bottom Center
// [5]  = Inner Top Right
// [6]  = Top
// [7]  = Bottom
// [8]  = Inner Bottom Left
// [9]  = Inner Top Center
// [10] = Inner Top Left
// [11] = Lower Left
// [12] = Middle Left
// [13] = Top Left

// FUEL LEFT Label (e.g., "FUEL" or custom indicator for the left fuel field)
const SegmentMap fuelLeftLabel = {21, 0, 1};   // RAM addr 21, bit 0, chip 1
const SegmentMap fuelLeftMap[6][14] = {
    // 100000s (leftmost)
    { {43,3,1}, {41,3,1}, {41,1,1}, {43,0,1}, {43,1,1}, {43,2,1}, {41,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 10000s
    { {39,3,1}, {37,3,1}, {37,1,1}, {39,0,1}, {39,1,1}, {39,2,1}, {37,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 1000s
    { {35,3,1}, {33,3,1}, {33,1,1}, {35,0,1}, {35,1,1}, {35,2,1}, {33,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 100s (LEFT FUEL)
    { {31,3,1}, {29,3,1}, {29,1,1}, {31,0,1}, {31,1,1}, {31,2,1}, {29,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 10s (single segment, only slot 0 used)
    { {27,0,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 1s (starburst, rightmost, remapped to industry standard bit order)
    { {21,1,1}, {21,2,1}, {21,3,1}, {23,0,1}, {23,1,1}, {23,2,1}, {23,3,1}, {25,0,1}, {25,1,1}, {25,2,1}, {25,3,1}, {27,1,1}, {27,2,1}, {27,3,1} }
};

// FUEL RIGHT Label (e.g., "FUEL" or custom indicator for the left fuel field)
const SegmentMap fuelRightLabel = {20, 0, 1};   // RAM addr 20, bit 0, chip 1
const SegmentMap fuelRightMap[6][14] = {
    // 100000s (RIGHT FUEL)
    { {41,0,1}, {40,3,1}, {40,2,1}, {40,1,1}, {42,1,1}, {42,3,1}, {42,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 10000s (RIGHT FUEL)
    { {37,0,1}, {36,3,1}, {36,2,1}, {36,1,1}, {38,1,1}, {38,3,1}, {38,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 1000s (RIGHT FUEL)
    { {33,0,1}, {32,3,1}, {32,2,1}, {32,1,1}, {34,1,1}, {34,3,1}, {34,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 100s (RIGHT FUEL)
    { {29,0,1}, {28,3,1}, {28,2,1}, {28,1,1}, {30,1,1}, {30,3,1}, {30,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 10s (single segment, only slot 0 used)
    { {26,0,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 1s (starburst, rightmost, direct copy—unchanged)
    { {20,1,1}, {20,2,1}, {20,3,1}, {22,0,1}, {22,1,1}, {22,2,1}, {22,3,1}, {24,0,1}, {24,1,1}, {24,2,1}, {24,3,1}, {26,1,1}, {26,2,1}, {26,3,1} }
};

// Set Clocks
const SegmentMap tModeUP[6][14] = {
    // 100000s (leftmost)
    { {43,3,1}, {41,3,1}, {41,1,1}, {43,0,1}, {43,1,1}, {43,2,1}, {41,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 10000s
    { {39,3,1}, {37,3,1}, {37,1,1}, {39,0,1}, {39,1,1}, {39,2,1}, {37,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 1000s
    { {35,3,1}, {33,3,1}, {33,1,1}, {35,0,1}, {35,1,1}, {35,2,1}, {33,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 100s
    { {31,3,1}, {29,3,1}, {29,1,1}, {31,0,1}, {31,1,1}, {31,2,1}, {29,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 10s (single segment, only slot 0 used)
    { {27,0,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 1s (starburst, rightmost, remapped to industry standard bit order)
    { {21,1,1}, {21,2,1}, {21,3,1}, {23,0,1}, {23,1,1}, {23,2,1}, {23,3,1}, {25,0,1}, {25,1,1}, {25,2,1}, {25,3,1}, {27,1,1}, {27,2,1}, {27,3,1} }
};

const SegmentMap timeSetModeDown[6][14] = {
    // 100000s 
    { {41,0,1}, {40,3,1}, {40,2,1}, {40,1,1}, {42,1,1}, {42,3,1}, {42,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 10000s 
    { {37,0,1}, {36,3,1}, {36,2,1}, {36,1,1}, {38,1,1}, {38,3,1}, {38,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 1000s 
    { {33,0,1}, {32,3,1}, {32,2,1}, {32,1,1}, {34,1,1}, {34,3,1}, {34,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 100s 
    { {29,0,1}, {28,3,1}, {28,2,1}, {28,1,1}, {30,1,1}, {30,3,1}, {30,2,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 10s 
    { {26,0,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1},
      {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },

    // 1s (starburst, rightmost, direct copy—unchanged)
    { {20,1,1}, {20,2,1}, {20,3,1}, {22,0,1}, {22,1,1}, {22,2,1}, {22,3,1}, {24,0,1}, {24,1,1}, {24,2,1}, {24,3,1}, {26,1,1}, {26,2,1}, {26,3,1} }
};


// BINGO LABEL
const SegmentMap bingoLabel = {44, 3, 1};

// BINGO FIELD [10000s][1000s][100s][10s][1s]
const SegmentMap bingoMap[5][7] = {
    // 10000s (leftmost, 7-seg): TOP, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM, BOTTOM_LEFT, TOP_LEFT, MIDDLE
    { {45,0,1}, {45,1,1}, {45,2,1}, {45,3,1}, {47,3,1}, {47,1,1}, {47,2,1} },
    // 1000s (7-seg)
    { {40,0,1}, {38,0,1}, {4,0,1}, {2,0,1}, {0,0,1}, {42,0,1}, {6,0,1} },
    // 100s (7-seg)
    { {34,0,1}, {32,0,1}, {14,0,1}, {10,0,1}, {8,0,1}, {36,0,1}, {16,0,1} },
    // 10s (single segment only: slot [0])
    { {30,0,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },
    // 1s (single segment only: slot [0])
    { {28,0,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} }
};

// Zulu label
const SegmentMap zuluLabel   = {18,0,1};

const SegmentMap timeHHMap[14] = {
    // HH 10s (Digit 1)
    {46,0,1}, {44,0,1}, {44,2,1}, {46,3,1}, {46,2,1}, {46,1,1}, {44,1,1},
    // HH 1s (Digit 2)
    {0,1,1},  {2,1,1},  {2,3,1},  {1,0,1},  {0,3,1},  {0,2,1},  {2,2,1},
};

const SegmentMap timeMMMap[14] = {
    // MM 10s (Digit 1)
    {4,1,1}, {6,1,1}, {6,3,1}, {5,0,1}, {4,3,1}, {4,2,1}, {6,2,1},
    // MM 1s (Digit 2)
    {8,1,1}, {10,1,1}, {10,3,1}, {9,0,1}, {8,3,1}, {8,2,1}, {10,2,1},
};

const SegmentMap timeSSMap[14] = {
    // SS 10s (Digit 1)
    {12,0,1}, {14,1,1}, {14,3,1}, {12,3,1}, {12,2,1}, {12,1,1}, {14,2,1},
    // SS 1s (Digit 2)
    {16,1,1}, {18,1,1}, {18,3,1}, {17,0,1}, {16,3,1}, {16,2,1}, {18,2,1},
};

// TIME COLON SEPARATOR (IFEI_DD_1) - Single segment indicator
const SegmentMap timeColonLabel = {13, 0, 1};   // RAM addr 13, bit 0, chip 1

const SegmentMap timerHoursMap[2][7] = {
    { {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1}, {0xFF,0xFF,1} },
    { {3,0,1}, {3,1,1}, {3,2,1}, {3,3,1}, {1,3,1}, {1,1,1}, {1,2,1} }
};

const SegmentMap timerMinutesMap[2][7] = {
    { {7,0,1}, {7,1,1}, {7,2,1}, {7,3,1}, {5,3,1}, {5,1,1}, {5,2,1} },
    { {11,0,1}, {11,1,1}, {11,2,1}, {11,3,1}, {9,3,1}, {9,1,1}, {9,2,1} }
};

const SegmentMap timerSecondsMap[2][7] = {
    { {15,0,1}, {15,1,1}, {15,2,1}, {15,3,1}, {13,3,1}, {13,1,1}, {13,2,1} },
    { {19,0,1}, {19,1,1}, {19,2,1}, {19,3,1}, {17,3,1}, {17,1,1}, {17,2,1} }
};

#ifdef SEGMENT_LABEL_MAP_TABLE
// Label-to-segment map table for code generation and validation.
// This is NOT compiled or linked: used ONLY for auto-generation tools and code audits.
// Format: { "LABEL", SEGMENT_MAP_POINTER },
// You can comment, duplicate, or edit as needed. Duplicates are allowed for clarity.

static const struct {
    const char* label;
    const void* segmentMap;
} segmentLabelMap[] = {
    { "IFEI_LPOINTER_TEXTURE", leftNozLabelPointerMap },
    { "IFEI_RPOINTER_TEXTURE", rightNozLabelPointerMap },
    { "IFEI_BINGO", &bingoMap[0][0] },
    { "IFEI_BINGO_TEXTURE", &bingoLabel },
    { "IFEI_CLOCK_H", timeHHMap },
    { "IFEI_CLOCK_M", timeMMMap },
    { "IFEI_CLOCK_S", timeSSMap },
    { "IFEI_Z_TEXTURE", &zuluLabel },
    { "IFEI_FF_L", &fuelFlowLeftMap[0][0] },
    { "IFEI_FF_R", &fuelFlowRightMap[0][0] },
    { "IFEI_FF_TEXTURE", &fuelFlowLabel },
    { "IFEI_TEMP_L", &tempLeftMap[0][0] },
    { "IFEI_TEMP_R", &tempRightMap[0][0] },
    { "IFEI_TEMP_TEXTURE", &tempLabel },
    { "IFEI_OIL_PRESS_L", &oilLeftMap[0][0] },
    { "IFEI_OIL_PRESS_R", &oilRightMap[0][0] },
    { "IFEI_OIL_TEXTURE", &oilLabel },
    { "IFEI_RPM_L", &leftRpmMap[0][0] },
    { "IFEI_RPM_R", &rightRpmMap[0][0] },
    { "IFEI_RPM_TEXTURE", &rpmLabel },
    { "IFEI_NOZ_TEXTURE", &nozLabel },
    { "IFEI_FUEL_UP", &fuelLeftMap[0][0] },
    { "IFEI_FUEL_DOWN", &fuelRightMap[0][0] },
    { "IFEI_L_TEXTURE", &fuelLeftLabel },
    { "IFEI_R_TEXTURE", &fuelRightLabel },
    { "IFEI_SP", &spTempLeftMap[0][0] },
    { "IFEI_CODES", &codesTempRightMap[0][0] },
    { "IFEI_T", &tModeUP[0][0] },
    { "IFEI_TIME_SET_MODE", &timeSetModeDown[0][0] },
    { "IFEI_DD_1", &timeColonLabel },
    { "IFEI_TIMER_H", &timerHoursMap[0][0] },
    { "IFEI_TIMER_M", &timerMinutesMap[0][0] },
    { "IFEI_TIMER_S", &timerSecondsMap[0][0] },
    // ...add more mappings as needed
};
#endif // SEGMENT_LABEL_MAP_TABLE
