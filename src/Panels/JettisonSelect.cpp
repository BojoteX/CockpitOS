// JettisonSelect.cpp - Any custom logic for Jettison Select should go here

// PANEL_KIND: JETTSEL

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()

// What LABEL SETs should include it? 
#if defined(HAS_IFEI)
	REGISTER_PANEL(JETTSEL, nullptr, nullptr, nullptr, nullptr, tm1637_tick, 100);
#endif