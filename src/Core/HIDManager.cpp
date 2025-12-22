// HIDManager.cpp

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

// DCSBIOS serial handling
#if !defined(ARDUINO_USB_CDC_ON_BOOT) || ARDUINO_USB_CDC_ON_BOOT == 0 // if CDC is off on boot
    #if (ARDUINO_USB_MODE == 0) // and we are in TinyUSB mode
        #if (USE_DCSBIOS_SERIAL || VERBOSE_MODE_SERIAL_ONLY || VERBOSE_MODE) // And some form of serial was enabled, then
            #include "USB.h"
            USBCDC USBSerial;
            #define LOADED_CDC_STACK 1 // Mark that we have a real HID stack loaded
        #else // No serial needed, but we still need to define the HWCDC object to compile (and close HWCDCSerial if opened by default)
            #if DEVICE_HAS_HWSERIAL == 1
                #include "HWCDC.h"
                #define CLOSE_HWCDC_SERIAL 1 // Mark that we need to close the HWserial port
            #endif
        #endif
    #elif (ARDUINO_USB_MODE == 1)
        #if (USE_DCSBIOS_SERIAL || VERBOSE_MODE_SERIAL_ONLY || VERBOSE_MODE) // Enable only if Serial is needed
            #include "HWCDC.h"
            HWCDC HWCDCSerial;  // not aliased to Serial when OFF_ON_BOOT
        #else
            #include "HWCDC.h"
            // HWCDC HWCDCSerial;  // not aliased to Serial when OFF_ON_BOOT
            #define CLOSE_HWCDC_SERIAL 1 // Mark that we need to close the HWserial port
        #endif
    #endif
#else // CDC is ON on boot, so Serial is already aliased to CDC
    #if (!USE_DCSBIOS_SERIAL && !VERBOSE_MODE_SERIAL_ONLY && !VERBOSE_MODE) // If serial is NOT needed, mark to close it later
        #if ARDUINO_USB_MODE == 1
            // HWCDC Serial;
            #define CLOSE_HWCDC_SERIAL 1 // Mark that we need to close the HW serial port
        #else   
            // USBCDC Serial;
            #define CLOSE_CDC_SERIAL 1 // Mark that we need to close the CDC serial port
        #endif
    #endif
#endif

// ----- USB build: real stack for S2 and S3 only -----
#if !USE_DCSBIOS_WIFI && !USE_DCSBIOS_BLUETOOTH // No WiFi or Bluetooth, so either USB or Serial only selected
    #if (ESP_FAMILY_S2 || ESP_FAMILY_S3 || ESP_FAMILY_P4) && ARDUINO_USB_MODE == 0  
        #if !defined(LOADED_CDC_STACK)
            #include "USB.h"
        #endif
        #include "../HIDDescriptors.h"   // provides extern decls
        USBHID HID;
        #define LOADED_USB_STACK 1 // Mark that we have a real HID stack loaded
    #else    
        // Stubs only as we don't have USB stack but need the types to compile
        #include "../CustomDescriptors/BidireccionalNew.h"   // defines GamepadReport_t and hidReportDesc
        struct { bool ready() const { return false; } } HID;
        #if !defined(LOADED_CDC_STACK) && !defined(LOADED_USB_STACK)
            struct { void begin() {} template<typename...A> void onEvent(A...) {} } USB;
        #endif
        class GPDevice { public: bool sendReport(const void*, int) { return false; } };
    #endif
#else // WiFi or Bluetooth selected, so no USB stack (But if ARDUINO_USB_MODE==0 we still need stubs except for TinyUSB CDC)
        // Stubs only as we don't have USB stack but need the types to compile
        #include "../CustomDescriptors/BidireccionalNew.h"   // defines GamepadReport_t and hidReportDesc
        struct { bool ready() const { return false; } } HID;
        #if !defined(LOADED_CDC_STACK) && !defined(LOADED_USB_STACK)
            #if CLOSE_CDC_SERIAL
		        // No stub for USB as we need to close the serial port AND USB is already defined above
            #else 
                struct { void begin() {} template<typename...A> void onEvent(A...) {} } USB;
            #endif
        #endif
        class GPDevice { public: bool sendReport(const void*, int) { return false; } };
#endif

#if USE_DCSBIOS_BLUETOOTH
#include "../BLEManager.h"
#endif

// === Self-learning axis calibration ===
static uint16_t axMin[HID_AXIS_COUNT];
static uint16_t axMax[HID_AXIS_COUNT];

static void axCalibInit() {
    for (uint8_t i = 0; i < HID_AXIS_COUNT; i++) {
        axMin[i] = 4095;  // Start high, any reading will be lower
        axMax[i] = 0;     // Start low, any reading will be higher
    }
}

static inline int axScale(int v, HIDAxis ax) {
    // Learn extremes (monotonic expansion)
    if (v < (int)axMin[ax]) axMin[ax] = v;
    if (v > (int)axMax[ax]) axMax[ax] = v;

    const int span = axMax[ax] - axMin[ax];
    if (span < 256) return v;              // Not calibrated yet

    // Scale to full range
    int out;
    if (v <= (int)axMin[ax]) out = 0;
    else if (v >= (int)axMax[ax]) out = 4095;
    else out = (v - axMin[ax]) * 4095 / span;

    // Sticky zones at extremes (latching for noisy axes)
    if (out > 0 && out <= LOWER_AXIS_THRESHOLD) out = 0;
    if (out < 4095 && out >= (4095 - UPPER_AXIS_THRESHOLD)) out = 4095;

    return out;
}

// Should we load USB events?
bool loadUSBevents = false;
bool loadCDCevents = false;

// Serial close flags based on CLOSE_CDC_SERIAL or CLOSE_HWCDC_SERIAL
bool closeCDCserial = false;
bool closeHWCDCserial = false;

// HID Report and device instance
GamepadReport_t report = { 0 };  // define the extern
GPDevice        gamepad;         // define the extern

// --- HID step pulse auto-clear ---
#ifndef STEP_PULSE_MS
#define STEP_PULSE_MS 250u    // HID pulse length for step controls
#endif
static uint32_t g_hidStepPulseMask = 0;        // bits (hidId-1) pending auto-clear
static uint32_t g_hidStepPulseDueMs[33] = { 0 }; // index 1..32 by hidId

// USB String Descriptors override for TinyUSB stack (S2/S3)
#if LOADED_USB_STACK
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
static bool reportPending = false;

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

    // Gate at function entry
    if (isModeSelectorDCS()) return;         // Exclusive: skip when in DCS mode

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

    // Step 3: Send any non-grouped commands (buttons, axes)
    for (size_t i = 0; i < n; ++i) {
        auto& e = history[i];
        if (!e.hasPending || e.group != 0) continue;
        // Find the mapping for this label (must have valid hidId)
        const InputMapping* m = findInputByLabel(e.label);
        if (!m || m->hidId <= 0 || m->hidId > 32) continue; // Skip if no HID mapping or invalid HID id

        uint32_t mask = (1UL << (m->hidId - 1));
        if (e.pendingValue)
            report.buttons |= mask;
        else
            report.buttons &= ~mask;

        HIDManager_dispatchReport(false);

        e.lastValue = e.pendingValue;
        e.lastSendTime = now;
        e.hasPending = false;
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
    
	// Re-initialize axis calibration
    axCalibInit();

    for (int i = 0; i < 40; ++i) {
        stabCount[i] = 0;
        stabilized[i] = false;
        lastOutput[i] = -1;
    }
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
#if LOADED_USB_STACK
    void onUsbStarted(void*, esp_event_base_t, int32_t, void*) { debugPrintln("🔌 USB Started"); }
    void onUsbStopped(void*, esp_event_base_t, int32_t, void*) { debugPrintln("❌ USB Stopped"); }
    void onUsbSuspended(void*, esp_event_base_t, int32_t, void*) { debugPrintln("💤 USB Suspended"); }
    void onUsbResumed(void*, esp_event_base_t, int32_t, void*) { debugPrintln("🔁 USB Resumed"); }

    static inline void setupUSBEvents() {
        USB.onEvent(ARDUINO_USB_STARTED_EVENT, onUsbStarted);
        USB.onEvent(ARDUINO_USB_STOPPED_EVENT, onUsbStopped);
        USB.onEvent(ARDUINO_USB_SUSPEND_EVENT, onUsbSuspended);
        USB.onEvent(ARDUINO_USB_RESUME_EVENT, onUsbResumed);
		debugPrint("USB Events registered\n");
    }
#else
    static inline void setupUSBEvents() {}
#endif

static inline void HID_dbgDumpHistory(const char* label, const char* where) {
    CommandHistoryEntry* e = findCmdEntry(label);
    if (!e) { debugPrintf("[HIST] %s @%s  <untracked>\n", label, where); return; }
    debugPrintf("[HIST] %s @%s  last=0x%04X known=%u isSel=%u grp=%u pend=%u pendVal=%u tChange=%lu tSend=%lu\n",
        label, where,
        (unsigned)e->lastValue,
        (unsigned)(e->lastValue != 0xFFFF),
        (unsigned)e->isSelector,
        (unsigned)e->group,
        (unsigned)e->hasPending,
        (unsigned)e->pendingValue,
        (unsigned long)e->lastChangeTime,
        (unsigned long)e->lastSendTime);
}

void HIDManager_dispatchReport(bool force) {

    const uint32_t now = micros();

#if USE_DCSBIOS_USB
    // USB transport: always send to trigger host drain 
    if (HID.ready()) {
        gamepad.sendReport(report.raw, sizeof(report.raw));
    }
    else {
        debugPrintln("❌ [HID] Not ready, cannot send HID report.");
    }
    return;
#endif

#if USE_DCSBIOS_BLUETOOTH
    // BLE transport selected
    if (isModeSelectorDCS()) {
        // DCS mode → always send to provoke GET_FEATURE drain
        BLEManager_send(report.raw, sizeof(report.raw));
        return;
    }
    // HID mode over BLE → apply same gates as your non-USB branch
    static uint32_t lastSendUs = 0;
    static uint8_t  lastSent[sizeof(report.raw)] = { 0 };

    if (force) {
        if ((now - lastSendUs) < HID_REPORT_MIN_INTERVAL_US) return;
    }
    else {
        if (memcmp(lastSent, report.raw, sizeof(report.raw)) == 0) return;
        if ((now - lastSendUs) < HID_REPORT_MIN_INTERVAL_US) return;
    }

    BLEManager_send(report.raw, sizeof(report.raw));
    memcpy(lastSent, report.raw, sizeof(report.raw));
    lastSendUs = now;
    return;
#endif

    // Fallback: pure HID mode (no USB, no BLE transport) — your existing gates
    if (isModeSelectorDCS()) return;

    static uint32_t lastHidSendUs = 0;
    static uint8_t  lastSentReport[sizeof(report.raw)] = { 0 };

    if (force) {
        if ((now - lastHidSendUs) < HID_REPORT_MIN_INTERVAL_US) return;
    }
    else {
        if (memcmp(lastSentReport, report.raw, sizeof(report.raw)) == 0) return;
        if ((now - lastHidSendUs) < HID_REPORT_MIN_INTERVAL_US) return;
    }

    if (HID.ready())
        gamepad.sendReport(report.raw, sizeof(report.raw));

    memcpy(lastSentReport, report.raw, sizeof(report.raw));
    lastHidSendUs = now;
}

void HIDManager_moveAxis(const char* dcsIdentifier, uint8_t pin, HIDAxis axis, bool forceSend, bool deferSend) {

	// deferSend is not implemented yet. forceSend is used instead (during init or panel sync)

    // --- constants ---
    constexpr int HID_MAX = 4095;
    constexpr int SMOOTHING_FACTOR = 4;
    constexpr int STABILIZATION_CYCLES = 8;

    const bool inDcsMode = isModeSelectorDCS();

#if SEND_HID_AXES_IN_DCS_MODE
    const bool hybridEnabled = true;
#else
	const bool hybridEnabled = false;
#endif

#if SKIP_ANALOG_FILTERING
    if (!inDcsMode) {                    // HID-only raw path
        int v = analogRead(pin);         // 0..4095
        if (axisInverted(axis)) v = HID_MAX - v;
        if (axis < HID_AXIS_COUNT) report.axes[axis] = (uint16_t)v;
        HIDManager_dispatchReport(forceSend);
        return;
    }
#endif

    // --- per-pin state ---
    static bool g_bootstrapped[64] = { false };

    auto sendHID = [&](int value, bool force) {
        if (axis < HID_AXIS_COUNT) report.axes[axis] = (uint16_t)value;
        HIDManager_dispatchReport(force);
    };
    auto sendDCS = [&](uint16_t dcsValue, bool force) {
        auto* e = findCmdEntry(dcsIdentifier);
        if (e && applyThrottle(*e, dcsIdentifier, dcsValue, force)) {
            sendDCSBIOSCommand(dcsIdentifier, dcsValue, force);
            e->lastValue = dcsValue;
            e->lastSendTime = millis();
        }
    };

    // --- read & EMA ---
    int raw = analogRead(pin);                 // 0..4095
    if (!g_bootstrapped[pin]) {
        lastFiltered[pin] = raw;
        lastOutput[pin] = raw;
        stabCount[pin] = 0;
        stabilized[pin] = false;
        g_bootstrapped[pin] = true;
    }
    else {
        lastFiltered[pin] =
            (lastFiltered[pin] * (SMOOTHING_FACTOR - 1) + raw) / SMOOTHING_FACTOR;
    }

    int filtered = lastFiltered[pin];
    if (filtered > HID_MAX) filtered = HID_MAX;
    if (axisInverted(axis)) filtered = HID_MAX - filtered;

    /*
    // HID late clamp with fixed windows
    int hid = filtered;
    if (hid > 0 && hid <= LOWER_AXIS_THRESHOLD) hid = 0;
    else if (hid >= (HID_MAX - UPPER_AXIS_THRESHOLD) && hid < HID_MAX) hid = HID_MAX;
    */

	// HID scaling with self-learning calibration
    int hid = axScale(filtered, axis);

    // --- force path (no DCS late-clamp here) ---
    if (forceSend) {
        stabCount[pin] = STABILIZATION_CYCLES;
        stabilized[pin] = true;
        lastOutput[pin] = filtered;

        if (inDcsMode) {
            uint16_t d16 = map(hid, 0, HID_MAX, 0, 65535);
            sendDCS(d16, true);
            if (hybridEnabled) sendHID(hid, true);
        }
        else {
            sendHID(hid, true);
        }
        return;
    }

    // --- stabilization send (no DCS late-clamp here) ---
    if (!stabilized[pin]) {
        if (++stabCount[pin] >= STABILIZATION_CYCLES) {
            stabilized[pin] = true;
            lastOutput[pin] = filtered;

			debugPrintf("🛩️ [HID] Axis %d stabilized at %d (raw=%d)\n", pin, filtered, raw);
            if (inDcsMode) {
                uint16_t d16 = map(hid, 0, HID_MAX, 0, 65535);
                sendDCS(d16, forcePanelSyncThisMission);
                if (hybridEnabled) sendHID(hid, false);
            }
            else {
                sendHID(hid, false);
            }
        }
        return;
    }

    // --- normal update: EARLY RETURN FIRST ---
    if (abs(filtered - lastOutput[pin]) <= MIDDLE_AXIS_THRESHOLD) return;
    lastOutput[pin] = filtered;    

    if (inDcsMode) {
        uint16_t d16 = map(hid, 0, HID_MAX, 0, 65535);
        sendDCS(d16, true);
        if (hybridEnabled) sendHID(hid, false);
    }
    else {
        sendHID(hid, false);
    }
}

void HIDManager_toggleIfPressed(bool isPressed, const char* label, bool deferSend) {
    CommandHistoryEntry* e = findCmdEntry(label); if (!e) return;

    static std::array<bool, MAX_TRACKED_RECORDS> lastStates = { false };
    int index = (int)(e - dcsbios_getCommandHistory());
    if (index < 0 || index >= (int)MAX_TRACKED_RECORDS) return;

    bool prev = lastStates[index];
    lastStates[index] = isPressed;

    if (isPressed && !prev) {
        HIDManager_setToggleNamedButton(label, deferSend);
    }
}

void HIDManager_setToggleNamedButton(const char* name, bool deferSend) {
    const InputMapping* m = findInputByLabel(name);
    if (!m) { debugPrintf("⚠️ [HIDManager] %s UNKNOWN (toggle)\n", name); return; }

    CommandHistoryEntry* e = findCmdEntry(name);
    if (!e) return;

    // Toggle state
    const bool curOn = (e->lastValue != 0xFFFF) && (e->lastValue > 0);
    const bool newOn = !curOn;
    e->lastValue = newOn ? 1 : 0;

    const bool inDcs = isModeSelectorDCS();
    const bool hybridEnabled = false;
    const bool dcsAllowed = inDcs || hybridEnabled;
    const bool hidAllowed = (!inDcs) || hybridEnabled;

    // DCS path
    if (dcsAllowed && m->oride_label && m->oride_value >= 0) {
        sendDCSBIOSCommand(m->oride_label, newOn ? m->oride_value : 0, forcePanelSyncThisMission);
    }

    // HID path
    if (!hidAllowed || m->hidId <= 0) return;
    const uint32_t mask = (1UL << (m->hidId - 1));
    if (m->group > 0 && newOn) report.buttons &= ~groupBitmask[m->group];
    if (newOn) report.buttons |= mask; else report.buttons &= ~mask;
    if (!deferSend) HIDManager_dispatchReport(false);
}

void HIDManager_setNamedButton(const char* name, bool deferSend, bool pressed) {
    const InputMapping* m = findInputByLabel(name);
    if (!m) { debugPrintf("⚠️ [HIDManager] %s UNKNOWN\n", name); return; }

    // Regular non-selector buttons
    if (deferSend && m->controlType && strcmp(m->controlType, "momentary") == 0) {
        debugPrintf("⚠️ [HIDManager] Momentary %s ignored during init.\n", name);
        return;
    }

    const bool inDcs = isModeSelectorDCS();
    const bool hybridEnabled = false;
    const bool dcsAllowed = inDcs || hybridEnabled;
    const bool hidAllowed = (!inDcs) || hybridEnabled;

    // HID-only latch handling: only when DCS is NOT allowed
    if (!dcsAllowed && isLatchedButton(name)) {
        HIDManager_toggleIfPressed(pressed, name, deferSend);
        return;
    }

    // -------- DCS path (HYBRID or physical DCS) --------
    if (dcsAllowed) {
        // Gate only DCS with cover logic; HID remains independent
        if (!(CoverGate_intercept(name, pressed) && !deferSend)) {
            const char* ctype = m->controlType ? m->controlType : "";
            const bool isVarStep = (strcmp(ctype, "variable_step") == 0);
            const bool isFixStep = (strcmp(ctype, "fixed_step") == 0);

            if (isVarStep || isFixStep) {
                static char buf[12];
                if (isVarStep) strcpy(buf, pressed ? "+3200" : "-3200");
                else           strcpy(buf, pressed ? "INC" : "DEC");
                sendCommand(m->oride_label, buf, false);
            }
            else if (isLatchedButton(name)) {
                // Rising-edge toggle handles both pipes when DCS is allowed
                HIDManager_toggleIfPressed(pressed, name, deferSend);
                return;
            }
            else if (m->oride_label && m->oride_value >= 0) {
                const bool force = forcePanelSyncThisMission;
                sendDCSBIOSCommand(m->oride_label, pressed ? m->oride_value : 0, force);
            }
        }
        // else: cover handled DCS side; HID may still run below.
    }

    // -------- HID path (HID or HYBRID) --------
    if (!hidAllowed || m->hidId <= 0 || m->hidId > 32) return;

    const uint32_t bit = (1UL << (m->hidId - 1));
    const char* ctype = m->controlType ? m->controlType : "";
    const bool isVarStep = (strcmp(ctype, "variable_step") == 0);
    const bool isFixStep = (strcmp(ctype, "fixed_step") == 0);

    // STEP CONTROLS → two distinct buttons (INC/DEC). Emit ON and schedule auto-OFF.
    if (isVarStep || isFixStep) {
        report.buttons |= bit;
        HIDManager_dispatchReport(false);
        g_hidStepPulseMask |= bit;
        g_hidStepPulseDueMs[m->hidId] = millis() + STEP_PULSE_MS;
        return;
    }

    // SELECTORS → enqueue only on PRESS; never enqueue 0 on RELEASE
    if (m->group > 0) {
        if (pressed) {
            // immediate UI exclusivity, then buffer the target position value
            report.buttons &= ~groupBitmask[m->group];
            report.buttons |= bit;
            HIDManager_sendReport(name, m->oride_value); // buffer value for dwell arbitration
        }
        else {
            // local UI clear only; do not push 0 to history
            report.buttons &= ~bit;
            HIDManager_dispatchReport(false);
        }
        return;
    }

    if (pressed) report.buttons |= bit; else report.buttons &= ~bit;
    if (!deferSend) HIDManager_dispatchReport(false);
}

void HIDManager_commitDeferredReport(const char* deviceName) {
    const bool inDcs = isModeSelectorDCS();
#if defined(MODE_HYBRID_DCS_HID) && (MODE_HYBRID_DCS_HID == 1)
    const bool hidPermitted = true;   // HYBRID: permit HID flush even in DCS
#else
    const bool hidPermitted = !inDcs; // Exclusive modes: HID only when selector says HID
#endif
    if (!hidPermitted) return;

#if !USE_DCSBIOS_WIFI && !USE_DCSBIOS_BLUETOOTH
    if (!cdcEnsureRxReady(CDC_TIMEOUT_RX_TX) || !cdcEnsureTxReady(CDC_TIMEOUT_RX_TX)) {
        debugPrintln("❌ [HID] No stream active yet or Tx buffer full");
        return;
    }
#endif

    HIDManager_dispatchReport(false);
    debugPrintf("🛩️ [HID] Deferred report sent for: \"%s\"\n", deviceName);
}

void HID_keepAlive() {
    static unsigned long lastHeartbeat = 0;
    unsigned long now = millis();

    if (now - lastHeartbeat >= HID_KEEP_ALIVE_MS) {
        lastHeartbeat = now;
        HIDManager_dispatchReport(true);
    }
}

void HIDManager_startUSB() {
    // THis will load only after loadUSBevents = true
    USB.begin();
}

void HIDManager_setup() {

	// Initialize axis calibration
    axCalibInit();

    // Load our Group
    buildHIDGroupBitmasks();

#if LOADED_CDC_STACK
    loadCDCevents = true;
#endif

#if LOADED_USB_STACK
    loadUSBevents = true;
    setupUSBEvents();
    HID.begin();
#endif

    // Set Serial close flags based on CLOSE_CDC_SERIAL or CLOSE_HWCDC_SERIAL like we do for USB   
#if defined(CLOSE_CDC_SERIAL)
    closeCDCserial = true;
#endif

#if defined(CLOSE_HWCDC_SERIAL)
    closeHWCDCserial = true;
#endif
}

void HIDManager_loop() {

#if HID_KEEP_ALIVE_ENABLED
    #if defined(MODE_HYBRID_DCS_HID) && (MODE_HYBRID_DCS_HID == 1)
        HID_keepAlive();
    #else
        if (!isModeSelectorDCS()) HID_keepAlive();
    #endif
#endif

	// Flush buffered HID commands every frame (if any)
    flushBufferedHidCommands();

    // Auto-clear pending HID pulses for variable/fixed_step controls
    if (g_hidStepPulseMask) {
        const unsigned long now = millis();
        uint32_t pending = g_hidStepPulseMask;

        for (uint8_t hid = 1; hid <= 32; ++hid) {
            const uint32_t bit = (1UL << (hid - 1));
            if (!(pending & bit)) continue;
            if ((int32_t)(now - g_hidStepPulseDueMs[hid]) >= 0) {
                report.buttons &= ~bit;
                HIDManager_dispatchReport(false);
                g_hidStepPulseMask &= ~bit;
            }
        }
    }
}