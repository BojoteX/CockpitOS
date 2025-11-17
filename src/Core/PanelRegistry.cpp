// PanelRegistry.cpp — modern only with runtime active gate
#include "../Globals.h"
#include "../../Mappings.h"
#include "../PanelRegistry.h"
#include <string.h>

#define PANELREGISTRY_MAX_PANELS 32u

static PanelHooks g_panels[PANELREGISTRY_MAX_PANELS];
static uint8_t    g_count = 0;

static uint32_t   g_presentMask = 0;   // compiled+registered
static uint32_t   g_activeMask = 0;   // runtime-enabled

static inline void setPresent(PanelKind k) {
    g_presentMask |= (1u << static_cast<uint8_t>(k));
    g_activeMask |= (1u << static_cast<uint8_t>(k));   // default: active
}
static inline bool isBit(uint32_t m, uint8_t idx) { return (m >> idx) & 1u; }

bool PanelRegistry_has(PanelKind k) {
    return isBit(g_presentMask, static_cast<uint8_t>(k));
}
bool PanelRegistry_isActive(PanelKind k) {
    return isBit(g_activeMask, static_cast<uint8_t>(k));
}
void PanelRegistry_setActive(PanelKind k, bool active) {
    const uint8_t i = static_cast<uint8_t>(k);
    const uint32_t bit = (1u << i);
    if (active) g_activeMask |= bit; else g_activeMask &= ~bit;
}

void PanelRegistry_register(const PanelHooks& h) {
    const uint8_t kind = static_cast<uint8_t>(h.kind);
    if (kind >= static_cast<uint8_t>(PanelKind::COUNT)) return;

    // dedup by kind
    for (uint8_t i = 0; i < g_count; ++i) {
        if (g_panels[i].kind == h.kind) {
            if (!g_panels[i].init)      g_panels[i].init = h.init;
            if (!g_panels[i].loop)      g_panels[i].loop = h.loop;
            if (!g_panels[i].disp_init) g_panels[i].disp_init = h.disp_init;
            if (!g_panels[i].disp_loop) g_panels[i].disp_loop = h.disp_loop;
            if (!g_panels[i].tick)      g_panels[i].tick = h.tick;
            setPresent(h.kind);
            return;
        }
    }

    // priority-sorted insert
    if (g_count >= PANELREGISTRY_MAX_PANELS) return;
    uint8_t pos = g_count;
    for (uint8_t i = 0; i < g_count; ++i) {
        if (h.prio < g_panels[i].prio) { pos = i; break; }
    }
    for (uint8_t j = g_count; j > pos; --j) g_panels[j] = g_panels[j - 1];
    g_panels[pos] = h;
    ++g_count;
    setPresent(h.kind);
}

bool PanelRegistry_registered(const char* label) {
    if (!label) return false;
    for (uint8_t i = 0; i < g_count; ++i) {
        if (g_panels[i].label && strcmp(g_panels[i].label, label) == 0) return true;
    }
    return false;
}
int PanelRegistry_count() { return static_cast<int>(g_count); }
const char* PanelRegistry_labelAt(int idx) {
    if (idx < 0 || idx >= static_cast<int>(g_count)) return nullptr;
    return g_panels[idx].label;
}

static inline void callIf(PanelFn f) { if (f) f(); }
#define FOR_ACTIVE(call_member) \
  for (uint8_t i=0; i<g_count; ++i){ \
    const uint8_t k = static_cast<uint8_t>(g_panels[i].kind); \
    if (!isBit(g_activeMask, k)) continue; \
    callIf(g_panels[i].call_member); \
  }

void PanelRegistry_forEachInit() { FOR_ACTIVE(init); }
void PanelRegistry_forEachLoop() { FOR_ACTIVE(loop); }
void PanelRegistry_forEachDisplayInit() { FOR_ACTIVE(disp_init); }
void PanelRegistry_forEachDisplayLoop() { FOR_ACTIVE(disp_loop); }
void PanelRegistry_forEachTick() { FOR_ACTIVE(tick); }

/* DEBUGGING ONLY
// PanelRegistry.cpp
void PanelRegistry_forEachLoop() {
    for (uint8_t i = 0; i < g_count; ++i) {
        const uint8_t k = static_cast<uint8_t>(g_panels[i].kind);
        if (!((g_activeMask >> k) & 1u)) continue;

        const char* name = g_panels[i].label ? g_panels[i].label : "<no-label>";
        debugPrintf("loop -> %s\n", name);
        if (g_panels[i].loop) g_panels[i].loop();
    }
}
*/
