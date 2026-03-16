# Security Policy

## Supported Versions

CockpitOS is ESP32 firmware for DCS World cockpit panels. Only the latest release is supported with fixes.

| Version | Supported          |
| ------- | ------------------ |
| Latest  | :white_check_mark: |
| Older   | :x:                |

## Scope

CockpitOS runs on local hardware (ESP32 boards) communicating with DCS World over UDP on a local network. It does not expose internet-facing services. That said, we take the following seriously:

- **WiFi credentials** hardcoded or leaked in label sets or configs
- **UDP injection** that could send unintended commands to DCS-BIOS
- **Supply chain issues** in dependencies (ESP32 Arduino core, bundled libraries)
- **Python tool vulnerabilities** in the Setup, Compiler, or Label Creator tools

## Reporting a Vulnerability

If you discover a security issue, please report it privately:

1. **Email:** [jaltuve@gmail.com](mailto:jaltuve@gmail.com)
2. **GitHub:** Use [Security Advisories](https://github.com/BojoteX/CockpitOS/security/advisories/new) to report privately

Please include:

- Description of the vulnerability
- Steps to reproduce
- Affected component (firmware, Python tools, label sets, etc.)
- Potential impact

## Response

- You will receive an acknowledgment within **72 hours**
- Confirmed issues will be patched in the next release
- You will be credited in the release notes unless you prefer otherwise

## What This Project Does NOT Handle

CockpitOS is offline flight simulator hardware. It does not process user accounts, authentication tokens, payment data, or personal information. Vulnerabilities in DCS World itself or DCS-BIOS should be reported to their respective maintainers.
