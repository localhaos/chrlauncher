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

static VOID cpb_sync_chrome_plus(void)
{
	WCHAR root[MAX_PATH * 4];
	WCHAR config_path[MAX_PATH * 4];
	WCHAR configured_bin[MAX_PATH * 4];
	WCHAR binary_dir[MAX_PATH * 4];
	WCHAR chrome_plus_dir[MAX_PATH * 4];
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

	cpb_copy_if_source_exists(chrome_plus_dir, binary_dir, L"version.dll");
	cpb_copy_if_source_exists(chrome_plus_dir, binary_dir, L"chrome++.ini");
}

static VOID __cdecl cpb_crt_startup(void)
{
	cpb_sync_chrome_plus();
}

#ifdef _MSC_VER
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (__cdecl *cpb_crt_startup_ptr)(void) = cpb_crt_startup;
#endif
