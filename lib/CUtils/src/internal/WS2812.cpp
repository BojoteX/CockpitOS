// WS2812.cpp — refactored for CockpitOS strict rules (no heap, deterministic)
// Keeps API and call sites stable. Uses fixed static buffers and compile-time limits.
// WCET notes (approx, 6 LEDs):
//  - Encoding: 6 * 24 symbols ≈ 144 iterations; trivial ops per loop.
//  - RMT TX: ~30 us per LED -> ~180 us payload + ≥60 us latch ~ <300 us total per show().

#include <Arduino.h>
#include <string.h>

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
#include <driver/rmt_tx.h>
#include <driver/rmt_types.h>
#else
#include <driver/rmt.h>
#endif

namespace WS2812Mini
{

    // ===== Strip: init =====
    bool Strip::init(int gpio, uint16_t count) noexcept {
        // Bound count to static capacity.
        n = (count <= WS2812_MAX_LEDS) ? count : WS2812_MAX_LEDS;
        pin = gpio;
        brightness = 255;
        lastShowUs = 0;

        // Clear buffer deterministically.
        memset(bufGRB, 0, sizeof(bufGRB));

        // GPIO prep
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
        // IDF v5 RMT TX setup. Note: IDF allocates internally. Our code uses no heap.
        rmt_tx_channel_config_t cfg = {};
        cfg.gpio_num = (gpio_num_t)pin;
        cfg.clk_src = RMT_CLK_SRC_DEFAULT;
        // cfg.resolution_hz = 10'000'000;     // 100 ns tick (removed)
        cfg.resolution_hz = 20'000'000;        // 50 ns tick, matches IDF4 math
        cfg.mem_block_symbols = 64;            // 64 symbols buffer per block
        cfg.trans_queue_depth = 2;
        if (rmt_new_tx_channel(&cfg, (rmt_channel_handle_t*)&tx_chan) != ESP_OK) return false;

        rmt_copy_encoder_config_t ecfg = {};
        if (rmt_new_copy_encoder(&ecfg, (rmt_encoder_handle_t*)&enc) != ESP_OK)   return false;

        if (rmt_enable((rmt_channel_handle_t)tx_chan) != ESP_OK) return false;
#else
        // IDF v4 RMT setup (static)
        // rmt_chan = (int)RMT_CHANNEL_0;

        static int next_ch = (int)RMT_CHANNEL_0;
        rmt_chan = next_ch;
        
		// In line with max strips, cycle channels.
        next_ch = (next_ch + 1) % WS2812_MAX_STRIPS;  // WS2812_MAX_STRIPS TX channels MAX

        rmt_config_t c = {};
        c.rmt_mode = RMT_MODE_TX;
        c.channel = (rmt_channel_t)rmt_chan;
        c.gpio_num = (gpio_num_t)pin;
        c.mem_block_num = 1;
        c.tx_config.loop_en = false;
        c.tx_config.carrier_en = false;
        c.tx_config.idle_output_en = true;
        c.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
        c.clk_div = 2; // 40 MHz APB / 2 = 20 MHz -> 50 ns tick
        if (rmt_config(&c) != ESP_OK) return false;
        if (rmt_driver_install((rmt_channel_t)rmt_chan, 0, 0) != ESP_OK) return false;
#endif

        return true;
    }

    // Missing non-inline defs
    Strip::Strip()
        : pin(-1),
        n(0),
        brightness(255),
        lastShowUs(0)
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
        , tx_chan(nullptr)
        , enc(nullptr)
#else
        , rmt_chan(0)
#endif
    {}

    void Strip::setBrightness(uint8_t b) noexcept {
        brightness = b;
    }

    uint16_t Strip::size() const noexcept {
        return n;
    }

    // ===== Strip: set LED =====
    void Strip::setLED(uint16_t i, uint8_t r, uint8_t g, uint8_t b) noexcept {
        if (i >= n) return;
        // WS2812 expects GRB order.
        bufGRB[3u * i + 0] = g;
        bufGRB[3u * i + 1] = r;
        bufGRB[3u * i + 2] = b;
    }

    // ===== Strip: clear =====
    void Strip::clear() noexcept {
        // Only clear the active portion to bounded time.
        const size_t bytes = (size_t)3u * (size_t)n;
        memset(bufGRB, 0, bytes);
    }

    // ===== Strip: show =====
    void Strip::show() noexcept {
        const uint32_t now = micros();
        const uint32_t dt = now - lastShowUs;
        if (dt < 300u) delayMicroseconds(300u - dt); // was 60us
        sendFrame();
        lastShowUs = micros();
    }

    // ===== Strip: sendFrame (encode + TX) =====
    void Strip::sendFrame() noexcept {
        // Encode GRB with brightness scaling into static symbol buffer.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
        size_t k = 0;
        for (uint16_t i = 0; i < n; ++i) {
            const uint8_t g = (uint8_t)((bufGRB[3u * i + 0] * (uint16_t)brightness) >> 8);
            const uint8_t r = (uint8_t)((bufGRB[3u * i + 1] * (uint16_t)brightness) >> 8);
            const uint8_t b = (uint8_t)((bufGRB[3u * i + 2] * (uint16_t)brightness) >> 8);
            const uint32_t pix = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

            for (int bit = 23; bit >= 0; --bit) {
                const bool one = ((pix >> bit) & 1u) != 0u;
                syms[k].level0 = 1;
                syms[k].duration0 = one ? T1H : T0H;
                syms[k].level1 = 0;
                syms[k].duration1 = one ? T1L : T0L;
                ++k;
            }
        }

        const uint16_t ResetTicks = static_cast<uint16_t>(300000 / 50); // 300us / 50ns = 6000
        if (k > 0) {
            uint16_t d = syms[k - 1].duration1;
            uint32_t sum = static_cast<uint32_t>(d) + ResetTicks;
            syms[k - 1].duration1 = (sum > 0x7FFF) ? 0x7FFF : static_cast<uint16_t>(sum);
        }

        rmt_transmit_config_t tcfg = {};
        (void)rmt_transmit((rmt_channel_handle_t)tx_chan,
            (rmt_encoder_handle_t)enc,
            syms,
            k * sizeof(syms[0]),
            &tcfg);
        (void)rmt_tx_wait_all_done((rmt_channel_handle_t)tx_chan, -1);

#else
        size_t k = 0;
        for (uint16_t i = 0; i < n; ++i) {
            const uint8_t g = (uint8_t)((bufGRB[3u * i + 0] * (uint16_t)brightness) >> 8);
            const uint8_t r = (uint8_t)((bufGRB[3u * i + 1] * (uint16_t)brightness) >> 8);
            const uint8_t b = (uint8_t)((bufGRB[3u * i + 2] * (uint16_t)brightness) >> 8);
            const uint32_t pix = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;

            for (int bit = 23; bit >= 0; --bit) {
                const bool one = ((pix >> bit) & 1u) != 0u;
                items[k].level0 = 1;
                items[k].duration0 = one ? T1H : T0H;
                items[k].level1 = 0;
                items[k].duration1 = one ? T1L : T0L;
                // Unused second pair kept zero; RMT ignores when writing raw items.
                items[k].level2 = 0;
                items[k].duration2 = 0;
                items[k].level3 = 0;
                items[k].duration3 = 0;
                ++k;
            }
        }

        const uint16_t ResetTicks = static_cast<uint16_t>(300000 / 50); // 6000
        if (k > 0) {
            uint16_t d = items[k - 1].duration1;
            uint32_t sum = static_cast<uint32_t>(d) + ResetTicks;
            items[k - 1].duration1 = (sum > 0x7FFF) ? 0x7FFF : static_cast<uint16_t>(sum);
        }

        (void)rmt_write_items((rmt_channel_t)rmt_chan, (rmt_item32_t*)items, k, true);
        (void)rmt_wait_tx_done((rmt_channel_t)rmt_chan, portMAX_DELAY);
#endif
    }

    // ===== Singleton-style compatibility layer =====
    static Strip   g_inst;
    static bool    g_init = false;
    static uint16_t g_count = 0;

	// For CockpitOS multi-strip support
    static int      g_defaultSlot = -1;
    struct Slot { int pin; bool init; uint16_t count; Strip strip; };
    static Slot     g_slots[WS2812_MAX_STRIPS];

    void WS2812_init(int gpio, uint16_t count) {
        g_init = g_inst.init(gpio, count);
        g_count = g_init ? ((count <= WS2812_MAX_LEDS) ? count : WS2812_MAX_LEDS) : 0;
    }

    void WS2812_init() {
        WS2812_init(WS2812B_PIN, NUM_LEDS);
    }

    void WS2812_setLEDColor(uint16_t i, const CRGB& c) {
        WS2812_setLEDColor(i, c.r, c.g, c.b);
    }

    void WS2812_setLEDColor(uint16_t i, uint8_t r, uint8_t g, uint8_t b) {
        if (!g_init) return;
        g_inst.setLED(i, r, g, b);
    }

    void WS2812_clearAll() {
        if (!g_init) return;
        g_inst.clear();
    }

    void WS2812_show() {
        if (!g_init) return;
        g_inst.show();
    }

    void WS2812_setBrightness(uint8_t b) {
        if (!g_init) return;
        g_inst.setBrightness(b);
    }

    uint16_t WS2812_count() {
        return g_init ? g_count : 0;
    }

	// New additions for CockpitOS compliance

    void WS2812_setLEDColor_unsafe(uint16_t i, uint8_t r, uint8_t g, uint8_t b) {
        // Generic driver has no gating; identical to the safe path.
        WS2812_setLEDColor(i, r, g, b);
    }

    void WS2812_allOn(const CRGB& /*color*/) {
        const uint16_t n = WS2812_count();
        for (uint16_t i = 0; i < n; ++i) {
            uint8_t r = 0, g = 0, b = 0;
            if (i <= 2) {            // Lock/Shoot trio always green
                r = 0; g = 255; b = 0;
            }
            else if (i == 3) {     // AOA HIGH -> green (legacy behavior)
                r = 0; g = 255; b = 0;
            }
            else if (i == 4) {     // AOA NORMAL -> orange-ish
                r = 255; g = 165; b = 0;
            }
            else if (i == 5) {     // AOA LOW -> red
                r = 255; g = 0; b = 0;
            }
            else {
                // anything beyond index 5 stays off
                r = g = b = 0;
            }
            WS2812_setLEDColor(i, r, g, b);
        }
        WS2812_show();
    }

    void WS2812_allOff() {
        WS2812_clearAll();
        WS2812_show();
    }

    void WS2812_setAllLEDs(bool state) {
        if (state) {
            // Mirror WS2812_allOn() semantics without requiring CRGB construction.
            const uint16_t n = WS2812_count();
            for (uint16_t i = 0; i < n; ++i) {
                uint8_t r = 0, g = 0, b = 0;
                if (i <= 2) { r = 0; g = 255; b = 0; }
                else if (i == 3) { r = 0; g = 255; b = 0; }
                else if (i == 4) { r = 255; g = 165; b = 0; }
                else if (i == 5) { r = 255; g = 0; b = 0; }
                WS2812_setLEDColor(i, r, g, b);
            }
            WS2812_show();
        }
        else {
            WS2812_allOff();
        }
    }

    void WS2812_sweep(const CRGB* /*colors*/, uint8_t /*count*/) {
        // Non-blocking stub: light each LED white in sequence once, no delay.
        const uint16_t n = WS2812_count();
        for (uint16_t i = 0; i < n; ++i) {
            WS2812_clearAll();
            WS2812_setLEDColor(i, 255, 255, 255);
            WS2812_show();
        }
        WS2812_allOff();
    }

    void testAOALevels() {
        // Legacy test pattern: 3=HIGH(red), 4=LOW(yellow), 5=NORMAL(green)
        WS2812_clearAll();
        if (WS2812_count() >= 6) {
            WS2812_setLEDColor(3, 255, 0, 0);   // red
            WS2812_setLEDColor(4, 255, 255, 0);   // yellow
            WS2812_setLEDColor(5, 0, 255, 0);   // green
        }
        WS2812_show();
    }

    void WS2812_testPattern() {
        // Mirror old behavior without needing CRGB construction
        WS2812_allOff();
        WS2812_setAllLEDs(true);   // applies per-index Lockshoot/AOA colors
    }

    /*
    void WS2812_tick() {
        // Single place to flush pending updates once per loop.
        WS2812_show();
    }
    */

    // helper: find slot by pin
    static int findSlotByPin(uint8_t pin) {
        for (int i = 0; i < (int)WS2812_MAX_STRIPS; ++i)
            if (g_slots[i].init && g_slots[i].pin == (int)pin) return i;
        return -1;
    }

    // helper: find free slot
    static int findFreeSlot() {
        for (int i = 0; i < (int)WS2812_MAX_STRIPS; ++i)
            if (!g_slots[i].init) return i;
        return -1;
    }

    // ===== Multi-strip API =====
    void WS2812_registerStrip(uint8_t pin, uint16_t count) {
        int s = findSlotByPin(pin);
        if (s < 0) {
            s = findFreeSlot();
            if (s < 0) return; // no capacity
            if (!g_slots[s].strip.init(pin, count)) return;
            g_slots[s].pin = pin;
            g_slots[s].count = (count <= WS2812_MAX_LEDS) ? count : WS2812_MAX_LEDS;
            g_slots[s].init = true;
            if (g_defaultSlot < 0) g_defaultSlot = s;          // set default once
            if (!g_init) { g_init = true; g_inst = g_slots[s].strip; g_count = g_slots[s].count; }
        }
        else {
            // already present: grow logical count if needed
            if (count > g_slots[s].count) g_slots[s].count = (count <= WS2812_MAX_LEDS) ? count : WS2812_MAX_LEDS;
        }
    }

    void WS2812_setLEDColor(uint8_t pin, uint16_t i, uint8_t r, uint8_t g, uint8_t b) {
        int s = findSlotByPin(pin);
        if (s < 0) return;
        g_slots[s].strip.setLED(i, r, g, b);
    }

    void WS2812_setLEDColor(uint8_t pin, uint16_t i, const CRGB& c) {
        WS2812_setLEDColor(pin, i, c.r, c.g, c.b);
    }

    void WS2812_setBrightness(uint8_t pin, uint8_t b) {
        int s = findSlotByPin(pin);
        if (s < 0) return;
        g_slots[s].strip.setBrightness(b);
    }

    void WS2812_showAll() {
        for (int s = 0; s < (int)WS2812_MAX_STRIPS; ++s)
            if (g_slots[s].init) g_slots[s].strip.show();
    }

    void WS2812_clearAllStrips() {
        for (int s = 0; s < (int)WS2812_MAX_STRIPS; ++s)
            if (g_slots[s].init) g_slots[s].strip.clear();
    }

    // Legacy tick now flushes all strips
    void WS2812_tick() {
        WS2812_showAll();
    }

    void initWS2812FromMap() {
        struct PinMax { uint8_t pin; uint8_t maxIndex; };
        PinMax list[WS2812_MAX_STRIPS]; uint8_t n = 0;

        for (size_t i = 0; i < panelLEDsCount; ++i) {
            const auto& m = panelLEDs[i];
            if (m.deviceType != DEVICE_WS2812) continue;
            const uint8_t pin = m.info.ws2812Info.pin;
            const uint8_t idx = m.info.ws2812Info.index;

            bool found = false;
            for (uint8_t j = 0; j < n; ++j) {
                if (list[j].pin == pin) { if (idx > list[j].maxIndex) list[j].maxIndex = idx; found = true; break; }
            }
            if (!found && n < WS2812_MAX_STRIPS) { list[n++] = PinMax{ pin, idx }; }
        }

        for (uint8_t j = 0; j < n; ++j) {
            WS2812_registerStrip(list[j].pin, (uint16_t)list[j].maxIndex + 1);
        }
    }

    void WS2812_allOnFromMap() {
        auto scale8 = [](uint8_t v, uint8_t gain) -> uint8_t {
            return static_cast<uint8_t>((static_cast<uint16_t>(v) * gain) >> 8);
            };

        for (size_t i = 0; i < panelLEDsCount; ++i) {
            const auto& m = panelLEDs[i];
            if (m.deviceType != DEVICE_WS2812) continue;
            const auto& w = m.info.ws2812Info;

            // Use mapping defaults only; no labels, no panel logic
            const uint8_t a = w.defBright;  // 0..255
            const CRGB color{ scale8(w.defR, a), scale8(w.defG, a), scale8(w.defB, a) };

            WS2812_setLEDColor(w.pin, w.index, color);
        }
        WS2812_showAll();
    }

    void WS2812_allOffAll() {
        WS2812_clearAllStrips();
        WS2812_showAll();
    }

} // namespace WS2812Mini