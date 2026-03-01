// HIDManager.cpp

#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

// DCSBIOS serial handling
#if defined(ARDUINO_USB_MODE)
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
#else
	// Do nothing, no USB mode defined
#endif


// ----- USB build: real stack for S2 and S3 only -----
#if !USE_DCSBIOS_WIFI && !USE_DCSBIOS_BLUETOOTH && !RS485_SLAVE_ENABLED // No WiFi, Bluetooth, or RS485 Slave - so either USB or Serial only selected
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
            #if defined(CLOSE_CDC_SERIAL)
		        // No stub for USB as we need to close the serial port AND USB is already defined above
            #else 
                struct { void begin() {} template<typename...A> void onEvent(A...) {} } USB;
            #endif
        #endif
        class GPDevice { public: bool sendReport(const void*, int) { return false; } };
#endif

// --- HID mode requires USB transport (non-negotiable) ---
// Only applies when HID is forced at compile time (MODE_DEFAULT_IS_HID=1).
// HAS_HID_MODE_SELECTOR only means a physical switch exists on the PCB;
// at runtime the user may choose DCS-BIOS mode over any transport.
#if !RS485_SLAVE_ENABLED
  #if defined(MODE_DEFAULT_IS_HID) && (MODE_DEFAULT_IS_HID == 1)
    #if !USE_DCSBIOS_USB
      #error "HID mode requires USE_DCSBIOS_USB=1. MODE_DEFAULT_IS_HID is enabled but transport is not USB."
    #endif
  #endif
#endif

#if USE_DCSBIOS_BLUETOOTH
#include "../BLEManager.h"
#endif

// === NVS Calibration Persistence ===
#include <Preferences.h>

static Preferences calPrefs;
static uint16_t axMinSaved[HID_AXIS_COUNT];
static uint16_t axMaxSaved[HID_AXIS_COUNT];
static bool     axCalibLoaded = false;

// === Self-learning axis calibration ===
static uint16_t axMin[HID_AXIS_COUNT];
static uint16_t axMax[HID_AXIS_COUNT];

// === Center deadzone state (inner/outer hysteresis) ===
static bool axInDeadzone[HID_AXIS_COUNT] = { false };

// Called once at boot (from HIDManager_setup)
static void axCalibLoad() {
    calPrefs.begin("axcal", true);  // read-only

    bool hasData = calPrefs.isKey("min") && calPrefs.isKey("max");

    if (hasData) {
        calPrefs.getBytes("min", axMinSaved, sizeof(axMinSaved));
        calPrefs.getBytes("max", axMaxSaved, sizeof(axMaxSaved));

        // Copy saved values to working arrays
        memcpy(axMin, axMinSaved, sizeof(axMin));
        memcpy(axMax, axMaxSaved, sizeof(axMax));

        debugPrintln("[CAL] Loaded calibration from NVS:");
        for (uint8_t i = 0; i < HID_AXIS_COUNT; i++) {
            if (axMin[i] < 4095 || axMax[i] > 0) {  // Only log axes with real data
                debugPrintf("[CAL]   Axis %u: min=%u, max=%u, span=%u\n",
                    i, axMin[i], axMax[i], axMax[i] - axMin[i]);
            }
        }
    }
    else {
        // No saved data — initialize saved arrays to "nothing learned"
        for (uint8_t i = 0; i < HID_AXIS_COUNT; i++) {
            axMinSaved[i] = 4095;
            axMaxSaved[i] = 0;
        }
        debugPrintln("[CAL] No saved calibration found — starting fresh");
    }

    calPrefs.end();
    axCalibLoaded = true;
}

// Called on mission stop
static void axCalibSave() {
    if (!axCalibLoaded) return;  // Safety: don't save if we never loaded

    // Check each axis: only save if we EXPANDED the range
    bool anyChanged = false;
    uint16_t toSaveMin[HID_AXIS_COUNT];
    uint16_t toSaveMax[HID_AXIS_COUNT];

    for (uint8_t i = 0; i < HID_AXIS_COUNT; i++) {
        // Start with saved values (preserve existing calibration)
        toSaveMin[i] = axMinSaved[i];
        toSaveMax[i] = axMaxSaved[i];

        // Only update if we found a LOWER min (axis was moved further down)
        if (axMin[i] < axMinSaved[i]) {
            toSaveMin[i] = axMin[i];
            anyChanged = true;
            debugPrintf("[CAL] Axis %u: min expanded %u → %u\n", i, axMinSaved[i], axMin[i]);
        }

        // Only update if we found a HIGHER max (axis was moved further up)
        if (axMax[i] > axMaxSaved[i]) {
            toSaveMax[i] = axMax[i];
            anyChanged = true;
            debugPrintf("[CAL] Axis %u: max expanded %u → %u\n", i, axMaxSaved[i], axMax[i]);
        }
    }

    if (!anyChanged) {
        debugPrintln("[CAL] No calibration changes to save");
        return;
    }

    // Write to NVS
    calPrefs.begin("axcal", false);  // read-write
    calPrefs.putBytes("min", toSaveMin, sizeof(toSaveMin));
    calPrefs.putBytes("max", toSaveMax, sizeof(toSaveMax));
    calPrefs.end();

    // Update saved tracking arrays
    memcpy(axMinSaved, toSaveMin, sizeof(axMinSaved));
    memcpy(axMaxSaved, toSaveMax, sizeof(axMaxSaved));

    debugPrintln("[CAL] Calibration saved to NVS");
}

// Called on factory reset / bond wipe — reserved for future use
#if 0   // TODO: wire into a factory-reset command or CLI verb
static void axCalibWipe() {
    calPrefs.begin("axcal", false);
    calPrefs.clear();
    calPrefs.end();

    // Reset all arrays
    for (uint8_t i = 0; i < HID_AXIS_COUNT; i++) {
        axMin[i] = AX_DEFAULT_MIN;
        axMax[i] = AX_DEFAULT_MAX;
        axMinSaved[i] = AX_DEFAULT_MIN;
        axMaxSaved[i] = AX_DEFAULT_MAX;
    }

    debugPrintln("[CAL] Calibration wiped");
}
#endif

// Public wrapper (non-static)
void HIDManager_saveCalibration() {
    axCalibSave();
}

static void axCalibInit() {
    for (uint8_t i = 0; i < HID_AXIS_COUNT; i++) {
        axMin[i] = AX_DEFAULT_MIN;
        axMax[i] = AX_DEFAULT_MAX;
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

    // ─── Center deadzone with inner/outer hysteresis ───
    constexpr int HID_CENTER = 2048;
    const bool inInner = (out >= (HID_CENTER - CENTER_DEADZONE_INNER))
        && (out <= (HID_CENTER + CENTER_DEADZONE_INNER));
    const bool inOuter = (out >= (HID_CENTER - CENTER_DEADZONE_OUTER))
        && (out <= (HID_CENTER + CENTER_DEADZONE_OUTER));

    if (axInDeadzone[ax]) {
        // Currently captured — must exit outer boundary to escape
        if (!inOuter) {
            axInDeadzone[ax] = false;
        }
        else {
            return HID_CENTER;  // Stay locked at center
        }
    }
    else {
        // Currently free — enter if we hit inner boundary
        if (inInner) {
            axInDeadzone[ax] = true;
            return HID_CENTER;
        }
    }

    return out;
}

// Should we load USB events?
bool loadUSBevents = false;
bool loadCDCevents = false;

// Serial close flags based on CLOSE_CDC_SERIAL or CLOSE_HWCDC_SERIAL
bool closeCDCserial = false;
bool closeHWCDCserial = false;

// HID Report and device instance
GamepadReport_t report = {};  // define the extern — zero-initializes all fields
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

// ── Deferred release for custom momentaries with releaseValue != 0 ──
// Same pattern as CoverGate: queue the release, fire after CUSTOM_RESPONSE_THROTTLE_MS
// so DCS-BIOS has time to register the press value before receiving the release.
#define MAX_PENDING_RELEASES 4
struct PendingRelease {
    const char* label;      // oride_label to send
    uint16_t    value;      // releaseValue
    uint32_t    due_ms;     // millis() when we should fire
    bool        active;
};
static PendingRelease s_pendingRelease[MAX_PENDING_RELEASES];

static void queueDeferredRelease(const char* label, uint16_t value) {
    const uint32_t due = millis() + CUSTOM_RESPONSE_THROTTLE_MS;
    // Debug print removed — the "fired" message below is sufficient
    // Overwrite any existing pending for the same label
    for (uint8_t i = 0; i < MAX_PENDING_RELEASES; ++i) {
        if (s_pendingRelease[i].active && strcmp(s_pendingRelease[i].label, label) == 0) {
            s_pendingRelease[i].value  = value;
            s_pendingRelease[i].due_ms = due;
            return;
        }
    }
    // Find an empty slot
    for (uint8_t i = 0; i < MAX_PENDING_RELEASES; ++i) {
        if (!s_pendingRelease[i].active) {
            s_pendingRelease[i].label  = label;
            s_pendingRelease[i].value  = value;
            s_pendingRelease[i].due_ms = due;
            s_pendingRelease[i].active = true;
            return;
        }
    }
    // Buffer full — send directly as fallback (shouldn't happen with 4 slots)
    debugPrintf("[DCS] ⚠️ Deferred release buffer full! Sending immediately: %s = %u\n", label, value);
    char fbuf[10];
    snprintf(fbuf, sizeof(fbuf), "%u", value);
    sendCommand(label, fbuf, false);
}

// Used for Axis stabilization and filtering (and reset for initialization)
// Indexed by GPIO pin number (not axis enum), so must match g_bootstrapped[64].
static int lastFiltered[64] = { 0 };
static int lastOutput[64] = { -1 };
static unsigned int stabCount[64] = { 0 };
static bool stabilized[64] = { false };

// Build HID group bitmasks
void buildHIDGroupBitmasks() {
  for (size_t i = 0; i < InputMappingSize; ++i) {
      const InputMapping& m = InputMappings[i];
      if (m.group > 0 && m.group < MAX_GROUPS && m.hidId > 0) {
          groupBitmask[m.group] |= (1UL << (m.hidId - 1));
      }
  }
}

// ----------------------------------------------------------------------------
// O(1) hash lookup for InputMappings[] by oride_label + oride_value
// Used by the sendCommand() HID gate to resolve slave commands to HID buttons.
// Composite key: hash(oride_label) ^ (oride_value * 7919) with linear probing.
// Only entries with valid hidId (1..32) are indexed.
// ----------------------------------------------------------------------------
static constexpr size_t HID_DCS_HASH_SIZE = 127; // prime, comfortably > 2x typical InputMappingSize
struct HidDcsHashEntry {
    const char*         oride_label;
    uint16_t            oride_value;
    const InputMapping* mapping;
};
static HidDcsHashEntry hidDcsHashTable[HID_DCS_HASH_SIZE];
static bool hidDcsHashBuilt = false;

static uint16_t hidDcsHash(const char* label, uint16_t value) {
    return labelHash(label) ^ (value * 7919);
}

static void buildHidDcsHashTable() {
    for (size_t i = 0; i < HID_DCS_HASH_SIZE; ++i)
        hidDcsHashTable[i] = { nullptr, 0, nullptr };

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.oride_label || m.hidId <= 0 || m.hidId > 32) continue;

        uint16_t h = hidDcsHash(m.oride_label, (uint16_t)m.oride_value) % HID_DCS_HASH_SIZE;
        for (size_t probe = 0; probe < HID_DCS_HASH_SIZE; ++probe) {
            if (!hidDcsHashTable[h].oride_label) {
                hidDcsHashTable[h] = { m.oride_label, (uint16_t)m.oride_value, &m };
                break;
            }
            h = (h + 1) % HID_DCS_HASH_SIZE;
        }
    }
    hidDcsHashBuilt = true;
}

// Find the HID mapping whose oride_label and oride_value match this DCS command+value.
const InputMapping* findHidMappingByDcs(const char* dcsLabel, uint16_t value) {
    if (!hidDcsHashBuilt) buildHidDcsHashTable();
    uint16_t h = hidDcsHash(dcsLabel, value) % HID_DCS_HASH_SIZE;
    for (size_t i = 0; i < HID_DCS_HASH_SIZE; ++i) {
        const auto& e = hidDcsHashTable[h];
        if (!e.oride_label) return nullptr;
        if (e.oride_value == value && strcmp(e.oride_label, dcsLabel) == 0)
            return e.mapping;
        h = (h + 1) % HID_DCS_HASH_SIZE;
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

    // O(1) hash lookup into shared command history
    CommandHistoryEntry* e = findCmdEntry(dcsLabel);
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
    if (m->hidId <= 0 || m->hidId > 32) return;
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

    for (int i = 0; i < 64; ++i) {
        stabCount[i] = 0;
        stabilized[i] = false;
        lastOutput[i] = -1;
        lastFiltered[i] = 0;
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
        #if RS485_SLAVE_ENABLED
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "%u", (uint16_t)v);
            sendCommand(dcsIdentifier, buf, false);
        }
        #endif
        if (axis < HID_AXIS_COUNT) report.axes[axis] = (uint16_t)v;
        HIDManager_dispatchReport(forceSend);
        return;
    }
#endif

    // --- per-pin state ---
    static bool g_bootstrapped[64] = { false };

    auto sendHID = [&](int value, bool force) {
        // RS485 slave: forward axis value to master via sendCommand.
        // HID report is stubbed on slaves, so dispatch is a no-op locally.
        // Value is sent as raw ADC (0-4095); master writes directly to report.axes[].
        #if RS485_SLAVE_ENABLED
        {
            char buf[8];
            snprintf(buf, sizeof(buf), "%u", (uint16_t)value);
            sendCommand(dcsIdentifier, buf, false);
        }
        #endif
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
    // Custom momentaries with releaseValue != 0 are fire-and-forget pulses:
    //   On press: send oride_value directly (bypass dwell) + queue deferred release.
    //   On release: do nothing (release was already scheduled at press time).
    // Direct sendCommand is required because the CommandHistoryEntry for the DCS label
    // (e.g. ENGINE_CRANK_SW) has group > 0 from the selector positions, and the dwell
    // gate in sendDCSBIOSCommand would buffer the press indefinitely.
    // Standard momentaries (releaseValue == 0): normal press/release behavior.
    if (dcsAllowed && m->oride_label) {
        if (newOn && m->releaseValue != 0) {
            // Custom momentary press: bypass dwell, send directly
            static char cbuf[10];
            snprintf(cbuf, sizeof(cbuf), "%u", m->oride_value);
            debugPrintf("[DCS] Custom momentary press: %s = %u\n", m->oride_label, m->oride_value);
            sendCommand(m->oride_label, cbuf, false);
            // Update command history for state tracking
            CommandHistoryEntry* ce = findCmdEntry(m->oride_label);
            if (ce) { ce->lastValue = m->oride_value; ce->lastSendTime = millis(); }
            // Schedule deferred release
            queueDeferredRelease(m->oride_label, m->releaseValue);
        } else if (newOn) {
            sendDCSBIOSCommand(m->oride_label, m->oride_value, forcePanelSyncThisMission);
        } else if (m->releaseValue == 0) {
            sendDCSBIOSCommand(m->oride_label, 0, forcePanelSyncThisMission);
        }
        // else: releaseValue != 0 and releasing — already handled by deferred queue at press time
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
                char buf[12];
                if (isVarStep) strcpy(buf, pressed ? "+3200" : "-3200");
                else           strcpy(buf, pressed ? "INC" : "DEC");
                sendCommand(m->oride_label, buf, false);
            }
            else if (isLatchedButton(name)) {
                // Rising-edge toggle handles both pipes when DCS is allowed
                HIDManager_toggleIfPressed(pressed, name, deferSend);
                return;
            }
            else if (m->oride_label) {
                if (pressed && m->releaseValue != 0) {
                    // Custom momentary press: bypass dwell, send directly
                    static char cbuf[10];
                    snprintf(cbuf, sizeof(cbuf), "%u", m->oride_value);
                    debugPrintf("[DCS] Custom momentary press: %s = %u\n", m->oride_label, m->oride_value);
                    sendCommand(m->oride_label, cbuf, false);
                    CommandHistoryEntry* ce = findCmdEntry(m->oride_label);
                    if (ce) { ce->lastValue = m->oride_value; ce->lastSendTime = millis(); }
                    queueDeferredRelease(m->oride_label, m->releaseValue);
                } else if (pressed) {
                    sendDCSBIOSCommand(m->oride_label, m->oride_value, forcePanelSyncThisMission);
                } else if (m->releaseValue == 0) {
                    sendDCSBIOSCommand(m->oride_label, 0, forcePanelSyncThisMission);
                }
                // else: releaseValue != 0 and releasing — already handled by deferred queue at press time
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

// ----------------------------------------------------------------------------
// RS485 Master HID passthrough: immediate dispatch, no dwell.
// Slave already performed dwell arbitration before sending over RS485,
// so the master must not add a second dwell layer on top.
// ----------------------------------------------------------------------------
void HIDManager_setButtonDirect(const char* name, bool pressed) {
    const InputMapping* m = findInputByLabel(name);
    if (!m || m->hidId <= 0 || m->hidId > 32) return;

    const uint32_t bit = (1UL << (m->hidId - 1));
    const char* ctype = m->controlType ? m->controlType : "";
    const bool isVarStep = (strcmp(ctype, "variable_step") == 0);
    const bool isFixStep = (strcmp(ctype, "fixed_step") == 0);

    // Step controls: pulse ON then auto-OFF (same as normal path)
    if (isVarStep || isFixStep) {
        report.buttons |= bit;
        HIDManager_dispatchReport(false);
        g_hidStepPulseMask |= bit;
        g_hidStepPulseDueMs[m->hidId] = millis() + STEP_PULSE_MS;
        return;
    }

    // Selectors: clear group, set bit, dispatch immediately (NO dwell)
    if (m->group > 0) {
        if (pressed) {
            report.buttons &= ~groupBitmask[m->group];
            report.buttons |= bit;
        } else {
            report.buttons &= ~bit;
        }
        HIDManager_dispatchReport(false);
        return;
    }

    // Momentary / everything else
    if (pressed) report.buttons |= bit; else report.buttons &= ~bit;
    HIDManager_dispatchReport(false);
}

// ----------------------------------------------------------------------------
// RS485 Master HID passthrough: write axis value directly, no filtering/dwell.
// Slave already performed ADC read, EMA filtering, inversion, and threshold
// gating before sending the value over RS485.
//
// AXIS CONVENTION FOR RS485 HID PASSTHROUGH:
// The 'hidId' field in InputMapping has a dual meaning based on controlType:
//   - Buttons/selectors: hidId = button bit position (1-32)
//   - Analog axes:       hidId = HIDAxis enum value (0=AXIS_X, ... 7=AXIS_SLIDER)
// The controlType ("analog" vs "selector"/"momentary") determines interpretation.
// ----------------------------------------------------------------------------
void HIDManager_setAxisDirect(HIDAxis axis, uint16_t value) {
    if (axis >= HID_AXIS_COUNT) return;
    // Slave sends DCS-range values (0-65535) through the DCS path on RS485.
    // HID descriptor declares Logical Max 4095, so we must scale down.
    report.axes[axis] = (uint16_t)map((long)value, 0, 65535, 0, 4095);
    HIDManager_dispatchReport(false);
}

void HIDManager_commitDeferredReport(const char* deviceName) {
    const bool inDcs = isModeSelectorDCS();
#if defined(MODE_HYBRID_DCS_HID) && (MODE_HYBRID_DCS_HID == 1)
    const bool hidPermitted = true;   // HYBRID: permit HID flush even in DCS
#else
    const bool hidPermitted = !inDcs; // Exclusive modes: HID only when selector says HID
#endif
    if (!hidPermitted) return;

#if !USE_DCSBIOS_WIFI && !USE_DCSBIOS_BLUETOOTH && !RS485_SLAVE_ENABLED
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
#if defined(ARDUINO_USB_MODE)
    // Only chips with native USB need USB.begin()
    USB.begin();
#endif
    // Original ESP32: nothing to do — Serial works via external bridge
}

void HIDManager_setup() {

    // Initialize axis calibration
    axCalibInit();   // Reset working arrays to default
    axCalibLoad();   // Load NVS (overwrites defaults if data exists)

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

// ── Deferred release tick — called from panel loop (same cadence as CoverGate_loop) ──
// Uses sendCommand() directly instead of sendDCSBIOSCommand() to bypass ALL gates:
//   - Selector dwell gate (group > 0 with !force) would buffer indefinitely
//   - Force+group logic would zero out real selector positions in the same group
//   - applyThrottle is unnecessary — we already waited CUSTOM_RESPONSE_THROTTLE_MS
//   - Redundancy suppression could block if lastValue matches
// The deferred release has already been validated and delayed. Just send it.
void HIDManager_releaseTick() {
    const uint32_t now = millis();
    for (uint8_t i = 0; i < MAX_PENDING_RELEASES; ++i) {
        if (!s_pendingRelease[i].active) continue;
        if ((int32_t)(now - s_pendingRelease[i].due_ms) < 0) continue;

        const char* label = s_pendingRelease[i].label;
        uint16_t    value = s_pendingRelease[i].value;

        debugPrintf("[DCS] Custom momentary release: %s = %u (%ums after press)\n",
                    label, value, (unsigned)CUSTOM_RESPONSE_THROTTLE_MS);

        char buf[10];
        snprintf(buf, sizeof(buf), "%u", value);
        sendCommand(label, buf, false);

        // Update command history so throttle/state tracking stays consistent
        CommandHistoryEntry* e = findCmdEntry(label);
        if (e) {
            e->lastValue    = value;
            e->lastSendTime = now;
        }

        s_pendingRelease[i].active = false;
    }
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