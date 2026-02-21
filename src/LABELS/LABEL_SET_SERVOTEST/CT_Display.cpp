#include "../../Globals.h"
#include "CT_Display.h"
#include "DisplayMapping.h"

char hud_atc_nws_engaged[7] = {};
bool hud_atc_nws_engaged_dirty = false;
char last_hud_atc_nws_engaged[7] = {};
char hud_ltdr[6] = {};
bool hud_ltdr_dirty = false;
char last_hud_ltdr[6] = {};

DisplayBufferEntry CT_DisplayBuffers[] = {
    { "HUD_ATC_NWS_ENGAGED", hud_atc_nws_engaged, 6, &hud_atc_nws_engaged_dirty, last_hud_atc_nws_engaged },
    { "HUD_LTDR", hud_ltdr, 5, &hud_ltdr_dirty, last_hud_ltdr },
};
const size_t numCTDisplayBuffers = sizeof(CT_DisplayBuffers)/sizeof(CT_DisplayBuffers[0]);

const DisplayBufferHashEntry CT_DisplayBufferHash[5] = {
    {"HUD_ATC_NWS_ENGAGED", &CT_DisplayBuffers[0]},
    {"HUD_LTDR", &CT_DisplayBuffers[1]},
    {nullptr, nullptr},
    {nullptr, nullptr},
    {nullptr, nullptr},
};

const DisplayBufferEntry* findDisplayBufferByLabel(const char* label) {
    uint16_t startH = labelHash(label) % 5;
    for (uint16_t i = 0; i < 5; ++i) {
        uint16_t idx = (startH + i >= 5) ? (startH + i - 5) : (startH + i);
        const auto& entry = CT_DisplayBufferHash[idx];
        if (!entry.label) return nullptr;
        if (strcmp(entry.label, label) == 0) return entry.entry;
    }
    return nullptr;
}


    void renderField(const char* label, const char* strValue,
                     const DisplayFieldDefLabel* defOverride,
                     FieldState* stateOverride) {
        const DisplayFieldDefLabel* def = defOverride ? defOverride : findFieldDefByLabel(label);
        if (!def) return;
        if (!def->renderFunc) { debugPrintf("[DISPLAY] No renderFunc for label '%s', skipping\n", def->label); return; }

        FieldState* state = stateOverride;
        if (!state) {
            int idx = def - fieldDefs;
            state = &fieldStates[idx];
        }

        bool valid = true;
        if (def->type == FIELD_NUMERIC) {
            const int value = strToIntFast(strValue);
            valid = (value >= def->minValue && value <= def->maxValue);
        }
        const char* toShow = valid ? strValue : state->lastValue;

        // bound compare/copy by field length, clamp to cache size
        uint8_t need = def->numDigits ? def->numDigits : 1;
        if (need > sizeof(state->lastValue)) need = sizeof(state->lastValue);

        if (memcmp(toShow, state->lastValue, need) == 0) return;

        memcpy(state->lastValue, toShow, need);
        if (def->clearFunc) def->clearFunc(def->driver, def->segMap, *def);
        def->renderFunc(def->driver, def->segMap, toShow, *def);
    }

    