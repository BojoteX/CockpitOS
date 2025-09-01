// DCSBIOSBridge.cpp

#include "../Globals.h"
#include "../DCSBIOSBridge.h"
#include "../HIDManager.h"
#include "../LEDControl.h"

#if (ARDUINO_USB_CDC_ON_BOOT == 1)
#include "tusb.h"
#endif

#if DEBUG_USE_WIFI || USE_DCSBIOS_WIFI
#include "../WiFiDebug.h"
#endif

#if DCSBIOS_USE_LITE_VERSION
    #include "../../lib/DCS-BIOS/src/DcsBios.h"
    #include "../../lib/DCS-BIOS/src/internal/Protocol.cpp"
#else
    #define DCSBIOS_DISABLE_SERVO
    #define DCSBIOS_DEFAULT_SERIAL
    #include "DcsBios.h"    
#endif

// For edge cases where we need to force a resync of the panel
volatile bool forcePanelResyncNow = false;

// Keep track of DCS-BIOS connection state
static unsigned long lastNotReadyPrint = 0;

#if (USE_DCSBIOS_WIFI || USE_DCSBIOS_USB)
// Size of max packet for DCS Receive
static uint8_t reassemblyBuf[DCS_UDP_MAX_REASSEMBLED];
static uint32_t maxFramesDrainOverflow = 0;  // Global or file-scope
#endif

// --- Selector Sync Validation (stage 1/3) ---
#define MAX_VALIDATED_SELECTORS 32  // Tune to match maximum selectors you need to track
#define MISSION_START_DEBOUNCE 500  // ms to wait before panel sync

struct SelectorValidationEntry {
    const char* label;
    uint16_t lastSimValue; // Value as reported by DCS-BIOS subscription
};

static SelectorValidationEntry validatedSelectors[MAX_VALIDATED_SELECTORS];
static size_t numValidatedSelectors = 0;

// Force Panel Sync Global flag
bool forcePanelSyncThisMission = false;

// Same group send spacing enforcement
static uint32_t lastGroupSendUs[MAX_GROUPS] = {0};

// Command History tracking.
static_assert(commandHistorySize <= MAX_TRACKED_RECORDS, "Not enough space for tracked entries. Increase MAX_TRACKED_RECORDS in Config.h");

struct PendingUpdate {
    const char* label;
    uint16_t value;
    uint16_t max_value;
};

// ——— Configuration ———
#define MAX_PENDING_UPDATES 220

// ——— Stream Health ———
#define STREAM_TIMEOUT_MS   1000 // ms without activity → consider dead

// Used for reliably detecting an active stream and when it goes up or down.
// static constexpr unsigned long STREAM_TIMEOUT_MS = 3;  // ms without activity → consider dead

// Max display fields across all panels (tune as needed, e.g., 64 or 128)
#define MAX_REGISTERED_DISPLAY_BUFFERS 64
static RegisteredDisplayBuffer registeredBuffers[MAX_REGISTERED_DISPLAY_BUFFERS];
static size_t numRegisteredBuffers = 0;

bool registerDisplayBuffer(const char* label, char* buf, uint8_t len, bool* dirtyFlag, char* last) {
    // Check for existing registration
    for (size_t i = 0; i < numRegisteredBuffers; ++i) {
        if (strcmp(registeredBuffers[i].label, label) == 0) {
            if(DEBUG) debugPrintf("[DISPLAY] Buffer '%s' already registered\n", label);
            return false;
        }
    }
    // Only add new if not present and room remains
    if (numRegisteredBuffers >= MAX_REGISTERED_DISPLAY_BUFFERS) {
        debugPrintf("[DISPLAY] Buffer overflow for '%s', increase MAX_REGISTERED_DISPLAY_BUFFERS!\n", label);
        return false;
    }
    // Null-terminate buffers on registration (ultra-safe)
    if (buf)   buf[len] = '\0';
    if (last) last[len] = '\0';

    registeredBuffers[numRegisteredBuffers++] = { label, buf, len, dirtyFlag, last };
    return true;
}

void updateAnonymousStringField(AnonymousStringBuffer& field, uint16_t addr, uint16_t value) {
    unsigned int idx = addr - field.base_addr;
    if (idx < field.length)      field.buffer[idx] = value & 0xFF;
    if (idx + 1 < field.length)  field.buffer[idx + 1] = (value >> 8) & 0xFF;
    field.buffer[field.length] = '\0';
    if (field.dirty) *field.dirty = true;
}

void commitAnonymousStringField(AnonymousStringBuffer& field) {
    if (!field.dirty || !*field.dirty) return;
    *field.dirty = false;
    if (strncmp(field.buffer, field.last, field.length) != 0) {
        if (field.onChange) field.onChange(field.buffer);
        strncpy(field.last, field.buffer, field.length);
        field.last[field.length] = '\0';
    }
}

// Subscribe to Metadata changes
bool subscribeToMetadataChange(const char* label, void (*callback)(const char* label, uint16_t value)) {
    if (metadataSubscriptionCount >= MAX_METADATA_SUBSCRIPTIONS) return false;
    metadataSubscriptions[metadataSubscriptionCount++] = { label, callback };
    return true;
}

// Subscribe to selector changes
bool subscribeToSelectorChange(const char* label, void (*callback)(const char* label, uint16_t value)) {
    if (selectorSubscriptionCount >= MAX_SELECTOR_SUBSCRIPTIONS) return false;
    selectorSubscriptions[selectorSubscriptionCount++] = { label, callback };
    return true;
}

// Subscribe to LED changes
bool subscribeToLedChange(const char* label, void (*callback)(const char*, uint16_t, uint16_t)) {
    if (ledSubscriptionCount >= MAX_LED_SUBSCRIPTIONS) return false;
    ledSubscriptions[ledSubscriptionCount++] = { label, callback };
    return true;
}

// Check when mission starts / stops
static volatile uint32_t lastMissionStart = 0; // 0 = never started or stopped
static volatile bool panelsSyncedThisMission = false;

// New debounce sync fields:
static unsigned long aircraftNameReceivedAt = 0;
static bool panelsInitializedThisMission = false;


// How many milliseconds since mission start
uint32_t msSinceMissionStart() {
    return (lastMissionStart != 0) ? (millis() - lastMissionStart) : 0;
}

// is mission running?
bool isMissionRunning() {
    return (lastMissionStart != 0);
}

// is mission running?
bool isPanelsSyncedThisMission() {
    return panelsSyncedThisMission;
}

// For debugging ONLY
static bool TEMP_DISABLE_LISTENER = false;

// Frame counter for debugging purposes
uint64_t frameCounter = 0;

// Init DCS-BIOS previous values on startup
static uint16_t g_prevValues[DcsOutputTableSize];
static bool     g_prevInit = false;

void DCSBIOS_bustPrevValues() {
    for (size_t i = 0; i < DcsOutputTableSize; ++i) g_prevValues[i] = 0xFFFFu;
    g_prevInit = true;
}

static void DCSBIOS_bustDisplayBuffers() {
    for (size_t i = 0; i < numRegisteredBuffers; ++i) {
        RegisteredDisplayBuffer& b = registeredBuffers[i];
        if (!b.buffer || !b.last) continue;

        const size_t n = b.length;

        memset(b.buffer, ' ', n);
        b.buffer[n] = '\0';

        memset(b.last, 0xAA, n);   // force first commit
        b.last[n] = '\0';

        if (b.dirtyFlag) *b.dirtyFlag = true;  // optional: harmless
    }
}

// DcsBiosSniffer Listener
class DcsBiosSniffer : public DcsBios::ExportStreamListener {
public:
    DcsBiosSniffer(): 
        DcsBios::ExportStreamListener(0x0000, 0x77FF),
        pendingUpdateCount(0),
        pendingUpdateOverflow(0),
        _lastWriteMs(0),
        _streamUp(false)
    {}

    void onDcsBiosWrite(unsigned int addr, unsigned int value) override {

        // This is used for debugging and testing
        if(TEMP_DISABLE_LISTENER) return;
        if (!g_prevInit) DCSBIOS_bustPrevValues();

        unsigned long now = millis();     

        // 1) Stream-health logic
        _lastWriteMs = now;
        if (!_streamUp) {
            _streamUp = true;
            onStreamUp();    // one-time “stream came up” hook
        }

		// Aircraft name handling (relaxed, reliable)
        if (addr >= aircraftNameField.base_addr && addr < (aircraftNameField.base_addr + aircraftNameField.length)) {
            updateAnonymousStringField(aircraftNameField, addr, value);
        }

        const AddressEntry* ae = findDcsOutputEntries(addr);
        if (!ae) return;

        // 2) Dispatch per control type
        for (uint8_t i = 0; i < ae->count; ++i) {
            const DcsOutputEntry* entry = ae->entries[i];
            uint16_t val = (value & entry->mask) >> entry->shift;
            size_t index = entry - DcsOutputTable;

            if (entry->controlType != CT_DISPLAY) {
                if (index >= DcsOutputTableSize || g_prevValues[index] == val) continue;
                g_prevValues[index] = val;            // admits first 0/1/etc.
            }

            switch (entry->controlType) {
                case CT_GAUGE:
                case CT_LED:
                case CT_ANALOG:
                    if (pendingUpdateCount < MAX_PENDING_UPDATES) {
                        pendingUpdates[pendingUpdateCount++] = {entry->label, val, entry->max_value};
                    } else {
                        pendingUpdateOverflow++;
                    }
                    break;

                case CT_SELECTOR:
                    onSelectorChange(entry->label, val);
                    break;

                case CT_DISPLAY: {
                    const DisplayBufferEntry* bufEntry = findDisplayBufferByLabel(entry->label);
                    if (bufEntry) {
                        const DisplayFieldDef* field = findDisplayFieldByLabel(entry->label);
                        if (field) {
                            unsigned int sb_index = entry->addr - field->base_addr;
                            if (sb_index < field->length)        bufEntry->buffer[sb_index] = value & 0xFF;
                            if (sb_index + 1 < field->length)    bufEntry->buffer[sb_index + 1] = (value >> 8) & 0xFF;
                            // if (bufEntry->dirty) *bufEntry->dirty = true;
                        }
                    }
                    break;
                }

                case CT_METADATA:
                    onMetaDataChange(entry->label, val);
                    break;
            }
        }
    }

    void onConsistentData() override {

        if (TEMP_DISABLE_LISTENER) return;

        if (_streamUp && (millis() - _lastWriteMs) >= STREAM_TIMEOUT_MS) {
            _streamUp = false;
            onStreamDown();
        }

        for (size_t i = 0; i < numRegisteredBuffers; ++i) {
            RegisteredDisplayBuffer& b = registeredBuffers[i];
            if (!b.buffer || !b.last) continue;
            if (strncmp(b.buffer, b.last, b.length) != 0) {
                onDisplayChange(b.label, b.buffer);
                strncpy(b.last, b.buffer, b.length);
                b.last[b.length] = '\0';
            }
        }

        for (uint16_t i = 0; i < pendingUpdateCount; ++i) {
            const PendingUpdate& u = pendingUpdates[i];
            onLedChange(u.label, u.value, u.max_value);
        }
        pendingUpdateCount = 0;

        if (pendingUpdateOverflow > 0) {
            debugPrintf("[WARNING] %u LED updates dropped\n", pendingUpdateOverflow);
            pendingUpdateOverflow = 0;
        }

        // --- AIRCRAFT NAME HANDLING (relaxed, reliable) ---
        commitAnonymousStringField(aircraftNameField);
    }

    // New method to forcibly simulate mission stop
    void forceMissionStop() {
        memset(aircraftNameBuf, ' ', 24);    // force 24 spaces
        aircraftNameBuf[24] = '\0';
        // Set lastAircraftName to IMPOSSIBLE value so next frame will *always* be seen as different
        memset(lastAircraftName, 0xAA, 24);  // 0xAA is an impossible ASCII aircraft name
        lastAircraftName[24] = '\0';
        aircraftNameDirty = true;            // Will trigger commit on next cycle

        // Reset global/volatile mission state
        lastMissionStart = 0;
        panelsSyncedThisMission = false;
        aircraftNameReceivedAt = 0;
        panelsInitializedThisMission = false;

        debugPrintf("[SYNC] forceMissionStop(): All mission state reset to menu, lastAircraftName set to impossible value.\n");
    }

    bool isStreamAlive() const {
        return (millis() - _lastWriteMs) < STREAM_TIMEOUT_MS;
    }

    unsigned long msSinceLastWrite() const { return millis() - _lastWriteMs; }

protected:
    virtual void onStreamUp() { debugPrintln("[DCS-BIOS] ✅ STREAM UP"); }
    virtual void onStreamDown() { debugPrintln("[DCS-BIOS] ❌ STREAM DOWN"); }

private:
    char aircraftNameBuf[25] = {};
    char lastAircraftName[25] = {};
    bool aircraftNameDirty = false;

    AnonymousStringBuffer aircraftNameField = {
    0x0000, 24, aircraftNameBuf, lastAircraftName, &aircraftNameDirty, onAircraftName
    };

    uint16_t aircraftNameWordsSeen = 0;
    PendingUpdate    pendingUpdates[MAX_PENDING_UPDATES];
    uint16_t         pendingUpdateCount;
    uint32_t         pendingUpdateOverflow;
    unsigned long    _lastWriteMs;
    bool             _streamUp;
};
DcsBiosSniffer mySniffer;

void DCSBIOS_forceMissionStop() {
    mySniffer.forceMissionStop();
}

void dumpAllMetadata() {
    debugPrintf("\n[METADATA DUMP]\n");
    for (size_t i = 0; i < numMetadataStates; ++i) {
        debugPrintf("  %s = %u\n", metadataStates[i].label, metadataStates[i].value);
    }
}

void syncCommandHistoryFromInputMapping() {
    for (size_t i = 0; i < commandHistorySize; ++i) {
        CommandHistoryEntry& e = commandHistory[i];
        e.isSelector = false;
        e.group = 0;
        // e.lastValue = 0xFFFF;   // unknown
        e.hasPending = false;
        e.pendingValue = 0;
        e.lastChangeTime = 0;
        e.lastSendTime = 0;

        // Bind any INPUT used by the firmware to this DCS label, so we always track it.
        for (size_t j = 0; j < InputMappingSize; ++j) {
            const InputMapping& m = InputMappings[j];
            if (!m.oride_label || strcmp(e.label, m.oride_label) != 0) continue;

            // Track selectors and buttons
            const bool isSel = (strcmp(m.controlType, "selector") == 0);
            const bool isBtn = (strcmp(m.controlType, "momentary") == 0);

            if (isSel || isBtn) {
                // Only selectors participate in dwell/group arbitration
                if (isSel && m.group > 0) {
                    e.isSelector = true;
                    if (m.group > e.group) e.group = m.group;
                }
                // Nothing else to set; onSelectorChange() will keep e.lastValue fresh
                break;
            }
        }
    }
    debugPrint("[SYNC] Command history initialized for ALL inputs (selectors + buttons).\n");
}

void initPanels() {
    debugPrintf("[SYNC PANELS] 🔁 Mission Started %u ms ago\n", msSinceMissionStart());

    forcePanelSyncThisMission = true;
    panelsSyncedThisMission = true;   // ← move back UP

    for (uint16_t g = 0; g < MAX_GROUPS; ++g) lastGroupSendUs[g] = 0;
    for (size_t i = 0; i < numValidatedSelectors; ++i) validatedSelectors[i].lastSimValue = 0xFFFF;

    HIDManager_resetAllAxes();
    syncCommandHistoryFromInputMapping();
    // debugPrintln("[SYNC PANELS] ❌ Just ran syncCommandHistoryFromInputMapping()\n");

    initializePanels(1);        // emits forced selector/axis commands
    // debugPrintln("[SYNC PANELS] ❌ Just ran  initializePanels(1)\n");

    flushBufferedDcsCommands(); // clears losers / commits winners immediately
    // debugPrintln("[SYNC PANELS] ❌ Just ran flushBufferedDcsCommands()\n");

    forcePanelSyncThisMission = false;

}

void onAircraftName(const char* str) {
    // 1. Check for blank (all spaces or nulls) == mission stop
    bool isBlank = true;
    for (int i = 0; i < 24; ++i) {
        if (str[i] != ' ' && str[i] != '\0') {
            isBlank = false;
            break;
        }
    }

    static bool alreadyStarted = false;
    if (!isBlank && strncmp(str, DCSBIOS_ACFT_NAME, strlen(DCSBIOS_ACFT_NAME)) == 0) {
        if (!alreadyStarted) {
            alreadyStarted = true;
            debugPrintf("[MISSION START] %.*s\n", 24, str);
            lastMissionStart = millis();
            aircraftNameReceivedAt = millis();

            // Treat next incoming output words as fresh for this mission
            DCSBIOS_bustPrevValues();
            DCSBIOS_bustDisplayBuffers(); // CT_DISPLAY fields
        }
    }
    else if (isBlank) {
        if (alreadyStarted) {
            debugPrintln("[MISSION STOP]");
            alreadyStarted = false;
        }
        lastMissionStart = 0;
        panelsSyncedThisMission = false;
    }
}

/*
// Frame counter for reference measure frame skips
void onUpdateCounterChange(unsigned int value) {
    uint8_t frameCounter = value & 0xFF;          // low byte
    uint8_t frameSkipCounter = (value >> 8) & 0xFF; // high byte
    // Used for debugging only
    debugPrintf("Frame: %u | Skips: %u\n", frameCounter, frameSkipCounter);
}
*/

void onLedChange(const char* label, uint16_t value, uint16_t max_value) {

	// Count how many times an "onChange" function was called to ensure we have a valid frame
    frameCounter++;

    if (max_value <= 1) {
        setLED(label, value > 0);
        char buf[128];
        snprintf(buf, sizeof(buf), "[LED] %s is set to %u", label, value);
        if(DEBUG) debugPrintln(buf);    
    } else {
        uint8_t intensity = (value * 100UL) / max_value;
        if (intensity < 7) {
            setLED(label, false, 0, value, max_value);  // treat as OFF
            char buf[128];
            snprintf(buf, sizeof(buf), "[LED] %s Intensity was set to 0", label);
            if(DEBUG) debugPrintln(buf);
        } else {
            if (intensity > 93) intensity = 100;
            setLED(label, true, intensity, value, max_value);
            char buf[128];
            snprintf(buf, sizeof(buf), "[LED] %s Intensity %u\%.", label, value);
            if(DEBUG) debugPrintln(buf);
        }
    }

    // Dispatch to subscribers
    for (size_t i = 0; i < ledSubscriptionCount; ++i) {
        if (strcmp(ledSubscriptions[i].label, label) == 0 && ledSubscriptions[i].callback) {
            ledSubscriptions[i].callback(label, value, max_value);
        }
    }
}

void onSelectorChange(const char* label, unsigned int value) {

    // Count how many times an "onChange" function was called to ensure we have a valid frame
    frameCounter++;

    CommandHistoryEntry* e = findCmdEntry(label);
    if (e) e->lastValue = value;

    const char* stateStr = nullptr;

    // 1. Covers
    if (strstr(label, "_COVER")) {
        stateStr = (value > 0) ? "OPEN" : "CLOSED";

    // 2. Strict _BTN suffix match (exclude _COVER_BTN)
    } else if (strlen(label) >= 4 && strcmp(label + strlen(label) - 4, "_BTN") == 0) {
        stateStr = (value > 0) ? "ON" : "OFF";

    // 3. Lookup in SelectorMap by exact (label + value) or (dcsCommand + value)
    } else {
        const SelectorEntry* match = nullptr;

        for (size_t i = 0; i < SelectorMapSize; ++i) {
            const SelectorEntry& entry = SelectorMap[i];

            bool matchLabelAndValue = (strcmp(entry.label, label) == 0 && entry.value == value);
            bool matchDcsAndValue   = (strcmp(entry.dcsCommand, label) == 0 && entry.value == value);

            if (matchLabelAndValue || matchDcsAndValue) {
                match = &entry;
                break;
            }
        }

        if (match && match->posLabel && match->posLabel[0] != '\0') {
            stateStr = match->posLabel;
        } else {
            static char fallback[16];
            snprintf(fallback, sizeof(fallback), "POS %u", value);
            stateStr = fallback;
        }
    }  

	// Handle selector subscriptions
    for (size_t i = 0; i < selectorSubscriptionCount; ++i) {
        if (strcmp(selectorSubscriptions[i].label, label) == 0 && selectorSubscriptions[i].callback) {
            selectorSubscriptions[i].callback(label, value);
        }
    }

    debugPrintf("[STATE UPDATE] 🔁 %s = %s\n", label, stateStr);
}

void onMetaDataChange(const char* label, unsigned int value) {

    // Count how many times an "onChange" function was called to ensure we have a valid frame
    frameCounter++;

    MetadataState* st = findMetadataState(label);
    if (st) st->value = value;    

    // Dispatch to subscribers
    for (size_t i = 0; i < metadataSubscriptionCount; ++i) {
        if (strcmp(metadataSubscriptions[i].label, label) == 0 && metadataSubscriptions[i].callback) {
            metadataSubscriptions[i].callback(label, value);
        }
    }

    if(DEBUG) debugPrintf("[METADATA] %s value is %u\n", label, value);
}

void onDisplayChange(const char* label, const char* value) {

    // Count how many times an "onChange" function was called to ensure we have a valid frame
    frameCounter++;

    if(DEBUG) debugPrintf("[DISPLAY] %s value is %s\n", label, value);

    renderField(label, value);
}

// Stage 3: Initialize validation selectors from InputMappings
void initializeSelectorValidation() {
    numValidatedSelectors = 0;
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const InputMapping& m = InputMappings[i];
        const char* dcs_label = m.oride_label;

        if (!dcs_label || !*dcs_label) {
            continue;
        }

        if (
            strcmp(m.controlType, "selector") != 0 ||
            m.group == 0
            ) continue;

		/*  RESTRICTIVE CHECKS
        if (
            strcmp(m.controlType, "selector") != 0 ||
            m.group == 0 ||
            (m.source && (strcmp(m.source, "NONE") == 0 || strcmp(m.source, "PCA_0x00") == 0))
            ) continue;
        */

        // if (m.hidId <= 0) continue;

        bool alreadyRegistered = false;
        for (size_t j = 0; j < numValidatedSelectors; ++j) {
            if (strcmp(validatedSelectors[j].label, dcs_label) == 0) {
                alreadyRegistered = true;
                break;
            }
        }
        if (!alreadyRegistered && numValidatedSelectors < MAX_VALIDATED_SELECTORS) {
            validatedSelectors[numValidatedSelectors].label = dcs_label;
            validatedSelectors[numValidatedSelectors].lastSimValue = 0xFFFF;
            ++numValidatedSelectors;
            debugPrintf("[SYNC] Tracking selector %s for changes\n", dcs_label);
            subscribeToSelectorChange(dcs_label, selectorValidationCallback);
        }
    }
}

void validateSelectorSync() {
    debugPrintf("\n[SELECTOR SYNC VALIDATION]\n");
    for (size_t i = 0; i < numValidatedSelectors; ++i) {
        const char* label = validatedSelectors[i].label;
        uint16_t simValue = validatedSelectors[i].lastSimValue;
        uint16_t fwValue = getLastKnownState(label);

        if (fwValue == simValue) {
            // debugPrintf("  OK: %s => FW=%u, SIM=%u\n", label, fwValue, simValue);
        }
        else {
            debugPrintf(" SYNC: %s => FW=%u, SIM=%u   <--- Forcing sim to FW\n", label, fwValue, simValue);
            sendDCSBIOSCommand(label, fwValue, true); // true = force
        }
    }
}

// Works for both UDP and HID incoming pipes
void parseDcsBiosUdpPacket(const uint8_t* data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        DcsBios::parser.processChar(data[i]);
    }
    // yield();
}

#if USE_DCSBIOS_WIFI || USE_DCSBIOS_USB
void onDcsBiosUdpPacket() {
    static struct {
        uint8_t  data[DCS_UDP_MAX_REASSEMBLED];
        size_t   len;
    } frames[MAX_UDP_FRAMES_PER_DRAIN];

    size_t frameCount = 0;
    size_t reassemblyLen = 0;
    DcsUdpRingMsg pkt;

    // Phase 1: Drain as many complete UDP frames as possible
    while (dcsUdpRingbufPop(&pkt)) {
        if (reassemblyLen + pkt.len > sizeof(frames[0].data)) {
            reassemblyLen = 0;
            debugPrintln("❌ [RING BUFFER] Overflow! increase DCS_UDP_MAX_REASSEMBLED");
            continue;
        }
        memcpy(frames[frameCount].data + reassemblyLen, pkt.data, pkt.len);
        reassemblyLen += pkt.len;

        if (pkt.isLastChunk) {
            frames[frameCount].len = reassemblyLen;
            frameCount++;
            reassemblyLen = 0;

            if (frameCount == MAX_UDP_FRAMES_PER_DRAIN) {
                maxFramesDrainOverflow++; // <-- INCREMENT COUNTER!
                break; // Stop draining more for this pass, parse now
            }
        }
    }

    // Phase 2: Parse all collected UDP frames
    for (size_t n = 0; n < frameCount; ++n) {
        for (size_t i = 0; i < frames[n].len; ++i) {
            DcsBios::parser.processChar(frames[n].data[i]);
        }
        // yield();
    }
}
#endif

// DcsbiosReplayData.h is generated from dcsbios_data.json using Python (see ReplayData directory). This was used in early development
// to test locally via Serial Console. The preferred method for live debugging is WiFi UDP. See Config.h for configuration.
#if IS_REPLAY
// #include "../PsramConfig.h"
// ReplayDataBuffer
uint8_t* replayBuffer = nullptr;
#include "../../ReplayData/DcsbiosReplayData.h"

bool replayData() {
  // Uses a header object (created from dcsbios_data.json) to simulate DCS traffic internally WITHOUT using your serial port (great for debugging) 
  if (initPSRAM()) {
      replayBuffer = (uint8_t*)PS_MALLOC(dcsbiosReplayLength);
      if (replayBuffer) {
          memcpy(replayBuffer, dcsbiosReplayData, dcsbiosReplayLength);
          debugPrintln("[PSRAM] ✅ Data loaded into PSRAM.");
      } else {
          debugPrintln("[PSRAM] ❌ Failed to allocate memory to run replay.");
          return false;
      }
  } else {
      debugPrintln("[PSRAM] ❌ Can't run replay as PSRAM is either not available or enabled.");
      return false;
  }
  // Begin simulated loop. 
  runReplayWithPrompt();
  return true;
}

void DcsbiosProtocolReplay() {
    debugPrintln("\n[REPLAY PROTOCOL] 🔁 Playing stream from binary blob...");

    if (!replayBuffer) return;

    const uint8_t* ptr = replayBuffer;
    const uint8_t* end = replayBuffer + dcsbiosReplayLength;

    while (ptr < end) {

        #if DEBUG_PERFORMANCE
            beginProfiling(PERF_REPLAY);
        #endif

        float frameDelay;
        memcpy_P(&frameDelay, ptr, sizeof(float));
        ptr += sizeof(float);

        uint16_t len = pgm_read_byte(ptr) | (pgm_read_byte(ptr + 1) << 8);
        ptr += 2;

        for (uint16_t i = 0; i < len; i++) {
            uint8_t b = pgm_read_byte(ptr + i);
            DcsBios::parser.processChar(b);
        }
        ptr += len;

        // Ticks and some of our panels require this.
        panelLoop();  

        #if DEBUG_PERFORMANCE
            endProfiling(PERF_REPLAY);
        #endif

        #if DEBUG_PERFORMANCE
            perfMonitorUpdate();
        #endif

        delay((unsigned long)(frameDelay * 1000));

    }
    debugPrintln("[REPLAY PROTOCOL] ✅ Complete.\n");
}
#endif

const char* getLastValueForDisplayLabel(const char* label) {
    const DisplayBufferEntry* bufEntry = findDisplayBufferByLabel(label);
    if (!bufEntry) return nullptr;
    return bufEntry->last;
}

uint16_t getMetadataValue(const char* label) {
    MetadataState* st = findMetadataState(label);
    return st ? st->value : 0;
}

uint16_t getLastKnownState(const char* label) {
    CommandHistoryEntry* e = findCmdEntry(label);
    return e ? e->lastValue : 0;
}

// Accessors into our TU static
CommandHistoryEntry* dcsbios_getCommandHistory() {
    return commandHistory;
}
size_t dcsbios_getCommandHistorySize() {
    return commandHistorySize;
}

// ----------------------------------------------------------------------------
// Lookup helper (O(N) for now, can swap to hash later)
// ----------------------------------------------------------------------------
CommandHistoryEntry* findCmdEntry(const char* label) {
    for (size_t i = 0; i < commandHistorySize; ++i) {
        if (strcmp(commandHistory[i].label, label) == 0) {
            return &commandHistory[i];
        }
    }
    return nullptr;
}

static void flushBufferedDcsCommands() {

    unsigned long now = millis();

    uint32_t nowUs = micros();
    const bool forceInit = forcePanelSyncThisMission;

    CommandHistoryEntry* groupLatest[MAX_GROUPS] = { nullptr };

    // Step 1: Find winner per group
    for (size_t i = 0; i < commandHistorySize; ++i) {
        CommandHistoryEntry& e = commandHistory[i];
        if (!e.hasPending || e.group == 0) continue;

        // NEW: during forced init, accept immediately
        const bool dwellOk = forceInit || (now - e.lastChangeTime >= SELECTOR_DWELL_MS);
        if (dwellOk) {
            uint16_t g = e.group;
            if (g >= MAX_GROUPS) {
                debugPrintf("❌ FATAL: group ID %u exceeds MAX_GROUPS (%u). Halting flush.\n", g, MAX_GROUPS);
                abort();
            }
            if (!groupLatest[g] || e.lastChangeTime > groupLatest[g]->lastChangeTime) {
                groupLatest[g] = &e;
            }
        }
    }

    // Step 2: Clear losers and send winner
    for (uint16_t g = 1; g < MAX_GROUPS; ++g) {
        CommandHistoryEntry* winner = groupLatest[g];
        if (!winner) continue;

        nowUs = micros();
        // NEW: during forced init, bypass spacing gate
        if (!forceInit && (nowUs - lastGroupSendUs[g]) < DCS_GROUP_MIN_INTERVAL_US) {
            continue;
        }

        // Clear losers
        for (size_t i = 0; i < commandHistorySize; ++i) {
            CommandHistoryEntry& e = commandHistory[i];
            if (e.group != g || &e == winner) continue;

            if (e.lastValue != 0) {
                char buf[10]; snprintf(buf, sizeof(buf), "0");
                sendCommand(e.label, buf, false);
                e.lastValue = 0;
                e.lastSendTime = now;
            }
            e.hasPending = false;
        }

        // Send winner
        if (winner->pendingValue != winner->lastValue) {
            char buf[10]; snprintf(buf, sizeof(buf), "%u", winner->pendingValue);
            sendCommand(winner->label, buf, false);
            winner->lastValue = winner->pendingValue;
            winner->lastSendTime = now;
        }

        winner->hasPending = false;
        lastGroupSendUs[g] = nowUs;
    }

	// Step 3: Send any non-grouped commands (buttons, axes)
    for (size_t i = 0; i < commandHistorySize; ++i) {
        CommandHistoryEntry& e = commandHistory[i];
        if (!e.hasPending || e.group != 0) continue;
        char buf[10]; snprintf(buf, sizeof(buf), "%u", e.pendingValue);
        sendCommand(e.label, buf, false);
        e.lastValue = e.pendingValue;
        e.lastSendTime = now;
        e.hasPending = false;
    }
}

// ----------------------------------------------------------------------------
// Send command to DCS-BIOS sendDcsBiosCommand 
// ----------------------------------------------------------------------------
bool applyThrottle(CommandHistoryEntry &e,
                          const char*    label,
                          uint16_t       value,
                          bool           force) {
    // Always allow forced and release events
    if (force || value == 0) {
        return true;
    }
    unsigned long now = millis();
    unsigned long dt  = now - e.lastSendTime;

    if (value == 1) {
        // BUTTON logic: very-fast duplicate first -> identical, slower -> rate
        if (dt < VALUE_THROTTLE_MS) {
            debugPrintf("[DCS] ⚠️ SKIP: %s debounced (%lums < %lums)\n",
                        label, dt, VALUE_THROTTLE_MS);
            return false;
        }
    } else {
        // KNOB/AXIS logic: simple rate limiting
        if (dt < ANY_VALUE_THROTTLE_MS) {
            debugPrintf("[DCS] ⚠️ SKIP: %s rate limited (%lums < %lums)\n",
                        label, dt, ANY_VALUE_THROTTLE_MS);
            return false;
        }
    }
    return true;
}

// Control Flags
static volatile bool isConnected    = false;
static volatile bool cdcTxReady     = true; // Needs to be true, otherwise we could never do a first send. We still prevent writes with cdcRxReady below
static volatile bool cdcRxReady     = false; // This will be set automatically when the stream starts

#if (ARDUINO_USB_CDC_ON_BOOT == 1)
static void cdcConnectedHandler(void* arg, esp_event_base_t base, int32_t id, void* event_data) {
    debugPrintln("🔌 CDC Connected (DTR asserted)");
    isConnected = true;
}

static void cdcDisconnectedHandler(void* arg, esp_event_base_t base, int32_t id, void* event_data) {
    debugPrintln("❌ CDC Disconnected (DTR deasserted)");
    isConnected = false;
}

static void cdcLineStateHandler(void* arg, esp_event_base_t base, int32_t id, void* event_data) {
    auto* ev = (arduino_usb_cdc_event_data_t*)event_data;
    bool dtr = ev->line_state.dtr;
    bool rts = ev->line_state.rts;

    debugPrintf("📡 CDC Line State: DTR=%s, RTS=%s\n",
                dtr ? "ON" : "OFF",
                rts ? "ON" : "OFF");
}

static void cdcLineCodingHandler(void* arg, esp_event_base_t base, int32_t id, void* event_data) {
    auto* ev = (arduino_usb_cdc_event_data_t*)event_data;

    debugPrintf("🔧 CDC Line Coding: Baud=%u, StopBits=%u, Parity=%u, DataBits=%u\n",
                ev->line_coding.bit_rate,
                ev->line_coding.stop_bits,
                ev->line_coding.parity,
                ev->line_coding.data_bits);
}

static void cdcRxHandler(void* arg,
                         esp_event_base_t base,
                         int32_t id,
                         void* event_data)
{
    // auto* ev = (arduino_usb_cdc_event_data_t*)event_data;
    // size = ev->rx.len
    cdcRxReady = true;              
}

static void cdcTxHandler(void* arg,
                         esp_event_base_t base,
                         int32_t id,
                         void* event_data)
{
    cdcTxReady = true;
}

// Let us know when there is an overflow event
static void cdcRxOvfHandler(void* arg,
                            esp_event_base_t base,
                            int32_t id,
                            void* event_data)
{
    auto* ev = (arduino_usb_cdc_event_data_t*)event_data;
    debugPrintf("[CDC RX_OVERFLOW] ❌ dropped=%u\n", (unsigned)ev->rx_overflow.dropped_bytes);
}
#endif

#if (ARDUINO_USB_CDC_ON_BOOT == 1)
// TinyUSB CDC events
void setupCDCEvents() {
#if defined(ENABLE_CDC_EVENTS) && (ENABLE_CDC_EVENTS > 0)
    Serial.onEvent(ARDUINO_USB_CDC_CONNECTED_EVENT, cdcConnectedHandler);
    Serial.onEvent(ARDUINO_USB_CDC_DISCONNECTED_EVENT, cdcDisconnectedHandler);
    // Serial.onEvent(ARDUINO_USB_CDC_LINE_STATE_EVENT, cdcLineStateHandler);
    // Serial.onEvent(ARDUINO_USB_CDC_LINE_CODING_EVENT, cdcLineCodingHandler);       
    Serial.onEvent(ARDUINO_USB_CDC_RX_EVENT, cdcRxHandler);
    Serial.onEvent(ARDUINO_USB_CDC_TX_EVENT, cdcTxHandler);
    Serial.onEvent(ARDUINO_USB_CDC_RX_OVERFLOW_EVENT, cdcRxOvfHandler);
#endif
}

// Hardware CDC events (ESP32 core 3.x)
void setupHWCDCEvents() {
#if defined(ENABLE_HWCDC_EVENTS) && (ENABLE_HWCDC_EVENTS > 0)
    Serial.onEvent(ARDUINO_HW_CDC_CONNECTED_EVENT, cdcConnectedHandler);
    Serial.onEvent(ARDUINO_HW_CDC_DISCONNECTED_EVENT, cdcDisconnectedHandler);
    Serial.onEvent(ARDUINO_HW_CDC_RX_EVENT, cdcRxHandler);
    Serial.onEvent(ARDUINO_HW_CDC_TX_EVENT, cdcTxHandler);
    Serial.onEvent(ARDUINO_HW_CDC_RX_OVERFLOW_EVENT, cdcRxOvfHandler);
#endif
}
#endif

bool cdcEnsureTxReady(uint32_t timeoutMs) {
    unsigned long start = millis();
    while (!cdcTxReady) {
        yield();
        if (millis() - start > timeoutMs) {
            return false;
        }
    }
    return true;
}

bool cdcEnsureRxReady(uint32_t timeoutMs) {
    unsigned long start = millis();
    while (!cdcRxReady) {
        yield();
        if (millis() - start > timeoutMs) {
            return false;
        }
    }
    return true;
}

bool isSerialConnected() {
    return isConnected;
}

bool tryToSendDcsBiosMessage(const char* msg, const char* arg) {
    // Estimate the total space needed
    if (!msg || !arg) return false;

    constexpr size_t maxMsgLen = 64;
    constexpr size_t maxArgLen = 32;

    size_t msgLen = strnlen(msg, maxMsgLen);
    size_t argLen = strnlen(arg, maxArgLen);

    if (msgLen == maxMsgLen || argLen == maxArgLen) return false;

    size_t len = msgLen + 1 + argLen + 2; // "CMD ARG\n"

#if (ARDUINO_USB_CDC_ON_BOOT == 1)
    // Serial ONLY send
    if (tud_cdc_write_available() < len) return false;
    tud_cdc_write(msg, msgLen);
    tud_cdc_write_str(" ");
    tud_cdc_write(arg, argLen);
    tud_cdc_write_str("\n");
    size_t before = tud_cdc_write_available();
    tud_cdc_write_flush();
    size_t after = tud_cdc_write_available();
    if (after <= before) return false;
    return true;
#else
    // No where to send this to...
    return false;
#endif
}

bool simReady() {

    if (isMissionRunning() && isPanelsSyncedThisMission() && mySniffer.isStreamAlive())
        return true;

    return false;
}

void sendCommand(const char* msg, const char* arg, bool silent) {

#if USE_DCSBIOS_WIFI
    // We completely bypass Serial+Socat and send our DCSBIOS command via UDP directly to the PC running DCS if mission is active
    if(simReady()) {
        if(tryToSendDcsBiosMessageUDP(msg, arg)) {
            if(!silent) debugPrintf("🛩️ [DCS-WIFI] %s %s\n", msg, arg);
        }
        else {
            if(!silent) debugPrintln("❌ [DCS-WIFI] Failed to send via UDP");
        }
    }
    else {
        if (!silent) debugPrintf("🛩️ [DCS-WIFI] DCS NOT READY! could not send %s %s\n", msg, arg);
	}
#elif USE_DCSBIOS_USB
    if(simReady()) {
   
        // This is the MAX size for our msg and arg. Lets try to fit INSIDE 63 Bytes to avoid chunking (faster)
        constexpr size_t maxMsgLen = 48;
        constexpr size_t maxArgLen =  8;

        size_t msgLen = strnlen(msg, maxMsgLen);
        size_t argLen = strnlen(arg, maxArgLen);

        char dcsCmd[maxMsgLen + maxArgLen + 4];
        size_t pos = 0;
        memcpy(&dcsCmd[pos], msg, msgLen); pos += msgLen;
        dcsCmd[pos++] = ' ';
        memcpy(&dcsCmd[pos], arg, argLen); pos += argLen;
        dcsCmd[pos++] = '\r';
        dcsCmd[pos++] = '\n';
        dcsRawUsbOutRingbufPushChunked((const uint8_t*)dcsCmd, pos);
        if (!silent) debugPrintf("🛩️ [DCS-USB] %s %s\n", msg, arg);

        // This is a dummy report, it will just trigger on the host side a Feature request to our device, which will drain our ring buffer on each call, host will keep calling until end of message received
        HIDManager_dispatchReport(true);
    }
    else {
        if (!silent) debugPrintf("🛩️ [DCS-USB] DCS NOT READY! could not send %s %s\n", msg, arg);
    }
#else
    if(!isConnected) {
        // Since socat does NOT assert DTR, we need to be able to send even if no Serial connection is detected, 
        // just check cdc Tx & Rx are working to ensure CDC is 100% healthy

        /*
        if (!cdcEnsureRxReady(CDC_TIMEOUT_RX_TX) || !cdcEnsureTxReady(CDC_TIMEOUT_RX_TX)) {
            if(!silent) debugPrintln("❌ [DCS] No stream active yet or Tx buffer full");
            return;
        }
        */

        if (simReady()) {

            if (!cdcEnsureTxReady(CDC_TIMEOUT_RX_TX)) {
                if (!silent) debugPrintln("❌ [DCS] Tx buffer full");
                return;
            }

            cdcTxReady = false;
            if (tryToSendDcsBiosMessage(msg, arg)) {
                if (!silent) debugPrintf("🛩️ [DCS] %s %s\n", msg, arg);
            }
            else {
                if (!silent) debugPrintf("❌ [DCS] Failed to send %s %s\n", msg, arg);
                return;
            }
        }
        else {
            if (!silent) debugPrintf("🛩️ [DCS-CDC] DCS NOT READY! could not send %s %s\n", msg, arg);
        }

    }
    else {
        bool bypass = false;
        if ((simReady()) || bypass) {

            // Testing
            if (bypass) {
				debugPrintf("🛩️ [DCS-CDC] Bypassing stream gating due to bypass flag\n");
            }

            // This assumes a healthy Serial connection is established
            cdcTxReady = false;
            if (tryToSendDcsBiosMessage(msg, arg)) {
                if (!silent) debugPrintf("🛩️ [DCS-UART] %s %s\n", msg, arg);
            }
            else {
                if (!silent) debugPrintf("❌ [DCS-UART] Failed to send %s %s\n", msg, arg);
            }

        }
        else {
            if (!silent) debugPrintf("🛩️ [DCS-UART] DCS NOT READY! could not send %s %s\n", msg, arg);
        }
    }
#endif
}

void DCSBIOS_keepAlive() {
    static unsigned long lastHeartbeat = 0;
    unsigned long now = millis();

    if (now - lastHeartbeat >= DCS_KEEP_ALIVE_MS) {
        lastHeartbeat = now;
        sendCommand("PING", "0", true); // Silent, wont show us any message
    }
}

void sendDCSBIOSCommand(const char* label, uint16_t value, bool force) {

	// Prevent any command if sim is not ready yet even if forced
    if (!simReady) 
        debugPrintf("⚠️ [DCS] NOT READY! ignoring command \"%s %u\" (force=%u)\n", label, value, force);

    static char buf[10];
    snprintf(buf, sizeof(buf), "%u", value);

    CommandHistoryEntry* e = findCmdEntry(label);
    if (!e) {
        debugPrintf("⚠️ [DCS] REJECTED untracked: %s = %u\n", label, value);
        return;
    }

    const unsigned long now = millis();

// #if defined(SELECTOR_DWELL_MS) && (SELECTOR_DWELL_MS > 0)
    if (!force && e->group > 0) {
        e->pendingValue = value;
        e->lastChangeTime = now;
        e->hasPending = true;
        return;
    }
// #endif

    if (force && e->group > 0) {
        const uint16_t g = e->group;
        // 1) kill any buffered state in this group
        for (size_t i = 0; i < commandHistorySize; ++i) {
            if (commandHistory[i].group == g) commandHistory[i].hasPending = false;
        }
        // 2) clear losers now
        for (size_t i = 0; i < commandHistorySize; ++i) {
            CommandHistoryEntry& x = commandHistory[i];
            if (x.group != g || &x == e) continue;
            if (x.lastValue != 0) {
                char zbuf[2] = { '0', '\0' };
                sendCommand(x.label, zbuf, false);  // still gated
                x.lastValue = 0;
                x.lastSendTime = now;
            }
        }
        if (g < MAX_GROUPS) lastGroupSendUs[g] = micros(); // spacing hygiene
    }

    if (!applyThrottle(*e, label, value, force)) return;

    sendCommand(label, buf, false); // still gated
    e->lastValue = value;
    e->lastSendTime = now;
}

// Stage 2: Callback for selector subscription
static void selectorValidationCallback(const char* label, uint16_t value) {
    for (size_t i = 0; i < numValidatedSelectors; ++i) {
        if (strcmp(validatedSelectors[i].label, label) == 0) {
            validatedSelectors[i].lastSimValue = value;
            return;
        }
    }
}

void forceResync() {
    forcePanelResyncNow = true;
    frameCounter = 0;

}

void DCSBIOSBridge_setup() {

    // If CDC ON BOOT means Serial has started already, so, we'll stop it
    #if (ARDUINO_USB_CDC_ON_BOOT == 1) 
        Serial.setDebugOutput(false); 
        Serial.setRxBufferSize(SERIAL_RX_BUFFER_SIZE);
        Serial.setTxTimeoutMs(SERIAL_TX_TIMEOUT);  // To avoid CDC getting stuck when SOCAT starts acting up
        Serial.setTimeout(SERIAL_TX_TIMEOUT);
        // Serial.enableReboot(false); // Should be set to false for PRODUCTION, true for development  

#if defined(ENABLE_CDC_EVENTS) && (ENABLE_CDC_EVENTS > 0)
        setupCDCEvents(); // Load CDC Events
#elif defined(ENABLE_HWCDC_EVENTS) && (ENABLE_HWCDC_EVENTS > 0)
		setupHWCDCEvents(); // Load Hardware CDC Events
#endif

		// Start Serial
		Serial.begin(115200); // Start Serial at 115200 baud
		debugPrintln("[CDC] Serial started at 115200 baud");
        debugPrintf("USB Mode is: %u", ARDUINO_USB_MODE);

    #endif

    // Register our Display buffers automatically (the ones starting with IFEI_)
    for (size_t i = 0; i < numCTDisplayBuffers; ++i) {
        const auto& e = CT_DisplayBuffers[i];
        if (registerDisplayBuffer(e.label, e.buffer, e.length, e.dirty, e.last)) {
            debugPrintf("[DISPLAY BUFFERS] Registered display buffer: %s (len=%u, ptr=%p, dirty=%p), last=%p\n", e.label, e.length, (void*)e.buffer, (void*)e.dirty, (void*)e.last);
        }
    }

	// Update our command history from InputMappings
    syncCommandHistoryFromInputMapping();

	// For selector validation after panel sync
    initializeSelectorValidation(); // [Selector sync validation] Register tracked selectors

 }

void DCSBIOSBridge_loop() { 

	// Check if sim is ready
    bool sr = simReady();
    if (!sr) {
        unsigned long now = millis();
        if (lastNotReadyPrint == 0 || now - lastNotReadyPrint >= 60000) {
            debugPrintln("[DCS] ❌ Sim not ready yet...");
            lastNotReadyPrint = now;
        }
    }
    else {
        lastNotReadyPrint = millis();
    }

    #if DEBUG_PERFORMANCE
    beginProfiling(PERF_DCSBIOS);
    #endif

#if USE_DCSBIOS_WIFI || USE_DCSBIOS_USB   
    #if DCS_USE_RINGBUFFER
        onDcsBiosUdpPacket(); // Only run when using a Ring Buffer for DCS Incoming frames
    #else
        // No need to do anything as udp.onPacket / or the HID onReport callback will automatically parse each packet received from DCS when no ring buffer is used.
    #endif
#else

    #if (ARDUINO_USB_CDC_ON_BOOT == 1)
        cdcRxReady = false;
        uint8_t b;
        while (Serial.available() > 0) {
            b = Serial.read();
            parseDcsBiosUdpPacket(&b, 1);
        }   
    #endif

#endif

    #if DEBUG_PERFORMANCE      
    endProfiling(PERF_DCSBIOS);
    #endif

    // Optional
    #if DCS_KEEP_ALIVE_ENABLED
    if (isModeSelectorDCS()) DCSBIOS_keepAlive();  // Still called only when in DCS mode   
    #endif

    // #if defined(SELECTOR_DWELL_MS) && (SELECTOR_DWELL_MS > 0)
    if (isModeSelectorDCS() && simReady()) flushBufferedDcsCommands();
    // #endif    

    if (aircraftNameReceivedAt != 0 && !panelsInitializedThisMission) {
        if (millis() - aircraftNameReceivedAt > MISSION_START_DEBOUNCE) {
            initPanels();
            panelsInitializedThisMission = true;
            forcePanelResyncNow = false;
            frameCounter = 0;
            debugPrintln("[SYNC] Normal mission start panel sync");
        }
    }
    else if (forcePanelResyncNow && mySniffer.isStreamAlive() && (frameCounter > 1) && (msSinceMissionStart() > MISSION_START_DEBOUNCE + 10) ) {
        initPanels();
        panelsInitializedThisMission = true;
        forcePanelResyncNow = false;
        frameCounter = 0;
        debugPrintln("[SYNC] Fallback: forced panel re-sync");
    }

    if (!isMissionRunning()) {
        panelsInitializedThisMission = false;
        aircraftNameReceivedAt = 0;
    }
    else {
        static bool wasPaused = false;
        static unsigned long streamStateChangedAt = 0;
        static bool debouncedPaused = false;

        const unsigned long DEBOUNCE_DOWN_MS = 2000; // Require 2s continuous "down" to declare PAUSED
        const unsigned long DEBOUNCE_UP_MS = 50;   // Require 50ms continuous "up" to declare RESUMED

        bool pausedRaw = !mySniffer.isStreamAlive();
        unsigned long now = millis();

        if (pausedRaw != wasPaused) {
            streamStateChangedAt = now;
            wasPaused = pausedRaw;
        }

        if (!debouncedPaused && pausedRaw && (now - streamStateChangedAt >= DEBOUNCE_DOWN_MS)) {
            debugPrintln("[MISSION PAUSED] DCSBIOS stream is NOT active");
            debouncedPaused = true;
			lastNotReadyPrint = 0; // Reset this so we get a print for sim not ready
        }
        else if (debouncedPaused && !pausedRaw && (now - streamStateChangedAt >= DEBOUNCE_UP_MS)) {
            debugPrintln("[MISSION RESUMED] DCSBIOS stream is UP again");
            debouncedPaused = false;
        }
    }
}

/*
// Fix for latest Adafruit TinyUSB with 3.2.0 Core. We are NOT using Adafruit's Library, this is just for testing
extern "C" bool __atomic_test_and_set(volatile void* ptr, int memorder) __attribute__((weak));
bool __atomic_test_and_set(volatile void* ptr, int memorder) {
  return false; // pretend the lock was not already set
}
*/