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

$emptyLaunchMarker = 'pbi->is_hasurls || _app_config_getboolean (L"ChromiumUseLastProfileOnTaskbarLaunch", TRUE)'
$pickerFirstMarker = '_app_apply_profile_picker_hook (pbi);' + "`r`n" + "`r`n" + "`t" + 'if (!pbi->is_hasurls && !pbi->is_hasprofiledir)'

$text = $text.Replace(
    'if (_r_obj_isstringempty (pbi->args_str) || pbi->is_hasprofiledir)',
    'if (pbi->is_hasprofiledir)'
)

if (-not $text.Contains($emptyLaunchMarker)) {
    $text = [regex]::Replace(
        $text,
        'if\s*\(pbi->is_hasurls\)\s*\{\s*profile_argument\s*=\s*_app_get_profile_argument\s*\(pbi\);',
        "if (pbi->is_hasurls || _app_config_getboolean (L`"ChromiumUseLastProfileOnTaskbarLaunch`", TRUE))`r`n`t{`r`n`t`tprofile_argument = _app_get_profile_argument (pbi);",
        [System.Text.RegularExpressions.RegexOptions]::Singleline
    )
}

$text = [regex]::Replace(
    $text,
    'if\s*\(!_app_apply_last_profile_taskbar_hook\s*\(pbi\)\)\s*_app_apply_profile_picker_hook\s*\(pbi\);',
    "_app_apply_profile_picker_hook (pbi);`r`n`r`n`tif (!pbi->is_hasurls && !pbi->is_hasprofiledir)`r`n`t`t_app_apply_last_profile_taskbar_hook (pbi);",
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

if (-not $text.Contains($emptyLaunchMarker)) {
    throw 'Taskbar empty-launch profile patch did not apply.'
}

if (-not $text.Contains('if (!pbi->is_hasurls && !pbi->is_hasprofiledir)')) {
    throw 'Profile picker priority patch did not apply.'
}

Set-Content -LiteralPath $path -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] Taskbar profile patch applied: profile picker has priority when multiple profiles exist.'
