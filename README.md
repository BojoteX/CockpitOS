# 🚀 CockpitController Firmware

[![Platform](https://img.shields.io/badge/Platform-ESP32--S2-blue.svg)](https://www.espressif.com/en/products/socs/esp32-s2) [![Framework](https://img.shields.io/badge/Framework-Arduino-green.svg)](https://www.arduino.cc/) [![USB HID](https://img.shields.io/badge/USB%20HID-Adafruit%20TinyUSB-yellow.svg)](https://github.com/adafruit/Adafruit_TinyUSB_Arduino)

## 🎯 Overview

The **CockpitController Firmware** is a modular, efficient, and highly extensible firmware solution specifically designed for the ESP32-S2 Mini microcontroller. It powers the TEKCreations F/A-18C Hornet cockpit panels, providing seamless integration via USB HID with popular flight simulators like **DCS World**, alongside comprehensive LED state management and planned full DCS-BIOS compatibility.

---

## ✨ Features

- **Modular Design**: Each cockpit panel is managed via dedicated, clearly structured modules.
- **USB HID Integration**: Seamless integration using Adafruit TinyUSB, presenting panels as standard HID devices.
- **Centralized LED Management**: Flexible and efficient LED control with support for multiple hardware drivers.
- **Efficient Hardware Abstraction**: Robust, optimized drivers for PCA9555, TM1637, GN1640T, and WS2812 components.
- **DCS-BIOS Ready**: Partially implemented DCS-BIOS bridge with planned full integration for deeper simulator interaction.
- **Scalable Architecture**: Designed to easily accommodate additional cockpit panels and hardware.
- **Comprehensive Debugging**: Built-in debugging tools and descriptive logging for simplified development and troubleshooting.

---

## 🛠 Supported Hardware

- **ESP32-S2 Mini (LOLIN S2 Mini)**
- **TEKCreations F/A-18C Cockpit Panels**
  - ECM Panel
  - Master Arm Panel
  - Left & Right Annunciator Panels
  - IR Cool Panel
  - Lock/Shoot Panel
  - Caution/Advisory Panel

---

## 🧑‍💻 Requirements and Compilation

### 📌 Requirements

- **Arduino IDE 2.x**
- **ESP32 by Espressif v3.1.3** (due to compatibility issues, v3.2 is not currently supported)
- **Adafruit TinyUSB Library v3.4.4** (Note: Compatibility issue with ESP32 v3.2 resolved but not yet available via Arduino IDE)
- **FastLED by Daniel Garcia** (required for the LockShoot indicator)

### 🔨 Compilation Steps

1. Install the **Arduino IDE 2.x** from the [official Arduino site](https://www.arduino.cc/en/software).
2. In Arduino IDE, open **File → Preferences** and add the following URL to the **Additional Boards Manager URLs**:

```
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
```

3. Go to **Tools → Board → Boards Manager**, search for "ESP32", select and install version **3.1.3** of **ESP32 by Espressif**.
4. Install required libraries via **Tools → Manage Libraries**:
   - **Adafruit TinyUSB v3.4.4**
   - **FastLED by Daniel Garcia**

5. Select the board **"LOLIN S2 Mini"** from the menu **Tools → Board → ESP32 Arduino**.

6. Open the `CockpitController.ino` file from this repository and click **Verify/Upload**.

---

## 📦 Project Structure

```
CockpitController
│
├── CockpitController.ino             # Main entry point and core logic
├── CUtils                            # Low-level hardware drivers
│   ├── PCA9555.cpp
│   ├── TM1637.cpp
│   ├── GN1640T.cpp
│   └── WS2812.cpp
├── LEDControl.cpp                    # Centralized LED management
├── HIDManager.cpp                    # USB HID abstraction
├── DCSBIOSBridge.cpp                 # DCS-BIOS integration (in progress)
├── Panels                            # Individual cockpit panel implementations
│   ├── ECMPanel.cpp
│   ├── MasterARMPanel.cpp
│   ├── LeftAnnunciator.cpp
│   ├── RightAnnunciator.cpp
│   ├── IRCoolPanel.cpp
│   └── LockShootPanel.cpp
└── src
    └── LEDMappings.h                 # Centralized LED mapping definitions
```

---

## 🚧 Upcoming Features

- **Complete DCS-BIOS integration**: Full serial interaction for bidirectional control with simulators.
- **Automated Panel Detection**: Enhanced I²C panel discovery for plug-and-play functionality.
- **Enhanced Documentation**: Comprehensive in-code and external documentation for contributors and users.

---

## 📝 Contributing

Contributions are welcome! To get started:

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/myFeature`)
3. Commit your changes (`git commit -am 'Add my new feature'`)
4. Push to the branch (`git push origin feature/myFeature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙌 Acknowledgments

Special thanks to:
- [Adafruit](https://www.adafruit.com/) for the TinyUSB library
- [TEKCreations](https://tekcreations.space/) for their outstanding cockpit panel hardware
- The DCS community for continued support and testing

---

**Enjoy flying with your enhanced cockpit immersion! 🚀**