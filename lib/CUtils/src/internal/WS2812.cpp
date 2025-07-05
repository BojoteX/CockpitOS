// ************************************************************
// WS2812 Unified Driver for ESP32 Arduino Core 2.x and 3.x
// ************************************************************

#include <Arduino.h>
#include <string.h>

// Version-adaptive RMT driver include
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
#include <driver/rmt_tx.h>
#else
#include <driver/rmt.h>
#endif

// All defines, struct CRGB, NUM_LEDS, pins, and color constants are declared in the header!
// Here, for completeness, put only if your build breaks without them; otherwise omit!

// === BEGIN YOUR GLOBALS (NO EXTERN!) ===
CRGB leds[NUM_LEDS];
uint8_t pixels[NUM_LEDS * 3];
uint8_t brightness = 255;
uint32_t lastShowTime = 0;
bool wsDirty = false;
// === END GLOBALS ===

void WS2812_init() {
    pinMode(WS2812B_PIN, OUTPUT);
    digitalWrite(WS2812B_PIN, LOW);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    rmtInit(WS2812B_PIN, RMT_TX_MODE, RMT_MEM_NUM_BLOCKS_1, 10000000);
    rmtSetEOT(WS2812B_PIN, 0);
#else
    rmt_config_t config;
    config.rmt_mode = RMT_MODE_TX;
    config.channel = RMT_CHANNEL_0;
    config.gpio_num = (gpio_num_t)WS2812B_PIN;
    config.mem_block_num = 1;
    config.tx_config.loop_en = false;
    config.tx_config.carrier_en = false;
    config.tx_config.idle_output_en = true;
    config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    config.clk_div = 2;
    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);
#endif

    memset(pixels, 0, NUM_LEDS * 3);
    memset(leds, 0, NUM_LEDS * sizeof(CRGB));
    wsDirty = true;
    WS2812_clearAll();
}

void WS2812_show() {
    uint32_t now = micros();
    if (now - lastShowTime < 50) {
        delayMicroseconds(50 - (now - lastShowTime));
    }

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    const uint8_t highTicks1 = 8;
    const uint8_t lowTicks1 = 4;
    const uint8_t highTicks0 = 4;
    const uint8_t lowTicks0 = 8;

    rmt_data_t rmtBuffer[NUM_LEDS * 24];
    int symbolIndex = 0;

    for (int i = 0; i < NUM_LEDS; ++i) {
        uint8_t g = (pixels[i * 3 + 0] * brightness) >> 8;
        uint8_t r = (pixels[i * 3 + 1] * brightness) >> 8;
        uint8_t b = (pixels[i * 3 + 2] * brightness) >> 8;

        uint32_t pixelData = (uint32_t)g << 16 | (uint32_t)r << 8 | b;

        for (int bit = 23; bit >= 0; --bit) {
            bool bitVal = pixelData & (1UL << bit);
            rmtBuffer[symbolIndex].level0 = 1;
            rmtBuffer[symbolIndex].duration0 = bitVal ? highTicks1 : highTicks0;
            rmtBuffer[symbolIndex].level1 = 0;
            rmtBuffer[symbolIndex].duration1 = bitVal ? lowTicks1 : lowTicks0;
            symbolIndex++;
        }
    }
    rmtWrite(WS2812B_PIN, rmtBuffer, symbolIndex, RMT_WAIT_FOR_EVER);
#else
    // WS2812B timing for Core 2.x, 40MHz, 25ns/tick
    static const uint8_t T1H = 32; // 0.8us
    static const uint8_t T1L = 18; // 0.45us
    static const uint8_t T0H = 16; // 0.4us
    static const uint8_t T0L = 34; // 0.85us

    rmt_item32_t items[NUM_LEDS * 24];
    int symbolIndex = 0;

    for (int i = 0; i < NUM_LEDS; ++i) {
        uint8_t g = (pixels[i * 3 + 0] * brightness) >> 8;
        uint8_t r = (pixels[i * 3 + 1] * brightness) >> 8;
        uint8_t b = (pixels[i * 3 + 2] * brightness) >> 8;

        uint32_t pixelData = (uint32_t)g << 16 | (uint32_t)r << 8 | b;

        for (int bit = 23; bit >= 0; --bit) {
            bool bitVal = pixelData & (1UL << bit);
            if (bitVal) {
                items[symbolIndex].level0 = 1;
                items[symbolIndex].duration0 = T1H;
                items[symbolIndex].level1 = 0;
                items[symbolIndex].duration1 = T1L;
            }
            else {
                items[symbolIndex].level0 = 1;
                items[symbolIndex].duration0 = T0H;
                items[symbolIndex].level1 = 0;
                items[symbolIndex].duration1 = T0L;
            }
            symbolIndex++;
        }
    }
    rmt_write_items(RMT_CHANNEL_0, items, symbolIndex, true);
    rmt_wait_tx_done(RMT_CHANNEL_0, portMAX_DELAY);
#endif
    lastShowTime = micros();
}

void WS2812_setLEDColor(uint8_t ledIndex, CRGB color) {
    if (!hasLockShoot) return;
    if (ledIndex >= NUM_LEDS) return;
    leds[ledIndex] = color;
    pixels[ledIndex * 3 + 0] = color.g;
    pixels[ledIndex * 3 + 1] = color.r;
    pixels[ledIndex * 3 + 2] = color.b;
    wsDirty = true;
}

void WS2812_clearAll() {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = { 0, 0, 0 };
        pixels[i * 3 + 0] = 0;
        pixels[i * 3 + 1] = 0;
        pixels[i * 3 + 2] = 0;
    }
    wsDirty = true;
}

static const CRGB VZColors[] = { Yellow, Blue, Red };

void WS2812_allOn(CRGB color) {
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        leds[i] = color;
        pixels[i * 3 + 0] = color.g;
        pixels[i * 3 + 1] = color.r;
        pixels[i * 3 + 2] = color.b;
    }
    WS2812_show();
}

void WS2812_allOff() {
    WS2812_clearAll();
    debugPrintln("⚫ WS2812 All OFF");
}

void WS2812_sweep(const CRGB* colors, uint8_t count) {
    debugPrintln("🔁 WS2812 Sweep with custom colors:");
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        WS2812_clearAll();
        leds[i] = colors[i % count];
        WS2812_show();
        debugPrintf("🟢 LED %d ON with color: R=%d G=%d B=%d\n",
            i, colors[i % count].r, colors[i % count].g, colors[i % count].b);
        delay(400);
    }
    WS2812_clearAll();
    debugPrintln("✅ WS2812 Sweep complete.");
}

void WS2812_testPattern() {
    debugPrintln("🧪 WS2812 Test Pattern Start");
    WS2812_allOff();
    WS2812_sweep(VZColors, sizeof(VZColors) / sizeof(CRGB));
    WS2812_allOn(Green);
    debugPrintln("✅ WS2812 Test Pattern Complete");
}

void WS2812_setAllLEDs(bool state) {
    if (state) WS2812_allOn(Green);
    else WS2812_clearAll();
}

void WS2812_tick() {
    if (wsDirty) {
        WS2812_show();
        wsDirty = false;
    }
}