[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
$config = Join-Path $repoRoot 'bin\Addons\config.ini'

if (-not (Test-Path -LiteralPath $config)) {
    throw "Missing config file: $config"
}

$text = Get-Content -LiteralPath $config -Raw

$replacements = [ordered]@{
    'ChromiumEnableLosslessOptimization=true' = 'ChromiumEnableLosslessOptimization=false'
    'ChromiumEnableTextPerformanceFixes=true' = 'ChromiumEnableTextPerformanceFixes=false'
    'ChromiumTextPerformanceCommandLine=--disable-translate --skia-font-cache-limit-mb=64 --skia-resource-cache-limit-mb=128' = 'ChromiumTextPerformanceCommandLine='
    'ChromiumIgnoreGpuBlocklist=true' = 'ChromiumIgnoreGpuBlocklist=false'
    'ChromiumEnableScrollableTabs=true' = 'ChromiumEnableScrollableTabs=false'
    'ChromiumEnableCloseTabsRightExtension=true' = 'ChromiumEnableCloseTabsRightExtension=false'
    'ChromiumEnableAutofillPasswordFixes=true' = 'ChromiumEnableAutofillPasswordFixes=false'
    'ChromiumEnableDnsBlocklistUrl=true' = 'ChromiumEnableDnsBlocklistUrl=false'
    'ChromiumDnsBlocklistUrl=https://sky.rethinkdns.com/1:gIAAgBgA' = 'ChromiumDnsBlocklistUrl='
    'ChromiumDnsBlocklistMaxRules=256' = 'ChromiumDnsBlocklistMaxRules=64'
    'ChromiumDnsBlocklistMaxCommandLineChars=12000' = 'ChromiumDnsBlocklistMaxCommandLineChars=4096'
}

foreach ($entry in $replacements.GetEnumerator()) {
    $text = $text.Replace($entry.Key, $entry.Value)
}

$stamp = @'

# Safe/low-memory restore profile:
# This artifact intentionally disables optional startup/GPU/extension flags that can
# increase memory pressure on systems with a small or disabled pagefile.
# For RethinkDNS DoH, do not put sky.rethinkdns.com/1:... into ChromiumDnsBlocklistUrl.
# Use ChromiumEnableSecureDns/ChromiumSecureDnsUrl only in v7 builds that support it.
'@

if ($text -notmatch 'Safe/low-memory restore profile') {
    $text += $stamp
}

Set-Content -LiteralPath $config -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] Low-memory config applied.'
