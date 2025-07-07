// Globals.h

#pragma once

/* NOT USED ANYMORE
// You dont need to change this addresses, they are the broadcast address from your network and the DCS address is obtained dynamically.
#define DCS_COMPUTER_IP_ADDRESS                   "255.255.255.255" // This is a placeholder, the real IP is obtained when the stream starts
#define DEBUG_CONSOLE_IP_ADDRESS                  "255.255.255.255" // Also a place holder, this sends debug info to any computer on your network
*/

#include "PsramConfig.h"
#include <Arduino.h>
#include "../Config.h"

// DO NOT CHANGE ANYTHING ABOVE THIS LINE....
#include "../lib/CUtils/src/CUtils.h" 

// So we can call our custom print functions from anywhere
#include "debugPrint.h"

#if (USE_DCSBIOS_WIFI || USE_DCSBIOS_USB)
#include "RingBuffer.h"
#endif

#if DEBUG_PERFORMANCE
#include "PerfMonitor.h"
#endif

///////////////////////////////////////////////////////////////////////////////////
// Extern declarations								  /
///////////////////////////////////////////////////////////////////////////////////

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
extern bool HID_can_send_report();
extern void panelLoop();
extern void initializePanels(bool force = false);