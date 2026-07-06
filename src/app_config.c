// chrlauncher
// Copyright (c) 2015-2026 Henry++

#include "app_config.h"
#include "app_paths.h"
#include "cfgread.h"

typedef enum _APP_CONFIG_FILE_KIND
{
	AppConfigFileBase = 0,
	AppConfigFileParameters,
	AppConfigFileVersion,
} APP_CONFIG_FILE_KIND;

static LPCWSTR const AppParameterKeys[] = {
	L"ChromiumCommandLine",
	L"ChromiumSpoofRegion",
	L"ChromiumSpoofUseWindowsLocale",
	L"ChromiumSpoofRegionLocale",
	L"ChromiumSpoofRegionAcceptLanguage",
	L"ChromiumSpoofGeolocation",
	L"ChromiumSpoofGeolocationCommandLine",
	L"ChromiumSpoofTimeZone",
	L"ChromiumSpoofTimeZoneId",
	L"ChromiumSpoofTimeZoneCommandLine",
	L"ChromiumSpoofHardwareId",
	L"ChromiumSpoofHardwareIdCommandLine",
	L"ChromiumEnableLosslessOptimization",
	L"ChromiumLosslessOptimizationCommandLine",
	L"ChromiumLosslessOptimizationFeatures",
	L"ChromiumEnableTextPerformanceFixes",
	L"ChromiumTextPerformanceCommandLine",
	L"ChromiumTextPerformanceDisableFeatures",
	L"ChromiumEnableAggressiveTextPerformanceFixes",
	L"ChromiumAggressiveTextPerformanceCommandLine",
	L"ChromiumEnableBackgroundResourceSaving",
	L"ChromiumBackgroundResourceSavingCommandLine",
	L"ChromiumBackgroundResourceSavingFeatures",
	L"ChromiumEnableRendererSafety",
	L"ChromiumRendererSafetyCommandLine",
	L"ChromiumEnableWindows11Features",
	L"ChromiumWindows11CommandLine",
	L"ChromiumWindows11Features",
	L"ChromiumEnableDirectX",
	L"ChromiumDirectXCommandLine",
	L"ChromiumEnableVulkan",
	L"ChromiumVulkanCommandLine",
	L"ChromiumVulkanFeatures",
	L"ChromiumEnableMultithreadedRaster",
	L"ChromiumMultithreadingCommandLine",
	L"ChromiumIgnoreGpuBlocklist",
	L"ChromiumIgnoreGpuBlocklistCommandLine",
	L"ChromiumEnableHardwareAcceleration",
	L"ChromiumHardwareAccelerationCommandLine",
	L"ChromiumEnableScrollableTabs",
	L"ChromiumScrollableTabsFeatures",
	L"ChromiumEnableCloseTabsRightExtension",
	L"ChromiumCloseTabsRightExtensionDirectory",
	L"ChromiumEnableAutofillPasswordFixes",
	L"ChromiumAutofillPasswordCommandLine",
	L"ChromiumEnableQuic",
	L"ChromiumQuicCommandLine",
	L"ChromiumEnableDnsOptions",
	L"ChromiumDnsCommandLine",
	L"ChromiumEnableSecureDns",
	L"ChromiumSecureDnsUrl",
	L"ChromiumEnableDnsBlocklistUrl",
	L"ChromiumDnsBlocklistUrl",
	L"ChromiumDnsBlocklistCache",
	L"ChromiumDnsBlocklistCacheDirectory",
	L"ChromiumDnsBlocklistCacheMaxAgeHours",
	L"ChromiumDnsBlocklistSink",
	L"ChromiumDnsBlocklistIncludeSubdomains",
	L"ChromiumDnsBlocklistMaxRules",
	L"ChromiumDnsBlocklistMaxCommandLineChars",
	L"ChromiumEnableGoogleWebStoreFix",
	L"ChromiumGoogleWebStoreCommandLine",
	L"ChromiumGoogleWebStoreDisableFeatures",
	L"ChromiumEnableCast",
	L"ChromiumCastCommandLine",
	L"ChromiumCastFeatures",
	L"ChromiumCastDisableFeatures",
};

static LPCWSTR const AppVersionKeys[] = {
	L"ChromiumUpdateUrl",
	L"ChromiumArchitecture",
	L"ChromiumAutoDownload",
	L"ChromiumBringToFront",
	L"ChromiumWaitForDownloadEnd",
	L"ChromiumUpdateOnly",
	L"ChromiumType",
	L"ChromiumCheckPeriod",
};

static BOOLEAN _app_config_key_in_list (
	_In_ LPCWSTR key_name,
	_In_reads_ (count) LPCWSTR const *keys,
	_In_ ULONG_PTR count
)
{
	if (!key_name || !key_name[0])
		return FALSE;

	for (ULONG_PTR i = 0; i < count; i++)
	{
		if (lstrcmpiW (key_name, keys[i]) == 0)
			return TRUE;
	}

	return FALSE;
}

static APP_CONFIG_FILE_KIND _app_config_get_file_kind_for_key (
	_In_ LPCWSTR key_name
)
{
	if (_app_config_key_in_list (key_name, AppVersionKeys, RTL_NUMBER_OF (AppVersionKeys)))
		return AppConfigFileVersion;

	if (_app_config_key_in_list (key_name, AppParameterKeys, RTL_NUMBER_OF (AppParameterKeys)))
		return AppConfigFileParameters;

	return AppConfigFileBase;
}

PR_STRING _app_get_parameters_path ()
{
	return _app_create_app_relative_path (APP_ADDONS_DIRECTORY L"\\" APP_ADDONS_PARAMETERS_FILE);
}

PR_STRING _app_get_version_path ()
{
	return _app_create_app_relative_path (APP_ADDONS_DIRECTORY L"\\" APP_ADDONS_VERSION_FILE);
}

static PR_STRING _app_config_create_path_for_kind (
	_In_ APP_CONFIG_FILE_KIND kind
)
{
	switch (kind)
	{
		case AppConfigFileParameters:
			return _app_get_parameters_path ();

		case AppConfigFileVersion:
			return _app_get_version_path ();

		default:
			return _app_get_config_path ();
	}
}

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

	// Split portable configs in this fork are intentionally flat files.
	// Unlike GetPrivateProfileStringW, the local parser can read global keys.
	return _app_config_read_file_string_section (path, NULL, key_name);
}

static PR_STRING _app_config_read_split_string (
	_In_ LPCWSTR key_name
)
{
	APP_CONFIG_FILE_KIND kind;
	PR_STRING path;
	PR_STRING value = NULL;

	kind = _app_config_get_file_kind_for_key (key_name);
	path = _app_config_create_path_for_kind (kind);

	if (path)
	{
		value = _app_config_read_file_string (path, key_name);
		_r_obj_dereference (path);
	}

	if (!value && kind != AppConfigFileBase)
	{
		// Compatibility fallback for older single-file Addons\Config.ini layouts.
		path = _app_get_config_path ();

		if (path)
		{
			value = _app_config_read_file_string (path, key_name);
			_r_obj_dereference (path);
		}
	}

	return value;
}

PR_STRING _app_config_getstring (
	_In_ LPCWSTR key_name,
	_In_opt_ LPCWSTR def_value
)
{
	PR_STRING value;

	value = _app_config_read_split_string (key_name);

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
