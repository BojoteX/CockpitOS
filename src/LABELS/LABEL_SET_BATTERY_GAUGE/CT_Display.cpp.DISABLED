#include "../../Globals.h"
#include "CT_Display.h"
#include "DisplayMapping.h"

char dcs_bios[7] = {};
bool dcs_bios_dirty = false;
char last_dcs_bios[7] = {};
char pilotname[25] = {};
bool pilotname_dirty = false;
char last_pilotname[25] = {};
char _acft_name[25] = {};
bool _acft_name_dirty = false;
char last__acft_name[25] = {};

DisplayBufferEntry CT_DisplayBuffers[] = {
    { "DCS_BIOS", dcs_bios, 6, &dcs_bios_dirty, last_dcs_bios },
    { "PILOTNAME", pilotname, 24, &pilotname_dirty, last_pilotname },
    { "_ACFT_NAME", _acft_name, 24, &_acft_name_dirty, last__acft_name },
};
const size_t numCTDisplayBuffers = sizeof(CT_DisplayBuffers)/sizeof(CT_DisplayBuffers[0]);

const DisplayBufferHashEntry CT_DisplayBufferHash[7] = {
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"DCS_BIOS", &CT_DisplayBuffers[0]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {"PILOTNAME", &CT_DisplayBuffers[1]},
    {"_ACFT_NAME", &CT_DisplayBuffers[2]},
};

const DisplayBufferEntry* findDisplayBufferByLabel(const char* label) {
    uint16_t startH = labelHash(label) % 7;
    for (uint16_t i = 0; i < 7; ++i) {
        uint16_t idx = (startH + i >= 7) ? (startH + i - 7) : (startH + i);
        const auto& entry = CT_DisplayBufferHash[idx];
        if (!entry.label) continue;
        if (strcmp(entry.label, label) == 0) return entry.entry;
    }
    return nullptr;
}


void renderField(const char* label, const char* strValue, const DisplayFieldDefLabel* defOverride, FieldState* stateOverride) {
    const DisplayFieldDefLabel* def = defOverride ? defOverride : findFieldDefByLabel(label);

    if (!def) return;

    if (!def->renderFunc) {
        debugPrintf("[DISPLAY] No renderFunc for label '%s', skipping\n", def->label);
        return;
    }

    FieldState* state = stateOverride;
    int idx = 0;

    if (!stateOverride) {
        idx = def - fieldDefs;
        state = &fieldStates[idx];
    }

    bool valid = true;
    if (def->type == FIELD_NUMERIC) {
        int value = strToIntFast(strValue);
        valid = (value >= def->minValue && value <= def->maxValue);
    }

    const char* toShow = valid ? strValue : state->lastValue;

    if (memcmp(toShow, state->lastValue, sizeof(state->lastValue)) == 0)
        return;

    memcpy(state->lastValue, toShow, sizeof(state->lastValue));
    if (def->clearFunc) def->clearFunc(def->driver, def->segMap, *def);

    // Call the render function with the appropriate parameters
    def->renderFunc(def->driver, def->segMap, toShow, *def);
}

