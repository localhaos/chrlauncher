// chrlauncher
// Optimizer Guard: re-apply portable/performance settings after Chromium or config updates.

#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#define OG_COUNTOF(x) (sizeof(x) / sizeof((x)[0]))
#define OG_ROOT_CONFIG_FILE L"Config.ini"
#define OG_ADDONS_CONFIG_FILE L"Addons\\Config.ini"
#define OG_ADDON_INI L"Addons\\optimizer_guard\\optimizer_guard.ini"
#define OG_DEFAULT_CHROMIUM_DIRECTORY L".\\bin"
#define OG_DEFAULT_CHROME_PLUS_DIRECTORY L".\\addons\\chrome_plus"
#define OG_CHROME_PLUS_INI L"chrome++.ini"

typedef struct _OG_KEY_PAIR
{
	LPCWSTR Section;
	LPCWSTR Key;
} OG_KEY_PAIR;

static BOOL og_file_exists(
	_In_ LPCWSTR path
)
{
	DWORD attributes;

	if (!path || !path[0])
		return FALSE;

	attributes = GetFileAttributesW(path);

	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static BOOL og_directory_exists(
	_In_ LPCWSTR path
)
{
	DWORD attributes;

	if (!path || !path[0])
		return FALSE;

	attributes = GetFileAttributesW(path);

	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static BOOL og_join_path(
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars,
	_In_ LPCWSTR left,
	_In_ LPCWSTR right
)
{
	WCHAR buffer[MAX_PATH * 4];

	if (!out || !out_chars || !left || !right)
		return FALSE;

	if (!PathCombineW(buffer, left, right))
		return FALSE;

	return GetFullPathNameW(buffer, out_chars, out, NULL) > 0;
}

static BOOL og_resolve_path(
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars,
	_In_ LPCWSTR root,
	_In_ LPCWSTR configured_path
)
{
	if (!out || !out_chars || !root || !configured_path || !configured_path[0])
		return FALSE;

	if (PathIsRelativeW(configured_path))
		return og_join_path(out, out_chars, root, configured_path);

	return GetFullPathNameW(configured_path, out_chars, out, NULL) > 0;
}

static BOOL og_get_root(
	_Out_writes_z_(root_chars) LPWSTR root,
	_In_ DWORD root_chars
)
{
	DWORD length;

	length = GetModuleFileNameW(NULL, root, root_chars);

	if (!length || length >= root_chars)
		return FALSE;

	return PathRemoveFileSpecW(root);
}

static BOOL og_get_config_path(
	_In_ LPCWSTR root,
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars
)
{
	if (!og_join_path(out, out_chars, root, OG_ROOT_CONFIG_FILE))
		return FALSE;

	if (og_file_exists(out))
		return TRUE;

	if (!og_join_path(out, out_chars, root, OG_ADDONS_CONFIG_FILE))
		return FALSE;

	return og_file_exists(out);
}

static BOOL og_read_ini_string(
	_In_ LPCWSTR ini_path,
	_In_ LPCWSTR section,
	_In_ LPCWSTR key,
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars
)
{
	DWORD length;

	if (!ini_path || !section || !key || !out || !out_chars)
		return FALSE;

	out[0] = L'\0';
	length = GetPrivateProfileStringW(section, key, L"", out, out_chars, ini_path);

	return length > 0 && out[0] != L'\0';
}

static VOID og_apply_chrlauncher_key(
	_In_ LPCWSTR addon_ini,
	_In_ LPCWSTR config_path,
	_In_ LPCWSTR key
)
{
	WCHAR value[8192];

	if (og_read_ini_string(addon_ini, L"chrlauncher", key, value, OG_COUNTOF(value)))
		WritePrivateProfileStringW(L"chrlauncher", key, value, config_path);
}

static VOID og_apply_chrlauncher_settings(
	_In_ LPCWSTR addon_ini,
	_In_ LPCWSTR config_path
)
{
	static const LPCWSTR keys[] = {
		L"ChromiumCommandLine",
		L"ChromiumEnableLosslessOptimization",
		L"ChromiumLosslessOptimizationCommandLine",
		L"ChromiumLosslessOptimizationFeatures",
		L"ChromiumEnableTextPerformanceFixes",
		L"ChromiumTextPerformanceCommandLine",
		L"ChromiumTextPerformanceDisableFeatures",
		L"ChromiumEnableBackgroundResourceSaving",
		L"ChromiumBackgroundResourceSavingCommandLine",
		L"ChromiumBackgroundResourceSavingFeatures",
		L"ChromiumEnableAggressiveTextPerformanceFixes",
		L"ChromiumAggressiveTextPerformanceCommandLine",
		L"ChromiumEnableRendererSafety",
		L"ChromiumRendererSafetyCommandLine",
		L"ChromiumEnableScrollableTabs",
		L"ChromiumScrollableTabsFeatures",
		L"ChromiumEnableMultithreadedRaster",
		L"ChromiumMultithreadingCommandLine",
		L"ChromiumIgnoreGpuBlocklist",
		L"ChromiumIgnoreGpuBlocklistCommandLine",
		L"ChromiumEnableHardwareAcceleration",
		L"ChromiumHardwareAccelerationCommandLine",
		L"ChromiumEnableDirectX",
		L"ChromiumDirectXCommandLine",
		L"ChromiumEnableVulkan",
		L"ChromiumVulkanCommandLine",
		L"ChromiumVulkanFeatures",
		L"ChromePlusDirectory"
	};
	SIZE_T i;

	for (i = 0; i < OG_COUNTOF(keys); i++)
		og_apply_chrlauncher_key(addon_ini, config_path, keys[i]);
}

static BOOL og_get_configured_directory(
	_In_ LPCWSTR root,
	_In_ LPCWSTR config_path,
	_In_ LPCWSTR key,
	_In_ LPCWSTR fallback,
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars
)
{
	WCHAR configured[MAX_PATH * 4];

	configured[0] = L'\0';

	if (config_path && config_path[0])
		GetPrivateProfileStringW(L"chrlauncher", key, fallback, configured, OG_COUNTOF(configured), config_path);

	if (!configured[0])
		lstrcpynW(configured, fallback, OG_COUNTOF(configured));

	return og_resolve_path(out, out_chars, root, configured);
}

static VOID og_apply_chrome_plus_ini(
	_In_ LPCWSTR addon_ini,
	_In_ LPCWSTR chrome_plus_ini
)
{
	WCHAR value[8192];

	if (!chrome_plus_ini || !chrome_plus_ini[0])
		return;

	if (og_read_ini_string(addon_ini, L"chrome_plus", L"data_dir", value, OG_COUNTOF(value)))
		WritePrivateProfileStringW(L"general", L"data_dir", value, chrome_plus_ini);

	if (og_read_ini_string(addon_ini, L"chrome_plus", L"cache_dir", value, OG_COUNTOF(value)))
		WritePrivateProfileStringW(L"general", L"cache_dir", value, chrome_plus_ini);

	if (og_read_ini_string(addon_ini, L"chrome_plus", L"command_line", value, OG_COUNTOF(value)))
		WritePrivateProfileStringW(L"general", L"command_line", value, chrome_plus_ini);
}

static VOID og_apply_chrome_plus_settings(
	_In_ LPCWSTR root,
	_In_ LPCWSTR config_path,
	_In_ LPCWSTR addon_ini
)
{
	WCHAR binary_dir[MAX_PATH * 4];
	WCHAR chrome_plus_dir[MAX_PATH * 4];
	WCHAR ini_path[MAX_PATH * 4];

	if (!og_get_configured_directory(root, config_path, L"ChromiumDirectory", OG_DEFAULT_CHROMIUM_DIRECTORY, binary_dir, OG_COUNTOF(binary_dir)))
		return;

	if (!og_get_configured_directory(root, config_path, L"ChromePlusDirectory", OG_DEFAULT_CHROME_PLUS_DIRECTORY, chrome_plus_dir, OG_COUNTOF(chrome_plus_dir)))
		return;

	if (og_directory_exists(chrome_plus_dir) && og_join_path(ini_path, OG_COUNTOF(ini_path), chrome_plus_dir, OG_CHROME_PLUS_INI))
		og_apply_chrome_plus_ini(addon_ini, ini_path);

	if (og_directory_exists(binary_dir) && og_join_path(ini_path, OG_COUNTOF(ini_path), binary_dir, OG_CHROME_PLUS_INI))
		og_apply_chrome_plus_ini(addon_ini, ini_path);
}

static VOID og_apply_optimizer_guard(void)
{
	WCHAR root[MAX_PATH * 4];
	WCHAR config_path[MAX_PATH * 4];
	WCHAR addon_ini[MAX_PATH * 4];
	UINT enabled;

	if (!og_get_root(root, OG_COUNTOF(root)))
		return;

	if (!og_join_path(addon_ini, OG_COUNTOF(addon_ini), root, OG_ADDON_INI))
		return;

	if (!og_file_exists(addon_ini))
		return;

	enabled = GetPrivateProfileIntW(L"optimizer_guard", L"Enabled", 1, addon_ini);

	if (!enabled)
		return;

	if (!og_get_config_path(root, config_path, OG_COUNTOF(config_path)))
		return;

	og_apply_chrlauncher_settings(addon_ini, config_path);
	og_apply_chrome_plus_settings(root, config_path, addon_ini);
}

static VOID __cdecl og_crt_startup(void)
{
	og_apply_optimizer_guard();
}

#ifdef _MSC_VER
#pragma section(".CRT$XCV", read)
__declspec(allocate(".CRT$XCV")) void (__cdecl *og_crt_startup_ptr)(void) = og_crt_startup;
#endif
