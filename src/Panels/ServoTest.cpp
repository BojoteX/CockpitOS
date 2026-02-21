// ServoTest.cpp -- Motorized throttle servo test panel
// Uses the Servo_* LEDC API to drive a servo on GPIO 3 that tracks
// INT_THROTTLE_LEFT position (ungated, always tracking).
//
// The APU solenoid test is handled entirely through the LEDMapping
// DEVICE_GAUGE path (no custom code needed here).
//
// NOTE: ATC gating is not possible — THROTTLE_ATC_SW is definePushButton
// in DCS-BIOS (momentary output, not latching state).

#include "../Globals.h"
#include "../DCSBIOSBridge.h"

#if defined(HAS_SERVOTEST)

// -- Config --
static constexpr uint8_t  SERVO_PIN      = 3;
static constexpr uint16_t SERVO_MIN_US   = 800;
static constexpr uint16_t SERVO_MAX_US   = 2200;

// -- State --
static uint8_t  servoId    = 0xFF;   // invalid until attached
static uint16_t thrValue   = 0;      // last known throttle position (0-65535)

// -- Callbacks --
static void onThrottleChanged(const char* /*label*/, uint16_t value, uint16_t /*max_value*/) {
    thrValue = value;
    if (servoId == 0xFF) return;
    Servo_write(servoId, value);
}

// -- Lifecycle --

// DINIT: runs once at boot (initializeDisplays) — hardware setup + subscriptions
static void ServoTest_setup() {
    servoId = Servo_attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);

    if (servoId == 0xFF) {
        debugPrintf("ServoTest: FAILED to attach servo on GPIO %u\n", SERVO_PIN);
        return;
    }

    // Init sweep — same as resetAllGauges does for LEDMapping gauges
    Servo_enable(servoId);
    Servo_write(servoId, 65535);            // sweep to max
    delay(2000);
    Servo_write(servoId, 0);               // sweep to min
    delay(2000);

    thrValue = 0;

    subscribeToGaugeChange("INT_THROTTLE_LEFT", onThrottleChanged);

    debugPrintf("ServoTest: setup OK  servo=%u pin=%u range=%u-%u us\n",
        servoId, SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
}

// INIT: runs on mission sync (initializePanels) — clear cached state
static void ServoTest_init() {
    if (servoId == 0xFF) return;
    thrValue = 0;
    debugPrintf("ServoTest: mission sync — state reset, waiting for DCS data\n");
}

static void ServoTest_loop() {
    // Nothing to poll -- everything is callback-driven
}

REGISTER_PANEL(ServoTest, ServoTest_init, ServoTest_loop, ServoTest_setup, nullptr, nullptr, 100);

#endif // HAS_SERVOTEST
