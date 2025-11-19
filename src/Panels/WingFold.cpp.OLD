// WingFold.cpp — Wing-fold decoder override using firmware PCA API + InputMappings
// Set up InputMappings for WingFold commands with source set to NONE so the firmware doesn't auto-handle them.

#include "../Globals.h"
#include "../HIDManager.h"
#include "includes/WingFold.h"

#if defined(HAS_CUSTOM_RIGHT)
REGISTER_PANEL(WingFold, WingFold_init, WingFold_loop, nullptr, nullptr, nullptr, 100);
#endif

// Labels
static constexpr const char* L_PULL_POS0   = "WING_FOLD_PULL_POS0";        // PUSH
static constexpr const char* L_PULL_POS1   = "WING_FOLD_PULL_POS1";        // PULL
static constexpr const char* L_CUSTOM_P0   = "WING_FOLD_CUSTOM_PULL_POS0"; // second PUSH line
static constexpr const char* L_ROT_UNFOLD  = "WING_FOLD_ROTATE_UNFOLD";    // SPREAD
static constexpr const char* L_ROT_HOLD    = "WING_FOLD_ROTATE_HOLD";      // HOLD
static constexpr const char* L_ROT_FOLD    = "WING_FOLD_ROTATE_FOLD";      // FOLD

// Resolved wiring (fallback defaults preserved)
static uint8_t g_addr = 0x26;
static uint8_t g_port = 0;
static int8_t  g_bit_push1  = -1;
static int8_t  g_bit_push2  = -1;
static int8_t  g_bit_fold   = -1;
static int8_t  g_bit_spread = -1;

// Timing
static constexpr uint32_t SCAN_MS        = 4;
static constexpr uint32_t DEB_MS         = 8;
static constexpr uint32_t BLANK_MS = 250; // time with no angle asserted to allow HOLD
static constexpr uint32_t AX_SUPPRESS_MS = 120;

// State
enum Axial { PULL=0, PUSH=1 };
enum Angle { FOLD_POS=0, HOLD_POS=1, SPREAD_POS=2 };
struct WFState { Axial ax=PULL; Angle ang=HOLD_POS; };

static WFState  prevS{}, curS{};
static uint8_t  deb_byte = 0xFF;
static uint32_t deb_t0 = 0;
static uint32_t lastAngleAssert = 0;
static uint32_t axSuppressUntil = 0;
static bool     lastChangeWasAxial = false;
static uint32_t lastScan = 0;

// Utils
static inline uint32_t el(uint32_t now, uint32_t then){ return (uint32_t)(now - then); }
static inline bool before(uint32_t now, uint32_t deadline){ return (int32_t)(now - deadline) < 0; }

// Resolve WingFold wiring strictly from InputMappings.
static void resolveWingFoldFromMappings() {
    g_addr = 0x26;   // keep last valid PCA address (optional)
    g_port = 0;
    g_bit_push1 = -1;
    g_bit_push2 = -1;
    g_bit_fold = -1;
    g_bit_spread = -1;

    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label) continue;

        if (strcmp(m.label, L_PULL_POS0) == 0 && m.bit >= 0) g_bit_push1 = m.bit;
        else if (strcmp(m.label, L_CUSTOM_P0) == 0 && m.bit >= 0) g_bit_push2 = m.bit;
        else if (strcmp(m.label, L_ROT_FOLD) == 0 && m.bit >= 0) g_bit_fold = m.bit;
        else if (strcmp(m.label, L_ROT_UNFOLD) == 0 && m.bit >= 0) g_bit_spread = m.bit;
    }

    // Sanity print for debugging
    debugPrintf("WingFold bits resolved: P1=%d P2=%d F=%d S=%d (addr=0x%02X port=%u)\n",
        g_bit_push1, g_bit_push2, g_bit_fold, g_bit_spread, g_addr, g_port);
}

// replace bitLow(...) calls with this safe version
static inline bool bitLowSafe(uint8_t v, int8_t b) {
    return (b >= 0 && b < 8) ? ((v & (1u << b)) == 0) : false;
}

static inline Axial decodeAx(uint8_t byte) {
    return (bitLowSafe(byte, g_bit_push1) || bitLowSafe(byte, g_bit_push2)) ? PUSH : PULL;
}

/*
// Emit only the selected entry for each selector, no deferral
static void emitState(){
  HIDManager_setNamedButton((curS.ax==PULL)? L_PULL_POS1 : L_PULL_POS0, false, true);
  const char* angLab = (curS.ang==FOLD_POS)? L_ROT_FOLD : (curS.ang==HOLD_POS)? L_ROT_HOLD : L_ROT_UNFOLD;
  HIDManager_setNamedButton(angLab, false, true);
}
*/

static void emitState(bool forceSend) {
    HIDManager_setNamedButton(
        (curS.ax == PULL) ? L_PULL_POS1 : L_PULL_POS0,
        forceSend,
        true
    );

    const char* angLab = (curS.ang == FOLD_POS) ? L_ROT_FOLD :
        (curS.ang == HOLD_POS) ? L_ROT_HOLD :
        L_ROT_UNFOLD;

    HIDManager_setNamedButton(angLab, forceSend, true);
}

/*
// ---- Lifecycle --------------------------------------------------------------
void WingFold_init() {
    resolveWingFoldFromMappings();

    uint8_t p0 = 0xFF, p1 = 0xFF;
    (void)readPCA9555(g_addr, p0, p1);
    const uint8_t snap = (g_port == 0) ? p0 : p1;

    deb_byte = snap;
    deb_t0 = millis();

    const uint32_t now = millis();              // <-- add this

    const bool fRaw = bitLowSafe(snap, g_bit_fold);
    const bool sRaw = bitLowSafe(snap, g_bit_spread);

    // Decode actual mechanical state
    curS.ax = decodeAx(snap);
    curS.ang = fRaw && !sRaw ? FOLD_POS :
        sRaw && !fRaw ? SPREAD_POS :
        HOLD_POS;

    // Keep prevS in lockstep so the first loop iteration sees "no change"
    prevS = curS;

    if (fRaw || sRaw) lastAngleAssert = now;
    axSuppressUntil = 0;
    lastChangeWasAxial = false;
    lastScan = 0;

    debugPrintf("WingFold_init: addr=0x%02X port=%u bits p1=%d p2=%d F=%d S=%d\n",
        g_addr, g_port, g_bit_push1, g_bit_push2, g_bit_fold, g_bit_spread);

    emitState();   // now emits the *real* lever position at mission start

}
*/

void WingFold_init() {
    resolveWingFoldFromMappings();

    uint8_t p0 = 0xFF, p1 = 0xFF;
    if (!readPCA9555(g_addr, p0, p1)) {
        debugPrintf("WingFold_init: PCA 0x%02X read FAILED\n", g_addr);
        return;
    }
    const uint8_t snap = (g_port == 0) ? p0 : p1;

    deb_byte = snap;
    deb_t0 = millis();
    const uint32_t now = millis();

    const bool fRaw = bitLowSafe(snap, g_bit_fold);
    const bool sRaw = bitLowSafe(snap, g_bit_spread);

    // Decode snapshot
    Axial ax = decodeAx(snap);
    Angle ang;
    if (fRaw && !sRaw)      ang = FOLD_POS;
    else if (sRaw && !fRaw) ang = SPREAD_POS;
    else                    ang = HOLD_POS;

    // Mechanical invariant: FOLD + PUSH is illegal
    if (fRaw && ax == PUSH) ax = PULL;

    curS.ax = ax;
    curS.ang = ang;
    prevS = curS;               // first loop sees "no change"

    if (fRaw || sRaw) lastAngleAssert = now;
    axSuppressUntil = 0;
    lastChangeWasAxial = false;
    lastScan = 0;

    debugPrintf("WingFold_init: addr=0x%02X port=%u bits p1=%d p2=%d F=%d S=%d\n",
        g_addr, g_port, g_bit_push1, g_bit_push2, g_bit_fold, g_bit_spread);

    // INIT-time authoritative event
    emitState(true);
}

void WingFold_loop(){
  const uint32_t now = millis();
  if (el(now,lastScan) < SCAN_MS) return;
  lastScan = now;

  uint8_t raw0=0xFF, raw1=0xFF;
  if (!readPCA9555(g_addr, raw0, raw1)) return;

  const uint8_t raw = (g_port==0)? raw0 : raw1;

  // Debounce selected port byte
  if (raw != deb_byte){ deb_byte = raw; deb_t0 = now; return; }
  if (el(now,deb_t0) < DEB_MS) return;

  // --- read raw contacts from resolved bits
  const bool fRaw = bitLowSafe(deb_byte, g_bit_fold);    // physical FOLD contact
  const bool sRaw = bitLowSafe(deb_byte, g_bit_spread);  // physical SPREAD contact
  Axial ax = decodeAx(deb_byte);

  // axial edge bookkeeping
  if (ax != prevS.ax) {
      axSuppressUntil = now + AX_SUPPRESS_MS;
      lastChangeWasAxial = true;
  }

  // track last explicit angle assertion
  if (fRaw || sRaw) lastAngleAssert = now;

  // Angle decode with guards
  Angle ang = prevS.ang;
  if (fRaw && sRaw) {
      // fault: keep previous
  }
  else if (fRaw) {
      ang = FOLD_POS;   lastChangeWasAxial = false;
  }
  else if (sRaw) {
      ang = SPREAD_POS; lastChangeWasAxial = false;
  }
  else {
      const bool inAx = before(now, axSuppressUntil);
      if (ax == PUSH || inAx) {
          ang = prevS.ang; // block HOLD while locked or during axial transition
      }
      else {
          const bool longGap = el(now, lastAngleAssert) >= BLANK_MS;
          if (longGap && !lastChangeWasAxial) ang = HOLD_POS;
      }
  }

  // Mechanical invariant must rely on RAW FOLD contact, not decoded angle
  if (fRaw && ax == PUSH) ax = PULL;

  curS.ax = ax;
  curS.ang = ang;

  if (curS.ax != prevS.ax || curS.ang != prevS.ang) {
      emitState(false);
      prevS = curS;
  }
}
