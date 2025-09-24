// HIDManager.h

#pragma once

#include "CoverGate.h"

void HIDManager_startUSB();
void HIDManager_setup();
void HIDManager_loop();
void HIDSenderTask(void* param);
size_t getMaxUsedGroup();

// ───── HID Axis ─────
enum HIDAxis : uint8_t {
  AXIS_X = 0,         // 0x30
  AXIS_Y,             // 0x31
  AXIS_Z,             // 0x32
  AXIS_RX,            // 0x33
  AXIS_RY,            // 0x34
  AXIS_RZ,            // 0x35
  AXIS_DIAL,          // 0x37
  AXIS_SLIDER,        // 0x36
  AXIS_SLIDER1,       // 0x36 (again) 
  AXIS_SLIDER2,       // 0x36 (again)
  AXIS_SLIDER3,       // 0x36 (again)
  AXIS_SLIDER4,       // 0x36 (again)
  AXIS_SLIDER5,       // 0x36 (again)
  AXIS_SLIDER6,       // 0x36 (again)
  AXIS_SLIDER7,       // 0x36 (again)
  AXIS_SLIDER8,       // 0x36 (again)
  HID_AXIS_COUNT
};

static constexpr uint16_t buildInvertMask() {
  uint16_t m = 0u;
  #define ADD(ax) m |= (uint16_t(1u) << (ax));
  INVERTED_AXIS_LIST(ADD)
  #undef ADD
  return m;
}

static constexpr uint16_t AXIS_INVERT_MASK = buildInvertMask();
static inline bool axisInverted(HIDAxis a) { return (AXIS_INVERT_MASK >> a) & 1u; }

// ───── Axis Input ─────
// void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin, bool forceSend = false);
void HIDManager_resetAllAxes();
void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin, HIDAxis axis, bool forceSend = false, bool deferredSend = false);

// ───── Named Button State Setters (zero heap) ─────
// void HIDManager_setNamedButton(const char* name, bool deferSend = false, bool pressed = true);
void HIDManager_setNamedButton(const char* name, bool deferSend, bool pressed);

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
