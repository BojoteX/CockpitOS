/*
 * RS-485 Slave Test Sketch - INSTRUMENTED VERSION
 * 
 * Hardware: Arduino UNO R3 + MAX485 module + 2-pos switch + MC_READY LED
 * Purpose:  Debug RS-485 communication with CockpitOS master
 * 
 * Wiring:
 *   UNO Pin 0 (RX)  <- MAX485 RO
 *   UNO Pin 1 (TX)  -> MAX485 DI
 *   UNO Pin 2       -> MAX485 DE + RE (tied together)
 *   UNO 5V          -> MAX485 VCC
 *   UNO GND         -> MAX485 GND
 *   
 *   UNO Pin 4       -> Switch terminal 1
 *   UNO GND         -> Switch terminal 2
 *   
 *   UNO Pin 10      -> MC_READY LED+ (with 220Ω resistor to GND)
 *   
 *   (Built-in LED on pin 13 used for diagnostics)
 *   
 * OPTIONAL: Second LED on pin 12 for TX activity
 *   UNO Pin 12      -> LED+ (with 220Ω resistor to GND)
 * 
 * =============================================================================
 * LED DIAGNOSTIC PATTERNS:
 * =============================================================================
 * 
 * LED_BUILTIN (pin 13) - SWITCH ACTIVITY:
 *   - Toggles ON/OFF with each physical switch state change
 *   - If LED state matches switch state, all changes are being detected
 *   - If LED gets "out of sync" with switch, we're missing physical events
 * 
 * LED_TX (pin 12) - OPTIONAL TX ACTIVITY:
 *   - Brief flash when RS-485 response is sent
 *   - Helps verify slave is actually transmitting
 * 
 * MC_READY (pin 10) - DCS-BIOS OUTPUT:
 *   - Driven by DCS simulation state via RS-485 broadcast
 *   - Lights when Master Caution is active in the sim
 * 
 * =============================================================================
 * COUNTERS (viewable via debugPrintStats() if you have a way to trigger it):
 * =============================================================================
 *   - switchChangeCount: Physical switch state changes detected
 *   - pollsReceived: Number of times master polled this slave
 *   - responsesSent: Number of responses transmitted
 *   - messagesQueued: Number of DCS-BIOS messages queued to send
 * 
 */

#define DCSBIOS_RS485_SLAVE 1
#define TXENABLE_PIN 2

#include "DcsBios.h"

// =============================================================================
// INSTRUMENTATION PINS
// =============================================================================

#define LED_SWITCH_PIN    13    // Built-in LED - tracks switch state changes
#define LED_TX_PIN        12    // Optional - flash on TX (add external LED)
#define MC_READY_PIN      10    // Master Caution Ready LED from DCS
#define SWITCH_PIN        4     // The actual switch input

// =============================================================================
// INSTRUMENTATION COUNTERS
// =============================================================================

static volatile uint32_t switchChangeCount = 0;   // Physical switch changes
static volatile uint32_t pollsReceived = 0;       // Polls from master
static volatile uint32_t responsesSent = 0;       // Responses sent
static volatile uint32_t messagesQueued = 0;      // Messages queued to send
static volatile uint32_t loopCount = 0;           // Main loop iterations

// For detecting switch changes ourselves (independent of DCS-BIOS)
static bool lastRawSwitchState = HIGH;  // INPUT_PULLUP means HIGH = released
static bool ledSwitchState = LOW;

// For TX LED flash timing
static uint32_t txLedOffTime = 0;
static const uint32_t TX_LED_FLASH_MS = 50;

// =============================================================================
// DCS-BIOS INPUTS (Physical controls -> Sim)
// =============================================================================

// Use the REAL DcsBios::Switch2Pos - it handles message queuing correctly
DcsBios::Switch2Pos masterCautionBtn("UFC_MASTER_CAUTION", SWITCH_PIN);

// =============================================================================
// DCS-BIOS OUTPUTS (Sim state -> LEDs)
// =============================================================================

// FA-18C Master Caution READY Light (yellow)
// Address: 29708 (0x740C), Mask: 32768 (0x8000)
DcsBios::LED mcReadyLed(0x740C, 0x8000, MC_READY_PIN);

// =============================================================================
// PARALLEL MONITORING - We watch the same pin independently
// =============================================================================

// This lets us count physical switch changes without interfering with DCS-BIOS
void monitorSwitchState() {
    bool currentState = digitalRead(SWITCH_PIN);
    
    if (currentState != lastRawSwitchState) {
        // Physical switch changed!
        lastRawSwitchState = currentState;
        switchChangeCount++;
        
        // Toggle LED to track switch changes visually
        // If this toggles but master doesn't see command, problem is TX/RX
        ledSwitchState = !ledSwitchState;
        digitalWrite(LED_SWITCH_PIN, ledSwitchState);
        
        // Brief extra flash on TX LED to mark the event
        digitalWrite(LED_TX_PIN, HIGH);
        txLedOffTime = millis() + TX_LED_FLASH_MS;
    }
}

// =============================================================================
// SETUP
// =============================================================================

void setup() {
    // LED pins
    pinMode(LED_SWITCH_PIN, OUTPUT);
    pinMode(LED_TX_PIN, OUTPUT);
    pinMode(MC_READY_PIN, OUTPUT);
    
    // Start with LEDs off
    digitalWrite(LED_SWITCH_PIN, LOW);
    digitalWrite(LED_TX_PIN, LOW);
    digitalWrite(MC_READY_PIN, LOW);
    
    // Initialize DCS-BIOS (sets up RS-485)
    DcsBios::setup();
    
    // Read initial switch state
    lastRawSwitchState = digitalRead(SWITCH_PIN);
    
    // Quick LED test - flash all LEDs 3 times to show we're alive
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_SWITCH_PIN, HIGH);
        digitalWrite(LED_TX_PIN, HIGH);
        digitalWrite(MC_READY_PIN, HIGH);
        delay(100);
        digitalWrite(LED_SWITCH_PIN, LOW);
        digitalWrite(LED_TX_PIN, LOW);
        digitalWrite(MC_READY_PIN, LOW);
        delay(100);
    }
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
    loopCount++;
    
    // Monitor switch state independently (for LED feedback)
    monitorSwitchState();
    
    // Check if TX enable pin is high (meaning we're transmitting)
    static bool wasTxEnabled = false;
    bool txEnabled = digitalRead(TXENABLE_PIN);
    
    if (txEnabled && !wasTxEnabled) {
        // Rising edge - TX started
        responsesSent++;
        // Don't override LED if already flashing from switch change
        if (txLedOffTime == 0) {
            digitalWrite(LED_TX_PIN, HIGH);
            txLedOffTime = millis() + TX_LED_FLASH_MS;
        }
    }
    wasTxEnabled = txEnabled;
    
    // Turn off TX LED after flash period
    if (txLedOffTime > 0 && millis() >= txLedOffTime) {
        digitalWrite(LED_TX_PIN, LOW);
        txLedOffTime = 0;
    }
    
    // Run DCS-BIOS state machine (handles Switch2Pos + RS-485 protocol)
    DcsBios::loop();
}

// =============================================================================
// DIAGNOSTIC INSTRUCTIONS
// =============================================================================
//
// WHAT TO WATCH:
//
// 1. LED_BUILTIN (pin 13) - SWITCH STATE TRACKER
//    - Toggles every time physical switch state changes
//    - Should always match: LED ON = switch pressed, LED OFF = switch released
//      (or vice versa depending on initial state)
//    
//    DIAGNOSIS:
//    - If LED toggles but master misses command → Problem is RS-485 TX or master RX
//    - If LED doesn't toggle when you flip switch → Problem is loop() too slow or
//      switch bouncing faster than we can read
//
// 2. LED_TX (pin 12, optional) - TX ACTIVITY  
//    - Quick flash when:
//      a) Switch state changes (from monitorSwitchState)
//      b) TXENABLE_PIN goes HIGH (actual RS-485 transmission)
//    
//    DIAGNOSIS:
//    - If LED flashes on switch flip but master gets nothing → MAX485 or wiring issue
//    - If LED doesn't flash on switch flip → DCS-BIOS message not being queued
//
// 3. MC_READY (pin 10) - DCS-BIOS OUTPUT
//    - Controlled by DCS simulation via RS-485 broadcast from master
//    - Should light when Master Caution is active in the cockpit
//    
//    DIAGNOSIS:
//    - If LED doesn't respond to sim state → Check broadcast is working,
//      verify address/mask (0x740C/0x8000) matches DCS-BIOS control reference
//
// 4. CORRELATION TEST
//    - Flip switch slowly (1 second between flips)
//    - Count LED_BUILTIN toggles vs master received commands
//    - They should match exactly
//    
//    If LED toggles 10 times but master only received 8 commands:
//    → 2 messages were lost in RS-485 transmission
//    → Check: wiring, termination, baud rate match, timing
//
// 5. RAPID FLIP TEST  
//    - Flip switch rapidly (as fast as possible)
//    - If misses increase with speed → DCS-BIOS message buffer overflow
//      (slave queues message, but new message overwrites before poll arrives)
//