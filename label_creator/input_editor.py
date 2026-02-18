"""
CockpitOS Label Creator -- InputMapping.h TUI editor.

Scrollable list of InputMapping records with per-record editing.
Only user-editable fields are modified: source, port, bit, hidId.
Hash tables, struct definitions, and TrackedSelectorLabels are untouched.
"""

import os
import re
import sys
import msvcrt
import threading

import ui

# ---------------------------------------------------------------------------
# ANSI constants (mirror ui.py / panels.py)
# ---------------------------------------------------------------------------
CYAN     = "\033[96m"
GREEN    = "\033[92m"
YELLOW   = "\033[93m"
RED      = "\033[91m"
BOLD     = "\033[1m"
DIM      = "\033[2m"
RESET    = "\033[0m"
REV      = "\033[7m"
HIDE_CUR = "\033[?25l"
SHOW_CUR = "\033[?25h"
ERASE_LN = "\033[2K"

_SECTION_SEP = f"\n  {DIM}\u2500\u2500 Configure \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500{RESET}\n"
_SEL_BG  = "\033[48;5;236m"    # subtle dark gray row highlight

# ---------------------------------------------------------------------------
# Input source options for the picker
# ---------------------------------------------------------------------------
INPUT_SOURCES = [
    ("NONE (not wired)", "NONE"),
    ("GPIO",             "GPIO"),
    ("HC165",            "HC165"),
    ("TM1637",           "TM1637"),
    ("PCA9555",          "PCA9555"),
]

# Regex: PCA_0x followed by exactly two hex digits (case-insensitive),
# but NOT 0x00 (firmware skips address 0x00).
_PCA_ADDR_RE = re.compile(r'^PCA_0x[0-9A-Fa-f]{2}$')


def _w(text):
    sys.stdout.write(text)
    sys.stdout.flush()


def _get_terminal_size():
    try:
        size = os.get_terminal_size()
        return size.columns, size.lines
    except Exception:
        return 120, 30


# ---------------------------------------------------------------------------
# Parser — reads InputMapping.h into (records, raw_lines)
# ---------------------------------------------------------------------------
_LINE_RE = re.compile(
    r'\{\s*"(?P<label>[^"]+)"\s*,\s*'
    r'"(?P<source>[^"]+)"\s*,\s*'
    r'(?P<port>-?\d+|PIN\(\d+\)|[A-Za-z_][A-Za-z0-9_]*)\s*,\s*'
    r'(?P<bit>-?\d+)\s*,\s*'
    r'(?P<hidId>-?\d+)\s*,\s*'
    r'"(?P<cmd>[^"]+)"\s*,\s*'
    r'(?P<value>-?\d+)\s*,\s*'
    r'"(?P<type>[^"]+)"\s*,\s*'
    r'(?P<group>\d+)\s*\}\s*,'
)


def parse_input_mapping(filepath):
    """Parse InputMapping.h into (records, raw_lines).

    Each record dict:
      label, source, port (str), bit (int), hidId (int),
      cmd, value (int), controlType, group (int), line_index (int)
    """
    with open(filepath, "r", encoding="utf-8") as f:
        raw_lines = f.readlines()

    records = []
    for i, line in enumerate(raw_lines):
        m = _LINE_RE.search(line)
        if not m:
            continue
        d = m.groupdict()
        records.append({
            "label":       d["label"],
            "source":      d["source"],
            "port":        d["port"].strip(),     # string to preserve PIN() / constants
            "bit":         int(d["bit"]),
            "hidId":       int(d["hidId"]),
            "cmd":         d["cmd"],
            "value":       int(d["value"]),
            "controlType": d["type"],
            "group":       int(d["group"]),
            "line_index":  i,
        })
    return records, raw_lines


# ---------------------------------------------------------------------------
# Write-back — only replaces record lines, everything else untouched
# ---------------------------------------------------------------------------
def _fmt_port(port_str):
    """Format port value for aligned output.

    Tries to format as right-aligned integer; falls back to raw string
    for PIN() macros and named constants.
    """
    try:
        return f"{int(port_str):>2}"
    except ValueError:
        return port_str


def write_input_mapping(filepath, records, raw_lines):
    """Write updated records back into raw_lines and save to disk.

    Only the lines at each record's line_index are replaced.
    Everything else (header, struct def, hash table, TrackedSelectorLabels)
    stays byte-for-byte untouched.
    """
    if not records:
        with open(filepath, "w", encoding="utf-8") as f:
            f.writelines(raw_lines)
        return

    # Compute alignment widths across all records
    max_label = max(len(r["label"]) for r in records)
    max_cmd   = max(len(r["cmd"])   for r in records)
    max_type  = max(len(r["controlType"]) for r in records)

    for r in records:
        lblf = f'"{r["label"]}"'.ljust(max_label + 2)
        cmdf = f'"{r["cmd"]}"'.ljust(max_cmd + 2)
        ctf  = f'"{r["controlType"]}"'.ljust(max_type + 2)
        port_fmt = _fmt_port(r["port"])
        val_str  = f'{r["value"]:>5}'

        new_line = (
            f'    {{ {lblf}, "{r["source"]}" , {port_fmt} , '
            f'{r["bit"]:>2} , {r["hidId"]:>3} , '
            f'{cmdf}, {val_str} , {ctf}, {r["group"]:>2} }},\n'
        )
        raw_lines[r["line_index"]] = new_line

    with open(filepath, "w", encoding="utf-8") as f:
        f.writelines(raw_lines)


# ---------------------------------------------------------------------------
# Quick count helper (for menu caption)
# ---------------------------------------------------------------------------
def count_wired(filepath):
    """Return (wired, total) counts from an InputMapping.h file."""
    try:
        records, _ = parse_input_mapping(filepath)
    except Exception:
        return (0, 0)
    wired = sum(1 for r in records if r["source"] != "NONE")
    return (wired, len(records))


# ---------------------------------------------------------------------------
# Source classification for field prompting
# ---------------------------------------------------------------------------
# Sources where port is always 0 (or irrelevant to the user)
_FIXED_PORT_SOURCES = {"HC165"}


# ---------------------------------------------------------------------------
# Selector source options (only sources that support selectors in firmware)
# ---------------------------------------------------------------------------
SELECTOR_SOURCES = [
    ("NONE (not wired)", "NONE"),
    ("GPIO",             "GPIO"),
    ("HC165",            "HC165"),
    ("PCA9555",          "PCA9555"),
    ("MATRIX",           "MATRIX"),
]

# Momentary/action source options (all sources that support momentary inputs)
MOMENTARY_SOURCES = INPUT_SOURCES  # same list (includes TM1637, PCA9555, etc.)

# Encoder source options (only GPIO supports encoders in firmware)
ENCODER_SOURCES = [
    ("NONE (not wired)", "NONE"),
    ("GPIO",             "GPIO"),
]

# Analog source options (only GPIO supports analog)
ANALOG_SOURCES = [
    ("NONE (not wired)", "NONE"),
    ("GPIO",             "GPIO"),
]

# ---------------------------------------------------------------------------
# Beginner-friendly descriptions shown in the source picker
# ---------------------------------------------------------------------------
_SOURCE_DESCRIPTIONS = {
    "NONE": [
        f"{BOLD}Not wired{RESET}",
        f"{DIM}Skip this control \u2014 it won't be connected to any{RESET}",
        f"{DIM}physical button or switch on your panel.{RESET}",
        f"{DIM}You can always come back and wire it later.{RESET}",
    ],
    "GPIO": [
        f"{CYAN}{BOLD}GPIO{RESET} {DIM}\u2014 Direct connection to an ESP32 pin{RESET}",
        f"{DIM}The simplest wiring: one wire from a button or switch{RESET}",
        f"{DIM}straight to a pin on the ESP32 board. No extra chips needed.{RESET}",
        f"{DIM}Uses internal pull-up resistors (active LOW).{RESET}",
        "",
        f"{DIM}Good for: {RESET}buttons, toggle switches, rotary encoders,",
        f"          potentiometers, and small panels with few controls.",
    ],
    "HC165": [
        f"{CYAN}{BOLD}HC165{RESET} {DIM}\u2014 74HC165 shift register chain{RESET}",
        f"{DIM}Reads many buttons using only 3 shared wires (SPI bus).{RESET}",
        f"{DIM}Up to 8 chips daisy-chained = 64 button inputs total.{RESET}",
        f"{DIM}Each button is identified by its chain position (0\u201363).{RESET}",
        "",
        f"{DIM}Good for: {RESET}large button panels where you're running",
        f"          out of ESP32 GPIO pins.",
    ],
    "TM1637": [
        f"{CYAN}{BOLD}TM1637{RESET} {DIM}\u2014 LED/keypad driver chip{RESET}",
        f"{DIM}Primarily an LED driver, but it also has a built-in key{RESET}",
        f"{DIM}scanner that can read up to 16 buttons. Two-wire interface{RESET}",
        f"{DIM}(CLK + DIO). Each button identified by key index (0\u201315).{RESET}",
        "",
        f"{DIM}Good for: {RESET}buttons co-located with TM1637-driven LED",
        f"          indicators on the same chip.",
    ],
    "PCA9555": [
        f"{CYAN}{BOLD}PCA9555{RESET} {DIM}\u2014 I\u00b2C GPIO expander chip{RESET}",
        f"{DIM}Adds 16 extra I/O pins via the I\u00b2C bus (just 2 wires).{RESET}",
        f"{DIM}Two 8-bit ports (bank A and B). Standard chip addresses{RESET}",
        f"{DIM}are 0x20\u20130x27. Clones may use other addresses.{RESET}",
        "",
        f"{DIM}Good for: {RESET}expanding I/O when you need more pins than",
        f"          the ESP32 has available.",
    ],
    "MATRIX": [
        f"{CYAN}{BOLD}MATRIX{RESET} {DIM}\u2014 Strobe/data GPIO matrix for rotary selectors{RESET}",
        f"{DIM}Uses one shared data pin and one strobe pin per switch{RESET}",
        f"{DIM}position. Firmware pulls each strobe LOW, reads the data{RESET}",
        f"{DIM}pin, and builds a binary pattern to identify the position.{RESET}",
        "",
        f"{DIM}Good for: {RESET}multi-position rotary switches with many",
        f"          positions (5+) that would use too many GPIO pins.",
    ],
}


def _pos_suffix(record, cmd):
    """Extract human-readable position suffix from a selector label."""
    return record["label"].replace(cmd + "_", "") if record["label"].startswith(cmd + "_") else record["label"]


def _prompt_pca_address(current_source=""):
    """Prompt for a PCA9555 I2C address and return the full source string.

    Accepts any hex address in PCA_0xHH format (e.g. PCA_0x20, PCA_0x5B).
    Standard PCA9555 addresses are 0x20-0x27, but clones may use any address.
    Firmware rejects address 0x00.

    Returns the validated source string (e.g. "PCA_0x20") or None on cancel.
    """
    # Extract current address from existing source (e.g. "PCA_0x20" -> "0x20")
    default_addr = ""
    if current_source.startswith("PCA_0x"):
        default_addr = current_source[4:]  # "0x20"

    print()
    ui.info(f"{DIM}Standard PCA9555 addresses: 0x20-0x27{RESET}")
    ui.info(f"{DIM}Clones may use other addresses (e.g. 0x5A, 0x5B){RESET}")
    ui.info(f"{DIM}Enter as hex: 0x20, 0x5B, etc.{RESET}")

    # Strict regex for the two hex digits (no dots, spaces, or other chars)
    _hex2 = re.compile(r'^[0-9A-Fa-f]{2}$')

    while True:
        addr = ui.text_input("PCA I2C address (e.g. 0x20)", default=default_addr or None)
        if addr is None:
            return None

        addr = addr.strip()

        # Strip known prefixes to isolate the two hex digits
        raw = addr
        raw_up = raw.upper()
        if raw_up.startswith("PCA_0X"):
            hex_part = raw[6:]
        elif raw_up.startswith("PCA_"):
            hex_part = raw[4:]
        elif raw_up.startswith("0X"):
            hex_part = raw[2:]
        else:
            hex_part = raw

        # Strict check: must be exactly two hex digits, nothing else
        if not _hex2.match(hex_part):
            ui.warn(f"Invalid address: {addr}")
            ui.info(f"{DIM}Enter exactly two hex digits, e.g. 0x20, 0x5B, 27{RESET}")
            continue

        # Normalize to canonical PCA_0xHH
        full_src = "PCA_0x" + hex_part.upper()

        # Reject 0x00
        if hex_part.upper() == "00":
            ui.warn("Address 0x00 is not valid (firmware skips it)")
            continue

        return full_src


# ---------------------------------------------------------------------------
# Contextual help viewer
# ---------------------------------------------------------------------------
def _show_help(title, lines):
    """Display a scrollable help screen.

    Shows help text in a full-screen view. Only up/down arrows scroll;
    any other key exits back to the previous screen.
    """
    cols, term_rows = _get_terminal_size()
    # Reserve: 3 header + 1 blank + content + 1 footer = 5 fixed
    content_height = term_rows - 5
    if content_height < 3:
        content_height = 3

    total = len(lines)
    scroll = [0]

    def _draw():
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        # Header
        width = cols - 4
        _w(f"  {CYAN}{BOLD}{'=' * width}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * width}{RESET}\n")
        _w("\n")

        # Content
        for row in range(content_height):
            li = scroll[0] + row
            if li < total:
                _w(f"  {lines[li]}\n")
            else:
                _w(ERASE_LN + "\n")

        # Footer
        if total > content_height:
            pct = min(100, int((scroll[0] + content_height) / total * 100))
            _w(f"\n  {DIM}\u2191\u2193=scroll  ({pct}%)  Any other key=back{RESET}")
        else:
            _w(f"\n  {DIM}Any key=back{RESET}")

    _draw()

    try:
        while True:
            ch = msvcrt.getwch()
            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "H":          # Up
                    if scroll[0] > 0:
                        scroll[0] -= 1
                        _draw()
                elif ch2 == "P":        # Down
                    if scroll[0] < total - content_height:
                        scroll[0] += 1
                        _draw()
                else:
                    _w(SHOW_CUR)
                    return
            else:
                _w(SHOW_CUR)
                return
    except KeyboardInterrupt:
        _w(SHOW_CUR)
        raise


# ---------------------------------------------------------------------------
# Help content for each context
# ---------------------------------------------------------------------------
_HELP_MAIN_LIST = [
    f"{CYAN}{BOLD}Input Mapping Editor{RESET}",
    "",
    "This screen shows all input records from InputMapping.h.",
    "Each row is a DCS-BIOS control mapped to a physical input on your panel.",
    "",
    f"{BOLD}Columns:{RESET}",
    f"  {CYAN}Label{RESET}    The DCS-BIOS label (auto-generated, do not change).",
    f"  {CYAN}Source{RESET}   Input driver: GPIO, HC165, PCA9555, TM1637, MATRIX, or NONE.",
    f"  {CYAN}Port{RESET}    Pin or address. Meaning depends on source:",
    f"            GPIO   = physical pin number (or PIN(n) macro)",
    f"            PCA    = port bank (0 or 1)",
    f"            HC165  = always 0",
    f"            TM1637 = DIO pin",
    f"            MATRIX = data or strobe GPIO pin",
    f"  {CYAN}Bit{RESET}     Bit position or mode flag. Meaning depends on source:",
    f"            GPIO selectors: -1 = one-hot mode, 0 = regular mode",
    f"            GPIO momentary: always 0 (active LOW)",
    f"            PCA    = bit 0-7 within port, -1 = fallback",
    f"            HC165  = chain position 0-63, -1 = fallback",
    f"            MATRIX = strobe pattern (auto), -1 = fallback",
    f"  {CYAN}HID{RESET}     HID joystick button/axis ID (-1 = none/auto)",
    f"  {CYAN}Type{RESET}    Control type: selector, momentary, fixed_step, etc.",
    "",
    f"  {GREEN}Green{RESET} rows are wired.  {DIM}Gray rows are unwired (NONE).{RESET}",
    "",
    f"{BOLD}Navigation:{RESET}",
    f"  {CYAN}\u2191\u2193{RESET}         Move selection up/down",
    f"  {CYAN}\u2192 / Enter{RESET}  Edit the selected record",
    f"  {CYAN}\u2190 / Esc{RESET}    Go back to the main menu",
    f"  {CYAN}H{RESET}           Show this help screen",
    "",
    f"{BOLD}Editing:{RESET}",
    "  Selectors (group > 0) are edited as a group — all positions together.",
    "  Encoders (fixed_step/variable_step) are edited as pairs (Pin A + Pin B).",
    "  Everything else is edited individually.",
    "",
    f"{BOLD}Source drivers:{RESET}",
    f"  {CYAN}GPIO{RESET}     Direct ESP32 GPIO pins. Supports selectors (one-hot",
    "           and regular modes), momentaries, encoders, and analog inputs.",
    f"  {CYAN}PCA9555{RESET}  I2C GPIO expander. 16 I/O pins in two 8-bit ports.",
    "           Standard addresses 0x20-0x27. Clones may use any address.",
    f"  {CYAN}HC165{RESET}    74HC165 shift register chain. Up to 64 inputs (8 chips).",
    "           Directly on the SPI bus, no address needed.",
    f"  {CYAN}TM1637{RESET}   LED+keypad driver. Keys indexed 0-15.",
    f"  {CYAN}MATRIX{RESET}   Strobe/data rotary scanning. For multi-position selectors",
    "           using a GPIO matrix (shared data pin + strobe pins).",
    "           Selectors only — not valid for buttons or encoders.",
]

_HELP_SELECTOR_GROUP = [
    f"{CYAN}{BOLD}Selector Group Editing{RESET}",
    "",
    "A selector group represents a multi-position switch (2-pos, 3-pos,",
    "rotary knob, etc). All positions in the group share the same DCS-BIOS",
    "command and are sent as value=0, value=1, value=2, etc.",
    "",
    f"{BOLD}Fallback position:{RESET}",
    "  One position is the 'fallback' — it activates when NO wired input",
    "  is active. This is typically the center/default position of the switch.",
    "  The fallback does not need physical wiring.",
    "",
    f"{BOLD}Source-specific behavior:{RESET}",
    "",
    f"  {CYAN}GPIO — One-Hot mode:{RESET}",
    "    ALL positions have bit=-1. Firmware counts one-hot entries.",
    "    Each wired position gets a unique GPIO pin (active LOW).",
    "    First LOW pin wins. Fallback has port=-1, bit=-1.",
    "    You can wire ALL positions (no fallback) if every position",
    "    has its own physical pin.",
    "",
    f"  {CYAN}GPIO — Regular mode:{RESET}",
    "    NOT all bits are -1. Firmware uses CASE 2 logic.",
    "    Each wired position gets a GPIO pin. bit is always 0 (active LOW).",
    "    Fallback has port=-1, bit=0 (bit MUST NOT be -1 or it would",
    "    pollute the one-hot count and potentially trigger CASE 1).",
    "",
    f"  {CYAN}PCA9555:{RESET}",
    "    No one-hot mode. Each wired position: port (0 or 1) + bit (0-7).",
    "    port selects bank A (0) or bank B (1) on the PCA9555 chip.",
    "    Fallback: bit=-1 (becomes 255 as uint8_t in firmware).",
    "",
    f"  {CYAN}HC165:{RESET}",
    "    No one-hot mode. Each wired position: chain position 0-63.",
    "    The chain supports up to 8 daisy-chained 74HC165 chips (8 bits each).",
    "    port is always 0. Fallback: bit=-1.",
    "",
    f"  {CYAN}MATRIX:{RESET}",
    "    Strobe/data rotary scanning via GPIO matrix. For multi-position",
    "    selectors where positions are encoded as binary strobe patterns.",
    "",
    "    One shared data pin reads all strobe signals. Each wired position",
    "    has its own strobe GPIO pin. Firmware pulls each strobe LOW one at",
    "    a time, reads the data pin, and builds a binary pattern.",
    "",
    "    Anchor position:  the resting state where all strobes connect to",
    "      data. port = data pin, bit = all-ones mask (auto-computed).",
    "    Strobe positions: each gets a unique strobe GPIO pin.",
    "      port = strobe pin, bit = (1 << strobe_index) (auto-computed).",
    "    Fallback position (optional): fires when no pattern matches.",
    "      port = data pin, bit = -1.",
    "",
    "    Example (5-pos rotary with 4 strobes):",
    "      Data pin: GPIO 38",
    "      Pos F (anchor):  port=38, bit=15  (0b1111, all 4 strobes)",
    "      Pos U (strobe0): port=16, bit=1   (0b0001)",
    "      Pos A (strobe1): port=17, bit=2   (0b0010)",
    "      Pos I (strobe2): port=21, bit=4   (0b0100)",
    "      Pos N (strobe3): port=37, bit=8   (0b1000)",
]

_HELP_ENCODER = [
    f"{CYAN}{BOLD}Encoder Pair Editing{RESET}",
    "",
    "Rotary encoders have two pins (A and B) that generate quadrature signals.",
    "The firmware pairs them by matching the DCS-BIOS command (oride_label).",
    "",
    f"  {CYAN}Pin A{RESET} — DEC / CCW (value=0):  fires when turned counter-clockwise",
    f"  {CYAN}Pin B{RESET} — INC / CW  (value=1):  fires when turned clockwise",
    "",
    f"{BOLD}Control types:{RESET}",
    f"  {CYAN}fixed_step{RESET}     Sends fixed increment/decrement commands",
    f"  {CYAN}variable_step{RESET}  Sends variable step commands",
    "",
    "Only GPIO source is supported for encoders. Each pin needs a unique",
    "GPIO pin number. bit is always 0 (auto-set, active LOW).",
]

_HELP_ANALOG = [
    f"{CYAN}{BOLD}Analog Input Editing{RESET}",
    "",
    "Analog inputs read a voltage level via the ESP32's ADC (analog-to-digital",
    "converter). Used for potentiometers, sliders, and other variable controls.",
    "",
    f"{BOLD}Fields:{RESET}",
    f"  {CYAN}ADC pin{RESET}    The GPIO pin connected to the potentiometer wiper.",
    "             Must be an ADC-capable pin (check ESP32 pinout).",
    "             Can use PIN(n) macro or raw pin number.",
    "",
    f"  {CYAN}HID axis{RESET}   Joystick axis assignment for the analog input:",
    "             -1 = auto-assign (firmware picks the next available axis)",
    "              0 = X axis,  1 = Y axis,  2 = Z axis",
    "              3 = Rx axis, 4 = Ry axis, 5 = Rz axis",
    "              6 = Slider,  7 = Dial",
    "",
    "  bit is unused for analog inputs (always 0).",
    "  Only GPIO source supports analog.",
]

_HELP_MOMENTARY = [
    f"{CYAN}{BOLD}Momentary / Action Input Editing{RESET}",
    "",
    "Momentary inputs are push buttons, toggle actions, and similar controls",
    "that send a command when pressed/activated.",
    "",
    f"{BOLD}Source-specific fields:{RESET}",
    "",
    f"  {CYAN}GPIO:{RESET}",
    "    port = GPIO pin number (or PIN(n) macro)",
    "    bit  = always 0 (active LOW with internal PULLUP, auto-set)",
    "",
    f"  {CYAN}PCA9555:{RESET}",
    "    First prompts for I2C address (e.g. 0x20).",
    "    port = 0 (bank A) or 1 (bank B)",
    "    bit  = 0-7 (bit position within the port)",
    "",
    f"  {CYAN}HC165:{RESET}",
    "    port = always 0 (auto-set)",
    "    bit  = chain position 0-63 (which bit in the shift register chain)",
    "",
    f"  {CYAN}TM1637:{RESET}",
    "    port = DIO pin (the data pin for the TM1637 chip)",
    "    bit  = key index 0-15 (which key on the keypad)",
    "",
    f"  {CYAN}HID ID{RESET} = joystick button number (-1 = none)",
]
def _edit_standalone(record):
    """Edit a standalone (non-selector-group) record.

    Adapts prompts based on control type and source:
    - momentary/action: source -> source-specific fields -> hidId
    - analog:           GPIO pin -> hidId (axis assignment)
    - encoder pairs are handled separately via _edit_encoder_pair()
    Returns True if record was modified.
    """
    label = record["label"]
    ctype = record["controlType"]

    ui.header(f"Edit: {label}")
    print()
    ui.info(f"Type: {CYAN}{ctype}{RESET}    Command: {record['cmd']}")
    ui.info(f"Current: source={record['source']}  port={record['port']}  "
            f"bit={record['bit']}  hidId={record['hidId']}")
    print()

    # ── Analog controls ───────────────────────────────────────────────
    if ctype == "analog":
        return _edit_analog(record)

    # ── Momentary / action / other standalone types ───────────────────
    # Choose source list based on control type
    if ctype in ("fixed_step", "variable_step"):
        source_list = ENCODER_SOURCES
    else:
        source_list = MOMENTARY_SOURCES

    # Default to PCA9555 in the picker if currently a PCA address
    pick_default = record["source"]
    if pick_default.startswith("PCA_0x"):
        pick_default = "PCA9555"

    # Pick help based on control type
    if ctype in ("fixed_step", "variable_step"):
        help_fn = lambda: _show_help("Help — Encoder", _HELP_ENCODER)
    else:
        help_fn = lambda: _show_help("Help — Momentary / Action", _HELP_MOMENTARY)

    src = ui.pick("Source:", source_list, default=pick_default, on_help=help_fn,
                   descriptions=_SOURCE_DESCRIPTIONS)
    if src is None:
        return False

    if src == "NONE":
        record["source"] = "NONE"
        record["port"]   = "0"
        record["bit"]    = 0
        record["hidId"]  = -1
        return True

    # PCA9555: resolve the actual I2C address
    if src == "PCA9555":
        src = _prompt_pca_address(record["source"])
        if src is None:
            return False

    record["source"] = src
    return _prompt_source_fields(record, src, ctype)


def _edit_analog(record):
    """Edit an analog input (source=GPIO, port=ADC pin, hidId=axis).

    Analog controls need: GPIO pin for ADC reading, and optionally a
    HID axis assignment (-1 = auto-assign).
    """
    help_fn = lambda: _show_help("Help — Analog Input", _HELP_ANALOG)
    src = ui.pick("Source:", ANALOG_SOURCES, default=record["source"], on_help=help_fn,
                   descriptions=_SOURCE_DESCRIPTIONS)
    if src is None:
        return False

    if src == "NONE":
        record["source"] = "NONE"
        record["port"]   = "0"
        record["bit"]    = 0
        record["hidId"]  = -1
        return True

    record["source"] = src

    print(_SECTION_SEP)

    # GPIO pin (ADC-capable)
    port_val = ui.text_input("ADC pin (e.g. PIN(1) or 1)", default=record["port"])
    if port_val is None:
        return False
    record["port"] = port_val.strip()

    # HID axis assignment
    print()
    ui.info(f"{DIM}Axis: -1 = auto-assign, 0 = X, 1 = Y, 2 = Z, ...{RESET}")
    hid_val = ui.text_input("HID axis (-1 = auto)", default=str(record["hidId"]))
    if hid_val is None:
        return False
    try:
        record["hidId"] = int(hid_val)
    except ValueError:
        record["hidId"] = -1

    # bit is unused for analog, keep at 0
    record["bit"] = 0
    return True


def _edit_encoder_pair(records, rec_a, rec_b):
    """Edit a GPIO encoder pair (fixed_step or variable_step).

    Encoder pairs share the same oride_label; rec_a has value=0 (CCW/DEC),
    rec_b has value=1 (CW/INC). Each needs its own GPIO pin.
    """
    ctype = rec_a["controlType"]
    cmd = rec_a["cmd"]

    ui.header(f"Edit Encoder: {cmd}")
    print()
    ui.info(f"Type: {CYAN}{ctype}{RESET}")
    print()
    ui.info(f"  Pin A (DEC):  source={rec_a['source']}  port={rec_a['port']}")
    ui.info(f"  Pin B (INC):  source={rec_b['source']}  port={rec_b['port']}")
    print()

    help_fn = lambda: _show_help("Help — Encoder Pair", _HELP_ENCODER)
    src = ui.pick("Source:", ENCODER_SOURCES, default=rec_a["source"], on_help=help_fn,
                   descriptions=_SOURCE_DESCRIPTIONS)
    if src is None:
        return False

    if src == "NONE":
        for r in (rec_a, rec_b):
            r["source"] = "NONE"
            r["port"]   = "0"
            r["bit"]    = 0
            r["hidId"]  = -1
        return True

    print(_SECTION_SEP)

    # Pin A (DEC / CCW — value=0)
    pin_a = ui.text_input("Pin A — DEC/CCW (e.g. PIN(33))", default=rec_a["port"])
    if pin_a is None:
        return False

    # Pin B (INC / CW — value=1)
    pin_b = ui.text_input("Pin B — INC/CW  (e.g. PIN(21))", default=rec_b["port"])
    if pin_b is None:
        return False

    rec_a["source"] = src
    rec_a["port"]   = pin_a.strip()
    rec_a["bit"]    = 0
    rec_b["source"] = src
    rec_b["port"]   = pin_b.strip()
    rec_b["bit"]    = 0

    return True


def _prompt_source_fields(record, src, ctype):
    """Prompt for source-specific fields (port, bit, hidId).

    Adapts prompts based on the source driver:
    - GPIO:  port = pin, bit = active level (0=LOW, 1=HIGH) for momentaries
    - PCA:   port = 0 or 1 (bank A/B), bit = 0-7
    - HC165: port = 0 (fixed), bit = chain position 0-63
    - TM1637: port = DIO pin, bit = key index 0-15
    - MATRIX: port = data/strobe pin, bit = pattern decimal
    """
    is_pca = src.startswith("PCA_0x")

    print(_SECTION_SEP)

    # ── Port ──────────────────────────────────────────────────────────
    if src in _FIXED_PORT_SOURCES:
        # HC165: port is always 0
        record["port"] = "0"
    elif is_pca:
        port_val = ui.text_input("PCA port (0 = bank A, 1 = bank B)", default=record["port"])
        if port_val is None:
            return False
        record["port"] = port_val.strip()
    elif src == "TM1637":
        port_val = ui.text_input("DIO pin (e.g. PIN(8))", default=record["port"])
        if port_val is None:
            return False
        record["port"] = port_val.strip()
    elif src == "MATRIX":
        port_val = ui.text_input("Data/strobe GPIO pin", default=record["port"])
        if port_val is None:
            return False
        record["port"] = port_val.strip()
    else:
        # GPIO and other sources
        port_val = ui.text_input("GPIO pin (e.g. PIN(5) or 5)", default=record["port"])
        if port_val is None:
            return False
        record["port"] = port_val.strip()

    # ── Bit ───────────────────────────────────────────────────────────
    if src == "GPIO":
        # GPIO momentaries/actions: bit is always 0 (active LOW, PULLUP)
        record["bit"] = 0
        bit_val = "0"  # skip prompt
    elif src == "HC165":
        bit_val = ui.text_input("Chain position (0-63)", default=str(record["bit"]))
        if bit_val is None:
            return False
    elif src == "TM1637":
        bit_val = ui.text_input("Key index (0-15)", default=str(record["bit"]))
        if bit_val is None:
            return False
    elif is_pca:
        bit_val = ui.text_input("Bit (0-7)", default=str(record["bit"]))
        if bit_val is None:
            return False
    elif src == "MATRIX":
        bit_val = ui.text_input("Pattern (decimal, -1 = fallback)", default=str(record["bit"]))
        if bit_val is None:
            return False
    else:
        bit_val = ui.text_input("Bit", default=str(record["bit"]))
        if bit_val is None:
            return False

    try:
        record["bit"] = int(bit_val)
    except ValueError:
        record["bit"] = 0

    # ── HID ID ────────────────────────────────────────────────────────
    hid_val = ui.text_input("HID ID (-1 = none)", default=str(record["hidId"]))
    if hid_val is None:
        return False
    try:
        record["hidId"] = int(hid_val)
    except ValueError:
        record["hidId"] = -1

    return True


def _edit_selector_group(records, group_records):
    """Edit a selector group — all positions sharing the same group ID.

    Dispatches to the appropriate sub-editor based on the chosen source:
    - GPIO: one-hot mode (each position gets own pin, fallback has port=-1)
    - PCA9555: each position gets port (0/1) + bit (0-7), fallback bit=-1
    - HC165: each position gets bit (chain position 0-63), fallback bit=-1
    Returns True if any record was modified.
    """
    cmd = group_records[0]["cmd"]
    group_id = group_records[0]["group"]

    ui.header(f"Edit Selector Group: {cmd}")
    print()
    ui.info(f"Group {group_id} — {len(group_records)} positions")
    print()

    # Show current state of all positions
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        status = f"{GREEN}wired{RESET}" if r["source"] != "NONE" else f"{DIM}unwired{RESET}"
        print(f"    {i+1}. {suffix:<20} src={r['source']:<10} port={r['port']:<8} "
              f"bit={r['bit']:>3}  hid={r['hidId']:>3}  [{status}]")

    print()

    # Pick source for the group
    current_src = group_records[0]["source"]
    pick_default = current_src
    if pick_default.startswith("PCA_0x"):
        pick_default = "PCA9555"

    help_fn = lambda: _show_help("Help — Selector Group", _HELP_SELECTOR_GROUP)
    src = ui.pick("Source for this group:", SELECTOR_SOURCES, default=pick_default,
                  on_help=help_fn, descriptions=_SOURCE_DESCRIPTIONS)
    if src is None:
        return False

    if src == "NONE":
        for r in group_records:
            r["source"] = "NONE"
            r["port"]   = "0"
            r["bit"]    = 0
            r["hidId"]  = -1
        return True

    # PCA9555: resolve the actual I2C address
    if src == "PCA9555":
        src = _prompt_pca_address(current_src)
        if src is None:
            return False

    # Dispatch to source-specific selector editor
    if src == "GPIO":
        return _edit_selector_gpio(group_records, cmd, src)
    elif src.startswith("PCA_0x"):
        return _edit_selector_pca(group_records, cmd, src)
    elif src == "HC165":
        return _edit_selector_hc165(group_records, cmd, src)
    elif src == "MATRIX":
        return _edit_selector_matrix(group_records, cmd, src)
    else:
        # Fallback for unexpected sources — generic per-position editor
        return _edit_selector_generic(group_records, cmd, src)


def _edit_selector_gpio(group_records, cmd, src):
    """GPIO selector editor — supports both one-hot and regular modes.

    TWO firmware modes exist for GPIO selectors (see pollGPIOSelectors):

    ONE-HOT (bit=-1 on ALL positions):
      - Firmware detects one-hot when oneHot == total (every entry bit==-1)
      - Each wired position gets a unique GPIO pin
      - Firmware reads each pin: first LOW wins
      - Fallback position (port=-1, bit=-1) fires when no pin is LOW
      - ALL positions can be wired (no fallback) if desired

    REGULAR (bit encodes active level — NOT all -1):
      - Firmware enters CASE 2 when oneHot != total
      - Each wired position gets its own GPIO pin + bit (0 or 1)
      - bit=0 → active LOW (PULLUP), bit=1 → active HIGH (PULLDOWN)
      - Fallback position has port=-1, bit=0 (bit must NOT be -1 or it
        would be counted as oneHot and potentially trigger CASE 1)
      - Works for any number of positions (2, 3, etc.)

    User picks the mode. Default is detected from existing bit values.
    """
    # Detect current mode from existing data
    all_bits_neg1 = all(r["bit"] == -1 for r in group_records)
    current_is_onehot = all_bits_neg1

    print()
    mode_default = "onehot" if current_is_onehot else "regular"
    help_fn = lambda: _show_help("Help — GPIO Selector Modes", _HELP_SELECTOR_GROUP)
    mode = ui.pick("Wiring mode:", [
        ("One-hot (all bit=-1, first LOW wins)",        "onehot"),
        ("Regular (bit=active level per position)",     "regular"),
    ], default=mode_default, on_help=help_fn)
    if mode is None:
        return False

    if mode == "onehot":
        return _edit_selector_gpio_onehot(group_records, cmd, src)
    else:
        return _edit_selector_gpio_regular(group_records, cmd, src)


def _edit_selector_gpio_onehot(group_records, cmd, src):
    """GPIO one-hot: each position gets its own pin, all bit=-1.

    Firmware CASE 1: ALL entries have bit==-1.
    - Wired positions: unique GPIO pin, bit=-1
    - Fallback position: port=-1, bit=-1 (fires when no pin is LOW)
    - If all positions are wired (no port=-1), there is no fallback
    """
    print(f"\n  {DIM}\u2500\u2500 {CYAN}One-Hot mode{RESET}{DIM} \u2500 one unique pin per position \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500{RESET}\n")

    # Ask user which position is the fallback (or none)
    fb_options = [("No fallback (all positions wired)", -1)]
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        fb_options.append((suffix, i))

    # Pre-select current fallback (port == "-1")
    current_fb = -1
    for i, r in enumerate(group_records):
        port_s = r["port"].strip()
        if port_s == "-1" or (port_s.lstrip("-").isdigit() and int(port_s) < 0):
            current_fb = i
            break

    help_fn = lambda: _show_help("Help — GPIO One-Hot Selector", _HELP_SELECTOR_GROUP)
    fb_idx = ui.pick("Which position is the fallback (center/default)?",
                     fb_options, default=current_fb, on_help=help_fn)
    if fb_idx is None:
        return False

    # Assign pins to each position
    print()
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        r["source"] = src
        r["bit"]    = -1      # one-hot: ALL bits are -1

        if i == fb_idx:
            r["port"] = "-1"
            ui.info(f"  {suffix}: {DIM}fallback (no pin){RESET}")
        else:
            default_pin = r["port"] if r["port"] not in ("0", "-1") else ""
            pin = ui.text_input(f"  {suffix} GPIO pin (e.g. PIN(5))", default=default_pin or None)
            if pin is None:
                return False
            r["port"] = pin.strip()

    # Optional HID IDs
    print()
    set_hid = ui.confirm("Set HID IDs for each position?")
    if set_hid:
        for r in group_records:
            suffix = _pos_suffix(r, cmd)
            hid_val = ui.text_input(f"  {suffix} HID ID", default=str(r["hidId"]))
            if hid_val is None:
                break
            try:
                r["hidId"] = int(hid_val)
            except ValueError:
                r["hidId"] = -1

    return True


def _edit_selector_gpio_regular(group_records, cmd, src):
    """GPIO regular: each position gets its own pin, bit is always 0.

    Firmware CASE 2: NOT all entries have bit==-1 (oneHot != total).
    - Wired positions: GPIO pin, bit=0 (active LOW with PULLUP)
    - Fallback position: port=-1, bit=0 (bit MUST NOT be -1 or it
      would count toward oneHot and potentially trigger CASE 1)
    - bit is set automatically — user only assigns GPIO pins
    """
    print(f"\n  {DIM}\u2500\u2500 {CYAN}Regular mode{RESET}{DIM} \u2500 one pin per position, bit auto-set \u2500\u2500\u2500\u2500\u2500{RESET}\n")

    # Ask which position is the fallback (port=-1)
    fb_options = []
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        fb_options.append((suffix, i))

    current_fb = None
    for i, r in enumerate(group_records):
        port_s = r["port"].strip()
        if port_s == "-1" or (port_s.lstrip("-").isdigit() and int(port_s) < 0):
            current_fb = i
            break

    help_fn = lambda: _show_help("Help — GPIO Regular Selector", _HELP_SELECTOR_GROUP)
    fb_idx = ui.pick("Which position is the fallback (default state)?",
                     fb_options, default=current_fb, on_help=help_fn)
    if fb_idx is None:
        return False

    # Assign each wired position a pin; bit is always 0
    print()
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        r["source"] = src
        r["bit"]    = 0         # regular mode: always 0 (active LOW)

        if i == fb_idx:
            r["port"] = "-1"
            ui.info(f"  {suffix}: {DIM}fallback (no pin){RESET}")
        else:
            default_pin = r["port"] if r["port"] not in ("0", "-1") else ""
            pin = ui.text_input(f"  {suffix} GPIO pin (e.g. PIN(5))", default=default_pin or None)
            if pin is None:
                return False
            r["port"] = pin.strip()

    # Optional HID IDs
    print()
    set_hid = ui.confirm("Set HID IDs for each position?")
    if set_hid:
        for r in group_records:
            suffix = _pos_suffix(r, cmd)
            hid_val = ui.text_input(f"  {suffix} HID ID", default=str(r["hidId"]))
            if hid_val is None:
                break
            try:
                r["hidId"] = int(hid_val)
            except ValueError:
                r["hidId"] = -1

    return True


def _edit_selector_pca(group_records, cmd, src):
    """PCA9555 selector: each position gets port (0/1) + bit (0-7).

    PCA selectors use active-low logic:
    - Each wired position: port = 0 or 1, bit = 0-7
    - Fallback position: bit = -1 (becomes 255 as uint8_t in firmware)
    - Firmware reads the port byte and checks each bit for LOW
    """
    print(_SECTION_SEP)

    # Ask which position is the fallback (bit=-1)
    fb_options = []
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        fb_options.append((suffix, i))

    # Pre-select current fallback (bit == -1)
    current_fb = None
    for i, r in enumerate(group_records):
        if r["bit"] == -1:
            current_fb = i
            break

    help_fn = lambda: _show_help("Help — PCA9555 Selector", _HELP_SELECTOR_GROUP)
    fb_idx = ui.pick("Which position is the fallback (default state)?",
                     fb_options, default=current_fb, on_help=help_fn)
    if fb_idx is None:
        return False

    # Assign each position
    print()
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        r["source"] = src

        if i == fb_idx:
            r["port"] = "0"
            r["bit"]  = -1
            ui.info(f"  {suffix}: {DIM}fallback (bit=-1){RESET}")
        else:
            port_val = ui.text_input(f"  {suffix} port (0 or 1)", default=r["port"])
            if port_val is None:
                return False
            r["port"] = port_val.strip()

            bit_val = ui.text_input(f"  {suffix} bit (0-7)", default=str(r["bit"]) if r["bit"] >= 0 else "")
            if bit_val is None:
                return False
            try:
                r["bit"] = int(bit_val)
            except ValueError:
                r["bit"] = 0

    # Optional HID IDs
    print()
    set_hid = ui.confirm("Set HID IDs for each position?")
    if set_hid:
        for r in group_records:
            suffix = _pos_suffix(r, cmd)
            hid_val = ui.text_input(f"  {suffix} HID ID", default=str(r["hidId"]))
            if hid_val is None:
                break
            try:
                r["hidId"] = int(hid_val)
            except ValueError:
                r["hidId"] = -1

    return True


def _edit_selector_hc165(group_records, cmd, src):
    """HC165 selector: each position gets a chain bit position (0-63).

    HC165 selectors use active-low:
    - Each wired position: bit = chain position (0-63), port = 0 (fixed)
    - Fallback position: bit = -1
    - Firmware reads the 64-bit chain and checks each bit
    """
    print(_SECTION_SEP)

    # Ask which position is the fallback (bit=-1)
    fb_options = []
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        fb_options.append((suffix, i))

    # Pre-select current fallback (bit == -1)
    current_fb = None
    for i, r in enumerate(group_records):
        if r["bit"] == -1:
            current_fb = i
            break

    help_fn = lambda: _show_help("Help — HC165 Selector", _HELP_SELECTOR_GROUP)
    fb_idx = ui.pick("Which position is the fallback (default state)?",
                     fb_options, default=current_fb, on_help=help_fn)
    if fb_idx is None:
        return False

    # Assign each position
    print()
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        r["source"] = src
        r["port"]   = "0"      # HC165 port is always 0

        if i == fb_idx:
            r["bit"] = -1
            ui.info(f"  {suffix}: {DIM}fallback (bit=-1){RESET}")
        else:
            bit_val = ui.text_input(f"  {suffix} chain position (0-63)",
                                    default=str(r["bit"]) if r["bit"] >= 0 else "")
            if bit_val is None:
                return False
            try:
                r["bit"] = int(bit_val)
            except ValueError:
                r["bit"] = 0

    # Optional HID IDs
    print()
    set_hid = ui.confirm("Set HID IDs for each position?")
    if set_hid:
        for r in group_records:
            suffix = _pos_suffix(r, cmd)
            hid_val = ui.text_input(f"  {suffix} HID ID", default=str(r["hidId"]))
            if hid_val is None:
                break
            try:
                r["hidId"] = int(hid_val)
            except ValueError:
                r["hidId"] = -1

    return True


def _edit_selector_matrix(group_records, cmd, src):
    """MATRIX selector: strobe/data rotary scanning via GPIO pins.

    MATRIX uses a shared data GPIO pin and individual strobe GPIO pins to scan
    multi-position switches.  Firmware strobes each pin LOW one at a time,
    reads the data pin, and builds a binary pattern.

    Positions:
      - Anchor:   resting position, connects all strobes to data.
                   port = dataPin, bit = all-ones mask (2^N - 1) for N strobes.
      - Strobe:   each wired position has its own strobe GPIO pin.
                   port = strobe pin, bit = (1 << strobe_index).
      - Fallback: fires when scanned pattern matches NO position (optional).
                   port = dataPin, bit = -1.

    Returns True if any record was modified.
    """
    print(_SECTION_SEP)

    # ── 1. Data pin ──────────────────────────────────────────────────
    # Try to detect current data pin from existing records
    current_data = ""
    for r in group_records:
        if r["source"] == "MATRIX":
            b = r["bit"]
            # Anchor (popcount > 1) or fallback (bit=-1) → port is data pin
            if b == -1 or (b > 0 and bin(b).count("1") > 1):
                current_data = r["port"]
                break

    data_pin = ui.text_input("Data GPIO pin (e.g. PIN(38))", default=current_data or None)
    if data_pin is None:
        return False
    data_pin = data_pin.strip()

    # ── 2. Fallback position ─────────────────────────────────────────
    fb_options = [("No fallback", -1)]
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        fb_options.append((suffix, i))

    # Pre-select current fallback (bit == -1)
    current_fb = -1
    for i, r in enumerate(group_records):
        if r["source"] == "MATRIX" and r["bit"] == -1:
            current_fb = i
            break

    help_fn = lambda: _show_help("Help — MATRIX Selector", _HELP_SELECTOR_GROUP)
    fb_idx = ui.pick("Fallback position (fires when no pattern matches)?",
                     fb_options, default=current_fb, on_help=help_fn)
    if fb_idx is None:
        return False

    # ── 3. Anchor position ───────────────────────────────────────────
    # Build list excluding fallback
    anchor_options = [("No anchor", -1)]
    for i, r in enumerate(group_records):
        if i == fb_idx:
            continue
        suffix = _pos_suffix(r, cmd)
        anchor_options.append((suffix, i))

    # Pre-select current anchor (popcount > 1)
    current_anchor = -1
    for i, r in enumerate(group_records):
        if i == fb_idx:
            continue
        if r["source"] == "MATRIX" and r["bit"] > 0 and bin(r["bit"]).count("1") > 1:
            current_anchor = i
            break

    print()
    ui.info(f"{DIM}The anchor is the resting position where all strobes connect to data.{RESET}")
    ui.info(f"{DIM}Its bit is set to the all-ones mask automatically.{RESET}")
    anchor_idx = ui.pick("Anchor position (resting/default state)?",
                         anchor_options, default=current_anchor, on_help=help_fn)
    if anchor_idx is None:
        return False

    # ── 4. Strobe pins for remaining positions ───────────────────────
    # Collect which positions need strobe pins
    strobe_positions = []
    for i, r in enumerate(group_records):
        if i == fb_idx or i == anchor_idx:
            continue
        strobe_positions.append(i)

    if not strobe_positions and anchor_idx < 0:
        ui.warn("No wired positions to configure.")
        return False

    print()
    ui.info(f"{DIM}Assign a strobe GPIO pin for each wired position:{RESET}")

    strobe_pins = {}
    for si, pos_i in enumerate(strobe_positions):
        r = group_records[pos_i]
        suffix = _pos_suffix(r, cmd)
        # Try to detect current strobe pin
        default_pin = ""
        if r["source"] == "MATRIX" and r["port"] != data_pin and r["port"] not in ("0", "-1"):
            default_pin = r["port"]
        pin = ui.text_input(f"  {suffix} strobe pin (e.g. PIN(16))",
                            default=default_pin or None)
        if pin is None:
            return False
        strobe_pins[pos_i] = pin.strip()

    # ── 5. Apply to all records ──────────────────────────────────────
    num_strobes = len(strobe_positions)
    all_ones_mask = (1 << num_strobes) - 1 if num_strobes > 0 else 0

    for i, r in enumerate(group_records):
        r["source"] = "MATRIX"

        if i == fb_idx:
            # Fallback: port = dataPin, bit = -1
            r["port"] = data_pin
            r["bit"]  = -1
        elif i == anchor_idx:
            # Anchor: port = dataPin, bit = all-ones mask
            r["port"] = data_pin
            r["bit"]  = all_ones_mask
        elif i in strobe_pins:
            # Strobe position: port = strobe pin, bit = (1 << strobe_index)
            strobe_index = strobe_positions.index(i)
            r["port"] = strobe_pins[i]
            r["bit"]  = (1 << strobe_index)

    # Show summary
    print()
    ui.info(f"{CYAN}Summary:{RESET}")
    ui.info(f"  Data pin: {YELLOW}{data_pin}{RESET}  |  Strobes: {num_strobes}")
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        if i == fb_idx:
            ui.info(f"  {suffix}: {DIM}fallback (bit=-1){RESET}")
        elif i == anchor_idx:
            ui.info(f"  {suffix}: anchor  port={r['port']}  bit={r['bit']}")
        else:
            ui.info(f"  {suffix}: strobe  port={r['port']}  bit={r['bit']}")

    # ── 6. Optional HID IDs ──────────────────────────────────────────
    print()
    set_hid = ui.confirm("Set HID IDs for each position?")
    if set_hid:
        for r in group_records:
            suffix = _pos_suffix(r, cmd)
            hid_val = ui.text_input(f"  {suffix} HID ID", default=str(r["hidId"]))
            if hid_val is None:
                break
            try:
                r["hidId"] = int(hid_val)
            except ValueError:
                r["hidId"] = -1

    return True


def _edit_selector_generic(group_records, cmd, src):
    """Generic selector editor for unexpected sources — per-position port/bit."""
    print()
    for i, r in enumerate(group_records):
        suffix = _pos_suffix(r, cmd)
        r["source"] = src

        port_val = ui.text_input(f"  {suffix} port", default=r["port"])
        if port_val is None:
            return False
        r["port"] = port_val.strip()

        bit_val = ui.text_input(f"  {suffix} bit (-1 = fallback)", default=str(r["bit"]))
        if bit_val is None:
            return False
        try:
            r["bit"] = int(bit_val)
        except ValueError:
            r["bit"] = 0

    return True


# ---------------------------------------------------------------------------
# Scrollable TUI list view
# ---------------------------------------------------------------------------
def edit_input_mapping(filepath, label_set_name="", aircraft_name=""):
    """Main entry point: scrollable TUI editor for InputMapping.h.

    Follows panels.py _show_detail() pattern: full-redraw, msvcrt keys,
    scroll bar, boundary flash.
    """
    records, raw_lines = parse_input_mapping(filepath)
    if not records:
        ui.header("Input Mapping")
        print()
        ui.warn("No records found in InputMapping.h")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    total = len(records)
    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    # Reserve: 3 header + 1 col-header + list + 1 blank + 1 footer = 6 fixed
    list_height = rows - 6
    if list_height < 5:
        list_height = 5

    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"
    needs_scroll = total > list_height

    _flash_timer  = [None]
    _flash_active = [False]

    def _wired_count():
        return sum(1 for r in records if r["source"] != "NONE")

    def _scroll_bar_positions():
        if not needs_scroll:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / total))
        max_scroll = total - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    # Column widths — fill terminal width
    # Layout: " > " (3) + label + source + port + bit + hid + type + scroll(1)
    src_w   = 10
    port_w  = 10
    bit_w   = 4
    hid_w   = 4
    type_w  = 15
    # Fixed overhead: prefix(3) + 6 gaps(2 each=12) + scrollbar(1) + margin(1)
    fixed   = 3 + (src_w + port_w + bit_w + hid_w + type_w) + 12 + 2
    label_w = cols - fixed
    label_w = max(label_w, 10)          # minimum 10

    def _render_row(i, is_highlighted, scroll_char):
        r = records[i]
        label_trunc = r["label"][:label_w]
        src_trunc   = r["source"][:src_w]
        port_trunc  = r["port"][:port_w]
        bit_str     = str(r["bit"])
        hid_str     = str(r["hidId"])
        type_trunc  = r["controlType"][:type_w]

        wired = r["source"] != "NONE"

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG} {YELLOW}> {label_trunc:<{label_w}}  "
                    f"{src_trunc:<{src_w}}  {port_trunc:<{port_w}}  "
                    f"{bit_str:>{bit_w}}  {hid_str:>{hid_w}}  "
                    f"{type_trunc:<{type_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            color = GREEN if wired else DIM
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} "
                    f"{color}{label_trunc:<{label_w}}{RESET}{_SEL_BG}  "
                    f"{color}{src_trunc:<{src_w}}{RESET}{_SEL_BG}  "
                    f"{color}{port_trunc:<{port_w}}{RESET}{_SEL_BG}  "
                    f"{color}{bit_str:>{bit_w}}{RESET}{_SEL_BG}  "
                    f"{color}{hid_str:>{hid_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{type_trunc:<{type_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        if wired:
            color = GREEN
        else:
            color = DIM

        return (f"   {color}{label_trunc:<{label_w}}{RESET}  "
                f"{color}{src_trunc:<{src_w}}{RESET}  "
                f"{color}{port_trunc:<{port_w}}{RESET}  "
                f"{color}{bit_str:>{bit_w}}{RESET}  "
                f"{color}{hid_str:>{hid_w}}{RESET}  "
                f"{DIM}{type_trunc:<{type_w}}{RESET}"
                f" {DIM}{scroll_char}{RESET}")

    def _clamp_scroll():
        nonlocal scroll
        if idx < scroll:
            scroll = idx
        if idx >= scroll + list_height:
            scroll = idx - list_height + 1

    def _flash_row():
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _flash_active[0] = True
        _draw()
        def _restore():
            _flash_active[0] = False
            _draw()
        _flash_timer[0] = threading.Timer(0.15, _restore)
        _flash_timer[0].daemon = True
        _flash_timer[0].start()

    header_w = cols - 4

    def _draw():
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        # Header
        parts = [label_set_name, aircraft_name]
        ctx = "  (" + ", ".join(p for p in parts if p) + ")" if any(parts) else ""
        wired = _wired_count()
        title = f"Input Mapping{ctx}"
        counter = f"{wired} / {total} wired"
        spacing = header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Column header
        _w(f" {BOLD}  {'Label':<{label_w}}  {'Source':<{src_w}}  "
           f"{'Port':<{port_w}}  {'Bit':>{bit_w}}  {'HID':>{hid_w}}  "
           f"{'Type':<{type_w}}{RESET}\n")

        # Scroll bar
        thumb_start, thumb_end = _scroll_bar_positions()

        # Rows
        for row in range(list_height):
            ri = scroll + row
            if needs_scroll:
                sc = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
            else:
                sc = " "
            if ri < total:
                _w(_render_row(ri, ri == idx, sc) + "\n")
            else:
                _w(ERASE_LN + "\n")

        # Footer
        _w(f"\n  {DIM}\u2191\u2193=move  \u2192/Enter=edit  \u2190/Esc=back  H=help{RESET}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                old = idx
                if ch2 == "H":          # Up
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                elif ch2 == "P":        # Down
                    if idx < total - 1:
                        idx += 1
                    else:
                        _flash_row()
                elif ch2 == "M":        # Right — edit
                    _w(SHOW_CUR)
                    modified = _do_edit(records, idx)
                    if modified:
                        write_input_mapping(filepath, records, raw_lines)
                        # Re-parse to stay consistent
                        records, raw_lines = parse_input_mapping(filepath)
                        total = len(records)
                        if idx >= total:
                            idx = max(0, total - 1)
                        needs_scroll = total > list_height
                    _clamp_scroll()
                    _draw()
                    continue
                elif ch2 == "K":        # Left — back
                    if _flash_timer[0]:
                        _flash_timer[0].cancel()
                    _w(SHOW_CUR)
                    return
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch == "\r":            # Enter — edit
                _w(SHOW_CUR)
                modified = _do_edit(records, idx)
                if modified:
                    write_input_mapping(filepath, records, raw_lines)
                    records, raw_lines = parse_input_mapping(filepath)
                    total = len(records)
                    if idx >= total:
                        idx = max(0, total - 1)
                    needs_scroll = total > list_height
                _clamp_scroll()
                _draw()

            elif ch == "\x1b":          # Esc — back
                if _flash_timer[0]:
                    _flash_timer[0].cancel()
                _w(SHOW_CUR)
                return

            elif ch in ("h", "H"):      # Help
                _show_help("Help — Input Mapping Editor", _HELP_MAIN_LIST)
                _clamp_scroll()
                _draw()

    except KeyboardInterrupt:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)
        raise


def _find_encoder_partner(records, record):
    """Find the encoder partner for a fixed_step/variable_step record.

    Encoder pairs share the same cmd (oride_label in firmware) and controlType.
    One has value=0 (DEC/CCW), the other value=1 (INC/CW).
    Returns the partner record, or None if not found.
    """
    ctype = record["controlType"]
    cmd = record["cmd"]
    val = record["value"]

    if ctype not in ("fixed_step", "variable_step"):
        return None

    target_val = 1 if val == 0 else 0
    for r in records:
        if (r is not record
                and r["controlType"] == ctype
                and r["cmd"] == cmd
                and r["value"] == target_val):
            return r
    return None


def _do_edit(records, idx):
    """Dispatch to the appropriate edit flow for the record at idx.

    - Selector records (group > 0): edits all positions in the group together
    - Encoder pairs (fixed_step/variable_step): edits both pins together
    - Everything else: edits individually with source-aware prompts

    Returns True if any record was modified.
    """
    record = records[idx]
    ctype = record["controlType"]
    group = record["group"]

    # Selector group — edit all positions together
    if ctype == "selector" and group > 0:
        group_records = [r for r in records if r["group"] == group
                         and r["controlType"] == "selector"]
        return _edit_selector_group(records, group_records)

    # Encoder pair — edit both pins together
    if ctype in ("fixed_step", "variable_step"):
        partner = _find_encoder_partner(records, record)
        if partner:
            # Ensure DEC (value=0) is rec_a, INC (value=1) is rec_b
            if record["value"] == 0:
                return _edit_encoder_pair(records, record, partner)
            else:
                return _edit_encoder_pair(records, partner, record)

    # Standalone record
    return _edit_standalone(record)
