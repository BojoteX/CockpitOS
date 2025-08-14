# CockpitOS HID Controller

**Cross-platform USB HID bridge for DCS-BIOS cockpit devices (ESP32/ESP32S2/ESP32S3)**  
*No more serial ports. No more COM driver headaches. Use your cockpit panels from anywhere on your network.*

---

## ✈️ Overview

CockpitOS HID Controller is a robust, user-friendly Python companion tool for your DCS-BIOS-enabled ESP32-based cockpit hardware.  
It replaces the traditional serial interface with a high-performance USB HID protocol—**and works across Windows, Linux, and macOS.**

- **No serial, no COM ports, no drivers. Just plug and fly.**
- **Run your panels on any PC (not just the DCS machine) thanks to seamless network bridging.**

---

## 🚀 Features

- **Truly Cross-Platform:** Runs on Windows, Linux, macOS.
- **No Serial Ports:** Communicates with your panels using USB HID, for plug-and-play experience.
- **Network Transparent:** Connect panels to any machine on your network. No need to physically plug into your DCS computer!
- **Live Dashboard:** Visualize connected devices, event logs, and bandwidth in real time.
- **Supports Multiple Panels:** Automatically detects and manages multiple devices.
- **Bulletproof Data Handling:** Ensures no lost or out-of-order events, even in case of network or USB hiccups.

---

## 🗺️ Architecture

1. **UDP RX Thread:**  
   Listens for DCS-BIOS UDP multicast data, splits it into 64-byte chunks, and sends as HID OUT reports to all connected panels.

2. **Per-Device Threads:**  
   For each ESP32 panel, handles handshake, reads button/axis/encoder events, and relays commands back to DCS World via UDP.

3. **GUI/Main Thread:**  
   The Tkinter-based dashboard shows device status, real-time logs, and network stats.

---

## 🌐 Use It From Anywhere

- **Your cockpit device can be plugged into ANY computer on your local network.**
- The HID Controller bridges cockpit events over your LAN to the DCS World PC.
- No need for USB extenders or running cables to your main simulator rig.

---

## 🖥️ Requirements

- **Python 3.7+**
- `hidapi` Python package:  
  `pip install hidapi`
- Supported ESP32 firmware: [See CockpitOS firmware](https://github.com/BojoteX/CockpitOS)
- DCS World with DCS-BIOS UDP export enabled

---

## 🔌 Installation

1. **Clone this repo**
    ```sh
    git clone https://github.com/BojoteX/CockpitOS.git
    cd CockpitOS/HID\ Controller
    ```

2. **Install Python requirements**
    ```sh
    pip install hidapi
    ```

3. **(Optional) Set your VID/PID in `settings.ini`**  
   If you use custom firmware VID/PID values.

---

## ⚡ Usage

1. **Plug in your ESP32-based cockpit panel(s).**
2. **Launch DCS World with DCS-BIOS UDP enabled.**
3. **Start the HID Controller:**
    ```sh
    python CockpitOS_HID_Manager.py
    ```

- The GUI will display:
    - All connected devices
    - Real-time button/axis events
    - Network stats (frames/sec, bandwidth)
    - Device connection/disconnection logs

**Your panel now works with DCS—even over the network!**

---

## 🧩 How It Works

- **DCS-BIOS ➔ UDP Multicast ➔ HID Controller ➔ HID OUT ➔ ESP32 Device**
- **ESP32 Device ➔ HID FEATURE Report ➔ HID Controller ➔ UDP ➔ DCS-BIOS**

- HID OUT reports carry DCS-BIOS packets to your cockpit.
- FEATURE reports (from device to host) send button/axis/encoder changes as ASCII commands, which are forwarded back to DCS World via UDP.

---

## 🏆 Advantages Over Serial

- No virtual COM ports or drivers
- Superior hotplug/reconnect support
- Multiple panels supported simultaneously
- Network-transparent (plug devices anywhere)
- No special configuration needed on the DCS PC

---

## 🛡️ Reliability

- Fully thread-safe, with robust event queuing
- Handles device disconnects/reconnects
- No data loss, even if triggers are missed or the network is busy

---

## 📝 File Structure

- `CockpitOS_HID_Manager.py` — Application
- `settings.ini` — Set your VID/PID (if needed)

---

## 🧑‍💻 For Developers

- Ready for porting to C++/Qt/Boost Asio/libusb
- Explicit, well-documented threading and locking
- Each core function is separable for easy extension

---

## ❓ FAQ

**Q: Can I use this on Linux or Mac?**  
A: Absolutely! Any OS with Python and the HID API Library installed.

**Q: Do I need to plug my panels into the DCS computer?**  
A: No! As long as the HID Manager is running on any LAN-connected PC, your panel(s) will work.

**Q: Will I lose events if my USB gets busy?**  
A: No. The program is designed to recover and deliver all pending events as soon as the system is ready.

**Q: Can I use multiple panels at once?**  
A: Yes! All are auto-detected and managed independently.

---

## 🆘 Troubleshooting

- **No device detected?**  
  Double-check your VID/PID in `settings.ini` and verify your firmware is loaded.
- **No events in DCS?**  
  Confirm DCS-BIOS UDP export is enabled and not firewalled. Multi-cast usually won't travel across networks/routers
- **Device randomly disconnects?**  
  USB power save or cable/connector issues—check system logs. Make sure CDC on boot is DISABLED (Serial conflicts with HID)

---

## 🤝 Contributing

PRs, suggestions, and bug reports are welcome! See `CONTRIBUTING.md` for guidelines.

---

## 📜 License

[MIT](LICENSE)

---

**Happy Flying!  
— The CockpitOS Firmware Project Team**

---