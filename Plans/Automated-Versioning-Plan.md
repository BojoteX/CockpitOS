# CockpitOS Automated Versioning Plan

## Context

VERSION_CURRENT in Config.h is manually edited (`"R.1.3.0_dev_03-11-26"`), disconnected from CHANGELOG.md (latest v1.2.8) and git tags (v1.2.0-v1.2.9). There's no way to know what build is running on an ESP32 or whether you're on the latest release. This plan adds a single source of truth (`version.py`), auto-stamps the firmware at compile time with version+git hash, and automates GitHub releases on PR merge to main.

## Files to Create

### 1. `version.py` (project root) -- NEW
Single source of truth for the version number:
```python
__version__ = "1.3.0"
```
- Importable by Python tools (`from version import __version__`)
- Parseable by CI via regex (`grep -oP '__version__\s*=\s*"\K[^"]+'`)
- Bump this file when you want a new release

### 2. `.github/workflows/auto-release.yml` -- NEW
Triggers on PR merge to main. Reads version from `version.py`, skips if tag already exists (so you control when releases happen by bumping version.py), packages ZIP using the same exclusions as existing `release.yml`, publishes GitHub release with auto-generated changelog.

## Files to Modify

### 3. `compiler/config.py`
Add two functions (~40 lines) after the existing `config_set()` (around line 280):

**`stamp_version()`** -- called before compilation:
- Reads `__version__` from `version.py` via regex
- Gets `git rev-parse --short=7 HEAD` for commit hash
- Gets `git diff --quiet HEAD` for dirty flag
- Builds stamp: `"1.3.0+abc1234"` or `"1.3.0+abc1234-dirty"` or `"1.3.0"` (no git)
- Saves current VERSION_CURRENT value in module-level `_VERSION_BACKUP`
- Writes stamped value to Config.h via existing `write_config_define()`

**`restore_version()`** -- called after compilation (in `finally`):
- Restores `_VERSION_BACKUP` to Config.h via `write_config_define()`

Key detail: `write_config_define()` regex replaces a single `\S+` token. The quoted C string `"1.3.0+abc1234"` has no spaces, so it matches correctly. The replacement value must include the C quotes: `'"1.3.0+abc1234"'`.

### 4. `compiler/cockpitos.py`

**a) `do_compile()` (line 152)** -- wrap compile in stamp/restore:
After `run_generate_data()` (line 191) and before `compile_sketch()` (line 198), add:
```python
stamp = stamp_version()
try:
    # existing compile_sketch() call
finally:
    restore_version()
```

**b) Main menu (around line 663)** -- add version display line:
Between the debug status line (line 661) and the `print()` before `menu_pick`, add a version info line:
```
     v1.3.0                                    (v1.3.1 available)
```
- Left side: local version from `version.py`
- Right side (yellow): latest GitHub release if different (fetched once per session via `urllib.request`, 3s timeout, cached, fails silently)
- Uses the same `_fetch_json` pattern from Setup-START.py (GitHub releases/latest API)

### 5. `Config.h` (line 24)
Change from manual version to auto-managed placeholder:
```c
#define VERSION_CURRENT        "dev"  // Auto-stamped at compile time from version.py
```
Comment makes it clear this value is no longer hand-edited.

### 6. `.github/workflows/ci-build.yml`
Add a version stamp step before compilation (~4 lines of shell) that reads version.py + git hash and writes it into Config.h via `sed`. No restore needed since CI checkouts are ephemeral.

### 7. `CLAUDE.md`
Add `version.py` to the Project Structure table. Add a note in Config.h section that VERSION_CURRENT is auto-managed.

## What Does NOT Change
- `CockpitOS.ino` line 350 -- `debugPrintf("CockpitOS version is %s\n", VERSION_CURRENT)` works automatically with the new stamped value
- `build.py` / firmware filenames -- no version added to filenames (they identify panel config, not software version)
- `CHANGELOG.md` -- still manually maintained
- `release.yml` -- kept as manual fallback, untouched

## Edge Cases
| Scenario | Behavior |
|----------|----------|
| No internet | 3s timeout, show local version only |
| Dirty working tree | Stamp includes `-dirty` suffix |
| Git not installed | Stamp is just the version number |
| `version.py` missing | Falls back to `"0.0.0"` |
| No GitHub releases yet | API returns 404, caught silently |
| Multiple PRs without version bump | Auto-release skips (tag exists) |
| Ctrl+C during compilation | `finally` block restores Config.h |

## Implementation Order
1. Create `version.py`
2. Add `stamp_version()` / `restore_version()` to `compiler/config.py`
3. Modify `do_compile()` in `compiler/cockpitos.py` to use stamp/restore
4. Add version display + GitHub check to main menu in `cockpitos.py`
5. Update `Config.h` line 24
6. Create `.github/workflows/auto-release.yml`
7. Update `.github/workflows/ci-build.yml` with stamp step
8. Update `CLAUDE.md`

## Verification
- Compile a firmware and check serial output shows `"1.3.0+<hash>"` at boot
- After compilation, verify Config.h is restored to `"dev"`
- Run compiler tool and verify version line appears in main menu
- Push to dev and verify CI build stamps correctly
- (Later) Merge PR to main with bumped version.py and verify GitHub release is created
