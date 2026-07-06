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

if ($text -match '_app_get_addons_root_directory') {
    Write-Host '[PREP] Addons-root patch already present.'
    exit 0
}

$absolutePathBlock = @'
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

$rootHelperBlock = @'
BOOLEAN _app_is_absolute_path (
	_In_ PR_STRING path
)
{
	if (_r_obj_isstringempty (path))
		return FALSE;

	return path->buffer[0] == OBJ_NAME_PATH_SEPARATOR ||
		(path->length >= 2 * sizeof (WCHAR) && path->buffer[1] == L':');
}

static BOOLEAN _app_is_addons_config_directory (
	_In_ PR_STRING directory_path
)
{
	PR_STRING config_path;
	BOOLEAN result = FALSE;

	if (_r_obj_isstringempty (directory_path))
		return FALSE;

	config_path = _r_format_string (L"%s\\%s", directory_path->buffer, APP_ADDONS_CONFIG_FILE);

	if (config_path)
	{
		result = _r_fs_exists (&config_path->sr);
		_r_obj_dereference (config_path);
	}

	if (result)
		return TRUE;

	config_path = _r_format_string (L"%s\\Config.ini", directory_path->buffer);

	if (config_path)
	{
		result = _r_fs_exists (&config_path->sr);
		_r_obj_dereference (config_path);
	}

	return result;
}

static PR_STRING _app_try_addons_root_candidate (
	_In_ LPCWSTR candidate_path
)
{
	PR_STRING resolved_path;
	NTSTATUS status;

	if (!candidate_path || !candidate_path[0])
		return NULL;

	status = _r_path_getfullpath (candidate_path, &resolved_path);

	if (!NT_SUCCESS (status))
		return NULL;

	if (_app_is_addons_config_directory (resolved_path))
		return resolved_path;

	_r_obj_dereference (resolved_path);

	return NULL;
}

PR_STRING _app_get_addons_root_directory ()
{
	static R_INITONCE init_once = PR_INITONCE_INIT;
	static PR_STRING cached_root = NULL;

	PR_STRING app_directory;
	PR_STRING candidate;
	PR_STRING root;

	if (_r_initonce_begin (&init_once))
	{
		app_directory = _r_app_getdirectory ();

		if (app_directory)
		{
			candidate = _r_format_string (L"%s\\%s", app_directory->buffer, APP_ADDONS_DIRECTORY);
			root = candidate ? _app_try_addons_root_candidate (candidate->buffer) : NULL;

			if (candidate)
				_r_obj_dereference (candidate);

			if (!root)
				root = _app_try_addons_root_candidate (app_directory->buffer);

			if (!root)
			{
				candidate = _r_format_string (L"%s\\..\\%s", app_directory->buffer, APP_ADDONS_DIRECTORY);
				root = candidate ? _app_try_addons_root_candidate (candidate->buffer) : NULL;

				if (candidate)
					_r_obj_dereference (candidate);
			}

			if (root)
				cached_root = root;
			else
				cached_root = (PR_STRING)_r_obj_reference (app_directory);
		}

		_r_initonce_end (&init_once);
	}

	return cached_root ? cached_root : _r_app_getdirectory ();
}

PR_STRING _app_create_addons_root_relative_path (
	_In_ LPCWSTR relative_path
)
{
	R_STRINGREF separator_sr = PR_STRINGREF_INIT (L"\\");
	PR_STRING root_directory;
	PR_STRING candidate;
	PR_STRING relative_string;
	PR_STRING resolved_path;
	NTSTATUS status;

	if (!relative_path || !relative_path[0])
		return NULL;

	root_directory = _app_get_addons_root_directory ();

	if (!root_directory)
		return _r_obj_createstring (relative_path);

	relative_string = _r_obj_createstring (relative_path);

	if (!relative_string)
		return NULL;

	candidate = _r_obj_concatstringrefs (3, &root_directory->sr, &separator_sr, &relative_string->sr);

	_r_obj_dereference (relative_string);

	if (!candidate)
		return NULL;

	status = _r_path_getfullpath (candidate->buffer, &resolved_path);

	_r_obj_dereference (candidate);

	if (!NT_SUCCESS (status))
		return _r_format_string (L"%s\\%s", root_directory->buffer, relative_path);

	return resolved_path;
}

'@

if (-not $text.Contains($absolutePathBlock)) {
    throw 'Cannot locate _app_is_absolute_path block.'
}

$oldAddonsDirectory = @'
PR_STRING _app_get_addons_directory ()
{
	return _app_create_app_relative_path (APP_ADDONS_DIRECTORY);
}

PR_STRING _app_get_config_path ()
{
	return _app_create_app_relative_path (APP_ADDONS_DIRECTORY L"\\" APP_ADDONS_CONFIG_FILE);
}
'@

$newAddonsDirectory = @'
PR_STRING _app_get_addons_directory ()
{
	return _app_get_addons_root_directory ();
}

PR_STRING _app_get_config_path ()
{
	return _app_create_addons_root_relative_path (APP_ADDONS_CONFIG_FILE);
}
'@

if (-not $text.Contains($oldAddonsDirectory)) {
    throw 'Cannot locate Addons/config path functions.'
}

$text = $text.Replace('app_directory = _r_app_getdirectory ();', 'app_directory = _app_get_addons_root_directory ();')
$text = $text.Replace($absolutePathBlock, $rootHelperBlock)
$text = $text.Replace($oldAddonsDirectory, $newAddonsDirectory)

Set-Content -LiteralPath $main -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] Addons-root source patch applied.'
