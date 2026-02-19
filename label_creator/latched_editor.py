"""
CockpitOS Label Creator -- LatchedButtons.h TUI editor.

Scrollable toggle list of wired momentary controls.
Space/Enter toggles latched state. Auto-saves on exit.
"""

import os
import re
import sys
import msvcrt
import threading

import ui

# ---------------------------------------------------------------------------
# ANSI constants (mirror ui.py)
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
_SEL_BG  = "\033[48;5;236m"


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
# Parser — reads InputMapping.h for momentary controls
# ---------------------------------------------------------------------------
_INPUT_LINE_RE = re.compile(
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


def _get_all_input_labels(input_filepath):
    """Return set of ALL label strings in InputMapping.h (any controlType/source)."""
    if not os.path.exists(input_filepath):
        return set()
    with open(input_filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()
    labels = set()
    for line in lines:
        m = _INPUT_LINE_RE.search(line)
        if m:
            labels.add(m.group("label"))
    return labels


def _get_momentary_labels(input_filepath):
    """Return list of momentary control labels from InputMapping.h (all sources)."""
    if not os.path.exists(input_filepath):
        return []
    with open(input_filepath, "r", encoding="utf-8") as f:
        lines = f.readlines()
    labels = []
    for line in lines:
        m = _INPUT_LINE_RE.search(line)
        if not m:
            continue
        d = m.groupdict()
        if d["type"] == "momentary":
            labels.append(d["label"])
    return labels


# ---------------------------------------------------------------------------
# Parser — reads LatchedButtons.h
# ---------------------------------------------------------------------------
_LATCHED_RE = re.compile(r'"([^"]+)"')


def parse_latched_buttons(filepath):
    """Return set of latched button labels from LatchedButtons.h."""
    if not os.path.exists(filepath):
        return set()
    with open(filepath, "r", encoding="utf-8") as f:
        text = f.read()
    # Find the array body between { and };
    match = re.search(r'kLatchedButtons\s*\[\s*\]\s*=\s*\{([^}]*)\}', text, re.DOTALL)
    if not match:
        return set()
    body = match.group(1)
    return set(_LATCHED_RE.findall(body))


def count_latched(filepath, input_filepath):
    """Return count of valid latched buttons (labels must exist in InputMapping.h)."""
    latched = parse_latched_buttons(filepath)
    valid = _get_all_input_labels(input_filepath)
    return len(latched & valid)


# ---------------------------------------------------------------------------
# Writer — writes LatchedButtons.h
# ---------------------------------------------------------------------------
def write_latched_buttons(filepath, latched_labels):
    """Write LatchedButtons.h with the given list of labels."""
    lines = []
    lines.append("// LatchedButtons.h -- Per-label-set latched button configuration")
    lines.append("// Buttons listed here toggle ON/OFF instead of acting as momentary press/release.")
    lines.append("// Edit via the Label Creator tool or manually.")
    lines.append("")
    lines.append("#pragma once")
    lines.append("")
    lines.append("inline const char* kLatchedButtons[] = {")
    for label in sorted(latched_labels):
        lines.append(f'    "{label}",')
    lines.append("};")
    lines.append("inline const unsigned kLatchedButtonCount = sizeof(kLatchedButtons)/sizeof(kLatchedButtons[0]);")
    lines.append("")
    with open(filepath, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))


# ---------------------------------------------------------------------------
# TUI editor — scrollable toggle list
# ---------------------------------------------------------------------------
def edit_latched_buttons(filepath, input_filepath, label_set_name="", aircraft_name=""):
    """Main entry point: scrollable toggle list for latched buttons."""

    momentary_labels = _get_momentary_labels(input_filepath)
    if not momentary_labels:
        ui.header("Latched Buttons")
        print()
        ui.warn("No momentary controls found in InputMapping.h")
        ui.info("Add some momentary buttons first using Edit Inputs.")
        input(f"\n  {ui.DIM}Press Enter to continue...{ui.RESET}")
        return

    latched_set = parse_latched_buttons(filepath)

    # Strip invalid labels (not in InputMapping.h = source of truth)
    valid_labels = _get_all_input_labels(input_filepath)
    stripped = latched_set - valid_labels
    if stripped:
        latched_set &= valid_labels
        write_latched_buttons(filepath, latched_set)

    original_set = set(latched_set)     # track for dirty detection

    total = len(momentary_labels)
    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    list_height = rows - 6
    if list_height < 5:
        list_height = 5

    _SCROLL_BLOCK = "\u2588"
    _SCROLL_LIGHT = "\u2591"
    needs_scroll = total > list_height

    _flash_timer  = [None]
    _flash_active = [False]

    def _latched_count():
        return sum(1 for lb in momentary_labels if lb in latched_set)

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

    # Column widths
    chk_w = 3      # [x] or [ ]
    # Fixed: prefix(3) + chk(3) + gap(2) + scrollbar(1) + margin(1) = 10
    label_w = max(cols - 10, 20)

    def _render_row(i, is_highlighted, scroll_char):
        label = momentary_labels[i]
        is_latched = label in latched_set
        chk = f"{GREEN}[x]{RESET}" if is_latched else f"{DIM}[ ]{RESET}"
        label_trunc = label[:label_w]

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG} {YELLOW}> {chk} {label_trunc:<{label_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            color = GREEN if is_latched else RESET
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} {chk} "
                    f"{color}{label_trunc:<{label_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        color = GREEN if is_latched else DIM
        return (f"   {chk} {color}{label_trunc:<{label_w}}{RESET}"
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
        latched_n = _latched_count()
        title = f"Latched Buttons{ctx}"
        counter = f"{latched_n} / {total} latched"
        spacing = header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{' ' * spacing}{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")

        # Column header
        _w(f"  {DIM}   Chk  Label{RESET}\n")

        # Rows
        thumb_start, thumb_end = _scroll_bar_positions()

        for row in range(list_height):
            ri = scroll + row
            if needs_scroll:
                sc = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
            else:
                sc = " "
            if ri < total:
                _w(_render_row(ri, ri == idx, sc) + "\n")
            else:
                _w(f"{'':>{cols - 1}}{DIM}{sc}{RESET}\n")

        _w(f"\n  {DIM}Space=toggle  Up/Down=navigate  Esc=save & exit{RESET}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "H":          # Up
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                        continue
                    _clamp_scroll()
                    _draw()
                elif ch2 == "P":        # Down
                    if idx < total - 1:
                        idx += 1
                    else:
                        _flash_row()
                        continue
                    _clamp_scroll()
                    _draw()
                elif ch2 == "I":        # Page Up
                    idx = max(0, idx - list_height)
                    _clamp_scroll()
                    _draw()
                elif ch2 == "Q":        # Page Down
                    idx = min(total - 1, idx + list_height)
                    _clamp_scroll()
                    _draw()

            elif ch == " " or ch == "\r":     # Space or Enter = toggle
                label = momentary_labels[idx]
                if label in latched_set:
                    latched_set.discard(label)
                else:
                    latched_set.add(label)
                _draw()

            elif ch == "\x1b":          # Esc = save and exit
                break

    finally:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)

    # Save if changed
    if latched_set != original_set:
        write_latched_buttons(filepath, latched_set)
