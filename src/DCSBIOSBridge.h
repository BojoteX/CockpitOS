// DCSBIOSBridge.h

#pragma once

#include "../Mappings.h"
#include "../Pins.h"

void DCSBIOSBridge_postSetup();
void initPanels();
void DCSBIOS_forceMissionStop();
void forceResync();
bool simReady();

// --- Prototypes for Selector Validation/Sync ---
void validateSelectorSync();
void initializeSelectorValidation();

// ───── Subscribe to metadata changes ─────
#define MAX_METADATA_SUBSCRIPTIONS 32
struct MetadataSubscription {
    const char* label;
    void (*callback)(const char* label, uint16_t value);
};
extern MetadataSubscription metadataSubscriptions[MAX_METADATA_SUBSCRIPTIONS];
extern size_t metadataSubscriptionCount;
bool subscribeToMetadataChange(const char* label, void (*callback)(const char* label, uint16_t value));

// ───── Subscribe to display changes ─────
#define MAX_DISPLAY_SUBSCRIPTIONS 32
struct DisplaySubscription {
    const char* label;
    void (*callback)(const char* label, const char* value);
};
extern DisplaySubscription displaySubscriptions[MAX_DISPLAY_SUBSCRIPTIONS];
extern size_t displaySubscriptionCount;
bool subscribeToDisplayChange(const char* label, void (*callback)(const char* label, const char* value));

// ───── Subscribe to selector changes ─────
#define MAX_SELECTOR_SUBSCRIPTIONS 32
struct SelectorSubscription {
    const char* label;
    void (*callback)(const char* label, uint16_t value);
};
extern SelectorSubscription selectorSubscriptions[MAX_SELECTOR_SUBSCRIPTIONS];
extern size_t selectorSubscriptionCount;
bool subscribeToSelectorChange(const char* label, void (*callback)(const char* label, uint16_t value));

// ───── Subscribe to LED changes ─────
#define MAX_LED_SUBSCRIPTIONS 32
struct LedSubscription {
    const char* label;
    void (*callback)(const char* label, uint16_t value, uint16_t max_value);
};
extern LedSubscription ledSubscriptions[MAX_LED_SUBSCRIPTIONS];
extern size_t ledSubscriptionCount;
bool subscribeToLedChange(const char* label, void (*callback)(const char* label, uint16_t value, uint16_t max_value));

// Semantic aliases — zero-cost, clearer intent for non-LED outputs (gauges, solenoids, servos, etc.)
inline bool subscribeToOutputChange(const char* label, void (*cb)(const char*, uint16_t, uint16_t)) {
    return subscribeToLedChange(label, cb);
}
inline bool subscribeToGaugeChange(const char* label, void (*cb)(const char*, uint16_t, uint16_t)) {
    return subscribeToLedChange(label, cb);
}

// ───── Query data from anywhere in our program for custom actions (e.g IFEI backlight) ─────
uint16_t getMetadataValue(const char* label);
uint16_t getLastKnownState(const char* label);
const char* getLastValueForDisplayLabel(const char* label);

// declare references to the one true table
CommandHistoryEntry*  dcsbios_getCommandHistory();
size_t                dcsbios_getCommandHistorySize();

// Throttling logic
bool throttleIdenticalValue(const char* label, CommandHistoryEntry &e, uint16_t value, bool force);
CommandHistoryEntry* findCmdEntry(const char* label);

// ───── Optional Helpers ─────
bool applyThrottle(CommandHistoryEntry &e, const char* label, uint16_t value, bool force=false);

// ───── Optional Replay Support ─────
void DcsbiosProtocolReplay();

// ───── Command Sender ─────
void sendDCSBIOSCommand(const char* label, uint16_t value, bool force = false);
void sendCommand(const char* label, const char* value, bool silent = false); // Overload for use with keep-alives
// void sendCommand(const char* label, const char* value);

// ───── Tracked State Accessors via CommandHistory ─────
inline bool isCoverOpen(const char* label) {
    CommandHistoryEntry* e = findCmdEntry(label);
    return e && e->lastValue > 0;
}
inline bool isToggleOn(const char* label) { return isCoverOpen(label); }
#define setCoverState(label,v) sendDCSBIOSCommand(label, v ? 1 : 0, true)
#define setToggleState(label,v) setCoverState(label,v)

// ───── Hooks ─────
void onAircraftName(const char* str);
void onLedChange(const char* label, uint16_t value, uint16_t max_value);
void onSelectorChange(const char* label, unsigned int newValue);
void onMetaDataChange(const char* label, unsigned int value);
void onDisplayChange(const char* label, const char* value);
void dumpAllMetadata();

void HID_sendRawReport(bool force);
bool tryToSendDcsBiosMessage(const char* msg, const char* arg);
bool isSerialConnected();
// flushBufferedDcsCommands() is file-local to DCSBIOSBridge.cpp
bool replayData();
void DCSBIOSBridge_setup();
void DCSBIOSBridge_loop();
void DCSBIOS_keepAlive();
void DcsBiosTask(void* param);
bool cdcEnsureRxReady(uint32_t timeoutMs = CDC_TIMEOUT_RX_TX); // make sure we are ACTIVELY receiving a stream
bool cdcEnsureTxReady(uint32_t timeoutMs = CDC_TIMEOUT_RX_TX); // make sure we are ACTIVELY receiving a stream

// ───── Display Logic for strings (e.g IFEI, HUD, UFC etc) ─────
struct RegisteredDisplayBuffer {
    const char* label;
    char* buffer;
    uint8_t length;
    bool* dirtyFlag;
    char* last;
    uint8_t updatedBytes[48];  // Large enough for your biggest field
};
bool registerDisplayBuffer(const char* label, char* buf, uint8_t len, bool* dirtyFlag, char* last);
void onDisplayChange(const char* label, const char* value);
void parseDcsBiosUdpPacket(const uint8_t* data, size_t len);

// ───── DCSBIOS WiFi implementation ─────
void onDcsBiosUdpPacket();

// Parse incoming data
void DCSBIOSBridge_feedBytes(const uint8_t* data, size_t len);

// V2.3 RS485 Slave: Process single export byte (for ring buffer byte-by-byte processing)
void processDcsBiosExportByte(uint8_t c);

// New Anon String Buffers

// --- Anonymous multi-word string field handler (for aircraft name, etc) ---
typedef void (*AnonStringChangeCallback)(const char* value);
struct AnonymousStringBuffer {
    uint16_t base_addr;
    uint8_t  length;
    char*    buffer;
    char*    last;
    bool*    dirty;
    AnonStringChangeCallback onChange;
};
void updateAnonymousStringField(AnonymousStringBuffer& field, uint16_t addr, uint16_t value);
void commitAnonymousStringField(AnonymousStringBuffer& field);