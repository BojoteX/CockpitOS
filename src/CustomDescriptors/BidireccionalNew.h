// BidireccionalNew.h — 16 axes, 32 buttons, 64-byte IN/OUT, no Report IDs

#pragma once

// ---- HID Report Descriptor ----
static const uint8_t hidReportDesc[] = {
  // Top-level
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x05,       // Usage (Gamepad)
  0xA1, 0x01,       // Collection (Application)

  // Axes: 16 × 16-bit = 32 bytes
  0x15, 0x00,             // Logical Min 0
  0x26, 0xFF, 0x0F,       // Logical Max 4095
  0x75, 0x10,             // Report Size 16
  0x95, 0x10,             // Report Count 16 fields

  // First 8 – what DirectInput actually maps
  0x09, 0x30,  // X
  0x09, 0x31,  // Y
  0x09, 0x32,  // Z
  0x09, 0x33,  // Rx
  0x09, 0x34,  // Ry
  0x09, 0x35,  // Rz
  0x09, 0x37,  // Dial
  0x09, 0x36,  // Slider

  // 0x09, 0x37,  // Dial  
  // 0x09, 0x36,  // Slider 1
  // 0x09, 0x36,  // Slider 2
  // 0x09, 0x36,  // Slider 3
  // 0x09, 0x36,  // Slider 4
  // 0x09, 0x36,  // Slider 5
  // 0x09, 0x36,  // Slider 6
  // 0x09, 0x36,  // Slider 7
  // 0x09, 0x36,  // Slider 8  

  0x81, 0x02,             // Input (Data,Var,Abs)

  // Buttons: 32 × 1-bit = 4 bytes
  0x05, 0x09,             // Usage Page (Button)
  0x19, 0x01,             // Usage Min 1
  0x29, 0x20,             // Usage Max 32
  0x15, 0x00,             // Logical Min 0
  0x25, 0x01,             // Logical Max 1
  0x75, 0x01,             // Report Size 1
  0x95, 0x20,             // Report Count 32
  0x81, 0x02,             // Input (Data,Var,Abs)

  // Padding to 64 bytes total input
  0x75, 0x08,             // Report Size 8
  0x95, 0x1C,             // Report Count 28 bytes padding = 64 − (32+4)
  0x81, 0x03,             // Input (Const,Var,Abs)

  // Output report: 64 bytes (vendor page)
  0x06, 0x00, 0xFF,       // Usage Page (Vendor Defined)
  0x09, 0x01,             // Usage (Vendor 1)
  0x75, 0x08,
  0x95, 0x40,
  0x91, 0x02,             // Output (Data,Var,Abs)

  // Feature report: 64 bytes (vendor page)
  0x06, 0x00, 0xFF,
  0x09, 0x02,
  0x75, 0x08,
  0x95, 0x40,
  0xB1, 0x02,             // Feature (Data,Var,Abs)

  0xC0                    // End Collection
};

// ---- Report payload layout (64 bytes IN) ----
typedef union {
  struct __attribute__((packed)) {
    uint16_t axes[16];                         // 32 bytes
    uint32_t buttons;                          // 4 bytes
    uint8_t  reserved[64 - (32 + 4)];          // 28 bytes padding
  };
  uint8_t raw[64];
} GamepadReport_t;
static_assert(sizeof(GamepadReport_t) == 64, "GamepadReport_t size mismatch!");
