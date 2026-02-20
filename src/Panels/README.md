# CockpitOS Panel Implementations

This directory contains the panel `.cpp` files that implement aircraft-specific cockpit logic.

---

## Files

| File | Description |
|------|-------------|
| `Generic.cpp` | Handles most panels — standard button/switch/LED logic that works for any aircraft panel without custom behavior |
| `IFEIPanel.cpp` | Custom logic for the IFEI (In-Flight Engine Instrument) display panel |
| `WingFold.cpp` | Custom logic for wing fold panel |
| `TestPanel.cpp` | Test/debug panel for development |
| `TFT_Gauges_CabinPressure.cpp` | TFT gauge: cabin pressure altimeter |
| `TFT_Gauges_RadarAlt.cpp` | TFT gauge: radar altimeter |
| `TFT_Gauges_Battery.cpp` | TFT gauge: battery voltage |
| `TFT_Gauges_BrakePress.cpp` | TFT gauge: brake pressure |
| `TFT_Gauges_HydPress.cpp` | TFT gauge: hydraulic pressure |
| `TFT_Gauges_CabPressTest.cpp` | TFT gauge: cabin pressure test variant |
| `TFT_Display_CMWS.cpp` | TFT display: CMWS (Common Missile Warning System) |

## Subdirectories

- `Assets/` — TFT gauge bitmap assets (background images, needle sprites, PROGMEM headers)
- `includes/` — Shared panel header files

## How Panels Work

Panels are registered using the `REGISTER_PANEL()` macro and conditionally compiled via `#if defined(HAS_*)` guards in the label set's `selected_panels.txt`. `Generic.cpp` handles the vast majority of panels — custom panel files are only needed when a panel requires logic beyond standard input/output mapping.
