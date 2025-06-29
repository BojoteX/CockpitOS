// WS2812 Standalone Test (LED Strip - Venezuela Flag Sweep)
#include <Arduino.h>
#include <FastLED.h>

#define WS2812B_PIN 35
#define NUM_LEDS 3

CRGB leds[NUM_LEDS];

// === Color Map (Venezuela Flag Colors) ===
enum VZColorKey { VZ_YELLOW, VZ_BLUE, VZ_RED };

const CRGB VZColors[] = {
  CRGB::Yellow,
  CRGB::Blue,
  CRGB::Red
};

// === Setup ===
void WS2812_init() {
  FastLED.addLeds<WS2812B, WS2812B_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  WS2812_clearAll();
}

void WS2812_clearAll() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void WS2812_allOn(CRGB color) {
  fill_solid(leds, NUM_LEDS, color);
  FastLED.show();
  Serial.println("🔆 WS2812 All ON");
}

void WS2812_allOff() {
  WS2812_clearAll();
  Serial.println("⚫ WS2812 All OFF");
}

void WS2812_sweep(CRGB colors[], uint8_t count) {
  Serial.println("🔁 WS2812 Sweep with custom colors:");
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    WS2812_clearAll();
    leds[i] = colors[i % count];
    FastLED.show();
    Serial.printf("🟢 LED %d ON with color: R=%d G=%d B=%d\n",
      i, colors[i % count].r, colors[i % count].g, colors[i % count].b);
    delay(400);
  }
  WS2812_clearAll();
  Serial.println("✅ WS2812 Sweep complete.");
}

void WS2812_testPattern() {
  Serial.println("🧪 WS2812 Test Pattern Start");
  WS2812_allOff();
  delay(300);

  // Sweep with Venezuela colors
  WS2812_sweep((CRGB*)VZColors, sizeof(VZColors) / sizeof(CRGB));
  delay(300);

  WS2812_allOn(CRGB::White);
  delay(1000);

  WS2812_clearAll();
  Serial.println("✅ WS2812 Test Pattern Complete");
  delay(2000);
}

void setup() {
  Serial.begin(115200);
  WS2812_init();
  delay(200);
}

void loop() {
  WS2812_testPattern();
  delay(3000);
}