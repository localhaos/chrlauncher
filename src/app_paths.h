// chrlauncher
// Copyright (c) 2015-2026 Henry++

#pragma once

#include "main.h"

BOOLEAN _app_is_absolute_path (
	_In_opt_ PR_STRING path
);

PR_STRING _app_create_app_relative_path (
	_In_ LPCWSTR relative_path
);

PR_STRING _app_get_addons_directory ();

PR_STRING _app_get_config_path ();

VOID _app_ensure_directory (
	_In_opt_ PR_STRING directory_path
);

PR_STRING _app_resolve_configured_path (
	_In_opt_ PR_STRING configured_path,
	_In_opt_ LPCWSTR default_path,
	_In_ BOOLEAN require_existing_directory
);

PR_STRING _app_resolve_legacy_parent_path (
	_In_opt_ PR_STRING configured_path,
	_In_ BOOLEAN require_existing_directory
);

PR_STRING _app_get_configured_directory (
	_In_ LPCWSTR key_name,
	_In_ LPCWSTR default_path,
	_In_ BOOLEAN require_existing_directory
);

VOID _app_initialize_addons_config ();

VOID _app_config_setstring_ex (
	_In_ LPCWSTR key_name,
	_In_opt_ LPCWSTR value,
	_In_opt_ LPCWSTR section_name
);

VOID _app_config_setboolean (
	_In_ LPCWSTR key_name,
	_In_ BOOLEAN value
);

VOID _app_config_setlong64 (
	_In_ LPCWSTR key_name,
	_In_ LONG64 value
);
