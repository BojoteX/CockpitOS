// HID report descriptor: 16 axes, 32 buttons. Padded. We use GAMEPAD_REPORT_SIZE
// 16 axes, 32 buttons. IN/OUT: 64 bytes each

static const uint8_t hidReportDesc[] = {
    // === Gamepad Input Report (64 bytes total) ===
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x05,       // Usage (Gamepad)
    0xA1, 0x01,       // Collection (Application)

    // Axes: 16 axes, each 16-bit = 32 bytes
    0x15, 0x00,             // Logical Minimum (0)
    0x26, 0x1F, 0x20,       // Logical Maximum (8191)
    0x75, 0x10,             // Report Size (16 bits)
    0x95, 0x10,             // Report Count (16 axes)
    // We’ll reuse Usage (Slider) multiple times for now
    0x09, 0x30,             // Usage (X)
    0x09, 0x31,             // Usage (Y)
    0x09, 0x32,             // Usage (Z)
    0x09, 0x33,             // Usage (Rx)
    0x09, 0x34,             // Usage (Ry)
    0x09, 0x35,             // Usage (Rz)
    0x09, 0x36,             // Usage (Slider 1)
    0x09, 0x36,             // Usage (Slider 2)
    0x09, 0x37,             // Usage (Dial)
    0x09, 0x38,             // Usage (Wheel)
    0x09, 0x36,             // Slider again (Brake)
    0x09, 0x36,             // Slider again (Throttle)
    0x09, 0x36,             // Custom1
    0x09, 0x36,             // Custom2
    0x09, 0x36,             // Custom3
    0x09, 0x36,             // Custom4
    0x81, 0x02,             // Input (Data,Var,Abs)

    // Buttons: 32 x 1-bit = 4 bytes
    0x05, 0x09,             // Usage Page (Buttons)
    0x19, 0x01,             // Usage Min (1)
    0x29, 0x20,             // Usage Max (32)
    0x15, 0x00,             // Logical Minimum (0)
    0x25, 0x01,             // Logical Maximum (1)
    0x75, 0x01,             // Report Size (1 bit)
    0x95, 0x20,             // Report Count (32)
    0x81, 0x02,             // Input (Data,Var,Abs)

    // Padding to 64 bytes (32 axes + 4 buttons = 36 bytes used)
    0x75, 0x08,             // Report Size (8 bits)
    0x95, 0x1C,             // Report Count (28 bytes padding = 64 - 36)
    0x81, 0x03,             // Input (Constant,Var,Abs)

    // === Output Report (64 bytes) ===
    0x06, 0x00, 0xFF,       // Usage Page (Vendor Defined)
    0x09, 0x01,             // Usage (Vendor 1)
    0x75, 0x08,
    0x95, 0x40,
    0x91, 0x02,             // Output (Data,Var,Abs)

    // === Feature Report (64 bytes) ===
    0x06, 0x00, 0xFF,
    0x09, 0x02,
    0x75, 0x08,
    0x95, 0x40,
    0xB1, 0x02,             // Feature (Data,Var,Abs)

    0xC0                    // End Collection
};

typedef union {
  struct __attribute__((packed)) {
    uint16_t axes[HID_AXIS_COUNT];  // 16 axes × 2 = 32 bytes
    uint32_t buttons;               // 4 bytes
    uint8_t  reserved[GAMEPAD_REPORT_SIZE - 36];  // Padding: 64 - (32+4)
  };
  uint8_t raw[GAMEPAD_REPORT_SIZE];
} GamepadReport_t;
static_assert(sizeof(GamepadReport_t) == GAMEPAD_REPORT_SIZE, "GamepadReport_t size mismatch!");