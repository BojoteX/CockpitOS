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
    {0x7526,0xFFFF,0,65535,"FORMATION_DIMMER",CT_ANALOG},
    {0x74A8,0x8000,15,1,"INT_WNG_TANK_SW",CT_SELECTOR},
    {0x7524,0xFFFF,0,65535,"POSITION_DIMMER",CT_ANALOG},
    {0x74B0,0x3000,12,2,"STROBE_SW",CT_SELECTOR},
    {0x74AC,0x0C00,10,2,"EXT_PWR_SW",CT_SELECTOR},
    {0x74AC,0x3000,12,2,"GND_PWR_1_SW",CT_SELECTOR},
    {0x74AC,0xC000,14,2,"GND_PWR_2_SW",CT_SELECTOR},
    {0x74B0,0x0300,8,2,"GND_PWR_3_SW",CT_SELECTOR},
    {0x74B0,0x0C00,10,2,"GND_PWR_4_SW",CT_SELECTOR},
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[3]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x7526, { &DcsOutputTable[0] }, 1 },
  { 0x74A8, { &DcsOutputTable[1] }, 1 },
  { 0x7524, { &DcsOutputTable[2] }, 1 },
  { 0x74B0, { &DcsOutputTable[3], &DcsOutputTable[7], &DcsOutputTable[8] }, 3 },
  { 0x74AC, { &DcsOutputTable[4], &DcsOutputTable[5], &DcsOutputTable[6] }, 3 },
  { 0x754A, { &DcsOutputTable[9] }, 1 },
  { 0x74C8, { &DcsOutputTable[10], &DcsOutputTable[14] }, 2 },
  { 0x7544, { &DcsOutputTable[11] }, 1 },
  { 0x7548, { &DcsOutputTable[12] }, 1 },
  { 0x7546, { &DcsOutputTable[13] }, 1 },
  { 0x754C, { &DcsOutputTable[15] }, 1 },
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
  { 0x74C8, &dcsAddressTable[6] },
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
  { 0x7544, &dcsAddressTable[7] },
  {0xFFFF, nullptr},
  { 0x7546, &dcsAddressTable[9] },
  { 0x74A8, &dcsAddressTable[1] },
  { 0x7548, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  { 0x754A, &dcsAddressTable[5] },
  { 0x74AC, &dcsAddressTable[4] },
  { 0x754C, &dcsAddressTable[10] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x74B0, &dcsAddressTable[3] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7524, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  { 0x7526, &dcsAddressTable[0] },
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
    { "FORMATION_DIMMER","FORMATION_DIMMER",65535,"analog",0,"LEVEL" },
    { "FORMATION_DIMMER_DEC","FORMATION_DIMMER",0,"variable_step",0,"DEC" },
    { "FORMATION_DIMMER_INC","FORMATION_DIMMER",1,"variable_step",0,"INC" },
    { "INT_WNG_TANK_SW_NORM","INT_WNG_TANK_SW",0,"selector",1,"NORM" },
    { "INT_WNG_TANK_SW_INHIBIT","INT_WNG_TANK_SW",1,"selector",1,"INHIBIT" },
    { "POSITION_DIMMER","POSITION_DIMMER",65535,"analog",0,"LEVEL" },
    { "POSITION_DIMMER_DEC","POSITION_DIMMER",0,"variable_step",0,"DEC" },
    { "POSITION_DIMMER_INC","POSITION_DIMMER",1,"variable_step",0,"INC" },
    { "STROBE_SW_POS0","STROBE_SW",0,"selector",9,"POS0" },
    { "STROBE_SW_POS1","STROBE_SW",1,"selector",9,"POS1" },
    { "STROBE_SW_POS2","STROBE_SW",2,"selector",9,"POS2" },
    { "EXT_PWR_SW_OFF","EXT_PWR_SW",0,"selector",2,"OFF" },
    { "EXT_PWR_SW_NORM","EXT_PWR_SW",1,"selector",2,"NORM" },
    { "EXT_PWR_SW_RESET","EXT_PWR_SW",2,"selector",2,"RESET" },
    { "GND_PWR_1_SW_B_ON","GND_PWR_1_SW",0,"selector",3,"B_ON" },
    { "GND_PWR_1_SW_AUTO","GND_PWR_1_SW",1,"selector",3,"AUTO" },
    { "GND_PWR_1_SW_A_ON","GND_PWR_1_SW",2,"selector",3,"A_ON" },
    { "GND_PWR_2_SW_B_ON","GND_PWR_2_SW",0,"selector",4,"B_ON" },
    { "GND_PWR_2_SW_AUTO","GND_PWR_2_SW",1,"selector",4,"AUTO" },
    { "GND_PWR_2_SW_A_ON","GND_PWR_2_SW",2,"selector",4,"A_ON" },
    { "GND_PWR_3_SW_B_ON","GND_PWR_3_SW",0,"selector",5,"B_ON" },
    { "GND_PWR_3_SW_AUTO","GND_PWR_3_SW",1,"selector",5,"AUTO" },
    { "GND_PWR_3_SW_A_ON","GND_PWR_3_SW",2,"selector",5,"A_ON" },
    { "GND_PWR_4_SW_B_ON","GND_PWR_4_SW",0,"selector",6,"B_ON" },
    { "GND_PWR_4_SW_AUTO","GND_PWR_4_SW",1,"selector",6,"AUTO" },
    { "GND_PWR_4_SW_A_ON","GND_PWR_4_SW",2,"selector",6,"A_ON" },
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "CHART_DIMMER_DEC","CHART_DIMMER",0,"variable_step",0,"DEC" },
    { "CHART_DIMMER_INC","CHART_DIMMER",1,"variable_step",0,"INC" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",7,"DAY" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",7,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",7,"NVG" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "CONSOLES_DIMMER_DEC","CONSOLES_DIMMER",0,"variable_step",0,"DEC" },
    { "CONSOLES_DIMMER_INC","CONSOLES_DIMMER",1,"variable_step",0,"INC" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER_DEC","FLOOD_DIMMER",0,"variable_step",0,"DEC" },
    { "FLOOD_DIMMER_INC","FLOOD_DIMMER",1,"variable_step",0,"INC" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER_DEC","INST_PNL_DIMMER",0,"variable_step",0,"DEC" },
    { "INST_PNL_DIMMER_INC","INST_PNL_DIMMER",1,"variable_step",0,"INC" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",8,"OFF" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",8,"TEST" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "WARN_CAUTION_DIMMER_DEC","WARN_CAUTION_DIMMER",0,"variable_step",0,"DEC" },
    { "WARN_CAUTION_DIMMER_INC","WARN_CAUTION_DIMMER",1,"variable_step",0,"INC" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 7, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "EXT_PWR_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "FORMATION_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "GND_PWR_1_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "GND_PWR_2_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "GND_PWR_3_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "GND_PWR_4_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "INT_WNG_TANK_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 8, 0,   0, false, {0}, {0}, 0 },
    { "POSITION_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "STROBE_SW", 0, 0, true, 9, 0,   0, false, {0}, {0}, 0 },
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
