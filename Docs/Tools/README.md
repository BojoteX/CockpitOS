# CockpitOS Tools

CockpitOS ships with a suite of Python-based tools that let you set up your build environment, compile and upload firmware, create label sets, and debug your cockpit panels -- all without opening the Arduino IDE.

## Requirements

- **Windows 10/11** (all tools are Windows-only)
- **Python 3.12+** (must be on your system PATH)
- No Arduino IDE installation required

## The Three Main Tools

| Tool | Entry Point | Purpose |
|------|-------------|---------|
| [Setup Tool](Setup-Tool.md) | `Setup-START.py` | Install and manage the ESP32 Arduino core and libraries |
| [Compiler Tool](Compiler-Tool.md) | `CockpitOS-START.py` | Compile and upload CockpitOS firmware to your board |
| [Label Creator](Label-Creator.md) | `LabelCreator-START.py` | Create, modify, and manage label sets for your cockpit panels |

All three tools share a consistent look and feel: ANSI-colored terminal UIs with arrow-key menus, status displays, and contextual actions.

## Tool Switching

The three main tools are designed to work as a seamless workflow. Each tool's menu includes a **Switch Tool** section that lets you jump directly to any of the other tools.

When you switch, the current tool replaces itself with the new one in the same console window (via `os.execl()`). There is no nesting of processes -- the old tool is gone and the new one takes over cleanly.

## Typical Workflow

```
 Setup Tool             Compiler Tool           Label Creator
+------------------+   +-------------------+   +-------------------+
| Install ESP32    |   | Select board      |   | Create label set  |
| core & libraries |-->| Select label set  |-->| Select aircraft   |
| (one-time)       |   | Compile & upload  |   | Pick panels       |
+------------------+   +-------------------+   | Wire inputs/LEDs  |
        ^                       ^               | Edit displays     |
        |                       |               +-------------------+
        |                       |                       |
        +-----------+-----------+-----------------------+
                    |
            Switch Tool (any direction)
```

**First time?** Start with the Setup Tool to install dependencies, then move to the Label Creator to define your cockpit panel, and finish in the Compiler Tool to build and flash.

**Iterating on a panel?** Jump straight into the Label Creator to adjust mappings, then switch to the Compiler Tool to rebuild.

## Debug and Utility Tools

Beyond the three main tools, CockpitOS includes several standalone utilities for testing and debugging:

| Tool | Purpose |
|------|---------|
| [HID Manager](Debug-Tools.md#hid-manager) | Bridges USB HID devices to DCS-BIOS over UDP |
| [Debug Tools](Debug-Tools.md#debug-tools) | UDP console, stream recording/replay, command testing, and more |

See the full [Debug Tools documentation](Debug-Tools.md) for details.

## Quick Reference

| Action | Command |
|--------|---------|
| First-time setup | `python Setup-START.py` |
| Create a label set | `python LabelCreator-START.py` |
| Compile and upload | `python CockpitOS-START.py` |
| Run HID Manager | `python "HID Manager/HID_Manager.py"` |
| View WiFi debug output | `python "Debug Tools/CONSOLE_UDP_debug.py"` |
| Replay a DCS stream | `python "Debug Tools/PLAY_DCS_stream.py"` |
