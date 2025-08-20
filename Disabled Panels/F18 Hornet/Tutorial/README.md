# TUTORIAL: How to Add a Custom Panel to CockpitOS

This guide explains, step by step, how to add a new panel (such as a TFT gauge or annunciator) to CockpitOS. Follow these instructions in order—do not skip steps.

---

## PHASE 1: Label Set and Data Generation

1. **Choose a Name for Your Panel ("Label Set")**
    - Example: `LABEL_SET_BATTERY_GAUGE`
    - This name will be used everywhere to reference your panel.

2. **Create a Directory for Your Label Set**
    - Inside the `LABELS/` directory, create a new folder named exactly as your label set (e.g., `LABELS/LABEL_SET_BATTERY_GAUGE`).

3. **Copy Generation Files into Your Label Set Directory**
    - Copy `display_gen.py`, `generate_data.py`, and the target aircraft JSON file (e.g., `FA-18C_hornet.json`) into your new directory.

4. **Run the Data Generator**
    - Open a terminal in your new label set directory.
    - Run:  
      ```
      python generate_data.py
      ```
    - **Note:** On the first run, it will generate a `selected_panels.txt` file listing all available panel clusters as commented-out entries.

5. **Select Components Needed for Your Panel**
    - Open `selected_panels.txt` in a text editor.
    - Uncomment (remove the `#`) from the **cluster(s)** that include the parts you need for your new panel.
      - *Tip:* To find the correct cluster, search your aircraft JSON for the components (e.g., `"battery"`, `"Utility Bus"`, etc.).
      - For the Battery Gauge, you’ll want both the "U" and "E" needles, which are usually in the "Electrical Power Panel" cluster.

6. **Save and Re-run the Data Generator**
    - Save your changes to `selected_panels.txt`.
    - Run:  
      ```
      python generate_data.py
      ```
    - This generates all code/data files for your new panel.

7. **Verify Files Were Generated**
    - Check the directory. You should now see new `.h` and `.cpp` files, and possibly a mapping header for your panel.
    - These files are ready to use in CockpitOS.

---

## PHASE 2: Panel Driver/Hardware Setup

1. **Decide How Your Panel Will Be Driven**
    - If your panel needs a new hardware driver, add its implementation to `lib/CUtils/src/internal/` as a new `.cpp` file.
    - For most panels (including TFT gauges), you can use existing drivers and do not need a new one.

2. **(If needed) Register New Drivers in CUtils**
    - In `CUtils.cpp`, `#include` your new driver file.
    - In `CUtils.h`, declare the driver functions you want available globally.

3. **Enable Your Label Set in `Config.h`**
    - Open `Config.h`.
    - **Comment out all other `LABEL_SET_` defines.**
    - **Uncomment/add**:  
      ```cpp
      #define LABEL_SET_BATTERY_GAUGE
      ```
    - Only your new label set should be active for this build.

---

## PHASE 3: Mappings Integration

### A. Update `Mappings.h`

1. **Add Preprocessor Logic for Your Label Set**
    - Follow the format of existing label sets.
    - Example:
      ```cpp
      #elif defined(LABEL_SET_BATTERY_GAUGE)
        // Any specific defines or includes for your panel
      ```

### B. Update `Mappings.cpp`

1. **Declare a Presence Flag and Include Your Panel Header**
    - Add at the top:
      ```cpp
      bool hasTFTGauge = false;
      #include "src/TFT_Gauges.h"
      ```

2. **Enable Your Panel in `initMappings()`**
    - In the preprocessor label set selection logic, add:
      ```cpp
      #elif defined(LABEL_SET_BATTERY_GAUGE)
        hasTFTGauge = true;
      ```

3. **Initialize the Panel in `initializePanels()`**
    - Add:
      ```cpp
      if (hasTFTGauge) BatteryGauge_init();
      ```

4. **Add Panel Logic to `panelLoop()`**
    - Add:
      ```cpp
      if (hasTFTGauge) BatteryGauge_loop();
      ```

---

## PHASE 4: Panel Logic Implementation

1. **Write Your Panel’s Implementation**
    - Create your panel’s logic in `src/Panels/TFT_Gauges.cpp`.
    - Use the auto-generated data/headers from Phase 1.
    - Follow CockpitOS’s init/loop/deinit function conventions.
    - (If needed, add a header: `TFT_Gauges.h`.)

2. **Test the Integration**
    - Build and upload your firmware.
    - Your panel will now be automatically included and run based on your label set selection.

---

## Summary (Cheat Sheet):

- **PHASE 1:** Create label set, uncomment clusters in `selected_panels.txt`, re-generate data.
- **PHASE 2:** (If needed) Add or reference drivers, enable only your label set in `Config.h`.
- **PHASE 3:** Update `Mappings.h`/`.cpp` for new panel flag, header, and init/loop logic.
- **PHASE 4:** Implement panel logic in its own `.cpp`, using the auto-generated mappings.

---

**Result:**  
Your new panel is now fully integrated, with all mappings, code, and data managed by the CockpitOS infrastructure—just like the built-in panels.

