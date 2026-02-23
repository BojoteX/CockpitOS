/*
 * RS-485 Slave Test Sketch - INSTRUMENTED VERSION
 * 
 * Hardware: Arduino Mega 2560 + MAX485 module + 2-pos switch + pushbutton + MC_READY LED
 * Purpose:  Debug RS-485 communication with CockpitOS master
 *
 * Wiring (UART1 — same pins as master, no cable swapping needed):
 *   Mega Pin 19 (RX1) <- MAX485 RO
 *   Mega Pin 18 (TX1) -> MAX485 DI
 *   Mega Pin 2        -> MAX485 DE + RE (tied together)
 *   Mega 5V           -> MAX485 VCC
 *   Mega GND          -> MAX485 GND
 *   
 *   Mega Pin 4        -> Switch terminal 1
 *   Mega GND          -> Switch terminal 2
 *
 *   Mega Pin 5        -> Pushbutton terminal 1
 *   Mega GND          -> Pushbutton terminal 2
 *
 *   Mega Pin A0       -> Potentiometer wiper (center pin)
 *   Mega 5V           -> Potentiometer one outer pin
 *   Mega GND          -> Potentiometer other outer pin
 *
 *   Mega Pin 10       -> MC_READY LED+ (with 220Ω resistor to GND)
 *   
 *   (Built-in LED on pin 13 used for diagnostics)
 *   
 * OPTIONAL: Second LED on pin 12 for TX activity
 *   Mega Pin 12       -> LED+ (with 220Ω resistor to GND)
 * 
 * =============================================================================
 * USER CONFIGURATION
 * =============================================================================
 */

#define SWITCH_PIN        4     // 2-position toggle switch
#define BUTTON_PIN        5     // Momentary pushbutton
#define POT_PIN           A0    // Potentiometer wiper (center pin)
#define MC_READY_PIN      10    // Master Caution Ready LED from DCS

/*
 * =============================================================================
 * LED DIAGNOSTIC PATTERNS:
 * =============================================================================
 * 
 * LED_BUILTIN (pin 13) - INPUT ACTIVITY:
 *   - Toggles ON/OFF with each physical input state change (switch or button)
 *   - If LED toggles but master doesn't see command, problem is TX/RX
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
 *   - inputChangeCount: Physical input state changes detected (switch + button)
 *   - pollsReceived: Number of times master polled this slave
 *   - responsesSent: Number of responses transmitted
 *   - messagesQueued: Number of DCS-BIOS messages queued to send
 * 
 */

#define DCSBIOS_RS485_SLAVE 2
#define TXENABLE_PIN 2
#define UART1_SELECT          // Use UART1 (pins 18/19) instead of UART0 (pins 0/1)

#include "DcsBios.h"

// =============================================================================
// INSTRUMENTATION PINS
// =============================================================================

#define LED_INPUT_PIN     13    // Built-in LED - tracks input state changes
#define LED_TX_PIN        12    // Optional - flash on TX (add external LED)

// =============================================================================
// INSTRUMENTATION COUNTERS
// =============================================================================

static volatile uint32_t inputChangeCount = 0;    // Physical input changes
static volatile uint32_t pollsReceived = 0;       // Polls from master
static volatile uint32_t responsesSent = 0;       // Responses sent
static volatile uint32_t messagesQueued = 0;      // Messages queued to send
static volatile uint32_t loopCount = 0;           // Main loop iterations

// For detecting input changes ourselves (independent of DCS-BIOS)
static bool lastRawSwitchState = HIGH;  // INPUT_PULLUP means HIGH = released
static bool lastRawButtonState = HIGH;  // INPUT_PULLUP means HIGH = released
static bool ledInputState = LOW;

// For TX LED flash timing
static uint32_t txLedOffTime = 0;
static const uint32_t TX_LED_FLASH_MS = 50;

// =============================================================================
// DCS-BIOS INPUTS (Physical controls -> Sim)
// =============================================================================

// 2-position toggle switch - Master Arm
DcsBios::Switch2Pos masterArmSw("MASTER_ARM_SW", SWITCH_PIN);

// Momentary pushbutton - Master Caution Reset
DcsBios::ActionButton masterCautionBtn("UFC_MASTER_CAUTION", "TOGGLE", BUTTON_PIN);

// Potentiometer - Left DDI Brightness
DcsBios::Potentiometer leftDdiBrt("LEFT_DDI_BRT_CTL", POT_PIN);

// =============================================================================
// DCS-BIOS OUTPUTS (Sim state -> LEDs)
// =============================================================================

// FA-18C Master Caution READY Light (yellow)
// Address: 29708 (0x740C), Mask: 32768 (0x8000)
DcsBios::LED mcReadyLed(0x740C, 0x8000, MC_READY_PIN);

// =============================================================================
// PARALLEL MONITORING - We watch the same pins independently
// =============================================================================

// This lets us count physical input changes without interfering with DCS-BIOS
void monitorInputStates() {
    bool changed = false;
    
    // Check switch
    bool currentSwitchState = digitalRead(SWITCH_PIN);
    if (currentSwitchState != lastRawSwitchState) {
        lastRawSwitchState = currentSwitchState;
        changed = true;
    }
    
    // Check button
    bool currentButtonState = digitalRead(BUTTON_PIN);
    if (currentButtonState != lastRawButtonState) {
        lastRawButtonState = currentButtonState;
        changed = true;
    }
    
    if (changed) {
        // Physical input changed!
        inputChangeCount++;
        
        // Toggle LED to track input changes visually
        ledInputState = !ledInputState;
        digitalWrite(LED_INPUT_PIN, ledInputState);
        
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
    pinMode(LED_INPUT_PIN, OUTPUT);
    pinMode(LED_TX_PIN, OUTPUT);
    pinMode(MC_READY_PIN, OUTPUT);
    
    // Start with LEDs off
    digitalWrite(LED_INPUT_PIN, LOW);
    digitalWrite(LED_TX_PIN, LOW);
    digitalWrite(MC_READY_PIN, LOW);
    
    // Initialize DCS-BIOS (sets up RS-485)
    DcsBios::setup();
    
    // Read initial input states
    lastRawSwitchState = digitalRead(SWITCH_PIN);
    lastRawButtonState = digitalRead(BUTTON_PIN);
    
    // Quick LED test - flash all LEDs 3 times to show we're alive
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_INPUT_PIN, HIGH);
        digitalWrite(LED_TX_PIN, HIGH);
        digitalWrite(MC_READY_PIN, HIGH);
        delay(100);
        digitalWrite(LED_INPUT_PIN, LOW);
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
    
    // Monitor input states independently (for LED feedback)
    monitorInputStates();
    
    // Check if TX enable pin is high (meaning we're transmitting)
    static bool wasTxEnabled = false;
    bool txEnabled = digitalRead(TXENABLE_PIN);
    
    if (txEnabled && !wasTxEnabled) {
        // Rising edge - TX started
        responsesSent++;
        // Don't override LED if already flashing from input change
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
    
    // Run DCS-BIOS state machine (handles inputs + RS-485 protocol)
    DcsBios::loop();
}

// =============================================================================
// DIAGNOSTIC INSTRUCTIONS
// =============================================================================
//
// WHAT TO WATCH:
//
// 1. LED_BUILTIN (pin 13) - INPUT STATE TRACKER
//    - Toggles every time physical switch OR button state changes
//    - Note: Button generates TWO toggles per press (press + release)
//    
//    DIAGNOSIS:
//    - If LED toggles but master misses command → Problem is RS-485 TX or master RX
//    - If LED doesn't toggle when you press button → Problem is wiring or loop() too slow
//
// 2. LED_TX (pin 12, optional) - TX ACTIVITY  
//    - Quick flash when:
//      a) Input state changes (from monitorInputStates)
//      b) TXENABLE_PIN goes HIGH (actual RS-485 transmission)
//    
//    DIAGNOSIS:
//    - If LED flashes on input change but master gets nothing → MAX485 or wiring issue
//    - If LED doesn't flash on input change → DCS-BIOS message not being queued
//
// 3. MC_READY (pin 10) - DCS-BIOS OUTPUT
//    - Controlled by DCS simulation via RS-485 broadcast from master
//    - Should light when Master Caution is active in the cockpit
//    - Press the Master Caution button to reset (light should go off)
//    
//    DIAGNOSIS:
//    - If LED doesn't respond to sim state → Check broadcast is working,
//      verify address/mask (0x740C/0x8000) matches DCS-BIOS control reference
//
// 4. CORRELATION TEST
//    - Press button slowly (1 second between presses)
//    - Count LED_BUILTIN toggles vs master received commands
//    - Remember: each button press = 2 toggles (press + release)
//    - But ActionButton only sends on PRESS, not release
//    
//    If LED toggles 20 times (10 presses) but master only received 8 commands:
//    → 2 messages were lost in RS-485 transmission
//    → Check: wiring, termination, baud rate match, timing
//
// 5. RAPID PRESS TEST  
//    - Press button rapidly (as fast as possible)
//    - If misses increase with speed → DCS-BIOS message buffer overflow
//      (slave queues message, but new message overwrites before poll arrives)
//
