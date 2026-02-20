// CUtils.cpp

#include "../../../src/Globals.h"

// --- Serial alias for this file only ---
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || (ARDUINO_USB_CDC_ON_BOOT == 0)
    #if defined(ARDUINO_USB_MODE) && (ARDUINO_USB_MODE == 0)   // TinyUSB CDC
        #ifdef Serial
            #undef Serial
        #endif
        #define Serial USBSerial
    #elif defined(ARDUINO_USB_MODE) && (ARDUINO_USB_MODE == 1) // HW CDC
        #ifdef Serial
            #undef Serial
        #endif
        #define Serial HWCDCSerial
    #endif
#endif

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "../../../src/WiFiDebug.h"
#endif

// Choose a compile-time constant that’s safely ≥ panelLEDsCount.
// Here we use 128 as a reasonable upper bound:
static int displayedIndexes[128];
static int displayedCount = 0;

// —— globals —— 
I2CDeviceInfo   discoveredDevices[MAX_DEVICES];
uint8_t         discoveredDeviceCount = 0;
const char*     panelNameByAddr[I2C_ADDR_SPACE] = { nullptr };

// PCA9555 write/read cache
// indexed by (address - 0x20), port 0 or 1
uint8_t PCA9555_cachedPortStates[8][2] = {{0}};

// Include all internal modules
#include "internal/WS2812.cpp"  
#include "internal/HT1622.cpp"
#include "internal/GPIO.cpp"   
#include "internal/TM1637.cpp"  
#include "internal/GN1640.cpp"  

#if ENABLE_PCA9555
    #include "internal/PCA9555.cpp" 
#endif

#include "internal/MatrixRotary.cpp"
#include "internal/HC165.cpp"
#include "internal/AnalogG.cpp"

// RS-485 Master (bus controller)
#if RS485_MASTER_ENABLED
#include "internal/RS485Master.cpp"
#endif

// RS-485 Slave (on existing bus)
#if RS485_SLAVE_ENABLED
#include "internal/RS485Slave.cpp"
#endif


bool panelExists(uint8_t targetAddr) {
  for (uint8_t i = 0; i < discoveredDeviceCount; ++i)
    if (discoveredDevices[i].address == targetAddr) return true;
  return false;
}

void printDiscoveredPanels() {
  if (discoveredDeviceCount == 0) {
    debugPrintln("No I2C devices found.");
    return;
  }
  debugPrintln("\n🔎 === Discovered I2C Devices ===");
  debugPrintln("📋 Address    | Device Description");
  debugPrintln("──────────────|─────────────────────────────");
  for (uint8_t i = 0; i < discoveredDeviceCount; ++i) {
    char buf[64];
    snprintf(buf, sizeof(buf),
             "📡 0x%02X       | %s",
             discoveredDevices[i].address,
             discoveredDevices[i].label);
    debugPrintln(buf);
  }
  debugPrintln("────────────────────────────────────────────\n");
}

// —— LED Debug Menu  —— 
void printLEDMenu() {
    constexpr int columns = 3;
    constexpr int colWidth = 25;
    char line[128];
    displayedCount = 0; // <-- Must be global/static for handleLEDSelection!
    serialDebugPrintln("\n--- LED Selection Menu ---\n");
    for (int i = 0; i < panelLEDsCount; ++i) {
        int cursor = 0;
        cursor += snprintf(line + cursor, sizeof(line) - cursor, "%d: %s", displayedCount, panelLEDs[i].label);
        int len = strlen(panelLEDs[i].label);
        for (int s = 0; s < colWidth - len && cursor < sizeof(line) - 1; ++s)
            line[cursor++] = ' ';
        displayedIndexes[displayedCount++] = i; // <---- CRITICAL!
        line[cursor] = '\0';
        serialDebugPrint(line);

        if ((i + 1) % columns == 0 || i == panelLEDsCount - 1)
            serialDebugPrintln("");
    }
    #if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
    wifiDebugPrintln("Open serial console for interactive LED test");
    #endif
}

void handleLEDSelection() {
  #if (USE_DCSBIOS_SERIAL || VERBOSE_MODE_SERIAL_ONLY || VERBOSE_MODE)
  while (true) {
    serialDebugPrintln("Enter LED number to activate (or press Enter to exit):");
    while (!Serial.available()) yield();

    char inputBuf[128];
    size_t len = Serial.readBytesUntil('\n', inputBuf, sizeof(inputBuf) - 1);
    inputBuf[len] = '\0';

    if (len == 0) break;

    int sel = atoi(inputBuf);
    if (sel >= 0 && sel < displayedCount) {
      int idx = displayedIndexes[sel];
      serialDebugPrintf("Activating LED: %s\n", panelLEDs[idx].label);
      setLED(panelLEDs[idx].label, true, 100);
      panelLoop();
      delay(5000);
      setLED(panelLEDs[idx].label, false, 0);
      panelLoop();
      serialDebugPrintf("Deactivated LED: %s\n", panelLEDs[idx].label);
      printLEDMenu();
    } else {
      serialDebugPrintln("Invalid selection or unsupported LED.");
    }
  }
  #endif
}

// —— Replay SOCAT Stream for debugging without using serial —— 
void runReplayWithPrompt() {
  #if (USE_DCSBIOS_SERIAL || VERBOSE_MODE_SERIAL_ONLY || VERBOSE_MODE)
  bool infinite = false;
  while (true) {
    DcsbiosProtocolReplay();
    if (infinite) continue;

    serialDebugPrintln("\n=== REPLAY FINISHED ===");
    serialDebugPrintln("1) One more iteration");
    serialDebugPrintln("2) Run infinitely");
    serialDebugPrintln("3) Quit to main program");
    serialDebugPrint("Choose [1-3]: ");
    while (!Serial.available()) yield();
    char c = Serial.read();
    while (Serial.available()) Serial.read();
    // serialDebugPrintln(c);
    char msg[2] = {c, 0};
    serialDebugPrintln(msg);

    switch (c) {
      case '1': break;
      case '2': infinite = true; serialDebugPrintln(">>> infinite replay mode <<<"); break;
      case '3': serialDebugPrintln(">>> exiting replay <<<"); return;
      default:  serialDebugPrintln("Invalid choice; please enter 1, 2, or 3."); break;
    }
  }
  #endif
}

// ============================================================================
//  UTILITY FUNCTIONS (Minimal, pure helpers)
// ============================================================================
bool startsWith(const char* s, const char* pfx) {
    return s && pfx && strncmp(s, pfx, strlen(pfx)) == 0;
}
uint8_t hexNib(char c) {
    return (c >= '0' && c <= '9') ? c - '0' :
        (c >= 'a' && c <= 'f') ? 10 + c - 'a' :
        (c >= 'A' && c <= 'F') ? 10 + c - 'A' : 0;
}
uint8_t parseHexByte(const char* s) { // expects "0xNN"
    if (!s || strlen(s) < 3) return 0;
    return (hexNib(s[2]) << 4) | hexNib(s[3]);
}