# Changelog

## v1.2.1 — 2026-02-28

### Added
- Add selectable LED output for RS485 Master debug sketch
- Add audit plan with fix details and false-positive documentation
- Add label set audit findings: selector hash collisions (H6) and TFT dead code (M15)
- Add critical-path firmware code audit report
- Implement validated code audit findings for performance and robustness
- Add comprehensive code audit report with implementation guide
- Add comprehensive code fix verification audit
- Add Config.h allowed-edits section to CLAUDE.md
- Add verified bug and enhancement items to TODO
- Add comprehensive bug report from code review

### Improved
- Reduced memory settings
- Misc cleanup
- RS485 Master Tests
- Modifying Master Arduino
- Clean up
- Rewrite audit report after validation pass â€” retract 21 of 25 findings
- Clean up temp files
- Custom Pin editor changes
- Re-apply all firmware changes from code audit
- Revert all firmware changes from code audit
- Clear screen after interface selection in all Debug Tools
- Update audit report with implementation outcomes and skip justifications
- Misc bug fixes
- Compile all 10 label sets for S2/USB hardware testing
- Revert subscription idempotency â€” subscriptions already run once at boot
- Revert code edits and keep audit as documentation-only proposals
- Regenerate all 18 label sets for S2 compile validation
- Disable WiFi debug and extended debug for S2 compatibility
- Regenerate all 18 label sets in batch mode, restore TEST_ONLY as active
- Expand bug report with additional findings from late-arriving review agents
- Switch active label set to MAIN for bug hunt build

### Fixed
- Fixed HID_Manager 0xFF padding
- Fix CT_METADATA regression: update g_prevValues to prevent per-frame re-fire
- Fix pendingUpdates ordering: defer g_prevValues until queue succeeds
- Fix compiler tool ignoring external Config.h changes
- Fix PCA9555 cache corruption for non-standard addresses and WiFi debug blank lines
- Fix subscription slot exhaustion on mission restart and hot-loop volatile write
- Fix documentation errors found in full audit of 48 docs
- Fix 10 low-risk bugs: RadarAlt deinit order, IFEI overflow guard, wifi timeout, strcmp, dead code cleanup
- Fix critical PCA9555 OOB write and 18 additional bug fixes
- Fix MATRIX strobeCount off-by-one, widen CoverGateDef delays, add null guard
- Fix HID axis arrays: must be pin-indexed [64], not axis-indexed [16]
- Fix generator exit code bug and HID axis array sizing

## v1.2.0 — 2026-02-23

### Added
- Implemented Magnetics Logic

### Improved
- Bulk Command Sender
- Magnetic Device Handling
- Custom Release field and

## v1.1.9 — 2026-02-22

- Release

## v1.1.5 — 2026-02-22

### Added
- Create release.yml
- Added PR template

### Improved
- Release flows updated
- Release workflows updated
- Updated Dwelling Logic
- Update generator_core.py
- Updated CLAUDE.md
- Updated CHANGELOG.md
- Update TODO.md
- Update ci-build.yml
- Misc setup environment changes

## v1.1.4 — 2026-02-22

### Added
- Create ARCHITECTURE.md
- Create transcript.md
- Add paced RS485 master and guarded slave variants
- Implementing dumb mode for RS485
- Added DCSBIOS Lib in Tools
- Support for multiple network interfaces
- Added new label set for Analogs

### Improved
- Updated DCSBIOS JSON Files
- Self-aligned sync implementation
- ServoTest and Doc Updates
- Updated docs
- Cleanup
- Docs updates
- Update ci-build.yml
- Updated Docs
- Misc fixes to label creator
- Updated label sets
- Update ci-build.yml
- Automated tests
- Update ci-build.yml
- Update ci-build.yml
- Automation scripts for Label Creation
- Label Sets Improvements
- Covergate & Latched buttons
- Claude Helper
- Docs and Lua tool installed
- Label Creator tool
- Label Creator Tool
- Label Creator Tool
- Label Creator Tool
- Perf improvements
- Update RS485_TECHNICAL_DEEP_DIVE.md
- RS485 Implementation
- Perf Improvements
- HID Manager fix
- Perf improvements
- Perf improvements
- Perf improvements
- Cleanup and minor refactor
- Label Creator implementation
- RS485 Fixes
- Compiler changes
- RS485 Fixes and Wizard
- Compiler tool
- Update README.md
- RS485 Fixes
- RS485 Fixes
- Housekeeping
- Misc RS485 fixes
- RS485 Fixes
- Updated samples
- Moved libs to Dev
- Uploaded sample binary Waveshare Gauge
- Uploaded Waveshare demo
- RS485 Testing
- RS485 Examples
- RS485 samples
- RS485 Testing
- Refactored RS485 Slave and Master
- RS485 Samples
- Configured IFEI binary for Jesus
- Updated tools for RS485
- RS485 Sample updates
- RS485 debugging
- RS485 Debugging
- RS485 Stand alone scripts
- Updated RS485 Samples
- Misc RS485 Fixes
- Maintenance release
- RS485 Fixes
- RS485 ESP32 implementation
- RS485 Tweaks
- Misc RS485
- RS485 Fixes
- RS485 Implementation
- Allow Wi-Fi with P4 devices
- Updated Pins for TEST Only Label
- P4 tests
- Renamed Python Tool scripts
- Moved bootloader functions outside RingBuffer
- Updated defaults
- Updated credentials
- Startup watchdog
- Config.h updates

### Fixed
- Fixed paths
- Fix Setup Issue with downgrading
- Fixed RS485 Master
- Fixed RS485 Master
- Fix RS485 Slave protocol for Arduino/CockpitOS master compatibility
- Fixed Analog gauges not working

## v1.1.3 — 2026-01-18

### Improved
- Sample binaries
- Update TODO.txt
- docs: continue documentation audit
- docs: fix ENABLE_TFT_GAUGES and ENABLE_PCA9555 location
- docs: audit and fix documentation accuracy
- docs: update Debug Tools file names in PYTHON_TOOLS.md
- Renamed Python Tools
- docs: add bootloader_tool.py and CustomPins.h documentation

## v1.1.2 — 2026-01-18

### Added
- Added Custom Pins to Label Sets
- Create favicon.ico
- Add index2.html with styling fixes and font discipline

### Improved
- Output refactor
- Calibration tweaks to axes
- Updated bootloader tool
- updated image
- Updated logos
- updated logos
- Update TODO.txt
- docs: add technical architecture section for developers
- docs: complete documentation with remaining advanced features
- docs: enhance documentation with missing features and new guides
- Update TODO.txt
- Update README.md
- Update index.html
- Misc updates
- Update README.md
- Updated website HTML
- Updated logos
- Updated logos
- Update README.md
- Updated logos
- Update CockpitOS_logo.png
- updated logos
- Updated logo
- Update README.md
- Capture Script uploaded
- Update CockpitOS_First_Panel_30_Minutes.md
- Update CockpitOS_First_Panel_30_Minutes.md
- Update CockpitOS_First_Panel_30_Minutes.md
- Update README.md
- Updated docs
- Update LLM instruction set
- Update README.md
- Update README.md
- Update COCKPITOS_LLM_INSTRUCTION_SET.txt

### Fixed
- Fixed CMWS ticks and small arrows operation

## v1.1.1 — 2026-01-10

### Improved
- Removed backup directory causing compile errors

## v1.1.0 — 2026-01-10

### Added
- Additional documentation
- Added hardware documentation
- Adding documentation
- Create TRANSPORT_MODES.md
- Implemented CMWS Display
- Added documentation

### Improved
- Code cleaning
- Updated docs
- PanelKind auto-generator
- Update README.md
- Update README.md
- Update README.md
- Updated docs
- More docs
- updated docs
- updated docs
- Note about a BUG
- Delete README.md.OLD
- Updated docs
- updated docs
- Updated docs
- Housekeeping
- Misc changes
- Code cleaning
- Code cleanup
- Misc changes
- Code cleaning

## v1.0.7 — 2026-01-06

### Added
- Added multiple guards

### Improved
- Code cleaning
- Mis changes
- Custom fonts for TFTs
- Misc improvements
- Misc Improvements
- Improved ESP32 Classic support
- Misc performance optimizations
- Refactored HID Manager
- Reverted to old HID Manager version
- HID Manager Uniform name change
- Updated HID Controller
- Center deadzone for axis

## v1.0.6 — 2025-12-25

### Improved
- Auto-calibration logic for axes
- Updated README.md for HID Manager
- HID Manager fixes

## v1.0.5 — 2025-12-22

### Improved
- Refactored Axis logic and HID manager script

## v1.0.4 — 2025-11-19

### Improved
- Fully refactored TM1637 to make it aircraft agnostic
- TM1637 refactor

### Fixed
- Fixed WiFi Debug bug. Also refactored TM1637 Input handling

## v1.0.3 — 2025-10-31

### Improved
- Latest stable release
- Misc fixes
- Minor README.md update
- Updated README.md

## v1.0.2 — 2025-09-26

### Added
- Additional support for ESP32 devices

### Improved
- README.md cleaning
- Updated README.md file
- Release friendly update
- Misc

### Fixed
- Fixed analogs accuracy

## v1.0.1 — 2025-09-14

### Added
- Added support for ALL ESP32 devices
- Added support for entire ESP32 family

### Improved
- Updated README.md
- Misc fixes for Arduino Core 2.x compatibility
- Cleanup
- Removed BLE support
- Misc changes
- Misc changes
- Clean up

## v1.0.0 — 2025-09-02

### Added
- Implemented custom front right panel
- Implemented new panel template
- Implemented Test Only Panel template
- Added TEST ONLY template for panel dev
- Implementing Radar Altimeter
- Implemented TFT Gauges with LovyanGFX
- Implemented Right Panel Controller
- Added S3 Pins
- Added documentation

### Improved
- Version 1.0 released
- Console is now default mode for HID manager
- Updated README.md
- HID Manager refactor
- Working on BT functionality on S3 and C3 devices
- Clean up and minor fixes
- Supressed button sync on init
- Refactor cont
- More tweaks and refactoring
- Complete panel refactoring
- Refactor cleanup
- More Panel refactoring
- Refactoring panel files
- Misc fixes to Test Only template
- Enhancements to auto-generator
- TFT Gauges optimization
- More TFT improvements
- Improved TFT gauges
- Updated logo
- Updated all README.md files
- Updated README
- Update README.md
- TFT Implementation
- Misc improvements
- Misc changes
- Update TODO.txt
- Panel sync logic refactor
- More robust panel sync logic
- Misc improvements
- HID Manager Refactor
- Panel sync logic fix
- Code cleanup
- Misc changes
- Misc changes
- Final version
- Initial commit
- Initial commit

### Fixed
- Fixed IFEI ghosting
- Fixed auto-generator selector group bug
