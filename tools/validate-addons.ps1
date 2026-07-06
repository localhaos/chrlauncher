[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$Root = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
$Addons = Join-Path $Root 'bin\Addons'
$Errors = New-Object System.Collections.Generic.List[string]

function Add-Err([string]$Message) {
    $Errors.Add($Message) | Out-Null
}

function Assert-File([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        Add-Err "Missing file: $Path"
    }
}

function Assert-Dir([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Container)) {
        Add-Err "Missing directory: $Path"
    }
}

function Read-Json([string]$Path) {
    Assert-File $Path
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    try {
        return Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json -ErrorAction Stop
    } catch {
        Add-Err "Invalid JSON: $Path :: $($_.Exception.Message)"
        return $null
    }
}

function Read-IniValue([string]$Path, [string]$Section, [string]$Key) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    $active = $false
    foreach ($line in Get-Content -LiteralPath $Path -Encoding UTF8) {
        $trim = $line.Trim()
        if (-not $trim -or $trim.StartsWith('#') -or $trim.StartsWith(';')) { continue }
        if ($trim -match '^\[(.+)\]$') {
            $active = ($matches[1] -ieq $Section)
            continue
        }
        if ($active -and $trim -match '^([^=]+)=(.*)$' -and $matches[1].Trim() -ieq $Key) {
            return $matches[2]
        }
    }
    return $null
}

Assert-Dir $Addons

$configCommand = Read-IniValue (Join-Path $Root 'bin\Config.ini') 'chrlauncher' 'ChromiumCommandLine'
if ($configCommand -notlike '*--disable-features=HardwareMediaKeyHandling,GlobalMediaControls*') {
    Add-Err 'Config.ini missing media/DevTools crash guard disable-features.'
}

$tabTools = Join-Path $Addons 'extensions\close-tabs-right'
Assert-Dir $tabTools
$tabManifestPath = Join-Path $tabTools 'manifest.json'
$tabManifest = Read-Json $tabManifestPath
if ($tabManifest) {
    if ($tabManifest.name -ne 'chrlauncher Tab Tools') { Add-Err 'Tab Tools manifest name mismatch.' }
    foreach ($file in @(
        'service_worker.js',
        'site_test_content.js',
        'chatgpt_download_content.js',
        'chatgpt_download_page_hook.js'
    )) {
        Assert-File (Join-Path $tabTools $file)
    }
    foreach ($stale in @('cf_manual_helper.js', 'slurg.html', 'slurg.css', 'slurg.js', 'slurg_data.js', 'devtools.html', 'devtools.js', 'devtools_panel.html', 'devtools_panel.css', 'devtools_panel.js')) {
        if (Test-Path -LiteralPath (Join-Path $tabTools $stale)) { Add-Err "Stale file still in Tab Tools: $stale" }
    }
    $manifestText = Get-Content -LiteralPath $tabManifestPath -Raw -Encoding UTF8
    if ($manifestText -match '"devtools_page"') { Add-Err 'Tab Tools must not register devtools_page in production.' }
    if ($manifestText -match 'http://\*/\*|https://\*/\*') { Add-Err 'Tab Tools should not request broad all-host permissions.' }
}

$devtoolsLag = Join-Path $Addons 'devtools_lag_guard'
Assert-Dir $devtoolsLag
$devtoolsManifest = Read-Json (Join-Path $devtoolsLag 'manifest.json')
if ($devtoolsManifest) {
    if ($devtoolsManifest.name -ne 'chrlauncher DevTools Lag Guard') { Add-Err 'DevTools Lag Guard manifest name mismatch.' }
    foreach ($file in @('devtools.html', 'devtools.js', 'devtools_panel.html', 'devtools_panel.css', 'devtools_panel.js', 'README.md')) {
        Assert-File (Join-Path $devtoolsLag $file)
    }
    $devtoolsText = Get-Content -LiteralPath (Join-Path $devtoolsLag 'devtools.js') -Raw -Encoding UTF8
    if ($devtoolsText -notlike '*facebook.com*') { Add-Err 'Optional DevTools Lag Guard must stay disabled on Facebook sessions.' }
}

$cf = Join-Path $Addons 'cf_manual_helper'
Assert-Dir $cf
$cfManifestPath = Join-Path $cf 'manifest.json'
$cfManifest = Read-Json $cfManifestPath
if ($cfManifest) {
    if ($cfManifest.name -ne 'chrlauncher CF Manual Helper') { Add-Err 'CF helper manifest name mismatch.' }
    foreach ($file in @('content.js', 'popup.html', 'popup.css', 'popup.js', 'README.md')) {
        Assert-File (Join-Path $cf $file)
    }
    $cfManifestText = Get-Content -LiteralPath $cfManifestPath -Raw -Encoding UTF8
    foreach ($requiredExclude in @('https://*.facebook.com/*', 'https://*.youtube.com/*', 'https://open.spotify.com/*')) {
        if ($cfManifestText -notlike "*$requiredExclude*") { Add-Err "CF helper missing heavy-site exclude: $requiredExclude" }
    }
    $cfText = Get-Content -LiteralPath (Join-Path $cf 'content.js') -Raw -Encoding UTF8
    foreach ($forbidden in @('document.cookie =', 'chrome.cookies.set', 'turnstile.click', '.click()')) {
        if ($cfText.Contains($forbidden)) { Add-Err "CF helper contains forbidden token/action: $forbidden" }
    }
    if ($cfText -notlike '*MAX_OBSERVER_LIFETIME_MS*') { Add-Err 'CF helper observer must remain time-bounded.' }
}

$slurg = Join-Path $Addons 'slurg'
Assert-Dir $slurg
foreach ($file in @('index.html', 'slurg.css', 'slurg.js', 'slurg_data.js', 'README.md', 'open-slurg.bat')) {
    Assert-File (Join-Path $slurg $file)
}
$slurgJs = Join-Path $slurg 'slurg.js'
if (Test-Path -LiteralPath $slurgJs) {
    $text = Get-Content -LiteralPath $slurgJs -Raw -Encoding UTF8
    if ($text -match 'chrome\.runtime|chrome\.tabs') { Add-Err 'Standalone Slurg must not depend on extension APIs.' }
}

$optimizer = Join-Path $Addons 'optimizer_guard'
Assert-Dir $optimizer
foreach ($file in @('optimizer_guard.ini', 'apply-optimizer-guard.ps1', 'apply-optimizer-guard.bat', 'README.md')) {
    Assert-File (Join-Path $optimizer $file)
}
$guardIni = Join-Path $optimizer 'optimizer_guard.ini'
$guardCommand = Read-IniValue $guardIni 'chrome_plus' 'command_line'
if (-not $guardCommand) { Add-Err 'optimizer_guard.ini missing [chrome_plus] command_line.' }
else {
    foreach ($required in @('addons\extensions\close-tabs-right', 'addons\cf_manual_helper', '--disk-cache-dir', '--disable-background-networking', '--disable-domain-reliability', '--disable-features=HardwareMediaKeyHandling,GlobalMediaControls')) {
        if ($guardCommand -notlike "*$required*") { Add-Err "optimizer_guard command_line missing: $required" }
    }
    if ($guardCommand -like '*devtools_lag_guard*') { Add-Err 'DevTools Lag Guard must not be loaded by default command_line.' }
}

$nativeReadme = Join-Path $Addons 'src\README.md'
Assert-File $nativeReadme
$nativeSources = @(
    'bin\Addons\src\optimizer_guard\optimizer_guard_bootstrap.c',
    'bin\Addons\src\cf_manual_helper\cf_manual_helper_bootstrap.c'
)
$props = Join-Path $Root 'Directory.Build.props'
Assert-File $props
$propsText = if (Test-Path -LiteralPath $props) { Get-Content -LiteralPath $props -Raw -Encoding UTF8 } else { '' }
foreach ($source in $nativeSources) {
    Assert-File (Join-Path $Root $source)
    if ($propsText -notlike "*$source*") { Add-Err "Native addon source not compiled in Directory.Build.props: $source" }
}

if ($Errors.Count) {
    Write-Host '[ADDONS] Validation failed:'
    foreach ($err in $Errors) { Write-Host " - $err" }
    throw "Addon validation failed with $($Errors.Count) error(s)."
}

Write-Host '[ADDONS] Validation OK.'
