// DisplayMapping.h - Auto-generated
#pragma once

#include "../../../lib/CUtils/src/CUtils.h"


// Enums for FieldType, DisplayDeviceType
enum FieldType { FIELD_LABEL, FIELD_STRING, FIELD_NUMERIC, FIELD_MIXED, FIELD_BARGRAPH };
enum DisplayDeviceType { DISPLAY_ARC210, DISPLAY_CDU, DISPLAY_CLOCK, DISPLAY_CMSC, DISPLAY_CMSP1, DISPLAY_CMSP2, DISPLAY_ILS, DISPLAY_TACAN, DISPLAY_UHF, DISPLAY_VHF };

class ARC210Display;
class CDUDisplay;
class CLOCKDisplay;
class CMSCDisplay;
class CMSP1Display;
class CMSP2Display;
class ILSDisplay;
class TACANDisplay;
class UHFDisplay;
class VHFDisplay;

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
extern ARC210Display arc210;
extern CDUDisplay cdu;
extern CLOCKDisplay clock;
extern CMSCDisplay cmsc;
extern CMSP1Display cmsp1;
extern CMSP2Display cmsp2;
extern ILSDisplay ils;
extern TACANDisplay tacan;
extern UHFDisplay uhf;
extern VHFDisplay vhf;
extern void renderARC210Dispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearARC210Dispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderCDUDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearCDUDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderCLOCKDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearCLOCKDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderCMSCDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearCMSCDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderCMSP1Dispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearCMSP1Dispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderCMSP2Dispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearCMSP2Dispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderILSDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearILSDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderTACANDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearTACANDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderUHFDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearUHFDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
extern void renderVHFDispatcher(void*, const SegmentMap*, const char*, const DisplayFieldDefLabel&);
extern void clearVHFDispatcher(void*, const SegmentMap*, const DisplayFieldDefLabel&);
// Function pointers (renderFunc, clearFunc) are nullptr in all generated records unless otherwise preserved.
