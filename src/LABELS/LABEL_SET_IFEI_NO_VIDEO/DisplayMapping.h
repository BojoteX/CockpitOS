// DisplayMapping.h - Auto-generated
#pragma once

#define HAS_IFEI_SEGMENT_MAP
#include "IFEI_SegmentMap.h"

// Enums for FieldType, DisplayDeviceType
enum FieldType { FIELD_LABEL, FIELD_STRING, FIELD_NUMERIC, FIELD_MIXED, FIELD_BARGRAPH };
enum DisplayDeviceType { DISPLAY_HUD, DISPLAY_IFEI, DISPLAY_UFC };

class HUDDisplay;
class IFEIDisplay;
class UFCDisplay;

enum FieldRenderType : uint8_t {
    FIELD_RENDER_7SEG,
    FIELD_RENDER_7SEG_SHARED,
    FIELD_RENDER_LABEL,
    FIELD_RENDER_BINGO,
    FIELD_RENDER_BARGRAPH,
    FIELD_RENDER_FUEL,
    FIELD_RENDER_RPM,
    FIELD_RENDER_ALPHA_NUM_FUEL,
    FIELD_RENDER_CUSTOM,
};

// Structure for a field definition
struct DisplayFieldDefLabel {
    const char* label;
    const SegmentMap* segMap;
    uint8_t numDigits;
    uint8_t segsPerDigit;
    int minValue;
    int maxValue;
    FieldType type;
    uint8_t barCount;
    void* driver;
    DisplayDeviceType deviceType;
    void (*renderFunc)(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
    void (*clearFunc)(void*, const SegmentMap*, const DisplayFieldDefLabel&);
    FieldRenderType renderType;
};

struct FieldState {
    char lastValue[8];
};

const DisplayFieldDefLabel* findFieldByLabel(const char* label);
const DisplayFieldDefLabel* findFieldDefByLabel(const char* label);

extern const DisplayFieldDefLabel fieldDefs[];
extern size_t numFieldDefs;
extern FieldState fieldStates[];
extern HUDDisplay hud;
extern IFEIDisplay ifei;
extern UFCDisplay ufc;
extern void renderHUDDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearHUDDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderIFEIDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearIFEIDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderUFCDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearUFCDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
// Function pointers (renderFunc, clearFunc) are nullptr in all generated records unless otherwise preserved.
