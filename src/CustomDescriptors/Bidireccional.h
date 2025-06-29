// HID report descriptor: 3 axes (Rx, Slider1, Slider2), 32 buttons. Padded to 16 Bytes. We use GAMEPAD_REPORT_SIZE
// 3 axes (Rx, Slider1, Slider2), 32 buttons. IN/OUT: 64 bytes each

// HID report descriptor: 3 axes (Rx, Slider1, Slider2), 32 buttons, 64 bytes IN/OUT/FEATURE
static const uint8_t hidReportDesc[] = {
  // === Gamepad Input Report (64 bytes) ===
  0x05, 0x01,       // Usage Page (Generic Desktop)
  0x09, 0x05,       // Usage (Gamepad)
  0xA1, 0x01,       // Collection (Application)

    // Axes (3 x 16 bits = 6 bytes)
    0x09, 0x36,       // Usage (Slider) (Rx)
    0x09, 0x36,       // Usage (Slider1)
    0x09, 0x36,       // Usage (Slider2)
    0x15, 0x00,       // Logical Minimum (0)
    0x26, 0xFF, 0x0F, // Logical Maximum (4095)
    0x75, 0x10,       // Report Size (16 bits)
    0x95, 0x03,       // Report Count (3)
    0x81, 0x02,       // Input (Data,Var,Abs)

    // Buttons (32 x 1 bit = 4 bytes)
    0x05, 0x09,       // Usage Page (Buttons)
    0x19, 0x01,       // Usage Min (1)
    0x29, 0x20,       // Usage Max (32)
    0x15, 0x00,       // Logical Minimum (0)
    0x25, 0x01,       // Logical Maximum (1)
    0x75, 0x01,       // Report Size (1)
    0x95, 0x20,       // Report Count (32)
    0x81, 0x02,       // Input (Data,Var,Abs)

    // Padding to reach 16 bytes total for gamepad
    0x75, 0x08,       // Report Size (8 bits)
    0x95, 0x06,       // Report Count (6 bytes padding)
    0x81, 0x03,       // Input (Constant,Var,Abs)

    // ADDITIONAL PADDING to reach 64 bytes
    0x75, 0x08,       // Report Size (8 bits)
    0x95, 0x30,       // Report Count (48 bytes padding: 64 - 16)
    0x81, 0x03,       // Input (Constant,Var,Abs)

    // === Output Report (64 bytes, Vendor Page) ===
    0x06, 0x00, 0xFF, // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x01,       // Usage (Vendor Usage 1)
    0x75, 0x08,       // Report Size (8 bits)
    0x95, 0x40,       // Report Count (64)
    0x91, 0x02,       // Output (Data,Var,Abs)

    // === Feature Report (64 bytes, Vendor Page) ===
    0x06, 0x00, 0xFF, // Usage Page (Vendor Defined 0xFF00)
    0x09, 0x02,       // Usage (Vendor Usage 2)
    0x75, 0x08,       // Report Size (8 bits)
    0x95, 0x40,       // Report Count (64)
    0xB1, 0x02,       // Feature (Data,Var,Abs)

  0xC0              // End Collection
};

// HID report structure
typedef union {
  struct __attribute__((packed)) {
    uint16_t rx;
    uint16_t slider1;
    uint16_t slider2;
    uint32_t buttons;
    uint8_t  reserved[GAMEPAD_REPORT_SIZE - 10]; // 10 bytes = 6 axes/buttons + 4 buttons
  };
  uint8_t raw[GAMEPAD_REPORT_SIZE];
} GamepadReport_t;
static_assert(sizeof(GamepadReport_t) == GAMEPAD_REPORT_SIZE, "GamepadReport_t size mismatch!");