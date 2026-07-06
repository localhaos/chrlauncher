[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
$path = Join-Path $repoRoot 'src\main.c'

if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing source file: $path"
}

$text = Get-Content -LiteralPath $path -Raw

$patchedMarker = 'pbi->is_hasurls || _app_config_getboolean (L"ChromiumUseLastProfileOnTaskbarLaunch", TRUE)'

if ($text.Contains($patchedMarker)) {
    Write-Host '[PREP] Taskbar profile patch already present.'
    exit 0
}

$text = $text.Replace(
    'if (_r_obj_isstringempty (pbi->args_str) || pbi->is_hasprofiledir)',
    'if (pbi->is_hasprofiledir)'
)

$text = [regex]::Replace(
    $text,
    'if\s*\(pbi->is_hasurls\)\s*\{\s*profile_argument\s*=\s*_app_get_profile_argument\s*\(pbi\);',
    "if (pbi->is_hasurls || _app_config_getboolean (L`"ChromiumUseLastProfileOnTaskbarLaunch`", TRUE))`r`n`t{`r`n`t`tprofile_argument = _app_get_profile_argument (pbi);",
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if (-not $text.Contains($patchedMarker)) {
    throw 'Taskbar profile patch did not apply.'
}

Set-Content -LiteralPath $path -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] Taskbar empty-launch profile patch applied.'
