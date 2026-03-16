# CockpitOS — Pending Items

---

## VERY HIGH PRIORITY

### New Hardware Support

These three implementations will complete CockpitOS hardware coverage, making it the most comprehensive open-source cockpit framework available — supporting every display type, input method, and output device a cockpit builder would realistically use.

#### HT16K33 — 16-Segment Alphanumeric LED Driver (I2C)
- Community standard for F/A-18 UFC builds (COMM + Option displays)
- I2C bus (SDA, SCL), up to 8 chips per bus (0x70-0x77)
- 16 segments x 8 digits per chip, built-in brightness control + key scanning
- Architecturally closest to existing HT1622 — same RAM-mapped segment approach
- Plugs into existing DisplayMapping pipeline with SegmentMap
- Needs: I2C driver class, seg16_ascii[] lookup table, renderHT16K33Dispatcher
- Reference: OpenHornet UFC, BlueFinBima DCS-FA18C-UFC, TekCreations display kits

#### MAX7219 — 7-Segment / 8x8 Dot Matrix LED Driver (SPI)
- Cheapest off-the-shelf numeric display option, daisy-chainable
- SPI (CLK, DIN, CS), unlimited chain length on one CS pin
- Three modes: 7-segment digits, 8x8 dot matrix, daisy-chained matrix arrays
- Standard segment layout — no hand-built segment maps needed for 7-seg mode
- Plugs into existing DisplayMapping pipeline with SegmentMap
- Needs: SPI driver class, shadow RAM + dirty commit, renderMAX7219Dispatcher
- Use cases: radio frequencies, fuel totalizer, TACAN, clock digits, altimeter digits

#### OLED SSD1306 — Small Monochrome Graphics Display (I2C)
- Tiny 128x64 pixel displays, perfect for text-heavy instruments
- I2C bus (SDA, SCL), shares bus with HT16K33
- LovyanGFX already supports SSD1306 natively — same API as TFT displays
- Does NOT use DisplayMapping pipeline — direct DCS-BIOS callbacks + pixel rendering
- Needs: LovyanGFX panel config, display panel implementation, label set integration
- Killer app: F-16 DED (Data Entry Display), F-16 PFL, Apache EUFD
- Also replaces TNLCD scratchpad on F/A-18 UFC builds

### Stepper Motors (COMPLETED)
- 28BYJ-48 + ULN2003 (geared, continuous 360, slow — throttles, altimeters)
- X27.168 / VID29 (direct drive, fast, ~315 or modded 360 — gauge needles)
- Non-blocking tick, shortest-path wraparound, configurable usPerStep
- Init sweep (full revolution forward + back to zero) for needle calibration
- Integrated in LEDControl, GPIO, PerfMonitor, Label Creator, generator

---

## HIGH PRIORITY

### YouTube Video Tutorials
- Short-form videos (YouTube Shorts)
- Full walkthrough: zero to hero in 30 minutes
- Complex panel build with custom front panel (long-form video)

---

## Current Hardware Support Summary

| Category | Supported |
|----------|-----------|
| **Inputs** | GPIO, PCA9555 (I2C), HC165 (shift register), Matrix keypad, TM1637 buttons, Rotary encoders, Analog axes |
| **LED Outputs** | GPIO, PCA9555 (I2C), WS2812 (addressable RGB), TM1637, GN1640T |
| **Segment Displays** | HT1622 (raw RAM — any segment layout including 7/14/16-seg) |
| **Digit Displays** | TM1637 (4-6 digit 7-seg), GN1640T |
| **Graphic Displays** | TFT via LovyanGFX (SPI + RGB parallel, multiple controllers) |
| **Analog Outputs** | Servo (PWM), Stepper motor (28BYJ-48, X27.168) |
| **Transport** | WiFi UDP, USB (TinyUSB), Serial (CDC), Bluetooth, RS485 master/slave |
| **Planned** | HT16K33 (16-seg I2C), MAX7219 (7-seg/matrix SPI), OLED SSD1306 (I2C) |


*Last updated 2026-03-15.*
