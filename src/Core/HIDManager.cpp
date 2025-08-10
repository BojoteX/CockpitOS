// HIDManager.cpp

#include <USB.h>
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"
#include "../HIDDescriptors.h"

#if (USE_DCSBIOS_USB && ARDUINO_USB_CDC_ON_BOOT == 1 && !ALLOW_CONCURRENT_CDC_AND_HID)
#error "Invalid configuration: USE_DCSBIOS_USB requires that you compile this sketch with ARDUINO_USB_CDC_ON_BOOT set to 0, to do this, see Tools menu and set option USB CDC On Boot: Disabled"
#endif

#if (VERBOSE_MODE || VERBOSE_MODE_SERIAL_ONLY) && !ARDUINO_USB_CDC_ON_BOOT == 1
#error "Invalid configuration: Your settings configure debug/verbose output to Serial, but the option USB CDC On Boot is Disabled, Enable it and remember to update ALLOW_CONCURRENT_CDC_AND_HID"
#endif


#if (ARDUINO_USB_CDC_ON_BOOT == 1)
#include "tusb.h"
// Descriptor Helper
static uint16_t _desc_str_buf[32];
static const uint16_t* make_str_desc(const char* s) {
  size_t len = strlen(s);
  if (len > 30) len = 30;
  // bDescriptorType=STRING (0x03), bLength = 2 + 2*len
  _desc_str_buf[0] = (TUSB_DESC_STRING << 8) | (uint16_t)(2 * len + 2);
  for (size_t i = 0; i < len; i++) {
    _desc_str_buf[1 + i] = (uint16_t)s[i];
  }
  return _desc_str_buf;
}

// Override the weak TinyUSB string callback to fix ESP32 Core not setting correct device names when using composite devices (e.g CDC+HID)
extern "C" {
  const uint16_t* tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    switch (index) {
      case 0: {
        static const uint16_t lang_desc[] = { (TUSB_DESC_STRING << 8) | 4, USB_LANG_ID };
        return lang_desc;
      }
      case 1: return make_str_desc(USB_MANUFACTURER);
      case 2: return make_str_desc(USB_SERIAL);
      case 3: return make_str_desc(USB_SERIAL);
      case 4: return make_str_desc(USB_SERIAL); // HID name
      case 5: return make_str_desc(USB_SERIAL); // CDC name
      case 6: return make_str_desc(USB_SERIAL); // Device name
      default: return nullptr;
    }
  }
}
#endif

// HID Report spacing & control
static uint32_t lastHidSendUs = 0;
static uint8_t lastSentReport[sizeof(report.raw)] = {0};
static bool reportPending = false;
static_assert(sizeof(report.raw) == sizeof(lastSentReport), "Report size mismatch!");

USBHID HID;

GamepadReport_t report = {0};
GPDevice gamepad; // Load the class

/*
// Wats the max number of groups?
constexpr size_t maxUsedGroup = []() {
    uint16_t max = 0;
    for (size_t i = 0; i < sizeof(InputMappings)/sizeof(InputMappings[0]); ++i) {
        if (InputMappings[i].group > max) max = InputMappings[i].group;
    }
    return max;
}();
static_assert(maxUsedGroup < MAX_GROUPS, "❌ Too many unique selector groups — increase MAX_GROUPS in Config.h");
*/

// Wats the max number of groups?
size_t getMaxUsedGroup() {
    uint16_t max = 0;
    for (size_t i = 0; i < sizeof(InputMappings) / sizeof(InputMappings[0]); ++i) {
        if (InputMappings[i].group > max) max = InputMappings[i].group;
    }
    return max;
}

// MAX groups in bitmasks
static uint32_t groupBitmask[MAX_GROUPS] = { 0 };

// Used for Axis stabilization and filtering (and reset for initialization)
static int lastFiltered[40] = { 0 };
static int lastOutput[40] = { -1 };
static unsigned int stabCount[40] = { 0 };
static bool stabilized[40] = { false };

// Build HID group bitmasks
void buildHIDGroupBitmasks() {
  for (size_t i = 0; i < InputMappingSize; ++i) {
      const InputMapping& m = InputMappings[i];
      if (m.group > 0 && m.hidId > 0) {
          groupBitmask[m.group] |= (1UL << (m.hidId - 1));
      }
  }
}

// Find the HID mapping whose oride_label and oride_value match this DCS command+value (for selectors).
static const InputMapping* findHidMappingByDcs(const char* dcsLabel, uint16_t value) {
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (m.oride_label
            && strcmp(m.oride_label, dcsLabel) == 0
            && (uint16_t)m.oride_value == value) {
            return &m;
        }
    }
    return nullptr;
}

void flushBufferedHidCommands() {
    auto history = dcsbios_getCommandHistory();
    size_t n     = dcsbios_getCommandHistorySize();
    unsigned long now = millis();

    CommandHistoryEntry* groupLatest[MAX_GROUPS] = { nullptr };

    // Step 1: Track most recent pending entry per group
    for (size_t i = 0; i < n; ++i) {
        auto& e = history[i];
        if (!e.hasPending || e.group == 0) continue;

        if (now - e.lastChangeTime >= SELECTOR_DWELL_MS) {
            uint16_t g = e.group;

            if (g >= MAX_GROUPS) {
              debugPrintf("❌ FATAL: group ID %u exceeds MAX_GROUPS (%u). Halting flush.\n", g, MAX_GROUPS);
              abort();  // Triggers clean panic for debugging
            }

            if (!groupLatest[g] || e.lastChangeTime > groupLatest[g]->lastChangeTime) {
                groupLatest[g] = &e;
            }
        }
    }
    for (uint16_t g = 1; g < MAX_GROUPS; ++g) {
        CommandHistoryEntry* winner = groupLatest[g];
        if (!winner) continue;

        // Always clear all bits in this group first
        report.buttons &= ~groupBitmask[g];

        /*
        const InputMapping* m = findHidMappingByDcs(winner->label, winner->pendingValue);
        // **NEW: Set winner's HID bit for ANY value (including 0)**
        if (m) {
            report.buttons |= (1UL << (m->hidId - 1));
        }
        */

        const InputMapping* match = nullptr;

        for (size_t i = 0; i < InputMappingSize; ++i) {
            const InputMapping& mapping = InputMappings[i];
            if (mapping.group == g && mapping.oride_value == winner->pendingValue
                && mapping.hidId > 0 && mapping.hidId <= 32) {
                report.buttons |= (1UL << (mapping.hidId - 1));
                match = &mapping;  // Keep pointer for debug
                break;
            }
            else if (mapping.group == g && mapping.oride_value == winner->pendingValue) {
                debugPrintf("❌ [HID] INVALID HID ID %d for %s (group=%u value=%u)\n", mapping.hidId, mapping.label, g, winner->pendingValue);
            }
        }

        HIDManager_dispatchReport(false);

        winner->lastValue = winner->pendingValue;
        winner->lastSendTime = now;
        winner->hasPending = false;
        
        debugPrintf("🛩️ [HID] GROUP %u FLUSHED: %s = %u (HID=%d)\n",g, winner->label, winner->lastValue, match ? match->hidId : -1);
        // debugPrintf("🛩️ [HID] GROUP %u FLUSHED: %s = %u (HID=%d)\n", g, winner->label, winner->lastValue, m ? m->hidId : -1);
    }
    if (reportPending) HIDManager_dispatchReport(false);
}

// Replace your current HIDManager_sendReport(...) with this:
void HIDManager_sendReport(const char* label, int32_t rawValue) {
    const InputMapping* m = findInputByLabel(label);
    if (!m) {
        debugPrintf("⚠️ [HID] %s UNKNOWN\n", label);
        return;
    }

    const char* dcsLabel = m->oride_label;
    uint16_t    dcsValue = rawValue < 0 ? 0 : (uint16_t)rawValue;

    // look up shared history
    CommandHistoryEntry* history = dcsbios_getCommandHistory();
    size_t n = dcsbios_getCommandHistorySize();
    CommandHistoryEntry* e = nullptr;
    for (size_t i = 0; i < n; ++i) {
        if (strcmp(history[i].label, dcsLabel) == 0) {
            e = &history[i];
            break;
        }
    }
    if (!e) {
        debugPrintf("⚠️ [HID] %s → no DCS entry (%s)\n", label, dcsLabel);
        return;
    }

    #if defined(SELECTOR_DWELL_MS) && (SELECTOR_DWELL_MS > 0)
    // buffer selectors
    if (e->group > 0) {
        e->pendingValue   = dcsValue;
        e->lastChangeTime = millis();
        e->hasPending     = true;
        // debugPrintf("🔁 [DCS] Buffer Selection for GroupID: %u - %s %u\n", e->group, label, e->pendingValue);
        return;
    }
    #endif

    // same throttle as DCS
    if (!applyThrottle(*e, dcsLabel, dcsValue, false))
        return;

    // flip just this bit
    uint32_t mask = (1UL << (m->hidId - 1));
    if (dcsValue)
        report.buttons |= mask;
    else
        report.buttons &= ~mask;

    HIDManager_dispatchReport(false);

    // update history
    e->lastValue    = dcsValue;
    e->lastSendTime = millis();
    debugPrintf("🛩️ [HID] %s = %u\n", dcsLabel, dcsValue);
}

void HIDManager_resetAllAxes() {
    for (int i = 0; i < 40; ++i) {
        stabCount[i] = 0;
        stabilized[i] = false;
        lastOutput[i] = -1;
    }
}

// Commit Deferred HID report for an entire panel
void HIDManager_commitDeferredReport(const char* deviceName) {

      // Skip is we are in DCS-MODE
    if (isModeSelectorDCS()) return;

  #if !USE_DCSBIOS_WIFI
    if (!cdcEnsureRxReady(CDC_TIMEOUT_RX_TX) || !cdcEnsureTxReady(CDC_TIMEOUT_RX_TX)) {
        debugPrintln("❌ [HID] No stream active yet or Tx buffer full");
        return;
    }
  #endif    

    // 2) Send the raw 64-byte gamepad report
    HIDManager_dispatchReport(false);

    // 3) (Optional) Debug
    debugPrintf("🛩️ [HID] Deferred report sent for: \"%s\"\n", deviceName);
}

// For polling rate on panels that need it
bool shouldPollMs(unsigned long &lastPoll) {
  const unsigned long pollingIntervalMs = 1000 / POLLING_RATE_HZ;
  unsigned long now = millis();
  if (now - lastPoll < pollingIntervalMs) return false;
  lastPoll = now;
  return true;
}

// USB Events
void onUsbStarted(void* arg, esp_event_base_t base, int32_t event_id, void* data) {
  debugPrintln("🔌 USB Started");
}

void onUsbStopped(void* arg, esp_event_base_t base, int32_t event_id, void* data) {
  debugPrintln("❌ USB Stopped");
}

void onUsbSuspended(void* arg, esp_event_base_t base, int32_t event_id, void* data) {
  debugPrintln("💤 USB Suspended");
}

void onUsbResumed(void* arg, esp_event_base_t base, int32_t event_id, void* data) {
  debugPrintln("🔁 USB Resumed");
}

void setupUSBEvents() {
    USB.onEvent(ARDUINO_USB_STARTED_EVENT, onUsbStarted);
    USB.onEvent(ARDUINO_USB_STOPPED_EVENT, onUsbStopped);
    USB.onEvent(ARDUINO_USB_SUSPEND_EVENT, onUsbSuspended);
    USB.onEvent(ARDUINO_USB_RESUME_EVENT, onUsbResumed);
}

void HIDManager_dispatchReport(bool force) {

// Only situation in which we are allowed to send HID reports while in DCS mode is when USE_DCSBIOS_USB is enabled, so we skip this check.
#if USE_DCSBIOS_USB
    if (HID.ready()) {
        gamepad.sendReport(report.raw, sizeof(report.raw)); // Already using HID_SENDREPORT_TIMEOUT
    }
    else {
		debugPrintln("❌ [HID] Not ready, cannot send HID report.");
    }
#else
    // DCS mode disables HID entirely
    if (isModeSelectorDCS()) return;

    uint32_t now = micros();

    if (force) {
        // REGULAR INTERVAL: Enforce minimum interval
        if ((now - lastHidSendUs) < HID_REPORT_MIN_INTERVAL_US) return;
        // Else, proceed and update last send time below
    }

    // Skip if report is identical and not forced
    if (!force && memcmp(lastSentReport, report.raw, sizeof(report.raw)) == 0) return;

    // We also check for HID_can_send_report to check for special Rx ready condition (ESP32 Arduino Core bug workaround!) 
    if(HID.ready())
      gamepad.sendReport(report.raw, sizeof(report.raw)); // Already using HID_SENDREPORT_TIMEOUT

    // Mark success
    memcpy(lastSentReport, report.raw, sizeof(report.raw));
    lastHidSendUs = now;
#endif    
}

void HIDManager_moveAxis(const char* dcsIdentifier,
    uint8_t      pin,
    HIDAxis      axis,
    bool         forceSend)
{

    constexpr int DEADZONE_LOW = 512;
    constexpr int DEADZONE_HIGH = 8000;
    constexpr int THRESHOLD = 128;
    constexpr int SMOOTHING_FACTOR = 8;
    constexpr int STABILIZATION_CYCLES = 10;
    constexpr int HID_MAX = 8191;

    // 1) Read & smooth
    int raw = analogRead(pin);
    if (stabCount[pin] == 0) {
        lastFiltered[pin] = raw;
    }
    else {
        lastFiltered[pin] = (lastFiltered[pin] * (SMOOTHING_FACTOR - 1) + raw) / SMOOTHING_FACTOR;
    }
    int filtered = lastFiltered[pin];
    if (filtered < DEADZONE_LOW)  filtered = 0;
    if (filtered > DEADZONE_HIGH) filtered = HID_MAX;

    if (forceSend) {
        stabCount[pin] = STABILIZATION_CYCLES;
        stabilized[pin] = true;
        lastOutput[pin] = filtered;
        uint16_t dcsValue = map(filtered, 0, HID_MAX, 0, 65535);

        if (isModeSelectorDCS()) {
            auto* e = findCmdEntry(dcsIdentifier);
            bool force = forcePanelSyncThisMission;
            if (e && applyThrottle(*e, dcsIdentifier, dcsValue, force)) {
                sendDCSBIOSCommand(dcsIdentifier, dcsValue, force);
                e->lastValue = dcsValue;
                e->lastSendTime = millis();
            }
        }
        else {
            if (axis < HID_AXIS_COUNT) {
                report.axes[axis] = filtered;
            }
            auto* e = findCmdEntry(dcsIdentifier);
            if (e && applyThrottle(*e, dcsIdentifier, dcsValue, false)) {
                HIDManager_dispatchReport(false);
                e->lastValue = dcsValue;
                e->lastSendTime = millis();
                debugPrintf("🛩️ [HID] Axis(%u) %s = %u [FORCE]\n", axis, dcsIdentifier, dcsValue);
            }
        }
        return;
    }


    // 2) Stabilization
    if (!stabilized[pin]) {
        stabCount[pin]++;
        if (stabCount[pin] >= STABILIZATION_CYCLES) {
            stabilized[pin] = true;
            lastOutput[pin] = filtered;
            uint16_t dcsValue = map(filtered, 0, HID_MAX, 0, 65535);

            if (isModeSelectorDCS()) {
                auto* e = findCmdEntry(dcsIdentifier);
                bool force = forcePanelSyncThisMission;
                if (e && applyThrottle(*e, dcsIdentifier, dcsValue, force)) {
                    sendDCSBIOSCommand(dcsIdentifier, dcsValue, force);
                    e->lastValue = dcsValue;
                    e->lastSendTime = millis();
                }
            }
            else {
                if (axis < HID_AXIS_COUNT) {
                    report.axes[axis] = filtered;
                }
                auto* e = findCmdEntry(dcsIdentifier);
                if (e && applyThrottle(*e, dcsIdentifier, dcsValue, false)) {
                    HIDManager_dispatchReport(false);
                    e->lastValue = dcsValue;
                    e->lastSendTime = millis();
                    debugPrintf("🛩️ [HID] Axis(%u) %s = %u [INITIAL]\n", axis, dcsIdentifier, dcsValue);
                }
            }
            return;
        }
        return;
    }

    // 3) Threshold check
    if (abs(filtered - lastOutput[pin]) <= THRESHOLD) {
        return;
    }
    lastOutput[pin] = filtered;

    // 4) Map and send
    uint16_t dcsValue = map(filtered, 0, HID_MAX, 0, 65535);

    if (isModeSelectorDCS()) {
        auto* e = findCmdEntry(dcsIdentifier);
        bool force = forcePanelSyncThisMission;
        if (e && applyThrottle(*e, dcsIdentifier, dcsValue, force)) {
            sendDCSBIOSCommand(dcsIdentifier, dcsValue, force);
            e->lastValue = dcsValue;
            e->lastSendTime = millis();
        }
    }
    else {
        if (axis < HID_AXIS_COUNT) {
            report.axes[axis] = filtered;
        }
        auto* e = findCmdEntry(dcsIdentifier);
        if (e && applyThrottle(*e, dcsIdentifier, dcsValue, false)) {
            HIDManager_dispatchReport(false);
            e->lastValue = dcsValue;
            e->lastSendTime = millis();
            debugPrintf("🛩️ [HID] Axis(%u) %s = %u\n", axis, dcsIdentifier, dcsValue);
        }
    }
}

/*
void HIDManager_moveAxis(const char* dcsIdentifier,
    uint8_t      pin,
    HIDAxis      axis) {
    constexpr int DEADZONE_LOW = 512;
    constexpr int DEADZONE_HIGH = 8000;
    constexpr int THRESHOLD = 128;
    constexpr int SMOOTHING_FACTOR = 8;
    constexpr int STABILIZATION_CYCLES = 10;
    constexpr int HID_MAX = 8191;

    // 1) Read & smooth
    int raw = analogRead(pin);
    if (stabCount[pin] == 0) {
        lastFiltered[pin] = raw;
    }
    else {
        lastFiltered[pin] = (lastFiltered[pin] * (SMOOTHING_FACTOR - 1) + raw) / SMOOTHING_FACTOR;
    }
    int filtered = lastFiltered[pin];
    if (filtered < DEADZONE_LOW)  filtered = 0;
    if (filtered > DEADZONE_HIGH) filtered = HID_MAX;

    // 2) Stabilization
    if (!stabilized[pin]) {
        stabCount[pin]++;
        if (stabCount[pin] >= STABILIZATION_CYCLES) {
            stabilized[pin] = true;
            lastOutput[pin] = filtered;
            uint16_t dcsValue = map(filtered, 0, HID_MAX, 0, 65535);

            if (isModeSelectorDCS()) {
                auto* e = findCmdEntry(dcsIdentifier);
                bool force = forcePanelSyncThisMission;
                if (e && applyThrottle(*e, dcsIdentifier, dcsValue, force)) {
                    sendDCSBIOSCommand(dcsIdentifier, dcsValue, force);
                    e->lastValue = dcsValue;
                    e->lastSendTime = millis();
                }
            }
            else {
                if (axis < HID_AXIS_COUNT) {
                    report.axes[axis] = filtered;
                }
                auto* e = findCmdEntry(dcsIdentifier);
                if (e && applyThrottle(*e, dcsIdentifier, dcsValue, false)) {
                    HIDManager_dispatchReport(false);
                    e->lastValue = dcsValue;
                    e->lastSendTime = millis();
                    debugPrintf("🛩️ [HID] Axis(%u) %s = %u [INITIAL]\n", axis, dcsIdentifier, dcsValue);
                }
            }
            return;
        }
        return;
    }

    // 3) Threshold check
    if (abs(filtered - lastOutput[pin]) <= THRESHOLD) {
        return;
    }
    lastOutput[pin] = filtered;

    // 4) Map and send
    uint16_t dcsValue = map(filtered, 0, HID_MAX, 0, 65535);

    if (isModeSelectorDCS()) {
        auto* e = findCmdEntry(dcsIdentifier);
        bool force = forcePanelSyncThisMission;
        if (e && applyThrottle(*e, dcsIdentifier, dcsValue, force)) {
            sendDCSBIOSCommand(dcsIdentifier, dcsValue, force);
            e->lastValue = dcsValue;
            e->lastSendTime = millis();
        }
    }
    else {
        if (axis < HID_AXIS_COUNT) {
            report.axes[axis] = filtered;
        }
        auto* e = findCmdEntry(dcsIdentifier);
        if (e && applyThrottle(*e, dcsIdentifier, dcsValue, false)) {
            HIDManager_dispatchReport(false);
            e->lastValue = dcsValue;
            e->lastSendTime = millis();
            debugPrintf("🛩️ [HID] Axis(%u) %s = %u\n", axis, dcsIdentifier, dcsValue);
        }
    }
}
*/

void HIDManager_toggleIfPressed(bool isPressed, const char* label, bool deferSend) {
  
    CommandHistoryEntry* e = findCmdEntry(label);
    if (!e) return;

    static std::array<bool, MAX_TRACKED_RECORDS> lastStates = {false};
    int index = e - dcsbios_getCommandHistory();
    if (index < 0 || index >= MAX_TRACKED_RECORDS) return;

    bool prev = lastStates[index];
    lastStates[index] = isPressed;

    if (isPressed && !prev) {
        HIDManager_setToggleNamedButton(label, deferSend);
    }
}

void HIDManager_setToggleNamedButton(const char* name, bool deferSend) {
  const char* label = name;
  const InputMapping* m = findInputByLabel(label);
  if (!m) {
    debugPrintf("⚠️ [HIDManager] %s UNKNOWN (toggle)\n", label);
    return;
  }

  CommandHistoryEntry* e = findCmdEntry(label);
  if (!e) return;
  bool newState = !(e->lastValue > 0);
  e->lastValue = newState ? 1 : 0;

  if (isModeSelectorDCS()) {
    if (m->oride_label && m->oride_value >= 0) {
        bool force = forcePanelSyncThisMission;
        sendDCSBIOSCommand(m->oride_label, newState ? m->oride_value : 0, force);
    }
    return;
  }

  if (m->hidId <= 0) return;

  uint32_t mask = (1UL << (m->hidId - 1));

  if (m->group > 0 && newState)
    report.buttons &= ~groupBitmask[m->group];

  if (newState)
    report.buttons |= mask;
  else
    report.buttons &= ~mask;

  if (!deferSend) {
    HIDManager_sendReport(name, newState ? 1 : 0);
  }
}

void HIDManager_setNamedButton(const char* name, bool deferSend, bool pressed) {
  const InputMapping* m = findInputByLabel(name);
  if (!m) {
    debugPrintf("⚠️ [HIDManager] %s UNKNOWN\n", name);
    return;
  }

  if (isModeSelectorDCS()) {
    if (m->oride_label && m->oride_value >= 0) {
        bool force = forcePanelSyncThisMission;
        sendDCSBIOSCommand(m->oride_label, pressed ? m->oride_value : 0, force);
    }
    return;
  }

  if (m->hidId <= 0) return;

  uint32_t mask = (1UL << (m->hidId - 1));

  if (m->group > 0 && pressed)
    report.buttons &= ~groupBitmask[m->group];

  if (pressed)
    report.buttons |= mask;
  else
    report.buttons &= ~mask;

  // OJO Cambio recient
  if (!deferSend) {
    HIDManager_sendReport(name, pressed ? m->oride_value : 0);
  }
}

// Handles a guarded latching toggle button:
// - First press (with cover closed): opens cover and leaves it open, does NOT latch the button.
// - Next press (with cover open): toggles/latches the button (toggleIfPressed logic).
// - Cover remains open until explicitly closed.
void HIDManager_handleGuardedToggleIfPressed(bool isPressed, const char* buttonLabel, const char* coverLabel, bool deferSend) {
    CommandHistoryEntry* e = findCmdEntry(buttonLabel);
    if (!e) return;

    static std::array<bool, MAX_TRACKED_RECORDS> lastStates = {false};
    int index = e - dcsbios_getCommandHistory();
    if (index < 0 || index >= MAX_TRACKED_RECORDS) return;

    bool wasPressed = lastStates[index];
    lastStates[index] = isPressed;

    if (isPressed && !wasPressed) {
        if (!isCoverOpen(coverLabel)) {
            HIDManager_setToggleNamedButton(coverLabel, deferSend); // Open and leave open
            debugPrintf("✅ Cover [%s] opened for [%s]\n", coverLabel, buttonLabel);
            return; // Do not latch button yet—require a new press
        }
        HIDManager_setToggleNamedButton(buttonLabel, deferSend); // Now toggle/latch the button
    }
}

void HID_keepAlive() {
    static unsigned long lastHeartbeat = 0;
    unsigned long now = millis();

    if (now - lastHeartbeat >= HID_KEEP_ALIVE_MS) {
        lastHeartbeat = now;
        HIDManager_dispatchReport(true);
    }
}

void HIDManager_setup() {

    // Load our Group Bitmasks
    buildHIDGroupBitmasks();    

    // Setup USB events
    setupUSBEvents();

    // Start HID Device
    HID.begin();

    // Start the interface
    USB.begin();  
    delay(3000);
}

void HIDManager_loop() {

    // Optional
    #if HID_KEEP_ALIVE_ENABLED
    if (!isModeSelectorDCS()) HID_keepAlive();  // Called only when in HID mode and if HID_KEEP_ALIVE_ENABLED    
    #endif    

    #if defined(SELECTOR_DWELL_MS) && (SELECTOR_DWELL_MS > 0)
    // In HID mode, flush any buffered selector-group presses
    if (!isModeSelectorDCS()) flushBufferedHidCommands();
    #endif

}