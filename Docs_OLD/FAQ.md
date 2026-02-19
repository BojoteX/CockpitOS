# CockpitOS — Frequently Asked Questions

Quick answers to common questions. For detailed guides, see the `/docs/` folder.

---

## General

**What is CockpitOS?**  
Firmware for ESP32 microcontrollers that interfaces physical cockpit panels with DCS World via DCS-BIOS. It handles inputs (buttons, switches, encoders), outputs (LEDs, displays), and communication with the simulator.

**Which aircraft are supported?**  
Any aircraft that DCS-BIOS supports. CockpitOS uses "Label Sets" to configure for specific aircraft. The F/A-18C Hornet is the primary reference implementation, but you can create label sets for any module.

**Is this only for DCS World?**  
Currently, yes. CockpitOS is built around the DCS-BIOS protocol. The HID gamepad output could theoretically work with other simulators, but DCS-BIOS integration is the core feature.

**What's the difference between CockpitOS and DCS-BIOS?**  
DCS-BIOS is a LUA script that runs inside DCS World and exports cockpit data. CockpitOS is firmware that runs on your ESP32 and receives that data to drive physical hardware.

---

## Hardware

**Which ESP32 board should I use?**  
ESP32-S2 or ESP32-S3 with native USB are recommended. They simplify USB communication and have good performance. The LOLIN S2 Mini and S3 Mini are popular choices.

**Can I use the original ESP32 (no native USB)?**  
Yes, but you'll need to use Serial or WiFi transport instead of USB. Native USB boards are simpler to set up.

**How many inputs/outputs can I have?**  
Depends on your configuration. A single ESP32 can handle dozens of direct GPIO inputs plus hundreds more via I²C expanders (PCA9555), shift registers (74HC165), and LED drivers (WS2812, TM1637, GN1640T). Large cockpit builds may use multiple ESP32 boards.

**What displays are supported?**  
- **HT1622 segment LCDs** — Used for IFEI, UFC, and similar numeric/alphanumeric displays
- **SPI TFT displays** — For gauges like CMWS, using LovyanGFX library
- **TM1637 7-segment modules** — Simple numeric displays
- **WS2812 addressable LEDs** — For indicators with color/brightness control

**Can I mix different hardware types?**  
Yes. A single label set can include GPIO buttons, PCA9555 expanders, WS2812 LEDs, HT1622 displays, and more. CockpitOS routes everything through unified abstractions.

---

## Setup & Configuration

**How do I get started?**  
1. Install Arduino IDE 2.x with ESP32 Arduino Core 3.3+
2. Open CockpitOS in the IDE
3. Choose a label set (start with `LABEL_SET_TEST_ONLY`)
4. Configure transport in `Config.h`
5. Compile and upload

See `docs/GETTING_STARTED.md` for the complete walkthrough.

**What is a "Label Set"?**  
A folder containing configuration files that define which inputs/outputs your panel has, which GPIO pins they use, and which DCS-BIOS commands they map to. Each aircraft or panel typically has its own label set.

**How do I create a label set for my aircraft?**  
1. Copy an existing label set folder
2. Download your aircraft's JSON from DCS-BIOS
3. Edit `InputMapping.h` and `LEDMapping.h` to define your hardware
4. Run `python generate_data.py` to generate the data files

See `docs/CREATING_LABEL_SETS.md` for details.

**Do I need to modify the core firmware?**  
No. All customization happens in label sets and panel files. The core firmware handles protocol parsing, timing, and hardware abstraction automatically.

---

## Connectivity

**USB, WiFi, or Serial — which should I use?**  
- **USB** — Recommended. Lowest latency, simplest setup, works with HID Manager
- **WiFi** — Good for wireless panels or when USB ports are limited
- **Serial** — Legacy option, works but requires additional bridging software

**What is the HID Manager?**  
A Python application that bridges USB HID communication between your ESP32 panels and DCS-BIOS. It handles device discovery, data routing, and supports multiple panels simultaneously.

**Can I run multiple ESP32 panels?**  
Yes. The HID Manager supports up to 32 devices. Each panel can have its own label set and hardware configuration.

**Why isn't my panel connecting?**  
Common causes:
- Wrong transport mode in `Config.h`
- DCS-BIOS not running or misconfigured
- USB cable is charge-only (no data)
- HID Manager not running (for USB mode)

See `docs/TRANSPORT_MODES.md` for troubleshooting.

---

## Inputs

**How do I wire a button?**  
Connect one side to GPIO, the other to GND. Enable the internal pull-up in your InputMapping. CockpitOS handles debouncing automatically.

**How do I wire a rotary encoder?**  
Connect A and B pins to GPIO (with pull-ups), common to GND. Define the encoder in InputMapping with type `ENCODER_NORMAL` or `ENCODER_REVERSED`.

**What's the difference between a selector and a button?**  
- **Button** — Momentary, sends a pulse when pressed
- **Selector** — Multi-position switch, sends position changes

**How do I handle a multi-position rotary switch?**  
Use a matrix rotary configuration with strobe pins and a data pin, or wire each position to a separate GPIO with mutual exclusivity groups.

**Can I use analog inputs (potentiometers)?**  
Yes. Define them with `INPUT_ANALOG` type. CockpitOS supports self-calibration and dead zones.

---

## Outputs

**How do I control an LED?**  
Add it to `LEDMapping.h` with the appropriate device type (GPIO, WS2812, PCA9555, etc.) and the DCS-BIOS label it should respond to. CockpitOS handles the rest automatically.

**My LED is inverted (on when it should be off)?**  
Set `activeLow = true` in the LEDMapping entry.

**How do I control LED brightness?**  
For dimmable LEDs (WS2812, PWM GPIO), set `dimmable = true`. DCS-BIOS will send intensity values that CockpitOS applies automatically.

**How do segment displays (IFEI, etc.) work?**  
HT1622 displays use segment maps that define which RAM address/bit controls each segment. CockpitOS receives string data from DCS-BIOS and renders it using font tables.

---

## Displays

**How do I create a custom segment display?**  
1. Wire your HT1622 display to CS/WR/DATA pins
2. Run the RAM Walker tool to discover segment mappings
3. Create a SegmentMap header file with the mappings
4. Register display fields in DisplayMapping.cpp

See `docs/ADVANCED_DISPLAYS.md` for the complete process.

**What is the "RAM Walker"?**  
A tool that lights each segment individually so you can record which HT1622 memory address controls which physical segment. This mapping is required to create new display implementations.

**Why are my display digits scrambled?**  
The segment order in your SegmentMap doesn't match the font table order. You must reorder segments to match: [0]=TOP, [1]=TOP-RIGHT, [2]=BOT-RIGHT, [3]=BOTTOM, [4]=BOT-LEFT, [5]=TOP-LEFT, [6]=MIDDLE.

**Can I use TFT displays for gauges?**  
Yes. CockpitOS supports SPI TFT displays via LovyanGFX. The CMWS display is the reference implementation. TFT code is allowed to use dynamic memory (sprites) as an exception to the normal no-heap rule.

---

## Performance

**What refresh rate can I expect?**  
- Input polling: ~250 Hz typical
- Display refresh: 30-60 Hz typical
- DCS-BIOS stream: 30 Hz from the simulator

**Is there input lag?**  
Minimal. USB HID adds ~1-4ms latency. The firmware processes inputs within a single loop iteration (~4ms worst case).

**Why does CockpitOS avoid dynamic memory?**  
Heap allocation can cause fragmentation, unpredictable timing, and memory exhaustion over time. Static allocation ensures deterministic behavior and prevents crashes during long sessions.

---

## Troubleshooting

**Compilation fails with "undefined reference"**  
Usually means a label set file is missing or not generated. Run `python generate_data.py` in your label set folder.

**How do I test without DCS running?**  
Set `IS_REPLAY = 1` in `Config.h` to enable stream replay mode. This plays back a captured DCS-BIOS stream from a header file, letting you test panel behavior without connecting to DCS. Use the tools in `Debug Tools/` to generate replay data from captured streams.

**No response from DCS**  
1. Verify DCS-BIOS is installed and running
2. Check transport configuration matches your setup
3. Ensure the aircraft JSON matches your current aircraft
4. Check serial monitor for error messages

**LEDs work but inputs don't**  
1. Check wiring (GPIO to GND for buttons)
2. Verify InputMapping entries have correct GPIO pins
3. Check if the input type matches your hardware
4. Enable DEBUG in Config.h to see input events

**Display shows garbage**  
1. Segment map order doesn't match font table — most common cause
2. Wrong chip ID (ledID) in segment map
3. Wiring issue (check CS/WR/DATA connections)

**ESP32 crashes or resets**  
1. Power supply issue — ensure adequate current
2. Stack overflow — reduce local variable sizes
3. Watchdog timeout — check for blocking code

---

## Development

**Can I add custom logic to my panel?**  
Yes. Use the subscription system to receive callbacks when DCS-BIOS data changes:
- `subscribeToDisplayChange()` — String data
- `subscribeToSelectorChange()` — Switch positions
- `subscribeToLedChange()` — Indicator states
- `subscribeToMetadataChange()` — Metadata values

**Where do I put custom panel code?**  
In `src/Panels/`. Copy an existing panel as a template. Register it with the `REGISTER_PANEL` macro.

**How do I send commands to DCS?**  
Use `sendDCSBIOSCommand(label, value, force)`. CockpitOS handles throttling and transport automatically.

**How do I profile performance?**  
`PerfMonitor.h` in `src/Core/` is the **only Core file you're expected to edit**. Add custom profiling labels to measure your panel's timing:
```cpp
// In PerfMonitor.h, add your label
enum PerfLabel { ..., PERF_MY_PANEL, ... };

// In your panel code
beginProfiling(PERF_MY_PANEL);
// ... your code ...
endProfiling(PERF_MY_PANEL);
```
The performance snapshot (enabled via `DEBUG_PERFORMANCE = 1` in Config.h) will include your custom measurements.

**Can I contribute to CockpitOS?**  
Yes! The project is open source under MIT license. Submit issues and pull requests on GitHub.

---

## Resources

**Documentation**
- `docs/GETTING_STARTED.md` — First-time setup
- `docs/CREATING_LABEL_SETS.md` — Configuration guide  
- `docs/HARDWARE_WIRING.md` — Wiring reference
- `docs/TRANSPORT_MODES.md` — USB/WiFi/Serial setup
- `docs/ADVANCED_DISPLAYS.md` — Display programming

**External**
- [DCS-BIOS Skunkworks](https://github.com/DCS-Skunkworks/dcs-bios) — The protocol CockpitOS uses
- [Open Hornet Project](https://github.com/jrsteensen/OpenHornet) — Community cockpit builds
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32) — Platform documentation

---

*Can't find your answer? Check the detailed documentation or open an issue on GitHub.*
