// BLEManager.cpp  — NimBLE-only implementation with on-device bond wipe (≥10s hold).
// Requires: NimBLE-Arduino >= 2.3.6 and ESP32 Arduino Core >= 3.3.2
#include "../Globals.h"

#if USE_DCSBIOS_BLUETOOTH

// ===== USER CONFIGURATION =====
// Adjust these settings before compiling to optimize for your use case

#define BLE_AUTOSLEEP_MINUTES         0   // 0 = disabled
#define SLEEP_ON_BOOT_ENABLED         0   // Enters deep sleep on boot until WAKE_PIN pressed
#define BLE_DELETE_BONDS_ON_BOOT      0   // dev only (we can force bond wipe with long-press, a red blink cue will occur)
#define BLE_TX_POWER                  0   // dBm
#define WS2812MINI_NUM_LEDS           1

// Other Settings
#define HID_VERSION                   0x0509
#define MEASURE_LATENCY_TEST          0   // set 0 for production
#define SAMPLING_BATT_AVG             8
#define STATUS_BRIGHT                16
#define USE_PUBLIC_BLE_ADDR           0   // Should always be 0 for HID devices
#define BATTERY_KA_TEST               0   // set 1 to periodically notify battery level (keep-alive)

// BLE Connection Parameters (values are in ms, converted to BLE units internally)
#define BLE_MIN_INTERVAL_MS           7    // Minimum connection interval in ms (7 → 7.5ms in BLE units)
#define BLE_MAX_INTERVAL_MS          15    // Maximum connection interval in ms (15 → 15ms in BLE units)
#define BLE_SLAVE_LATENCY             0    // Number of connection events peripheral can skip when idle
                                           // 0 = fastest response, highest power (competitive gaming)
                                           // 2 = ~60% idle power savings, imperceptible lag (recommended)
                                           // 4 = ~80% idle power savings, still responsive (battery saver)
#define BLE_SUPERVISION_TIMEOUT_MS 4000    // Connection timeout in ms (must be > (1+latency)*max_interval*2)

// Advanced BLE settings (usually don't need to change)
#define BLE_DATALENGTH              251    // Max data length (27, 51, 123, 251)
#define BLE_MTU                     517    // ATT MTU size (23..517), must be >= HID report size (64B)

// Compile-time safety checks (NASA-style)
static_assert(BLE_SLAVE_LATENCY >= 0 && BLE_SLAVE_LATENCY <= 4,
              "BLE_SLAVE_LATENCY out of tested range (0-4). Higher values may cause host compatibility issues.");
static_assert(BLE_MIN_INTERVAL_MS <= BLE_MAX_INTERVAL_MS,
              "BLE_MIN_INTERVAL_MS must be <= BLE_MAX_INTERVAL_MS");
static_assert(BLE_MIN_INTERVAL_MS >= 7 && BLE_MAX_INTERVAL_MS <= 4000,
              "BLE intervals out of spec (7ms min, 4000ms max)");

// For S3, C3 and H2 PINs are hardware specific. Use -1 when unsure or dont need that pin

#if ESP_FAMILY_C3
#define LAT_TEST_PIN         9
#define WS2812MINI_PIN       7
#define PIN_VBAT             3 
static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_2;
#elif ESP_FAMILY_S3
#define LAT_TEST_PIN        -1 // was 9
#define WS2812MINI_PIN      -1 // was 47
#define PIN_VBAT            -1 // was 3
static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_NC; // was GPIO_NUM_10;
#elif ESP_FAMILY_H2 // This is for my Custom H2 board
#define LAT_TEST_PIN         9
#define WS2812MINI_PIN       24
#define PIN_VBAT             4
#define LED_ON_PIN           1   // enables VCC rail for WS2812
#define BAT_SENS_ON_PIN      3   // enables battery divider power
static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_10; // 8, 9, 10, 11, 12, 13 & 14 are RTC GPIOs
#elif ESP_FAMILY_C6
#define LAT_TEST_PIN         -1
#define WS2812MINI_PIN        8  // Built-in RGB on most C6 dev boards
#define PIN_VBAT             -1
static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_9;   // BOOT button on most C6 boards
#elif ESP_FAMILY_C2
#define LAT_TEST_PIN         -1
#define WS2812MINI_PIN       -1
#define PIN_VBAT             -1
static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_NC;
#elif ESP_FAMILY_C5
#define LAT_TEST_PIN         -1
#define WS2812MINI_PIN       -1
#define PIN_VBAT             -1
static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_NC;
#elif ESP_FAMILY_CLASSIC
// Lolin D32 / ESP32 Classic pin assignments
#define LAT_TEST_PIN         -1  // not used
#define WS2812MINI_PIN       -1  // no WS2812 on this board (uses regular LED on GPIO5)
#define PIN_VBAT             35  // Battery divider on ADC1_CH7 (safe with BLE)
static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_NC;  // BOOT button for deep sleep wake
#else
// Unknown platform - disable all optional features
#define LAT_TEST_PIN         -1
#define WS2812MINI_PIN       -1 // Careful here as our WS2812 library does not check for -1. May crash or cause reboots, set to a valid GPIO if unsure.
#define PIN_VBAT             -1
static constexpr gpio_num_t WAKE_PIN = GPIO_NUM_NC;
#endif

// Long-press thresholds
static constexpr uint32_t SLEEP_HOLD_MS = 4000;  // ≥4s → deep sleep
static constexpr uint32_t BOND_WIPE_HOLD_MS = 10000;  // ≥10s → delete bonds + reboot

// Button debounce
static constexpr uint32_t DEBOUNCE_MS = 20;  // 20ms debounce for mechanical buttons

#include "esp_sleep.h"
#include "driver/gpio.h"
#if ESP_FAMILY_S3 || ESP_FAMILY_CLASSIC
  #include "driver/rtc_io.h"
#endif

// ---- GPIO deep-sleep wake API shim (IDF 5.x variants) ----
#if !defined(GPIO_DS_WAKE_LOW)
  #if defined(ESP_SLEEP_GPIO_WAKEUP_LOW_LEVEL)
    #define GPIO_DS_WAKE_LOW  ESP_SLEEP_GPIO_WAKEUP_LOW_LEVEL
    #define GPIO_DS_WAKE_HIGH ESP_SLEEP_GPIO_WAKEUP_HIGH_LEVEL
  #elif defined(ESP_GPIO_WAKEUP_GPIO_LOW_LEVEL)
    #define GPIO_DS_WAKE_LOW  ESP_GPIO_WAKEUP_GPIO_LOW_LEVEL
    #define GPIO_DS_WAKE_HIGH ESP_GPIO_WAKEUP_GPIO_HIGH_LEVEL
  #elif defined(ESP_GPIO_WAKEUP_GPIO_LOW)
    #define GPIO_DS_WAKE_LOW  ESP_GPIO_WAKEUP_GPIO_LOW
    #define GPIO_DS_WAKE_HIGH ESP_GPIO_WAKEUP_GPIO_HIGH
  #endif
#endif

// ---- EXT1 wake mode shim (IDF enum variants) ----
#if !defined(EXT1_WAKE_LOW_MODE)
  #if defined(ESP_EXT1_WAKEUP_ALL_LOW)
    #define EXT1_WAKE_LOW_MODE ESP_EXT1_WAKEUP_ALL_LOW
  #elif defined(ESP_EXT1_WAKEUP_ANY_LOW)
    #define EXT1_WAKE_LOW_MODE ESP_EXT1_WAKEUP_ANY_LOW
  #endif
#endif

#include "../DCSBIOSBridge.h"
#include "../BLEManager.h"
#include "../CustomDescriptors/BidireccionalNew.h"

#if !__has_include(<NimBLEDevice.h>)
  #error "Missing NimBLEDevice.h — install NimBLE-Arduino: https://github.com/h2zero/NimBLE-Arduino"
#endif
#if !__has_include(<NimBLEHIDDevice.h>)
  #error "Missing NimBLEHIDDevice.h — install NimBLE-Arduino: https://github.com/h2zero/NimBLE-Arduino"
#endif
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

// HID report state (64B IN/OUT/FEATURE)
extern GamepadReport_t report;

// ======================== Statics ========================

// ===== Low power button handling (WAKE_PIN) =====
static uint8_t  lp_prev = 1;
static uint32_t lp_edge_ms = 0;
static uint32_t lp_t0_ms = 0;

// Blink until connected; solid after connect
static bool     g_blink = true;
static uint32_t g_blinkMs = 0;
static bool     g_ledOn = false;

// Tiered advertising state
static uint32_t g_advStartMs = 0;
static bool     g_advSlowed = false;

// Used for idle timeout to sleep
static uint32_t lastUserInputMs = 0;
static uint8_t  last_report_snapshot[64] = { 0 };

// ===== VBAT sense (MATCHES ACTUAL HARDWARE: R204=330kΩ, R205=1MΩ) =====
static constexpr uint32_t RTOP_OHMS = 330000UL;   // R204: 330kΩ (top, to VBAT)
static constexpr uint32_t RBOT_OHMS = 1000000UL;  // R205: 1MΩ (bottom, to GND via Q3)

// 2× AA Alkaline: 3.2V fresh → 1.8V depleted (0.9V/cell cutoff)
static constexpr uint32_t VBAT_MIN_mV = 1800;  // 0% - time to replace batteries
static constexpr uint32_t VBAT_MAX_mV = 3200;  // 100% - fresh cells

static uint32_t g_vbatFilt_mV = 0;

static inline void battSenseEnable(bool on);
static inline void ledPowerEnable(bool on);

static inline void batteryInit() {
    if (PIN_VBAT < 0) return;  // No battery monitoring on this board
    battSenseEnable(false);
    pinMode(PIN_VBAT, INPUT);
    analogSetPinAttenuation(PIN_VBAT, ADC_11db);
}

static inline uint32_t adcVBAT_mV() {
    if (PIN_VBAT < 0) return 0;  // Return 0 = "USB powered" (100% battery)

    battSenseEnable(true);
    delayMicroseconds(200);

    (void)analogReadMilliVolts(PIN_VBAT);  // throwaway for ADC settle

    uint32_t sum = 0;
    for (int i = 0; i < SAMPLING_BATT_AVG; ++i) {
        sum += analogReadMilliVolts(PIN_VBAT);
    }

    battSenseEnable(false);
    return sum / SAMPLING_BATT_AVG;
}

// FIX: Integer-only battery voltage calculation (no floats)
static inline uint32_t readVBAT_mV() {
    const uint32_t adc_mv = adcVBAT_mV();
    // Integer math with rounding: (adc * (R_top + R_bot) + R_bot/2) / R_bot
    const uint32_t vbat_mv = (adc_mv * (RTOP_OHMS + RBOT_OHMS) + RBOT_OHMS / 2) / RBOT_OHMS;
    debugPrintf("[VBAT] adc=%u vbat=%u\n", adc_mv, vbat_mv);
    return vbat_mv;
}

static inline uint8_t mvToPct(uint32_t mv) {
    // USB detection: no battery reads < 1400mV (dead 2×AA still reads ~1600mV)
    if (mv < 1500) return 100;  // USB powered = "full"

    if (mv <= VBAT_MIN_mV) return 0;
    if (mv >= VBAT_MAX_mV) return 100;
    return (uint8_t)((mv - VBAT_MIN_mV) * 100UL / (VBAT_MAX_mV - VBAT_MIN_mV));
}

static uint32_t lastBatteryUpdateMs = 0;
static uint8_t  g_battPct = 0;

#if MEASURE_LATENCY_TEST
// 64-byte report buffer aligned for ESP32 DMA
static uint8_t g_bleReport64[64] __attribute__((aligned(4))) = { 0 };
#endif

// Feature report TX buffer
static uint8_t feat_tx_buf[64] __attribute__((aligned(4))) = { 0 };

// Coherent BLE snapshot published by HIDManager_send(...)
static uint8_t ble_last_published[64] = { 0 };

// Helpers
static inline uint16_t ms_to_itvl(uint16_t ms){ return (uint16_t)((ms*4 + 2)/5); }
static inline uint16_t ms_to_supv(uint16_t ms){ return (uint16_t)((ms + 9)/10); }

// Feature report handshake state (protected by g_bleMux)
static volatile bool feat_has_tx = false;

static NimBLEServer*        gServer   = nullptr;
static NimBLEAdvertising*   gAdv      = nullptr;
static NimBLEHIDDevice*     gHid      = nullptr;
static NimBLECharacteristic* gHidInput = nullptr;
static NimBLECharacteristic* gOut      = nullptr;
static NimBLECharacteristic* gFeat     = nullptr;
static volatile bool        g_connected    = false;
static volatile bool        readyToNotify  = false;
static volatile bool        g_inputDirty   = false;  // v2.1: Set when notify() fails, cleared on successful retry

// Critical section for feat_tx_buf and feat_has_tx access
static portMUX_TYPE g_bleMux = portMUX_INITIALIZER_UNLOCKED;

#if MEASURE_LATENCY_TEST
static inline void clearRttPadding_unsafe(){ memset(&g_bleReport64[36], 0, 4); }
static portMUX_TYPE g_reportMux       = portMUX_INITIALIZER_UNLOCKED;
static constexpr uint32_t LAT_MAGIC   = 0xA5A55A5Au;
static volatile uint64_t  lat_t0_sent = 0;
#endif

// ======================== Callbacks ========================
struct BatCb : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic* c, NimBLEConnInfo&, uint16_t cccd) override {
    if (cccd & 0x0001u) gHid->setBatteryLevel(g_battPct, true);
    debugPrintln("[BLUETOOTH] Battery notify subscribed");
  }
};

class InputReportCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic*, NimBLEConnInfo& info, uint16_t cccd) override {
    readyToNotify = (cccd & 0x0001u) != 0;
    if (!readyToNotify) return;
    debugPrintf("[BLUETOOTH] Input report READY, cccd=0x%04X\n", cccd);
    
    // v2.2: Apply connection parameters from compile-time #defines
    // BLE_SLAVE_LATENCY controls battery/latency tradeoff (0=fastest, 2=balanced, 4=battery saver)
    if (auto* s = NimBLEDevice::getServer()) {
      const uint16_t h = info.getConnHandle();
      (void)s->updateConnParams(h, 
          ms_to_itvl(BLE_MIN_INTERVAL_MS), 
          ms_to_itvl(BLE_MAX_INTERVAL_MS), 
          BLE_SLAVE_LATENCY, 
          ms_to_supv(BLE_SUPERVISION_TIMEOUT_MS));
      debugPrintf("[BLUETOOTH] Conn params: interval=%u-%ums, latency=%u\n", 
          BLE_MIN_INTERVAL_MS, BLE_MAX_INTERVAL_MS, BLE_SLAVE_LATENCY);
    }
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* p, NimBLEConnInfo& info) override {
    g_connected = true;
    g_blink = false;
    if (WS2812MINI_PIN >= 0) {
        WS2812Mini::WS2812_setLEDColor(0, 0, 0, 64);
        WS2812Mini::WS2812_show();
    }
    const uint16_t h = info.getConnHandle();
    debugPrintf("[BLUETOOTH] Connected: interval=%.2f ms, latency=%u, timeout=%u ms\n",
      info.getConnInterval()*1.25f, info.getConnLatency(), info.getConnTimeout()*10u);
    
    // FIX: Removed duplicate connection param update (now only in onSubscribe after bonding)
    // PHY and MTU can be requested early (link-layer, not security-dependent)
    p->updatePhy(h, BLE_GAP_LE_PHY_2M, BLE_GAP_LE_PHY_2M, 0);
    p->setDataLen(h, BLE_DATALENGTH);

    if (gAdv) gAdv->stop();
  }
  
  void onDisconnect(NimBLEServer*, NimBLEConnInfo& info, int reason) override {
    g_connected = false;
    readyToNotify = false;
    g_inputDirty = false;  // v2.1: Clear dirty flag — no point retrying when disconnected
    
    // FIX: Protected access to feat_has_tx
    portENTER_CRITICAL(&g_bleMux);
    feat_has_tx = false;
    portEXIT_CRITICAL(&g_bleMux);
    
    g_blink = true;
    g_ledOn = false;
    
    if (WS2812MINI_PIN >= 0) {
        WS2812Mini::WS2812_clearAll();
    }
    
#if MEASURE_LATENCY_TEST
      portENTER_CRITICAL(&g_reportMux);
      lat_t0_sent = 0;
      portEXIT_CRITICAL(&g_reportMux);
    #endif
    debugPrintf("[BLUETOOTH] Disconnected rc=%d\n", reason);
    
    // FIX: Reset to fast advertising for quick reconnect
    g_advStartMs = millis();
    g_advSlowed = false;
    if (gAdv) {
        gAdv->stop();
        gAdv->setMinInterval(32);   // 20ms
        gAdv->setMaxInterval(48);   // 30ms
        gAdv->start();
    }
  }
  
  void onAuthenticationComplete(NimBLEConnInfo&) override { debugPrint("[BLUETOOTH] Pairing finished\n"); }
  
  void onConnParamsUpdate(NimBLEConnInfo& info) override {
    debugPrintf("[BLUETOOTH] Params: interval=%.2f ms, latency=%u, timeout=%u ms\n",
      info.getConnInterval()*1.25f, info.getConnLatency(), info.getConnTimeout()*10u);  
  }
  
  void onMTUChange(uint16_t mtu, NimBLEConnInfo& info) override {
      const uint16_t eff = gServer->getPeerMTU(info.getConnHandle());
      debugPrintf("[BLUETOOTH] MTU change: mtu=%u (peer=%u) — %s for 64B reports\n",
          mtu, eff, (eff >= 67) ? "SUFFICIENT" : "⚠️ TOO SMALL!");
  }
  
  void onPhyUpdate(NimBLEConnInfo&, uint8_t tx, uint8_t rx) override {
    debugPrintf("[BLUETOOTH] PHY update: tx=%u rx=%u\n", tx, rx);
  }
};

class OutputCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* chr, NimBLEConnInfo&) override {
    const std::string& v = chr->getValue();
    size_t n = v.size();

    if (n > 64) n = 64;
    #if MEASURE_LATENCY_TEST
    if (n >= 4){
      uint32_t magic=0;
      memcpy(&magic, v.data(), 4);
      if (magic == LAT_MAGIC){
        uint64_t t0;
        portENTER_CRITICAL(&g_reportMux);
        t0 = lat_t0_sent;
        portEXIT_CRITICAL(&g_reportMux);
        if (t0 != 0 && gHidInput && readyToNotify){
          const uint32_t rtt_us = (uint32_t)(esp_timer_get_time() - t0);
          uint8_t tx[64];
          portENTER_CRITICAL(&g_reportMux);
          memcpy(&g_bleReport64[36], &rtt_us, 4);
          memcpy(tx, g_bleReport64, 64);
          clearRttPadding_unsafe();
          lat_t0_sent = 0;
          portEXIT_CRITICAL(&g_reportMux);
          (void)gHidInput->notify(tx, 64);
          debugPrintf("[LATENCY] RTT %.3f ms\n", (double)rtt_us/1000.0);
        }
        return;
      }
    }
    #endif
    if (!mainLoopStarted || n == 0) return;
    dcsUdpRingbufPushChunked((const uint8_t*)v.data(), n);
  }
};

class FeatureCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
    const std::string& v = c->getValue();
    const uint8_t* data = (const uint8_t*)v.data();
    size_t len = v.size(); if (len > 64) len = 64;

    static const char kReq[] = "DCSBIOS-HANDSHAKE";
    static const char kRsp[] = "DCSBIOS-READY";
    if (len == sizeof(kReq)-1 && memcmp(data, kReq, sizeof(kReq)-1) == 0){
      // FIX: Protected access to feat_tx_buf and feat_has_tx
      portENTER_CRITICAL(&g_bleMux);
      memset(feat_tx_buf, 0, 64);
      memcpy(feat_tx_buf, kRsp, sizeof(kRsp)-1);
      feat_has_tx = true;
      portEXIT_CRITICAL(&g_bleMux);
      return;
    }
    if (mainLoopStarted && len){
      dcsRawUsbOutRingbufPushChunked(data, len);
      forceResync();
    }
  }
  
  // FIX: Complete rewrite to fix double-write bug and race condition
  void onRead(NimBLECharacteristic* c, NimBLEConnInfo&) override {
    bool hasTx = false;
    uint8_t txCopy[64];
    
    // Atomically check and claim the pending TX data
    portENTER_CRITICAL(&g_bleMux);
    if (feat_has_tx) {
        hasTx = true;
        memcpy(txCopy, feat_tx_buf, 64);
        feat_has_tx = false;
        memset(feat_tx_buf, 0, 64);  // Clear for next use
    }
    portEXIT_CRITICAL(&g_bleMux);
    
    if (hasTx) {
        c->setValue(txCopy, 64);
        return;
    }
    
    if (!mainLoopStarted) { 
        static uint8_t z[64] = {0}; 
        c->setValue(z, 64); 
        return; 
    }
    
    DcsRawUsbOutRingMsg msg;
    if (dcsRawUsbOutRingbufPop(&msg)) {
        uint8_t out[64] = {0};
        if (msg.len) memcpy(out, msg.data, msg.len);
        c->setValue(out, 64);
    } else {
        static uint8_t z[64] = {0};
        c->setValue(z, 64);
    }
  }
};

// ========== Bond wipe helper ==========
static void wipeBondsAndReboot(){
  debugPrintln("[BLUETOOTH] Factory reset: deleting all bonds");
  if (gAdv) gAdv->stop();
  // Visual feedback: brief red blink 6 times (bounded loop)
  if (WS2812MINI_PIN >= 0) {
      for (int i = 0; i < 6; i++) {
          WS2812Mini::WS2812_setLEDColor(0, 64, 0, 0); // Red
          WS2812Mini::WS2812_show();
          delay(120);
          WS2812Mini::WS2812_setLEDColor(0, 0, 0, 0);
          WS2812Mini::WS2812_show();
          delay(120);
      }
  }

  NimBLEDevice::deleteAllBonds();

  debugPrintln("[BLUETOOTH] Bonds deleted, restarting");
  delay(50);
  
  // FIX: Bounded whitelist clear (max 16 entries per BLE spec)
  for (int i = 0; i < 16 && NimBLEDevice::getWhiteListCount() > 0; ++i) {
      NimBLEDevice::whiteListRemove(NimBLEDevice::getWhiteListAddress(0));
  }
  
  esp_restart(); // does not return
}

// ======================== Internal helpers ========================

void BLEManager_dispatchReport(bool force) {
    const bool can = g_connected && readyToNotify && gHidInput;
    if (!can && !force) return;
    uint8_t tx[64];
    portENTER_CRITICAL(&g_bleMux);
    memcpy(tx, ble_last_published, 64);
    portEXIT_CRITICAL(&g_bleMux);
    if (can) {
        // v2.1 FIX: Check notify() return value to detect TX buffer congestion
        if (!gHidInput->notify(tx, 64)) {
            g_inputDirty = true;  // Mark for retry in BLE_loop
        }
    }
}

void BLEManager_send(const uint8_t* data, size_t len) {
    if (!g_connected || !readyToNotify || !gHidInput || !data || !len) return;
    uint8_t tx[64] = { 0 };
    const size_t n = len <= 64 ? len : 64;
    memcpy(tx, data, n);

    portENTER_CRITICAL(&g_bleMux);
    memcpy(ble_last_published, tx, 64);
    portEXIT_CRITICAL(&g_bleMux);

    // v2.1 FIX: Check notify() return value — if TX buffer full, mark dirty for retry
    // This prevents state desync when BLE radio is congested (e.g., Master Arm flip missed)
    if (!gHidInput->notify(tx, 64)) {
        g_inputDirty = true;
    }
}

#if ESP_FAMILY_H2
static inline void ledPowerEnable(bool on) {
    pinMode(LED_ON_PIN, OUTPUT);
    digitalWrite(LED_ON_PIN, on ? HIGH : LOW);
}

static inline void battSenseEnable(bool on) {
    pinMode(BAT_SENS_ON_PIN, OUTPUT);
    digitalWrite(BAT_SENS_ON_PIN, on ? HIGH : LOW);
}
#else
// No-op stubs for platforms without switchable power rails
static inline void ledPowerEnable(bool) {}
static inline void battSenseEnable(bool) {}
#endif

static inline void ws_off() {
    if (WS2812MINI_PIN >= 0) {
        WS2812Mini::WS2812_setLEDColor(0, 0, 0, 0);
        WS2812Mini::WS2812_show();
        WS2812Mini::WS2812_clearAll();
    }
}
static inline void ws_green(uint8_t v = 64) {
    // RGB order: (R,G,B,?) -> set green only
    if (WS2812MINI_PIN >= 0) {
        WS2812Mini::WS2812_setLEDColor(0, 0, v, 0);
        WS2812Mini::WS2812_show();
    }
}

static void ws_hold_pin_low_for_sleep() {
    if (WS2812MINI_PIN < 0) return;  // Guard for boards without WS2812
    pinMode(WS2812MINI_PIN, OUTPUT);
    digitalWrite(WS2812MINI_PIN, LOW);
    gpio_hold_en((gpio_num_t)WS2812MINI_PIN);
}

static void ws_release_hold_after_wake() {
    if (WS2812MINI_PIN < 0) return;  // Guard for boards without WS2812
    gpio_hold_dis((gpio_num_t)WS2812MINI_PIN);
    pinMode(WS2812MINI_PIN, OUTPUT);
    digitalWrite(WS2812MINI_PIN, LOW);
}

// ========== Per-target deep sleep helpers ==========
#if ESP_FAMILY_C3
static void arm_deepsleep_wait_gpio2() {
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << WAKE_PIN;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);
    gpio_sleep_set_direction(WAKE_PIN, GPIO_MODE_INPUT);
    gpio_sleep_set_pull_mode(WAKE_PIN, GPIO_PULLUP_ONLY);
    gpio_sleep_sel_en(WAKE_PIN);
#if defined(GPIO_DS_WAKE_LOW)
    (void)esp_deep_sleep_enable_gpio_wakeup(1ULL << WAKE_PIN, GPIO_DS_WAKE_LOW);
#else
    (void)esp_deep_sleep_enable_gpio_wakeup(1ULL << WAKE_PIN, (esp_deepsleep_gpio_wake_up_mode_t)0);
#endif
    esp_deep_sleep_start();
}
#elif ESP_FAMILY_H2
static void arm_deepsleep_wait_gpio2() {
    // Disable ALL other wake sources first
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);

    // Configure wake pin with internal pull-up
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << WAKE_PIN;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);

    // Configure for sleep state
    gpio_sleep_set_direction(WAKE_PIN, GPIO_MODE_INPUT);
    gpio_sleep_set_pull_mode(WAKE_PIN, GPIO_PULLUP_ONLY);
    gpio_sleep_sel_en(WAKE_PIN);

    // H2 uses EXT1 for deep sleep wake (LOW level trigger)
#if defined(EXT1_WAKE_LOW_MODE)
    esp_sleep_enable_ext1_wakeup(1ULL << WAKE_PIN, EXT1_WAKE_LOW_MODE);
#else
    esp_sleep_enable_ext1_wakeup(1ULL << WAKE_PIN, (esp_sleep_ext1_wakeup_mode_t)0);
#endif

    // Optional: Power down flash for minimum current
    esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);

    // Enter deep sleep - does not return
    esp_deep_sleep_start();
}

#elif ESP_FAMILY_C6
static void arm_deepsleep_wait_gpio2() {
    // ESP32-C6: RISC-V, no RTC GPIO subsystem — uses GPIO deep-sleep wake (like C3)
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << WAKE_PIN;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);
    gpio_sleep_set_direction(WAKE_PIN, GPIO_MODE_INPUT);
    gpio_sleep_set_pull_mode(WAKE_PIN, GPIO_PULLUP_ONLY);
    gpio_sleep_sel_en(WAKE_PIN);
#if defined(GPIO_DS_WAKE_LOW)
    (void)esp_deep_sleep_enable_gpio_wakeup(1ULL << WAKE_PIN, GPIO_DS_WAKE_LOW);
#else
    (void)esp_deep_sleep_enable_gpio_wakeup(1ULL << WAKE_PIN, (esp_deepsleep_gpio_wake_up_mode_t)0);
#endif
    esp_deep_sleep_start();
}

#elif ESP_FAMILY_CLASSIC
static void arm_deepsleep_wait_gpio2() {
    // ESP32 Classic: Uses EXT0 wake with RTC GPIO API
    // Note: Only RTC GPIOs support deep sleep wake (GPIO 0,2,4,12-15,25-27,32-39)

    rtc_gpio_init(WAKE_PIN);
    rtc_gpio_set_direction(WAKE_PIN, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_en(WAKE_PIN);
    rtc_gpio_pulldown_dis(WAKE_PIN);

    // EXT0 wake: single GPIO, level-triggered (0 = LOW to wake)
    esp_sleep_enable_ext0_wakeup(WAKE_PIN, 0);

    esp_deep_sleep_start();
}

#elif ESP_FAMILY_S3
// S3 has both RTC GPIO and gpio_sleep_* APIs
static void arm_deepsleep_wait_gpio2() {
    if (WAKE_PIN == GPIO_NUM_NC) { esp_deep_sleep_start(); return; }
    const unsigned wp = static_cast<unsigned>(WAKE_PIN);
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << (wp & 63);
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);

    rtc_gpio_init(WAKE_PIN);
    rtc_gpio_pullup_en(WAKE_PIN);
    rtc_gpio_pulldown_dis(WAKE_PIN);
    gpio_sleep_set_direction(WAKE_PIN, GPIO_MODE_INPUT);
    gpio_sleep_set_pull_mode(WAKE_PIN, GPIO_PULLUP_ONLY);
    gpio_sleep_sel_en(WAKE_PIN);

    esp_sleep_enable_ext0_wakeup(WAKE_PIN, 0);
    esp_deep_sleep_start();
}

#else
// Unknown platform — attempt GPIO deep-sleep wake (safest default, no RTC dependency)
static void arm_deepsleep_wait_gpio2() {
    gpio_config_t io = {};
    io.pin_bit_mask = 1ULL << WAKE_PIN;
    io.mode = GPIO_MODE_INPUT;
    io.pull_up_en = GPIO_PULLUP_ENABLE;
    io.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io);
    gpio_sleep_set_direction(WAKE_PIN, GPIO_MODE_INPUT);
    gpio_sleep_set_pull_mode(WAKE_PIN, GPIO_PULLUP_ONLY);
    gpio_sleep_sel_en(WAKE_PIN);
#if defined(GPIO_DS_WAKE_LOW)
    (void)esp_deep_sleep_enable_gpio_wakeup(1ULL << WAKE_PIN, GPIO_DS_WAKE_LOW);
#else
    (void)esp_deep_sleep_enable_gpio_wakeup(1ULL << WAKE_PIN, (esp_deepsleep_gpio_wake_up_mode_t)0);
#endif
    esp_deep_sleep_start();
}
#endif

// ========== GPIO isolation for minimum deep sleep current ==========
static void isolate_gpios_for_deepsleep() {
#if ESP_FAMILY_H2
    // 1. Ensure power switches are OFF and HELD
    ledPowerEnable(false);
    pinMode(LED_ON_PIN, OUTPUT);
    digitalWrite(LED_ON_PIN, LOW);
    gpio_hold_en((gpio_num_t)LED_ON_PIN);

    battSenseEnable(false);
    pinMode(BAT_SENS_ON_PIN, OUTPUT);
    digitalWrite(BAT_SENS_ON_PIN, LOW);
    gpio_hold_en((gpio_num_t)BAT_SENS_ON_PIN);

    // 2. WS2812 data line held LOW
    ws_hold_pin_low_for_sleep();

    // 3. ADC pin as input with no pull (minimize leakage)
    gpio_config_t adc_cfg = {};
    adc_cfg.pin_bit_mask = 1ULL << PIN_VBAT;
    adc_cfg.mode = GPIO_MODE_INPUT;
    adc_cfg.pull_up_en = GPIO_PULLUP_DISABLE;
    adc_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    adc_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&adc_cfg);
    gpio_hold_en((gpio_num_t)PIN_VBAT);

    // Note: H2 doesn't have gpio_deep_sleep_hold_en() — individual gpio_hold_en() calls
    // automatically persist through deep sleep on this platform.
#else
    // For non-H2 platforms: just hold the WS2812 pin
    ws_hold_pin_low_for_sleep();
#endif
}

static void arm_deepsleep_wait_gpio2_after_release() {
    // Visual cue: LED off
    ws_off();

    // Isolate ALL GPIOs for minimum leakage
    isolate_gpios_for_deepsleep();

    // v2.1 FIX: Wait for actual button release before sleeping
    // Previously used a 5s timeout which could cause immediate wake (boot loop) if button still held
    // Now we wait indefinitely with visual feedback until user releases
    uint32_t waitStart = millis();
    while (WAKE_PIN != GPIO_NUM_NC && digitalRead((int)WAKE_PIN) == LOW) {
        
        if (WS2812MINI_PIN >= 0) {
            // Blink dim blue every 500ms to indicate "release button to sleep"
            // This provides clear UX feedback that device is waiting for release
            if (((millis() - waitStart) / 500) & 1) {
                WS2812Mini::WS2812_setLEDColor(0, 0, 0, 32);  // dim blue
            }
            else {
                WS2812Mini::WS2812_setLEDColor(0, 0, 0, 0);   // off
            }
            WS2812Mini::WS2812_show();
        }

        delay(50);  // 50ms poll interval — responsive but not busy-spinning
    }
    
    delay(50);  // debounce after release confirmed
    ws_off();   // ensure LED off before sleep

    arm_deepsleep_wait_gpio2();
}

// ======================== Public API ========================
void BLE_setup(){

#if ESP_FAMILY_H2
    // Release any GPIO holds from previous deep sleep (H2-specific power rails)
    // Note: H2 doesn't have gpio_deep_sleep_hold_dis() — release individual pins instead
    gpio_hold_dis((gpio_num_t)LED_ON_PIN);
    gpio_hold_dis((gpio_num_t)BAT_SENS_ON_PIN);
    gpio_hold_dis((gpio_num_t)PIN_VBAT);
#endif

  ws_release_hold_after_wake();              // safe if not held

  // NEW: power the WS2812 rail before touching the data line
#if ESP_FAMILY_H2
  // Power the WS2812 rail before touching the data line (H2 has switchable LED power)
  ledPowerEnable(true);
#endif

  // Initialize WS2812 mini — ONLY if pin is valid!
  if (WS2812MINI_PIN >= 0) {
      WS2812Mini::WS2812_init(WS2812MINI_PIN, WS2812MINI_NUM_LEDS);
      WS2812Mini::WS2812_setBrightness(STATUS_BRIGHT);
  }
  else {
      debugPrintln("[BLE] WS2812 disabled (pin=-1)");
  }

  // Decide sleep before any other work
    bool willSleep = false;
#if SLEEP_ON_BOOT_ENABLED
    const auto cause = esp_sleep_get_wakeup_cause();
    #if ESP_FAMILY_C2 || ESP_FAMILY_C3 || ESP_FAMILY_C5 || ESP_FAMILY_C6 || ESP_FAMILY_H2
        willSleep = (cause != ESP_SLEEP_WAKEUP_GPIO && cause != ESP_SLEEP_WAKEUP_EXT1);
    #else
        willSleep = (cause != ESP_SLEEP_WAKEUP_EXT0 && cause != ESP_SLEEP_WAKEUP_EXT1);
    #endif
#endif

  if (willSleep) {
      ws_off();                           // LED off (consistent with other path)
      isolate_gpios_for_deepsleep();
      arm_deepsleep_wait_gpio2();         // Now safe to sleep
  }

  // not sleeping
  ws_green(64);

  debugPrintln("[BLUETOOTH] Starting BLE HID");
  debugPrintf("WS2812 RGB LED pin %d\n", WS2812MINI_PIN);
  #if defined(LED_BUILTIN)
    debugPrintf("BUILT-IN LED pin %d\n", LED_BUILTIN);
  #endif

  g_blink = true;
  g_blinkMs = millis();
  g_ledOn = true;

  if (WAKE_PIN != GPIO_NUM_NC) pinMode((int)WAKE_PIN, INPUT_PULLUP);

  // NimBLE GAP device name is limited to 31 bytes (CONFIG_BT_NIMBLE_GAP_DEVICE_NAME_MAX_LEN).
  // If the name exceeds this, ble_svc_gap_device_name_set() silently fails and the
  // device advertises as "nimble" (the NimBLE default). Truncate to guarantee our name.
  constexpr size_t BLE_NAME_MAX = 31;
  char bleName[BLE_NAME_MAX + 1];
  strncpy(bleName, USB_PRODUCT, BLE_NAME_MAX);
  bleName[BLE_NAME_MAX] = '\0';

  NimBLEDevice::init(bleName);

#if BLE_DELETE_BONDS_ON_BOOT
  NimBLEDevice::deleteAllBonds();
#endif

#if USE_PUBLIC_BLE_ADDR
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_PUBLIC);
#else
  NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RPA_RANDOM_DEFAULT);
#endif

  NimBLEDevice::setSecurityAuth(true,false,false);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

  NimBLEDevice::setMTU(BLE_MTU);
  NimBLEDevice::setDefaultPhy(BLE_GAP_LE_PHY_2M, BLE_GAP_LE_PHY_2M);
  NimBLEDevice::setPower(BLE_TX_POWER, NimBLETxPowerType::All);

  static ServerCallbacks s_srvCb;
  gServer = NimBLEDevice::createServer();
  gServer->setCallbacks(&s_srvCb);
  gServer->advertiseOnDisconnect(true);

  static NimBLEHIDDevice hidObj(gServer);
  gHid = &hidObj;

  gHid->setManufacturer(USB_MANUFACTURER);
  gHid->setPnp(0x02, USB_VID, USB_PID, HID_VERSION);

  if (auto* dis = gHid->getDeviceInfoService()){
    dis->createCharacteristic((uint16_t)0x2A29, NIMBLE_PROPERTY::READ)->setValue(USB_MANUFACTURER);
    dis->createCharacteristic((uint16_t)0x2A24, NIMBLE_PROPERTY::READ)->setValue(USB_PRODUCT);
    dis->createCharacteristic((uint16_t)0x2A25, NIMBLE_PROPERTY::READ)->setValue(USB_SERIAL);
  }

  gHid->setHidInfo(0x00, 0x03);
  gHid->setReportMap((uint8_t*)hidReportDesc, sizeof(hidReportDesc));

  gHidInput = gHid->getInputReport(0);
  gOut      = gHid->getOutputReport(0);
  gFeat     = gHid->getFeatureReport(0);

  static OutputCallbacks      s_outCb;
  static FeatureCallbacks     s_featCb;
  static InputReportCallbacks s_inCb;

  if (gOut){
    gOut->setCallbacks(&s_outCb);
    static uint8_t z[64]={0}; gOut->setValue(z,64);
  }
  if (gFeat){
    gFeat->setCallbacks(&s_featCb);
    static uint8_t z[64]={0}; gFeat->setValue(z,64);
  }
  if (gHidInput){
    gHidInput->setCallbacks(&s_inCb);
    static uint8_t z[64]={0}; gHidInput->setValue(z,64);
  }

  // FIX: Single battery read for consistent init state
  batteryInit();
  const uint32_t initMv = readVBAT_mV();
  g_vbatFilt_mV = initMv;
  g_battPct = mvToPct(initMv);

  static BatCb s_batCb;
  if (auto* bat = gHid->getBatteryLevel()){ bat->setCallbacks(&s_batCb); }
  gHid->setBatteryLevel(g_battPct, false);

  gHid->startServices();

  if (NimBLEDevice::getServer()->getPeerMTU(BLE_HS_CONN_HANDLE_NONE) == 0)
    debugPrintln("[BLUETOOTH] ✅ GATT services started");
  else
    debugPrintln("[BLUETOOTH] ✅ GATT services restarted");

  gAdv = NimBLEDevice::getAdvertising();
  gAdv->reset();

  NimBLEAdvertisementData adv;
  adv.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);

  static const uint8_t swift_pair_beacon[] = {
      0x06, 0x00,
      0x03, 0x00,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  adv.setManufacturerData(swift_pair_beacon, sizeof(swift_pair_beacon));
  adv.setAppearance(0x03C4);

  static const uint8_t hid_service_ad[] = { 0x03, 0x03, 0x12, 0x18 };
  adv.addData(hid_service_ad, sizeof(hid_service_ad));

  // === DEBUG: Dump exactly what NimBLE is generating ===
  const std::vector<uint8_t>& payload = adv.getPayload();
  debugPrintf("[BLE DEBUG] ADV payload (%d bytes):", payload.size());
  for (size_t i = 0; i < payload.size(); i++) {
      debugPrintf(" %02X", payload[i]);
  }
  debugPrintln();
  // === END DEBUG ===

  NimBLEAdvertisementData scan;
  scan.setName(bleName, true);

  gAdv->enableScanResponse(true);
  bool ok = true;
  ok &= gAdv->setAdvertisementData(adv);
  ok &= gAdv->setScanResponseData(scan);

  gAdv->setMinInterval(32);
  gAdv->setMaxInterval(48);

  if (!ok) debugPrint("[BLUETOOTH] ADV payload config failed");
  if (!gAdv->start()) debugPrint("[BLUETOOTH] ADV start failed");

  g_advStartMs = millis();
  g_advSlowed = false;

}

// Call this each main loop()
void BLE_loop(){

#if BATTERY_KA_TEST
    {
        static uint32_t lastBAT = 0;
        const uint32_t KA_MS = 1500;               // try 1000–2000 ms
        if (g_connected && readyToNotify && gHid && (millis() - lastBAT) >= KA_MS) {
            lastBAT = millis();
            gHid->setBatteryLevel(g_battPct, true);  // 1-byte notify; primes link
            // do NOT touch lastUserInputMs here
        }
    }
#endif

  const uint32_t now = millis();

  static bool autosleep_init=false;
  if (!autosleep_init){
    #if MEASURE_LATENCY_TEST
      portENTER_CRITICAL(&g_reportMux);
      memcpy(last_report_snapshot, ble_last_published, 64);
      portEXIT_CRITICAL(&g_reportMux);
    #else
      memcpy(last_report_snapshot, ble_last_published, 64);
    #endif
    lastUserInputMs = now;
    autosleep_init = true;
  }

  bool inputChanged;
  #if MEASURE_LATENCY_TEST
  portENTER_CRITICAL(&g_reportMux);
    inputChanged = (memcmp(last_report_snapshot, ble_last_published, 64) != 0);
    if (inputChanged) memcpy(last_report_snapshot, ble_last_published, 64);
    portEXIT_CRITICAL(&g_reportMux);
  #else
    portENTER_CRITICAL(&g_bleMux);
    inputChanged = memcmp(last_report_snapshot, ble_last_published, 64) != 0;
    if (inputChanged) memcpy(last_report_snapshot, ble_last_published, 64);
    portEXIT_CRITICAL(&g_bleMux);
  #endif
  if (inputChanged) lastUserInputMs = now;

  // v2.1 FIX: Retry failed notify() calls to prevent state desync
  // If a previous notify() failed (TX buffer full), retry now
  // This ensures switch state changes eventually reach the host
  if (g_inputDirty && g_connected && readyToNotify && gHidInput) {
      uint8_t tx[64];
      portENTER_CRITICAL(&g_bleMux);
      memcpy(tx, ble_last_published, 64);
      portEXIT_CRITICAL(&g_bleMux);
      if (gHidInput->notify(tx, 64)) {
          // v2.1.1 CRITICAL FIX: Prevent race condition on dirty flag clear
          // Only clear if 'tx' (what we just sent) still matches 'ble_last_published'
          // If they differ, new data arrived during notify() — keep flag TRUE to retry NEW data
          portENTER_CRITICAL(&g_bleMux);
          if (memcmp(tx, ble_last_published, 64) == 0) {
              g_inputDirty = false;  // Safe to clear — data is in sync
          }
          // else: new data arrived, flag stays true, next loop iteration will retry
          portEXIT_CRITICAL(&g_bleMux);
      } else {
          // notify() still failing — wait for BLE stack to process TX queue
          // delay(2) is more battery-efficient than taskYIELD() because:
          // - taskYIELD() only yields to equal/higher priority tasks; returns immediately if none ready
          // - delay(2) guarantees 1-2ms of actual sleep, letting RF hardware events fire
          delay(2);
      }
  }

  #if BLE_AUTOSLEEP_MINUTES > 0
  if ((now - lastUserInputMs) >= (uint32_t)BLE_AUTOSLEEP_MINUTES * 60000UL){
    g_blink = false;     // stop heartbeat
    g_ledOn = false;
    arm_deepsleep_wait_gpio2_after_release();
  }
  #endif

  // --- Button handling with debounce and two thresholds ---
  if (WAKE_PIN != GPIO_NUM_NC) {
      static bool sleepCueShown = false;
      static bool redPhase = false;

      const uint8_t cur = (uint8_t)digitalRead((int)WAKE_PIN); // 0 = pressed

      // FIX: Only accept edge if debounce period elapsed
      if (cur != lp_prev && (now - lp_edge_ms) >= DEBOUNCE_MS) {
          const bool wasPressed = (lp_prev == 0);
          lp_prev = cur;
          lp_edge_ms = now;

          if (!wasPressed && cur == 0) {
              // press start
              lp_t0_ms = now;
              sleepCueShown = false;
              redPhase = false;
          }
          else if (wasPressed && cur == 1 && lp_t0_ms) {
              // release -> decide by total hold time
              const uint32_t held = now - lp_t0_ms;
              lp_t0_ms = 0;

              if (held >= BOND_WIPE_HOLD_MS) {
                  wipeBondsAndReboot();                         // never returns
              }
              else if (held >= SLEEP_HOLD_MS) {
                  arm_deepsleep_wait_gpio2_after_release();     // never returns
              }
              // else: short press - do nothing
          }
      }

      // Hold-phase cues
      if (lp_t0_ms) {
          const uint32_t held = now - lp_t0_ms;

          // 3 s cue: cut blue heartbeat so LED goes OFF while still holding
          if (!sleepCueShown && held >= SLEEP_HOLD_MS) {
              g_blink = false;
              if (WS2812MINI_PIN >= 0) {
                  WS2812Mini::WS2812_setLEDColor(0, 0, 0, 0); // blue off
                  WS2812Mini::WS2812_show();
              }
              sleepCueShown = true;
          }

          // ≥10 s: enter red warning phase; wipe occurs on release
          if (held >= BOND_WIPE_HOLD_MS) redPhase = true;

          if (redPhase) {
              if (WS2812MINI_PIN >= 0) {
                  // fast red blink while held beyond 10 s
                  if (((held / 150) & 1) != 0) WS2812Mini::WS2812_setLEDColor(0, 64, 0, 0);
                  else WS2812Mini::WS2812_setLEDColor(0, 0, 0, 0);
                  WS2812Mini::WS2812_show();
              }
          }
      }
  }

  // Blink until connected
  if (g_blink){
    if (now - g_blinkMs >= 200){
      g_blinkMs = now;
      g_ledOn = !g_ledOn;

      if (WS2812MINI_PIN >= 0) {
          if (g_ledOn) WS2812Mini::WS2812_setLEDColor(0, 0, 0, 64);
          else WS2812Mini::WS2812_setLEDColor(0, 0, 0, 0);
          WS2812Mini::WS2812_show();
      }
    }
  }
  
  // FIX: Tiered advertising - slow down after 30s to save power if not connected
  // Guard: gAdv null check for defensive robustness against partial init or reordering
  if (!g_connected && !g_advSlowed && gAdv && (now - g_advStartMs) >= 30000) {
      gAdv->stop();
      gAdv->setMinInterval(160);  // 100ms
      gAdv->setMaxInterval(240);  // 150ms  
      gAdv->start();
      g_advSlowed = true;
      debugPrintln("[BLUETOOTH] Slowed advertising to conserve power");
  }

  // Battery sample every 30 s, EMA + 2% hysteresis
  if (millis() - lastBatteryUpdateMs >= 30000){
    lastBatteryUpdateMs = millis();
    const uint32_t mv_now = readVBAT_mV();
    g_vbatFilt_mV = (3*g_vbatFilt_mV + mv_now)/4;
    const uint8_t newPct = mvToPct(g_vbatFilt_mV);
    if ((newPct > g_battPct ? newPct - g_battPct : g_battPct - newPct) >= 2){
      g_battPct = newPct;
      if (g_connected && gHid){
        gHid->setBatteryLevel(g_battPct, true);
      }
    }
  }

  #if MEASURE_LATENCY_TEST
  static bool latInit=false;
  if (!latInit){ pinMode(LAT_TEST_PIN, INPUT_PULLUP); latInit=true; }
  if (!g_connected) return;
  static uint8_t  prev=1;
  static uint32_t lastEdgeMs=0;
  const uint8_t  cur = (uint8_t)digitalRead(LAT_TEST_PIN);
  const bool     changed = (cur != prev) && (now - lastEdgeMs >= 3);
  const bool     press = changed && (prev==1 && cur==0);
  if (changed){ lastEdgeMs = now; prev = cur; }
  if (press){
    static uint32_t seq=0;
    const uint64_t t0_us = esp_timer_get_time();
    const uint32_t s = (++seq ? seq : ++seq);
    
    // FIX: Protected access to feat_tx_buf and feat_has_tx
    portENTER_CRITICAL(&g_bleMux);
    memset((void*)feat_tx_buf, 0, 64);
    memcpy((void*)(feat_tx_buf+0), &LAT_MAGIC, 4);
    memcpy((void*)(feat_tx_buf+4), &s, 4);
    memcpy((void*)(feat_tx_buf+8), &t0_us, 8);
    feat_has_tx = true;
    portEXIT_CRITICAL(&g_bleMux);
    
    portENTER_CRITICAL(&g_reportMux);
    lat_t0_sent = t0_us;
    portEXIT_CRITICAL(&g_reportMux);
  }
  if (changed && readyToNotify && gHidInput){
    uint8_t tx[64];
    portENTER_CRITICAL(&g_reportMux);
    if (cur == 0) g_bleReport64[32] |= 0x01;
    else          g_bleReport64[32] &= (uint8_t)~0x01;
    clearRttPadding_unsafe();
    memcpy(tx, g_bleReport64, 64);
    portEXIT_CRITICAL(&g_reportMux);
    (void)gHidInput->notify(tx, 64);
  }
  #endif
}

#endif // USE_DCSBIOS_BLUETOOTH
