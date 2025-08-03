// DCSBIOSBridge.h

#pragma once

#include "../Mappings.h"

void initPanels();
void DCSBIOS_forceMissionStop();
void forceResync();

// --- Prototypes for Selector Validation/Sync ---
void validateSelectorSync();
void initializeSelectorValidation();
static void selectorValidationCallback(const char* label, uint16_t value);

// ───── Subscribe to metadata changes ─────
#define MAX_METADATA_SUBSCRIPTIONS 32
struct MetadataSubscription {
    const char* label;
    void (*callback)(const char* label, uint16_t value);
};
static MetadataSubscription metadataSubscriptions[MAX_METADATA_SUBSCRIPTIONS];
static size_t metadataSubscriptionCount = 0;
bool subscribeToMetadataChange(const char* label, void (*callback)(const char* label, uint16_t value));

// ───── Subscribe to selector changes ─────
#define MAX_SELECTOR_SUBSCRIPTIONS 32
struct SelectorSubscription {
    const char* label;
    void (*callback)(const char* label, uint16_t value);
};
static SelectorSubscription selectorSubscriptions[MAX_SELECTOR_SUBSCRIPTIONS];
static size_t selectorSubscriptionCount = 0;
bool subscribeToSelectorChange(const char* label, void (*callback)(const char* label, uint16_t value));

// ───── Subscribe to LED changes ─────
#define MAX_LED_SUBSCRIPTIONS 32
struct LedSubscription {
    const char* label;
    void (*callback)(const char* label, uint16_t value, uint16_t max_value);
};
static LedSubscription ledSubscriptions[MAX_LED_SUBSCRIPTIONS];
static size_t ledSubscriptionCount = 0;
bool subscribeToLedChange(const char* label, void (*callback)(const char* label, uint16_t value, uint16_t max_value));

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
#define isCoverOpen(label)     (findCmdEntry(label) ? (findCmdEntry(label)->lastValue > 0) : false)
#define isToggleOn(label)      isCoverOpen(label)
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
static void flushBufferedDcsCommands();
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
void onDcsBiosUdpPacket(const uint8_t* data, size_t len);

// Parse incoming data
void DCSBIOSBridge_feedBytes(const uint8_t* data, size_t len);

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