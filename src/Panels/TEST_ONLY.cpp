// TEST_ONLY.cpp — tiny, readable template panel
// Uses ONLY: HIDManager_setNamedButton / HIDManager_toggleIfPressed
// Auto-detects GPIO, HC165, PCA9555 from InputMappings.
// Fill the 3 HC165 pin defines and the bit count for your panel.

// ---------- Includes ----------
#include "../Globals.h"
#include "../HIDManager.h"
#include "../DCSBIOSBridge.h"   // for timing helpers like shouldPollMs()

// ---------- HC165 wiring (edit these 4 lines per panel) ----------
#define HC165_BITS 32          // total bits daisy-chained (e.g. 8, 16, 24, 32, ...)

// ---------- Minimal helpers ----------
static inline bool startsWith(const char* s, const char* pfx) {
  return s && pfx && strncmp(s, pfx, strlen(pfx)) == 0;
}
static uint8_t hexNib(char c){ return (c>='0'&&c<='9')?c-'0':(c>='a'&&c<='f')?10+c-'a':(c>='A'&&c<='F')?10+c-'A':0; }
static uint8_t parseHexByte(const char* s){ // expects "0xNN"
  if (!s || strlen(s)<3) return 0;
  return (hexNib(s[2])<<4) | hexNib(s[3]);
}

// ---------- PCA9555 tracking ----------
struct PcaState { uint8_t addr; uint8_t p0=0xFF, p1=0xFF; };
static PcaState pcas[8]; static size_t numPcas = 0;

// Build unique PCA list from InputMappings (sources like "PCA_0x22")
static void buildPcaList() {
  bool seen[256] = {false};
  for (size_t i=0;i<InputMappingSize;++i){
    const auto& m = InputMappings[i];
    if (!m.source) continue;
    if (startsWith(m.source, "PCA_0x")) {
      uint8_t a = parseHexByte(m.source+4);
      if (!seen[a] && numPcas < 8) { seen[a]=true; pcas[numPcas++].addr = a; }
    }
  }
}

// ---------- HC165 state ----------
static uint64_t hc165Bits      = ~0ULL;
static uint64_t hc165PrevBits  = ~0ULL;

// ---------- GPIO cache per-selector-group ----------
#define MAX_GROUPS_LOCAL 64
static uint16_t gpioSelCache[MAX_GROUPS_LOCAL];

// ---------- INIT ----------
void TEST_ONLY_init() {
  // 1) GPIO inputs → INPUT_PULLUP for any mapping with source="GPIO" and port>=0
  for (size_t i=0;i<InputMappingSize;++i) {
    const auto& m = InputMappings[i];
    if (m.source && strcmp(m.source,"GPIO")==0 && m.port >= 0) pinMode(m.port, INPUT_PULLUP);
  }

  // 2) HC165 (optional)
  if (HC165_BITS > 0) {
    HC165_init(HC165_PL, HC165_CP, HC165_QH, HC165_BITS);
    hc165Bits = hc165PrevBits = HC165_read();
  }

  // 3) PCA9555 list + first snapshot
  buildPcaList();
  for (size_t i=0;i<numPcas;++i) {
    uint8_t p0,p1; if (readPCA9555(pcas[i].addr, p0, p1)) { pcas[i].p0=p0; pcas[i].p1=p1; }
  }

  // 4) Clear selector caches
  for (auto& v: gpioSelCache) v = 0xFFFF;

  // 5) One-time initial dispatch from current hardware state
  //    (a) HC165 (momentaries + selectors; fallbacks: bit==-1)
  if (HC165_BITS > 0) {
    bool groupActive[MAX_GROUPS_LOCAL] = {false};
    for (size_t i=0;i<InputMappingSize;++i){
      const auto& m = InputMappings[i];
      if (!m.label || !m.source || strcmp(m.source,"HC165")!=0) continue;
      if (m.bit >= 0 && m.bit < 64) {
        bool pressed = ((hc165Bits >> m.bit) & 1) == 0; // active LOW convention
        if (strcmp(m.controlType,"momentary")==0) {
          HIDManager_setNamedButton(m.label, true, pressed);
        } else if (m.group>0 && pressed) {
          groupActive[m.group] = true;
          HIDManager_setNamedButton(m.label, true, true);
        }
      }
    }
    // fallbacks (bit == -1)
    for (size_t i=0;i<InputMappingSize;++i){
      const auto& m = InputMappings[i];
      if (!m.label || !m.source || strcmp(m.source,"HC165")!=0) continue;
      if (m.bit == -1 && m.group > 0 && !groupActive[m.group]) {
        HIDManager_setNamedButton(m.label, true, true);
      }
    }
  }

  //    (b) GPIO selectors (LOW = active). Fallbacks use port==-1
  auto pollGpio = [&](bool forceSend){
    bool groupActive[MAX_GROUPS_LOCAL] = {false};
    // real pins
    for (size_t i = 0; i < InputMappingSize; ++i) {
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || strcmp(m.source, "GPIO") != 0 || m.port < 0 || m.group == 0) continue;

        int pinState = digitalRead(m.port);
        bool isActive = (m.bit == 0) ? (pinState == LOW) : (pinState == HIGH);

        if (isActive) {
            groupActive[m.group] = true;
            if (forceSend || gpioSelCache[m.group] != m.oride_value) {
                gpioSelCache[m.group] = m.oride_value;
                HIDManager_setNamedButton(m.label, true, true);
            }
        }
    }

    // fallbacks
    for (size_t i=0;i<InputMappingSize;++i){
      const auto& m = InputMappings[i];
      if (!m.label || !m.source || strcmp(m.source,"GPIO")!=0 || m.port!=-1 || m.group==0) continue;
      if (!groupActive[m.group]) {
        if (forceSend || gpioSelCache[m.group]!=m.oride_value) {
          gpioSelCache[m.group]=m.oride_value;
          HIDManager_setNamedButton(m.label, true, true);
        }
      }
    }
  };
  pollGpio(true);

  //    (c) PCA9555 selectors/momentaries from the snapshot
  for (size_t i=0;i<InputMappingSize;++i){
    const auto& m = InputMappings[i];
    if (!m.label || !m.source || !startsWith(m.source,"PCA_0x")) continue;
    uint8_t a = parseHexByte(m.source+4);
    // locate state
    const PcaState* ps=nullptr; for(size_t k=0;k<numPcas;++k) if (pcas[k].addr==a) { ps=&pcas[k]; break; }
    if(!ps) continue;
    uint8_t byte = (m.port==0)? ps->p0 : ps->p1;
    bool pressed = ((byte >> m.bit) & 1) == 0; // active LOW
    if (strcmp(m.controlType,"momentary")==0) {
      HIDManager_setNamedButton(m.label, true, pressed);
    } else if (m.group>0 && pressed) {
      HIDManager_setNamedButton(m.label, true, true);
    }
  }

  debugPrintln("✅ TEST_ONLY panel initialized");
}

// ---------- LOOP ----------
void TEST_ONLY_loop() {
  static unsigned long lastPoll=0;
  if (!shouldPollMs(lastPoll)) return;

  // 1) HC165 edge handling
  if (HC165_BITS > 0) {
    uint64_t bits = HC165_read();
    if (bits != hc165PrevBits) {
      // momentaries + selectors in one pass
      bool groupActive[MAX_GROUPS_LOCAL] = {false};
      for (size_t i=0;i<InputMappingSize;++i){
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || strcmp(m.source,"HC165")!=0 || m.bit < 0 || m.bit >= 64) continue;
        bool nowPressed  = ((bits        >> m.bit) & 1) == 0;
        bool prevPressed = ((hc165PrevBits>> m.bit) & 1) == 0;

        if (strcmp(m.controlType,"momentary")==0) {
          if (nowPressed != prevPressed) HIDManager_setNamedButton(m.label, false, nowPressed);
        } else if (m.group>0 && nowPressed) {
          groupActive[m.group]=true;
          HIDManager_setNamedButton(m.label, false, true);
        }
      }
      // HC165 fallbacks (bit == -1)
      for (size_t i=0;i<InputMappingSize;++i){
        const auto& m = InputMappings[i];
        if (!m.label || !m.source || strcmp(m.source,"HC165")!=0 || m.bit!=-1 || m.group==0) continue;
        if (!groupActive[m.group]) HIDManager_setNamedButton(m.label, false, true);
      }
      hc165PrevBits = bits;
    }
  }

  // 2) GPIO selectors (LOW = active) + fallbacks
  {
    bool groupActive[MAX_GROUPS_LOCAL] = {false};
    for (size_t i=0;i<InputMappingSize;++i){
      const auto& m = InputMappings[i];
      if (!m.label || !m.source || strcmp(m.source,"GPIO")!=0 || m.port < 0 || m.group==0) continue;

      int pinState = digitalRead(m.port); // HIGH or LOW
      bool isActive = (m.bit == 0) ? (pinState == LOW) : (pinState == HIGH);

      if (isActive) {
        groupActive[m.group]=true;
        if (gpioSelCache[m.group]!=m.oride_value) {
          gpioSelCache[m.group]=m.oride_value;
          HIDManager_setNamedButton(m.label, false, true);
        }
      }
    }

    for (size_t i=0;i<InputMappingSize;++i){
      const auto& m = InputMappings[i];
      if (!m.label || !m.source || strcmp(m.source,"GPIO")!=0 || m.port!=-1 || m.group==0) continue;
      if (!groupActive[m.group] && gpioSelCache[m.group]!=m.oride_value) {
        gpioSelCache[m.group]=m.oride_value;
        HIDManager_setNamedButton(m.label, false, true);
      }
    }
  }

  // 3) PCA9555: read each address once, then walk mappings
  for (size_t i=0;i<numPcas;++i) {
    uint8_t p0,p1; if (!readPCA9555(pcas[i].addr, p0, p1)) continue;
    if (p0==pcas[i].p0 && p1==pcas[i].p1) continue; // no change

    for (size_t j=0;j<InputMappingSize;++j){
      const auto& m = InputMappings[j];
      if (!m.label || !m.source || !startsWith(m.source,"PCA_0x")) continue;
      if (parseHexByte(m.source+4) != pcas[i].addr) continue;

      uint8_t oldB = (m.port==0)? pcas[i].p0 : pcas[i].p1;
      uint8_t newB = (m.port==0)? p0        : p1;
      bool prev = ((oldB >> m.bit) & 1) == 0;
      bool now  = ((newB >> m.bit) & 1) == 0;

      if (strcmp(m.controlType,"momentary")==0) {
        if (prev != now) HIDManager_setNamedButton(m.label, false, now);
      } else if (m.group>0 && now && !prev) {
        HIDManager_setNamedButton(m.label, false, true);
      }
    }
    pcas[i].p0 = p0; pcas[i].p1 = p1;
  }
}