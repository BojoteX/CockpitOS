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
    {0x740C,0x0008,3,1,"APU_FIRE_BTN",CT_SELECTOR},
    {0x740C,0x0004,2,1,"FIRE_APU_LT",CT_LED},
    {0x7408,0x0008,3,1,"AOA_INDEXER_HIGH",CT_LED},
    {0x758C,0xFFFF,0,65535,"AOA_INDEXER_HIGH_F",CT_GAUGE},
    {0x7408,0x0020,5,1,"AOA_INDEXER_LOW",CT_LED},
    {0x7590,0xFFFF,0,65535,"AOA_INDEXER_LOW_F",CT_GAUGE},
    {0x7408,0x0010,4,1,"AOA_INDEXER_NORMAL",CT_LED},
    {0x758E,0xFFFF,0,65535,"AOA_INDEXER_NORMAL_F",CT_GAUGE},
    {0x742C,0x0100,8,1,"EMER_JETT_BTN",CT_SELECTOR},
    {0x740E,0x0001,0,1,"FIRE_EXT_BTN",CT_SELECTOR},
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
    {0x740A,0x0008,3,1,"LH_ADV_ASPJ_OH",CT_LED},
    {0x740A,0x0010,4,1,"LH_ADV_GO",CT_LED},
    {0x740A,0x0002,1,1,"LH_ADV_L_BAR_GREEN",CT_LED},
    {0x7408,0x8000,15,1,"LH_ADV_L_BAR_RED",CT_LED},
    {0x7408,0x0800,11,1,"LH_ADV_L_BLEED",CT_LED},
    {0x740A,0x0020,5,1,"LH_ADV_NO_GO",CT_LED},
    {0x740A,0x0001,0,1,"LH_ADV_REC",CT_LED},
    {0x7408,0x1000,12,1,"LH_ADV_R_BLEED",CT_LED},
    {0x7408,0x2000,13,1,"LH_ADV_SPD_BRK",CT_LED},
    {0x7408,0x4000,14,1,"LH_ADV_STBY",CT_LED},
    {0x740A,0x0004,2,1,"LH_ADV_XMIT",CT_LED},
    {0x7408,0x0040,6,1,"FIRE_LEFT_LT",CT_LED},
    {0x7408,0x0080,7,1,"LEFT_FIRE_BTN",CT_SELECTOR},
    {0x7408,0x0100,8,1,"LEFT_FIRE_BTN_COVER",CT_SELECTOR},
    {0x7408,0x0001,0,1,"LS_LOCK",CT_LED},
    {0x7408,0x0002,1,1,"LS_SHOOT",CT_LED},
    {0x7408,0x0004,2,1,"LS_SHOOT_STROBE",CT_LED},
    {0x7456,0xFFFF,0,65535,"HMD_OFF_BRT",CT_ANALOG},
    {0x742A,0xC000,14,2,"IR_COOL_SW",CT_SELECTOR},
    {0x742A,0x0800,11,1,"SPIN_LT",CT_LED},
    {0x742A,0x1000,12,1,"SPIN_RECOVERY_COVER",CT_SELECTOR},
    {0x742A,0x2000,13,1,"SPIN_RECOVERY_SW",CT_SELECTOR},
    {0x740C,0x2000,13,1,"MASTER_ARM_SW",CT_SELECTOR},
    {0x740C,0x0800,11,1,"MASTER_MODE_AA",CT_SELECTOR},
    {0x740C,0x0200,9,1,"MASTER_MODE_AA_LT",CT_LED},
    {0x740C,0x1000,12,1,"MASTER_MODE_AG",CT_SELECTOR},
    {0x740C,0x0400,10,1,"MASTER_MODE_AG_LT",CT_LED},
    {0x740C,0x4000,14,1,"MC_DISCH",CT_LED},
    {0x740C,0x8000,15,1,"MC_READY",CT_LED},
    {0x7408,0x0200,9,1,"MASTER_CAUTION_LT",CT_LED},
    {0x7408,0x0400,10,1,"MASTER_CAUTION_RESET_SW",CT_SELECTOR},
    {0x740A,0x0800,11,1,"RH_ADV_AAA",CT_LED},
    {0x740A,0x0400,10,1,"RH_ADV_AI",CT_LED},
    {0x740A,0x1000,12,1,"RH_ADV_CW",CT_LED},
    {0x740A,0x0100,8,1,"RH_ADV_DISP",CT_LED},
    {0x740A,0x0080,7,1,"RH_ADV_RCDR_ON",CT_LED},
    {0x740A,0x0200,9,1,"RH_ADV_SAM",CT_LED},
    {0x740A,0x2000,13,1,"RH_ADV_SPARE_RH1",CT_LED},
    {0x740A,0x4000,14,1,"RH_ADV_SPARE_RH2",CT_LED},
    {0x740A,0x8000,15,1,"RH_ADV_SPARE_RH3",CT_LED},
    {0x740C,0x0001,0,1,"RH_ADV_SPARE_RH4",CT_LED},
    {0x740C,0x0002,1,1,"RH_ADV_SPARE_RH5",CT_LED},
    {0x740C,0x0010,4,1,"FIRE_RIGHT_LT",CT_LED},
    {0x740C,0x0020,5,1,"RIGHT_FIRE_BTN",CT_SELECTOR},
    {0x740C,0x0040,6,1,"RIGHT_FIRE_BTN_COVER",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[16]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x740C, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[39], &DcsOutputTable[40], &DcsOutputTable[41], &DcsOutputTable[42], &DcsOutputTable[43], &DcsOutputTable[44], &DcsOutputTable[45], &DcsOutputTable[57], &DcsOutputTable[58], &DcsOutputTable[59], &DcsOutputTable[60], &DcsOutputTable[61] }, 14 },
  { 0x7408, { &DcsOutputTable[2], &DcsOutputTable[4], &DcsOutputTable[6], &DcsOutputTable[20], &DcsOutputTable[21], &DcsOutputTable[24], &DcsOutputTable[25], &DcsOutputTable[26], &DcsOutputTable[28], &DcsOutputTable[29], &DcsOutputTable[30], &DcsOutputTable[31], &DcsOutputTable[32], &DcsOutputTable[33], &DcsOutputTable[46], &DcsOutputTable[47] }, 16 },
  { 0x758C, { &DcsOutputTable[3] }, 1 },
  { 0x7590, { &DcsOutputTable[5] }, 1 },
  { 0x758E, { &DcsOutputTable[7] }, 1 },
  { 0x742C, { &DcsOutputTable[8] }, 1 },
  { 0x740E, { &DcsOutputTable[9] }, 1 },
  { 0x754A, { &DcsOutputTable[10] }, 1 },
  { 0x74C8, { &DcsOutputTable[11], &DcsOutputTable[15] }, 2 },
  { 0x7544, { &DcsOutputTable[12] }, 1 },
  { 0x7548, { &DcsOutputTable[13] }, 1 },
  { 0x7546, { &DcsOutputTable[14] }, 1 },
  { 0x754C, { &DcsOutputTable[16] }, 1 },
  { 0x740A, { &DcsOutputTable[17], &DcsOutputTable[18], &DcsOutputTable[19], &DcsOutputTable[22], &DcsOutputTable[23], &DcsOutputTable[27], &DcsOutputTable[48], &DcsOutputTable[49], &DcsOutputTable[50], &DcsOutputTable[51], &DcsOutputTable[52], &DcsOutputTable[53], &DcsOutputTable[54], &DcsOutputTable[55], &DcsOutputTable[56] }, 15 },
  { 0x7456, { &DcsOutputTable[34] }, 1 },
  { 0x742A, { &DcsOutputTable[35], &DcsOutputTable[36], &DcsOutputTable[37], &DcsOutputTable[38] }, 4 },
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
  { 0x74C8, &dcsAddressTable[8] },
  { 0x742A, &dcsAddressTable[15] },
  {0xFFFF, nullptr},
  { 0x742C, &dcsAddressTable[5] },
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
  { 0x7544, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  { 0x7408, &dcsAddressTable[1] },
  { 0x7546, &dcsAddressTable[11] },
  { 0x7548, &dcsAddressTable[10] },
  { 0x740A, &dcsAddressTable[13] },
  { 0x740C, &dcsAddressTable[0] },
  { 0x754A, &dcsAddressTable[7] },
  { 0x740E, &dcsAddressTable[6] },
  { 0x754C, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x758C, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x758E, &dcsAddressTable[4] },
  {0xFFFF, nullptr},
  { 0x7590, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7456, &dcsAddressTable[14] },
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
    if (entry.addr == 0xFFFF) continue;
    if (entry.addr == addr) return entry.entry;
  }
  return nullptr;
}

struct SelectorEntry { const char* label; const char* dcsCommand; uint16_t value; const char* controlType; uint16_t group; const char* posLabel; };
static const SelectorEntry SelectorMap[] = {
    { "APU_FIRE_BTN","APU_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "EMER_JETT_BTN","EMER_JETT_BTN",1,"momentary",0,"PRESS" },
    { "FIRE_EXT_BTN","FIRE_EXT_BTN",1,"momentary",0,"PRESS" },
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
    { "LEFT_FIRE_BTN","LEFT_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "LEFT_FIRE_BTN_COVER","LEFT_FIRE_BTN_COVER",1,"momentary",0,"OPEN" },
    { "HMD_OFF_BRT","HMD_OFF_BRT",65535,"analog",0,"LEVEL" },
    { "HMD_OFF_BRT_DEC","HMD_OFF_BRT",0,"variable_step",0,"DEC" },
    { "HMD_OFF_BRT_INC","HMD_OFF_BRT",1,"variable_step",0,"INC" },
    { "IR_COOL_SW_OFF","IR_COOL_SW",0,"selector",3,"OFF" },
    { "IR_COOL_SW_NORM","IR_COOL_SW",1,"selector",3,"NORM" },
    { "IR_COOL_SW_ORIDE","IR_COOL_SW",2,"selector",3,"ORIDE" },
    { "SPIN_RECOVERY_COVER","SPIN_RECOVERY_COVER",1,"momentary",0,"OPEN" },
    { "SPIN_RECOVERY_SW_NORM","SPIN_RECOVERY_SW",0,"selector",4,"NORM" },
    { "SPIN_RECOVERY_SW_RCVY","SPIN_RECOVERY_SW",1,"selector",4,"RCVY" },
    { "MASTER_ARM_SW_SAFE","MASTER_ARM_SW",0,"selector",5,"SAFE" },
    { "MASTER_ARM_SW_ARM","MASTER_ARM_SW",1,"selector",5,"ARM" },
    { "MASTER_MODE_AA","MASTER_MODE_AA",1,"momentary",0,"PRESS" },
    { "MASTER_MODE_AG","MASTER_MODE_AG",1,"momentary",0,"PRESS" },
    { "MASTER_CAUTION_RESET_SW","MASTER_CAUTION_RESET_SW",1,"momentary",0,"PRESS" },
    { "RIGHT_FIRE_BTN","RIGHT_FIRE_BTN",1,"momentary",0,"PRESS" },
    { "RIGHT_FIRE_BTN_COVER","RIGHT_FIRE_BTN_COVER",1,"momentary",0,"OPEN" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "APU_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "EMER_JETT_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "FIRE_EXT_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "HMD_OFF_BRT", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "IR_COOL_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LEFT_FIRE_BTN_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_ARM_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_CAUTION_RESET_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_MODE_AA", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "MASTER_MODE_AG", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RIGHT_FIRE_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RIGHT_FIRE_BTN_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SPIN_RECOVERY_COVER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SPIN_RECOVERY_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "WARN_CAUTION_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
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
    if (!entry.label) continue;
    if (strcmp(entry.label, label) == 0) return entry.def;
  }
  return nullptr;
}

// No tracked metadata fields found
struct MetadataState { const char* label; uint16_t value; };
static MetadataState metadataStates[] = {};
static const size_t numMetadataStates = 0;
inline MetadataState* findMetadataState(const char*) { return nullptr; }
