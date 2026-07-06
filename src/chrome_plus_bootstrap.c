// chrlauncher
// Auto-sync chrome_plus hook files before launcher startup.

#include <windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#define CPB_CONFIG_FILE L"Config.ini"
#define CPB_ADDONS_CONFIG_FILE L"Addons\\Config.ini"
#define CPB_DEFAULT_CHROMIUM_DIRECTORY L".\\bin"
#define CPB_DEFAULT_CHROME_PLUS_DIRECTORY L".\\chrome_plus"
#define CPB_LEGACY_CHROME_PLUS_DIRECTORY L"Addons\\chrome_plus"
#define CPB_CLOSE_TABS_EXTENSION_DIRECTORY L"addons\\extensions\\close-tabs-right"
#define CPB_CF_MANUAL_HELPER_DIRECTORY L"addons\\cf_manual_helper"
#define CPB_CHROME_PLUS_INI L"chrome++.ini"
#define CPB_CHROME_PLUS_DATA_DIR L"%app%\\..\\Data"
#define CPB_CHROME_PLUS_CACHE_DIR L"%app%\\..\\Cache"
#define CPB_CHROME_PLUS_EXTENSION_COMMAND_LINE L"--load-extension=\"%app%\\..\\addons\\extensions\\close-tabs-right,%app%\\..\\addons\\cf_manual_helper\" --disk-cache-dir=\"%app%\\..\\Cache\" --disable-background-networking --disable-domain-reliability --disable-features=HardwareMediaKeyHandling,GlobalMediaControls --disable-sync-types=Extensions,Apps,ExtensionSettings"

static BOOL cpb_file_exists(
	_In_ LPCWSTR path
)
{
	DWORD attributes;

	if (!path || !path[0])
		return FALSE;

	attributes = GetFileAttributesW(path);

	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static BOOL cpb_directory_exists(
	_In_ LPCWSTR path
)
{
	DWORD attributes;

	if (!path || !path[0])
		return FALSE;

	attributes = GetFileAttributesW(path);

	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static BOOL cpb_join_path(
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

static BOOL cpb_resolve_path(
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars,
	_In_ LPCWSTR root,
	_In_ LPCWSTR configured_path
)
{
	if (!out || !out_chars || !root || !configured_path || !configured_path[0])
		return FALSE;

	if (PathIsRelativeW(configured_path))
		return cpb_join_path(out, out_chars, root, configured_path);

	return GetFullPathNameW(configured_path, out_chars, out, NULL) > 0;
}

static BOOL cpb_copy_if_source_exists(
	_In_ LPCWSTR source_dir,
	_In_ LPCWSTR target_dir,
	_In_ LPCWSTR file_name
)
{
	WCHAR source_path[MAX_PATH * 4];
	WCHAR target_path[MAX_PATH * 4];

	if (!cpb_join_path(source_path, RTL_NUMBER_OF(source_path), source_dir, file_name))
		return FALSE;

	if (!cpb_file_exists(source_path))
		return FALSE;

	if (!cpb_directory_exists(target_dir))
		return FALSE;

	if (!cpb_join_path(target_path, RTL_NUMBER_OF(target_path), target_dir, file_name))
		return FALSE;

	return CopyFileW(source_path, target_path, FALSE);
}

static BOOL cpb_find_chrome_plus_dir(
	_In_ LPCWSTR root,
	_In_ LPCWSTR config_path,
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars
)
{
	WCHAR configured[MAX_PATH * 4];
	WCHAR candidate[MAX_PATH * 4];

	configured[0] = L'\0';

	if (config_path && config_path[0])
		GetPrivateProfileStringW(L"chrlauncher", L"ChromePlusDirectory", L"", configured, RTL_NUMBER_OF(configured), config_path);

	if (configured[0])
	{
		if (cpb_resolve_path(candidate, RTL_NUMBER_OF(candidate), root, configured) && cpb_directory_exists(candidate))
		{
			lstrcpynW(out, candidate, out_chars);
			return TRUE;
		}
	}

	if (cpb_resolve_path(candidate, RTL_NUMBER_OF(candidate), root, CPB_DEFAULT_CHROME_PLUS_DIRECTORY) && cpb_directory_exists(candidate))
	{
		lstrcpynW(out, candidate, out_chars);
		return TRUE;
	}

	if (cpb_resolve_path(candidate, RTL_NUMBER_OF(candidate), root, CPB_LEGACY_CHROME_PLUS_DIRECTORY) && cpb_directory_exists(candidate))
	{
		lstrcpynW(out, candidate, out_chars);
		return TRUE;
	}

	return FALSE;
}

static BOOL cpb_any_managed_extension_exists(
	_In_ LPCWSTR root
)
{
	WCHAR extension_dir[MAX_PATH * 4];

	if (cpb_join_path(extension_dir, RTL_NUMBER_OF(extension_dir), root, CPB_CLOSE_TABS_EXTENSION_DIRECTORY) && cpb_directory_exists(extension_dir))
		return TRUE;

	if (cpb_join_path(extension_dir, RTL_NUMBER_OF(extension_dir), root, CPB_CF_MANUAL_HELPER_DIRECTORY) && cpb_directory_exists(extension_dir))
		return TRUE;

	return FALSE;
}

static VOID cpb_normalize_chrome_plus_ini(
	_In_ LPCWSTR directory,
	_In_ BOOL write_extension_command_line
)
{
	WCHAR ini_path[MAX_PATH * 4];

	if (!cpb_directory_exists(directory))
		return;

	if (!cpb_join_path(ini_path, RTL_NUMBER_OF(ini_path), directory, CPB_CHROME_PLUS_INI))
		return;

	WritePrivateProfileStringW(L"general", L"data_dir", CPB_CHROME_PLUS_DATA_DIR, ini_path);
	WritePrivateProfileStringW(L"general", L"cache_dir", CPB_CHROME_PLUS_CACHE_DIR, ini_path);

	if (write_extension_command_line)
		WritePrivateProfileStringW(L"general", L"command_line", CPB_CHROME_PLUS_EXTENSION_COMMAND_LINE, ini_path);
}

static VOID cpb_sync_chrome_plus(void)
{
	WCHAR root[MAX_PATH * 4];
	WCHAR config_path[MAX_PATH * 4];
	WCHAR configured_bin[MAX_PATH * 4];
	WCHAR binary_dir[MAX_PATH * 4];
	WCHAR chrome_plus_dir[MAX_PATH * 4];
	BOOL has_managed_extension_dir;
	DWORD length;

	length = GetModuleFileNameW(NULL, root, RTL_NUMBER_OF(root));

	if (!length || length >= RTL_NUMBER_OF(root))
		return;

	if (!PathRemoveFileSpecW(root))
		return;

	if (!cpb_join_path(config_path, RTL_NUMBER_OF(config_path), root, CPB_CONFIG_FILE))
		return;

	if (!cpb_file_exists(config_path))
	{
		if (!cpb_join_path(config_path, RTL_NUMBER_OF(config_path), root, CPB_ADDONS_CONFIG_FILE))
			return;

		if (!cpb_file_exists(config_path))
			config_path[0] = L'\0';
	}

	configured_bin[0] = L'\0';

	if (config_path[0])
		GetPrivateProfileStringW(L"chrlauncher", L"ChromiumDirectory", CPB_DEFAULT_CHROMIUM_DIRECTORY, configured_bin, RTL_NUMBER_OF(configured_bin), config_path);

	if (!configured_bin[0])
		lstrcpynW(configured_bin, CPB_DEFAULT_CHROMIUM_DIRECTORY, RTL_NUMBER_OF(configured_bin));

	if (!cpb_resolve_path(binary_dir, RTL_NUMBER_OF(binary_dir), root, configured_bin))
		return;

	if (!cpb_directory_exists(binary_dir))
		return;

	if (!cpb_find_chrome_plus_dir(root, config_path, chrome_plus_dir, RTL_NUMBER_OF(chrome_plus_dir)))
		return;

	has_managed_extension_dir = cpb_any_managed_extension_exists(root);

	cpb_normalize_chrome_plus_ini(chrome_plus_dir, has_managed_extension_dir);
	cpb_copy_if_source_exists(chrome_plus_dir, binary_dir, L"version.dll");
	cpb_copy_if_source_exists(chrome_plus_dir, binary_dir, CPB_CHROME_PLUS_INI);
	cpb_normalize_chrome_plus_ini(binary_dir, has_managed_extension_dir);
}

static VOID __cdecl cpb_crt_startup(void)
{
	cpb_sync_chrome_plus();
}

#ifdef _MSC_VER
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (__cdecl *cpb_crt_startup_ptr)(void) = cpb_crt_startup;
#endif
