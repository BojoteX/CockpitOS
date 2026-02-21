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
    {0x75A8,0x00FF,0,6,"HUD_ATC_NWS_ENGAGED",CT_DISPLAY},
    {0x75A9,0x00FF,0,6,"HUD_ATC_NWS_ENGAGED",CT_DISPLAY},
    {0x75AA,0x00FF,0,6,"HUD_ATC_NWS_ENGAGED",CT_DISPLAY},
    {0x75AB,0x00FF,0,6,"HUD_ATC_NWS_ENGAGED",CT_DISPLAY},
    {0x75AC,0x00FF,0,6,"HUD_ATC_NWS_ENGAGED",CT_DISPLAY},
    {0x75AD,0x00FF,0,6,"HUD_ATC_NWS_ENGAGED",CT_DISPLAY},
    {0x75A2,0x00FF,0,5,"HUD_LTDR",CT_DISPLAY},
    {0x75A3,0x00FF,0,5,"HUD_LTDR",CT_DISPLAY},
    {0x75A4,0x00FF,0,5,"HUD_LTDR",CT_DISPLAY},
    {0x75A5,0x00FF,0,5,"HUD_LTDR",CT_DISPLAY},
    {0x75A6,0x00FF,0,5,"HUD_LTDR",CT_DISPLAY},
    {0x74C8,0x3000,12,2,"FLIR_SW",CT_SELECTOR},
    {0x74CA,0x3800,11,7,"INS_SW",CT_SELECTOR},
    {0x74C8,0x8000,15,1,"LST_NFLR_SW",CT_SELECTOR},
    {0x74C8,0x4000,14,1,"LTD_R_SW",CT_SELECTOR},
    {0x74CA,0x0300,8,3,"RADAR_SW",CT_SELECTOR},
    {0x74CA,0x0400,10,1,"RADAR_SW_PULL",CT_SELECTOR},
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
  { 0x75A8, { &DcsOutputTable[3] }, 1 },
  { 0x75A9, { &DcsOutputTable[4] }, 1 },
  { 0x75AA, { &DcsOutputTable[5] }, 1 },
  { 0x75AB, { &DcsOutputTable[6] }, 1 },
  { 0x75AC, { &DcsOutputTable[7] }, 1 },
  { 0x75AD, { &DcsOutputTable[8] }, 1 },
  { 0x75A2, { &DcsOutputTable[9] }, 1 },
  { 0x75A3, { &DcsOutputTable[10] }, 1 },
  { 0x75A4, { &DcsOutputTable[11] }, 1 },
  { 0x75A5, { &DcsOutputTable[12] }, 1 },
  { 0x75A6, { &DcsOutputTable[13] }, 1 },
  { 0x74C8, { &DcsOutputTable[14], &DcsOutputTable[16], &DcsOutputTable[17] }, 3 },
  { 0x74CA, { &DcsOutputTable[15], &DcsOutputTable[18], &DcsOutputTable[19] }, 3 },
  { 0x7588, { &DcsOutputTable[20] }, 1 },
  { 0x758A, { &DcsOutputTable[21] }, 1 },
  { 0x74D4, { &DcsOutputTable[22], &DcsOutputTable[25], &DcsOutputTable[26], &DcsOutputTable[29] }, 4 },
  { 0x74D2, { &DcsOutputTable[23], &DcsOutputTable[24] }, 2 },
  { 0x7522, { &DcsOutputTable[27] }, 1 },
  { 0x7556, { &DcsOutputTable[28] }, 1 },
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
  { 0x74C8, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  { 0x74CA, &dcsAddressTable[13] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x75A2, &dcsAddressTable[7] },
  { 0x75A3, &dcsAddressTable[8] },
  { 0x75A4, &dcsAddressTable[9] },
  { 0x75A5, &dcsAddressTable[10] },
  { 0x75A6, &dcsAddressTable[11] },
  { 0x74D2, &dcsAddressTable[17] },
  { 0x75A8, &dcsAddressTable[1] },
  { 0x75A9, &dcsAddressTable[2] },
  { 0x75AA, &dcsAddressTable[3] },
  { 0x75AB, &dcsAddressTable[4] },
  { 0x75AC, &dcsAddressTable[5] },
  { 0x75AD, &dcsAddressTable[6] },
  { 0x74D4, &dcsAddressTable[16] },
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
  { 0x7588, &dcsAddressTable[14] },
  {0xFFFF, nullptr},
  { 0x758A, &dcsAddressTable[15] },
  { 0x7556, &dcsAddressTable[19] },
  { 0x7522, &dcsAddressTable[18] },
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
    { "FLIR_SW_OFF","FLIR_SW",0,"selector",3,"OFF" },
    { "FLIR_SW_STBY","FLIR_SW",1,"selector",3,"STBY" },
    { "FLIR_SW_ON","FLIR_SW",2,"selector",3,"ON" },
    { "INS_SW_TEST","INS_SW",0,"selector",4,"TEST" },
    { "INS_SW_GB","INS_SW",1,"selector",4,"GB" },
    { "INS_SW_GYRO","INS_SW",2,"selector",4,"GYRO" },
    { "INS_SW_IFA","INS_SW",3,"selector",4,"IFA" },
    { "INS_SW_NAV","INS_SW",4,"selector",4,"NAV" },
    { "INS_SW_GND","INS_SW",5,"selector",4,"GND" },
    { "INS_SW_CV","INS_SW",6,"selector",4,"CV" },
    { "INS_SW_OFF","INS_SW",7,"selector",4,"OFF" },
    { "INS_SW_DEC","INS_SW",0,"fixed_step",0,"DEC" },
    { "INS_SW_INC","INS_SW",1,"fixed_step",0,"INC" },
    { "LST_NFLR_SW_OFF","LST_NFLR_SW",0,"selector",5,"OFF" },
    { "LST_NFLR_SW_ON","LST_NFLR_SW",1,"selector",5,"ON" },
    { "LTD_R_SW_SAFE","LTD_R_SW",0,"selector",6,"SAFE" },
    { "LTD_R_SW_ARM","LTD_R_SW",1,"selector",6,"ARM" },
    { "RADAR_SW_EMERG(PULL)","RADAR_SW",0,"selector",7,"EMERG(PULL)" },
    { "RADAR_SW_OPR","RADAR_SW",1,"selector",7,"OPR" },
    { "RADAR_SW_STBY","RADAR_SW",2,"selector",7,"STBY" },
    { "RADAR_SW_OFF","RADAR_SW",3,"selector",7,"OFF" },
    { "RADAR_SW_DEC","RADAR_SW",0,"fixed_step",0,"DEC" },
    { "RADAR_SW_INC","RADAR_SW",1,"fixed_step",0,"INC" },
    { "RADAR_SW_PULL","RADAR_SW_PULL",1,"momentary",0,"PRESS" },
    { "THROTTLE_ATC_SW","THROTTLE_ATC_SW",1,"momentary",0,"PRESS" },
    { "THROTTLE_CAGE_BTN","THROTTLE_CAGE_BTN",1,"momentary",0,"PRESS" },
    { "THROTTLE_DISP_SW_FORWARD(CHAFF)","THROTTLE_DISP_SW",0,"selector",8,"FORWARD(CHAFF)" },
    { "THROTTLE_DISP_SW_CENTER(OFF)","THROTTLE_DISP_SW",1,"selector",8,"CENTER(OFF)" },
    { "THROTTLE_DISP_SW_AFT(FLARE)","THROTTLE_DISP_SW",2,"selector",8,"AFT(FLARE)" },
    { "THROTTLE_EXT_L_SW_OFF","THROTTLE_EXT_L_SW",0,"selector",9,"OFF" },
    { "THROTTLE_EXT_L_SW_ON","THROTTLE_EXT_L_SW",1,"selector",9,"ON" },
    { "THROTTLE_FOV_SEL_SW","THROTTLE_FOV_SEL_SW",1,"momentary",0,"PRESS" },
    { "THROTTLE_FRICTION","THROTTLE_FRICTION",65535,"analog",0,"LEVEL" },
    { "THROTTLE_FRICTION_DEC","THROTTLE_FRICTION",0,"variable_step",0,"DEC" },
    { "THROTTLE_FRICTION_INC","THROTTLE_FRICTION",1,"variable_step",0,"INC" },
    { "THROTTLE_RADAR_ELEV","THROTTLE_RADAR_ELEV",65535,"analog",0,"LEVEL" },
    { "THROTTLE_RADAR_ELEV_DEC","THROTTLE_RADAR_ELEV",0,"variable_step",0,"DEC" },
    { "THROTTLE_RADAR_ELEV_INC","THROTTLE_RADAR_ELEV",1,"variable_step",0,"INC" },
    { "THROTTLE_SPEED_BRK_RETRACT","THROTTLE_SPEED_BRK",0,"selector",10,"RETRACT" },
    { "THROTTLE_SPEED_BRK_OFF","THROTTLE_SPEED_BRK",1,"selector",10,"OFF" },
    { "THROTTLE_SPEED_BRK_EXTEND","THROTTLE_SPEED_BRK",2,"selector",10,"EXTEND" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// O(1) hash lookup for SelectorMap[] by (dcsCommand, value)
struct SelectorHashEntry { const char* dcsCommand; uint16_t value; const SelectorEntry* entry; };
static const SelectorHashEntry selectorHashTable[97] = {
  {"INS_SW", 4, &SelectorMap[12]},
  {"RADAR_SW", 3, &SelectorMap[25]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"THROTTLE_CAGE_BTN", 1, &SelectorMap[30]},
  {nullptr, 0, nullptr},
  {"THROTTLE_FRICTION", 65535, &SelectorMap[37]},
  {nullptr, 0, nullptr},
  {"THROTTLE_FOV_SEL_SW", 1, &SelectorMap[36]},
  {nullptr, 0, nullptr},
  {"ENGINE_CRANK_SW", 0, &SelectorMap[2]},
  {"INS_SW", 7, &SelectorMap[15]},
  {nullptr, 0, nullptr},
  {"INS_SW", 5, &SelectorMap[13]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"LST_NFLR_SW", 1, &SelectorMap[19]},
  {nullptr, 0, nullptr},
  {"FLIR_SW", 2, &SelectorMap[7]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"LTD_R_SW", 0, &SelectorMap[20]},
  {nullptr, 0, nullptr},
  {"INS_SW", 6, &SelectorMap[14]},
  {nullptr, 0, nullptr},
  {"THROTTLE_ATC_SW", 1, &SelectorMap[29]},
  {"LTD_R_SW", 1, &SelectorMap[21]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"RADAR_SW", 0, &SelectorMap[22]},
  {"RADAR_SW", 0, &SelectorMap[26]},
  {"THROTTLE_SPEED_BRK", 2, &SelectorMap[45]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"ENGINE_CRANK_SW", 1, &SelectorMap[3]},
  {"THROTTLE_EXT_L_SW", 0, &SelectorMap[34]},
  {"THROTTLE_DISP_SW", 0, &SelectorMap[31]},
  {"FLIR_SW", 1, &SelectorMap[6]},
  {"THROTTLE_RADAR_ELEV", 65535, &SelectorMap[40]},
  {"THROTTLE_DISP_SW", 1, &SelectorMap[32]},
  {"THROTTLE_FRICTION", 0, &SelectorMap[38]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"LST_NFLR_SW", 0, &SelectorMap[18]},
  {"THROTTLE_EXT_L_SW", 1, &SelectorMap[35]},
  {"THROTTLE_DISP_SW", 2, &SelectorMap[33]},
  {"APU_CONTROL_SW", 1, &SelectorMap[1]},
  {"FLIR_SW", 0, &SelectorMap[5]},
  {"THROTTLE_SPEED_BRK", 1, &SelectorMap[44]},
  {"APU_CONTROL_SW", 0, &SelectorMap[0]},
  {"THROTTLE_RADAR_ELEV", 1, &SelectorMap[42]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"THROTTLE_SPEED_BRK", 0, &SelectorMap[43]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"INS_SW", 0, &SelectorMap[8]},
  {"INS_SW", 0, &SelectorMap[16]},
  {nullptr, 0, nullptr},
  {"THROTTLE_RADAR_ELEV", 0, &SelectorMap[41]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"INS_SW", 3, &SelectorMap[11]},
  {"RADAR_SW_PULL", 1, &SelectorMap[28]},
  {"INS_SW", 1, &SelectorMap[9]},
  {"INS_SW", 1, &SelectorMap[17]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"ENGINE_CRANK_SW", 2, &SelectorMap[4]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
  {"INS_SW", 2, &SelectorMap[10]},
  {"THROTTLE_FRICTION", 1, &SelectorMap[39]},
  {nullptr, 0, nullptr},
  {"RADAR_SW", 1, &SelectorMap[23]},
  {"RADAR_SW", 1, &SelectorMap[27]},
  {nullptr, 0, nullptr},
  {"RADAR_SW", 2, &SelectorMap[24]},
  {nullptr, 0, nullptr},
  {nullptr, 0, nullptr},
};

// Composite hash: labelHash(dcsCommand) ^ (value * 7919)
inline const SelectorEntry* findSelectorByDcsAndValue(const char* dcsCommand, uint16_t value) {
  uint16_t startH = (labelHash(dcsCommand) ^ (value * 7919u)) % 97;
  for (uint16_t i = 0; i < 97; ++i) {
    uint16_t idx = (startH + i >= 97) ? (startH + i - 97) : (startH + i);
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
    { "FLIR_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "INS_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "LST_NFLR_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "LTD_R_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "RADAR_SW", 0, 0, true, 7, 0,   0, false, {0}, {0}, 0 },
    { "RADAR_SW_PULL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_ATC_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_CAGE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_DISP_SW", 0, 0, true, 8, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_EXT_L_SW", 0, 0, true, 9, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_FOV_SEL_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_FRICTION", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_RADAR_ELEV", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "THROTTLE_SPEED_BRK", 0, 0, true, 10, 0,   0, false, {0}, {0}, 0 },
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
    { "HUD", "HUD_ATC_NWS_ENGAGED", 0x75A8, 6 },
    { "HUD", "HUD_LTDR", 0x75A2, 5 },
};
static constexpr size_t numDisplayFields = sizeof(displayFields)/sizeof(displayFields[0]);

struct DisplayFieldHashEntry { const char* label; const DisplayFieldDef* def; };
static const DisplayFieldHashEntry displayFieldsByLabel[5] = {
  {"HUD_ATC_NWS_ENGAGED", &displayFields[0]},
  {"HUD_LTDR", &displayFields[1]},
  {nullptr, nullptr},
  {nullptr, nullptr},
  {nullptr, nullptr},
};

// Shared recursive hash implementation for display label lookup
constexpr uint16_t labelHash(const char* s);

inline const DisplayFieldDef* findDisplayFieldByLabel(const char* label) {
  uint16_t startH = labelHash(label) % 5;
  for (uint16_t i = 0; i < 5; ++i) {
    uint16_t idx = (startH + i >= 5) ? (startH + i - 5) : (startH + i);
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
