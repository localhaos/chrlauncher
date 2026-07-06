// chrlauncher
// Copyright (c) 2015-2026 Henry++

#include "routine.h"
#include "main.h"
#include "app_extensions.h"
#include "app_paths.h"

PR_STRING _app_get_existing_extension_directory (
	_In_ PR_STRING configured_directory
)
{
	PR_STRING extension_directory;

	if (_r_obj_isstringempty (configured_directory))
		return NULL;

	extension_directory = _app_resolve_configured_path (configured_directory, CHROMIUM_CLOSE_TABS_RIGHT_EXTENSION_DIRECTORY, TRUE);

	if (extension_directory)
		return extension_directory;

	return _app_resolve_legacy_parent_path (configured_directory, TRUE);
}

PR_STRING _app_create_close_tabs_right_extension_arguments ()
{
	PR_STRING configured_directory;
	PR_STRING extension_directory;
	PR_STRING arguments;

	configured_directory = _r_config_getstringexpand (L"ChromiumCloseTabsRightExtensionDirectory", CHROMIUM_CLOSE_TABS_RIGHT_EXTENSION_DIRECTORY);

	if (!configured_directory)
		return NULL;

	extension_directory = _app_get_existing_extension_directory (configured_directory);

	_r_obj_dereference (configured_directory);

	if (!extension_directory)
		return NULL;

	arguments = _r_format_string (L"--load-extension=\"%s\"", extension_directory->buffer);

	_r_obj_dereference (extension_directory);

	return arguments;
}
