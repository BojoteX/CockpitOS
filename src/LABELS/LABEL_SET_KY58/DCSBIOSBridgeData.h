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
    uint16_t        maxPositions;
    uint8_t         lastReport[GAMEPAD_REPORT_SIZE];
    uint8_t         pendingReport[GAMEPAD_REPORT_SIZE];
    unsigned long   lastHidSendTime;
};

struct DcsOutputEntry { uint16_t addr, mask; uint8_t shift; uint16_t max_value; const char* label; ControlType controlType; };
static const DcsOutputEntry DcsOutputTable[] = {
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
    {0x74CC,0x0700,8,7,"KY58_FILL_SELECT",CT_SELECTOR},
    {0x74D8,0x0200,9,1,"KY58_FILL_SEL_PULL",CT_SELECTOR},
    {0x74CA,0xC000,14,3,"KY58_MODE_SELECT",CT_SELECTOR},
    {0x74CC,0x1800,11,2,"KY58_POWER_SELECT",CT_SELECTOR},
    {0x754E,0xFFFF,0,65535,"KY58_VOLUME",CT_ANALOG},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[2]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x754A, { &DcsOutputTable[0] }, 1 },
  { 0x74C8, { &DcsOutputTable[1], &DcsOutputTable[5] }, 2 },
  { 0x7544, { &DcsOutputTable[2] }, 1 },
  { 0x7548, { &DcsOutputTable[3] }, 1 },
  { 0x7546, { &DcsOutputTable[4] }, 1 },
  { 0x754C, { &DcsOutputTable[6] }, 1 },
  { 0x74CC, { &DcsOutputTable[7], &DcsOutputTable[10] }, 2 },
  { 0x74D8, { &DcsOutputTable[8] }, 1 },
  { 0x74CA, { &DcsOutputTable[9] }, 1 },
  { 0x754E, { &DcsOutputTable[11] }, 1 },
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
  { 0x74C8, &dcsAddressTable[1] },
  {0xFFFF, nullptr},
  { 0x74CA, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  { 0x74CC, &dcsAddressTable[6] },
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
  { 0x74D8, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x7544, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x7546, &dcsAddressTable[4] },
  {0xFFFF, nullptr},
  { 0x7548, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  { 0x754A, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  { 0x754C, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  { 0x754E, &dcsAddressTable[9] },
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
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "CHART_DIMMER_DEC","CHART_DIMMER",0,"variable_step",0,"DEC" },
    { "CHART_DIMMER_INC","CHART_DIMMER",1,"variable_step",0,"INC" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",1,"DAY" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",1,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",1,"NVG" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "CONSOLES_DIMMER_DEC","CONSOLES_DIMMER",0,"variable_step",0,"DEC" },
    { "CONSOLES_DIMMER_INC","CONSOLES_DIMMER",1,"variable_step",0,"INC" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER_DEC","FLOOD_DIMMER",0,"variable_step",0,"DEC" },
    { "FLOOD_DIMMER_INC","FLOOD_DIMMER",1,"variable_step",0,"INC" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER_DEC","INST_PNL_DIMMER",0,"variable_step",0,"DEC" },
    { "INST_PNL_DIMMER_INC","INST_PNL_DIMMER",1,"variable_step",0,"INC" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",2,"OFF" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",2,"TEST" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "WARN_CAUTION_DIMMER_DEC","WARN_CAUTION_DIMMER",0,"variable_step",0,"DEC" },
    { "WARN_CAUTION_DIMMER_INC","WARN_CAUTION_DIMMER",1,"variable_step",0,"INC" },
    { "KY58_FILL_SELECT_Z_ALL","KY58_FILL_SELECT",0,"selector",3,"Z_ALL" },
    { "KY58_FILL_SELECT_6","KY58_FILL_SELECT",1,"selector",3,"6" },
    { "KY58_FILL_SELECT_5","KY58_FILL_SELECT",2,"selector",3,"5" },
    { "KY58_FILL_SELECT_4","KY58_FILL_SELECT",3,"selector",3,"4" },
    { "KY58_FILL_SELECT_3","KY58_FILL_SELECT",4,"selector",3,"3" },
    { "KY58_FILL_SELECT_2","KY58_FILL_SELECT",5,"selector",3,"2" },
    { "KY58_FILL_SELECT_1","KY58_FILL_SELECT",6,"selector",3,"1" },
    { "KY58_FILL_SELECT_Z_1-5","KY58_FILL_SELECT",7,"selector",3,"Z_1-5" },
    { "KY58_FILL_SELECT_DEC","KY58_FILL_SELECT",0,"fixed_step",0,"DEC" },
    { "KY58_FILL_SELECT_INC","KY58_FILL_SELECT",1,"fixed_step",0,"INC" },
    { "KY58_FILL_SEL_PULL_POS0","KY58_FILL_SEL_PULL",0,"selector",6,"POS0" },
    { "KY58_FILL_SEL_PULL_POS1","KY58_FILL_SEL_PULL",1,"selector",6,"POS1" },
    { "KY58_MODE_SELECT_RV","KY58_MODE_SELECT",0,"selector",4,"RV" },
    { "KY58_MODE_SELECT_LD","KY58_MODE_SELECT",1,"selector",4,"LD" },
    { "KY58_MODE_SELECT_C","KY58_MODE_SELECT",2,"selector",4,"C" },
    { "KY58_MODE_SELECT_P","KY58_MODE_SELECT",3,"selector",4,"P" },
    { "KY58_MODE_SELECT_DEC","KY58_MODE_SELECT",0,"fixed_step",0,"DEC" },
    { "KY58_MODE_SELECT_INC","KY58_MODE_SELECT",1,"fixed_step",0,"INC" },
    { "KY58_POWER_SELECT_TD","KY58_POWER_SELECT",0,"selector",5,"TD" },
    { "KY58_POWER_SELECT_ON","KY58_POWER_SELECT",1,"selector",5,"ON" },
    { "KY58_POWER_SELECT_OFF","KY58_POWER_SELECT",2,"selector",5,"OFF" },
    { "KY58_VOLUME","KY58_VOLUME",65535,"analog",0,"LEVEL" },
    { "KY58_VOLUME_DEC","KY58_VOLUME",0,"variable_step",0,"DEC" },
    { "KY58_VOLUME_INC","KY58_VOLUME",1,"variable_step",0,"INC" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 1, 0,   0, false, 0, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "KY58_FILL_SELECT", 0, 0, true, 3, 0,   0, false, 0, {0}, {0}, 0 },
    { "KY58_FILL_SEL_PULL", 0, 0, true, 6, 0,   0, false, 0, {0}, {0}, 0 },
    { "KY58_MODE_SELECT", 0, 0, true, 4, 0,   0, false, 0, {0}, {0}, 0 },
    { "KY58_POWER_SELECT", 0, 0, true, 5, 0,   0, false, 0, {0}, {0}, 0 },
    { "KY58_VOLUME", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 2, 0,   0, false, 0, {0}, {0}, 0 },
    { "WARN_CAUTION_DIMMER", 0, 0, false, 0, 0,   0, false, 0, {0}, {0}, 0 },
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
