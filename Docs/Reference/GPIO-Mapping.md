# GPIO Mapping — PIN() Macro

CockpitOS label sets use **ESP32-S2 Mini GPIO numbers** as the canonical pin namespace. The `PIN()` macro (defined in `src/PinMap.h`) transparently converts these to the correct GPIO for the target board at compile time.

## Why This Exists

TEK Creations PCBs use the Lolin S2 Mini footprint. If you swap an S2 Mini for an **S3 Mini** (same physical footprint, different GPIO assignments), the `PIN()` macro remaps automatically — no wiring or config changes needed.

## How It Works

```cpp
// src/PinMap.h
#if defined(ARDUINO_LOLIN_S3_MINI)
  #define PIN(n) (map_s2_to_board(n))   // remap S2 → S3
#else
  #define PIN(n) (n)                     // identity (S2, S3 DevKit, etc.)
#endif
```

On S3 Mini, `PIN(12)` returns `10`. On S2 Mini (or any other board), `PIN(12)` returns `12`.

## S2 Mini → S3 Mini Conversion Table

| S2 Mini GPIO | S3 Mini GPIO (same physical position) |
|:------------:|:-------------------------------------:|
| 2            | 3                                     |
| 3            | 2                                     |
| 4            | 5                                     |
| 5            | 4                                     |
| 7            | 12                                    |
| 8            | 7                                     |
| 9            | 13                                    |
| 10           | 8                                     |
| 12           | 10                                    |
| 13           | 9                                     |
| 33           | 35                                    |
| 35           | 36                                    |
| 36           | 38                                    |
| 37           | 44                                    |
| 38           | 37                                    |
| 39           | 43                                    |
| 40           | 33                                    |

Any GPIO not in this table passes through unchanged (`default: return n`).

## Usage

Always use `PIN(x)` in `InputMapping.h`, `LEDMapping.h`, and `CustomPins.h`:

```cpp
{ "FIRE_BTN", "GPIO", PIN(4), -1, -1, "FIRE_BTN", 1, "momentary", 0 },
```

Direct GPIO numbers (without `PIN()`) will work on S2 but break on S3.
