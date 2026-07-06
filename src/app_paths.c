// chrlauncher
// Copyright (c) 2015-2026 Henry++

#include "app_paths.h"
#include "app_config.h"

#include <shlobj.h>

static BOOLEAN _app_path_is_legacy_bin_directory (
	_In_opt_ PR_STRING path
)
{
	if (_r_obj_isstringempty (path))
		return FALSE;

	return (lstrcmpiW (path->buffer, L"bin") == 0 ||
		lstrcmpiW (path->buffer, L".\\bin") == 0 ||
		lstrcmpiW (path->buffer, L"./bin") == 0);
}

static BOOLEAN _app_directory_is_named_bin (
	_In_opt_ PR_STRING directory
)
{
	LPCWSTR name;

	if (_r_obj_isstringempty (directory))
		return FALSE;

	name = PathFindFileNameW (directory->buffer);

	return name && lstrcmpiW (name, L"bin") == 0;
}

BOOLEAN _app_is_absolute_path (
	_In_opt_ PR_STRING path
)
{
	if (_r_obj_isstringempty (path))
		return FALSE;

	return PathIsRelativeW (path->buffer) == FALSE;
}

PR_STRING _app_create_app_relative_path (
	_In_ LPCWSTR relative_path
)
{
	R_STRINGREF separator_sr = PR_STRINGREF_INIT (L"\\");
	PR_STRING app_directory;
	PR_STRING candidate;
	PR_STRING relative_string;
	PR_STRING resolved_path;
	NTSTATUS status;

	if (!relative_path || !relative_path[0])
		return NULL;

	app_directory = _r_app_getdirectory ();

	if (!app_directory)
		return _r_obj_createstring (relative_path);

	relative_string = _r_obj_createstring (relative_path);

	if (!relative_string)
		return NULL;

	candidate = _r_obj_concatstringrefs (
		3,
		&app_directory->sr,
		&separator_sr,
		&relative_string->sr
	);

	_r_obj_dereference (relative_string);

	if (!candidate)
		return NULL;

	status = _r_path_getfullpath (candidate->buffer, &resolved_path);

	_r_obj_dereference (candidate);

	if (!NT_SUCCESS (status))
		return _r_format_string (L"%s\\%s", app_directory->buffer, relative_path);

	return resolved_path;
}

PR_STRING _app_get_addons_directory ()
{
	return _app_create_app_relative_path (APP_ADDONS_DIRECTORY);
}

PR_STRING _app_get_config_path ()
{
	PR_STRING root_config;
	PR_STRING addons_config;

	// launchbro-compatible layout: keep the primary INI next to the launcher EXE.
	// This avoids special Addons-root/cwd assumptions and makes relative paths such
	// as .\bin resolve from the launcher directory.
	root_config = _app_create_app_relative_path (APP_ADDONS_CONFIG_FILE);

	if (root_config && _r_fs_exists (&root_config->sr))
		return root_config;

	addons_config = _app_create_app_relative_path (APP_ADDONS_DIRECTORY L"\\" APP_ADDONS_CONFIG_FILE);

	if (root_config)
		_r_obj_dereference (root_config);

	return addons_config;
}

VOID _app_ensure_directory (
	_In_opt_ PR_STRING directory_path
)
{
	if (_r_obj_isstringempty (directory_path))
		return;

	if (!_r_fs_exists (&directory_path->sr))
		SHCreateDirectoryExW (NULL, directory_path->buffer, NULL);
}

PR_STRING _app_resolve_configured_path (
	_In_opt_ PR_STRING configured_path,
	_In_opt_ LPCWSTR default_path,
	_In_ BOOLEAN require_existing_directory
)
{
	R_STRINGREF separator_sr = PR_STRINGREF_INIT (L"\\");
	PR_STRING app_directory;
	PR_STRING candidate = NULL;
	PR_STRING path;
	PR_STRING resolved_path = NULL;
	NTSTATUS status;

	if (_r_obj_isstringempty (configured_path))
	{
		if (!default_path || !default_path[0])
			return NULL;

		path = _r_obj_createstring (default_path);
	}
	else
	{
		path = (PR_STRING)_r_obj_reference (configured_path);
	}

	if (!path)
		return NULL;

	if (_app_is_absolute_path (path))
	{
		status = _r_path_getfullpath (path->buffer, &resolved_path);

		if (!NT_SUCCESS (status))
			resolved_path = (PR_STRING)_r_obj_reference (path);
	}
	else
	{
		app_directory = _r_app_getdirectory ();

		if (!app_directory)
		{
			resolved_path = (PR_STRING)_r_obj_reference (path);
		}
		else
		{
			candidate = _r_obj_concatstringrefs (
				3,
				&app_directory->sr,
				&separator_sr,
				&path->sr
			);

			if (candidate)
			{
				status = _r_path_getfullpath (candidate->buffer, &resolved_path);

				if (!NT_SUCCESS (status))
					resolved_path = (PR_STRING)_r_obj_reference (candidate);

				_r_obj_dereference (candidate);
			}

			if (resolved_path && !_r_fs_exists (&resolved_path->sr) && _app_path_is_legacy_bin_directory (path) && _app_directory_is_named_bin (app_directory))
			{
				_r_obj_dereference (resolved_path);
				resolved_path = (PR_STRING)_r_obj_reference (app_directory);
			}
		}
	}

	_r_obj_dereference (path);

	if (!resolved_path)
		return NULL;

	_r_str_trimstring (&resolved_path->sr, &separator_sr, 0);

	if (require_existing_directory && (!_r_fs_exists (&resolved_path->sr) || !_r_fs_isdirectory (&resolved_path->sr)))
	{
		_r_obj_dereference (resolved_path);

		return NULL;
	}

	return resolved_path;
}

PR_STRING _app_resolve_legacy_parent_path (
	_In_opt_ PR_STRING configured_path,
	_In_ LPCWSTR default_path
)
{
	return _app_resolve_configured_path (configured_path, default_path, TRUE);
}
