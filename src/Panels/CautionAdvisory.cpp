// CautionAdvisory.cpp - Any custom logic for the CA (Caution Advisory) should go here

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h" // for timing helpers like shouldPollMs()

// No include for CA as its already in CUtils.h

// What LABEL SETs should include it? 
#if defined(HAS_CUSTOM_RIGHT)
	REGISTER_PANEL(CA, nullptr, nullptr, nullptr, nullptr, GN1640_tick, 100);
#endif