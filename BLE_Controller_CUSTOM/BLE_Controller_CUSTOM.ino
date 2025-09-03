#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include "RingBuffer.h"
#include "BidireccionalNew.h"

// 16-bit compile-time byte swap
#define BSWAP16(x) (uint16_t)((((x) & 0xFFu) << 8) | (((x) >> 8) & 0xFFu))

// Toggle once here. If your stack needs swapped values, leave ON.
#define BLE_NEEDS_SWAP 1

#if BLE_NEEDS_SWAP
  #define LE16(x) BSWAP16(x)
#else
  #define LE16(x) (uint16_t)(x)
#endif

// Write “normal” values; macro produces what the lib expects.
#define HID_VID             LE16(0xCAFE)  // CockpitOS VID
#define HID_PID             LE16(0xCAF3)  // CockpitOS PID
#define HID_VERSION         LE16(0x0509)  // optional: bcdDevice-style
#define DEVICE_NAME         "BojoteX NavX"
#define BUTTON_PIN          9
#define VIBRATION_PIN       5
#define DEBUG_BT_CONTROLLER 1

// Handshake protocol constants
#define FEATURE_HANDSHAKE_REQ  "DCSBIOS-HANDSHAKE"
#define FEATURE_HANDSHAKE_RESP "DCSBIOS-READY"

volatile bool mainLoopStarted = true; // mimic your USB gate

// --- globals ---
// globals
BLECharacteristic* featureCtrl;

// Tiny static FIFO for host→device messages (no heap)
static volatile uint8_t  feat_rx_len = 0;
static uint8_t           feat_rx_buf[64];
static volatile bool     feat_has_tx = false;
static uint8_t feat_tx_buf[64] = {0};

/*
class FeatureCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    const uint8_t* data = (const uint8_t*)c->getData();
    size_t len = c->getLength();
    if (len > 64) len = 64;

    if (len == sizeof(FEATURE_HANDSHAKE_REQ)-1 &&
        memcmp(data, FEATURE_HANDSHAKE_REQ, len) == 0) {
      memset(feat_tx_buf, 0, 64);
      memcpy(feat_tx_buf, FEATURE_HANDSHAKE_RESP, sizeof(FEATURE_HANDSHAKE_RESP)-1);
      feat_has_tx = true;                               // <<< add this
      c->setValue((uint8_t*)feat_tx_buf, 64);           // stage reply
      return;
    }
  }

  void onRead(BLECharacteristic* c) override {
    if (feat_has_tx) {
      c->setValue((uint8_t*)feat_tx_buf, 64);
      feat_has_tx = false;
    } else {
      static uint8_t zeros[64] = {0};
      c->setValue(zeros, 64);
    }
  }
};
*/

class FeatureCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    const uint8_t* data = (const uint8_t*)c->getData();
    size_t len = c->getLength();
    if (len > 64) len = 64;

    // 1) Handshake short-circuit
    if (len == sizeof(FEATURE_HANDSHAKE_REQ)-1 &&
        memcmp(data, FEATURE_HANDSHAKE_REQ, len) == 0) {
      memset(feat_tx_buf, 0, 64);
      memcpy(feat_tx_buf, FEATURE_HANDSHAKE_RESP, sizeof(FEATURE_HANDSHAKE_RESP)-1);
      feat_has_tx = true;
      c->setValue((uint8_t*)feat_tx_buf, 64);
      return;
    }

    // 2) Normal SET_FEATURE → push to RAW USB OUT ring (USB semantics)
    if (mainLoopStarted && len) dcsRawUsbOutRingbufPushChunked(data, len);
  }

  void onRead(BLECharacteristic* c) override {
    // GET_FEATURE → drain next 64B if any; else zeros
    if (feat_has_tx) {
      c->setValue((uint8_t*)feat_tx_buf, 64);
      feat_has_tx = false;
      return;
    }
    if (!mainLoopStarted) { static uint8_t z[64]={0}; c->setValue(z,64); return; }

    DcsRawUsbOutRingMsg msg;
    if (dcsRawUsbOutRingbufPop(&msg)) {
      uint8_t out[64] = {0};
      if (msg.len) memcpy(out, msg.data, msg.len);
      c->setValue(out, 64);
    } else {
      static uint8_t z[64]={0}; c->setValue(z,64);
    }
  }
};


// --------- BUTTON LOGIC ----------
enum ButtonEvent {
  BTN_NONE,
  BTN_SHORT,
  BTN_LONG,
  BTN_ULTRA
};

// --- Button press timing (milliseconds) ---
#define BTN_DEBOUNCE_MS    20
#define BTN_SHORT_MS      150    // Minimum time for a short press
#define BTN_LONG_MS       700    // Minimum time for a long press
#define BTN_ULTRA_MS     2500    // Minimum time for ultra long press
static bool btnPrevState = false;
static uint32_t btnDownTime = 0;
static uint32_t btnUpTime = 0;
static bool btnWaitingForRelease = false;

BLEHIDDevice* hid;
BLECharacteristic* inputGamepad;
BLECharacteristic* outputVibration;
BLEServer* pServer;
bool deviceConnected = false;

uint32_t lastBatteryUpdateMs  = 0;
uint8_t simulatedBatteryLevel = 25;
uint32_t vibrationEndTime     = 0;
uint8_t vibrationMagnitude    = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
    if(DEBUG_BT_CONTROLLER) printf("BLE device connected\n");

    BLE2902* desc = (BLE2902*)inputGamepad->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    if (desc) desc->setNotifications(true);
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
    if(DEBUG_BT_CONTROLLER) printf("BLE device disconnected\n");
    vibrationMagnitude = 0;
    BLEDevice::startAdvertising();
  }
};

class VibrationCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* chr) override {
    uint8_t* data = chr->getData();
    size_t len = chr->getLength();

    static uint8_t last[9] = {0};
    static size_t lastLen = 0;
    bool changed = (len != lastLen || memcmp(data, last, len) != 0);

    if (changed) {
      char hex[64];
      char* p = hex;
      for (size_t i = 0; i < len && i < sizeof(hex) - 3; ++i)
        p += sprintf(p, "%02X ", data[i]);
      *p = '\0';
      if(DEBUG_BT_CONTROLLER) printf("Received vibration report (len=%d): %s\n", (int)len, hex);

      memcpy(last, data, len);
      lastLen = len;
    }

    if (len == 8 && data[0] == 0x0F) {
      // Generic/Steam HID format
    } else if (len == 9 && data[0] == 0x03) {
      // Xbox BLE format
    } else if (changed) {
      if(DEBUG_BT_CONTROLLER) printf("[WARN] Unknown vibration report format (len=%d)\n", (int)len);
    }
  }
};

void debugOut() {
  if(DEBUG_BT_CONTROLLER) printf("Dumping all known HID characteristics:");
  BLEService* svc = hid->hidService();
  if (!svc) {
    if(DEBUG_BT_CONTROLLER) printf("[ERROR] hid->hidService() returned nullptr\n");
    return;
  }

  const char* knownUUIDs[] = {"2a4a", "2a4b", "2a4c", "2a4d", "2902", "2908"};
  for (const char* uuid : knownUUIDs) {
    BLECharacteristic* chr = svc->getCharacteristic(BLEUUID(uuid));
    if (!chr) continue;
    if(DEBUG_BT_CONTROLLER) printf("✅ Characteristic UUID: 0x%s\n", uuid);
    uint16_t descUUIDs[] = {0x2902, 0x2908};
    for (uint16_t did : descUUIDs) {
      BLEDescriptor* desc = chr->getDescriptorByUUID(BLEUUID(did));
      if (desc && DEBUG_BT_CONTROLLER) printf("  ↳ Descriptor UUID: 0x%04X\n", did);
    }
  }
}

ButtonEvent readButtonEvent() {
  static bool lastStable = false;
  static uint32_t lastDebounceTime = 0;
  static bool debouncedState = false;

  bool raw = !digitalRead(BUTTON_PIN);  // Active LOW
  if (raw != lastStable) {
    lastDebounceTime = millis();
    lastStable = raw;
  }

  if ((millis() - lastDebounceTime) > BTN_DEBOUNCE_MS) {
    if (debouncedState != raw) {
      debouncedState = raw;
      if (debouncedState) {
        // Button pressed
        btnDownTime = millis();
        btnWaitingForRelease = true;
      } else {
        // Button released
        if (btnWaitingForRelease) {
          btnWaitingForRelease = false;
          uint32_t pressLen = millis() - btnDownTime;
          if (pressLen >= BTN_ULTRA_MS)      return BTN_ULTRA;
          else if (pressLen >= BTN_LONG_MS)  return BTN_LONG;
          else if (pressLen >= BTN_SHORT_MS) return BTN_SHORT;
        }
      }
    }
  }
  return BTN_NONE;
}

void enterDeepSleep() {
  if (DEBUG_BT_CONTROLLER) printf("[SLEEP] Going to deep sleep now.");
  BLEDevice::deinit(true); // Clean disconnect if you want
  delay(100);
  // Configure wakeup on button press (active low)
  // esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 0);
  // Optionally: flash LED or haptic as feedback
  delay(100);
  // esp_deep_sleep_start();
}

void requireLongPressOnBoot() {
  uint32_t t0 = millis();
  while (digitalRead(BUTTON_PIN) == LOW) { // Button is held (active low)
    if (millis() - t0 > BTN_ULTRA_MS) { // Held long enough
      if (DEBUG_BT_CONTROLLER) printf("[WAKE] Long press detected, booting up.");
      return; // Continue with normal setup
    }
    delay(10);
  }
  // Not held long enough — go back to deep sleep
  if (DEBUG_BT_CONTROLLER) printf("[WAKE] Button not held long enough, going back to sleep.");
  delay(100);
  enterDeepSleep();
}

void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  hid = new BLEHIDDevice(pServer);
  inputGamepad = hid->inputReport(0);

  outputVibration = hid->outputReport(0);
  outputVibration->setCallbacks(new VibrationCallbacks()); // no 0x2908 descriptor

  // Feature report, Report ID 0
  featureCtrl = hid->featureReport(0);

  // Override the library’s encrypted perms → allow immediate GET/SET_FEATURE
  featureCtrl->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);              // char perms
  featureCtrl->setReadProperty(true);
  featureCtrl->setWriteProperty(true);

  // Also relax the existing Report Reference (0x2908) descriptor perms (library set them encrypted)
  BLEDescriptor* rr_feat = featureCtrl->getDescriptorByUUID((uint16_t)0x2908);
  if (rr_feat) {
      rr_feat->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
  }

  // Keep your callbacks/value
  featureCtrl->setCallbacks(new FeatureCallbacks());
  static uint8_t zeros64[64] = {0};
  featureCtrl->setValue(zeros64, 64);

  hid->manufacturer()->setValue("Microsoft");
  hid->pnp(0x02, HID_VID, HID_PID, HID_VERSION);
  hid->hidInfo(0x00, 0x01);
  hid->reportMap((uint8_t*)hidReportDesc, sizeof(hidReportDesc));
  hid->startServices();

  BLEDevice::getAdvertising()->addServiceUUID(hid->hidService()->getUUID());

  BLESecurity security;
  security.setAuthenticationMode(ESP_LE_AUTH_BOND);
  security.setCapability(ESP_IO_CAP_NONE);
  security.setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(hid->hidService()->getUUID());

  BLEAdvertisementData advData;
  BLEAdvertisementData scanResp;

  static const uint8_t swift_pair_beacon[] = {
    0x06, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  advData.setManufacturerData(String((char*)swift_pair_beacon, sizeof(swift_pair_beacon)));
  advData.setFlags(ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  advData.setAppearance(HID_GAMEPAD);
  advData.addData(String("\x03\x03\x12\x18", 4));
  scanResp.setName(DEVICE_NAME);

  adv->setAdvertisementData(advData);
  adv->setScanResponseData(scanResp);
  adv->start();

  if(DEBUG_BT_CONTROLLER) printf("Advertising started\n");
  if(DEBUG_BT_CONTROLLER) printf("Device Name: %s\n", DEVICE_NAME);
}

void loop() {

  static uint8_t lastReport[sizeof(GamepadReport_t)] = {0};
  static uint8_t lastReportedLevel = 0xFF;
  GamepadReport_t report = {};

  // === HID button logic: Press maps to HID button 0 ===
  if (!digitalRead(BUTTON_PIN)) {   // active LOW
      report.buttons |= 0x01;
  }

  // Optionally: do special logic on long/ultra press
  ButtonEvent btnEvent = readButtonEvent();
  switch (btnEvent) {
    case BTN_LONG:
      if (DEBUG_BT_CONTROLLER) printf("Long press: macro\n");
      // Optional: trigger something
      break;
    case BTN_ULTRA:
      if (DEBUG_BT_CONTROLLER) printf("Ultra long press: Power toggle\n");
      // Optional: devicePowerToggle();
      break;
    default:
      break;
  }






  if (dcsUdpRingbufPending()) {
    DcsUdpRingMsg m;
    if (dcsUdpRingbufPop(&m)) {
      // do whatever your firmware normally does with incoming OUT data
      // e.g., echo a short status back through FEATURE:
      const char ack[] = "OK";
      dcsRawUsbOutRingbufPushChunked((const uint8_t*)ack, sizeof(ack)-1);
    }
  }





  if (deviceConnected && memcmp(&report, lastReport, sizeof(report)) != 0) {
    inputGamepad->setValue((uint8_t*)&report, sizeof(report));
    inputGamepad->notify();
    memcpy(lastReport, &report, sizeof(report));
  }

  // Battery update logic, unchanged
  if (millis() - lastBatteryUpdateMs >= 2000) {
    lastBatteryUpdateMs = millis();
    if (simulatedBatteryLevel > 1) simulatedBatteryLevel--;
    if (simulatedBatteryLevel != lastReportedLevel) {
      hid->setBatteryLevel(simulatedBatteryLevel);
      lastReportedLevel = simulatedBatteryLevel;
    }
  }

  delay(10);
}