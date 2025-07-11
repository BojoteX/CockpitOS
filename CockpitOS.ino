// Main CockpitOS.ino

#include "src/Globals.h" // Common header used by everyone
#include "src/HIDManager.h" // Needed for _init and _loop for HIDManager
#include "src/DCSBIOSBridge.h" // Needed for _init and _loop for DCSBIOSBridge
#include "src/HIDManager.h" // Needed for MAX_GROUPS check
#include "src/LEDControl.h" // Needed to initialize LEDs from our main .ino file

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "src/WiFiDebug.h" // Only WiFi
#endif

// Keep track when the main loop starts
volatile bool mainLoopStarted = false;

// Force compilation of all CUtils internals (Arduino IDE won't compile .cpps in lib automatically)
#include "lib/CUtils/src/CUtils.cpp"

// Checks mode selector state
bool isModeSelectorDCS() {
  #if defined(HAS_HID_MODE_SELECTOR) && (HAS_HID_MODE_SELECTOR == 1)
    return digitalRead(MODE_SWITCH_PIN) == LOW;
  #else
    #if MODE_DEFAULT_IS_HID
        return false;
    #else
        return true;
    #endif
  #endif
}

void checkHealth() {
  // --- Internal SRAM (on-chip) ---
  size_t free_int    = heap_caps_get_free_size         (MALLOC_CAP_INTERNAL);
  size_t largest_int = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
  float  frag_int    = free_int
                     ? 100.0f * (1.0f - (float)largest_int / (float)free_int)
                     : 0.0f;

  // --- External PSRAM (if present) ---
  size_t free_psram    = heap_caps_get_free_size         (MALLOC_CAP_SPIRAM);
  size_t largest_psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
  float  frag_psram    = free_psram
                       ? 100.0f * (1.0f - (float)largest_psram / (float)free_psram)
                       : 0.0f;

  // --- Print it all out ---
  debugPrintf(
    "SRAM free: %6u KB, largest: %6u KB, frag: %5.1f%%\n",
    (unsigned)(free_int    / 1024),
    (unsigned)(largest_int / 1024),
    frag_int
  );

  debugPrintf(
    "PSRAM free: %6u KB, largest: %6u KB, frag: %5.1f%%\n",
    (unsigned)(free_psram    / 1024),
    (unsigned)(largest_psram / 1024),
    frag_psram
  );

  if (getMaxUsedGroup() >= MAX_GROUPS) {
      debugPrintln("❌ Too many unique selector groups — increase MAX_GROUPS in Config.h");
      while (true); // or handle as needed
  }

}

void setup() {

    // Only do this if we have a selector
    #if defined(HAS_HID_MODE_SELECTOR) && (HAS_HID_MODE_SELECTOR == 1)
    pinMode(MODE_SWITCH_PIN, INPUT_PULLUP); // GPIO Setup
    #endif

    // First parameter is output to Serial, second one is output to UDP (only use this for overriding output)
    debugInit();
    debugSetOutput(debugToSerial, debugToUDP);  

    // Sets standard read resolution and attenuation
    analogReadResolution(13); // Only value consistent across Arduino Core installs 2.0+ / 3.0+ (0-8191)
    analogSetAttenuation(ADC_11db);      

    // Init our CDC + HID Interfaces
    DCSBIOSBridge_setup();  // We get CDC started here    
    HIDManager_setup();     // We get HID started here 

    #if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
    wifi_setup();
    #endif

    debugPrintln("\n=================== [STARTING DEVICE] ===================");

    // Shows available mem/heap frag etc.
    checkHealth();   
    debugPrintln();

    // Init PCA + Detect PCA Panels (Automatic PCA detection)    
    PCA9555_init();
    
    // Init Mappings (Aircraft specific)
    initMappings();   

    // If you set DEBUG_ENABLED or DEBUG_ENABLED_FOR_PCA_ONLY you get PCA9555 logging
  #if DEBUG_ENABLED_FOR_PCA_ONLY
    enablePCA9555Logging(1);
  #else  
    enablePCA9555Logging(DEBUG); // If you set DEBUG_ENABLED only it will also log PCA, otherwise no PCA logging.
  #endif

    // Initialize PCA9555 Inputs + Cached Port States explicitly to OFF (active-low LEDs)
    PCA9555_initCache();   

    // Initializes your Displays etc. 
    debugPrintln("Initializing Displays");
    initializeDisplays();

    // Initializes your LEDs
    debugPrintln("Initializing LEDs");
    initializeLEDs();

    // Initializes you panel buttons (forced)
    initializePanels(1); // Force panel init (they only sync in the main loop unless we pass 1/true)
    
    // When TEST_LEDS is active device enters a menu selection to test LEDs individually. You activate them via Serial Console
    #if TEST_LEDS
        printLEDMenu();
        handleLEDSelection();
        debugPrintln("Exiting LED selection menu. Continuing execution...");
    #endif 
    
    // A loopback stream test that feeds the parser with simulated DCS data while leaving the Serial console available for debugging
    #if IS_REPLAY
    replayData();
    #endif

    #if DEBUG_PERFORMANCE
    initPerfMonitor(); // this is used for profiling, see PerfMonitor.h for details, format and how to add LABELS for profiling blocks
    #endif     

    if (!isModeSelectorDCS()) {
        debugPrintln("Selected mode: HID");
    }
    else {
        debugPrintln("Selected mode: DCS-BIOS");
    }    

    if(DEBUG) {
        debugPrintf("Device \"%s\" is ready (DEBUG ENABLED)\n", USB_SERIAL);
    }
    else {
        debugPrintf("Device \"%s\" is ready\n", USB_SERIAL);
    }       
    #if USE_DCSBIOS_USB
    debugPrintln("ATTENTION: USB mode ENABLED. Run CockpitOS_HID_Controller.py on the computer where your devices are connected.");
    #endif
}

void loop() {

    if(!mainLoopStarted) {
      // This is a dummy report, it will only run ONCE (when loop starts) to trigger a Feature request to our device when USE_DCSBIOS_USB is active to do an initial drain of our ring buffer to allow the USB handshake to happen
      HIDManager_dispatchReport(true);
    }
    mainLoopStarted = true;

    // Performance Profiling using beginProfiling("name") -> endProfiling("name") but only when DEBUG_PERFORMANCE  
    #if DEBUG_PERFORMANCE
    beginProfiling(PERF_MAIN_LOOP);
    #endif

    // Call our panels loop logic (Mappings.cpp is where this function lives)
    panelLoop();  

    // Loop functions for specific Buttons/DCSBIOS Library logic
    DCSBIOSBridge_loop();
    HIDManager_loop();  

    // All profiling blocks REQUIRE we close them
    #if DEBUG_PERFORMANCE
    endProfiling(PERF_MAIN_LOOP);
    #endif

    // If you are profiling in a self contained block outside of the main loop, use perfMonitorUpdate() to simulate iterations.
    #if DEBUG_PERFORMANCE
    perfMonitorUpdate();
    #endif   
}