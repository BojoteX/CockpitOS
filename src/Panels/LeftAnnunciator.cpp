// LeftAnnunciator.cpp - Any custom logic for the Left Annunciator should go here

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()

// No include for LeftAnnunciator as its already in CUtils.h

// What LABEL SETs should include it? 
#if defined(HAS_MAIN)
	REGISTER_PANEL(LA, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
#endif