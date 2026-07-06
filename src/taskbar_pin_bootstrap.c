// chrlauncher
// Repair Windows taskbar pinned shortcuts that point directly to portable chrome.exe.

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <shobjidl.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shell32.lib")

#define TPB_CONFIG_FILE L"Config.ini"
#define TPB_ADDONS_CONFIG_FILE L"Addons\\Config.ini"
#define TPB_DEFAULT_CHROMIUM_DIRECTORY L".\\bin"
#define TPB_DEFAULT_CHROMIUM_BINARY L"chrome.exe"

static BOOL tpb_file_exists(
	_In_ LPCWSTR path
)
{
	DWORD attributes;

	if (!path || !path[0])
		return FALSE;

	attributes = GetFileAttributesW(path);

	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

static BOOL tpb_join_path(
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

static BOOL tpb_resolve_path(
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars,
	_In_ LPCWSTR root,
	_In_ LPCWSTR configured_path
)
{
	if (!out || !out_chars || !root || !configured_path || !configured_path[0])
		return FALSE;

	if (PathIsRelativeW(configured_path))
		return tpb_join_path(out, out_chars, root, configured_path);

	return GetFullPathNameW(configured_path, out_chars, out, NULL) > 0;
}

static BOOL tpb_get_root(
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

static BOOL tpb_get_config_path(
	_In_ LPCWSTR root,
	_Out_writes_z_(config_chars) LPWSTR config_path,
	_In_ DWORD config_chars
)
{
	if (!tpb_join_path(config_path, config_chars, root, TPB_CONFIG_FILE))
		return FALSE;

	if (tpb_file_exists(config_path))
		return TRUE;

	if (!tpb_join_path(config_path, config_chars, root, TPB_ADDONS_CONFIG_FILE))
		return FALSE;

	return tpb_file_exists(config_path);
}

static BOOL tpb_get_chrome_path(
	_In_ LPCWSTR root,
	_Out_writes_z_(chrome_path_chars) LPWSTR chrome_path,
	_In_ DWORD chrome_path_chars
)
{
	WCHAR config_path[MAX_PATH * 4];
	WCHAR configured_dir[MAX_PATH * 4];
	WCHAR configured_binary[MAX_PATH * 4];
	WCHAR binary_dir[MAX_PATH * 4];

	configured_dir[0] = L'\0';
	configured_binary[0] = L'\0';

	if (tpb_get_config_path(root, config_path, RTL_NUMBER_OF(config_path)))
	{
		GetPrivateProfileStringW(L"chrlauncher", L"ChromiumDirectory", TPB_DEFAULT_CHROMIUM_DIRECTORY, configured_dir, RTL_NUMBER_OF(configured_dir), config_path);
		GetPrivateProfileStringW(L"chrlauncher", L"ChromiumBinary", TPB_DEFAULT_CHROMIUM_BINARY, configured_binary, RTL_NUMBER_OF(configured_binary), config_path);
	}

	if (!configured_dir[0])
		lstrcpynW(configured_dir, TPB_DEFAULT_CHROMIUM_DIRECTORY, RTL_NUMBER_OF(configured_dir));

	if (!configured_binary[0])
		lstrcpynW(configured_binary, TPB_DEFAULT_CHROMIUM_BINARY, RTL_NUMBER_OF(configured_binary));

	if (!tpb_resolve_path(binary_dir, RTL_NUMBER_OF(binary_dir), root, configured_dir))
		return FALSE;

	return tpb_join_path(chrome_path, chrome_path_chars, binary_dir, configured_binary);
}

static BOOL tpb_same_path(
	_In_ LPCWSTR left,
	_In_ LPCWSTR right
)
{
	WCHAR left_full[MAX_PATH * 4];
	WCHAR right_full[MAX_PATH * 4];

	if (!left || !right || !left[0] || !right[0])
		return FALSE;

	if (!GetFullPathNameW(left, RTL_NUMBER_OF(left_full), left_full, NULL))
		lstrcpynW(left_full, left, RTL_NUMBER_OF(left_full));

	if (!GetFullPathNameW(right, RTL_NUMBER_OF(right_full), right_full, NULL))
		lstrcpynW(right_full, right, RTL_NUMBER_OF(right_full));

	return lstrcmpiW(left_full, right_full) == 0;
}

static BOOL tpb_get_taskbar_pin_dir(
	_Out_writes_z_(out_chars) LPWSTR out,
	_In_ DWORD out_chars
)
{
	WCHAR appdata[MAX_PATH * 4];

	if (FAILED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, appdata)))
		return FALSE;

	return tpb_join_path(out, out_chars, appdata, L"Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar");
}

static VOID tpb_repair_one_shortcut(
	_In_ LPCWSTR lnk_path,
	_In_ LPCWSTR launcher_path,
	_In_ LPCWSTR root,
	_In_ LPCWSTR chrome_path
)
{
	IShellLinkW* shell_link = NULL;
	IPersistFile* persist_file = NULL;
	WCHAR target_path[MAX_PATH * 4];
	HRESULT hr;

	if (!lnk_path || !launcher_path || !root || !chrome_path)
		return;

	hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkW, (LPVOID*)&shell_link);

	if (FAILED(hr) || !shell_link)
		return;

	hr = shell_link->lpVtbl->QueryInterface(shell_link, &IID_IPersistFile, (LPVOID*)&persist_file);

	if (SUCCEEDED(hr) && persist_file)
	{
		hr = persist_file->lpVtbl->Load(persist_file, lnk_path, STGM_READWRITE);

		if (SUCCEEDED(hr))
		{
			target_path[0] = L'\0';
			hr = shell_link->lpVtbl->GetPath(shell_link, target_path, RTL_NUMBER_OF(target_path), NULL, SLGP_RAWPATH);

			if (SUCCEEDED(hr) && tpb_same_path(target_path, chrome_path))
			{
				shell_link->lpVtbl->SetPath(shell_link, launcher_path);
				shell_link->lpVtbl->SetArguments(shell_link, L"");
				shell_link->lpVtbl->SetWorkingDirectory(shell_link, root);
				shell_link->lpVtbl->SetIconLocation(shell_link, chrome_path, 0);
				persist_file->lpVtbl->Save(persist_file, lnk_path, TRUE);
			}
		}

		persist_file->lpVtbl->Release(persist_file);
	}

	shell_link->lpVtbl->Release(shell_link);
}

static VOID tpb_repair_taskbar_pins(void)
{
	WIN32_FIND_DATAW find_data;
	WCHAR root[MAX_PATH * 4];
	WCHAR launcher_path[MAX_PATH * 4];
	WCHAR chrome_path[MAX_PATH * 4];
	WCHAR pin_dir[MAX_PATH * 4];
	WCHAR search_path[MAX_PATH * 4];
	WCHAR lnk_path[MAX_PATH * 4];
	HANDLE find_handle;
	HRESULT hr;
	BOOL com_initialized = FALSE;
	DWORD length;

	if (!tpb_get_root(root, RTL_NUMBER_OF(root)))
		return;

	length = GetModuleFileNameW(NULL, launcher_path, RTL_NUMBER_OF(launcher_path));

	if (!length || length >= RTL_NUMBER_OF(launcher_path))
		return;

	if (!tpb_get_chrome_path(root, chrome_path, RTL_NUMBER_OF(chrome_path)))
		return;

	if (!tpb_file_exists(chrome_path))
		return;

	if (!tpb_get_taskbar_pin_dir(pin_dir, RTL_NUMBER_OF(pin_dir)))
		return;

	if (!tpb_join_path(search_path, RTL_NUMBER_OF(search_path), pin_dir, L"*.lnk"))
		return;

	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	com_initialized = SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE;

	find_handle = FindFirstFileW(search_path, &find_data);

	if (find_handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			if (!tpb_join_path(lnk_path, RTL_NUMBER_OF(lnk_path), pin_dir, find_data.cFileName))
				continue;

			tpb_repair_one_shortcut(lnk_path, launcher_path, root, chrome_path);
		}
		while (FindNextFileW(find_handle, &find_data));

		FindClose(find_handle);
	}

	if (com_initialized && hr != RPC_E_CHANGED_MODE)
		CoUninitialize();
}

static VOID __cdecl tpb_crt_startup(void)
{
	tpb_repair_taskbar_pins();
}

#ifdef _MSC_VER
#pragma section(".CRT$XCU", read)
__declspec(allocate(".CRT$XCU")) void (__cdecl *tpb_crt_startup_ptr)(void) = tpb_crt_startup;
#endif
