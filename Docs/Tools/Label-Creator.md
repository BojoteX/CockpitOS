# Label Creator

The Label Creator is a full-featured terminal UI for creating, browsing, modifying, and deleting label sets. It handles all the file generation and editing so you never need to touch mapping files by hand.

**Entry point:** `LabelCreator-START.py`

## How to Run

Double-click `LabelCreator-START.py` in the CockpitOS root folder, or run from a terminal:

```
python LabelCreator-START.py
```

## Main Menu

The main screen shows a status summary (number of label sets, active label set, aircraft) and these options:

| Option | Description |
|--------|-------------|
| **Create New Label Set** | Full guided flow: name, aircraft, panels, generate |
| **Modify Label Set** | Browse existing sets and edit their contents |
| **Auto-Generate Label Set** | Pick a set and re-run code generation |
| **Delete Label Set** | Remove a label set (with double confirmation) |
| **Switch Tool** | Jump to the Compiler Tool or Setup Tool |

## Creating a Label Set

The create flow walks you through three steps:

### Step 1: Name Your Label Set

Enter a name for the new label set. The prefix `LABEL_SET_` is added automatically. Names are sanitized: spaces and special characters become underscores, and the tool enforces a minimum of 2 characters. The tool shows the adjusted name if any changes were made.

### Step 2: Select Aircraft and Panels

After naming, the tool runs the aircraft selector. Aircraft definitions come from centralized JSON files in `src/LABELS/_core/aircraft/`. Each aircraft JSON defines the available cockpit panels and their input/output mappings.

Once an aircraft is selected, you enter the **panel selector** -- a toggle-list UI where you check or uncheck the panels you want included in this label set. Selected panels determine which inputs, LEDs, and displays appear in the generated mapping files.

### Step 3: Generate

The tool runs `generate_data.py` in batch mode (no manual pauses) and validates the output. It checks for the expected files:

- `InputMapping.h`
- `LEDMapping.h`
- `DCSBIOSBridgeData.h`
- `LabelSetConfig.h`

Each file's existence and size are reported. On success, the tool transitions directly into the Modify screen for your new label set.

## Modifying a Label Set

The Modify screen is where you spend most of your time. It is the heart of the Label Creator.

### Header

The top of the screen shows:
- **Device Name** -- The human-readable name for this panel (shown in DCS and the HID Manager)
- **Aircraft** -- Which aircraft this label set targets

### Menu Sections

The Modify screen menu adapts based on the current state. When files have been generated and panels are selected, you see the full menu:

#### Panels

| Option | Description |
|--------|-------------|
| **Select / deselect panels** | Opens the toggle-list panel selector. Shows the current count (e.g., "3 selected"). After changing panels, regeneration runs automatically. |
| **Generate & Set as Default** | Regenerates all files and sets this label set as the active one for compilation. Shows "(active)" or "(not active)" status. |

#### LEDs & Controls (Wiring)

| Option | Description |
|--------|-------------|
| **Edit Inputs** | Opens the Input Editor for `InputMapping.h`. Shows wiring progress (e.g., "12/45 wired"). |
| **Edit Outputs (LEDs)** | Opens the LED Editor for `LEDMapping.h`. Shows wiring progress. |

#### Displays

| Option | Description |
|--------|-------------|
| **Segment Maps** | Manages `*_SegmentMap.h` files for your display devices. Shows device count. |
| **Edit Displays** | Opens the Display Editor for `DisplayMapping.cpp`. Only shown when display entries exist. Shows configuration progress. |

#### Misc Options

| Option | Description |
|--------|-------------|
| **Device Name** | Set or change the `LABEL_SET_FULLNAME` (max 32 characters) |
| **HID Mode Selector** | Toggle whether this device includes the HID mode selector. Shows ON/OFF status. |
| **Edit Custom Pins** | Opens `CustomPins.h` in your default text editor |
| **RESET LABEL SET** | Wipes generated files and lets you pick a new aircraft |

The menu remembers your cursor position -- after performing an action, you return to the same menu item you were on.

When no panels have been selected yet, the menu shows a simplified view with just the panel selector and reset option, along with guidance text.

## Input Editor

The Input Editor presents a scrollable list of all input mappings from `InputMapping.h`.

For each input record, you can edit:

| Field | Description |
|-------|-------------|
| **Source** | The input device type: `GPIO`, `PCA9555`, `HC165`, `TM1637`, `MATRIX`, or `NONE` |
| **Port** | Pin number, PCA address (hex), or shift register position |
| **Bit** | Bit index within the port |
| **Control Type** | The DCS-BIOS control type |

Key features:
- **Scrollable list** with the current record highlighted
- **Source picker** with descriptions of each input type
- **Field validation** -- PCA addresses must be valid hex (e.g., `PCA_0x20`), bit ranges are checked against the device type
- **Selector group awareness** -- Records in the same selector group are visually grouped
- **Help system** -- Press `?` at any time for contextual help about the current field
- **Terminal-width-responsive columns** -- The display adapts to your terminal width

## LED Editor

The LED Editor presents a scrollable list of all LED mappings from `LEDMapping.h`.

For each LED record, you can edit:

| Field | Description |
|-------|-------------|
| **Device** | The output device type |
| **Info fields** | Device-specific configuration (pin, address, index, etc.) |
| **DIM** | Whether this LED supports dimming |
| **ActiveLow** | Whether the LED logic is inverted |

Supported device types:

| Device | Description |
|--------|-------------|
| `GPIO` | Direct ESP32 pin connection |
| `PCA9555` | I2C I/O expander |
| `TM1637` | 7-segment display driver (used for LED control) |
| `GN1640T` | LED driver IC |
| `WS2812` | Addressable RGB LED strip |
| `GAUGE` | Analog gauge output |

Each device type shows tailored field guidance when selected. For example, selecting `PCA9555` prompts for an I2C address and bit position, while `WS2812` prompts for the LED index.

## Display Editor

The Display Editor manages `DisplayMapping.cpp` entries that link DCS-BIOS data fields to physical display segments.

Features:
- Scrollable list of all display field entries
- Edit which segment map a field uses
- Press `Ctrl+D` to delete an entry
- Changes are validated before saving

## Segment Map Editor

The Segment Map Editor manages `*_SegmentMap.h` files that define how display data maps to physical display segments.

Features:
- Lists all segment map files for the current aircraft
- Auto-detects field types when adding new entries
- Auto-regenerates files after changes so new maps are picked up immediately
- Changes in segment maps trigger an automatic regeneration of the label set

## Deleting a Label Set

Deletion requires two confirmations to prevent accidents. The tool also clears the "last label set" preference if the deleted set was the remembered one.

## Preferences

The Label Creator saves preferences to `label_creator/creator_prefs.json`:
- Last selected aircraft
- Last selected label set

## Single-Instance Guard

Only one instance of the Label Creator can run at a time. A second launch brings the existing window to the foreground.

## Tool Switching

From the main menu, switch directly to:
- **Compiler Tool** -- To compile and upload after editing your label set
- **Environment Setup** -- To install or update dependencies

The Label Creator replaces itself seamlessly in the same console window.
