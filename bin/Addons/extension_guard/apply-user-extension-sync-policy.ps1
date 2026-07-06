[CmdletBinding()]
param(
    [switch]$WhatIfOnly,
    [switch]$ChromiumOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$policyRoots = @('HKCU:\Software\Policies\Chromium')
if (-not $ChromiumOnly) {
    $policyRoots += 'HKCU:\Software\Policies\Google\Chrome'
}

foreach ($root in $policyRoots) {
    $key = Join-Path $root 'SyncTypesListDisabled'
    if ($WhatIfOnly) {
        Write-Host "[WHATIF] create/update $key"
        Write-Host "[WHATIF] 1=extensions"
        Write-Host "[WHATIF] 2=apps"
        continue
    }

    New-Item -Path $key -Force | Out-Null
    New-ItemProperty -Path $key -Name '1' -Value 'extensions' -PropertyType String -Force | Out-Null
    New-ItemProperty -Path $key -Name '2' -Value 'apps' -PropertyType String -Force | Out-Null
    Write-Host "[OK] Applied user policy: $key"
}

Write-Host '[OK] Extension/app sync disabled by user policy for selected browser policy roots.'
Write-Host 'Restart all Chromium/Chrome processes before testing.'
