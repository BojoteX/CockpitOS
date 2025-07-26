// HIDManager.h

#pragma once

void HIDManager_setup();
void HIDManager_loop();
void HIDSenderTask(void* param);
size_t getMaxUsedGroup();

// ───── HID Axis ─────
/*
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
*/

// ───── HID Axis ─────
enum HIDAxis : uint8_t {
  AXIS_X = 0,         // 0x30
  AXIS_Y,             // 0x31
  AXIS_Z,             // 0x32
  AXIS_RX,            // 0x33
  AXIS_RY,            // 0x34
  AXIS_RZ,            // 0x35
  AXIS_SLIDER1,       // 0x36
  AXIS_SLIDER2,       // 0x36 (again)
  AXIS_DIAL,          // 0x37
  AXIS_WHEEL,         // 0x38
  AXIS_BRAKE,         // reuse Slider or vendor page
  AXIS_THROTTLE,      // reuse Slider or vendor page
  AXIS_CUSTOM1,       // vendor-defined (or Slider)
  AXIS_CUSTOM2,       // vendor-defined (or Slider)
  AXIS_CUSTOM3,       // vendor-defined (or Slider)
  AXIS_CUSTOM4,       // vendor-defined (or Slider)
  HID_AXIS_COUNT
};


// ───── Axis Input ─────
// void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin);
void HIDManager_resetAllAxes();
void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin, HIDAxis axis);

// ───── Named Button State Setters (zero heap) ─────
void HIDManager_setNamedButton(const char* name, bool deferSend = false, bool pressed = true);
void HIDManager_setToggleNamedButton(const char* name, bool deferSend = false);
void HIDManager_toggleIfPressed(bool isPressed, const char* label, bool deferSend = false);
void HIDManager_handleGuardedToggleIfPressed(bool isPressed, const char* buttonLabel, const char* coverLabel, bool deferSend = false);

// ───── Utility / Maintenance ─────
void HIDManager_commitDeferredReport(const char* deviceName);
bool shouldPollMs(unsigned long &lastPoll);
bool shouldPollDisplayRefreshMs(unsigned long& lastPoll);
void HIDManager_keepAlive();
void HIDManager_sendReport(const char* label, int32_t value = -1);
void flushBufferedHidCommands();
void HIDManager_dispatchReport(bool force = false);
void HID_keepAlive();
