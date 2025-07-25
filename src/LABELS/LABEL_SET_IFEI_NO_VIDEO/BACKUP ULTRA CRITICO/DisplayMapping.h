#pragma once

#include "IFEI_SegmentMap.h"

// Enums for FieldType, DisplayDeviceType
enum FieldType { FIELD_LABEL, FIELD_STRING, FIELD_NUMERIC, FIELD_MIXED, FIELD_BARGRAPH };
enum DisplayDeviceType { DISPLAY_IFEI, DISPLAY_UFC /*, ... */ };

// Forward declarations (include real class headers in panel cpp/h)
class IFEIDisplay;
class UFCDisplay;

enum FieldRenderType : uint8_t {
    FIELD_RENDER_7SEG,
    FIELD_RENDER_LABEL,
    FIELD_RENDER_BINGO,
    FIELD_RENDER_BARGRAPH,
    FIELD_RENDER_FUEL,
    FIELD_RENDER_RPM,
    FIELD_RENDER_ALPHA_NUM_FUEL,
    FIELD_RENDER_CUSTOM, // For oddballs or future expansion
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

// FieldState struct, extern fieldDefs[], etc.
struct FieldState {
    char lastValue[8];
    // Add more as needed
};

extern const DisplayFieldDefLabel fieldDefs[];
extern size_t numFieldDefs;
extern FieldState fieldStates[];

// --- Panel-specific driver instance/externs
extern IFEIDisplay ifei;
extern void renderIFEIDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearIFEIDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
// ... (same for UFC, etc.)