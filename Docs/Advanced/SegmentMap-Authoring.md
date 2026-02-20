# SegmentMap Authoring Guide

This guide covers how to create and maintain `*_SegmentMap.h` files for HT1622-based segment displays (IFEI, UFC, etc.).

**If you ignore this guide, your display output will be broken.**

---

## What a SegmentMap File Is

A SegmentMap file defines the **physical mapping** between logical display fields (digits, bargraphs, indicators) and the specific RAM addresses, bits, and **chip IDs (ledID)** for each segment in the display hardware.

Every `SegmentMap[]` array (e.g. `myMap[3][7]`) defines the segment order for each digit. **This order is almost never standard and must be documented.**

**The segment order in each array MUST match the software font table order.**

---

## Segment Order

The ONLY correct order is the one your **font tables** expect.

For standard 7-segment displays:
```
// For each digit [d][s]:
// [0]=TOP, [1]=TOP-RIGHT, [2]=BOTTOM-RIGHT, [3]=BOTTOM,
// [4]=BOTTOM-LEFT, [5]=TOP-LEFT, [6]=MIDDLE
```

If your RAM walker discovered segments in a different order (and it almost always does), **you must reorder them to match your font table**.

**DO NOT** copy the physical RAM walker order directly.

---

## Why This Matters

- If your SegmentMap order does NOT match your font tables, you WILL get scrambled digits, missing segments, or unreadable output. "8" will look like "0", "0" will look like a "C", etc.
- ALL digit/letter patterns in your font tables MUST map to the segments in the same index order as your SegmentMap.
- Hardware wiring is never consistent between displays. If you do not remap, your code WILL be broken.

---

## Chip ID (ledID) — Critical

**Every SegmentMap entry has a CHIP_ID (aka ledID). This is NOT optional.**

- If you set the wrong ledID, your digits/segments will appear on the wrong physical chip, the wrong display, or not at all.
- If you want two fields (e.g. clock and timer) to show on the same physical HT1622, they MUST have the same ledID.
- If you change wiring or want to move a field to a different display, update ledID everywhere in the array.

---

## Structure

Each entry: `{ RAM_ADDR, BIT, CHIP_ID }`

Array dimensions `[digit][segment]` — e.g., `[3][7]` = 3 digits, 7 segments per digit. The segment order is **custom** (must match font table).

You MUST document the meaning of each index above every map.

---

## How to Build and Verify

1. Use your RAM walker (or manual test code) to discover the physical on-screen order of each segment for each digit. Cycle every segment in hardware and write down the exact sequence as they appear.
2. Map EVERY discovered segment to the correct logical position in your font order.
3. For every map: add a comment block above it listing what each index means (e.g. `[0]=TOP`, `[1]=TOP-RIGHT`, etc.).
4. When writing/using font tables, make sure the index order matches this mapping.
5. Do NOT skip ledID. Do NOT skip comments.

---

## Bad Practices

- **Never** paste RAM walker output directly as your SegmentMap — this WILL scramble your digits.
- **Never** ignore chip ID (ledID) — if it's wrong, nothing will show, or you'll get ghosting/crossed-up panels.
- **Never** omit index documentation — without it, anyone maintaining the code will get the order wrong.

---

## Example

```cpp
// 7-segment (logical font order!)
// [0]=TOP, [1]=TOP-RIGHT, [2]=BOTTOM-RIGHT, [3]=BOTTOM,
// [4]=BOTTOM-LEFT, [5]=TOP-LEFT, [6]=MIDDLE
const SegmentMap myMap[3][7] = {
  { ... }, // Digit 1 (100s)
  { ... }, // Digit 2 (10s)
  { ... }, // Digit 3 (1s)
};
```

---

## Typical Usage

Calling `renderField("IFEI_SP", "SP ")` uses this map and the matching pattern for "S" and "P" in the documented order, on the correct chip/ledID.

If you get scrambled digits, missing bars, or ghost segments — you have the segment order or ledID wrong.
