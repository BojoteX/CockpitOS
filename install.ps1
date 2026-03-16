# CockpitOS Installer
# Usage: powershell -ExecutionPolicy Bypass -c "irm https://bojotex.github.io/CockpitOS/install.ps1 | iex"
#
# First-time install only. If CockpitOS is already installed, this script
# will notify the user and launch the existing Setup tool instead.

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# ── Branding ────────────────────────────────────────────────────────────
function Write-Banner {
    Write-Host ""
    Write-Host "  +--------------------------------------------+" -ForegroundColor Cyan
    Write-Host "  |         CockpitOS Installer                |" -ForegroundColor Cyan
    Write-Host "  |   Firmware platform for DCS cockpits       |" -ForegroundColor Cyan
    Write-Host "  +--------------------------------------------+" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Step($msg) {
    Write-Host "  [*] $msg" -ForegroundColor Yellow
}

function Write-Ok($msg) {
    Write-Host "  [+] $msg" -ForegroundColor Green
}

function Write-Err($msg) {
    Write-Host "  [!] $msg" -ForegroundColor Red
}

# ── Resolve install path ────────────────────────────────────────────────
# Uses the Windows known-folder API so OneDrive-redirected Documents works.
$Documents = [Environment]::GetFolderPath('MyDocuments')
$ArduinoDir = Join-Path $Documents "Arduino"
$InstallDir = Join-Path $ArduinoDir "CockpitOS"

# ── Already installed? ──────────────────────────────────────────────────
function Test-ExistingInstall {
    if (Test-Path (Join-Path $InstallDir "CockpitOS.ino")) {
        return $true
    }
    return $false
}

# ── Python check ────────────────────────────────────────────────────────
function Test-Python {
    try {
        $ver = & python --version 2>&1
        if ($ver -match 'Python (\d+)\.(\d+)') {
            $major = [int]$Matches[1]
            $minor = [int]$Matches[2]
            if ($major -ge 3 -and $minor -ge 10) {
                return $true
            }
        }
    } catch { }
    return $false
}

function Install-Python {
    Write-Step "Python >= 3.10 not found. Installing Python via winget..."
    try {
        & winget install --id Python.Python.3.12 --accept-source-agreements --accept-package-agreements 2>&1 | Out-Null
        # Refresh PATH so python is available in this session
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
        if (Test-Python) {
            Write-Ok "Python installed successfully."
            return $true
        }
    } catch { }
    return $false
}

# ── Download helpers ────────────────────────────────────────────────────
function Get-LatestVersion {
    $api = "https://api.github.com/repos/BojoteX/CockpitOS/releases/latest"
    $headers = @{ 'User-Agent' = 'CockpitOS-Installer' }
    $release = Invoke-RestMethod -Uri $api -Headers $headers -TimeoutSec 10
    return $release.tag_name  # e.g. "v1.3.0"
}

function Get-CockpitOS($tag) {
    $url = "https://github.com/BojoteX/CockpitOS/releases/download/$tag/CockpitOS.zip"
    $tmp = Join-Path $env:TEMP "CockpitOS_install.zip"

    Write-Step "Downloading CockpitOS $tag ..."

    $progressPref = $ProgressPreference
    $ProgressPreference = 'SilentlyContinue'  # drastically speeds up Invoke-WebRequest
    try {
        Invoke-WebRequest -Uri $url -OutFile $tmp -UseBasicParsing -TimeoutSec 300
    } finally {
        $ProgressPreference = $progressPref
    }

    if (!(Test-Path $tmp)) {
        throw "Download failed."
    }
    return $tmp
}

# ── Extract ─────────────────────────────────────────────────────────────
function Expand-CockpitOS($zipPath) {
    Write-Step "Extracting to $InstallDir ..."

    # Ensure parent directories exist
    if (!(Test-Path $ArduinoDir)) {
        New-Item -ItemType Directory -Path $ArduinoDir -Force | Out-Null
    }
    if (!(Test-Path $InstallDir)) {
        New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
    }

    Expand-Archive -Path $zipPath -DestinationPath $InstallDir -Force

    # Validate sentinel
    if (!(Test-Path (Join-Path $InstallDir "CockpitOS.ino"))) {
        throw "Extraction failed — CockpitOS.ino not found in $InstallDir"
    }

    # Cleanup
    Remove-Item $zipPath -Force -ErrorAction SilentlyContinue
    Write-Ok "Files extracted successfully."
}

# ── Launch setup ────────────────────────────────────────────────────────
function Start-Setup {
    $setup = Join-Path $InstallDir "Setup-START.py"
    if (Test-Path $setup) {
        Write-Step "Launching CockpitOS Setup..."
        Write-Host ""
        & python $setup
    } else {
        Write-Err "Setup-START.py not found at $setup"
        Write-Host "  Please run it manually from: $InstallDir" -ForegroundColor Gray
    }
}

# ════════════════════════════════════════════════════════════════════════
#  MAIN
# ════════════════════════════════════════════════════════════════════════

Write-Banner

# 1. Check for existing install
if (Test-ExistingInstall) {
    Write-Ok "CockpitOS is already installed at:"
    Write-Host "  $InstallDir" -ForegroundColor Gray
    Write-Host ""

    # Ensure Python is still available before launching Setup
    Write-Step "Checking Python..."
    if (Test-Python) {
        $pyVer = & python --version 2>&1
        Write-Ok "Found $pyVer"
    } else {
        if (!(Install-Python)) {
            Write-Err "Could not install Python. Please install Python 3.10+ manually:"
            Write-Host "  https://www.python.org/downloads/" -ForegroundColor Gray
            exit 1
        }
    }

    Write-Host "  To update, use the built-in updater from the CockpitOS menu." -ForegroundColor Yellow
    Write-Host "  Launching Setup now..." -ForegroundColor Yellow
    Write-Host ""
    Start-Setup
    exit 0
}

# 2. Check Python
Write-Step "Checking Python..."
if (Test-Python) {
    $pyVer = & python --version 2>&1
    Write-Ok "Found $pyVer"
} else {
    if (!(Install-Python)) {
        Write-Err "Could not install Python. Please install Python 3.10+ manually:"
        Write-Host "  https://www.python.org/downloads/" -ForegroundColor Gray
        exit 1
    }
}

# 3. Get latest release
Write-Step "Checking latest CockpitOS release..."
try {
    $tag = Get-LatestVersion
    Write-Ok "Latest release: $tag"
} catch {
    Write-Err "Could not reach GitHub. Check your internet connection."
    Write-Host "  Error: $_" -ForegroundColor Gray
    exit 1
}

# 4. Download
try {
    $zip = Get-CockpitOS $tag
} catch {
    Write-Err "Download failed."
    Write-Host "  Error: $_" -ForegroundColor Gray
    exit 1
}

# 5. Extract
try {
    Expand-CockpitOS $zip
} catch {
    Write-Err "Extraction failed."
    Write-Host "  Error: $_" -ForegroundColor Gray
    exit 1
}

# 6. Launch setup
Write-Ok "CockpitOS $tag installed to:"
Write-Host "  $InstallDir" -ForegroundColor Gray
Write-Host ""

Start-Setup
