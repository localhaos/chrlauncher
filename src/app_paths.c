// chrlauncher
// Copyright (c) 2015-2026 Henry++

#include "routine.h"
#include "main.h"
#include "app_paths.h"

#include <shlobj.h>

BOOLEAN _app_is_absolute_path (
	_In_ PR_STRING path
)
{
	if (_r_obj_isstringempty (path))
		return FALSE;

	return path->buffer[0] == OBJ_NAME_PATH_SEPARATOR ||
		(path->length >= 2 * sizeof (WCHAR) && path->buffer[1] == L':');
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
	return _app_create_app_relative_path (APP_ADDONS_DIRECTORY L"\\" APP_ADDONS_CONFIG_FILE);
}

VOID _app_ensure_directory (
	_In_ PR_STRING directory_path
)
{
	if (_r_obj_isstringempty (directory_path))
		return;

	if (!_r_fs_exists (&directory_path->sr))
		SHCreateDirectoryExW (NULL, directory_path->buffer, NULL);
}

PR_STRING _app_resolve_configured_path (
	_In_opt_ PR_STRING configured_path,
	_In_ LPCWSTR default_path,
	_In_ BOOLEAN require_existing_directory
)
{
	R_STRINGREF separator_sr = PR_STRINGREF_INIT (L"\\");
	PR_STRING app_directory;
	PR_STRING candidate;
	PR_STRING path;
	PR_STRING resolved_path;
	NTSTATUS status;

	if (_r_obj_isstringempty (configured_path))
		path = _r_obj_createstring (default_path);
	else
		path = (PR_STRING)_r_obj_reference (configured_path);

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

		if (app_directory)
		{
			candidate = _r_obj_concatstringrefs (
				3,
				&app_directory->sr,
				&separator_sr,
				&path->sr
			);
		}
		else
		{
			candidate = (PR_STRING)_r_obj_reference (path);
		}

		if (candidate)
		{
			status = _r_path_getfullpath (candidate->buffer, &resolved_path);

			if (!NT_SUCCESS (status))
				resolved_path = (PR_STRING)_r_obj_reference (candidate);

			_r_obj_dereference (candidate);
		}
		else
		{
			resolved_path = NULL;
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
	_In_ PR_STRING configured_path,
	_In_ BOOLEAN require_existing_directory
)
{
	PR_STRING app_directory;
	PR_STRING candidate;
	PR_STRING resolved_path;
	NTSTATUS status;

	if (_r_obj_isstringempty (configured_path) || _app_is_absolute_path (configured_path))
		return NULL;

	app_directory = _r_app_getdirectory ();

	if (!app_directory)
		return NULL;

	candidate = _r_format_string (L"%s\\..\\%s", app_directory->buffer, configured_path->buffer);

	if (!candidate)
		return NULL;

	status = _r_path_getfullpath (candidate->buffer, &resolved_path);

	_r_obj_dereference (candidate);

	if (!NT_SUCCESS (status))
		return NULL;

	if (require_existing_directory && (!_r_fs_exists (&resolved_path->sr) || !_r_fs_isdirectory (&resolved_path->sr)))
	{
		_r_obj_dereference (resolved_path);

		return NULL;
	}

	return resolved_path;
}

PR_STRING _app_get_configured_directory (
	_In_ LPCWSTR key_name,
	_In_ LPCWSTR default_path,
	_In_ BOOLEAN require_existing_directory
)
{
	PR_STRING configured_path;
	PR_STRING resolved_path;

	configured_path = _r_config_getstringexpand (key_name, default_path);
	resolved_path = _app_resolve_configured_path (configured_path, default_path, require_existing_directory);

	if (!resolved_path)
		resolved_path = _app_resolve_legacy_parent_path (configured_path, require_existing_directory);

	if (configured_path)
		_r_obj_dereference (configured_path);

	return resolved_path;
}

VOID _app_copy_legacy_config_if_needed (
	_In_ PR_STRING config_path
)
{
	PR_STRING app_directory;
	PR_STRING legacy_config_path;

	if (_r_obj_isstringempty (config_path) || _r_fs_exists (&config_path->sr))
		return;

	app_directory = _r_app_getdirectory ();

	if (!app_directory)
		return;

	legacy_config_path = _r_format_string (L"%s\\%s.ini", app_directory->buffer, APP_NAME_SHORT);

	if (legacy_config_path)
	{
		if (_r_fs_exists (&legacy_config_path->sr))
			_r_fs_copyfile (&legacy_config_path->sr, &config_path->sr, FALSE);

		_r_obj_dereference (legacy_config_path);
	}
}

VOID _app_initialize_addons_config ()
{
	PR_HASHTABLE config_table = NULL;
	PR_STRING addons_directory;
	PR_STRING config_path;

	addons_directory = _app_get_addons_directory ();

	if (addons_directory)
	{
		_app_ensure_directory (addons_directory);
		_r_obj_dereference (addons_directory);
	}

	config_path = _app_get_config_path ();

	if (!config_path)
		return;

	_app_copy_legacy_config_if_needed (config_path);

	_r_parseini (config_path, &config_table, NULL);

	if (config_table)
	{
		_r_queuedlock_acquireexclusive (&app_global.config.lock);
		_r_obj_movereference (&app_global.config.table, config_table);
		_r_queuedlock_releaseexclusive (&app_global.config.lock);
	}

	_r_obj_dereference (config_path);
}

VOID _app_config_setstring_ex (
	_In_ LPCWSTR key_name,
	_In_opt_ LPCWSTR value,
	_In_opt_ LPCWSTR section_name
)
{
	PR_OBJECT_POINTER object_ptr;
	WCHAR section_string[128];
	PR_STRING config_path;
	PR_STRING section_string_full;
	ULONG hash_code;

	if (!key_name)
		return;

	if (!app_global.config.table)
		_app_initialize_addons_config ();

	if (!app_global.config.table)
		return;

	if (section_name)
	{
		_r_str_printf (section_string, RTL_NUMBER_OF (section_string), L"%s\\%s", APP_NAME_SHORT, section_name);

		section_string_full = _r_obj_concatstrings (
			5,
			APP_NAME_SHORT,
			L"\\",
			section_name,
			L"\\",
			key_name
		);
	}
	else
	{
		_r_str_copy (section_string, RTL_NUMBER_OF (section_string), APP_NAME_SHORT);

		section_string_full = _r_obj_concatstrings (
			3,
			APP_NAME_SHORT,
			L"\\",
			key_name
		);
	}

	if (!section_string_full)
		return;

	hash_code = _r_str_gethash2 (&section_string_full->sr, TRUE);

	_r_obj_dereference (section_string_full);

	if (!hash_code)
		return;

	_r_queuedlock_acquireshared (&app_global.config.lock);
	object_ptr = _r_obj_findhashtable (app_global.config.table, hash_code);
	_r_queuedlock_releaseshared (&app_global.config.lock);

	if (!object_ptr)
	{
		_r_queuedlock_acquireexclusive (&app_global.config.lock);
		object_ptr = _r_obj_addhashtablepointer (app_global.config.table, hash_code, NULL);
		_r_queuedlock_releaseexclusive (&app_global.config.lock);
	}

	if (!object_ptr)
		return;

	if (value)
		_r_obj_movereference (&object_ptr->object_body, _r_obj_createstring (value));
	else
		_r_obj_clearreference (&object_ptr->object_body);

	config_path = _app_get_config_path ();

	if (config_path)
	{
		WritePrivateProfileStringW (section_string, key_name, _r_obj_getstring (object_ptr->object_body), config_path->buffer);
		_r_obj_dereference (config_path);
	}
}

VOID _app_config_setboolean (
	_In_ LPCWSTR key_name,
	_In_ BOOLEAN value
)
{
	_app_config_setstring_ex (key_name, value ? L"true" : L"false", NULL);
}

VOID _app_config_setlong64 (
	_In_ LPCWSTR key_name,
	_In_ LONG64 value
)
{
	WCHAR value_text[64];

	_r_str_fromlong64 (value_text, RTL_NUMBER_OF (value_text), value);
	_app_config_setstring_ex (key_name, value_text, NULL);
}
