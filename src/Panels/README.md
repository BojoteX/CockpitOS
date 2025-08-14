# Panel Implementation & Mappings (CockpitOS)

This directory contains **all panel-specific implementation files** and the `Mappings.cpp/h` files that centralize panel management and integration. do NOT mix panels here from different Aircraft. If building firmware for the HORNET for example, just keep HORNET panels here. If building for a DIFFERENT aircraft, simply MOVE the panels OUT of this directory into the DISABLED PANELS directory in the root folder as they will NOT compile there

---

## What Are Panel Files?

- Each panel file (e.g., `ECMPanel.cpp`, `IFEIPanel.cpp`, `ALR67Panel.cpp`, etc.) implements the *minimal, hardware-accurate logic* for one cockpit device or panel.
- **Do not reinvent panel logic!**  
  *Always start from an existing panel as a template—copy, rename, and adjust only what's necessary for your new device.*

### Panel Implementation Best Practices

- **All device reads/writes go through CUtils**  
  e.g., `readPCA9555`, `MatrixRotary_readPattern`, `tm1637_readKeys`, etc.
- **All logical actions, button/rotary/selector updates go through HIDManager**  
  e.g., `HIDManager_setNamedButton`, `HIDManager_toggleIfPressed`, `HIDManager_moveAxis`.
- **Never implement drivers or I/O in panel files**—only use the centralized API!
- **IFEIPanel.cpp** is the prime example for complex panels:  
  - Manages both button and display logic,
  - Subscribes to display field/LED/selector events from `DCSBIOSBridge`,
  - Uses dispatcher pattern to integrate with auto-generated display and field rendering code.
- **State caching** is allowed, but only for tracking previous values for edge detection.

---

## Mappings.cpp/h: Panel Integration Layer

`Mappings.cpp/h` are **not panel code**—they are the "glue" that binds all panels together.

- **Centralizes all initialization and polling:**  
  - Calls every `Panel_init` and `Panel_loop` method, gated by boolean presence flags.
- **Panel presence is controlled by booleans** (`hasIFEI`, `hasALR67`, etc.), which are determined based on label set and runtime detection.
- **No feature control via scattered defines!**  
  All inclusion/exclusion is managed via the current label set and these runtime flags.
- **Panel-agnostic:** The main firmware loop never needs to know which panels are present or how to call them—it only ever calls the integration routines from Mappings.
- **Mappings.h** controls which label set and generated mappings are used (via `LABEL_SET_XXXX` and auto-generated includes).

---

## Adding or Extending Panels

1. **Copy an existing panel .cpp/.h** (start with the closest match).
2. Adjust only the input mappings, edge logic, and hardware setup code required for your new hardware.
3. Add your new panel to `Mappings.cpp` in both the `_init` and `_loop` routines, using a new boolean flag if needed.
4. Make sure your panel is present in the relevant label set and that the boolean gets set in Mappings.h.
5. All hardware interaction is through CUtils; all logic through HIDManager.  
   Do **not** add device drivers, global flags, or duplicate logic.
6. For advanced panels (displays):  
   - Integrate with the auto-generated `CT_Display` and `DisplayMapping` using dispatcher callbacks and subscription logic as shown in IFEIPanel.cpp.

---

## Examples (see source for full details)

- **ALR67Panel.cpp**:  
  Rotary position scanning (matrix strobe), analog axes, button edge detection, HIDManager dispatch.
- **ECMPanel.cpp**:  
  Multi-switch group detection (via PCA9555), selector logic, deferred HID reporting.
- **IFEIPanel.cpp**:  
  Advanced: Button input (shift register), backlight logic, display field subscription/dispatch, field rendering via auto-generated table.
- **Annunciators (Left/Right)**:  
  Button state, guarded switch logic (covers), multi-sample debouncing, all via TM1637 API and HIDManager.

---

## Important Patterns

- **Panel files are NOT the place for protocol or HID/gamepad logic.**
- All group/selector/toggle logic is *centralized*.
- No dynamic allocation or custom main loops in panel code—just hardware polling and edge/event dispatch.
- All state (e.g., previous button states) is local to the panel for clarity and maintainability.

---

For more on integrating new panels, study the code comments in these files, and see `Mappings.cpp/h` for how all panels are unified.

