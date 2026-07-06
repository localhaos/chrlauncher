[CmdletBinding()]
param(
    [switch]$WhatIfOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$AddonDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = Resolve-Path -LiteralPath (Join-Path $AddonDir '..\..')
$ConfigPath = Join-Path $Root 'Config.ini'
$ChromePlusDir = Join-Path $Root 'addons\chrome_plus'
$ChromeBinDir = Join-Path $Root 'bin'
$ChromePlusIni = Join-Path $ChromePlusDir 'chrome++.ini'
$ChromeBinIni = Join-Path $ChromeBinDir 'chrome++.ini'

$ProtectedChromiumCommandLine = '--flag-switches-begin --test-type --user-data-dir=..\Data --disk-cache-dir=..\Cache --no-default-browser-check --disable-logging --no-report-upload --disable-background-networking --disable-domain-reliability --disable-features=HardwareMediaKeyHandling,GlobalMediaControls --disable-sync-types=Extensions,Apps,ExtensionSettings --flag-switches-end'
$ProtectedChromePlusCommandLine = '--load-extension="%app%\..\addons\extensions\close-tabs-right,%app%\..\addons\cf_manual_helper" --disk-cache-dir="%app%\..\Cache" --disable-background-networking --disable-domain-reliability --disable-features=HardwareMediaKeyHandling,GlobalMediaControls --disable-sync-types=Extensions,Apps,ExtensionSettings'

function Set-IniValue {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][string]$Section,
        [Parameter(Mandatory=$true)][string]$Key,
        [Parameter(Mandatory=$true)][AllowEmptyString()][string]$Value
    )

    if (-not (Test-Path -LiteralPath $Path)) {
        if ($WhatIfOnly) { Write-Host "[WHATIF] create $Path"; return }
        New-Item -ItemType File -Path $Path -Force | Out-Null
        Set-Content -LiteralPath $Path -Value "[$Section]`r`n" -Encoding UTF8
    }

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.AddRange([string[]](Get-Content -LiteralPath $Path -Encoding UTF8))

    $sectionStart = -1
    $sectionEnd = $lines.Count

    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\[(.+)\]\s*$') {
            if ($matches[1] -ieq $Section) {
                $sectionStart = $i
                $sectionEnd = $lines.Count
                for ($j = $i + 1; $j -lt $lines.Count; $j++) {
                    if ($lines[$j] -match '^\[.+\]\s*$') { $sectionEnd = $j; break }
                }
                break
            }
        }
    }

    if ($sectionStart -lt 0) {
        if ($lines.Count -gt 0 -and $lines[$lines.Count - 1].Trim()) { $lines.Add('') }
        $lines.Add("[$Section]")
        $lines.Add("$Key=$Value")
    } else {
        $keyLine = -1
        for ($i = $sectionStart + 1; $i -lt $sectionEnd; $i++) {
            if ($lines[$i] -match '^\s*([^=]+)\s*=') {
                if ($matches[1].Trim() -ieq $Key) { $keyLine = $i; break }
            }
        }
        if ($keyLine -ge 0) { $lines[$keyLine] = "$Key=$Value" }
        else { $lines.Insert($sectionEnd, "$Key=$Value") }
    }

    if ($WhatIfOnly) { Write-Host "[WHATIF] $Path [$Section] $Key=$Value"; return }
    Set-Content -LiteralPath $Path -Value $lines -Encoding UTF8
}

foreach ($required in @(
    (Join-Path $Root 'addons\extensions\close-tabs-right\manifest.json'),
    (Join-Path $Root 'addons\cf_manual_helper\manifest.json')
)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Missing required local addon file: $required"
    }
}

Set-IniValue -Path $ConfigPath -Section 'chrlauncher' -Key 'ChromiumCommandLine' -Value $ProtectedChromiumCommandLine
Set-IniValue -Path $ConfigPath -Section 'chrlauncher' -Key 'ChromePlusDirectory' -Value '.\addons\chrome_plus'

foreach ($target in @($ChromePlusIni, $ChromeBinIni)) {
    $dir = Split-Path -Parent $target
    if (-not (Test-Path -LiteralPath $dir)) { continue }
    Set-IniValue -Path $target -Section 'general' -Key 'data_dir' -Value '%app%\..\Data'
    Set-IniValue -Path $target -Section 'general' -Key 'cache_dir' -Value '%app%\..\Cache'
    Set-IniValue -Path $target -Section 'general' -Key 'command_line' -Value $ProtectedChromePlusCommandLine
}

Write-Host '[OK] Extension Guard repair applied.'
Write-Host 'Close all Chromium/Chrome processes and start chrlauncher again.'
