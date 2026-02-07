/*──────────────────────────────────────────────────────────────────────────────
  Panel_ST77916.hpp — LovyanGFX QSPI Panel Driver for Sitronix ST77916
  CockpitOS TFT Gauge Pipeline

  Target Hardware:
    Waveshare ESP32-S3-LCD-1.85 (360×360, ST77916, QSPI)
    Any ST77916-based round TFT using QSPI interface

  ─── WHY THIS FILE EXISTS ───────────────────────────────────────────────────

  LovyanGFX (as of v1.2.19) has two separate panel architectures:

    Panel_LCD    Standard SPI/parallel panels (ST7789, ILI9341, etc.)
                 Commands sent as raw 8-bit bytes with a DC pin to distinguish
                 commands from data.  Works great for 1-wire and 4-wire SPI.

    Panel_AMOLED QSPI panels (RM67162, CO5300, RM690B0, NV3041A)
                 No DC pin.  Every command is wrapped in a 32-bit opcode frame:
                   Command:    [0x02] [0x00] [cmd] [0x00]
                   Pixel data: [0x32] [0x00] [0x2C] [0x00]  (RAMWR opcode)
                 This is the QSPI "opcode framing" protocol.

  The ST77916 uses the EXACT same opcode framing as the AMOLED panels:
    - Command write opcode: 0x02
    - Pixel data opcode:    0x32
    - Standard MIPI DCS commands (CASET 0x2A, RASET 0x2B, RAMWR 0x2C)
    - No DC pin (all signaling through opcodes)

  But LovyanGFX has no Panel_ST77916.  The library DOES have "Panel_ST77961"
  but that name is a TYPO — the actual chip is ST77916 (the digits 16 and 61
  were swapped).  More critically, Panel_ST77961 inherits from Panel_LCD,
  which sends raw commands without opcode framing → blank screen on QSPI.

  Our solution: inherit from Panel_AMOLED, which already implements the
  correct opcode framing.  We override setWindow / drawPixelPreclipped /
  writeFillRectPreclipped to remove the even-pixel alignment constraint
  that is specific to AMOLED panels (the ST77916 LCD has no such
  restriction).  writeImage() is intentionally NOT overridden — the
  base Panel_AMOLED implementation works correctly; its only AMOLED
  quirk (even-pixel alignment) is harmless in practice because our
  gauge pipeline uses setAddrWindow + pushPixelsDMA, not writeImage.

  ─── USAGE ──────────────────────────────────────────────────────────────────

  This is a drop-in replacement for Panel_ST77961.  In your LGFX device class:

    // Before (broken on QSPI):
    lgfx::Panel_ST77961 _panel;

    // After (works on QSPI):
    lgfx::Panel_ST77916 _panel;

  Bus configuration remains the same — set pin_io0–io3 for QSPI, no DC pin:

    cfg.pin_dc   = -1;             // No DC line in QSPI mode
    cfg.pin_mosi = -1;             // Not used in QSPI
    cfg.pin_miso = -1;             // Not used in QSPI
    cfg.pin_io0  = 46;             // QSPI Data 0
    cfg.pin_io1  = 45;             // QSPI Data 1
    cfg.pin_io2  = 42;             // QSPI Data 2
    cfg.pin_io3  = 41;             // QSPI Data 3

  ─── FOR IMPLEMENTERS OF OTHER QSPI DISPLAYS ───────────────────────────────

  If your display controller uses the same 0x02/0x32 opcode convention
  (most Sitronix QSPI controllers do: ST77916, ST77922, ST77903, etc.),
  you can follow this same pattern:

    1. Inherit from Panel_AMOLED (not Panel_LCD)
    2. Set your resolution in the constructor
    3. Override getInitCommands() with your vendor init sequence
    4. Override setWindow() to remove the even-pixel constraint if your
       display doesn't need it
    5. Override drawPixelPreclipped / writeFillRectPreclipped to remove
       the same constraint (writeImage override is usually unnecessary)

  The opcode framing, QSPI bus management, and pixel transfer are all
  handled by the Panel_AMOLED base class.

  ─── INIT SEQUENCE SOURCE ──────────────────────────────────────────────────

  The vendor-specific register initialization below is sourced from
  Espressif's esp-iot-solution (v2.0.3, Apache 2.0 license) and matches
  the Waveshare ESP32-S3-LCD-1.85 reference design.

  ─── LICENSE ────────────────────────────────────────────────────────────────

  This file: same license as CockpitOS.
  Vendor init sequence: Apache License 2.0 (Espressif Systems).
──────────────────────────────────────────────────────────────────────────────*/
#pragma once

#if defined(ESP_PLATFORM)

// ── PREREQUISITES ──────────────────────────────────────────────────────────
//
// This header requires <LovyanGFX.hpp> to be included BEFORE this file.
// LovyanGFX.hpp pulls in:
//   - platforms/common.hpp  → defines LGFX_USE_QSPI (ESP32-S2/S3, IDF ≥ 4.4)
//   - Panel_AMOLED.hpp      → the QSPI base class we inherit from
//
// Do NOT include this header standalone — it has no self-contained includes
// because Panel_AMOLED.hpp uses #pragma once and is already fully processed
// by the time we get here.
//
#if !defined(LOVYANGFX_HPP_)
#error "Panel_ST77916.hpp must be included AFTER <LovyanGFX.hpp>"
#endif

#if !defined(LGFX_USE_QSPI)
#error "Panel_ST77916.hpp requires QSPI support (LGFX_USE_QSPI). Build for ESP32-S2/S3 with IDF >= 4.4."
#endif

namespace lgfx
{
  inline namespace v1
  {
    // ────────────────────────────────────────────────────────────────────────
    //  Panel_ST77916 — QSPI round TFT (360×360)
    //
    //  Inherits from Panel_AMOLED for opcode-framed QSPI transport.
    //  Overrides AMOLED-specific even-pixel constraints (the ST77916 LCD
    //  accepts any x/w alignment) and provides the ST77916 vendor init.
    // ────────────────────────────────────────────────────────────────────────

    struct Panel_ST77916 : public Panel_AMOLED
    {
    public:
      Panel_ST77916(void)
      {
        _cfg.memory_width  = _cfg.panel_width  = 360;
        _cfg.memory_height = _cfg.panel_height = 360;
        _write_depth = color_depth_t::rgb565_2Byte;
        _read_depth  = color_depth_t::rgb565_2Byte;
        _cfg.dummy_read_pixel = 8;
        _cfg.rgb_order = true;   // ST77916 Waveshare default (BGR subpixels)

        // Inversion: Panel_AMOLED::setInvert() ignores _cfg.invert (unlike
        // Panel_LCD which XORs).  Instead, LGFXBase::init_impl() calls
        // setInvert(getInvert()) which reads the _invert member directly.
        // So we set _invert = true here.  The actual INVON command is sent
        // by init_impl AFTER getInitCommands() runs — NOT in the init sequence.
        _invert = true;
      }

      // ──────────────────────────────────────────────────────────────────────
      //  setWindow — Remove AMOLED even-pixel constraint
      //
      //  Panel_AMOLED silently discards setWindow calls where x or width
      //  are odd (an AMOLED display restriction).  The ST77916 LCD has no
      //  such constraint, so we override with a clean implementation that
      //  accepts any coordinates.
      // ──────────────────────────────────────────────────────────────────────
      void setWindow(uint_fast16_t xs, uint_fast16_t ys, uint_fast16_t xe, uint_fast16_t ye) override
      {
        // _width/_height are the post-rotation logical dimensions
        // (always 360×360 for this round display, regardless of rotation).
        if (xs > xe || xe >= _width)  return;
        if (ys > ye || ye >= _height) return;

        xs += _colstart;
        xe += _colstart;

        // CASET — Column Address Set
        write_cmd(0x2A);
        _bus->writeCommand(__builtin_bswap32(xs << 16 | xe), 32);

        ys += _rowstart;
        ye += _rowstart;

        // RASET — Row Address Set
        write_cmd(0x2B);
        _bus->writeCommand(__builtin_bswap32(ys << 16 | ye), 32);
      }

      // ──────────────────────────────────────────────────────────────────────
      //  drawPixelPreclipped — Remove AMOLED even-x constraint
      // ──────────────────────────────────────────────────────────────────────
      void drawPixelPreclipped(uint_fast16_t x, uint_fast16_t y, uint32_t rawcolor) override
      {
        setWindow(x, y, x, y);
        start_qspi();
        _bus->writeData(rawcolor, _write_bits);
        _bus->wait();
        end_qspi();
      }

      // ──────────────────────────────────────────────────────────────────────
      //  writeFillRectPreclipped — Remove AMOLED even-x/w constraint
      // ──────────────────────────────────────────────────────────────────────
      void writeFillRectPreclipped(uint_fast16_t x, uint_fast16_t y, uint_fast16_t w, uint_fast16_t h, uint32_t rawcolor) override
      {
        uint32_t len = w * h;
        setWindow(x, y, x + w - 1, y + h - 1);
        start_qspi();
        _bus->writeDataRepeat(rawcolor, _write_bits, len);
      }

      // ──────────────────────────────────────────────────────────────────────
      //  writeImage — intentionally NOT overridden.
      //
      //  Panel_AMOLED::writeImage() has an even-pixel alignment check,
      //  but our gauge pipeline never calls writeImage (it uses
      //  setAddrWindow + pushPixelsDMA).  The base implementation is
      //  correct for any future callers where alignment holds, and the
      //  cost of carrying an 80-line near-duplicate is not worth it.
      // ──────────────────────────────────────────────────────────────────────

      // ──────────────────────────────────────────────────────────────────────
      //  setColorDepth — ST77916 supports RGB565 and RGB666
      //
      //  Sends COLMOD (0x3A) to the display so the pixel format register
      //  stays in sync with what LovyanGFX is pushing.
      //    0x55 = RGB565 (16-bit)
      //    0x66 = RGB666 (18-bit, packed in 24-bit transfers)
      // ──────────────────────────────────────────────────────────────────────
      color_depth_t setColorDepth(color_depth_t depth) override
      {
        _write_depth = ((int)depth & color_depth_t::bit_mask) > 16 ? rgb888_3Byte : rgb565_2Byte;
        _read_depth = _write_depth;

        // Send COLMOD command to hardware if bus is available (post-init)
        if (_bus)
        {
          uint8_t colmod = (_write_depth == rgb888_3Byte) ? 0x66 : 0x55;
          startWrite();
          write_cmd(0x3A);  // COLMOD — Interface Pixel Format
          _bus->writeData(colmod, 8);
          endWrite();
        }

        return _write_depth;
      }

      // ──────────────────────────────────────────────────────────────────────
      //  setBrightness — Use hardware PWM backlight (not AMOLED register)
      //
      //  Panel_AMOLED::setBrightness() sends MIPI DCS command 0x51 (WRDISBV)
      //  to control AMOLED panel brightness via an internal register.
      //  The ST77916 is an LCD with a hardware backlight LED controlled by
      //  PWM on a GPIO pin (via Light_PWM).  We override to call the
      //  Panel_Device base implementation which delegates to _light.
      // ──────────────────────────────────────────────────────────────────────
      void setBrightness(uint8_t brightness) override
      {
        if (_light) { _light->setBrightness(brightness); }
      }

    protected:

      // ──────────────────────────────────────────────────────────────────────
      //  Vendor Init Sequence — ST77916 (Waveshare ESP32-S3-LCD-1.85)
      //
      //  Format (LovyanGFX byte-packed command list):
      //    cmd, num_args [| CMD_INIT_DELAY], arg0, arg1, ..., [delay_ms]
      //  Terminated by: 0xFF, 0xFF
      //
      //  Source: Espressif esp-iot-solution v2.0.3 (Apache 2.0)
      //  File:   esp_lcd_st77916_spi.c → vendor_specific_init_default[]
      //
      //  The sequence configures power, gamma, GIP timing, and OTP.
      //  If your specific ST77916 board requires a different init sequence,
      //  override getInitCommands() in your own subclass.
      // ──────────────────────────────────────────────────────────────────────
      const uint8_t* getInitCommands(uint8_t listno) const override
      {
        static constexpr uint8_t list0[] = {
          // ── Page / Command Set Control ──
          0xF0, 1, 0x08,             // CMD_SET: Test command page enable
          0xF2, 1, 0x08,             // CSC3: Test command page enable
          0x9B, 1, 0x51,             // (vendor)
          0x86, 1, 0x53,             // (vendor)
          0xF2, 1, 0x80,             // CSC3
          0xF0, 1, 0x00,             // CMD_SET: Disable
          0xF0, 1, 0x01,             // CMD_SET: Command2 enable
          0xF1, 1, 0x01,             // CSC2: Command2 enable

          // ── Voltage / Power Settings ──
          0xB0, 1, 0x54,             // VRHPS
          0xB1, 1, 0x3F,             // VRHNS
          0xB2, 1, 0x2A,             // VCOMS
          0xB4, 1, 0x46,             // GAMOPPS
          0xB5, 1, 0x34,             // STEP14S
          0xB6, 1, 0xD5,             // STEP23S
          0xB7, 1, 0x30,             // (vendor)
          0xB8, 1, 0x04,             // (vendor)
          0xBA, 1, 0x00,             // TCONS
          0xBB, 1, 0x08,             // RGB_VBP
          0xBC, 1, 0x08,             // RGB_HBP
          0xBD, 1, 0x00,             // RGB_SET

          // ── Frame Rate Control ──
          0xC0, 1, 0x80,             // FRCTRA1 (Normal)
          0xC1, 1, 0x10,             // FRCTRA2
          0xC2, 1, 0x37,             // FRCTRA3 (53.86 Hz)
          0xC3, 1, 0x80,             // FRCTRB1 (Idle)
          0xC4, 1, 0x10,             // FRCTRB2
          0xC5, 1, 0x37,             // FRCTRB3
          0xC6, 1, 0xA9,             // PWRCTRA1
          0xC7, 1, 0x41,             // PWRCTRA2
          0xC8, 1, 0x51,             // PWRCTRA3
          0xC9, 1, 0xA9,             // PWRCTRB1
          0xCA, 1, 0x41,             // PWRCTRB2
          0xCB, 1, 0x51,             // PWRCTRB3

          // ── Resolution ──
          0xD0, 1, 0x91,             // RESSET1
          0xD1, 1, 0x68,             // RESSET2
          0xD2, 1, 0x69,             // RESSET3
          0xF5, 2, 0x00, 0xA5,
          0xDD, 1, 0x35,             // VCMOFSET
          0xDE, 1, 0x35,             // VCMOFNSET

          // ── Exit Command2, enter Gamma ──
          0xF1, 1, 0x10,
          0xF0, 1, 0x00,
          0xF0, 1, 0x02,             // Gamma correction page

          // ── Gamma Curves ──
          0xE0, 14, 0x70, 0x09, 0x12, 0x0C, 0x0B, 0x27, 0x38, 0x54, 0x4E, 0x19, 0x15, 0x15, 0x2C, 0x2F,
          0xE1, 14, 0x70, 0x08, 0x11, 0x0C, 0x0B, 0x27, 0x38, 0x43, 0x4C, 0x18, 0x14, 0x14, 0x2B, 0x2D,

          // ── GIP (Gate-In-Panel) Timing ──
          0xF0, 1, 0x10,             // GIP page enable
          0xF3, 1, 0x10,
          0xE0, 1, 0x0A,
          0xE1, 1, 0x00,
          0xE2, 1, 0x0B,
          0xE3, 1, 0x00,
          0xE4, 1, 0xE0,
          0xE5, 1, 0x06,
          0xE6, 1, 0x21,
          0xE7, 1, 0x00,
          0xE8, 1, 0x05,
          0xE9, 1, 0x82,
          0xEA, 1, 0xDF,
          0xEB, 1, 0x89,
          0xEC, 1, 0x20,
          0xED, 1, 0x14,
          0xEE, 1, 0xFF,
          0xEF, 1, 0x00,
          0xF8, 1, 0xFF,
          0xF9, 1, 0x00,
          0xFA, 1, 0x00,
          0xFB, 1, 0x30,
          0xFC, 1, 0x00,
          0xFD, 1, 0x00,
          0xFE, 1, 0x00,
          0xFF, 1, 0x00,

          // ── GIP Source/Gate Waveform ──
          0x60, 1, 0x42,
          0x61, 1, 0xE0,
          0x62, 1, 0x40,
          0x63, 1, 0x40,
          0x64, 1, 0x02,
          0x65, 1, 0x00,
          0x66, 1, 0x40,
          0x67, 1, 0x03,
          0x68, 1, 0x00,
          0x69, 1, 0x00,
          0x6A, 1, 0x00,
          0x6B, 1, 0x00,

          0x70, 1, 0x42,
          0x71, 1, 0xE0,
          0x72, 1, 0x40,
          0x73, 1, 0x40,
          0x74, 1, 0x02,
          0x75, 1, 0x00,
          0x76, 1, 0x40,
          0x77, 1, 0x03,
          0x78, 1, 0x00,
          0x79, 1, 0x00,
          0x7A, 1, 0x00,
          0x7B, 1, 0x00,

          // ── GIP Power Sequence ──
          0x80, 1, 0x38,
          0x81, 1, 0x00,
          0x82, 1, 0x04,
          0x83, 1, 0x02,
          0x84, 1, 0xDC,
          0x85, 1, 0x00,
          0x86, 1, 0x00,
          0x87, 1, 0x00,

          0x88, 1, 0x38,
          0x89, 1, 0x00,
          0x8A, 1, 0x06,
          0x8B, 1, 0x02,
          0x8C, 1, 0xDE,
          0x8D, 1, 0x00,
          0x8E, 1, 0x00,
          0x8F, 1, 0x00,

          0x90, 1, 0x38,
          0x91, 1, 0x00,
          0x92, 1, 0x08,
          0x93, 1, 0x02,
          0x94, 1, 0xE0,
          0x95, 1, 0x00,
          0x96, 1, 0x00,
          0x97, 1, 0x00,

          0x98, 1, 0x38,
          0x99, 1, 0x00,
          0x9A, 1, 0x0A,
          0x9B, 1, 0x02,
          0x9C, 1, 0xE2,
          0x9D, 1, 0x00,
          0x9E, 1, 0x00,
          0x9F, 1, 0x00,

          0xA0, 1, 0x38,
          0xA1, 1, 0x00,
          0xA2, 1, 0x03,
          0xA3, 1, 0x02,
          0xA4, 1, 0xDB,
          0xA5, 1, 0x00,
          0xA6, 1, 0x00,
          0xA7, 1, 0x00,

          0xA8, 1, 0x38,
          0xA9, 1, 0x00,
          0xAA, 1, 0x05,
          0xAB, 1, 0x02,
          0xAC, 1, 0xDD,
          0xAD, 1, 0x00,
          0xAE, 1, 0x00,
          0xAF, 1, 0x00,

          0xB0, 1, 0x38,
          0xB1, 1, 0x00,
          0xB2, 1, 0x07,
          0xB3, 1, 0x02,
          0xB4, 1, 0xDF,
          0xB5, 1, 0x00,
          0xB6, 1, 0x00,
          0xB7, 1, 0x00,

          0xB8, 1, 0x38,
          0xB9, 1, 0x00,
          0xBA, 1, 0x09,
          0xBB, 1, 0x02,
          0xBC, 1, 0xE1,
          0xBD, 1, 0x00,
          0xBE, 1, 0x00,
          0xBF, 1, 0x00,

          // ── GIP Mux / Timing ──
          0xC0, 1, 0x22,
          0xC1, 1, 0xAA,
          0xC2, 1, 0x65,
          0xC3, 1, 0x74,
          0xC4, 1, 0x47,
          0xC5, 1, 0x56,
          0xC6, 1, 0x00,
          0xC7, 1, 0x88,
          0xC8, 1, 0x99,
          0xC9, 1, 0x33,

          0xD0, 1, 0x11,
          0xD1, 1, 0xAA,
          0xD2, 1, 0x65,
          0xD3, 1, 0x74,
          0xD4, 1, 0x47,
          0xD5, 1, 0x56,
          0xD6, 1, 0x00,
          0xD7, 1, 0x88,
          0xD8, 1, 0x99,
          0xD9, 1, 0x33,

          // ── Exit GIP, OTP Programming ──
          0xF3, 1, 0x01,
          0xF0, 1, 0x00,
          0xF0, 1, 0x01,             // Command2 enable
          0xF1, 1, 0x01,             // Command2 enable

          0xA0, 1, 0x0B,             // OTP_MODE_SEL
          0xA3, 1, 0x2A,             // OTP page addr
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x2B,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x2C,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x2D,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x2E,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x2F,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x30,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x31,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x32,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA3, 1, 0x33,
          0xA5, 1 + CMD_INIT_DELAY, 0xC3, 1,
          0xA0, 1, 0x09,             // OTP_MODE_SEL exit

          // ── Exit Command2 ──
          0xF1, 1, 0x10,
          0xF0, 1, 0x00,

          // ── Display Window & Clear ──
          0x2A, 4, 0x00, 0x00, 0x01, 0x67,  // CASET 0–359
          // RASET set to row 360 only (0x0168–0x0168) — intentional.
          // The RAMCLACT clear command below fills the entire framebuffer
          // regardless of RASET; this narrow window avoids visual artifacts
          // during the clear.  The full 0–359 window is restored after clear.
          0x2B, 4, 0x01, 0x68, 0x01, 0x68,  // RASET (pre-clear, single row)
          0x4D, 1, 0x00,                     // RAMCLSETR (clear R)
          0x4E, 1, 0x00,                     // RAMCLSETG (clear G)
          0x4F, 1, 0x00,                     // RAMCLSETB (clear B)
          0x4C, 1 + CMD_INIT_DELAY, 0x01, 10, // RAMCLACT (trigger fill)
          0x4C, 1, 0x00,

          0x2A, 4, 0x00, 0x00, 0x01, 0x67,  // CASET 0–359
          0x2B, 4, 0x00, 0x00, 0x01, 0x67,  // RASET 0–359

          // ── Interface Pixel Format ──
          0x3A, 1, 0x55,                     // COLMOD: RGB565 (default; setColorDepth() updates if changed)

          // ── Final Display On ──
          // NOTE: No INVON here — inversion is handled by LGFXBase::init_impl()
          // which calls setInvert(getInvert()) after this sequence completes.
          // We set _invert = true in the constructor so INVON is sent once, correctly.
          // Putting INVON here would cause a double-invert (our INVON + init_impl's INVOFF).
          0x11, 0 + CMD_INIT_DELAY, 120,     // SLPOUT + 120ms
          0x29, 0,                           // DISPON

          0xFF, 0xFF  // ── End of list ──
        };

        switch (listno) {
          case 0:  return list0;
          default: return nullptr;
        }
      }
    };

    // ────────────────────────────────────────────────────────────────────────
  }
}

#endif // ESP_PLATFORM
