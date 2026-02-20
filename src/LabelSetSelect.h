#pragma once
#ifndef LABEL_SET
#  error "Define LABEL_SET in Config.h (e.g., IFEI)"
#endif

// helpers (undef to override Config.h's version — identical macro, avoids redefinition warning)
#undef STR_
#undef STR
#define STR_(x) #x
#define STR(x)  STR_(x)
#define CAT_(a,b) a##b
#define CAT(a,b) CAT_(a,b)

// default directory name: LABEL_SET_<TOKEN>
#ifndef LABEL_DIR
#  define LABEL_DIR CAT(LABEL_SET_, LABEL_SET)
#endif

// Build "LABELS/<LABEL_DIR>/<file>" relative to src/
#define LBLPATH(dir, file) STR(LABELS/dir/file)

// Traits file uses .h so Arduino 2.x copies it
#ifndef LABEL_TRAITS_FILE
#  define LABEL_TRAITS_FILE LabelSetConfig.h
#endif

// includes
#include LBLPATH(LABEL_DIR, LABEL_TRAITS_FILE)
#include LBLPATH(LABEL_DIR, CT_Display.h)
#include LBLPATH(LABEL_DIR, DCSBIOSBridgeData.h)
#include LBLPATH(LABEL_DIR, InputMapping.h)
#include LBLPATH(LABEL_DIR, LEDMapping.h)
#include LBLPATH(LABEL_DIR, DisplayMapping.h)

// --- Per-label-set latched buttons (created by generator or Label Creator) ---
#if __has_include(LBLPATH(LABEL_DIR, LatchedButtons.h))
  #include LBLPATH(LABEL_DIR, LatchedButtons.h)
#else
  static const char* kLatchedButtons[] = {};
  static const unsigned kLatchedButtonCount = 0;
#endif

// --- Per-label-set cover gates (created by generator or Label Creator) ---
#include "Core/CoverGateDef.h"
#if __has_include(LBLPATH(LABEL_DIR, CoverGates.h))
  #include LBLPATH(LABEL_DIR, CoverGates.h)
#else
  static const CoverGateDef kCoverGates[] = {};
  static const unsigned kCoverGateCount = 0;
#endif

// friendly name + USB strings
#ifndef LABEL_SET_NAME
#  define LABEL_SET_NAME STR(LABEL_SET)
#endif
#ifndef LABEL_SET_FULLNAME
#  define LABEL_SET_FULLNAME LABEL_SET_NAME
#endif
#ifdef USB_SERIAL
#  undef USB_SERIAL
#endif
#define USB_SERIAL  LABEL_SET_FULLNAME
#ifdef USB_PRODUCT
#  undef USB_PRODUCT
#endif
#define USB_PRODUCT LABEL_SET_FULLNAME