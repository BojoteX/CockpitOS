// HIDManager.h

#pragma once

void HIDManager_setup();
void HIDManager_loop();
void HIDSenderTask(void* param);

// ───── Axis Input ─────
void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin);

// ───── Named Button State Setters (zero heap) ─────
void HIDManager_setNamedButton(const char* name, bool deferSend = false, bool pressed = true);
void HIDManager_setToggleNamedButton(const char* name, bool deferSend = false);
void HIDManager_toggleIfPressed(bool isPressed, const char* label, bool deferSend = false);

// ───── Guarded Helpers ─────
void HIDManager_handleGuardedToggleIfPressed(bool isPressed, const char* buttonLabel, const char* coverLabel, bool deferSend = false);
void HIDManager_handleCosmeticGuardedSelector(bool switchOn, const char* labelOn, const char* labelOff, const char* coverLabel, bool deferSend = false);
void HIDManager_handleGuardedToggle(bool isPressed, const char* switchLabel, const char* coverLabel, const char* fallbackLabel = nullptr, bool deferSend = false);
void HIDManager_handleGuardedMomentary(bool isPressed, const char* buttonLabel, const char* coverLabel, bool deferSend = false);

// ───── Utility / Maintenance ─────
void HIDManager_commitDeferredReport(const char* deviceName);
bool shouldPollMs(unsigned long &lastPoll);
bool shouldPollDisplayRefreshMs(unsigned long& lastPoll);
void HIDManager_keepAlive();
void HIDManager_sendReport(const char* label, int32_t value = -1);

// ───── HID Axis ─────
enum HIDAxis : uint8_t {
  AXIS_X = 0,
  AXIS_Y,
  AXIS_Z,
  AXIS_RX,
  AXIS_RY,
  AXIS_RZ,
  AXIS_SLIDER1,
  AXIS_SLIDER2,
  HID_AXIS_COUNT
};
void HIDManager_resetAllAxes();

void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin, HIDAxis axis);
void flushBufferedHidCommands();
void HIDManager_dispatchReport(bool force = false);
void HID_keepAlive();
