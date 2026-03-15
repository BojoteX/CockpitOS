# Contributing to CockpitOS

Thanks for your interest in CockpitOS! This guide explains how to report bugs,
request features, and submit code changes.

## Before You Start

CockpitOS is ESP32 firmware for DCS World cockpit panels. It has its own toolchain
(Setup Tool, Compile Tool, Label Creator) and does not use the Arduino IDE. Please
read the relevant docs before diving into the code:

- **Getting started:** `Docs/Getting-Started/`
- **Tools overview:** `Docs/Tools/`
- **Hardware reference:** `Docs/Hardware/`

## Reporting Bugs

1. **Search existing issues first** to avoid duplicates.
2. Open a new issue using the **Bug Report** template.
3. Include your ESP32 board variant, transport mode (WiFi/USB/Serial), DCS-BIOS
   version, and the label set you are using.
4. If the issue involves compile errors, paste the full output from the Compile
   Tool (CockpitOS-START.py).

## Requesting Features

1. Open a new issue using the **Feature Request** template.
2. Describe the use case and the problem it solves — not just the solution.
3. Wait for feedback before writing code. This prevents wasted effort on features
   that may already be in progress or that don't fit the project's direction.

## Submitting Code Changes

### Issue First

Every pull request **must** reference an open issue. If there isn't one, create it
first and wait for a maintainer to confirm the approach before starting work.

Small typo fixes and documentation corrections are exempt from this rule.

### Branch Workflow

```
main  <-- protected, releases only (do NOT target PRs here)
dev   <-- active development (ALL PRs target this branch)
```

1. **Fork** the repository.
2. **Branch** off `origin/dev` — name your branch descriptively
   (e.g., `fix/wifi-reconnect`, `feat/new-gauge-type`).
3. Make your changes.
4. **Open a PR targeting `dev`** — never `main`.

PRs to `main` will be closed and asked to retarget `dev`.

### Commit Messages

- Short imperative style: "Add wifi reconnect logic", "Fix LED flicker on PCA9555"
- Describe the *what*, not the *how*
- One logical change per commit

### What You Can Change

| Area | Rules |
|------|-------|
| Firmware (`src/`) | Follow existing patterns. No `new`/`malloc` in loop paths. Non-blocking only. |
| Label sets (`src/LABELS/`) | Do NOT rename, move, or delete existing label set files. |
| Python tools | ANSI TUI only (no curses, no external TUI libraries). Windows-native (`msvcrt`). |
| Config.h | Only touch defines listed in `compiler/config.py` `TRACKED_DEFINES`. |
| Generators | Do NOT run `generate_data.py` or `reset_data.py` in interactive mode. |

### Build and Test

Before submitting a PR, verify your changes compile:

1. Run `Setup-START.py` to install the ESP32 core and libraries (first time only).
2. Run `CockpitOS-START.py` to compile your label set.
3. If you modified Python tools, test them on Windows with Python 3.12+.

CI will automatically compile all 16 label sets when your PR is opened against
`dev`. All builds must pass before the PR can be merged.

### Code Style

- **C++ (firmware):** Arduino framework, static allocation, `CUtils` API for
  hardware access, `#if defined(HAS_*)` guards for conditional compilation.
- **Python (tools):** ANSI escape codes for TUI output, `msvcrt` for input,
  `os.execl()` for tool switching. No external dependencies beyond the standard
  library.

## What Happens After Your PR

1. A maintainer reviews the changes.
2. CI builds all label sets against your branch.
3. Once approved and green, the PR is merged into `dev`.
4. Periodically, `dev` is merged into `main` via a separate PR, and a release is
   published from `main`.

## Code of Conduct

Be respectful. This is a hobby project built by people who love flight sims and
building things. Keep discussions constructive and on-topic.

## Questions?

Open a **Discussion** on the GitHub repo or reach out via the issue tracker.
