"""
CockpitOS Label Creator -- Custom.json TUI editor.

Lets users create modified copies of DCS-BIOS controls: change control_type
(selector -> momentary), max_value, etc. while preserving the original
DCS-BIOS identifier and addresses.

Main entry: edit_custom_controls()
"""

import os
import sys
import json
import copy
import msvcrt
import threading
from pathlib import Path

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

_SCROLL_BLOCK = "\u2588"
_SCROLL_LIGHT = "\u2591"

EDITABLE_TYPES = [
    ("selector",        "Multi-position switch (0..N)"),
    ("toggle_switch",   "Two-position toggle (0/1)"),
    ("momentary",       "Momentary push button"),
    ("action",          "One-shot action trigger"),
    ("limited_dial",    "Rotary dial with limits"),
    ("analog_dial",     "Analog rotary (full range)"),
    ("fixed_step_dial", "Stepped rotary dial"),
]


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
# Data Layer
# ---------------------------------------------------------------------------
def load_custom_json(filepath) -> list[dict]:
    """Load Custom.json, return list of entry dicts with '_label' key."""
    path = Path(filepath)
    if not path.exists():
        return []
    try:
        with open(path, "r", encoding="utf-8") as f:
            raw = json.load(f)
    except (json.JSONDecodeError, OSError):
        return []

    custom = raw.get("Custom", {})
    if not isinstance(custom, dict):
        return []

    entries = []
    for label, item in custom.items():
        if isinstance(item, dict):
            entry = dict(item)
            entry["_label"] = label
            entries.append(entry)
    return entries


def write_custom_json(filepath, entries: list[dict]):
    """Write entries back to Custom.json with {"Custom": {...}} wrapper."""
    path = Path(filepath)
    path.parent.mkdir(parents=True, exist_ok=True)

    custom = {}
    for entry in entries:
        e = {k: v for k, v in entry.items() if k != "_label"}
        custom[entry["_label"]] = e

    with open(path, "w", encoding="utf-8") as f:
        json.dump({"Custom": custom}, f, indent=4, ensure_ascii=False)
        f.write("\n")


def count_custom(filepath) -> int:
    """Quick count of custom entries (for menu caption)."""
    return len(load_custom_json(filepath))


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
def _collect_all_controls(aircraft_data: dict,
                          selected_panels: list[str] | None = None) -> list[dict]:
    """Flatten categories into a single control list.

    Each item: {panel, _label, identifier, description, control_type, inputs, outputs}
    Excludes metadata-type controls.
    When selected_panels is given, only controls from those panels are included.
    """
    allowed = set(selected_panels) if selected_panels else None
    items = []
    for panel, controls in sorted(aircraft_data.items()):
        if not isinstance(controls, dict):
            continue
        if allowed and panel not in allowed:
            continue
        for ctrl_label, ctrl in sorted(controls.items()):
            if not isinstance(ctrl, dict):
                continue
            ct = ctrl.get("control_type", "")
            if ct == "metadata":
                continue
            items.append({
                "panel": panel,
                "_label": ctrl_label,
                "identifier": ctrl.get("identifier", ctrl_label),
                "description": ctrl.get("description", ""),
                "control_type": ct,
                "inputs": ctrl.get("inputs", []),
                "outputs": ctrl.get("outputs", []),
                "category": ctrl.get("category", panel),
            })
    return items


def _get_all_existing_labels(aircraft_data: dict, custom_entries: list[dict]) -> set:
    """Collect every label across all categories + custom entries."""
    labels = set()
    for category, controls in aircraft_data.items():
        if isinstance(controls, dict):
            labels.update(controls.keys())
    for entry in custom_entries:
        labels.add(entry.get("_label", ""))
    return labels


def _generate_unique_label(identifier: str, existing: set) -> str:
    """Generate a unique custom label from an identifier."""
    candidate = f"{identifier}_CUSTOM"
    if candidate not in existing:
        return candidate
    for i in range(2, 100):
        candidate = f"{identifier}_CUSTOM_{i}"
        if candidate not in existing:
            return candidate
    return f"{identifier}_CUSTOM_{id(identifier)}"


# ---------------------------------------------------------------------------
# Control Browser (full-screen filterable picker)
# ---------------------------------------------------------------------------
def _pick_source_control(aircraft_data: dict,
                         selected_panels: list[str] | None = None) -> dict | None:
    """Full-screen filterable picker of aircraft controls.

    Shows: Identifier | Panel | Type    (Identifier first — that's what users search for)
    Filter matches across identifier, panel, description, and type.
    Tab toggles between selected panels only and all panels.
    Returns the full control dict or None on Esc.
    """
    # Two item pools: selected-only and all
    has_selection = bool(selected_panels)
    items_selected = _collect_all_controls(aircraft_data, selected_panels) if has_selection else []
    items_all      = _collect_all_controls(aircraft_data)

    # Start with selected-only if available, otherwise all
    show_all = [not has_selection]  # mutable for nested access

    def _build_search_strs(item_list):
        return [
            f"{it['identifier']} {it['panel']} {it['description']} {it['control_type']}".lower()
            for it in item_list
        ]

    search_selected = _build_search_strs(items_selected)
    search_all      = _build_search_strs(items_all)

    # Active references (swapped on toggle)
    items      = [items_all if show_all[0] else items_selected]
    search_strs = [search_all if show_all[0] else search_selected]

    if not items_all:
        ui.error("No controls found in aircraft data.")
        _w(f"\n  {DIM}Press any key...{RESET}")
        msvcrt.getwch()
        return None

    filter_text = ""
    filtered_indices = list(range(len(items[0])))
    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    # Reserve: 3 header + 1 blank + 1 footer + 1 blank = 6 lines
    list_height = rows - 6
    if list_height < 5:
        list_height = 5

    # Column widths — identifier is the primary column, gets the most space
    # Layout: " > IDENTIFIER              Panel Name            TYPE  ░"
    #   prefix = 3 chars (" > "), suffix = 3 chars (space + scroll + margin)
    type_w = 12
    prefix_len = 3
    suffix_len = 3
    available = cols - prefix_len - suffix_len - type_w - 4  # 4 = gaps between cols
    ident_w = max(available * 3 // 5, 16)
    panel_w = max(available - ident_w, 14)

    _flash_timer  = [None]
    _flash_active = [False]

    def _total():
        return len(items[0])

    def _apply_filter():
        nonlocal filtered_indices, idx
        t = _total()
        if filter_text:
            ft = filter_text.lower()
            filtered_indices = [i for i in range(t) if ft in search_strs[0][i]]
        else:
            filtered_indices = list(range(t))
        if not filtered_indices:
            idx = 0
        elif idx >= len(filtered_indices):
            idx = len(filtered_indices) - 1

    def _toggle_scope():
        nonlocal filtered_indices, idx, scroll
        show_all[0] = not show_all[0]
        if show_all[0]:
            items[0] = items_all
            search_strs[0] = search_all
        else:
            items[0] = items_selected
            search_strs[0] = search_selected
        idx = 0
        scroll = 0
        filter_text_ref = filter_text  # noqa: preserve current filter
        _apply_filter()

    def _needs_scroll():
        return len(filtered_indices) > list_height

    def _scroll_bar_positions():
        ftotal = len(filtered_indices)
        if ftotal <= list_height:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / ftotal))
        max_scroll = ftotal - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    def _clamp_scroll():
        nonlocal scroll
        if not filtered_indices:
            scroll = 0
            return
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

    def _render_row(ri, is_highlighted, scroll_char):
        """Render a single control row with scroll indicator on the right."""
        item = items[0][filtered_indices[ri]]
        ident_t = item["identifier"][:ident_w]
        panel_t = item["panel"][:panel_w]
        type_t  = item["control_type"][:type_w]

        # Pad identifier to column width
        ident_pad = ident_w - len(ident_t)

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG}{YELLOW} > {ident_t}"
                    f"{' ' * ident_pad}  {panel_t:<{panel_w}}  {type_t:<{type_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} {GREEN}{ident_t}{RESET}{_SEL_BG}"
                    f"{' ' * ident_pad}  {DIM}{panel_t:<{panel_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{type_t:<{type_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        return (f"   {ident_t}"
                f"{' ' * ident_pad}  {DIM}{panel_t:<{panel_w}}{RESET}  "
                f"{DIM}{type_t:<{type_w}}{RESET}"
                f" {DIM}{scroll_char}{RESET}")

    _header_w = cols - 4

    def _draw():
        """Full redraw of the control browser."""
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        ftotal = len(filtered_indices)
        total = _total()

        # Scope indicator
        scope_tag = "all panels" if show_all[0] else "selected panels"

        # Header with optional filter display
        title = "Select Source Control to Copy"
        if filter_text:
            right_text = f"filter: {filter_text}"
            right_ansi = f"{DIM}filter: {RESET}{YELLOW}{filter_text}"
        else:
            right_text = f"{ftotal} / {total} controls ({scope_tag})"
            right_ansi = f"{GREEN}{ftotal} / {total} controls{RESET} {DIM}({scope_tag})"
        spacing = _header_w - len(title) - len(right_text) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{RESET}{' ' * spacing}{right_ansi}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")

        # Compute scroll bar thumb positions
        thumb_start, thumb_end = _scroll_bar_positions()

        # Control rows
        if ftotal == 0:
            _w(f"\n  {DIM}No matches.{RESET}\n")
            for _ in range(list_height - 2):
                _w("\n")
        else:
            for row in range(list_height):
                ri = scroll + row
                if _needs_scroll():
                    scroll_char = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
                else:
                    scroll_char = " "

                if ri < ftotal:
                    _w(_render_row(ri, ri == idx, scroll_char) + "\n")
                else:
                    _w(ERASE_LN + "\n")

        # Footer
        if filter_text:
            match_info = f"{ftotal}/{total} matches"
        else:
            match_info = f"{total} controls"
        filter_hint = f"  {DIM}\u2190=clear filter{RESET}" if filter_text else ""
        tab_hint = f"  Tab={DIM}{'selected' if show_all[0] else 'all'} panels{RESET}" if has_selection else ""
        _w(f"\n  {DIM}{match_info}    type to filter  Enter=select  Esc=back{RESET}{filter_hint}{tab_hint}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if ch2 == "K":        # Left -- clear filter
                    if filter_text:
                        filter_text = ""
                        _apply_filter()
                        _clamp_scroll()
                        _draw()
                    continue
                if not filtered_indices:
                    continue
                old = idx
                if ch2 == "H":          # Up
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                        continue
                elif ch2 == "P":        # Down
                    if idx < len(filtered_indices) - 1:
                        idx += 1
                    else:
                        _flash_row()
                        continue
                elif ch2 == "I":        # Page Up
                    idx = max(0, idx - list_height)
                elif ch2 == "Q":        # Page Down
                    idx = min(max(0, len(filtered_indices) - 1), idx + list_height)
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch == "\r":            # Enter -- select
                if filtered_indices:
                    if _flash_timer[0]:
                        _flash_timer[0].cancel()
                    _w(SHOW_CUR)
                    return items[0][filtered_indices[idx]]
                continue

            elif ch == "\x1b":          # Esc -- cancel
                if _flash_timer[0]:
                    _flash_timer[0].cancel()
                _w(SHOW_CUR)
                return None

            elif ch == "\t":            # Tab -- toggle scope
                if has_selection:
                    _toggle_scope()
                    _clamp_scroll()
                    _draw()

            elif ch == "\x08":          # Backspace
                if filter_text:
                    filter_text = filter_text[:-1]
                    _apply_filter()
                    _clamp_scroll()
                    _draw()

            elif ch.isprintable():
                filter_text += ch
                _apply_filter()
                _clamp_scroll()
                _draw()

    except KeyboardInterrupt:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)
        raise


# ---------------------------------------------------------------------------
# Add / Edit flow
# ---------------------------------------------------------------------------
def _pick_control_type(current: str = "") -> str | None:
    """Small picker for control_type. Returns type string or None."""
    options = []
    for ctype, desc in EDITABLE_TYPES:
        label = f"{ctype:<20} {DIM}{desc}{RESET}"
        options.append((label, ctype))
    return ui.pick_filterable("Control type:", options, default=current)


INPUT_INTERFACES = [
    ("fixed_step",    "Switch to previous/next state"),
    ("set_state",     "Set position directly"),
    ("variable_step", "Turn dial left/right"),
    ("action",        "One-shot trigger"),
    ("set_string",    "Set string value"),
]


def _pick_interface(current: str = "") -> str | None:
    """Small picker for input interface type. Returns string or None."""
    options = []
    for iface, desc in INPUT_INTERFACES:
        label = f"{iface:<16} {DIM}{desc}{RESET}"
        options.append((label, iface))
    return ui.pick_filterable("Interface:", options, default=current)


OUTPUT_TYPES = [
    ("integer", "Integer value"),
    ("string",  "String value"),
]


def _edit_inputs(inputs: list[dict]) -> list[dict]:
    """Let user review and edit each input interface.

    Shows each input with its fields.  Key fields editable:
      interface, max_value, suggested_step
    Other fields (description, argument) shown and preserved.
    Enter keeps current default.  Esc skips remaining inputs.
    Returns modified deep copy.
    """
    result = copy.deepcopy(inputs)
    if not result:
        _w(f"\n  {DIM}No inputs on this control.{RESET}\n")
        return result

    print()
    total = len(result)
    for i, inp in enumerate(result):
        iface = inp.get("interface", "unknown")
        desc = inp.get("description", "")
        _w(f"  {BOLD}Input {i + 1}/{total}{RESET}")
        if desc:
            _w(f"  {DIM}{desc}{RESET}")
        _w("\n")

        # Interface (key field)
        new_iface = _pick_interface(current=iface)
        if new_iface is None:
            break   # Esc = stop editing inputs
        inp["interface"] = new_iface

        # max_value (present on set_state, variable_step)
        if "max_value" in inp:
            cur = inp["max_value"]
            val = ui.text_input(f"    max_value", default=str(cur))
            if val is None:
                break
            try:
                inp["max_value"] = int(val)
            except ValueError:
                ui.warn(f"Invalid integer, keeping {cur}")

        # suggested_step (present on variable_step)
        if "suggested_step" in inp:
            cur = inp["suggested_step"]
            val = ui.text_input(f"    suggested_step", default=str(cur))
            if val is None:
                break
            try:
                inp["suggested_step"] = int(val)
            except ValueError:
                ui.warn(f"Invalid integer, keeping {cur}")

        # argument (present on action inputs like TOGGLE)
        if "argument" in inp:
            cur = inp["argument"]
            val = ui.text_input(f"    argument", default=str(cur))
            if val is None:
                break
            if val:
                inp["argument"] = val

        print()

    return result


def _edit_outputs(outputs: list[dict]) -> list[dict]:
    """Let user review and edit each output.

    Key fields editable: max_value, mask, shift_by, type
    Address fields shown read-only and preserved.
    Enter keeps current default.  Esc skips remaining outputs.
    Returns modified deep copy.
    """
    result = copy.deepcopy(outputs)
    if not result:
        _w(f"\n  {DIM}No outputs on this control.{RESET}\n")
        return result

    print()
    total = len(result)
    for i, out in enumerate(result):
        desc = out.get("description", "")
        addr = out.get("address", "?")
        _w(f"  {BOLD}Output {i + 1}/{total}{RESET}")
        if desc:
            _w(f"  {DIM}{desc}{RESET}")
        _w(f"  {DIM}(address: {addr}){RESET}\n")

        # type (key field)
        cur_type = out.get("type", "integer")
        options = []
        for otype, odesc in OUTPUT_TYPES:
            label = f"{otype:<10} {DIM}{odesc}{RESET}"
            options.append((label, otype))
        new_type = ui.pick_filterable("Output type:", options, default=cur_type)
        if new_type is None:
            break
        out["type"] = new_type

        # max_value
        if "max_value" in out:
            cur = out["max_value"]
            val = ui.text_input(f"    max_value", default=str(cur))
            if val is None:
                break
            try:
                out["max_value"] = int(val)
            except ValueError:
                ui.warn(f"Invalid integer, keeping {cur}")

        # mask
        if "mask" in out:
            cur = out["mask"]
            val = ui.text_input(f"    mask", default=str(cur))
            if val is None:
                break
            try:
                out["mask"] = int(val)
            except ValueError:
                ui.warn(f"Invalid integer, keeping {cur}")

        # shift_by
        if "shift_by" in out:
            cur = out["shift_by"]
            val = ui.text_input(f"    shift_by", default=str(cur))
            if val is None:
                break
            try:
                out["shift_by"] = int(val)
            except ValueError:
                ui.warn(f"Invalid integer, keeping {cur}")

        print()

    return result


def _create_custom_entry(source: dict, aircraft_data: dict,
                         existing_entries: list[dict]) -> dict | None:
    """Guided flow: copy source control, let user customize, return entry or None."""
    existing_labels = _get_all_existing_labels(aircraft_data, existing_entries)

    os.system("cls" if os.name == "nt" else "clear")

    cols, _ = _get_terminal_size()
    header_w = cols - 4

    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
    _w(f"  {CYAN}{BOLD}  Creating Custom Control{RESET}\n")
    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n\n")

    _w(f"     {DIM}Source:{RESET}       {GREEN}{source['identifier']}{RESET}\n")
    _w(f"     {DIM}Panel:{RESET}        {source['panel']}\n")
    _w(f"     {DIM}Type:{RESET}         {source['control_type']}\n")
    _w(f"     {DIM}Description:{RESET}  {source['description']}\n")
    print()

    _w(f"  {DIM}The DCS-BIOS identifier and addresses are preserved.{RESET}\n")
    _w(f"  {DIM}Only the control type and input values can be changed.{RESET}\n")
    _w(f"  {DIM}Press Esc at any time to cancel.{RESET}\n")
    print()

    # --- Label ---
    default_label = _generate_unique_label(source["identifier"], existing_labels)
    while True:
        label = ui.text_input("Custom label", default=default_label)
        if label is None:
            return None
        label = label.strip().upper().replace(" ", "_")
        if not label:
            ui.error("Label cannot be empty.")
            continue
        if label in existing_labels:
            ui.error(f"Label '{label}' already exists. Choose a unique name.")
            continue
        break

    # --- Control type ---
    print()
    ctype = _pick_control_type(current=source["control_type"])
    if ctype is None:
        return None

    # --- Inputs ---
    _w(f"\n  {CYAN}{BOLD}--- Inputs ---{RESET}\n")
    inputs = _edit_inputs(source.get("inputs", []))

    # Outputs are copied as-is from the source control.
    # Custom controls only change input behaviour; the output
    # (address/mask/shift) is identical to the original and the
    # generator skips custom outputs entirely.
    outputs = copy.deepcopy(source.get("outputs", []))

    # --- Build entry ---
    entry = {
        "_label": label,
        "category": source.get("category", source["panel"]),
        "control_type": ctype,
        "description": source["description"],
        "identifier": source["identifier"],
        "inputs": inputs,
        "outputs": outputs,
    }

    # --- Confirm ---
    print()
    _w(f"  {BOLD}Summary:{RESET}\n")
    _w(f"     Label:    {GREEN}{label}{RESET}\n")
    _w(f"     Type:     {ctype}\n")
    _w(f"     Source:   {source['identifier']} ({source['panel']})\n")
    for inp in inputs:
        iface = inp.get("interface", "?")
        mv = inp.get("max_value", "")
        mv_str = f"  max={mv}" if mv != "" else ""
        _w(f"     Input:    {iface}{mv_str}\n")
    print()

    if not ui.confirm("Create this custom control?"):
        return None

    return entry


def _edit_custom_entry(entry: dict, aircraft_data: dict,
                       existing_entries: list[dict]) -> dict | None:
    """Edit an existing custom entry. Returns modified dict or None on cancel."""
    existing_labels = _get_all_existing_labels(aircraft_data, existing_entries)
    current_label = entry.get("_label", "")
    existing_labels.discard(current_label)  # allow keeping same label

    os.system("cls" if os.name == "nt" else "clear")

    cols, _ = _get_terminal_size()
    header_w = cols - 4

    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n")
    _w(f"  {CYAN}{BOLD}  Editing Custom Control{RESET}\n")
    _w(f"  {CYAN}{BOLD}{'=' * header_w}{RESET}\n\n")

    _w(f"     {DIM}Label:{RESET}        {GREEN}{current_label}{RESET}\n")
    _w(f"     {DIM}Identifier:{RESET}   {entry.get('identifier', '')}\n")
    _w(f"     {DIM}Category:{RESET}     {entry.get('category', '')}\n")
    _w(f"     {DIM}Type:{RESET}         {entry.get('control_type', '')}\n")
    _w(f"     {DIM}Description:{RESET}  {entry.get('description', '')}\n")
    print()
    _w(f"  {DIM}Press Esc at any time to cancel.{RESET}\n")
    print()

    # --- Label ---
    while True:
        label = ui.text_input("Custom label", default=current_label)
        if label is None:
            return None
        label = label.strip().upper().replace(" ", "_")
        if not label:
            ui.error("Label cannot be empty.")
            continue
        if label in existing_labels:
            ui.error(f"Label '{label}' already exists. Choose a unique name.")
            continue
        break

    # --- Control type ---
    print()
    ctype = _pick_control_type(current=entry.get("control_type", ""))
    if ctype is None:
        return None

    # --- Inputs ---
    _w(f"\n  {CYAN}{BOLD}--- Inputs ---{RESET}\n")
    inputs = _edit_inputs(entry.get("inputs", []))

    # Outputs preserved as-is (custom controls only change inputs)
    # --- Build updated entry ---
    updated = dict(entry)
    updated["_label"] = label
    updated["control_type"] = ctype
    updated["inputs"] = inputs

    # --- Confirm ---
    print()
    _w(f"  {BOLD}Summary:{RESET}\n")
    _w(f"     Label:    {GREEN}{label}{RESET}\n")
    _w(f"     Type:     {ctype}\n")
    _w(f"     Source:   {updated.get('identifier', '')} ({updated.get('category', '')})\n")
    for inp in inputs:
        iface = inp.get("interface", "?")
        mv = inp.get("max_value", "")
        mv_str = f"  max={mv}" if mv != "" else ""
        _w(f"     Input:    {iface}{mv_str}\n")
    print()

    if not ui.confirm("Save changes?"):
        return None

    return updated


# ---------------------------------------------------------------------------
# Main list view
# ---------------------------------------------------------------------------
def edit_custom_controls(custom_json_path: str, aircraft_data: dict,
                         label_set_name: str = "", aircraft_name: str = "",
                         selected_panels: list[str] | None = None):
    """Main entry point: scrollable list editor for Custom.json."""

    entries = load_custom_json(custom_json_path)
    original = [dict(e) for e in entries]  # shallow copy for dirty check

    idx = 0
    scroll = 0

    cols, rows = _get_terminal_size()
    # Reserve: 3 header + 1 blank + 1 footer + 1 blank = 6 lines (same as control browser)
    list_height = rows - 6
    if list_height < 5:
        list_height = 5

    _flash_timer  = [None]
    _flash_active = [False]

    # Column widths — Label is the primary column, gets the most space
    # Layout: " > LABEL              Panel Name       TYPE         ░"
    #   prefix = 3 chars (" > "), suffix = 3 chars (space + scroll + margin)
    type_w = 14
    prefix_len = 3
    suffix_len = 3
    available = cols - prefix_len - suffix_len - type_w - 4  # 4 = gaps between cols
    label_w = max(available * 3 // 5, 16)
    panel_w = max(available - label_w, 14)

    def _needs_scroll():
        return len(entries) > list_height

    def _scroll_bar_positions():
        total = len(entries)
        if total <= list_height:
            return (0, list_height)
        thumb = max(1, round(list_height * list_height / total))
        max_scroll = total - list_height
        if max_scroll <= 0:
            top = 0
        else:
            top = round(scroll * (list_height - thumb) / max_scroll)
        return (top, top + thumb)

    def _clamp_scroll():
        nonlocal scroll, idx
        total = len(entries)
        if total == 0:
            idx = 0
            scroll = 0
            return
        if idx >= total:
            idx = total - 1
        if idx < 0:
            idx = 0
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

    def _render_row(i, is_highlighted, scroll_char):
        """Render a single custom entry row with scroll indicator."""
        e = entries[i]
        label_t = e.get("_label", "")[:label_w]
        panel_t = e.get("category", "")[:panel_w]
        type_t  = e.get("control_type", "")[:type_w]

        # Pad label to column width
        label_pad = label_w - len(label_t)

        if is_highlighted and _flash_active[0]:
            return (f"{_SEL_BG}{YELLOW} > {label_t}"
                    f"{' ' * label_pad}  {panel_t:<{panel_w}}  {type_t:<{type_w}}"
                    f" {scroll_char}{RESET}")
        elif is_highlighted:
            return (f"{_SEL_BG} {CYAN}>{RESET}{_SEL_BG} {GREEN}{label_t}{RESET}{_SEL_BG}"
                    f"{' ' * label_pad}  {DIM}{panel_t:<{panel_w}}{RESET}{_SEL_BG}  "
                    f"{DIM}{type_t:<{type_w}}{RESET}{_SEL_BG}"
                    f" {DIM}{scroll_char}{RESET}")

        return (f"   {label_t}"
                f"{' ' * label_pad}  {DIM}{panel_t:<{panel_w}}{RESET}  "
                f"{DIM}{type_t:<{type_w}}{RESET}"
                f" {DIM}{scroll_char}{RESET}")

    _header_w = cols - 4

    def _draw():
        """Full redraw of the custom controls list."""
        os.system("cls" if os.name == "nt" else "clear")
        _w(HIDE_CUR)

        total = len(entries)

        # Header with right-aligned green counter
        parts = [label_set_name, aircraft_name]
        ctx = "  (" + ", ".join(p for p in parts if p) + ")" if any(parts) else ""
        title = f"Custom Controls{ctx}"
        counter = f"{total} entr{'ies' if total != 1 else 'y'}"
        spacing = _header_w - len(title) - len(counter) - 4
        if spacing < 1:
            spacing = 1
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")
        _w(f"  {CYAN}{BOLD}  {title}{RESET}{' ' * spacing}{GREEN}{counter}{RESET}\n")
        _w(f"  {CYAN}{BOLD}{'=' * _header_w}{RESET}\n")

        # Compute scroll bar thumb positions
        thumb_start, thumb_end = _scroll_bar_positions()

        # Entry rows
        if total == 0:
            _w(f"\n  {DIM}No custom controls defined. Press A to add one.{RESET}\n")
            for _ in range(list_height - 2):
                _w("\n")
        else:
            for row in range(list_height):
                ri = scroll + row
                if _needs_scroll():
                    scroll_char = _SCROLL_BLOCK if thumb_start <= row < thumb_end else _SCROLL_LIGHT
                else:
                    scroll_char = " "

                if ri < total:
                    _w(_render_row(ri, ri == idx, scroll_char) + "\n")
                else:
                    _w(ERASE_LN + "\n")

        # Footer
        _w(f"\n  {DIM}{total} entries    Enter=edit  A=add  D=delete  Esc=save & exit{RESET}")

    _clamp_scroll()
    _draw()

    try:
        while True:
            ch = msvcrt.getwch()

            if ch in ("\xe0", "\x00"):
                ch2 = msvcrt.getwch()
                if not entries:
                    continue
                old = idx
                if ch2 == "H":          # Up
                    if idx > 0:
                        idx -= 1
                    else:
                        _flash_row()
                        continue
                elif ch2 == "P":        # Down
                    if idx < len(entries) - 1:
                        idx += 1
                    else:
                        _flash_row()
                        continue
                elif ch2 == "I":        # Page Up
                    idx = max(0, idx - list_height)
                elif ch2 == "Q":        # Page Down
                    idx = min(max(0, len(entries) - 1), idx + list_height)
                else:
                    continue
                if old != idx:
                    _clamp_scroll()
                    _draw()

            elif ch in ("a", "A"):      # Add new entry
                source = _pick_source_control(aircraft_data, selected_panels)
                if source:
                    result = _create_custom_entry(source, aircraft_data, entries)
                    if result:
                        entries.append(result)
                        idx = len(entries) - 1
                _clamp_scroll()
                _draw()

            elif ch == "\r":            # Enter = edit selected
                if entries:
                    result = _edit_custom_entry(entries[idx], aircraft_data, entries)
                    if result:
                        entries[idx] = result
                _clamp_scroll()
                _draw()

            elif ch in ("d", "D", "\x04"):     # D or Ctrl+D = delete
                if entries:
                    _w(SHOW_CUR)
                    label = entries[idx].get("_label", "")
                    _w(f"\n\n  {YELLOW}Delete '{label}'? (y/N): {RESET}")
                    confirm_ch = msvcrt.getwch()
                    if confirm_ch in ("y", "Y"):
                        entries.pop(idx)
                    _clamp_scroll()
                    _draw()

            elif ch == "\x1b":          # Esc = save and exit
                break

    finally:
        if _flash_timer[0]:
            _flash_timer[0].cancel()
        _w(SHOW_CUR)

    # Save if changed
    if entries != original:
        write_custom_json(custom_json_path, entries)
        return True
    return False
