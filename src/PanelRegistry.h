// PanelRegistry.h — modern only
#pragma once
#include <stdint.h>
enum class PanelKind : uint8_t;  // from Mappings.h

using PanelFn = void (*)();

struct PanelHooks {
  const char* label;   // debug only
  PanelKind   kind;    // identity
  uint8_t     prio;    // lower runs earlier; 100 default
  PanelFn     init;
  PanelFn     loop;
  PanelFn     disp_init;
  PanelFn     disp_loop;
  PanelFn     tick;    // optional per-frame work
};

// Registry API
void PanelRegistry_register(const PanelHooks& h);
bool PanelRegistry_has(PanelKind k);
bool PanelRegistry_isActive(PanelKind k);              // runtime gate
void PanelRegistry_setActive(PanelKind k, bool active);

// Debug/query
bool        PanelRegistry_registered(const char* label);
int         PanelRegistry_count();
const char* PanelRegistry_labelAt(int idx);

// Iterators
void PanelRegistry_forEachInit();
void PanelRegistry_forEachLoop();
void PanelRegistry_forEachDisplayInit();
void PanelRegistry_forEachDisplayLoop();
void PanelRegistry_forEachTick();

// Auto-register helper
struct _AutoPanelRegister {
  explicit _AutoPanelRegister(const PanelHooks& h){ PanelRegistry_register(h); }
};

/*
// Single registration macro. Pass nullptr for unused hooks.
#define REGISTER_PANEL(KIND, INIT, LOOP, DINIT, DLOOP, TICK, PRIO) \
  static const PanelHooks _hooks_##KIND = { "has" #KIND, PanelKind::KIND, \
    (uint8_t)(PRIO), (INIT), (LOOP), (DINIT), (DLOOP), (TICK) }; \
  static _AutoPanelRegister _apr_##KIND(_hooks_##KIND)
*/

// Compile-time check: PanelKind::KIND must exist in the enum.
// If it doesn't, template instantiation fails with the static_assert message.
template<PanelKind K> constexpr bool _panelKindValid() { return true; }

#define ASSERT_PANEL_KIND_EXISTS(KIND)                                           \
  static_assert(_panelKindValid<PanelKind::KIND>(),                              \
    "\n\n[PanelRegistry] Invalid panel name '" #KIND                            \
    "'.\n--> You probably forgot to add '" #KIND                                \
    "' to enum class PanelKind in Mappings.h (and to its activation in Mappings.cpp).\n")

#define REGISTER_PANEL(KIND, INIT, LOOP, DINIT, DLOOP, TICK, PRIO) \
  ASSERT_PANEL_KIND_EXISTS(KIND); \
  static const PanelHooks _hooks_##KIND = { "has" #KIND, PanelKind::KIND, \
    (uint8_t)(PRIO), (INIT), (LOOP), (DINIT), (DLOOP), (TICK) }; \
  static _AutoPanelRegister _apr_##KIND(_hooks_##KIND)

