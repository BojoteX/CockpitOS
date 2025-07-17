#pragma once

// Handshake protocol constants
#define FEATURE_HANDSHAKE_REQ  "DCSBIOS-HANDSHAKE"
#define FEATURE_HANDSHAKE_RESP "DCSBIOS-READY"

#include <USBHID.h>

// Just choose ONE set of descriptors
#include "CustomDescriptors/Bidireccional.h" // 3 Axes, 32 Buttons + Incoming reports (from PC)

extern const uint8_t hidReportDesc[];
extern GamepadReport_t report;

// Declare HID so the class can reference it
extern USBHID HID;

class GPDevice : public USBHIDDevice {
public: 

  GPDevice() {
    // Set identifiers before adding device
    USB.VID(USB_VID);
    USB.PID(USB_PID);
    USB.manufacturerName(USB_MANUFACTURER);
    USB.productName(USB_PRODUCT);
    USB.serialNumber(USB_SERIAL);
    HID.addDevice(this, sizeof(hidReportDesc));

    // Register only valid HID events!
    HID.onEvent(ARDUINO_USB_HID_SET_PROTOCOL_EVENT, GPDevice::hidSetProtocolHandler);
    HID.onEvent(ARDUINO_USB_HID_SET_IDLE_EVENT,     GPDevice::hidSetIdleHandler);
  }

  // Descriptor supply
  uint16_t _onGetDescriptor(uint8_t* buf) override {
    memcpy(buf, hidReportDesc, sizeof(hidReportDesc));
    return sizeof(hidReportDesc);
  }


#if USE_DCSBIOS_USB
  // GET FEATURE: drain ring buffer if mainLoopStarted
  uint16_t _onGetFeature(uint8_t report_id, uint8_t* buffer, uint16_t len) override {

    // Always clear buffer before returning if not started
    if(!mainLoopStarted) {
        memset(buffer, 0, 64);
        return 64;
    }

    if (dcsRawUsbOutRingbufPending() > 0) {
        DcsRawUsbOutRingMsg msg;
        dcsRawUsbOutRingbufPop(&msg);
	memcpy(buffer, msg.data, msg.len);
	if (msg.len < 64)
    	    memset(buffer + msg.len, 0, 64 - msg.len);
        return 64;
    }
    memset(buffer, 0, 64);
    return 64;
  }

  // SET FEATURE: any host message is just pushed to ring buffer, even before main loop
  void _onSetFeature(uint8_t report_id, const uint8_t* buffer, uint16_t len) override {
    if(mainLoopStarted && len > 0) {
        dcsRawUsbOutRingbufPushChunked(buffer, len);
        debugPrintf("[SET FEATURE] pushed %.*s to ring buffer\n", len, buffer);
	forceResync();
    }
  }

  // Output report (host → device)
  void _onOutput(uint8_t report_id, const uint8_t* buffer, uint16_t len) override {

        // Don't do anything unless we start our main loop
        if(!mainLoopStarted) return;

	dcsUdpRingbufPushChunked(buffer, len);
  }
#endif

  // Input report (device → host)
  bool sendReport(const void* data, int len) {
       return HID.SendReport(0, data, len, HID_SENDREPORT_TIMEOUT);
  }

  // RAW Input report (device → host) — for ring buffer
  bool sendRawReport(const void* data, int len) {
    // Report ID 1 (64 bytes)
    return HID.SendReport(1, data, len, HID_SENDREPORT_TIMEOUT);
  }

  // ---- GLOBAL EVENT HANDLERS (STATIC) ----
  static void hidSetProtocolHandler(void* arg, esp_event_base_t base, int32_t id, void* event_data) {
    auto* d = (arduino_usb_hid_event_data_t*)event_data;
    debugPrintf("[HID EVENT] SET_PROTOCOL itf=%u → %s\n",
                (unsigned)d->instance,
                d->set_protocol.protocol ? "REPORT" : "BOOT");
  }
  static void hidSetIdleHandler(void* arg, esp_event_base_t base, int32_t id, void* event_data) {
    auto* d = (arduino_usb_hid_event_data_t*)event_data;
    if (d->set_idle.idle_rate == 0) {
      debugPrintf("[HID EVENT] SET_IDLE: Interface %u → no periodic reports required\n",
                  (unsigned)d->instance);
    } else {
      unsigned ms = d->set_idle.idle_rate * 4;
      debugPrintf("[HID EVENT] SET_IDLE: Interface %u → periodic report required every %u ms\n",
                  (unsigned)d->instance, ms);
    }
  }
};
extern GPDevice gamepad;