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
    {0x7514,0xFFFF,0,65535,"PRESSURE_ALT",CT_GAUGE},
    {0x7488,0x0800,11,1,"AUX_REL_SW",CT_SELECTOR},
    {0x7484,0x6000,13,2,"CMSD_DISPENSE_SW",CT_SELECTOR},
    {0x7484,0x8000,15,1,"CMSD_JET_SEL_BTN",CT_SELECTOR},
    {0x74D4,0x8000,15,1,"CMSD_JET_SEL_L",CT_LED},
    {0x7488,0x0700,8,4,"ECM_MODE_SW",CT_SELECTOR},
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
    {0x7554,0xFFFF,0,65535,"RWR_AUDIO_CTRL",CT_ANALOG},
    {0x7498,0x0100,8,1,"RWR_BIT_BTN",CT_SELECTOR},
    {0x749C,0x1000,12,1,"RWR_BIT_LT",CT_LED},
    {0x7488,0x2000,13,1,"RWR_DISPLAY_BTN",CT_SELECTOR},
    {0x7498,0x4000,14,1,"RWR_DISPLAY_LT",CT_LED},
    {0x7498,0x0E00,9,4,"RWR_DIS_TYPE_SW",CT_SELECTOR},
    {0x7508,0xFFFF,0,65535,"RWR_DMR_CTRL",CT_ANALOG},
    {0x749C,0x0200,9,1,"RWR_ENABLE_LT",CT_LED},
    {0x749C,0x0800,11,1,"RWR_FAIL_LT",CT_LED},
    {0x7498,0x2000,13,1,"RWR_LIMIT_LT",CT_LED},
    {0x7498,0x1000,12,1,"RWR_LOWER_LT",CT_LED},
    {0x7568,0xFFFF,0,65535,"RWR_LT_BRIGHT",CT_GAUGE},
    {0x7488,0x8000,15,1,"RWR_OFFSET_BTN",CT_SELECTOR},
    {0x749C,0x0400,10,1,"RWR_OFFSET_LT",CT_LED},
    {0x7488,0x1000,12,1,"RWR_POWER_BTN",CT_SELECTOR},
    {0x750A,0xFFFF,0,65535,"RWR_RWR_INTESITY",CT_ANALOG},
    {0x7488,0x4000,14,1,"RWR_SPECIAL_BTN",CT_SELECTOR},
    {0x7498,0x8000,15,1,"RWR_SPECIAL_EN_LT",CT_LED},
    {0x749C,0x0100,8,1,"RWR_SPECIAL_LT",CT_LED},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[6]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x7514, { &DcsOutputTable[0] }, 1 },
  { 0x7488, { &DcsOutputTable[1], &DcsOutputTable[5], &DcsOutputTable[16], &DcsOutputTable[25], &DcsOutputTable[27], &DcsOutputTable[29] }, 6 },
  { 0x7484, { &DcsOutputTable[2], &DcsOutputTable[3] }, 2 },
  { 0x74D4, { &DcsOutputTable[4] }, 1 },
  { 0x754A, { &DcsOutputTable[6] }, 1 },
  { 0x74C8, { &DcsOutputTable[7], &DcsOutputTable[11] }, 2 },
  { 0x7544, { &DcsOutputTable[8] }, 1 },
  { 0x7548, { &DcsOutputTable[9] }, 1 },
  { 0x7546, { &DcsOutputTable[10] }, 1 },
  { 0x754C, { &DcsOutputTable[12] }, 1 },
  { 0x7554, { &DcsOutputTable[13] }, 1 },
  { 0x7498, { &DcsOutputTable[14], &DcsOutputTable[17], &DcsOutputTable[18], &DcsOutputTable[22], &DcsOutputTable[23], &DcsOutputTable[30] }, 6 },
  { 0x749C, { &DcsOutputTable[15], &DcsOutputTable[20], &DcsOutputTable[21], &DcsOutputTable[26], &DcsOutputTable[31] }, 5 },
  { 0x7508, { &DcsOutputTable[19] }, 1 },
  { 0x7568, { &DcsOutputTable[24] }, 1 },
  { 0x750A, { &DcsOutputTable[28] }, 1 },
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
  { 0x74C8, &dcsAddressTable[5] },
  { 0x7568, &dcsAddressTable[14] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7498, &dcsAddressTable[11] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x749C, &dcsAddressTable[12] },
  {0xFFFF, nullptr},
  { 0x7508, &dcsAddressTable[13] },
  { 0x74D4, &dcsAddressTable[3] },
  { 0x750A, &dcsAddressTable[15] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7544, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  { 0x7546, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  { 0x7548, &dcsAddressTable[7] },
  { 0x7514, &dcsAddressTable[0] },
  { 0x754A, &dcsAddressTable[4] },
  {0xFFFF, nullptr},
  { 0x754C, &dcsAddressTable[9] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7554, &dcsAddressTable[10] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7484, &dcsAddressTable[2] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7488, &dcsAddressTable[1] },
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
    { "AUX_REL_SW_NORM","AUX_REL_SW",0,"selector",1,"NORM" },
    { "AUX_REL_SW_ENABLE","AUX_REL_SW",1,"selector",1,"ENABLE" },
    { "CMSD_DISPENSE_SW_OFF","CMSD_DISPENSE_SW",0,"selector",2,"OFF" },
    { "CMSD_DISPENSE_SW_ON","CMSD_DISPENSE_SW",1,"selector",2,"ON" },
    { "CMSD_DISPENSE_SW_BYPASS","CMSD_DISPENSE_SW",2,"selector",2,"BYPASS" },
    { "CMSD_JET_SEL_BTN","CMSD_JET_SEL_BTN",1,"momentary",0,"PRESS" },
    { "ECM_MODE_SW_OFF","ECM_MODE_SW",0,"selector",3,"OFF" },
    { "ECM_MODE_SW_STBY","ECM_MODE_SW",1,"selector",3,"STBY" },
    { "ECM_MODE_SW_BIT","ECM_MODE_SW",2,"selector",3,"BIT" },
    { "ECM_MODE_SW_REC","ECM_MODE_SW",3,"selector",3,"REC" },
    { "ECM_MODE_SW_XMIT","ECM_MODE_SW",4,"selector",3,"XMIT" },
    { "ECM_MODE_SW_DEC","ECM_MODE_SW",0,"fixed_step",0,"DEC" },
    { "ECM_MODE_SW_INC","ECM_MODE_SW",1,"fixed_step",0,"INC" },
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "CHART_DIMMER_DEC","CHART_DIMMER",0,"variable_step",0,"DEC" },
    { "CHART_DIMMER_INC","CHART_DIMMER",1,"variable_step",0,"INC" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",4,"DAY" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",4,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",4,"NVG" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "CONSOLES_DIMMER_DEC","CONSOLES_DIMMER",0,"variable_step",0,"DEC" },
    { "CONSOLES_DIMMER_INC","CONSOLES_DIMMER",1,"variable_step",0,"INC" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER_DEC","FLOOD_DIMMER",0,"variable_step",0,"DEC" },
    { "FLOOD_DIMMER_INC","FLOOD_DIMMER",1,"variable_step",0,"INC" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER_DEC","INST_PNL_DIMMER",0,"variable_step",0,"DEC" },
    { "INST_PNL_DIMMER_INC","INST_PNL_DIMMER",1,"variable_step",0,"INC" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",5,"OFF" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",5,"TEST" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "WARN_CAUTION_DIMMER_DEC","WARN_CAUTION_DIMMER",0,"variable_step",0,"DEC" },
    { "WARN_CAUTION_DIMMER_INC","WARN_CAUTION_DIMMER",1,"variable_step",0,"INC" },
    { "RWR_AUDIO_CTRL","RWR_AUDIO_CTRL",65535,"analog",0,"LEVEL" },
    { "RWR_AUDIO_CTRL_DEC","RWR_AUDIO_CTRL",0,"variable_step",0,"DEC" },
    { "RWR_AUDIO_CTRL_INC","RWR_AUDIO_CTRL",1,"variable_step",0,"INC" },
    { "RWR_BIT_BTN","RWR_BIT_BTN",1,"momentary",0,"PRESS" },
    { "RWR_DISPLAY_BTN","RWR_DISPLAY_BTN",1,"momentary",0,"PRESS" },
    { "RWR_DIS_TYPE_SW_F","RWR_DIS_TYPE_SW",0,"selector",6,"F" },
    { "RWR_DIS_TYPE_SW_U","RWR_DIS_TYPE_SW",1,"selector",6,"U" },
    { "RWR_DIS_TYPE_SW_A","RWR_DIS_TYPE_SW",2,"selector",6,"A" },
    { "RWR_DIS_TYPE_SW_I","RWR_DIS_TYPE_SW",3,"selector",6,"I" },
    { "RWR_DIS_TYPE_SW_N","RWR_DIS_TYPE_SW",4,"selector",6,"N" },
    { "RWR_DIS_TYPE_SW_DEC","RWR_DIS_TYPE_SW",0,"fixed_step",0,"DEC" },
    { "RWR_DIS_TYPE_SW_INC","RWR_DIS_TYPE_SW",1,"fixed_step",0,"INC" },
    { "RWR_DMR_CTRL","RWR_DMR_CTRL",65535,"analog",0,"LEVEL" },
    { "RWR_DMR_CTRL_DEC","RWR_DMR_CTRL",0,"variable_step",0,"DEC" },
    { "RWR_DMR_CTRL_INC","RWR_DMR_CTRL",1,"variable_step",0,"INC" },
    { "RWR_OFFSET_BTN","RWR_OFFSET_BTN",1,"momentary",0,"PRESS" },
    { "RWR_POWER_BTN","RWR_POWER_BTN",1,"momentary",0,"PRESS" },
    { "RWR_RWR_INTESITY","RWR_RWR_INTESITY",65535,"analog",0,"LEVEL" },
    { "RWR_RWR_INTESITY_DEC","RWR_RWR_INTESITY",0,"variable_step",0,"DEC" },
    { "RWR_RWR_INTESITY_INC","RWR_RWR_INTESITY",1,"variable_step",0,"INC" },
    { "RWR_SPECIAL_BTN","RWR_SPECIAL_BTN",1,"momentary",0,"PRESS" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "AUX_REL_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "CMSD_DISPENSE_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "CMSD_JET_SEL_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "ECM_MODE_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "RWR_AUDIO_CTRL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_BIT_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_DISPLAY_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_DIS_TYPE_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "RWR_DMR_CTRL", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_OFFSET_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_POWER_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_RWR_INTESITY", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "RWR_SPECIAL_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
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
