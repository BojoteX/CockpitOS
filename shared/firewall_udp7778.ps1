#Requires -RunAsAdministrator
<#
.SYNOPSIS
    Creates a Windows Firewall inbound rule to allow UDP port 7778
    (DCS-BIOS communication for WiFi-connected ESP32 devices).

.DESCRIPTION
    Called by CockpitOS Setup-START.py via Start-Process -Verb RunAs
    to get a proper UAC elevation prompt. Writes "OK" or "FAIL: ..."
    to a result file so the calling Python process can read the outcome.

.PARAMETER ResultFile
    Path to a text file where the result ("OK" or "FAIL: message") is written.
#>
param(
    [Parameter(Mandatory=$true)]
    [string]$ResultFile
)

$RuleName = "CockpitOS - DCS-BIOS UDP 7778"
$Port     = 7778

try {
    netsh advfirewall firewall add rule `
        name="$RuleName" `
        dir=in `
        action=allow `
        protocol=udp `
        localport=$Port `
        profile="private,domain" `
        enable=yes | Out-Null

    if ($LASTEXITCODE -eq 0) {
        [IO.File]::WriteAllText($ResultFile, "OK")
    } else {
        [IO.File]::WriteAllText($ResultFile, "FAIL: netsh returned exit code $LASTEXITCODE")
    }
} catch {
    [IO.File]::WriteAllText($ResultFile, "FAIL: $($_.Exception.Message)")
}
