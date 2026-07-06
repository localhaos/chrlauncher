[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
$main = Join-Path $repoRoot 'src\main.c'

if (-not (Test-Path -LiteralPath $main)) {
    throw "Missing source file: $main"
}

$text = Get-Content -LiteralPath $main -Raw

if ($text -match '_app_get_config_cwd_directory') {
    Write-Host '[PREP] CWD-Addons patch already present.'
    exit 0
}

$anchor = @'
PR_STRING _app_get_config_path ()
{
	return _app_create_app_relative_path (APP_ADDONS_DIRECTORY L"\\" APP_ADDONS_CONFIG_FILE);
}

'@

$insert = @'
PR_STRING _app_get_config_path ()
{
	return _app_create_app_relative_path (APP_ADDONS_DIRECTORY L"\\" APP_ADDONS_CONFIG_FILE);
}

PR_STRING _app_get_config_cwd_directory ()
{
	return _app_get_addons_directory ();
}

'@

if (-not $text.Contains($anchor)) {
    throw 'Cannot locate _app_get_config_path block.'
}

$text = $text.Replace($anchor, $insert)

$oldResolve = @'
	else
	{
		app_directory = _r_app_getdirectory ();

		if (app_directory)
'@

$newResolve = @'
	else
	{
		app_directory = _app_get_config_cwd_directory ();

		if (app_directory)
'@

if (-not $text.Contains($oldResolve)) {
    throw 'Cannot locate relative configured path resolver block.'
}

$text = $text.Replace($oldResolve, $newResolve)

Set-Content -LiteralPath $main -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] CWD-Addons config-only patch applied.'
