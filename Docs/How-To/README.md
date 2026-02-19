# How-To Guides

Step-by-step tutorials for accomplishing specific hardware tasks with CockpitOS. These guides cover the practical details that are not immediately obvious from the hardware reference alone.

---

## Guides by Difficulty

### Getting Started

| Guide | Description | When You Need It |
|---|---|---|
| [Wire Analog Gauges](Wire-Analog-Gauges.md) | Drive physical needles with servo motors | You want a real needle gauge (fuel, oil pressure, RPM) that moves in response to DCS data |
| [Wire TFT Gauges](Wire-TFT-Gauges.md) | Render graphical instruments on TFT displays | You want a round or rectangular LCD showing a rendered gauge face with animated needles |
| [Wire Solenoid Switches](Wire-Solenoid-Switches.md) | Actuate toggle switches with solenoids | You want your physical toggles to snap to the correct position when DCS state changes |

### Intermediate

| Guide | Description | When You Need It |
|---|---|---|
| [Wire RS485 Network](Wire-RS485-Network.md) | Connect multiple panels on a shared bus | You have more than one ESP32 panel and want them all on one DCS-BIOS connection |
| [Wire Matrix Switches](Wire-Matrix-Switches.md) | Read multi-position rotary switches efficiently | You have rotary selectors with many positions and not enough GPIO pins to wire each one directly |
| [Wire Segment Displays](Wire-Segment-Displays.md) | Drive HT1622 segment LCD panels | You are building an IFEI, UFC, or other panel that uses multiplexed segment LCDs |

### Advanced

| Guide | Description | When You Need It |
|---|---|---|
| [Create Custom Panels](Create-Custom-Panel.md) | Write your own panel logic in C++ | The standard input/output model does not cover your use case and you need custom state machines or special hardware drivers |
| [Use Multiple Aircraft](Use-Multiple-Aircraft.md) | Manage label sets for different aircraft | You fly more than one aircraft module and want the same physical panel to work with each one |

---

## Prerequisites

All How-To guides assume you have:

1. Completed the [Getting Started](../Getting-Started/README.md) environment setup
2. Run the [Setup Tool](../Tools/Setup-Tool.md) at least once
3. A working [Quick Start](../Getting-Started/Quick-Start.md) panel (recommended)

---

## Conventions Used in These Guides

- `Config.h` settings are shown as `#define FLAG_NAME value`
- `CustomPins.h` settings are per-label-set (in `src/LABELS/LABEL_SET_YOURPANEL/`)
- Pin references use the `PIN(x)` macro for S2/S3 portability
- **Label Creator** refers to `LabelCreator-START.py`
- **Compiler Tool** refers to `CockpitOS-START.py`
- ASCII wiring diagrams show logical connections, not physical layout

---

*See also: [Hardware Reference](../Hardware/README.md) | [Config Reference](../Reference/Config.md) | [Tools Overview](../Tools/README.md)*
