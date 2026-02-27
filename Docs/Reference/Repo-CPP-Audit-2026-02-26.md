# Repo-wide C++ Audit (Performance + Correctness)

Date: 2026-02-26

## Scope
- Scanned **39** `.cpp` files in the repository.
- Reference guidance consulted first: `Docs/LLM/CockpitOS-GPT-Index.md` and `Docs/LLM/CockpitOS-LLM-Reference.md`.
- **Documentation-only report:** this file lists findings and proposed fixes. No source-code implementation is included in this report.

## Files scanned
- `Mappings.cpp`
- `lib/CUtils/src/CUtils.cpp`
- `lib/CUtils/src/internal/AnalogG.cpp`
- `lib/CUtils/src/internal/GN1640.cpp`
- `lib/CUtils/src/internal/GPIO.cpp`
- `lib/CUtils/src/internal/HC165.cpp`
- `lib/CUtils/src/internal/HT1622.cpp`
- `lib/CUtils/src/internal/MatrixRotary.cpp`
- `lib/CUtils/src/internal/PCA9555.cpp`
- `lib/CUtils/src/internal/RS485Master.cpp`
- `lib/CUtils/src/internal/RS485Slave.cpp`
- `lib/CUtils/src/internal/TM1637.cpp`
- `lib/CUtils/src/internal/WS2812.cpp`
- `lib/DCS-BIOS/src/internal/Protocol.cpp`
- `src/Core/Bootloader.cpp`
- `src/Core/CoverGate.cpp`
- `src/Core/DCSBIOSBridge.cpp`
- `src/Core/HIDManager.cpp`
- `src/Core/InputControl.cpp`
- `src/Core/LEDControl.cpp`
- `src/Core/PanelRegistry.cpp`
- `src/Core/PerfMonitor.cpp`
- `src/Core/RingBuffer.cpp`
- `src/Core/WiFiDebug.cpp`
- `src/Core/debugPrint.cpp`
- `src/LABELS/LABEL_SET_TEST_ONLY/CT_Display.cpp`
- `src/LABELS/LABEL_SET_TEST_ONLY/DisplayMapping.cpp`
- `src/Panels/Generic.cpp`
- `src/Panels/IFEIPanel.cpp`
- `src/Panels/ServoTest.cpp`
- `src/Panels/TFT_Display_CMWS.cpp`
- `src/Panels/TFT_Gauges_Battery.cpp`
- `src/Panels/TFT_Gauges_BrakePress.cpp`
- `src/Panels/TFT_Gauges_CabPressTest.cpp`
- `src/Panels/TFT_Gauges_CabinPressure.cpp`
- `src/Panels/TFT_Gauges_HydPress.cpp`
- `src/Panels/TFT_Gauges_RadarAlt.cpp`
- `src/Panels/TestPanel.cpp`
- `src/Panels/WingFold.cpp`

## Key finding and proposed change (not implemented)
1. **Duplicate subscription registration risk in mission re-sync paths** (`src/Core/DCSBIOSBridge.cpp`).
   - Repeated calls to `subscribeToDisplayChange`, `subscribeToMetadataChange`, `subscribeToSelectorChange`, and `subscribeToLedChange` can register duplicate `(label, callback)` pairs.
   - In long-running sessions with mission restarts, this may create redundant callback work per frame and exhaust subscription slots earlier.
   - **Proposed fix:** make subscription functions idempotent by checking for an existing `(label, callback)` pair before appending.

## Additional proposed optimization (not implemented)
1. **Startup mode logging in `CockpitOS.ino`:** evaluate `isModeSelectorDCS()` once and emit a single mode log message to avoid duplicated branch logging.
2. **Main loop flag write in `CockpitOS.ino`:** set `mainLoopStarted` only once on first loop entry, avoiding repeated writes in hot loop path.

## Commands used during the audit
- `find . -type f -name "*.cpp" | sort`
- `sed -n` / `rg -n` spot checks for loop paths, panel init/re-sync, and subscription registration.
- `python3 -m compileall -q .` (sanity check for Python tooling unaffected by C++ review).
