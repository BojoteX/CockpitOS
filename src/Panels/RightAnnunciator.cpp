// RightAnnunciator.cpp - Any custom logic for the Right Annunciator should go here

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()

// No include for RightAnnunciator as its already in CUtils.h

// What LABEL SETs should include it? 
#if defined(HAS_MAIN)
	REGISTER_PANEL(RA, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
#endif