[CmdletBinding()]
param(
    [string]$PatchDirectory = (Join-Path $PSScriptRoot '..\patches'),
    [string]$RoutineDirectory = (Join-Path $PSScriptRoot '..\..\routine')
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')

if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    throw 'git.exe was not found in PATH; cannot apply repository patches.'
}

function Invoke-GitPatchSet {
    param(
        [Parameter(Mandatory = $true)]
        [string]$TargetDirectory,
        [Parameter(Mandatory = $true)]
        [string]$PatchRoot,
        [Parameter(Mandatory = $true)]
        [string]$Label
    )

    if (-not (Test-Path -LiteralPath $TargetDirectory)) {
        Write-Host "[PATCH] $Label target not found, skipping: $TargetDirectory"
        return
    }

    if (-not (Test-Path -LiteralPath $PatchRoot)) {
        Write-Host "[PATCH] $Label patch directory not found, skipping: $PatchRoot"
        return
    }

    $patches = Get-ChildItem -LiteralPath $PatchRoot -Filter '*.patch' -File | Sort-Object Name
    if (-not $patches) {
        Write-Host "[PATCH] No $Label *.patch files in: $PatchRoot"
        return
    }

    Push-Location -LiteralPath $TargetDirectory
    try {
        foreach ($patch in $patches) {
            $patchPath = $patch.FullName
            Write-Host "[PATCH][$Label] Checking $($patch.Name)"

            & git apply --reverse --check --ignore-whitespace --whitespace=nowarn $patchPath 2>$null
            if ($LASTEXITCODE -eq 0) {
                Write-Host "[PATCH][$Label] Already applied: $($patch.Name)"
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

            Write-Host "[PATCH][$Label] Applied: $($patch.Name)"
        }
    }
    finally {
        Pop-Location
    }
}

function Disable-DefaultMinimizedStartup {
    $projectPath = Join-Path $repoRoot 'chrlauncher.vcxproj'
    if (-not (Test-Path -LiteralPath $projectPath)) {
        Write-Host "[PATCH] Project file not found, skipping startup visibility normalization: $projectPath"
        return
    }

    $old = Get-Content -LiteralPath $projectPath -Raw
    $new = $old -replace 'APP_STARTMINIMIZED;', ''

    if ($old -ne $new) {
        Set-Content -LiteralPath $projectPath -Value $new -NoNewline -Encoding UTF8
        Write-Host "[PATCH] Disabled default minimized startup in chrlauncher.vcxproj"
    } else {
        Write-Host "[PATCH] Default minimized startup already disabled in chrlauncher.vcxproj"
    }
}

Invoke-GitPatchSet `
    -TargetDirectory $repoRoot `
    -PatchRoot $PatchDirectory `
    -Label 'chrlauncher'

Invoke-GitPatchSet `
    -TargetDirectory $RoutineDirectory `
    -PatchRoot (Join-Path $PatchDirectory 'routine') `
    -Label 'routine'

Disable-DefaultMinimizedStartup
