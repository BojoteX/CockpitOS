#pragma once
#ifndef LABEL_SET
#  error "Define LABEL_SET in Config.h (e.g., TEST_ONLY)"
#endif

// helpers
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

/*
#pragma once
#ifndef LABEL_SET
#  error "Define LABEL_SET in Config.h (e.g., TEST_ONLY)"
#endif

// stringize + paste (with expansion)
#define STR_(x) #x
#define STR(x)  STR_(x)
#define CAT_(a,b) a##b
#define CAT(a,b) CAT_(a,b)

// Directory name: default "LABEL_SET_<LABEL_SET>"
#ifndef LABEL_DIR
#  define LABEL_DIR CAT(LABEL_SET_, LABEL_SET)   // e.g., LABEL_SET_TEST_ONLY
#endif

// Traits filename; keep constant across sets (override if needed)
#ifndef LABEL_TRAITS_FILE
#  define LABEL_TRAITS_FILE LabelSet.def
#endif

// Build "src/LABELS/<LABEL_DIR>/<file>"
#define LBLFILE2(dir, file) STR(src/LABELS/dir/file)
#define LBLFILE(file)       LBLFILE2(LABEL_DIR, file)

// Per-set traits + generated mappings
#include LBLFILE(LABEL_TRAITS_FILE)
#include LBLFILE(CT_Display.h)
#include LBLFILE(DCSBIOSBridgeData.h)
#include LBLFILE(InputMapping.h)
#include LBLFILE(LEDMapping.h)
#include LBLFILE(DisplayMapping.h)

// ---- Friendly name + USB strings ----
// In your LabelSet.def you may define either or both:
//   #define LABEL_SET_NAME     "TEST ONLY"
//   #define LABEL_SET_FULLNAME "CockpitOS Firmware: Test Only"

#ifndef LABEL_SET_NAME
#  define LABEL_SET_NAME STR(LABEL_SET)     // fallback to token
#endif
#ifndef LABEL_SET_FULLNAME
#  define LABEL_SET_FULLNAME LABEL_SET_NAME // fallback to short name
#endif

#ifdef USB_SERIAL
#  undef USB_SERIAL
#endif
#define USB_SERIAL  LABEL_SET_FULLNAME
*/