// CYD Backlight PWM Test — ESP32-2432S028R
// Fades backlight from dim to bright and back, repeating forever.

#define BACKLIGHT_PIN 21
#define LEDC_FREQ     5000
#define LEDC_BITS     8       // 0-255 duty range

void setup() {
    Serial.begin(115200);
    Serial.println("CYD Backlight Test - GPIO 21");

    // Arduino ESP32 core 3.x API (ESP-IDF v5)
    ledcAttach(BACKLIGHT_PIN, LEDC_FREQ, LEDC_BITS);
    ledcWrite(BACKLIGHT_PIN, 255);  // start bright
}

void loop() {
    // Bright -> Dim
    Serial.println("Fading down...");
    for (int i = 255; i >= 0; i--) {
        ledcWrite(BACKLIGHT_PIN, i);
        delay(8);
    }
    delay(500);

    // Dim -> Bright
    Serial.println("Fading up...");
    for (int i = 0; i <= 255; i++) {
        ledcWrite(BACKLIGHT_PIN, i);
        delay(8);
    }
    delay(500);
}
