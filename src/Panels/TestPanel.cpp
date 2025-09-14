// TestPanel.cpp

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()

// What LABEL SETs should include it? 
#if defined(HAS_TEST_ONLY)
	// REGISTER_PANEL(LockShoot, nullptr, nullptr, nullptr, nullptr, WS2812_tick, 100);
#endif