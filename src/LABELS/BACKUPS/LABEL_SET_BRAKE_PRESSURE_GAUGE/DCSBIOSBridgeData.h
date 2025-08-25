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
    {0x754A,0xFFFF,0,65535,"CHART_DIMMER",CT_ANALOG},
    {0x74C8,0x0600,9,2,"COCKKPIT_LIGHT_MODE_SW",CT_SELECTOR},
    {0x7544,0xFFFF,0,65535,"CONSOLES_DIMMER",CT_ANALOG},
    {0x7548,0xFFFF,0,65535,"FLOOD_DIMMER",CT_ANALOG},
    {0x7546,0xFFFF,0,65535,"INST_PNL_DIMMER",CT_ANALOG},
    {0x74C8,0x0800,11,1,"LIGHTS_TEST_SW",CT_SELECTOR},
    {0x754C,0xFFFF,0,65535,"WARN_CAUTION_DIMMER",CT_ANALOG},
    {0x7480,0x1000,12,1,"ANTI_SKID_SW",CT_SELECTOR},
    {0x7484,0x0300,8,2,"FLAP_SW",CT_SELECTOR},
    {0x7480,0x4000,14,1,"HOOK_BYPASS_SW",CT_SELECTOR},
    {0x7506,0xFFFF,0,65535,"HYD_IND_BRAKE",CT_GAUGE},
    {0x7480,0x2000,13,1,"LAUNCH_BAR_SW",CT_SELECTOR},
    {0x7480,0x8000,15,1,"LDG_TAXI_SW",CT_SELECTOR},
    {0x7480,0x0100,8,1,"SEL_JETT_BTN",CT_SELECTOR},
    {0x7480,0x0E00,9,4,"SEL_JETT_KNOB",CT_SELECTOR},
};
static const size_t DcsOutputTableSize = sizeof(DcsOutputTable)/sizeof(DcsOutputTable[0]);

// Static flat address-to-output entry lookup
struct AddressEntry {
  uint16_t addr;
  const DcsOutputEntry* entries[6]; // max entries per address
  uint8_t count;
};

static const AddressEntry dcsAddressTable[] = {
  { 0x754A, { &DcsOutputTable[0] }, 1 },
  { 0x74C8, { &DcsOutputTable[1], &DcsOutputTable[5] }, 2 },
  { 0x7544, { &DcsOutputTable[2] }, 1 },
  { 0x7548, { &DcsOutputTable[3] }, 1 },
  { 0x7546, { &DcsOutputTable[4] }, 1 },
  { 0x754C, { &DcsOutputTable[6] }, 1 },
  { 0x7480, { &DcsOutputTable[7], &DcsOutputTable[9], &DcsOutputTable[11], &DcsOutputTable[12], &DcsOutputTable[13], &DcsOutputTable[14] }, 6 },
  { 0x7484, { &DcsOutputTable[8] }, 1 },
  { 0x7506, { &DcsOutputTable[10] }, 1 },
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
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7506, &dcsAddressTable[8] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
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
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7480, &dcsAddressTable[6] },
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  {0xFFFF, nullptr},
  { 0x7484, &dcsAddressTable[7] },
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
    { "CHART_DIMMER","CHART_DIMMER",65535,"analog",0,"LEVEL" },
    { "COCKKPIT_LIGHT_MODE_SW_NVG","COCKKPIT_LIGHT_MODE_SW",2,"selector",1,"NVG" },
    { "COCKKPIT_LIGHT_MODE_SW_NITE","COCKKPIT_LIGHT_MODE_SW",1,"selector",1,"NITE" },
    { "COCKKPIT_LIGHT_MODE_SW_DAY","COCKKPIT_LIGHT_MODE_SW",0,"selector",1,"DAY" },
    { "CONSOLES_DIMMER","CONSOLES_DIMMER",65535,"analog",0,"LEVEL" },
    { "FLOOD_DIMMER","FLOOD_DIMMER",65535,"analog",0,"LEVEL" },
    { "INST_PNL_DIMMER","INST_PNL_DIMMER",65535,"analog",0,"LEVEL" },
    { "LIGHTS_TEST_SW_TEST","LIGHTS_TEST_SW",1,"selector",2,"TEST" },
    { "LIGHTS_TEST_SW_OFF","LIGHTS_TEST_SW",0,"selector",2,"OFF" },
    { "WARN_CAUTION_DIMMER","WARN_CAUTION_DIMMER",65535,"analog",0,"LEVEL" },
    { "ANTI_SKID_SW_PRESS","ANTI_SKID_SW",0,"selector",6,"PRESS" },
    { "ANTI_SKID_SW_RELEASE","ANTI_SKID_SW",1,"selector",6,"RELEASE" },
    { "FLAP_SW_AUTO","FLAP_SW",2,"selector",3,"AUTO" },
    { "FLAP_SW_HALF","FLAP_SW",1,"selector",3,"HALF" },
    { "FLAP_SW_FULL","FLAP_SW",0,"selector",3,"FULL" },
    { "HOOK_BYPASS_SW_FIELD","HOOK_BYPASS_SW",1,"action",4,"FIELD" },
    { "HOOK_BYPASS_SW_CARRIER","HOOK_BYPASS_SW",0,"action",4,"CARRIER" },
    { "LAUNCH_BAR_SW_PRESS","LAUNCH_BAR_SW",0,"action",0,"PRESS" },
    { "LAUNCH_BAR_SW_RELEASE","LAUNCH_BAR_SW",1,"action",0,"RELEASE" },
    { "LDG_TAXI_SW_LDG","LDG_TAXI_SW",1,"selector",5,"LDG" },
    { "LDG_TAXI_SW_TAXI_LIGHT_SWITCH","LDG_TAXI_SW",0,"selector",5,"TAXI_LIGHT_SWITCH" },
    { "SEL_JETT_BTN","SEL_JETT_BTN",1,"momentary",0,"PRESS" },
    { "SEL_JETT_KNOB_POS0","SEL_JETT_KNOB",0,"selector",7,"POS0" },
    { "SEL_JETT_KNOB_POS1","SEL_JETT_KNOB",1,"selector",7,"POS1" },
    { "SEL_JETT_KNOB_POS2","SEL_JETT_KNOB",2,"selector",7,"POS2" },
    { "SEL_JETT_KNOB_POS3","SEL_JETT_KNOB",3,"selector",7,"POS3" },
    { "SEL_JETT_KNOB_POS4","SEL_JETT_KNOB",4,"selector",7,"POS4" },
};
static const size_t SelectorMapSize = sizeof(SelectorMap)/sizeof(SelectorMap[0]);

// Unified Command History Table (used for throttling, optional keep-alive, and HID dedupe)
static CommandHistoryEntry commandHistory[] = {
    { "ANTI_SKID_SW", 0, 0, true, 6, 0,   0, false, {0}, {0}, 0 },
    { "CHART_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "COCKKPIT_LIGHT_MODE_SW", 0, 0, true, 1, 0,   0, false, {0}, {0}, 0 },
    { "CONSOLES_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "FLAP_SW", 0, 0, true, 3, 0,   0, false, {0}, {0}, 0 },
    { "FLOOD_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "HOOK_BYPASS_SW", 0, 0, true, 4, 0,   0, false, {0}, {0}, 0 },
    { "INST_PNL_DIMMER", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LAUNCH_BAR_SW", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "LDG_TAXI_SW", 0, 0, true, 5, 0,   0, false, {0}, {0}, 0 },
    { "LIGHTS_TEST_SW", 0, 0, true, 2, 0,   0, false, {0}, {0}, 0 },
    { "SEL_JETT_BTN", 0, 0, false, 0, 0,   0, false, {0}, {0}, 0 },
    { "SEL_JETT_KNOB", 0, 0, true, 7, 0,   0, false, {0}, {0}, 0 },
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
