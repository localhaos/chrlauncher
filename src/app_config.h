// chrlauncher
// Copyright (c) 2015-2026 Henry++

#pragma once

#include "main.h"

PR_STRING _app_get_parameters_path ();

PR_STRING _app_get_version_path ();

PR_STRING _app_config_getstring (
	_In_ LPCWSTR key_name,
	_In_opt_ LPCWSTR def_value
);

PR_STRING _app_config_getstringexpand (
	_In_ LPCWSTR key_name,
	_In_opt_ LPCWSTR def_value
);

BOOLEAN _app_config_getboolean (
	_In_ LPCWSTR key_name,
	_In_ BOOLEAN def_value
);

LONG _app_config_getlong (
	_In_ LPCWSTR key_name,
	_In_ LONG def_value
);

LONG64 _app_config_getlong64 (
	_In_ LPCWSTR key_name,
	_In_ LONG64 def_value
);
