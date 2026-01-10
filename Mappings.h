// Mappings.h – Panel and Device Definitions for CockpitOS

#pragma once

// ==============================================================================================================
// Use this file to define new panel kinds, assign custom panel types, and register PCA9555 devices by I2C address.
// Most logic for button handling (cover gates, latched/momentary behavior) is automated and managed in Mappings.cpp.
// You only need to edit this file when:
//   - Adding a completely new custom panel (update PanelKind and PanelID)
//   - Registering a new PCA9555 device address (update kPanels[] and PanelID)
// See comments throughout for specific instructions.
//
// All other button/cover logic is handled automatically unless you require custom overrides.
// ==============================================================================================================

#include "src/Generated/PanelKind.h"

// Give your PCA custom panel a basic short name and address where it is located (use the I2C scanner in Tools) 
enum class PanelID : uint8_t { 
   ECM      = 0x22, // ECM Panel bundled with ALR-67
   BRAIN    = 0x26, // TEK Brain Controller (IR Cool uses the PCA on the brain, IRCOOL itself its a dumb panel)
   CUSTOM   = 0x27, // Used for testing simulating a panel (for the Tutorial)
   ARM      = 0x5B, // Master ARM, 0x5B is non-standard (PCA clone? hack?) don't know, don't care
   UNKNOWN  = 0x00  // Placeholder
};

// —— Source panel table —— (don't forget to update the ENUM above, here you give it a friendly name) 
struct PanelDef { uint8_t addr; PanelID id; const char* label; };
static constexpr PanelDef kPanels[] = {
  { 0x22, PanelID::ECM,    "ECM Panel"             },
  { 0x26, PanelID::BRAIN,  "Brain / IRCool Panel"  },
  { 0x27, PanelID::CUSTOM, "Custom Panel"          },
  { 0x5B, PanelID::ARM,    "Master Arm Panel"      },
};

PanelID     getPanelID(uint8_t address);
const char* panelIDToString(PanelID id);
const char* getPanelName(uint8_t addr);  // Declaration

#define MAX_COVER_GATES 16   // Or whatever is your true max, bump as needed!
#define MAX_LATCHED_BUTTONS 16  // or however many you'll need

enum class CoverGateKind : uint8_t {
    Selector,
    ButtonMomentary,
    ButtonLatched,
};

struct CoverGateDef {
    const char* action_label;
    const char* release_label;
    const char* cover_label;
    CoverGateKind kind;
    uint16_t delay_ms;
    uint16_t close_delay_ms;
};
extern const CoverGateDef kCoverGates[];
extern const unsigned kCoverGateCount;
extern const char* kLatchedButtons[];
extern const unsigned kLatchedButtonCount;

void initMappings();
void initializePanels(bool force);
void panelLoop();
void initializeLEDs();
void initializeDisplays();

bool isLatchedButton(const char* label);

// Uncomment any X(AXIS_*) to invert, comment again with /*  X(AXIS_*)  */ to revert
#define INVERTED_AXIS_LIST(X)             \
  /*  X(AXIS_X)         */                \
  /*  X(AXIS_Y)         */                \
  /*  X(AXIS_Z)         */                \
  /*  X(AXIS_RX)        */                \
  /*  X(AXIS_RY)        */                \
  /*  X(AXIS_RZ)        */                \
  /*  X(AXIS_DIAL)      */                \
  /*  X(AXIS_SLIDER)    */                \
  /*  X(AXIS_SLIDER1)   */                \
  /*  X(AXIS_SLIDER2)   */                \
  /*  X(AXIS_SLIDER3)   */                \
  /*  X(AXIS_SLIDER4)   */                \
  /*  X(AXIS_SLIDER5)   */                \
  /*  X(AXIS_SLIDER6)   */                \
  /*  X(AXIS_SLIDER7)   */                \
  /*  X(AXIS_SLIDER8)   */
