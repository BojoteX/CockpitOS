// Mappings.h – Panel and Device Definitions for CockpitOS

#pragma once

// ==============================================================================================================
// PCA9555 panels are auto-detected from InputMapping.h and LEDMapping.h at startup.
// No manual registration is needed — just add PCA_0xNN entries to your label set's mappings.
// Most logic for button handling (cover gates, latched/momentary behavior) is automated and managed in Mappings.cpp.
//
// All other button/cover logic is handled automatically unless you require custom overrides.
// ==============================================================================================================

#include "src/Generated/PanelKind.h"

const char* getPanelName(uint8_t addr);  // Returns "PCA 0xNN" for discovered PCA devices

#define MAX_COVER_GATES 16   // Or whatever is your true max, bump as needed!
#define MAX_LATCHED_BUTTONS 16  // or however many you'll need

#include "src/Core/CoverGateDef.h"
// kCoverGates[] and kLatchedButtons[] are defined inline in LabelSetSelect.h
// (included via Pins.h into every translation unit)

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
