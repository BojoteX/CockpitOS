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
    {0x74C4,0x1800,11,2,"BATTERY_SW",CT_SELECTOR},
    {0x74C4,0x2000,13,1,"L_GEN_SW",CT_SELECTOR},
    {0x74C4,0x4000,14,1,"R_GEN_SW",CT_SELECTOR},
    {0x753E,0xFFFF,0,65535,"VOLT_E",CT_GAUGE},
    {0x753C,0xFFFF,0,65535,"VOLT_U",CT_GAUGE},
    {0x74C6,0x0300,8,3,"BLEED_AIR_KNOB",CT_SELECTOR},
    {0x74C4,0x8000,15,1,"BLEED_AIR_PULL",CT_SELECTOR},
    {0x74C6,0x3000,12,2,"CABIN_PRESS_SW",CT_SELECTOR},
    {0x7540,0xFFFF,0,65535,"CABIN_TEMP",CT_ANALOG},
    {0x74C6,0x0C00,10,2,"ECS_MODE_SW",CT_SELECTOR},
    {0x74C6,0xC000,14,2,"ENG_ANTIICE_SW",CT_SELECTOR},
    {0x74C8,0x0100,8,1,"PITOT_HEAT_SW",CT_SELECTOR},
    {0x7542,0xFFFF,0,65535,"SUIT_TEMP",CT_ANALOG},
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
    {0x74C8,0x3000,12,2,"FLIR_SW",CT_SELECTOR},
    {0x74CA,0x3800,11,7,"INS_SW",CT_SELECTOR},
    {0x74C8,0x8000,15,1,"LST_NFLR_SW",CT_SELECTOR},
    {0x74C8,0x4000,14,1,"LTD_R_SW",CT_SELECTOR},
    {0x74CA,0x0300,8,3,"RADAR_SW",CT_SELECTOR},
    {0x74CA,0x0400,10,1,"RADAR_SW_PULL",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[6]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x74C4, { &DcsOutputTable[0], &DcsOutputTable[1], &DcsOutputTable[2], &DcsOutputTable[6] }, 4 },
  { 0x753E, { &DcsOutputTable[3] }, 1 },
  { 0x753C, { &DcsOutputTable[4] }, 1 },
  { 0x74C6, { &DcsOutputTable[5], &DcsOutputTable[7], &DcsOutputTable[9], &DcsOutputTable[10] }, 4 },
  { 0x7540, { &DcsOutputTable[8] }, 1 },
  { 0x74C8, { &DcsOutputTable[11], &DcsOutputTable[14], &DcsOutputTable[18], &DcsOutputTable[20], &DcsOutputTable[22], &DcsOutputTable[23] }, 6 },
  { 0x7542, { &DcsOutputTable[12] }, 1 },
  { 0x754A, { &DcsOutputTable[13] }, 1 },
  { 0x7544, { &DcsOutputTable[15] }, 1 },
  { 0x7548, { &DcsOutputTable[16] }, 1 },
  { 0x7546, { &DcsOutputTable[17] }, 1 },
  { 0x754C, { &DcsOutputTable[19] }, 1 },
  { 0x74CA, { &DcsOutputTable[21], &DcsOutputTable[24], &DcsOutputTable[25] }, 3 },
};

// Address hash entry
struct DcsAddressHashEntry {
  uint16_t addr;
  const AddressEntry* entry;
};

static const DcsAddressHashEntry dcsAddressHashTable[53] = {
  { 0x74C4, &dcsAddressTable[0] },
  {0xFFFF, nullptr},
  { 0x74C6, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  { 0x74C8, &dcsAddressTable[5] },
  {0xFFFF, nullptr},
  { 0x74CA, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x753C, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x753E, &dcsAddressTable[1] },
  {0xFFFF, nullptr},
  { 0x7540, &dcsAddressTable[4] },
  {0xFFFF, nullptr},
  { 0x7542, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  { 0x7544, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  { 0x7546, &dcsAddressTable[10] },
  {0xFFFF, nullptr},
  { 0x7548, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  { 0x754A, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x754C, &dcsAddressTable[11] },
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
    { "BATTERY_SW_ON","BATTERY_SW",2,"selector",1,"ON" },
    { "BATTERY_SW_OFF","BATTERY_SW",1,"selector",1,"OFF" },
    { "BATTERY_SW_ORIDE","BATTERY_SW",0,"selector",1,"ORIDE" },
    { "L_GEN_SW_NORM","L_GEN_SW",1,"selector",2,"NORM" },
    { "L_GEN_SW_OFF","L_GEN_SW",0,"selector",2,"OFF" },
    { "R_GEN_SW_NORM","R_GEN_SW",1,"selector",3,"NORM" },
    { "R_GEN_SW_OFF","R_GEN_SW",0,"selector",3,"OFF" },
    { "BLEED_AIR_KNOB_R_OFF","BLEED_AIR_KNOB",3,"selector",4,"R_OFF" },
    { "BLEED_AIR_KNOB_NORM","BLEED_AIR_KNOB",2,"selector",4,"NORM" },
    { "BLEED_AIR_KNOB_L_OFF","BLEED_AIR_KNOB",1,"selector",4,"L_OFF" },
    { "BLEED_AIR_KNOB_OFF","BLEED_AIR_KNOB",0,"selector",4,"OFF" },
    { "BLEED_AIR_PULL_POS0","BLEED_AIR_PULL",0,"selector",14,"POS0" },
    { "BLEED_AIR_PULL_POS1","BLEED_AIR_PULL",1,"selector",14,"POS1" },
    { "CABIN_PRESS_SW_POS0","CABIN_PRESS_SW",0,"selector",15,"POS0" },
    { "CABIN_PRESS_SW_POS1","CABIN_PRESS_SW",1,"selector",15,"POS1" },
    { "CABIN_PRESS_SW_POS2","CABIN_PRESS_SW",2,"selector",15,"POS2" },
    { "CABIN_TEMP","CABIN_TEMP",65535,"analog",0,"LEVEL" },
    { "ECS_MODE_SW_POS0","ECS_MODE_SW",0,"selector",16,"POS0" },
    { "ECS_MODE_SW_POS1","ECS_MODE_SW",1,"selector",16,"POS1" },
    { "ECS_MODE_SW_POS2","ECS_MODE_SW",2,"selector",16,"POS2" },
    { "ENG_ANTIICE_SW_ON","ENG_ANTIICE_SW",2,"selector",5,"ON" },
    { "ENG_ANTIICE_SW_OFF","ENG_ANTIICE_SW",1,"selector",5,"OFF" },
    { "ENG_ANTIICE_SW_TEST","ENG_ANTIICE_SW",0,"selector",5,"TEST" },
    { "PITOT_HEAT_SW_ON","PITOT_HEAT_SW",1,"action",6,"ON" },
    { "PITOT_HEAT_SW_AUTO","PITOT_HEAT_SW",0,"action",6,"AUTO" },
    { "SUIT_TEMP","SUIT_TEMP",65535,"analog",0,"LEVEL" },
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",7,"NVG" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",7,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",7,"DAY" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",8,"TEST" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",8,"OFF" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLIR_SW_ON","FLIR_SW",2,"selector",9,"ON" },
    { "FLIR_SW_STBY","FLIR_SW",1,"selector",9,"STBY" },
    { "FLIR_SW_OFF","FLIR_SW",0,"selector",9,"OFF" },
    { "INS_SW_OFF","INS_SW",7,"selector",10,"OFF" },
    { "INS_SW_CV","INS_SW",6,"selector",10,"CV" },
    { "INS_SW_GND","INS_SW",5,"selector",10,"GND" },
    { "INS_SW_NAV","INS_SW",4,"selector",10,"NAV" },
    { "INS_SW_IFA","INS_SW",3,"selector",10,"IFA" },
    { "INS_SW_GYRO","INS_SW",2,"selector",10,"GYRO" },
    { "INS_SW_GB","INS_SW",1,"selector",10,"GB" },
    { "INS_SW_TEST","INS_SW",0,"selector",10,"TEST" },
    { "LST_NFLR_SW_ON","LST_NFLR_SW",1,"selector",11,"ON" },
    { "LST_NFLR_SW_OFF","LST_NFLR_SW",0,"selector",11,"OFF" },
    { "LTD_R_SW_ARM","LTD_R_SW",1,"selector",12,"ARM" },
    { "LTD_R_SW_SAFE","LTD_R_SW",0,"selector",12,"SAFE" },
    { "RADAR_SW_OFF","RADAR_SW",3,"selector",13,"OFF" },
    { "RADAR_SW_STBY","RADAR_SW",2,"selector",13,"STBY" },
    { "RADAR_SW_OPR","RADAR_SW",1,"selector",13,"OPR" },
    { "RADAR_SW_EMERG(PULL)","RADAR_SW",0,"selector",13,"EMERG(PULL)" },
    { "RADAR_SW_PULL","RADAR_SW_PULL",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "BATTERY_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "BLEED_AIR_KNOB", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "BLEED_AIR_PULL", 0, 0, true, 14, 0,   0, false, {0}, {0}, 0 },
    { "CABIN_PRESS_SW", 0, 0, true, 15, 0,   0, false, {0}, {0}, 0 },
    { "CABIN_TEMP", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 7, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "ECS_MODE_SW", 0, 0, true, 16, 0,   0, false, {0}, {0}, 0 },
    { "ENG_ANTIICE_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "FLIR_SW", 0, 0, true, 9, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "INS_SW", 0, 0, true, 10, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 8, 0,   0, false, {0}, {0}, 0 },
    { "LST_NFLR_SW", 0, 0, true, 11, 0,   0, false, {0}, {0}, 0 },
    { "LTD_R_SW", 0, 0, true, 12, 0,   0, false, {0}, {0}, 0 },
    { "L_GEN_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "PITOT_HEAT_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "RADAR_SW", 0, 0, true, 13, 0,   0, false, {0}, {0}, 0 },
    { "RADAR_SW_PULL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "R_GEN_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "SUIT_TEMP", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
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
