[CmdletBinding()]
param(
    [switch]$WhatIfOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$AddonDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = Resolve-Path -LiteralPath (Join-Path $AddonDir '..\..')
$ConfigPath = Join-Path $Root 'Config.ini'
$GuardIni = Join-Path $AddonDir 'optimizer_guard.ini'
$ChromePlusDir = Join-Path $Root 'addons\chrome_plus'
$ChromeBinDir = Join-Path $Root 'bin'
$ChromePlusIni = Join-Path $ChromePlusDir 'chrome++.ini'
$ChromeBinIni = Join-Path $ChromeBinDir 'chrome++.ini'

function Read-IniSection {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][string]$Section
    )

    $result = [ordered]@{}
    $active = $false

    foreach ($line in Get-Content -LiteralPath $Path -Encoding UTF8) {
        $trim = $line.Trim()
        if (-not $trim -or $trim.StartsWith(';') -or $trim.StartsWith('#')) { continue }
        if ($trim -match '^\[(.+)\]$') {
            $active = ($matches[1] -ieq $Section)
            continue
        }
        if ($active -and $trim -match '^([^=]+)=(.*)$') {
            $result[$matches[1].Trim()] = $matches[2]
        }
    }

    return $result
}

function Set-IniValue {
    param(
        [Parameter(Mandatory=$true)][string]$Path,
        [Parameter(Mandatory=$true)][string]$Section,
        [Parameter(Mandatory=$true)][string]$Key,
        [Parameter(Mandatory=$true)][AllowEmptyString()][string]$Value
    )

    if (-not (Test-Path -LiteralPath $Path)) {
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

if (-not (Test-Path -LiteralPath $GuardIni)) { throw "Missing $GuardIni" }
if (-not (Test-Path -LiteralPath $ConfigPath)) { throw "Missing $ConfigPath" }

$guard = Read-IniSection -Path $GuardIni -Section 'optimizer_guard'
if ($guard.Contains('Enabled') -and $guard.Enabled -notmatch '^(1|true|yes)$') {
    Write-Host '[SKIP] optimizer_guard is disabled.'
    exit 0
}

$chr = Read-IniSection -Path $GuardIni -Section 'chrlauncher'
foreach ($item in $chr.GetEnumerator()) {
    Set-IniValue -Path $ConfigPath -Section 'chrlauncher' -Key $item.Key -Value ([string]$item.Value)
}

$cp = Read-IniSection -Path $GuardIni -Section 'chrome_plus'
foreach ($target in @($ChromePlusIni, $ChromeBinIni)) {
    if (-not (Test-Path -LiteralPath (Split-Path -Parent $target))) { continue }
    if (-not (Test-Path -LiteralPath $target)) { New-Item -ItemType File -Path $target -Force | Out-Null }
    foreach ($item in $cp.GetEnumerator()) {
        Set-IniValue -Path $target -Section 'general' -Key $item.Key -Value ([string]$item.Value)
    }
}

Write-Host '[OK] Optimizer Guard applied.'
