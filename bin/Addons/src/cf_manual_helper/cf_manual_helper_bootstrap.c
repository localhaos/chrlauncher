// chrlauncher
// CF Manual Helper native addon bootstrap.
// Keeps the standalone cf_manual_helper extension loaded after updates without storing or applying cf_clearance.

#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#define CFM_COUNTOF(x) (sizeof(x) / sizeof((x)[0]))
#define CFM_CONFIG_FILE L"Config.ini"
#define CFM_DEFAULT_CHROMIUM_DIRECTORY L".\\bin"
#define CFM_DEFAULT_CHROME_PLUS_DIRECTORY L".\\addons\\chrome_plus"
#define CFM_CHROME_PLUS_INI L"chrome++.ini"
#define CFM_CLOSE_TABS_DIR L"%app%\\..\\addons\\extensions\\close-tabs-right"
#define CFM_HELPER_DIR L"%app%\\..\\addons\\cf_manual_helper"
#define CFM_COMMAND_LINE L"--load-extension=\"%app%\\..\\addons\\extensions\\close-tabs-right,%app%\\..\\addons\\cf_manual_helper\""

static BOOL cfm_file_exists(
	_In_ LPCWSTR path
)
{
	DWORD attributes;

	if (!path || !path[0])
		return FALSE;

	attributes = GetFileAttributesW(path);

	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static BOOL cfm_directory_exists(
	_In_ LPCWSTR path
)
{
	DWORD attributes;

	if (!path || !path[0])
		return FALSE;

	attributes = GetFileAttributesW(path);

	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static BOOL cfm_join_path(
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

static BOOL cfm_resolve_path(
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars,
	_In_ LPCWSTR root,
	_In_ LPCWSTR configured_path
)
{
	if (!out || !out_chars || !root || !configured_path || !configured_path[0])
		return FALSE;

	if (PathIsRelativeW(configured_path))
		return cfm_join_path(out, out_chars, root, configured_path);

	return GetFullPathNameW(configured_path, out_chars, out, NULL) > 0;
}

static BOOL cfm_get_root(
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

static BOOL cfm_get_configured_directory(
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
		GetPrivateProfileStringW(L"chrlauncher", key, fallback, configured, CFM_COUNTOF(configured), config_path);

	if (!configured[0])
		lstrcpynW(configured, fallback, CFM_COUNTOF(configured));

	return cfm_resolve_path(out, out_chars, root, configured);
}

static VOID cfm_apply_chrome_plus_command_line(
	_In_ LPCWSTR chrome_plus_ini
)
{
	if (!chrome_plus_ini || !chrome_plus_ini[0])
		return;

	WritePrivateProfileStringW(L"general", L"command_line", CFM_COMMAND_LINE, chrome_plus_ini);
}

static VOID cfm_keep_loaded(void)
{
	WCHAR root[MAX_PATH * 4];
	WCHAR config_path[MAX_PATH * 4];
	WCHAR chromium_dir[MAX_PATH * 4];
	WCHAR chrome_plus_dir[MAX_PATH * 4];
	WCHAR helper_manifest[MAX_PATH * 4];
	WCHAR ini_path[MAX_PATH * 4];

	if (!cfm_get_root(root, CFM_COUNTOF(root)))
		return;

	if (!cfm_join_path(helper_manifest, CFM_COUNTOF(helper_manifest), root, L"addons\\cf_manual_helper\\manifest.json"))
		return;

	if (!cfm_file_exists(helper_manifest))
		return;

	if (!cfm_join_path(config_path, CFM_COUNTOF(config_path), root, CFM_CONFIG_FILE))
		return;

	if (!cfm_file_exists(config_path))
		config_path[0] = L'\0';

	if (cfm_get_configured_directory(root, config_path, L"ChromePlusDirectory", CFM_DEFAULT_CHROME_PLUS_DIRECTORY, chrome_plus_dir, CFM_COUNTOF(chrome_plus_dir)) && cfm_directory_exists(chrome_plus_dir))
	{
		if (cfm_join_path(ini_path, CFM_COUNTOF(ini_path), chrome_plus_dir, CFM_CHROME_PLUS_INI))
			cfm_apply_chrome_plus_command_line(ini_path);
	}

	if (cfm_get_configured_directory(root, config_path, L"ChromiumDirectory", CFM_DEFAULT_CHROMIUM_DIRECTORY, chromium_dir, CFM_COUNTOF(chromium_dir)) && cfm_directory_exists(chromium_dir))
	{
		if (cfm_join_path(ini_path, CFM_COUNTOF(ini_path), chromium_dir, CFM_CHROME_PLUS_INI))
			cfm_apply_chrome_plus_command_line(ini_path);
	}
}

static VOID __cdecl cfm_crt_startup(void)
{
	cfm_keep_loaded();
}

#ifdef _MSC_VER
#pragma section(".CRT$XCW", read)
__declspec(allocate(".CRT$XCW")) void (__cdecl *cfm_crt_startup_ptr)(void) = cfm_crt_startup;
#endif
