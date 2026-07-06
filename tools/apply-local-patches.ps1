[CmdletBinding()]
param(
    [string]$PatchDirectory = (Join-Path $PSScriptRoot '..\patches')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
Set-Location -LiteralPath $repoRoot

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'git.exe was not found in PATH; cannot apply repository patches.'
}

if (-not (Test-Path -LiteralPath $PatchDirectory)) {
    Write-Host "[PATCH] Directory not found, nothing to apply: $PatchDirectory"
    exit 0
}

$patches = Get-ChildItem -LiteralPath $PatchDirectory -Filter '*.patch' -File | Sort-Object Name

if (-not $patches) {
    Write-Host "[PATCH] No *.patch files in: $PatchDirectory"
    exit 0
}

foreach ($patch in $patches) {
    $patchPath = $patch.FullName
    Write-Host "[PATCH] Checking $($patch.Name)"

    & git apply --reverse --check --ignore-whitespace --whitespace=nowarn $patchPath 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "[PATCH] Already applied: $($patch.Name)"
        continue
    }

    & git apply --check --ignore-whitespace --whitespace=nowarn $patchPath
    if ($LASTEXITCODE -ne 0) {
        throw "Patch check failed: $patchPath"
    }

    & git apply --ignore-whitespace --whitespace=nowarn $patchPath
    if ($LASTEXITCODE -ne 0) {
        throw "Patch apply failed: $patchPath"
    }

    Write-Host "[PATCH] Applied: $($patch.Name)"
}
