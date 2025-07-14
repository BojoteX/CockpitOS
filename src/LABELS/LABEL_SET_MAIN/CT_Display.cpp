#include "../../Globals.h"
#include "CT_Display.h"
#include "DisplayMapping.h"


DisplayBufferEntry CT_DisplayBuffers[] = {
};
const size_t numCTDisplayBuffers = sizeof(CT_DisplayBuffers)/sizeof(CT_DisplayBuffers[0]);

const DisplayBufferHashEntry CT_DisplayBufferHash[2] = {
    {nullptr, nullptr},
    {nullptr, nullptr},
};

const DisplayBufferEntry* findDisplayBufferByLabel(const char* label) {
    uint16_t startH = labelHash(label) % 2;
    for (uint16_t i = 0; i < 2; ++i) {
        uint16_t idx = (startH + i >= 2) ? (startH + i - 2) : (startH + i);
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

