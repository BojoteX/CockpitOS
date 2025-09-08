// Globals.h

#pragma once

/* NOT USED ANYMORE
// You dont need to change this addresses, they are the broadcast address from your network and the DCS address is obtained dynamically.
#define DCS_COMPUTER_IP_ADDRESS                   "255.255.255.255" // This is a placeholder, the real IP is obtained when the stream starts
#define DEBUG_CONSOLE_IP_ADDRESS                  "255.255.255.255" // Also a place holder, this sends debug info to any computer on your network
*/

// Converts S2 PINs to S3 for compatibility with TEK backplane (Brain Controller) used by the S2

#include "PsramConfig.h"
#include <Arduino.h>
#include "../Config.h"

// PIN Macro and PIN defines
#include "PinMap.h"
#include "../Pins.h"

// DO NOT CHANGE ANYTHING ABOVE THIS LINE....
#include "../lib/CUtils/src/CUtils.h" 

// Centralized input control management (PCA/GPIO/HC165)
#include "InputControl.h"

// So we can call our custom print functions from anywhere
#include "debugPrint.h"

#if (USE_DCSBIOS_WIFI || USE_DCSBIOS_USB || USE_DCSBIOS_BLUETOOTH)
#include "RingBuffer.h"
#endif

#if DEBUG_PERFORMANCE
#include "PerfMonitor.h"
#endif

// Panel Auto-Registration
#include "PanelRegistry.h"

///////////////////////////////////////////////////////////////////////////////////
// Extern declarations								  /
///////////////////////////////////////////////////////////////////////////////////

#if ARDUINO_USB_CDC_ON_BOOT == 1
	// Do nothing, we use Serial, which is already aliased to CDC
#else
    #if ARDUINO_USB_MODE == 1
        extern HWCDC HWCDCSerial;
    #else
        #if (ARDUINO_USB_MODE == 0)
            #if (USE_DCSBIOS_SERIAL || VERBOSE_MODE_SERIAL_ONLY || VERBOSE_MODE)
                #include "USB.h"
                extern USBCDC USBSerial;
            #endif
        #endif
    #endif
#endif

// For edge cases
extern volatile bool forcePanelResyncNow;

// From our main .ino file
extern bool isModeSelectorDCS();	// To Check in which mode we are operating

// From our DCSBIOSBridge.cpp
extern bool isSerialConnected();	// Check if we are connected
extern bool isMissionRunning();		
extern bool isPanelsSyncedThisMission();
extern bool forcePanelSyncThisMission;

// first feclared in main .ino and debugPrint.cpp
extern volatile bool mainLoopStarted;
extern bool DEBUG;
extern bool debugToSerial;   		// true = allow Serial prints
extern bool debugToUDP;      		// true = allow UDP prints

// Init panels from anywhere
extern bool loadUSBevents;
extern bool loadCDCevents;
extern bool HID_can_send_report();
extern void panelLoop();
extern void initializePanels(bool force = false);