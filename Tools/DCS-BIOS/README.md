# DCS-BIOS Arduino Library

> DCS-Skunkworks is looking for additional contributors for this repository.  If you have strong C coding skills and a passion for DCS-BIOS, please contact talbotmcinnis@alumni.uwo.ca

This is an Arduino library that makes it easy to write sketches that talk to DCS-BIOS.

---

## ESP32 RS485 Support (This Fork)

This fork adds **native RS485 Master and Slave support for all ESP32 variants**:

| Chip | Cores | USB | Status |
|------|-------|-----|--------|
| ESP32 (Classic) | Dual | USB-to-Serial | Supported |
| ESP32-S2 | Single | Native CDC | Supported |
| ESP32-S3 | Dual | Native CDC | Supported |
| ESP32-C3 | Single (RISC-V) | Serial/JTAG | Supported |
| ESP32-C6 | Single (RISC-V) | Serial/JTAG | Supported |

### Features

- **Zero risk to AVR users** - ESP32 code only activates when compiling for ESP32
- **Same API as AVR** - Uses identical `DCSBIOS_RS485_MASTER` and `DCSBIOS_RS485_SLAVE` defines
- **Protocol compatible** - Works with existing AVR masters/slaves on the same bus
- **Multi-bus master** - Supports up to 3 independent RS485 buses
- **ISR-driven RX** - Bare-metal UART with interrupt-driven receive for lowest latency
- **All ESP32 variants** - Including RISC-V chips (C3, C6) with proper memory barriers

### Quick Start

**Master:**
```cpp
#define DCSBIOS_DISABLE_SERVO
#define DCSBIOS_RS485_MASTER
#include <DcsBios.h>

void setup() { DcsBios::setup(); }
void loop() { DcsBios::loop(); }
```

**Slave:**
```cpp
#define DCSBIOS_DISABLE_SERVO
#define DCSBIOS_RS485_SLAVE 1  // Address 1-127
#include <DcsBios.h>

void setup() { DcsBios::setup(); }
void loop() { DcsBios::loop(); }
```

### Pin Configuration

Define pins before `#include <DcsBios.h>` to override defaults:

```cpp
#define RS485_TX_PIN  17   // Default: 17
#define RS485_RX_PIN  16   // Default: 16
#define RS485_DE_PIN  -1   // -1 = auto-direction board
```

See `examples/ESP32_RS485Master/` and `examples/ESP32_RS485Slave/` for complete examples.

---

For more information and documentation, see the [DCS-BIOS FlightPanels Project](https://github.com/DCSFlightpanels).  The example "OneOfEverything" is a good place to start looking for controls to use.

## Origins

DCS-BIOS was originally developed here, [DCS-BIOS project.](https://github.com/dcs-bios/dcs-bios)  DCS-BIOS Flightpanels forked all of DCS-BIOS in order to initially provide support for Saitek Flight Panels.  As of Nov, 2020, the original project has received limited support (despite a major update for HUB), and FlightPanels is gaining in popularity due to it's support.  Late in 2020, this fork of the arduino-only portion was created to become the "official" branch associated with the FlightPanels DCS-BIOS side.

## Support & Documentation

This is a community maintained plugin.  Support is best found at the DCS-Flightpanels discord channel.  The wiki is very much a work in progress, and can be found here: https://github.com/DCSFlightpanels/dcs-bios-arduino-library/wiki

## Releasing

1. Bump version number in library.properties
2. Run make_release, providing the same version number when prompted.
3. Manually make a zip file of the folder created in /Releases, and upload to github.
