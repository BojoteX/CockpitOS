# How To: Use Multiple Aircraft

CockpitOS supports multiple aircraft by using separate **label sets** -- each label set defines the DCS-BIOS mappings, pin assignments, and display configurations for one aircraft module. The same physical panel hardware can work with different aircraft by switching the active label set, recompiling, and uploading.

---

## What Multi-Aircraft Means

A label set is a folder in `src/LABELS/` that contains all the configuration files for one panel + aircraft combination:

```
src/LABELS/
  LABEL_SET_ALR67/          <-- F/A-18C ALR-67 panel
    CustomPins.h
    InputMapping.h
    LEDMapping.h
    DisplayMapping.h
    DisplayMapping.cpp
    LabelSetConfig.h
    CT_Display.h
    IFEI_SegmentMap.h       (if applicable)

  LABEL_SET_CUSTOM_RIGHT/   <-- F/A-18C right console
    CustomPins.h
    InputMapping.h
    LEDMapping.h
    ...

  LABEL_SET_KY58/           <-- F/A-18C KY-58 panel
    CustomPins.h
    InputMapping.h
    LEDMapping.h
    ...

  active_set.h              <-- Points to the currently active label set
```

Each label set maps the same physical GPIO pins to different DCS-BIOS commands. The physical wiring does not change -- only the firmware's interpretation of the inputs and outputs changes.

---

## Step 1: Understand the Label Set Structure

Each label set folder contains these files:

| File | Purpose |
|---|---|
| `LabelSetConfig.h` | Label set name, USB PID, feature flags, `HAS_*` macro |
| `CustomPins.h` | Pin assignments for this label set (GPIO, SPI, I2C, HC165, RS485) |
| `InputMapping.h` | Input hardware configuration (buttons, switches, encoders, selectors) |
| `LEDMapping.h` | Output hardware configuration (LEDs, WS2812, gauges) |
| `DisplayMapping.h` | Display field declarations |
| `DisplayMapping.cpp` | Display field definitions and rendering functions |
| `CT_Display.h` | DCS-BIOS string buffer registrations for displays |
| `*_SegmentMap.h` | HT1622 segment maps (if applicable) |

---

## Step 2: Create a Label Set for Each Aircraft

### Using Label Creator

1. Open Label Creator (`LabelCreator-START.py`)
2. Select **Create New**
3. Choose the target aircraft module (e.g., "F/A-18C Hornet")
4. Name the label set (e.g., "MY_F18_LEFT_PANEL")
5. Select the hardware panels and controls you want to include
6. The Label Creator generates all the configuration files automatically
7. Repeat for each aircraft you want to support

### Example: Same Panel, Two Aircraft

Suppose you have a left console panel with toggle switches and LEDs. You fly both the F/A-18C and the F-16C.

```
src/LABELS/
  LABEL_SET_F18_LEFT/       <-- F/A-18C mappings
    InputMapping.h           (maps GPIO 5 -> "MASTER_ARM_SW")
    LEDMapping.h             (maps GPIO 12 -> "MASTER_CAUTION_LT")
    CustomPins.h
    ...

  LABEL_SET_F16_LEFT/       <-- F-16C mappings
    InputMapping.h           (maps GPIO 5 -> "MASTER_ARM_SW")
    LEDMapping.h             (maps GPIO 12 -> "MASTER_CAUTION")
    CustomPins.h
    ...
```

The physical toggle on GPIO 5 does the same thing (master arm) in both aircraft, but the DCS-BIOS command names differ. The LED on GPIO 12 drives a master caution light in both cases, but the DCS-BIOS export names differ.

---

## Step 3: Switch Between Label Sets

### Using the Compiler Tool

1. Open the Compiler Tool (`CockpitOS-START.py`)
2. Select **Choose Label Set**
3. Pick the label set for the aircraft you want to fly
4. Compile and upload

The Compiler Tool writes the selection to `src/LABELS/active_set.h`:

```cpp
// active_set.h
#pragma once
#define LABEL_SET CUSTOM_RIGHT
```

This tells the build system which label set folder to include.

### Using Label Creator

1. Open Label Creator
2. Select **Modify** to open an existing label set
3. Make any changes needed
4. Save
5. Use the Compiler Tool to compile and upload

---

## Step 4: Shared vs. Different Pin Assignments

### Shared Hardware (Most Common)

When the same physical panel is used for multiple aircraft, the GPIO wiring stays the same. Only the DCS-BIOS command mappings change:

```cpp
// F/A-18C label set - InputMapping.h
{ "MASTER_ARM_SW_ON",  "GPIO", PIN(5), -1, -1, "MASTER_ARM_SW", 1, "selector", 1 },

// F-16C label set - InputMapping.h
{ "MASTER_ARM_SW_ON",  "GPIO", PIN(5), -1, -1, "MASTER_ARM_SW", 1, "selector", 1 },
```

Same pin, same control type -- different DCS-BIOS context.

### Different Pin Assignments

If your hardware differs between aircraft configurations (e.g., different I2C expander addresses, different shift register configurations), each label set can have its own `CustomPins.h`:

```cpp
// F/A-18C CustomPins.h
#define ENABLE_PCA9555     1
#define SDA_PIN            PIN(8)
#define SCL_PIN            PIN(9)

// AH-64D CustomPins.h (different hardware)
#define ENABLE_PCA9555     0
#define HC165_BITS         32
#define HC165_CONTROLLER_PL  PIN(35)
#define HC165_CONTROLLER_CP  PIN(34)
#define HC165_CONTROLLER_QH  PIN(33)
```

---

## Step 5: Feature Flags per Label Set

Each label set's `CustomPins.h` controls which hardware features are enabled:

```cpp
// CustomPins.h
#define ENABLE_TFT_GAUGES   1    // Enable TFT gauge rendering
#define ENABLE_PCA9555       1    // Enable PCA9555 I2C expanders
```

And `LabelSetConfig.h` defines the label set identity:

```cpp
// LabelSetConfig.h
#define LABEL_SET_NAME       "ALR67"
#define LABEL_SET_FULLNAME   "ALR-67 RWR Panel"
#define HAS_ALR67                              // Compile guard macro
#define AUTOGEN_USB_PID      0x522B            // Unique USB PID (auto-generated)
#define HAS_HID_MODE_SELECTOR 0
```

The `HAS_*` macro is what custom panel code uses in its `#if defined(...)` guards.

---

## Step 6: Recompile and Upload After Switching

After switching label sets, you **must recompile and upload** the firmware. The label set selection is a compile-time decision -- it determines which InputMapping.h, LEDMapping.h, CustomPins.h, and display files are included in the build.

### Workflow Summary

```
1. Open Compiler Tool (or Label Creator)
2. Select the label set for your target aircraft
3. Compile
4. Upload to ESP32
5. Fly
```

If you frequently switch aircraft, keep the Compiler Tool open. The compile + upload cycle typically takes 30-60 seconds.

---

## Step 7: Future -- Runtime Aircraft Switching

Runtime aircraft switching (changing the active label set without recompiling) is **not yet implemented**. Currently, switching aircraft requires a recompile and re-upload.

This is a planned feature for a future release. The architecture is designed to support it -- label sets are self-contained, and the panel registry can be reconfigured at runtime -- but the implementation is not yet complete.

---

## Tips

- **Name your label sets clearly** -- include the aircraft and panel name (e.g., `LABEL_SET_F18_LEFT_CONSOLE`)
- **Keep CustomPins.h consistent** -- if the same physical hardware is used across aircraft, keep the pin definitions the same to avoid wiring changes
- **Use the Label Creator for editing** -- it validates DCS-BIOS command names against the aircraft module's export list, preventing typos
- **Each label set gets a unique USB PID** -- this is auto-generated so Windows recognizes each configuration as a distinct HID device

---

## Troubleshooting

| Symptom | Cause | Fix |
|---|---|---|
| Wrong aircraft controls | Active label set does not match the aircraft you are flying | Switch label sets in the Compiler Tool and re-upload |
| Compilation error about missing `HAS_*` | Label set not properly selected, or LabelSetConfig.h missing | Verify `active_set.h` points to the correct label set; check that LabelSetConfig.h exists in the label set folder |
| USB device not recognized after switching | Different USB PID | Windows may need to re-enumerate. Unplug and re-plug the ESP32. |
| Some controls work but others do not | InputMapping.h for this label set is incomplete | Open Label Creator, select the label set, and verify all controls are mapped |

---

*See also: [Label Creator Tool](../Tools/Label-Creator.md) | [Compiler Tool](../Tools/Compiler-Tool.md) | [Config Reference](../Reference/Config.md)*
