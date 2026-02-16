// WS2812Mini.h — cleaned header (prototypes only) + optional legacy exports
#pragma once
#include <Arduino.h>

// ===== Multi-strip support (non-breaking extensions) =====
// WS2812.h (before using WS2812_MAX_STRIPS)
#ifndef WS2812_MAX_STRIPS
  #if __has_include("soc/rmt_caps.h")
    #include "soc/rmt_caps.h"
    #if defined(SOC_RMT_TX_CHANNELS_PER_GROUP)
      #define WS2812_MAX_STRIPS  (SOC_RMT_TX_CHANNELS_PER_GROUP)
    #else
      #define WS2812_MAX_STRIPS  4
    #endif
  #else
    #if defined(CONFIG_IDF_TARGET_ESP32)
      #define WS2812_MAX_STRIPS  8
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)
      #define WS2812_MAX_STRIPS  4
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
      #define WS2812_MAX_STRIPS  4
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
      #define WS2812_MAX_STRIPS  2
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
      #define WS2812_MAX_STRIPS  4
    #elif defined(CONFIG_IDF_TARGET_ESP32H2)
      #define WS2812_MAX_STRIPS  2
    #elif defined(CONFIG_IDF_TARGET_ESP32C2)
      #define WS2812_MAX_STRIPS  0
    #else
      #define WS2812_MAX_STRIPS  4
    #endif
  #endif
#endif

// ===== Compile-time limits =====
#ifndef WS2812_MAX_LEDS
#define WS2812_MAX_LEDS 8
#endif
static_assert(WS2812_MAX_LEDS > 0, "WS2812_MAX_LEDS must be > 0");

namespace WS2812Mini {

struct CRGB {
  uint8_t r, g, b;
  constexpr CRGB(uint8_t R, uint8_t G, uint8_t B) : r(R), g(G), b(B) {}
};
inline constexpr CRGB Black{0,0,0};
inline constexpr CRGB Green{0,255,0};
inline constexpr CRGB Red  {255,0,0};
inline constexpr CRGB Yellow{255,255,0};
inline constexpr CRGB Blue {0,0,255};

// ===== Core driver =====
class Strip {
public:
  Strip();

  // Hardware init; returns false on HW init failure
  bool     init(int gpio, uint16_t count) noexcept;

  // Pixel ops
  void     setLED(uint16_t i, uint8_t r, uint8_t g, uint8_t b) noexcept;
  void     clear() noexcept;
  void     show() noexcept;
  void     setBrightness(uint8_t b) noexcept;
  uint16_t size() const noexcept;

private:
  int       pin;
  uint16_t  n;
  uint8_t   bufGRB[3u * WS2812_MAX_LEDS];
  uint8_t   brightness;
  uint32_t  lastShowUs;
  bool      dirty;       // skip show() when nothing changed

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  void*     tx_chan;  // rmt_channel_handle_t
  void*     enc;      // rmt_encoder_handle_t
  struct RmtSym { uint16_t duration0:15, level0:1, duration1:15, level1:1; };
  RmtSym    syms[WS2812_MAX_LEDS * 24u];
#else
  int       rmt_chan; // rmt_channel_t
  typedef struct {
    uint16_t duration0;
    uint16_t level0:1;  uint16_t duration1:15;
    uint16_t level1:1;  uint16_t duration2:15;
    uint16_t level2:1;  uint16_t duration3:15;
  } rmt_item32_t_local;
  rmt_item32_t_local items[WS2812_MAX_LEDS * 24u];
#endif

  void sendFrame() noexcept;

// WS2812.h  — inside class Strip (private:)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
// IDF5 path uses 20 MHz (50 ns) tick now
	static constexpr uint8_t  T1H = 16;  // 0.8 us
	static constexpr uint8_t  T1L = 9;   // 0.45 us
	static constexpr uint8_t  T0H = 8;   // 0.4 us
	static constexpr uint8_t  T0L = 17;  // 0.85 us
#else
// IDF4 path (clk_div=2 -> 50 ns tick)
	static constexpr uint16_t T1H = 16;  // 0.8 us
	static constexpr uint16_t T1L = 9;   // 0.45 us
	static constexpr uint16_t T0H = 8;   // 0.4 us
	static constexpr uint16_t T0L = 17;  // 0.85 us
#endif

};

// ===== Singleton-style surface (new API) =====

// Register or update a strip on 'pin' with at least 'count' LEDs.
void WS2812_registerStrip(uint8_t pin, uint16_t count);

// Pin-aware setters (overloads). Do NOT remove old index-only versions.
void WS2812_setLEDColor(uint8_t pin, uint16_t i, uint8_t r, uint8_t g, uint8_t b);
void WS2812_setLEDColor(uint8_t pin, uint16_t i, const CRGB& c);

// Per-strip brightness
void WS2812_setBrightness(uint8_t pin, uint8_t b);

// Flush all configured strips (legacy WS2812_show() keeps flushing the default strip)
void WS2812_showAll();

// Clear all strips
void WS2812_clearAllStrips();

void     WS2812_init(int gpio, uint16_t count);
void     WS2812_setLEDColor(uint16_t i, uint8_t r, uint8_t g, uint8_t b);
void     WS2812_clearAll();
void     WS2812_show();
void     WS2812_setBrightness(uint8_t b);
uint16_t WS2812_count();

// ===== Temporary shims for legacy calls (prototypes only) =====
// zero-arg init (uses WS2812B_PIN, NUM_LEDS defined by the app)
void WS2812_init();
void initWS2812FromMap();

// CRGB overload for setLEDColor
void WS2812_setLEDColor(uint16_t i, const CRGB& c);

// legacy helpers
void WS2812_setLEDColor_unsafe(uint16_t i, uint8_t r, uint8_t g, uint8_t b);
void WS2812_allOn(const CRGB& color);
void WS2812_allOff();
void WS2812_setAllLEDs(bool state);
void WS2812_sweep(const CRGB* colors, uint8_t count);
void testAOALevels();
void WS2812_testPattern();
void WS2812_tick();

// LED Test
void WS2812_allOnFromMap();   // set every WS2812 LED to its mapped default color/brightness
void WS2812_allOffAll();      // clear all strips and flush

} // namespace WS2812Mini

// ===== Optional: export to global namespace during migration =====
// Define WS2812MINI_EXPORT_LEGACY_GLOBALS before including this header to enable.
#ifdef WS2812MINI_EXPORT_LEGACY_GLOBALS
using WS2812Mini::WS2812_init;                // both overloads are exported
using WS2812Mini::WS2812_setLEDColor;         // both overloads are exported
using WS2812Mini::WS2812_clearAll;
using WS2812Mini::WS2812_show;
using WS2812Mini::WS2812_setBrightness;
using WS2812Mini::WS2812_count;

using WS2812Mini::WS2812_setLEDColor_unsafe;
using WS2812Mini::WS2812_allOn;
using WS2812Mini::WS2812_allOff;
using WS2812Mini::WS2812_setAllLEDs;
using WS2812Mini::WS2812_sweep;
using WS2812Mini::testAOALevels;
using WS2812Mini::WS2812_testPattern;
using WS2812Mini::WS2812_tick;
using WS2812Mini::initWS2812FromMap;

using WS2812Mini::CRGB;
using WS2812Mini::Black;
using WS2812Mini::Green;
using WS2812Mini::Red;
using WS2812Mini::Yellow;
using WS2812Mini::Blue;

#endif
