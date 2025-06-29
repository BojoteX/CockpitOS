// IFEIPanel.cpp

#define RUN_IFEI_DISPLAY_AS_TASK 0
#define IFEI_DISPLAY_REFRESH_RATE_HZ 120 // Profilers show better performance when doing fast partial commits, so even 1000Hz works well 

#include "../Globals.h"
#include "../IFEIPanel.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"

// To enforce refresh rate for the display
static uint32_t lastCommitTimeMs = 0;

// Bit mapping (top-to-bottom, see your test order and confirmed labels)
enum {
    IFEI_MODE_BIT = 0,  // 0x01
    IFEI_QTY_BIT = 1,  // 0x02
    IFEI_UP_BIT = 2,  // 0x04
    IFEI_DWN_BIT = 3,  // 0x08
    IFEI_ZONE_BIT = 4,  // 0x10
    IFEI_ET_BIT = 5   // 0x20
};

// --- Button labels (order: top to bottom on panel) ---
static const char* const IFEI_BUTTON_LABELS[6] = {
    "IFEI_MODE_BTN",
    "IFEI_QTY_BTN",
    "IFEI_UP_BTN",
    "IFEI_DWN_BTN",
    "IFEI_ZONE_BTN",
    "IFEI_ET_BTN"
};

// [a, b, c, d, e, f, g] -- top, top-right, bottom-right, bottom, bottom-left, top-left, middle
// INDEX:  0    1     2     3      4      5      6
static const uint8_t seg7_ascii[128] = {
    0x00, //   0
    0x00, //   1
    0x00, //   2
    0x00, //   3
    0x00, //   4
    0x00, //   5
    0x00, //   6
    0x00, //   7
    0x00, //   8
    0x00, //   9
    0x00, //  10
    0x00, //  11
    0x00, //  12
    0x00, //  13
    0x00, //  14
    0x00, //  15
    0x00, //  16
    0x00, //  17
    0x00, //  18
    0x00, //  19
    0x00, //  20
    0x00, //  21
    0x00, //  22
    0x00, //  23
    0x00, //  24
    0x00, //  25
    0x00, //  26
    0x00, //  27
    0x00, //  28
    0x00, //  29
    0x00, //  30
    0x00, //  31
    0x00, //  32  ' '
    0x86, //  33  '!'
    0x22, //  34  '"'
    0x7E, //  35  '#'
    0x6D, //  36  '$'
    0xD2, //  37  '%'
    0x46, //  38  '&'
    0x20, //  39  '''
    0x29, //  40  '('
    0x0B, //  41  ')'
    0x21, //  42  '*'
    0x70, //  43  '+'
    0x10, //  44  ','
    0x40, //  45  '-'
    0x80, //  46  '.'
    0x52, //  47  '/'
    0x3F, //  48  '0'
    0x06, //  49  '1'
    0x5B, //  50  '2'
    0x4F, //  51  '3'
    0x66, //  52  '4'
    0x6D, //  53  '5'
    0x7D, //  54  '6'
    0x07, //  55  '7'
    0x7F, //  56  '8'
    0x6F, //  57  '9'
    0x09, //  58  ':'
    0x0D, //  59  ';'
    0x61, //  60  '<'
    0x48, //  61  '='
    0x43, //  62  '>'
    0xD3, //  63  '?'
    0x5F, //  64  '@'
    0x77, //  65  'A'
    0x7C, //  66  'B'
    0x39, //  67  'C'
    0x5E, //  68  'D'
    0x79, //  69  'E'
    0x71, //  70  'F'
    0x3D, //  71  'G'
    0x76, //  72  'H'
    0x30, //  73  'I'
    0x1E, //  74  'J'
    0x75, //  75  'K'
    0x38, //  76  'L'
    0x15, //  77  'M'
    0x37, //  78  'N'
    0x3F, //  79  'O'
    0x73, //  80  'P'
    0x6B, //  81  'Q'
    0x33, //  82  'R'
    0x6D, //  83  'S'
    0x78, //  84  'T'
    0x3E, //  85  'U'
    0x3E, //  86  'V'
    0x2A, //  87  'W'
    0x76, //  88  'X'
    0x6E, //  89  'Y'
    0x5B, //  90  'Z'
    0x39, //  91  '['
    0x64, //  92  '\'
    0x0F, //  93  ']'
    0x23, //  94  '^'
    0x08, //  95  '_'
    0x02, //  96  '`'
    0x5F, //  97  'a'
    0x7C, //  98  'b'
    0x58, //  99  'c'
    0x5E, // 100  'd'
    0x7B, // 101  'e'
    0x71, // 102  'f'
    0x6F, // 103  'g'
    0x74, // 104  'h'
    0x10, // 105  'i'
    0x0C, // 106  'j'
    0x75, // 107  'k'
    0x30, // 108  'l'
    0x14, // 109  'm'
    0x54, // 110  'n'
    0x5C, // 111  'o'
    0x73, // 112  'p'
    0x67, // 113  'q'
    0x50, // 114  'r'
    0x6D, // 115  's'
    0x78, // 116  't'
    0x1C, // 117  'u'
    0x1C, // 118  'v'
    0x14, // 119  'w'
    0x76, // 120  'x'
    0x6E, // 121  'y'
    0x5B, // 122  'z'
    0x46, // 123  '{'
    0x30, // 124  '|'
    0x70, // 125  '}'
    0x01, // 126  '~'
    0x00, // 127  (DEL)
};

// Custom order is 0=c, 1=g2, 2=b, 3=m, 4=l, 5=j, 6=a, 7=d, 8=k, 9=i, 10=h, 11=e, 12=g1, 13=f
uint16_t seg14_ascii[128] = {
    0x0000, //   0
    0x0000, //   1
    0x0000, //   2
    0x0000, //   3
    0x0000, //   4
    0x0000, //   5
    0x0000, //   6
    0x0000, //   7
    0x0000, //   8
    0x0000, //   9
    0x0000, //  10
    0x0000, //  11
    0x0000, //  12
    0x0000, //  13
    0x0000, //  14
    0x0000, //  15
    0x0000, //  16
    0x0000, //  17
    0x0000, //  18
    0x0000, //  19
    0x0000, //  20
    0x0000, //  21
    0x0000, //  22
    0x0000, //  23
    0x0000, //  24
    0x0000, //  25
    0x0000, //  26
    0x0000, //  27
    0x0000, //  28
    0x0000, //  29
    0x0000, //  30
    0x0000, //  31
    0x0000, //  32  (space)
    0x0846, //  33  !
    0x0402, //  34  "
    0x1297, //  35  #
    0x32D3, //  36  $
    0x373B, //  37  %
    0x1EC8, //  38  &
    0x0400, //  39  '
    0x0028, //  40  (
    0x0500, //  41  )
    0x173A, //  42  *
    0x1212, //  43  +
    0x0100, //  44  ,
    0x1002, //  45  -
    0x8000, //  46  .
    0x0120, //  47  /
    0x29E5, //  48  0
    0x0025, //  49  1
    0x18C6, //  50  2
    0x00C7, //  51  3
    0x3007, //  52  4
    0x30C8, //  53  5
    0x38C3, //  54  6
    0x0045, //  55  7
    0x38C7, //  56  8
    0x30C7, //  57  9
    0x0210, //  58  :
    0x0300, //  59  ;
    0x1028, //  60  <
    0x1082, //  61  =
    0x0502, //  62  >
    0x2056, //  63  ?
    0x0AC6, //  64  @
    0x3847, //  65  A
    0x02D7, //  66  B
    0x28C0, //  67  C
    0x02D5, //  68  D
    0x38C0, //  69  E
    0x3840, //  70  F
    0x28C3, //  71  G
    0x3807, //  72  H
    0x02D0, //  73  I
    0x0885, //  74  J
    0x3828, //  75  K
    0x2880, //  76  L
    0x2C25, //  77  M (set bit 11 to light 'e')
    0x240D, //  78  N
    0x28C5, //  79  O
    0x3846, //  80  P
    0x28CD, //  81  Q
    0x384E, //  82  R
    0x30C3, //  83  S
    0x0250, //  84  T
    0x2885, //  85  U
    0x2920, //  86  V
    0x290D, //  87  W
    0x0528, //  88  X
    0x3087, //  89  Y
    0x01E0, //  90  Z
    0x28C0, //  91  [
    0x0408, //  92  \
    0x00C5, //  93  ]
    0x0108, //  94  ^
    0x0080, //  95  _
    0x0400, //  96  `
    0x1890, //  97  a
    0x3888, //  98  b
    0x1882, //  99  c
    0x0187, // 100  d
    0x1980, // 101  e
    0x1032, // 102  f
    0x00A7, // 103  g
    0x3810, // 104  h
    0x0010, // 105  i
    0x0B00, // 106  j
    0x0238, // 107  k
    0x2800, // 108  l
    0x1813, // 109  m
    0x1810, // 110  n
    0x1883, // 111  o
    0x3C00, // 112  p
    0x0027, // 113  q
    0x1800, // 114  r
    0x008A, // 115  s
    0x3880, // 116  t
    0x0881, // 117  u
    0x0900, // 118  v
    0x0909, // 119  w
    0x0528, // 120  x
    0x0287, // 121  y
    0x1180, // 122  z
    0x14C0, // 123  {
    0x0210, // 124  |
    0x0501, // 125  }
    0x120A, // 126  ~
    0x0000, // 127  (del)
};

// Helper for 7-Seg to char
char lookup7SegToChar(uint8_t pattern) {
    // 1. Digits first
    for (int c = '0'; c <= '9'; ++c)
        if (seg7_ascii[c] == pattern) return (char)c;
    // 2. Uppercase next
    for (int c = 'A'; c <= 'Z'; ++c)
        if (seg7_ascii[c] == pattern) return (char)c;
    // 3. Lowercase next
    for (int c = 'a'; c <= 'z'; ++c)
        if (seg7_ascii[c] == pattern) return (char)c;
    // 4. Symbols
    for (int c = 32; c < 127; ++c)
        if (seg7_ascii[c] == pattern) return (char)c;
    return ' ';
}

// Helper for 14-Seg to char
char lookup14SegToChar(uint16_t pattern) {
    // 1. Try digits first
    for (int c = '0'; c <= '9'; ++c)
        if (seg14_ascii[c] == pattern) return (char)c;
    // 2. Uppercase next
    for (int c = 'A'; c <= 'Z'; ++c)
        if (seg14_ascii[c] == pattern) return (char)c;
    // 3. Lowercase next
    for (int c = 'a'; c <= 'z'; ++c)
        if (seg14_ascii[c] == pattern) return (char)c;
    // 4. Symbols (and any printable ASCII)
    for (int c = 32; c < 127; ++c)
        if (seg14_ascii[c] == pattern) return (char)c;
    return ' ';
}

// Utility
bool isFieldBlank(const char* s) {
    if (!s) return true;
    while (*s) { if (*s != ' ') return false; ++s; }
    return true;
}

static uint8_t prevButtonBits = 0xFF;

// ---- Instantiate chips and driver ----
HT1622 chip0(CS0_PIN, WR0_PIN, DATA0_PIN);
HT1622 chip1(CS1_PIN, WR1_PIN, DATA1_PIN);
HT1622* chips[IFEI_MAX_CHIPS] = { &chip0, &chip1 };
IFEIDisplay ifei(chips);

static uint8_t currentIFEIMode = 0;
static uint8_t currentIFEIIntensity = 255; // default to max for power-up

inline void ensureBacklightPins() {
    static bool pinsInitialized = false;
    if (!pinsInitialized) {
        pinMode(BL_WHITE_PIN, OUTPUT);
        pinMode(BL_GREEN_PIN, OUTPUT);
        pinMode(BL_NVG_PIN, OUTPUT);
        pinsInitialized = true;
    }
}

// Backlight for IFEI: 0=Day, 1=Nite, 2=NVG
void setBacklightMode(uint8_t mode, uint8_t brightness) {
    static uint8_t lastMode = 0xFF;      // impossible startup value
    static uint8_t lastBrightness = 0xFF;

    if (mode == lastMode && brightness == lastBrightness)
        return;

    ensureBacklightPins();

    // First, shut off all to avoid ghosting if switching between modes
    if (lastMode != mode) {
        analogWrite(BL_WHITE_PIN, 0);
        analogWrite(BL_GREEN_PIN, 0);
        analogWrite(BL_NVG_PIN, 0);
    }

    switch (mode) {
    case 0:
        analogWrite(BL_WHITE_PIN, brightness);
        if (DEBUG) debugPrintf("🔆 IFEI White Backlight intensity set to %u\n", brightness);
        break;
    case 1:
        analogWrite(BL_NVG_PIN, brightness);
        if (DEBUG) debugPrintf("🔆 IFEI Nite Backlight intensity set to %u\n", brightness);
        break;
    case 2:
        analogWrite(BL_NVG_PIN, brightness);
        if (DEBUG) debugPrintf("🔆 IFEI NVG Backlight intensity set to %u\n", brightness);
        break;
    default:
        // All off
        if (DEBUG) debugPrintln("⚫ IFEI Backlight OFF");
        break;
    }
    lastMode = mode;
    lastBrightness = brightness;
}

void showLampTest() {
    // Lamp test: cycle through all backlight modes and turn on all segments for visual check

    // White lamp test
    setBacklightMode(0, 255); // 0 = white
    for (int i = 0; i < IFEI_MAX_CHIPS; ++i)
        if (chips[i]) chips[i]->allSegmentsOn();
    delay(1000);

    // Green lamp test
    setBacklightMode(1, 255); // 1 = green
    for (int i = 0; i < IFEI_MAX_CHIPS; ++i)
        if (chips[i]) chips[i]->allSegmentsOn();
    delay(1000);

    // NVG lamp test (if you want)
    setBacklightMode(2, 255); // 2 = NVG
    for (int i = 0; i < IFEI_MAX_CHIPS; ++i)
        if (chips[i]) chips[i]->allSegmentsOn();
    delay(1000);

    // All off
    for (int i = 0; i < IFEI_MAX_CHIPS; ++i)
        if (chips[i]) chips[i]->allSegmentsOff();
    setBacklightMode(0xFF, 0); // 0xFF = invalid/off
}

void onBackLightIntensityChange(const char* label, uint16_t value, uint16_t max_value) {
    // Intensity was updated by sim
    currentIFEIIntensity = (value * 255UL) / max_value;
    setBacklightMode(currentIFEIMode, currentIFEIIntensity);
}

void onBackLightChange(const char* label, uint16_t val) {
    // Mode was updated by sim
    currentIFEIMode = val;
    setBacklightMode(currentIFEIMode, currentIFEIIntensity);
}

/*
void onBackLightIntensityChange(const char* label, uint16_t value, uint16_t max_value) {
    int16_t backlightMode = getLastKnownState("COCKKPIT_LIGHT_MODE_SW");
    // FIX: Use 255 for analogWrite, not 100!
    uint8_t intensity = (value * 255UL) / max_value;
    setBacklightMode(backlightMode, intensity);
}

void onBackLightChange(const char* label, uint16_t val) {
    setBacklightMode(val, 255);
}
*/

inline void itoa_percent(char* out, int val) {
    if (val >= 100) { out[0] = '1'; out[1] = '0'; out[2] = '0'; out[3] = 0; return; }
    if (val >= 10)  { out[0] = '0' + val / 10; out[1] = '0' + val % 10; out[2] = 0; return; }
    out[0] = '0' + val; out[1] = 0;
}

// ---- Exclusive mode trackers (SP, CODES) ----
static volatile bool isSPon = false;
static volatile bool isCODESon = false;

// ---- Exclusive mode trackers (for FUEL/ALPHA_NUM_FUEL shared exclusivity) ----
static volatile bool isTimeSetModeOn = false;
static volatile bool isTestModeOn    = false;

// Since there is no LABEL for Nozzles we create them

static const DisplayFieldDefLabel nozzleL = { "IFEI_NOZZLE_L", &IFEI_NOZZLE_L_MAP[0][0], 0, 0, 0, 100, FIELD_NUMERIC, 11, &ifei, DISPLAY_IFEI, renderIFEIDispatcher, nullptr, FIELD_RENDER_BARGRAPH };
static const DisplayFieldDefLabel nozzleR = { "IFEI_NOZZLE_R", &IFEI_NOZZLE_R_MAP[0][0], 0, 0, 0, 100, FIELD_NUMERIC, 11, &ifei, DISPLAY_IFEI, renderIFEIDispatcher, nullptr, FIELD_RENDER_BARGRAPH };

static FieldState nozzleLState = { "" };
static FieldState nozzleRState = { "" };

// ---- Nozzle pointer trackers ----
static int lastPercentL = -1;
static int lastPercentR = -1;
static bool showLeftNozPointer = false;
static bool showRightNozPointer = false;

void updateLEFTNozzle(const char* label, uint16_t value) {
    int percentL = (value * 100 + 32767) / 65535;
    if (percentL > 100) percentL = 100;
    if (percentL < 0) percentL = 0;

    // Use global lastPercentL and showLeftNozPointer
    static bool lastPointerOn = false;
    if (percentL == lastPercentL && showLeftNozPointer == lastPointerOn)
        return;
    lastPercentL = percentL;
    lastPointerOn = showLeftNozPointer;

    char buf[4] = "";
    if (showLeftNozPointer) {
        itoa_percent(buf, percentL);
    }

    // Direct dispatcher call—bypasses renderField
    renderIFEIDispatcher(nozzleL.driver, nozzleL.segMap, buf, nozzleL);

}

void updateRIGHTNozzle(const char* label, uint16_t value) {
    int percentR = (value * 100 + 32767) / 65535;
    if (percentR > 100) percentR = 100;
    if (percentR < 0) percentR = 0;

    static bool lastPointerOn = false;
    if (percentR == lastPercentR && showRightNozPointer == lastPointerOn)
        return;
    lastPercentR = percentR;
    lastPointerOn = showRightNozPointer;

    char buf[4] = "";
    if (showRightNozPointer) {
        itoa_percent(buf, percentR);
    }

    // Direct dispatcher call—bypasses renderField
    renderIFEIDispatcher(nozzleR.driver, nozzleR.segMap, buf, nozzleR);

}

#if RUN_IFEI_DISPLAY_AS_TASK
void IFEIDisplayTask(void* pv) {
    const TickType_t tickDelay = pdMS_TO_TICKS(1000 / IFEI_DISPLAY_REFRESH_RATE_HZ);
    while (true) {

        // Commit to the Display on every iteration (it will skip if shadow buffer is unchanged)
        ifei.commit();              

        vTaskDelay(tickDelay);

    }
}
#endif

// Imit for the buttons, NOT the Display
void IFEI_init() {

    delay(50);  // Small delay to ensure when init is called DCS has settled

    HC165_init(HC165_PL, HC165_CP, HC165_QH);

    // Optionally read and set initial state
    prevButtonBits = HC165_read();

    // Clear shadow/state, INCLUDING all integrer Buffers
    ifei.blankBuffersAndDirty();

    debugPrintln("✅ Initialized IFEI Buttons and cleared IFEI display");
}

// Loop for the buttons, NOT the Display
void IFEI_loop() {
    static unsigned long lastIFEIPoll = 0;
    if (!shouldPollMs(lastIFEIPoll)) return;

    uint8_t buttonBits = HC165_read();

    for (uint8_t i = 0; i < 6; ++i) {
        uint8_t mask = (1 << i);
        // Edge detection: only on change
        if ((prevButtonBits ^ buttonBits) & mask) {
            // active-low: pressed = 0
            bool pressed = !(buttonBits & mask);
            HIDManager_setNamedButton(IFEI_BUTTON_LABELS[i], false, pressed);
        }
    }
    prevButtonBits = buttonBits;  
}

// Init for the actual Display only
void IFEIDisplay_init() {

    delay(50);  // Small delay to ensure when init is called DCS has settled

    ifei.buildCommitRegions();  // Automatically builds per-field update regions

    // Initialize hardware driver(s)
    chip0.init();
    chip1.init();

    // Set backlight
    showLampTest();
    setBacklightMode(0,255); // White backlight
    ifei.clear();

    // Subscriptions to events
    subscribeToLedChange("IFEI_DISP_INT_LT", onBackLightIntensityChange);
    subscribeToSelectorChange("COCKKPIT_LIGHT_MODE_SW", onBackLightChange);
    subscribeToMetadataChange("EXT_NOZZLE_POS_L", updateLEFTNozzle);
    subscribeToMetadataChange("EXT_NOZZLE_POS_R", updateRIGHTNozzle);

    #if RUN_IFEI_DISPLAY_AS_TASK
    // Create FreeRTOS task to update our display
    xTaskCreate(IFEIDisplayTask, "IFEIDisplay", 4096, NULL, 1, NULL);
    #endif

    /*
    // Register our Display buffers automatically (the ones starting with IFEI_)
    for (size_t i = 0; i < numCTDisplayBuffers; ++i) {
        const auto& e = CT_DisplayBuffers[i];
        if (strncmp(e.label, "IFEI_", 5) == 0) {
            if (registerDisplayBuffer(e.label, e.buffer, e.length, e.dirty, e.last)) {
                if (DEBUG) debugPrintf("[IFEI_INIT] Registered display buffer: %s (len=%u, ptr=%p, dirty=%p), last=%p\n", e.label, e.length, (void*)e.buffer, (void*)e.dirty, (void*)e.last);
            }
        }
    }    
    */

    debugPrintln("✅ Initialized IFEI Display");
}

// Loop for Display only
void IFEIDisplay_loop() {

        #if !RUN_IFEI_DISPLAY_AS_TASK  

        // Commit to the Display on every iteration (it will skip if shadow buffer is unchanged)
        ifei.commit();          

        #endif // RUN_IFEI_DISPLAY_AS_TASK
}

void renderIFEIDispatcher(void* drv, const SegmentMap* segMap, const char* value, const DisplayFieldDefLabel& def) {
    IFEIDisplay* display = reinterpret_cast<IFEIDisplay*>(drv);

	if (!isMissionRunning()) return; // Do not render if mission is not running

    if(DEBUG) {
        // For debugging only
        char buf[14];
        display->readRegionFromShadow(segMap, def.numDigits, def.segsPerDigit, display->getRamShadow(), buf, sizeof(buf));
        debugPrintf("🔁 Shadow buffer contents for %s is %s and value in renderer is %s\n", def.label, buf, value);
    }

    switch (def.renderType) {
        case FIELD_RENDER_7SEG:
            display->addAsciiString7SegToShadow(value, segMap, def.numDigits);
            break;

        // Special case handling for shared IFEI_SP, IFEI_CODES, IFEI_TEMP_L & IFEI_TEMP_R
        case FIELD_RENDER_7SEG_SHARED: {
            if (strcmp(def.label, "IFEI_SP") == 0) {
                bool prev = isSPon;
                isSPon = !isFieldBlank(value);
            }
            if (strcmp(def.label, "IFEI_CODES") == 0) {
                bool prev = isCODESon;
                isCODESon = !isFieldBlank(value);
            }

            bool isTempField = (strcmp(def.label, "IFEI_TEMP_L") == 0 || strcmp(def.label, "IFEI_TEMP_R") == 0);

            if ((isSPon || isCODESon) && isTempField) {
                char buf[8] = {0};
                return;
            }

            // Proceed with update
            display->addAsciiString7SegToShadow(value, segMap, def.numDigits);
            break;
        }

        case FIELD_RENDER_LABEL: {
            display->addLabelToShadow(*reinterpret_cast<const SegmentMap*>(segMap), value);
            break;
        }

        case FIELD_RENDER_BINGO:
            display->addBingoStringToShadow(value, (const SegmentMap(*)[7])segMap);
            break;

        case FIELD_RENDER_BARGRAPH:
            if (strcmp(def.label, "IFEI_LPOINTER_TEXTURE") == 0) {
                if (value && value[0] == '1' && value[1] == 0) {
                    display->addPointerBarToShadow(lastPercentL, &IFEI_NOZZLE_L_MAP[0][0], 11); // Fixed: pass flat pointer
                    showLeftNozPointer = true;
                }
                else {
                    showLeftNozPointer = false;
                    display->clearBarFromShadow(&IFEI_NOZZLE_L_MAP[0][0], 11);
                }
            }
            else if (strcmp(def.label, "IFEI_RPOINTER_TEXTURE") == 0) {
                if (value && value[0] == '1' && value[1] == 0) {
                    display->addPointerBarToShadow(lastPercentR, &IFEI_NOZZLE_R_MAP[0][0], 11); // Fixed: pass flat pointer
                    showRightNozPointer = true;
                }
                else {
                    showRightNozPointer = false;
                    display->clearBarFromShadow(&IFEI_NOZZLE_R_MAP[0][0], 11);
                }
            }
            else {
                int percent = strToIntFast(value);
                if (percent < 0) percent = 0;
                if (percent > 100) percent = 100;

                // Custom logic for the nozzles
                if (showRightNozPointer && strcmp(def.label, "IFEI_NOZZLE_R") == 0)
                    display->addPointerBarToShadow(percent, segMap, 11); // segMap is already a flat pointer

                if (showLeftNozPointer && strcmp(def.label, "IFEI_NOZZLE_L") == 0)
                    display->addPointerBarToShadow(percent, segMap, 11);
            }
            break;

        case FIELD_RENDER_RPM:
            display->addRpmStringToShadow(value, (const SegmentMap(*)[7])segMap);
            break;

        case FIELD_RENDER_ALPHA_NUM_FUEL:
        case FIELD_RENDER_FUEL: {
            // 1. Update exclusive mode trackers for special fields
            if (strcmp(def.label, "IFEI_T") == 0) {
                bool prev = isTestModeOn;
                isTestModeOn = !isFieldBlank(value);
            }
            if (strcmp(def.label, "IFEI_TIME_SET_MODE") == 0) {
                bool prev = isTimeSetModeOn;
                isTimeSetModeOn = !isFieldBlank(value);
            }

            // 2. Detect if this is a FUEL_UP or FUEL_DOWN field
            bool isFuelField =
                (strcmp(def.label, "IFEI_FUEL_UP") == 0) ||
                (strcmp(def.label, "IFEI_FUEL_DOWN") == 0);

            // 3. Exclusive mode logic (skip FUEL updates if in either special mode)
            if ((isTestModeOn || isTimeSetModeOn) && isFuelField) {
                // (Optional: debug)
                // char buf[16];
                // display->readRegionFromShadow(segMap, def.numDigits, def.segsPerDigit, display->getRamShadow(), buf, sizeof(buf));
                // debugPrintf("[EXCL] Skipping %s in exclusive mode\n", def.label);
                return;
            }

            // 4. Proceed with update
            display->addAlphaNumFuelStringToShadow(value, (const SegmentMap(*)[14])segMap);
            break;
        }

        case FIELD_RENDER_CUSTOM:
            debugPrintf("❌ Label %s does not have a matching case\n", def.label);
            // Custom or error handler
            break;
    }
}

void clearIFEIDispatcher(void* drv, const SegmentMap* segMap, const DisplayFieldDefLabel& def) {
    IFEIDisplay* display = reinterpret_cast<IFEIDisplay*>(drv);

    if (!isMissionRunning()) return; // Do not render if mission is not running

    debugPrintf("[DEBUG] Clear Function called for %s\n", def.label);

    switch (def.renderType) {
        case FIELD_RENDER_7SEG:
            display->clear7SegFromShadow(segMap, def.numDigits);
            break;
        case FIELD_RENDER_LABEL:
            display->clearLabelFromShadow(segMap);
            break;
        case FIELD_RENDER_BINGO:
            display->clearBingoFromShadow(reinterpret_cast<const SegmentMap(*)[7]>(segMap));
            break;
        case FIELD_RENDER_BARGRAPH:
            display->clearBarFromShadow(segMap, def.barCount);
            break;
        case FIELD_RENDER_FUEL:
        case FIELD_RENDER_ALPHA_NUM_FUEL:
            display->clearFuelFromShadow(reinterpret_cast<const SegmentMap(*)[14]>(segMap));
            break;
        // You can add more types here as needed for future special cases
        case FIELD_RENDER_CUSTOM:
        default:
            // Fallback: safe clear for 7-segment style
            display->clear7SegFromShadow(segMap, def.numDigits);
            break;
    }
}

IFEIDisplay::IFEIDisplay(HT1622* chips[IFEI_MAX_CHIPS]) {
    for (int i = 0; i < IFEI_MAX_CHIPS; ++i) {
        _chips[i] = chips[i];
        memset(ramShadow[i], 0, 64);
        memset(lastShadow[i], 0xFF, 64);  // Pre-invalidate
    }
}

void IFEIDisplay::clear() {
    for (int i = 0; i < IFEI_MAX_CHIPS; ++i)
        memset(ramShadow[i], 0, 64);
    
    invalidateHardwareCache();  // Ensure commit() actually pushes full blank
    commit(1); // Forced
}

void IFEIDisplay::commitNextRegion() {
    static int currentRegion = 0;

    if (numCommitRegions == 0) return;

    // Sanity check
    if (currentRegion >= numCommitRegions) currentRegion = 0;

    const CommitRegion& region = commitRegions[currentRegion];
    currentRegion = (currentRegion + 1) % numCommitRegions;

    // Defensive: Check chip index
    if (region.chip >= IFEI_MAX_CHIPS) {
        debugPrintf("[IFEI] ERROR: commitNextRegion bad chip=%u (label=%s)\n", region.chip, region.label);
        return;
    }
    HT1622* chip = _chips[region.chip];
    if (!chip) {
        debugPrintf("[IFEI] ERROR: Null chip ptr at idx %u (label=%s)\n", region.chip, region.label);
        return;
    }
    // Defensive: Check addr range
    if (region.addrStart > region.addrEnd || region.addrEnd >= 64 || region.addrStart >= 64) {
        debugPrintf("[IFEI] ERROR: commitNextRegion bad addr %u..%u (label=%s)\n", region.addrStart, region.addrEnd, region.label);
        return;
    }

    #if DEBUG_PERFORMANCE
    beginProfiling(PERF_DISPLAY_RENDER);
    #endif

    chip->commitPartial(ramShadow[region.chip], lastShadow[region.chip],
                        region.addrStart, region.addrEnd);

    #if DEBUG_PERFORMANCE
    endProfiling(PERF_DISPLAY_RENDER);
    #endif                        
}

void IFEIDisplay::buildCommitRegions() {
    numCommitRegions = 0;
    for (size_t i = 0; i < numFieldDefs; ++i) {
        const DisplayFieldDefLabel& def = fieldDefs[i];
        const SegmentMap* map = reinterpret_cast<const SegmentMap*>(def.segMap);

        uint8_t minAddr = 0xFF, maxAddr = 0x00, chipId = 0xFF;
        bool valid = false;

        for (int d = 0; d < def.numDigits; ++d) {
            for (int s = 0; s < def.segsPerDigit; ++s) {
                int idx = d * def.segsPerDigit + s;
                const SegmentMap& seg = map[idx];
                if (seg.addr == 0xFF || seg.bit == 0xFF) continue;
                if (seg.addr >= 64 || seg.bit >= 4) {
                    debugPrintf("[IFEI] BAD SEGMENT: %s d=%d s=%d addr=%u bit=%u\n", def.label, d, s, seg.addr, seg.bit);
                    continue;
                }
                if (chipId == 0xFF) chipId = seg.ledID;
                if (seg.ledID != chipId) continue;
                if (seg.addr < minAddr) minAddr = seg.addr;
                if (seg.addr > maxAddr) maxAddr = seg.addr;
                valid = true;
            }
        }

        // DO NOT OVERRUN ARRAY!
        if (valid) {
            if (numCommitRegions >= MAX_DISPLAY_FIELDS) {
                debugPrintf("[IFEI] ERROR: Overflow commitRegions[] on %s\n", def.label);
                break;
            }
            // Do not allow invalid chipId!
            if (chipId >= IFEI_MAX_CHIPS) {
                debugPrintf("[IFEI] ERROR: Bad chipId %u for field %s\n", chipId, def.label);
                continue;
            }
            // Do not allow bogus addr ranges!
            if (minAddr > maxAddr || minAddr >= 64 || maxAddr >= 64) {
                debugPrintf("[IFEI] ERROR: Bad addr range for %s: %u..%u\n", def.label, minAddr, maxAddr);
                continue;
            }
            commitRegions[numCommitRegions++] = {
                def.label,
                chipId,
                minAddr,
                maxAddr
            };
        }
    }
}

void IFEIDisplay::commit(bool force) {
    // User-configurable refresh rate in Hz (set elsewhere or pass as argument if needed)
    constexpr uint32_t MIN_COMMIT_INTERVAL_MS = 1000 / IFEI_DISPLAY_REFRESH_RATE_HZ;

    uint32_t now = millis();
    if (!force && (now - lastCommitTimeMs < MIN_COMMIT_INTERVAL_MS)) {
        return;
    }
    lastCommitTimeMs = now;

    #if DEBUG_PERFORMANCE
    beginProfiling(PERF_DISPLAY_RENDER);
    #endif

    for (int chip = 0; chip < IFEI_MAX_CHIPS; ++chip) {
        if (!_chips[chip]) continue;
        if (memcmp(ramShadow[chip], lastShadow[chip], 64) != 0)
            _chips[chip]->commit(ramShadow[chip], lastShadow[chip], 64);
    }

    #if DEBUG_PERFORMANCE
    endProfiling(PERF_DISPLAY_RENDER);
    #endif

}

void IFEIDisplay::blankBuffersAndDirty() {

    // We clear out screen and invalidate the shadow buffer
    clear();

    // Now we reset all string buffers (the ones we automatically registered)
    for (size_t i = 0; i < numCTDisplayBuffers; ++i) {
        DisplayBufferEntry& b = CT_DisplayBuffers[i];
        if (!b.label || strncmp(b.label, "IFEI_", 5) != 0) continue;

        if (b.length > 32) continue;  // Cap safeguard

        for (uint8_t j = 0; j < b.length; ++j) b.buffer[j] = ' ';
        b.buffer[b.length] = '\0';

        memset(b.last, 0xFF, b.length);
        b.last[b.length] = '\0';

        if (b.dirty) *b.dirty = true;
    }
}

void IFEIDisplay::invalidateHardwareCache() {
    for (int i = 0; i < IFEI_MAX_CHIPS; ++i) {
        if (_chips[i])
            _chips[i]->invalidateLastShadow(lastShadow[i], 64);
    }
}

void IFEIDisplay::readRegionFromShadow(
    const SegmentMap* map,
    int numDigits,
    int segsPerDigit,
    uint8_t ramShadow[IFEI_MAX_CHIPS][64],
    char* out,
    size_t outSize
) {
    if (!map || !out || outSize < 2 || numDigits <= 0 || segsPerDigit <= 0) {
        if (out && outSize > 0) out[0] = '\0';
        return;
    }

    const int maxDigits = (outSize > 1) ? (int)(outSize - 1) : 0;
    const int chars = (numDigits < maxDigits) ? numDigits : maxDigits;
    out[0] = '\0';

    // --- Find chip from first valid segment ---
    int chip = -1;
    int mapEntries = numDigits * segsPerDigit;
    for (int idx = 0; idx < mapEntries; ++idx) {
        if (map[idx].addr < 64 && map[idx].bit < 4) {
            chip = map[idx].ledID;
            break;
        }
    }
    if (chip < 0 || chip >= IFEI_MAX_CHIPS) {
        for (int d = 0; d < chars; ++d) out[d] = ' ';
        out[chars] = '\0';
        return;
    }
    const uint8_t* shadow = ramShadow[chip];

    // ---- MAIN SAFE LOOP ----
    for (int d = 0; d < chars; ++d) {
        int baseIdx = d * segsPerDigit;
        int validSegs = 0, litSegs = 0;
        uint8_t segPattern = 0;
        uint16_t seg14Pattern = 0;

        // --- Dynamically count valid segments for THIS digit ---
        for (int s = 0; s < segsPerDigit; ++s) {
            int idx = baseIdx + s;
            if (idx >= mapEntries) break;
            const SegmentMap& seg = map[idx];
            if (seg.addr == 0xFF || seg.bit == 0xFF) break; // End of this digit's map
            if (seg.addr >= 64 || seg.bit >= 4) continue;
            ++validSegs;
            bool isLit = (shadow[seg.addr] & (1 << seg.bit)) != 0;
            if (isLit) {
                ++litSegs;
                segPattern |= (1 << s);
                seg14Pattern |= (1 << s);
            }
        }

        char render = ' ';
        if (validSegs == 0) render = ' ';
        else if (validSegs == 1) render = (litSegs ? '1' : ' '); // Only show '1' if explicitly lit, else blank.
        else if (validSegs == 7) render = lookup7SegToChar(segPattern);
        else if (validSegs == 14) render = lookup14SegToChar(seg14Pattern);
        else render = (litSegs ? '*' : ' ');

        out[d] = render;
    }
    out[chars] = '\0';
}

// Render Methods
void IFEIDisplay::addAlphaNumFuelStringToShadow(const char* str, const SegmentMap map[6][14]) {
    char buf[7] = "      "; // Pad left with spaces if needed (right-align)
    int len = strlen(str);
    if (len > 6) len = 6;
    strncpy(&buf[6 - len], str, len);

    // [0-3]: 7-seg (alpha+num), [4]: single segment, [5]: 14-seg starburst
    for (int d = 0; d < 6; d++) {
        char c = buf[d];
        if (d == 5) { // Starburst (14-seg)
            if (c != ' ') {
                uint16_t segs = seg14_ascii[(uint8_t)c];
                for (int s = 0; s < 14; s++) {
                    const SegmentMap& seg = map[d][s];
                    if (seg.addr < 64 && seg.bit < 4) {
                        if (segs & (1 << s))
                            ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
                        else
                            ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
                    }
                }
            } else {
                // Blank starburst
                for (int s = 0; s < 14; s++) {
                    const SegmentMap& seg = map[d][s];
                    if (seg.addr < 64 && seg.bit < 4)
                        ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
                }
            }
        }
        else if (d == 4) { // Single segment, only '0' is shown
            const SegmentMap& seg = map[d][0];
            if (c == '0') {
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
            } else {
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
            }
            // Blank all other segments in this digit, just in case
            for (int s = 1; s < 14; s++) {
                const SegmentMap& seg = map[d][s];
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
            }
        }
        else if (d >= 0 && d < 4) { // 7-seg: full ASCII lookup
            uint8_t segBits = seg7_ascii[(uint8_t)c];
            for (int s = 0; s < 7; s++) {
                const SegmentMap& seg = map[d][s];
                if (seg.addr < 64 && seg.bit < 4) {
                    if (segBits & (1 << s)) // <<---- THIS IS CORRECT!
                        ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
                    else
                        ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
                }
            }
            // Blank all "upper" bits just in case (for safety, optional)
            for (int s = 7; s < 14; s++) {
                const SegmentMap& seg = map[d][s];
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
            }
        }
    }
}

void IFEIDisplay::addRpmStringToShadow(const char* str, const SegmentMap map[3][7]) {
    int len = strlen(str);

    // 100s: always clear first
    if (map[0][0].addr < 64 && map[0][0].bit < 4)
        ramShadow[map[0][0].ledID][map[0][0].addr] &= ~(1 << map[0][0].bit);

    // Only light if first char is '1'
    if (len == 3 && str[0] == '1' && map[0][0].addr < 64 && map[0][0].bit < 4)
        ramShadow[map[0][0].ledID][map[0][0].addr] |= (1 << map[0][0].bit);

    // 10s and 1s: always clear unused bits (full 7-seg clear for blanks)
    for (int d = 0; d < 2; d++) {
        int strIdx = len - 2 + d;
        uint8_t segBits = 0;
        if (strIdx >= 0) {
            char c = str[strIdx];
            if (c != ' ') segBits = seg7_ascii[(uint8_t)c];
        }
        for (int s = 0; s < 7; s++) {
            if (map[d + 1][s].addr < 64 && map[d + 1][s].bit < 4) {
                if (segBits & (1 << s))
                    ramShadow[map[d + 1][s].ledID][map[d + 1][s].addr] |= (1 << map[d + 1][s].bit);
                else
                    ramShadow[map[d + 1][s].ledID][map[d + 1][s].addr] &= ~(1 << map[d + 1][s].bit);
            }
        }
    }
}

void IFEIDisplay::addFuelStringToShadow(const char* str, const SegmentMap map[6][14]) {
    char buf[7] = "      "; // fill with spaces, right-align
    int len = strlen(str);
    if (len > 6) len = 6;
    strncpy(&buf[6 - len], str, len);

    // buf[0]=100000s (leftmost), buf[5]=1s (rightmost/starburst)
    for (int d = 0; d < 6; d++) {
        char c = buf[d];
        if (d == 5) { // 14-seg
            uint16_t segs = (c != ' ') ? seg14_ascii[(uint8_t)c] : 0;
            for (int s = 0; s < 14; s++) {
                const SegmentMap& seg = map[d][s];
                if (seg.addr < 64 && seg.bit < 4) {
                    if (segs & (1 << s))
                        ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
                    else
                        ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
                }
            }
        }
        else if (d == 4) { // 10s digit (single segment)
            const SegmentMap& seg = map[d][0];
            if (c == '0') {
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
            } else {
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
            }
            // Blank all other segments
            for (int s = 1; s < 14; s++) {
                const SegmentMap& seg = map[d][s];
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
            }
        }
        else if (d >= 0 && d < 4) { // 7-seg
            uint8_t segBits = (c != ' ') ? seg7_ascii[(uint8_t)c] : 0;
            for (int s = 0; s < 7; s++) {
                const SegmentMap& seg = map[d][s];
                if (seg.addr < 64 && seg.bit < 4) {
                    if (segBits & (1 << s))
                        ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
                    else
                        ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
                }
            }
            // Blank all others
            for (int s = 7; s < 14; s++) {
                const SegmentMap& seg = map[d][s];
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
            }
        }
    }
}

void IFEIDisplay::addBingoStringToShadow(const char* str, const SegmentMap map[5][7]) {
    int len = strlen(str);
    for (int d = 0; d < 5; d++) {
        int strIdx = d - (5 - len);
        if (strIdx >= 0) {
            char c = str[strIdx];
            if (d < 3 && c != ' ') { // 7-seg digits
                uint8_t segBits = seg7_ascii[(uint8_t)c];
                for (int s = 0; s < 7; s++) {
                    const SegmentMap& seg = map[d][s];
                    if (seg.addr < 64 && seg.bit < 4) {
                        if (segBits & (1 << s))
                            ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
                        else
                            ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
                    }
                }
            }
            else if (d >= 3 && c != ' ') { // single segment (for 10s and 1s)
                const SegmentMap& seg = map[d][0];
                if (seg.addr < 64 && seg.bit < 4)
                    ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
                // Blank other (if any)
                for (int s = 1; s < 7; s++) {
                    const SegmentMap& seg = map[d][s];
                    if (seg.addr < 64 && seg.bit < 4)
                        ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
                }
            } else {
                // Blank digit
                for (int s = 0; s < 7; s++) {
                    const SegmentMap& seg = map[d][s];
                    if (seg.addr < 64 && seg.bit < 4)
                        ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
                }
            }
        }
    }
}

void IFEIDisplay::addPointerBarToShadow(int percent, const SegmentMap* barMap, int numBars) {
    int barIdx = constrain((percent + 5) / 10, 0, numBars - 1);
    for (int i = 0; i < numBars; ++i) {
        if (barMap[i].addr < 64 && barMap[i].bit < 4)
            ramShadow[barMap[i].ledID][barMap[i].addr] &= ~(1 << barMap[i].bit);
    }
    if (barIdx >= 0 && barMap[barIdx].addr < 64 && barMap[barIdx].bit < 4)
        ramShadow[barMap[barIdx].ledID][barMap[barIdx].addr] |= (1 << barMap[barIdx].bit);
}

void IFEIDisplay::addLabelToShadow(const SegmentMap& label, const char* value) {
    bool set = false;
    if (value && value[0] && value[1] == 0 && value[0] != '0' && value[0] > ' ' && value[0] <= '~') {
        // Any single visible ASCII except '0'
        set = true;
    }
    if (label.addr < 64 && label.bit < 4) {
        if (set)
            ramShadow[label.ledID][label.addr] |= (1 << label.bit);
        else
            ramShadow[label.ledID][label.addr] &= ~(1 << label.bit);
    }
}

void IFEIDisplay::addAsciiString7SegToShadow(const char* str, const SegmentMap* map, int numDigits) {
    int len = strlen(str);
    for (int d = 0; d < numDigits; d++) {
        int charIdx = d - (numDigits - len);
        uint8_t segBits = 0;
        if (charIdx >= 0) {
            char c = str[charIdx];
            segBits = seg7_ascii[(uint8_t)c];
        }
        for (int s = 0; s < 7; s++) {
            const SegmentMap& seg = map[d * 7 + s];
            if (seg.addr < 64 && seg.bit < 4) {
                if (segBits & (1 << s))
                    ramShadow[seg.ledID][seg.addr] |= (1 << seg.bit);
                else
                    ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
            }
        }
    }
}

// Clear methods
void IFEIDisplay::clearBingoFromShadow(const SegmentMap map[5][7]) {
    for (int d = 0; d < 5; d++) {
        for (int s = 0; s < 7; s++) {
            const SegmentMap& seg = map[d][s];
            if (seg.addr < 64 && seg.bit < 4)
                ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
        }
    }
}

void IFEIDisplay::clearFuelFromShadow(const SegmentMap map[6][14]) {
    for (int d = 0; d < 6; d++) {
        for (int s = 0; s < 14; s++) {
            const SegmentMap& seg = map[d][s];
            if (seg.addr < 64 && seg.bit < 4)
                ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
        }
    }
}

void IFEIDisplay::clearLabelFromShadow(const SegmentMap* segMap) {
    const SegmentMap& seg = *segMap;
    if (seg.addr < 64 && seg.bit < 4)
        ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
}

void IFEIDisplay::clearBarFromShadow(const SegmentMap* barMap, int numBars) {
    for (int i = 0; i < numBars; ++i) {
        const SegmentMap& seg = barMap[i];
        if (seg.addr < 64 && seg.bit < 4)
            ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
    }
}

void IFEIDisplay::clear7SegFromShadow(const SegmentMap* map, int numDigits) {
    for (int d = 0; d < numDigits; d++) {
        for (int s = 0; s < 7; s++) {
            const SegmentMap& seg = map[d * 7 + s];
            if (seg.addr < 64 && seg.bit < 4)
                ramShadow[seg.ledID][seg.addr] &= ~(1 << seg.bit);
        }
    }
}