// chrlauncher
// Copyright (c) 2015-2026 Henry++

#include "app_config.h"
#include "app_paths.h"
#include "cfgread.h"
#include "xallocator/xallocator.c"
#include "cfgread.c"

static PR_STRING _app_config_read_file_string_section (
	_In_ PR_STRING path,
	_In_opt_ LPCWSTR section_name,
	_In_ LPCWSTR key_name
)
{
	PCFGREAD file;
	LPCWSTR value;
	PR_STRING result = NULL;

	if (_r_obj_isstringempty (path) || !key_name || !key_name[0])
		return NULL;

	file = cfgread_open (path);

	if (!file)
		return NULL;

	value = cfgread_get (file, section_name, key_name);

	if (value && value[0])
		result = _r_obj_createstring (value);

	cfgread_close (file);

	return result;
}

static PR_STRING _app_config_read_file_string (
	_In_ PR_STRING path,
	_In_ LPCWSTR key_name
)
{
	LPCWSTR current_section;
	PR_STRING value;

	if (_r_obj_isstringempty (path) || !key_name || !key_name[0])
		return NULL;

	current_section = _r_app_getnameshort ();

	if (current_section && current_section[0])
	{
		value = _app_config_read_file_string_section (path, current_section, key_name);

		if (value)
			return value;
	}

	if (!current_section || lstrcmpiW (current_section, APP_NAME_SHORT) != 0)
	{
		value = _app_config_read_file_string_section (path, APP_NAME_SHORT, key_name);

		if (value)
			return value;
	}

	// Unified portable configuration format:
	// all keys are read from Addons\Config.ini, including former Parameters.ini
	// and Version.ini keys. Flat files without sections are supported.
	return _app_config_read_file_string_section (path, NULL, key_name);
}

static PR_STRING _app_config_read_unified_string (
	_In_ LPCWSTR key_name
)
{
	PR_STRING path;
	PR_STRING value = NULL;

	path = _app_get_config_path ();

	if (path)
	{
		value = _app_config_read_file_string (path, key_name);
		_r_obj_dereference (path);
	}

	return value;
}

PR_STRING _app_config_getstring (
	_In_ LPCWSTR key_name,
	_In_opt_ LPCWSTR def_value
)
{
	PR_STRING value;

	value = _app_config_read_unified_string (key_name);

	if (value)
		return value;

	value = _r_config_getstring (key_name, NULL);

	if (value)
		return value;

	if (def_value && def_value[0])
		return _r_obj_createstring (def_value);

	return NULL;
}

PR_STRING _app_config_getstringexpand (
	_In_ LPCWSTR key_name,
	_In_opt_ LPCWSTR def_value
)
{
	PR_STRING expanded_value;
	PR_STRING value;
	NTSTATUS status;

	value = _app_config_getstring (key_name, def_value);

	if (!value)
		return NULL;

	status = _r_str_environmentexpandstring (NULL, &value->sr, &expanded_value);

	if (!NT_SUCCESS (status))
		expanded_value = _r_obj_createstring2 (&value->sr);

	_r_obj_dereference (value);

	return expanded_value;
}

BOOLEAN _app_config_getboolean (
	_In_ LPCWSTR key_name,
	_In_ BOOLEAN def_value
)
{
	PR_STRING value;
	BOOLEAN result;

	value = _app_config_getstring (key_name, def_value ? L"true" : L"false");

	if (!value)
		return def_value;

	result = _r_str_toboolean (&value->sr);

	_r_obj_dereference (value);

	return result;
}

LONG _app_config_getlong (
	_In_ LPCWSTR key_name,
	_In_ LONG def_value
)
{
	WCHAR default_value[64];
	PR_STRING value;
	LONG result;

	_r_str_fromlong (default_value, RTL_NUMBER_OF (default_value), def_value);
	value = _app_config_getstring (key_name, default_value);

	if (!value)
		return def_value;

	result = _r_str_tolong (&value->sr);

	_r_obj_dereference (value);

	return result;
}

LONG64 _app_config_getlong64 (
	_In_ LPCWSTR key_name,
	_In_ LONG64 def_value
)
{
	WCHAR default_value[64];
	PR_STRING value;
	LONG64 result;

	_r_str_fromlong64 (default_value, RTL_NUMBER_OF (default_value), def_value);
	value = _app_config_getstring (key_name, default_value);

	if (!value)
		return def_value;

	result = _r_str_tolong64 (&value->sr);

	_r_obj_dereference (value);

	return result;
}
