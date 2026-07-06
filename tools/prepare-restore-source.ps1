[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$root = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
$main = Join-Path $root 'src\main.c'

if (-not (Test-Path -LiteralPath $main)) {
    throw "Missing source file: $main"
}

$text = Get-Content -LiteralPath $main -Raw

if ($text -match '_app_get_portable_root_directory') {
    Write-Host '[PREP] Portable root source patch already present.'
    exit 0
}

$needle = @'
BOOLEAN _app_is_absolute_path (
	_In_ PR_STRING path
)
{
	if (_r_obj_isstringempty (path))
		return FALSE;

	return path->buffer[0] == OBJ_NAME_PATH_SEPARATOR ||
		(path->length >= 2 * sizeof (WCHAR) && path->buffer[1] == L':');
}

'@

$insert = @'
BOOLEAN _app_is_absolute_path (
	_In_ PR_STRING path
)
{
	if (_r_obj_isstringempty (path))
		return FALSE;

	return path->buffer[0] == OBJ_NAME_PATH_SEPARATOR ||
		(path->length >= 2 * sizeof (WCHAR) && path->buffer[1] == L':');
}

BOOLEAN _app_directory_has_addons_config (
	_In_ PR_STRING directory_path
)
{
	static LPCWSTR const config_names[] = {
		APP_ADDONS_DIRECTORY L"\\" APP_ADDONS_CONFIG_FILE,
		APP_ADDONS_DIRECTORY L"\\Config.ini",
		APP_ADDONS_DIRECTORY L"\\config.ini",
	};

	PR_STRING config_path;

	if (_r_obj_isstringempty (directory_path))
		return FALSE;

	for (ULONG_PTR i = 0; i < RTL_NUMBER_OF (config_names); i++)
	{
		config_path = _r_format_string (L"%s\\%s", directory_path->buffer, config_names[i]);

		if (!config_path)
			continue;

		if (_r_fs_exists (&config_path->sr))
		{
			_r_obj_dereference (config_path);

			return TRUE;
		}

		_r_obj_dereference (config_path);
	}

	return FALSE;
}

PR_STRING _app_get_portable_root_directory ()
{
	static R_INITONCE init_once = PR_INITONCE_INIT;
	static PR_STRING cached_root = NULL;

	PR_STRING app_directory;
	PR_STRING parent_candidate;
	PR_STRING parent_directory;
	NTSTATUS status;

	if (_r_initonce_begin (&init_once))
	{
		app_directory = _r_app_getdirectory ();

		if (app_directory)
		{
			if (_app_directory_has_addons_config (app_directory))
			{
				cached_root = (PR_STRING)_r_obj_reference (app_directory);
			}
			else
			{
				parent_candidate = _r_format_string (L"%s\\..", app_directory->buffer);

				if (parent_candidate)
				{
					status = _r_path_getfullpath (parent_candidate->buffer, &parent_directory);

					if (NT_SUCCESS (status))
					{
						if (_app_directory_has_addons_config (parent_directory))
						{
							cached_root = parent_directory;
						}
						else
						{
							_r_obj_dereference (parent_directory);
						}
					}

					_r_obj_dereference (parent_candidate);
				}
			}

			if (!cached_root)
				cached_root = (PR_STRING)_r_obj_reference (app_directory);
		}

		_r_initonce_end (&init_once);
	}

	return cached_root ? cached_root : _r_app_getdirectory ();
}

'@

if (-not $text.Contains($needle)) {
    throw 'Cannot locate _app_is_absolute_path block for portable root patch.'
}

$text = $text.Replace($needle, $insert)
$text = $text.Replace('app_directory = _r_app_getdirectory ();', 'app_directory = _app_get_portable_root_directory ();')

Set-Content -LiteralPath $main -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] Portable root source patch applied.'
