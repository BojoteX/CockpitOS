// LockShoot.cpp - Any custom logic for the LockShoot should go here

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()

// No include for LockShoot as its already in CUtils.h

// What LABEL SETs should include it? 
#if defined(HAS_MAIN)
	REGISTER_PANEL(LockShoot, nullptr, nullptr, nullptr, nullptr, WS2812_tick, 100);
#endif