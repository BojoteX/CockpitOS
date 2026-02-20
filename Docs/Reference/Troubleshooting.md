# Troubleshooting

This guide is organized by symptom. Find the problem you are experiencing, then follow the diagnostic steps and solutions.

---

## Compilation Errors

### "USB CDC" errors or "Invalid config: Set 'USB CDC On Boot' to 'Disabled'"

**Cause:** The Arduino IDE (or board configuration) has "USB CDC On Boot" set to Enabled. CockpitOS requires it to be Disabled.

**Fix:**
- If using the **Compiler Tool** (`CockpitOS-START.py`): This is handled automatically. Re-run the tool.
- If using **Arduino IDE**: Go to Tools > USB CDC On Boot > **Disabled**.

The firmware enforces this with a compile-time check:
```
#error "Invalid config: Set 'USB CDC On Boot' to 'Disabled' (see Tools menu in Arduino IDE)."
```

---

### "TinyUSB" errors or "USE_DCSBIOS_USB is only supported on ESP32-S2 / ESP32-S3 / ESP32-P4"

**Cause:** You have `USE_DCSBIOS_USB=1` but the selected board is not an S2, S3, or P4.

**Fix:**
- Select the correct board in the Compiler Tool or Arduino IDE (ESP32-S2, ESP32-S3, or ESP32-P4).
- If your board does not support USB HID, switch to WiFi transport using the Compiler Tool's **Role / Transport** menu.

---

### "You need to enable USB-OTG (TinyUSB)"

**Cause:** On S3 or P4 boards, the USB Mode must be set to USB-OTG (TinyUSB) instead of the default Hardware CDC.

**Fix:**
- If using the **Compiler Tool**: This is handled automatically.
- If using **Arduino IDE**: Go to Tools > USB Mode > **USB-OTG (TinyUSB)**.

---

### "LovyanGFX" not found or library errors

**Cause:** Required libraries are not installed.

**Fix:** Run the Setup Tool:
```
python Setup-START.py
```
Select the option to install or update libraries. This installs LovyanGFX and all other dependencies.

---

### "Invalid config: Exactly ONE transport must be enabled"

**Cause:** You have zero or more than one transport flag set to `1` in `Config.h`.

**Fix:** Use the Compiler Tool's **Role / Transport** menu to select your transport mode. The tool ensures exactly one transport is enabled and sets all others to `0` automatically.

If using Arduino IDE, verify in `Config.h` that exactly one of these is `1`:
```cpp
#define USE_DCSBIOS_BLUETOOTH    0
#define USE_DCSBIOS_WIFI         0
#define USE_DCSBIOS_USB          1  // <-- only ONE is 1
#define USE_DCSBIOS_SERIAL       0
#define RS485_SLAVE_ENABLED      0
```

---

### "Cannot be both RS485 Master and Slave"

**Cause:** Both `RS485_MASTER_ENABLED` and `RS485_SLAVE_ENABLED` are set to `1`.

**Fix:** A device can only be a master or a slave, not both. Set one to `0`.

---

### "You need to run the auto-generator script in the LABELS directory"

**Cause:** No label set is selected as active, or the active label set is missing its generated files.

**Fix:**
1. Open the Label Creator (`LabelCreator-START.py`).
2. Select or create a label set.
3. Use "Generate & Set as Default" to regenerate files and mark it active.

---

### Out of memory during compilation (S2 devices)

**Cause:** Enabling WiFi stack + TinyUSB + debug output simultaneously on an S2 exceeds available memory.

**Fix:**
- Disable all debug flags for production builds.
- If debugging, use only Serial debug when on USB transport, or WiFi debug when on WiFi transport. Do not enable both stacks at once on S2.
- Consider upgrading to an S3 board, which has significantly more memory.

---

## Upload Problems

### Device not detected / no COM port appears

**Possible causes and fixes:**

1. **Charge-only USB cable.** Try a different cable that supports data transfer.
2. **Wrong USB port.** Some PC ports may not supply enough power. Try a different port.
3. **Driver not installed.** On Windows, check Device Manager for unrecognized devices. Most modern ESP32 boards use a built-in USB-to-UART bridge (CP2102, CH340) or native USB.
4. **Board needs bootloader mode.** Hold the **BOOT** button while connecting USB, then release.

---

### Upload fails or times out

**Steps to try, in order:**

1. **Bootloader mode:** Disconnect USB. Hold the BOOT button. Connect USB while holding BOOT. Release BOOT. Try uploading again.
2. **Close other programs** that may be using the COM port (serial monitors, other IDEs, HID Manager).
3. **Try a lower upload speed** in the Compiler Tool or Arduino IDE (115200 instead of 921600).
4. **Different USB cable/port** -- rules out hardware issues.
5. **Check Device Manager** (Windows) or `ls /dev/tty*` (Linux/Mac) to confirm the port exists.

---

### Upload succeeds but device does not respond

**Cause:** The firmware may be booting into an error state.

**Fix:**
1. Open a serial monitor (115200 baud) and check for error messages.
2. If you see watchdog timeout messages, the startup is taking too long. Check that your label set files are generated correctly.
3. Re-flash using the Compiler Tool with a known-good label set (like `LABEL_SET_TEST_ONLY`).

---

## Connection Issues

### WiFi will not connect

**Diagnostic steps:**

1. **Check credentials.** SSID and password are case-sensitive. Use the Compiler Tool's **Misc Options** > **Wi-Fi Credentials** to review and update them.
2. **2.4 GHz only.** ESP32 cannot connect to 5 GHz networks. If your router has a combined SSID for both bands, try creating a separate 2.4 GHz SSID.
3. **WPA2-PSK (AES/CCMP) required.** Older WEP or WPA1 networks are not supported. Enterprise (802.1X) authentication is not supported.
4. **Enable network scanning** to see what the ESP32 can detect:
   ```cpp
   #define SCAN_WIFI_NETWORKS  1
   ```
   Recompile, upload, and check Serial output for the list of visible networks.
5. **Run the WiFi debug console:**
   ```
   python "Debug Tools\CONSOLE_UDP_debug.py"
   ```

---

### USB HID not connecting to DCS

**Diagnostic steps:**

1. **HID Manager running?** The HID Manager companion app must be running on the PC. Check the system tray or task manager.
2. **VID match?** The `USB_VID` in `Config.h` (default `0xCAFE`) must match the VID in HID Manager's `settings.ini`.
3. **USB CDC On Boot?** Must be **Disabled**. If it was previously Enabled, recompile and re-upload.
4. **Device visible?** Open Device Manager on Windows and look for the device under "USB HID devices" or "Game controllers."
5. **DCS-BIOS running?** HID Manager bridges between the ESP32 and DCS-BIOS. DCS-BIOS must be installed and a mission must be loaded.

---

### DCS-BIOS not receiving data

**Diagnostic steps:**

1. **DCS-BIOS installed?** Check that the DCS-BIOS mod folder exists in `Saved Games\DCS\Scripts\DCS-BIOS\`.
2. **Mission loaded?** DCS-BIOS only exports data when a mission is running and an aircraft is loaded.
3. **Multicast network check.** DCS-BIOS uses UDP multicast `239.255.50.10:5010`. Ensure:
   - Your PC and ESP32 are on the same subnet
   - Your router/switch supports multicast (most consumer routers do)
   - Windows Firewall is not blocking UDP ports 5010 and 7778
4. **Test with the debug console:**
   ```
   python "Debug Tools\CONSOLE_UDP_debug.py"
   ```
   If the console receives data, the network is working. If not, check firewall rules.

---

### Panel connects but DCS does not respond to commands

**Possible causes:**

1. **Wrong command names.** The DCS-BIOS command names in your `InputMapping.h` must exactly match the DCS-BIOS control identifiers for your aircraft.
2. **Port 7778 blocked.** CockpitOS sends commands to UDP port 7778 on the PC. Check that this port is not blocked by the firewall.
3. **Aircraft mismatch.** Your label set was generated for a different aircraft than the one you are flying.
4. **Debug with command logger:**
   ```
   python "Debug Tools\LOG_DCS_commands.py"
   ```

---

## Input Problems

### Button press does nothing

1. **Check wiring.** Buttons connect between a GPIO pin and GND. CockpitOS enables internal pull-ups, so no external resistor is needed.
2. **Verify InputMapping.** In the Label Creator, confirm that the button's Source is set to `GPIO` and the Port matches the physical pin.
3. **Wrong control type.** Push buttons should be `"momentary"`. Toggle switches that need to track position should be `"selector"`.
4. **Enable debug output.** In the Compiler Tool, go to **Misc Options** > **Debug / Verbose Toggles** and enable the appropriate verbose mode (Serial or WiFi). Recompile and check the output for button state changes.

---

### Rotary encoder skips values or jumps

1. **Noisy encoder.** Add 100nF (0.1uF) capacitors between each encoder pin and GND.
2. **Pin assignment.** Verify that the A and B pins are correctly assigned and not swapped.
3. **Polling rate.** The default `POLLING_RATE_HZ=250` should be sufficient for most encoders. If your encoder has very fine detents, try increasing to 500.
4. **Control type.** Encoders should use `"variable_step"` (for relative movement) or `"fixed_step"` (for fixed increments). Do not use `"momentary"` for encoders.

---

### Potentiometer is jumpy or noisy

1. **Add filtering capacitor.** Solder a 100nF ceramic capacitor between the wiper pin and GND, as close to the ESP32 as possible.
2. **Check voltage.** The potentiometer must be connected between 3.3V and GND (not 5V).
3. **Adjust thresholds.** In `Config.h`, increase `MIDDLE_AXIS_THRESHOLD`, `UPPER_AXIS_THRESHOLD`, and `LOWER_AXIS_THRESHOLD` to larger values (128-256).
4. **Skip filtering (HID only).** If you need raw values, set `SKIP_ANALOG_FILTERING=1`.

---

### Selector switch sends wrong position

1. **Check selector group.** All positions of the same physical switch must share the same non-zero `group` number in `InputMapping.h`.
2. **Verify the send value.** Each position must have a unique `oride_value` (0, 1, 2, etc.) matching the DCS-BIOS control's expected values.
3. **Wiring.** For GPIO-based selectors where each position has its own pin: each position pin connects to a separate GPIO, and the common terminal goes to GND.
4. **Dwell time.** If the switch bounces between positions, increase `SELECTOR_DWELL_MS` in `Config.h` (default 250ms).

---

### TM1637 button inputs trigger randomly (ghost keys)

**Fix:** Enable advanced TM1637 input filtering:
```cpp
#define ADVANCED_TM1637_INPUT_FILTERING  1
```
Recompile and re-upload.

---

## Output Problems

### LED does not light up

1. **Check polarity.** LEDs have a specific orientation: anode (+, longer leg) toward the GPIO pin (through a resistor), cathode (-, shorter leg) to GND.
2. **Check resistor.** Use a 220-330 ohm resistor in series.
3. **Verify LEDMapping.** In the Label Creator, confirm the LED's Device is set to `GPIO` and the Port matches the physical pin.
4. **Check activeLow flag.** Some circuits (e.g., PCA9555 with active-low sinking) require `activeLow=true`. If the LED is always on when it should be off (or vice versa), toggle this flag.
5. **Test independently.** Use `TEST_LEDS=1` in `Config.h` to access the interactive LED test menu over Serial.

---

### 7-segment display shows wrong data

1. **Check SegmentMap.** The segment map file defines which DCS-BIOS data field maps to which physical display segments. Verify the segment and bit assignments.
2. **Verify DCS-BIOS control names.** The control identifier in your `DisplayMapping.cpp` must exactly match the DCS-BIOS export definition.
3. **Check CLK/DIO pins.** TM1637 and HT1622 displays need correct clock and data pin assignments in `CustomPins.h`.

---

### WS2812 LEDs show wrong colors or do not light

1. **Data pin.** Verify the `WS2812B_PIN` definition in `CustomPins.h` matches your wiring.
2. **Power supply.** WS2812 LEDs require 5V power. Do not power more than a few LEDs from the ESP32's 3.3V pin. Use an external 5V supply for strips.
3. **LED index.** Each WS2812 LED has a unique index in the strip. Verify the index in `LEDMapping.h` matches the physical position.
4. **Level shifting.** Some ESP32 boards output 3.3V logic which may not reliably drive WS2812 data lines. A logic level shifter (3.3V to 5V) on the data pin can help.

---

### Servo gauge jitters or does not move

1. **External power.** Servos draw significant current and must be powered from an external supply, not the ESP32 board. Connect servo power to an external 5V source and share GND with the ESP32.
2. **Signal pin.** Verify the servo signal pin matches the `gaugeInfo.gpio` in your `LEDMapping.h`.
3. **Pulse range.** If the servo moves to wrong positions, adjust `minPulse` and `maxPulse` in the gauge info to match your servo's specifications (typically 500-2500 microseconds).
4. **Update rate.** The default `SERVO_UPDATE_FREQ_MS=20` (50Hz) matches most hobby servos.

---

## Performance Issues

### Slow response or input lag

1. **Debug flags.** Make sure all debug flags are disabled for production. Use the Compiler Tool's **Misc Options** > **Debug / Verbose Toggles** to turn them off. Debug output significantly increases latency.
2. **Polling rate.** Check `POLLING_RATE_HZ` (default 250). Do not set it below 125.
3. **Transport mode.** USB HID has the lowest latency. WiFi adds network latency. Serial depends on baud rate and bridge software.
4. **Ring buffer tuning.** If using WiFi and experiencing lag, `DCS_UDP_RINGBUF_SIZE=32` is optimal. Larger values add latency.

---

### Display flicker

1. **Refresh rate.** Check `DISPLAY_REFRESH_RATE_HZ` (default 60). Increasing it may reduce visible flicker but uses more CPU.
2. **I2C speed.** If using PCA9555 or I2C displays, ensure `PCA_FAST_MODE=1` for 400kHz operation.
3. **CPU load.** Enable **Performance profiling** in the Compiler Tool (**Misc Options** > **Debug / Verbose Toggles**) temporarily to check if the CPU is overloaded. Look for high task utilization in the snapshots.

---

### Device resets or watchdog timeout

1. **Startup timeout.** If the device resets during boot, the startup watchdog (`STARTUP_WATCHDOG_TIMEOUT_MS=30000`) may be triggering. This usually means the label set files are corrupted or the WiFi connection is hanging.
2. **Power supply.** Insufficient power can cause random resets. Use a quality USB cable and port. If driving many LEDs or servos, use external power.
3. **Memory overflow.** S2 devices with many features enabled can run out of memory. Reduce the number of enabled features or upgrade to an S3.

---

## Quick Diagnostic Checklist

When something is not working, run through this list:

```
+-----------------------------------------------------------------------+
|  DIAGNOSTIC CHECKLIST                                                  |
+-----------------------------------------------------------------------+
|  [ ] Only ONE transport flag is set (via Compiler Tool Role/Transport)|
|  [ ] All debug flags are off (Compiler Tool > Misc > Debug Toggles)  |
|  [ ] Label set is generated and set as active (Label Creator)         |
|  [ ] Board type matches physical hardware (S2, S3, Classic, etc.)     |
|  [ ] USB cable is a data cable (not charge-only)                      |
|  [ ] USB CDC On Boot is Disabled                                      |
|  [ ] USB Mode is USB-OTG/TinyUSB (S3/P4 only, for USB transport)     |
|  [ ] DCS-BIOS is installed in DCS World                               |
|  [ ] A mission is loaded and an aircraft is in the cockpit            |
|  [ ] HID Manager is running (USB transport only)                      |
|  [ ] WiFi is 2.4 GHz WPA2-PSK (WiFi transport only)                  |
|  [ ] Firewall allows UDP 5010 and 7778 (WiFi transport only)         |
+-----------------------------------------------------------------------+
```

---

## See Also

- [Config.h Reference](Config.md) -- All settings with descriptions
- [Transport Modes](Transport-Modes.md) -- Transport-specific setup and verification
- [FAQ](FAQ.md) -- Common questions answered
- [Tools Overview](../Tools/README.md) -- How to use the Python tools

---

*CockpitOS Troubleshooting Guide | Last updated: February 2026*
