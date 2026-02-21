// Auto-generated DCSBIOS Bridge Data (JSON‑only) - DO NOT EDIT
#pragma once

#define DCSBIOS_ACFT_NAME "FA-18C_hornet"
static constexpr const char* DCSBIOS_AIRCRAFT_NAME = "FA-18C_hornet";

enum ControlType : uint8_t {
  CT_LED,
  CT_ANALOG,
  CT_GAUGE,
  CT_SELECTOR,
  CT_DISPLAY,
  CT_METADATA
};

// --- Command History Tracking Struct ---
struct CommandHistoryEntry {
    const char*     label;
    uint16_t        lastValue;
    unsigned long   lastSendTime;
    bool            isSelector;
    uint16_t        group;
    uint16_t        pendingValue;
    unsigned long   lastChangeTime;
    bool            hasPending;
    uint8_t         lastReport[GAMEPAD_REPORT_SIZE];
    uint8_t         pendingReport[GAMEPAD_REPORT_SIZE];
    unsigned long   lastHidSendTime;
};

struct DcsOutputEntry { uint16_t addr, mask; uint8_t shift; uint16_t max_value; const char* label; ControlType controlType; };
static const DcsOutputEntry DcsOutputTable[] = {
    {0x74C2,0x0100,8,1,"APU_CONTROL_SW",CT_SELECTOR},
    {0x74C2,0x0800,11,1,"APU_READY_LT",CT_LED},
    {0x74C2,0x0600,9,2,"ENGINE_CRANK_SW",CT_SELECTOR},
    {0x7588,0xFFFF,0,65535,"INT_THROTTLE_LEFT",CT_GAUGE},
    {0x758A,0xFFFF,0,65535,"INT_THROTTLE_RIGHT",CT_GAUGE},
    {0x74D4,0x0400,10,1,"THROTTLE_ATC_SW",CT_SELECTOR},
    {0x74D2,0x2000,13,1,"THROTTLE_CAGE_BTN",CT_SELECTOR},
    {0x74D2,0xC000,14,2,"THROTTLE_DISP_SW",CT_SELECTOR},
    {0x74D4,0x1000,12,1,"THROTTLE_EXT_L_SW",CT_SELECTOR},
    {0x74D4,0x0800,11,1,"THROTTLE_FOV_SEL_SW",CT_SELECTOR},
    {0x7522,0xFFFF,0,65535,"THROTTLE_FRICTION",CT_ANALOG},
    {0x7556,0xFFFF,0,65535,"THROTTLE_RADAR_ELEV",CT_ANALOG},
    {0x74D4,0x0300,8,2,"THROTTLE_SPEED_BRK",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[4]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x74C2, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[2] }, 3 },
  { 0x7588, { &DcsOutputTable[3] }, 1 },
  { 0x758A, { &DcsOutputTable[4] }, 1 },
  { 0x74D4, { &DcsOutputTable[5], &DcsOutputTable[8], &DcsOutputTable[9], &DcsOutputTable[12] }, 4 },
  { 0x74D2, { &DcsOutputTable[6], &DcsOutputTable[7] }, 2 },
  { 0x7522, { &DcsOutputTable[10] }, 1 },
  { 0x7556, { &DcsOutputTable[11] }, 1 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[53] = {
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74D2, &dcsAddressTable[4] },
  {0xFFFF, nullptr},
  { 0x74D4, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7588, &dcsAddressTable[1] },
  {0xFFFF, nullptr},
  { 0x758A, &dcsAddressTable[2] },
  { 0x7556, &dcsAddressTable[6] },
  { 0x7522, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74C2, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
};

// Simple address hash (modulo)
constexpr uint16_t addrHash(uint16_t addr) {
  return addr % 53;
}

inline const AddressEntry* findDcsOutputEntries(uint16_t addr) {
  uint16_t startH = addrHash(addr);
  for (uint16_t i = 0; i < 53; ++i) {
    uint16_t idx = (startH + i >= 53) ? (startH + i - 53) : (startH + i);
    const auto& entry = dcsAddressHashTable[idx];
    if (entry.addr == 0xFFFF) return nullptr;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "APU_CONTROL_SW_OFF","APU_CONTROL_SW",0,"selector",1,"OFF" },
    { "APU_CONTROL_SW_ON","APU_CONTROL_SW",1,"selector",1,"ON" },
    { "ENGINE_CRANK_SW_RIGHT","ENGINE_CRANK_SW",0,"selector",2,"RIGHT" },
    { "ENGINE_CRANK_SW_OFF","ENGINE_CRANK_SW",1,"selector",2,"OFF" },
    { "ENGINE_CRANK_SW_LEFT","ENGINE_CRANK_SW",2,"selector",2,"LEFT" },
    { "THROTTLE_ATC_SW","THROTTLE_ATC_SW",1,"momentary",0,"PRESS" },
    { "THROTTLE_CAGE_BTN","THROTTLE_CAGE_BTN",1,"momentary",0,"PRESS" },
    { "THROTTLE_DISP_SW_FORWARD(CHAFF)","THROTTLE_DISP_SW",0,"selector",3,"FORWARD(CHAFF)" },
    { "THROTTLE_DISP_SW_CENTER(OFF)","THROTTLE_DISP_SW",1,"selector",3,"CENTER(OFF)" },
    { "THROTTLE_DISP_SW_AFT(FLARE)","THROTTLE_DISP_SW",2,"selector",3,"AFT(FLARE)" },
    { "THROTTLE_EXT_L_SW_OFF","THROTTLE_EXT_L_SW",0,"selector",4,"OFF" },
    { "THROTTLE_EXT_L_SW_ON","THROTTLE_EXT_L_SW",1,"selector",4,"ON" },
    { "THROTTLE_FOV_SEL_SW","THROTTLE_FOV_SEL_SW",1,"momentary",0,"PRESS" },
    { "THROTTLE_FRICTION","THROTTLE_FRICTION",65535,"analog",0,"LEVEL" },
    { "THROTTLE_FRICTION_DEC","THROTTLE_FRICTION",0,"variable_step",0,"DEC" },
    { "THROTTLE_FRICTION_INC","THROTTLE_FRICTION",1,"variable_step",0,"INC" },
    { "THROTTLE_RADAR_ELEV","THROTTLE_RADAR_ELEV",65535,"analog",0,"LEVEL" },
    { "THROTTLE_RADAR_ELEV_DEC","THROTTLE_RADAR_ELEV",0,"variable_step",0,"DEC" },
    { "THROTTLE_RADAR_ELEV_INC","THROTTLE_RADAR_ELEV",1,"variable_step",0,"INC" },
    { "THROTTLE_SPEED_BRK_RETRACT","THROTTLE_SPEED_BRK",0,"selector",5,"RETRACT" },
    { "THROTTLE_SPEED_BRK_OFF","THROTTLE_SPEED_BRK",1,"selector",5,"OFF" },
    { "THROTTLE_SPEED_BRK_EXTEND","THROTTLE_SPEED_BRK",2,"selector",5,"EXTEND" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)
struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };
static const SelectorHashEntry selectorHashTable[53] = {
  {"APU_CONTROL_SW", 1, &SelectorMap[1]},
  {nullptr, 0, nullptr},
  {"ENGINE_CRANK_SW", 2, &SelectorMap[4]},
  {"THROTTLE_RADAR_ELEV", 65535, &SelectorMap[16]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"THROTTLE_CAGE_BTN", 1, &SelectorMap[6]},
  {"THROTTLE_FOV_SEL_SW", 1, &SelectorMap[12]},
  {"THROTTLE_FRICTION", 65535, &SelectorMap[13]},
  {"APU_CONTROL_SW", 0, &SelectorMap[0]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"ENGINE_CRANK_SW", 1, &SelectorMap[3]},
  {"THROTTLE_SPEED_BRK", 1, &SelectorMap[20]},
  {"ENGINE_CRANK_SW", 0, &SelectorMap[2]},
  {"THROTTLE_RADAR_ELEV", 0, &SelectorMap[17]},
  {"THROTTLE_FRICTION", 1, &SelectorMap[15]},
  {"THROTTLE_FRICTION", 0, &SelectorMap[14]},
  {"THROTTLE_DISP_SW", 2, &SelectorMap[9]},
  {nullptr, 0, nullptr},
  {"THROTTLE_SPEED_BRK", 0, &SelectorMap[19]},
  {"THROTTLE_ATC_SW", 1, &SelectorMap[5]},
  {nullptr, 0, nullptr},
  {"THROTTLE_EXT_L_SW", 1, &SelectorMap[11]},
  {"THROTTLE_DISP_SW", 1, &SelectorMap[8]},
  {"THROTTLE_EXT_L_SW", 0, &SelectorMap[10]},
  {"THROTTLE_RADAR_ELEV", 1, &SelectorMap[18]},
  {"THROTTLE_DISP_SW", 0, &SelectorMap[7]},
  {"THROTTLE_SPEED_BRK", 2, &SelectorMap[21]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
};

// Composite hash: labelHash(dcsCommand) ^ (value * 7919)
inline const SelectorEntry* findSelectorByDcsAndValue(const char* dcsCommand, uint16_t value) {
  uint16_t startH = (labelHash(dcsCommand) ^ (value * 7919u)) % 53;
  for (uint16_t i = 0; i < 53; ++i) {
    uint16_t idx = (startH + i >= 53) ? (startH + i - 53) : (startH + i);
    const auto& entry = selectorHashTable[idx];
    if (!entry.dcsCommand) return nullptr;
    if (entry.value == value && strcmp(entry.dcsCommand, dcsCommand) == 0) return entry.entry;
  }
  return nullptr;
}


// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "APU_CONTROL_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "ENGINE_CRANK_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_ATC_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_CAGE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_DISP_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_EXT_L_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_FOV_SEL_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_FRICTION", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_RADAR_ELEV", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_SPEED_BRK", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
};
static const size_t commandHistorySize = sizeof(commandHistory)/sizeof(CommandHistoryEntry);

// --- Auto-generated display string field grouping ---
struct DisplayFieldDef {
    const char* panel;
    const char* label;
    uint16_t base_addr;
    uint8_t  length;
};
static const DisplayFieldDef displayFields[] = {
};
static constexpr size_t numDisplayFields = sizeof(displayFields)/sizeof(displayFields[0]);

struct DisplayFieldHashEntry { const char* label; const DisplayFieldDef* def; };
static const DisplayFieldHashEntry displayFieldsByLabel[2] = {
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);

inline const DisplayFieldDef* findDisplayFieldByLabel(const char* label) {
  uint16_t startH = labelHash(label) % 2;
  for (uint16_t i = 0; i < 2; ++i) {
    uint16_t idx = (startH + i >= 2) ? (startH + i - 2) : (startH + i);
    const auto& entry = displayFieldsByLabel[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.def;
  }
  return nullptr;
}

// No tracked metadata fields found
struct MetadataState { const char* label; uint16_t value; };
static MetadataState metadataStates[] __attribute__((unused)) = {};
static const size_t numMetadataStates __attribute__((unused)) = 0;
inline MetadataState* findMetadataState(const char*) { return nullptr; }
