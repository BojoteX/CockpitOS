// Auto-generated DCSBIOS Bridge Data (JSON‑only) - DO NOT EDIT
#pragma once

#define DCSBIOS_ACFT_NAME "AH-64D"
static constexpr const char* DCSBIOS_AIRCRAFT_NAME = "AH-64D";

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
    {0x873E,0x2000,13,1,"PLT_CMWS_AFT_LEFT_BRT_L",CT_METADATA},
    {0x874C,0x0200,9,1,"PLT_CMWS_AFT_LEFT_DIM_L",CT_METADATA},
    {0x873E,0x4000,14,1,"PLT_CMWS_AFT_RIGHT_BRT_L",CT_METADATA},
    {0x874C,0x0400,10,1,"PLT_CMWS_AFT_RIGHT_DIM_L",CT_METADATA},
    {0x8712,0x0020,5,1,"PLT_CMWS_ARM",CT_SELECTOR},
    {0x8758,0x00FF,0,3,"PLT_CMWS_BIT_LINE_1",CT_DISPLAY},
    {0x8759,0x00FF,0,3,"PLT_CMWS_BIT_LINE_1",CT_DISPLAY},
    {0x875A,0x00FF,0,3,"PLT_CMWS_BIT_LINE_1",CT_DISPLAY},
    {0x875C,0x00FF,0,4,"PLT_CMWS_BIT_LINE_2",CT_DISPLAY},
    {0x875D,0x00FF,0,4,"PLT_CMWS_BIT_LINE_2",CT_DISPLAY},
    {0x875E,0x00FF,0,4,"PLT_CMWS_BIT_LINE_2",CT_DISPLAY},
    {0x875F,0x00FF,0,4,"PLT_CMWS_BIT_LINE_2",CT_DISPLAY},
    {0x8712,0x0080,7,1,"PLT_CMWS_BYPASS",CT_SELECTOR},
    {0x8754,0x00FF,0,3,"PLT_CMWS_CHAFF_COUNT",CT_DISPLAY},
    {0x8755,0x00FF,0,3,"PLT_CMWS_CHAFF_COUNT",CT_DISPLAY},
    {0x8756,0x00FF,0,3,"PLT_CMWS_CHAFF_COUNT",CT_DISPLAY},
    {0x8752,0x00FF,0,1,"PLT_CMWS_CHAFF_LETTER",CT_DISPLAY},
    {0x873E,0x0400,10,1,"PLT_CMWS_D_BRT_L",CT_METADATA},
    {0x873E,0x0800,11,1,"PLT_CMWS_D_DIM_L",CT_METADATA},
    {0x874E,0x00FF,0,3,"PLT_CMWS_FLARE_COUNT",CT_DISPLAY},
    {0x874F,0x00FF,0,3,"PLT_CMWS_FLARE_COUNT",CT_DISPLAY},
    {0x8750,0x00FF,0,3,"PLT_CMWS_FLARE_COUNT",CT_DISPLAY},
    {0x874C,0x00FF,0,1,"PLT_CMWS_FLARE_LETTER",CT_DISPLAY},
    {0x873E,0x1000,12,1,"PLT_CMWS_FWD_LEFT_BRT_L",CT_METADATA},
    {0x874C,0x0100,8,1,"PLT_CMWS_FWD_LEFT_DIM_L",CT_METADATA},
    {0x873E,0x8000,15,1,"PLT_CMWS_FWD_RIGHT_BRT_L",CT_METADATA},
    {0x874C,0x0800,11,1,"PLT_CMWS_FWD_RIGHT_DIM_L",CT_METADATA},
    {0x8712,0x0200,9,1,"PLT_CMWS_JETT",CT_SELECTOR},
    {0x8712,0x0100,8,1,"PLT_CMWS_JETT_CVR",CT_SELECTOR},
    {0x871A,0xFFFF,0,65535,"PLT_CMWS_LAMP",CT_ANALOG},
    {0x8712,0x0040,6,1,"PLT_CMWS_MODE",CT_SELECTOR},
    {0x8748,0x00FF,0,4,"PLT_CMWS_PAGE",CT_DISPLAY},
    {0x8749,0x00FF,0,4,"PLT_CMWS_PAGE",CT_DISPLAY},
    {0x874A,0x00FF,0,4,"PLT_CMWS_PAGE",CT_DISPLAY},
    {0x874B,0x00FF,0,4,"PLT_CMWS_PAGE",CT_DISPLAY},
    {0x873E,0x0100,8,1,"PLT_CMWS_R_BRT_L",CT_METADATA},
    {0x873E,0x0200,9,1,"PLT_CMWS_R_DIM_L",CT_METADATA},
    {0x8718,0xFFFF,0,65535,"PLT_CMWS_VOL",CT_ANALOG},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[8]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x873E, { &DcsOutputTable[0], &DcsOutputTable[2], &DcsOutputTable[17], &DcsOutputTable[18], &DcsOutputTable[23], &DcsOutputTable[25], &DcsOutputTable[35], &DcsOutputTable[36] }, 8 },
  { 0x874C, { &DcsOutputTable[1], &DcsOutputTable[3], &DcsOutputTable[22], &DcsOutputTable[24], &DcsOutputTable[26] }, 5 },
  { 0x8712, { &DcsOutputTable[4], &DcsOutputTable[12], &DcsOutputTable[27], &DcsOutputTable[28], &DcsOutputTable[30] }, 5 },
  { 0x8758, { &DcsOutputTable[5] }, 1 },
  { 0x8759, { &DcsOutputTable[6] }, 1 },
  { 0x875A, { &DcsOutputTable[7] }, 1 },
  { 0x875C, { &DcsOutputTable[8] }, 1 },
  { 0x875D, { &DcsOutputTable[9] }, 1 },
  { 0x875E, { &DcsOutputTable[10] }, 1 },
  { 0x875F, { &DcsOutputTable[11] }, 1 },
  { 0x8754, { &DcsOutputTable[13] }, 1 },
  { 0x8755, { &DcsOutputTable[14] }, 1 },
  { 0x8756, { &DcsOutputTable[15] }, 1 },
  { 0x8752, { &DcsOutputTable[16] }, 1 },
  { 0x874E, { &DcsOutputTable[19] }, 1 },
  { 0x874F, { &DcsOutputTable[20] }, 1 },
  { 0x8750, { &DcsOutputTable[21] }, 1 },
  { 0x871A, { &DcsOutputTable[29] }, 1 },
  { 0x8748, { &DcsOutputTable[31] }, 1 },
  { 0x8749, { &DcsOutputTable[32] }, 1 },
  { 0x874A, { &DcsOutputTable[33] }, 1 },
  { 0x874B, { &DcsOutputTable[34] }, 1 },
  { 0x8718, { &DcsOutputTable[37] }, 1 },
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
  { 0x873E, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x8712, &dcsAddressTable[2] },
  { 0x8748, &dcsAddressTable[18] },
  { 0x8749, &dcsAddressTable[19] },
  { 0x874A, &dcsAddressTable[20] },
  { 0x874B, &dcsAddressTable[21] },
  { 0x874C, &dcsAddressTable[1] },
  { 0x8718, &dcsAddressTable[22] },
  { 0x874E, &dcsAddressTable[14] },
  { 0x874F, &dcsAddressTable[15] },
  { 0x8750, &dcsAddressTable[16] },
  { 0x871A, &dcsAddressTable[17] },
  { 0x8752, &dcsAddressTable[13] },
  {0xFFFF, nullptr},
  { 0x8754, &dcsAddressTable[10] },
  { 0x8755, &dcsAddressTable[11] },
  { 0x8756, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  { 0x8758, &dcsAddressTable[3] },
  { 0x8759, &dcsAddressTable[4] },
  { 0x875A, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  { 0x875C, &dcsAddressTable[6] },
  { 0x875D, &dcsAddressTable[7] },
  { 0x875E, &dcsAddressTable[8] },
  { 0x875F, &dcsAddressTable[9] },
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
    { "PLT_CMWS_ARM_SAFE","PLT_CMWS_ARM",0,"selector",1,"SAFE" },
    { "PLT_CMWS_ARM_ARM","PLT_CMWS_ARM",1,"selector",1,"ARM" },
    { "PLT_CMWS_BYPASS_AUTO","PLT_CMWS_BYPASS",0,"selector",2,"AUTO" },
    { "PLT_CMWS_BYPASS_BYPASS","PLT_CMWS_BYPASS",1,"selector",2,"BYPASS" },
    { "PLT_CMWS_JETT_POS0","PLT_CMWS_JETT",0,"selector",6,"POS0" },
    { "PLT_CMWS_JETT_POS1","PLT_CMWS_JETT",1,"selector",6,"POS1" },
    { "PLT_CMWS_JETT_CVR_CLOSE","PLT_CMWS_JETT_CVR",0,"selector",3,"CLOSE" },
    { "PLT_CMWS_JETT_CVR_OPEN","PLT_CMWS_JETT_CVR",1,"selector",3,"OPEN" },
    { "PLT_CMWS_LAMP","PLT_CMWS_LAMP",65535,"analog",0,"LEVEL" },
    { "PLT_CMWS_LAMP_DEC","PLT_CMWS_LAMP",0,"variable_step",0,"DEC" },
    { "PLT_CMWS_LAMP_INC","PLT_CMWS_LAMP",1,"variable_step",0,"INC" },
    { "PLT_CMWS_MODE_NAV","PLT_CMWS_MODE",0,"selector",4,"NAV" },
    { "PLT_CMWS_MODE_CMWS","PLT_CMWS_MODE",1,"selector",4,"CMWS" },
    { "PLT_CMWS_PW_TEST","PLT_CMWS_PW",0,"3pos_2command_switch_openclose",5,"TEST" },
    { "PLT_CMWS_PW_ON","PLT_CMWS_PW",1,"3pos_2command_switch_openclose",5,"ON" },
    { "PLT_CMWS_PW_OFF","PLT_CMWS_PW",2,"3pos_2command_switch_openclose",5,"OFF" },
    { "PLT_CMWS_VOL","PLT_CMWS_VOL",65535,"analog",0,"LEVEL" },
    { "PLT_CMWS_VOL_DEC","PLT_CMWS_VOL",0,"variable_step",0,"DEC" },
    { "PLT_CMWS_VOL_INC","PLT_CMWS_VOL",1,"variable_step",0,"INC" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)
struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };
static const SelectorHashEntry selectorHashTable[53] = {
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_PW", 2, &SelectorMap[15]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_JETT", 1, &SelectorMap[5]},
  {"PLT_CMWS_PW", 1, &SelectorMap[14]},
  {"PLT_CMWS_LAMP", 0, &SelectorMap[9]},
  {"PLT_CMWS_VOL", 65535, &SelectorMap[16]},
  {"PLT_CMWS_VOL", 0, &SelectorMap[17]},
  {"PLT_CMWS_BYPASS", 1, &SelectorMap[3]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_LAMP", 65535, &SelectorMap[8]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_LAMP", 1, &SelectorMap[10]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_VOL", 1, &SelectorMap[18]},
  {"PLT_CMWS_MODE", 1, &SelectorMap[12]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_PW", 0, &SelectorMap[13]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_JETT_CVR", 0, &SelectorMap[6]},
  {"PLT_CMWS_MODE", 0, &SelectorMap[11]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_BYPASS", 0, &SelectorMap[2]},
  {"PLT_CMWS_JETT_CVR", 1, &SelectorMap[7]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_JETT", 0, &SelectorMap[4]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"PLT_CMWS_ARM", 1, &SelectorMap[1]},
  {"PLT_CMWS_ARM", 0, &SelectorMap[0]},
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
    { "PLT_CMWS_ARM", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "PLT_CMWS_BYPASS", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "PLT_CMWS_JETT", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "PLT_CMWS_JETT_CVR", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "PLT_CMWS_LAMP", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "PLT_CMWS_MODE", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "PLT_CMWS_PW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "PLT_CMWS_VOL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
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
    { "PLT CMWS", "PLT_CMWS_BIT_LINE_1", 0x8758, 3 },
    { "PLT CMWS", "PLT_CMWS_BIT_LINE_2", 0x875C, 4 },
    { "PLT CMWS", "PLT_CMWS_CHAFF_COUNT", 0x8754, 3 },
    { "PLT CMWS", "PLT_CMWS_CHAFF_LETTER", 0x8752, 1 },
    { "PLT CMWS", "PLT_CMWS_FLARE_COUNT", 0x874E, 3 },
    { "PLT CMWS", "PLT_CMWS_FLARE_LETTER", 0x874C, 1 },
    { "PLT CMWS", "PLT_CMWS_PAGE", 0x8748, 4 },
};
static constexpr size_t numDisplayFields = sizeof(displayFields)/sizeof(displayFields[0]);

struct DisplayFieldHashEntry { const char* label; const DisplayFieldDef* def; };
static const DisplayFieldHashEntry displayFieldsByLabel[17] = {
  {nullptr, nullptr},
  {"PLT_CMWS_CHAFF_COUNT", &displayFields[2]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_FLARE_LETTER", &displayFields[5]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_CHAFF_LETTER", &displayFields[3]},
  {"PLT_CMWS_FLARE_COUNT", &displayFields[4]},
  {nullptr, nullptr},
  {"PLT_CMWS_BIT_LINE_1", &displayFields[0]},
  {"PLT_CMWS_BIT_LINE_2", &displayFields[1]},
  {"PLT_CMWS_PAGE", &displayFields[6]},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);

inline const DisplayFieldDef* findDisplayFieldByLabel(const char* label) {
  uint16_t startH = labelHash(label) % 17;
  for (uint16_t i = 0; i < 17; ++i) {
    uint16_t idx = (startH + i >= 17) ? (startH + i - 17) : (startH + i);
    const auto& entry = displayFieldsByLabel[idx];
    if (!entry.label) return nullptr;
    if (strcmp(entry.label, label) == 0) return entry.def;
  }
  return nullptr;
}


// --- Auto-generated tracked metadata fields ---
struct MetadataState {
    const char* label;
    uint16_t    value;
};
static MetadataState metadataStates[] = {
    { "PLT_CMWS_AFT_LEFT_BRT_L", 0 }, // PLT CMWS: Aft left sector lights, bright (orange)
    { "PLT_CMWS_AFT_LEFT_DIM_L", 0 }, // PLT CMWS: Aft left sector lights, dim (orange)
    { "PLT_CMWS_AFT_RIGHT_BRT_L", 0 }, // PLT CMWS: Aft right sector lights, bright (orange)
    { "PLT_CMWS_AFT_RIGHT_DIM_L", 0 }, // PLT CMWS: Aft right sector lights, dim (orange)
    { "PLT_CMWS_D_BRT_L", 0 }, // PLT CMWS: D light, bright (orange)
    { "PLT_CMWS_D_DIM_L", 0 }, // PLT CMWS: D light, dim (orange)
    { "PLT_CMWS_FWD_LEFT_BRT_L", 0 }, // PLT CMWS: Forward left sector lights, bright (orange)
    { "PLT_CMWS_FWD_LEFT_DIM_L", 0 }, // PLT CMWS: Forward left sector lights, dim (orange)
    { "PLT_CMWS_FWD_RIGHT_BRT_L", 0 }, // PLT CMWS: Forward right sector lights, bright (orange)
    { "PLT_CMWS_FWD_RIGHT_DIM_L", 0 }, // PLT CMWS: Forward right sector lights, dim (orange)
    { "PLT_CMWS_R_BRT_L", 0 }, // PLT CMWS: R light, bright (orange)
    { "PLT_CMWS_R_DIM_L", 0 }, // PLT CMWS: R light, dim (orange)
};
static const size_t numMetadataStates = sizeof(metadataStates)/sizeof(metadataStates[0]);

struct MetadataHashEntry { const char* label; MetadataState* state; };
static MetadataHashEntry metadataHashTable[29] = {
  {"PLT_CMWS_AFT_RIGHT_BRT_L", &metadataStates[2]},
  {"PLT_CMWS_FWD_RIGHT_DIM_L", &metadataStates[9]},
  {"PLT_CMWS_R_BRT_L", &metadataStates[10]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_AFT_RIGHT_DIM_L", &metadataStates[3]},
  {"PLT_CMWS_D_DIM_L", &metadataStates[5]},
  {"PLT_CMWS_FWD_LEFT_BRT_L", &metadataStates[6]},
  {"PLT_CMWS_R_DIM_L", &metadataStates[11]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_FWD_LEFT_DIM_L", &metadataStates[7]},
  {"PLT_CMWS_FWD_RIGHT_BRT_L", &metadataStates[8]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {"PLT_CMWS_AFT_LEFT_BRT_L", &metadataStates[0]},
  {"PLT_CMWS_D_BRT_L", &metadataStates[4]},
  {"PLT_CMWS_AFT_LEFT_DIM_L", &metadataStates[1]},
};

constexpr uint16_t metadataHash(const char* s) { return labelHash(s); }

inline MetadataState* findMetadataState(const char* label) {
    uint16_t startH = metadataHash(label) % 29;
    for (uint16_t i = 0; i < 29; ++i) {
        uint16_t idx = (startH + i >= 29) ? (startH + i - 29) : (startH + i);
        const auto& entry = metadataHashTable[idx];
        if (!entry.label) return nullptr;
        if (strcmp(entry.label, label) == 0) return entry.state;
    }
    return nullptr;
}
