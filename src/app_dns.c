// chrlauncher
// Copyright (c) 2015-2026 Henry++

#include "app_dns.h"
#include "app_paths.h"

PR_STRING _app_download_text_url (
	_In_ LPCWSTR url
);

static PR_STRING _app_create_string_from_text_buffer (
	_In_reads_bytes_ (buffer_size) PBYTE buffer,
	_In_ DWORD buffer_size
)
{
	PR_STRING string;
	LPWSTR string_buffer;
	INT length;
	UINT codepage;
	BOOLEAN is_utf16le = FALSE;

	if (!buffer || !buffer_size)
		return NULL;

	if (buffer_size >= 2 && buffer[0] == 0xFF && buffer[1] == 0xFE)
		return _r_obj_createstring_ex ((LPWSTR)(buffer + 2), (buffer_size - 2) & ~(sizeof (WCHAR) - 1));

	if (buffer_size >= 2 && !(buffer_size & 1))
	{
		DWORD sample_size = min (buffer_size, 128);
		DWORD zero_count = 0;

		for (DWORD i = 1; i < sample_size; i += 2)
		{
			if (!buffer[i])
				zero_count += 1;
		}

		is_utf16le = zero_count >= (sample_size / 4);
	}

	if (is_utf16le)
		return _r_obj_createstring_ex ((LPWSTR)buffer, buffer_size & ~(sizeof (WCHAR) - 1));

	if (buffer_size >= 3 && buffer[0] == 0xEF && buffer[1] == 0xBB && buffer[2] == 0xBF)
	{
		buffer += 3;
		buffer_size -= 3;
	}

	codepage = CP_UTF8;
	length = MultiByteToWideChar (codepage, MB_ERR_INVALID_CHARS, (LPCCH)buffer, buffer_size, NULL, 0);

	if (!length)
	{
		codepage = CP_ACP;
		length = MultiByteToWideChar (codepage, 0, (LPCCH)buffer, buffer_size, NULL, 0);
	}

	if (!length)
		return NULL;

	string_buffer = _r_mem_allocate ((length + 1) * sizeof (WCHAR));

	if (!string_buffer)
		return NULL;

	if (!MultiByteToWideChar (codepage, codepage == CP_UTF8 ? MB_ERR_INVALID_CHARS : 0, (LPCCH)buffer, buffer_size, string_buffer, length))
	{
		_r_mem_free (string_buffer);

		return NULL;
	}

	string_buffer[length] = UNICODE_NULL;
	string = _r_obj_createstring_ex (string_buffer, length * sizeof (WCHAR));

	_r_mem_free (string_buffer);

	return string;
}

static BOOLEAN _app_is_valid_secure_dns_template (
	_In_opt_ PR_STRING dns_template
)
{
	LPCWSTR ptr;

	if (_r_obj_isstringempty (dns_template))
		return FALSE;

	if (_wcsnicmp (dns_template->buffer, L"https://", 8) != 0)
		return FALSE;

	for (ptr = dns_template->buffer; *ptr; ptr++)
	{
		if (*ptr <= L' ' || *ptr == L'"')
			return FALSE;
	}

	return TRUE;
}

static PR_STRING _app_create_secure_dns_arguments (
	_In_ PR_STRING dns_template
)
{
	if (!_app_is_valid_secure_dns_template (dns_template))
		return NULL;

	return _r_format_string (CHROMIUM_SECURE_DNS_COMMAND_LINE, dns_template->buffer);
}

PR_STRING _app_create_secure_dns_arguments_from_config ()
{
	PR_STRING arguments;
	PR_STRING dns_template;

	if (!_r_config_getboolean (L"ChromiumEnableSecureDns", FALSE))
		return NULL;

	dns_template = _r_config_getstringexpand (L"ChromiumSecureDnsUrl", L"");

	if (!_app_is_valid_secure_dns_template (dns_template))
	{
		if (dns_template)
			_r_obj_dereference (dns_template);

		return NULL;
	}

	arguments = _app_create_secure_dns_arguments (dns_template);

	_r_obj_dereference (dns_template);

	return arguments;
}

static BOOLEAN _app_append_dns_blocklist_rule (
	_Inout_ PR_STRING *rules,
	_In_ LPCWSTR hostname,
	_In_ PR_STRING sink,
	_In_ BOOLEAN include_subdomains,
	_In_ LONG max_chars
)
{
	PR_STRING next_rules;
	PR_STRING rule;

	if (!rules || !*rules || _r_obj_isstringempty (sink) || !hostname || !hostname[0])
		return FALSE;

	if (include_subdomains)
		rule = _r_format_string (L"%sMAP %s %s,MAP *.%s %s", _r_obj_isstringempty (*rules) ? L"" : L",", hostname, sink->buffer, hostname, sink->buffer);
	else
		rule = _r_format_string (L"%sMAP %s %s", _r_obj_isstringempty (*rules) ? L"" : L",", hostname, sink->buffer);

	if (!rule)
		return FALSE;

	if (((*rules)->length + rule->length) > ((ULONG_PTR)max_chars * sizeof (WCHAR)))
	{
		_r_obj_dereference (rule);

		return FALSE;
	}

	next_rules = _r_obj_concatstrings (2, (*rules)->buffer, rule->buffer);

	_r_obj_dereference (rule);

	if (!next_rules)
		return FALSE;

	_r_obj_movereference ((PVOID_PTR)rules, next_rules);

	return TRUE;
}

static PR_STRING _app_get_dnsblock_cache_path (
	_In_ PR_STRING url
)
{
	R_STRINGREF separator_sr = PR_STRINGREF_INIT (L"\\");
	PR_STRING cache_directory;
	PR_STRING cache_path;
	PR_STRING string;

	if (_r_obj_isstringempty (url))
		return NULL;

	cache_directory = _app_get_configured_directory (L"ChromiumDnsBlocklistCacheDirectory", CHROMIUM_DNS_BLOCKLIST_CACHE_DIRECTORY, FALSE);

	if (!cache_directory)
		return NULL;

	_app_ensure_directory (cache_directory);

	string = _r_format_string (L"list_%" TEXT (PR_ULONG) L".txt", _r_str_gethash2 (&url->sr, TRUE));

	if (string)
	{
		cache_path = _r_obj_concatstringrefs (
			3,
			&cache_directory->sr,
			&separator_sr,
			&string->sr
		);

		_r_obj_dereference (string);
	}
	else
	{
		cache_path = NULL;
	}

	_r_obj_dereference (cache_directory);

	return cache_path;
}

static PR_STRING _app_read_text_cache_file (
	_In_ PR_STRING cache_path,
	_In_ LONG max_age_hours,
	_In_ BOOLEAN ignore_age
)
{
	BY_HANDLE_FILE_INFORMATION file_info;
	FILETIME now_filetime;
	HANDLE hfile;
	LARGE_INTEGER file_size;
	ULARGE_INTEGER last_write_time;
	ULARGE_INTEGER now_time;
	PR_STRING string = NULL;
	PBYTE buffer;
	DWORD bytes_read = 0;
	DWORD bytes_to_read;
	NTSTATUS status;

	if (_r_obj_isstringempty (cache_path) || !_r_fs_exists (&cache_path->sr))
		return NULL;

	status = _r_fs_createfile (&cache_path->sr, FILE_OPEN, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0, 0, FALSE, NULL, &hfile);

	if (!NT_SUCCESS (status))
		return NULL;

	if (!GetFileInformationByHandle (hfile, &file_info))
	{
		NtClose (hfile);

		return NULL;
	}

	if (!ignore_age && max_age_hours > 0)
	{
		GetSystemTimeAsFileTime (&now_filetime);

		last_write_time.LowPart = file_info.ftLastWriteTime.dwLowDateTime;
		last_write_time.HighPart = file_info.ftLastWriteTime.dwHighDateTime;
		now_time.LowPart = now_filetime.dwLowDateTime;
		now_time.HighPart = now_filetime.dwHighDateTime;

		if (now_time.QuadPart > last_write_time.QuadPart && (now_time.QuadPart - last_write_time.QuadPart) > ((ULONGLONG)max_age_hours * 60ULL * 60ULL * 10000000ULL))
		{
			NtClose (hfile);

			return NULL;
		}
	}

	if (!GetFileSizeEx (hfile, &file_size) || file_size.QuadPart <= 0 || file_size.QuadPart > (4 * 1024 * 1024))
	{
		NtClose (hfile);

		return NULL;
	}

	bytes_to_read = (DWORD)file_size.QuadPart;
	buffer = _r_mem_allocate (bytes_to_read);

	if (buffer)
	{
		if (ReadFile (hfile, buffer, bytes_to_read, &bytes_read, NULL) && bytes_read)
			string = _app_create_string_from_text_buffer (buffer, bytes_read);

		_r_mem_free (buffer);
	}

	NtClose (hfile);

	return string;
}

static VOID _app_write_text_cache_file (
	_In_ PR_STRING cache_path,
	_In_ PR_STRING text
)
{
	HANDLE hfile;
	DWORD bytes_written;
	NTSTATUS status;

	if (_r_obj_isstringempty (cache_path) || _r_obj_isstringempty (text))
		return;

	status = _r_fs_createfile (&cache_path->sr, FILE_OVERWRITE_IF, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_DELETE, 0, 0, FALSE, NULL, &hfile);

	if (!NT_SUCCESS (status))
		return;

	WriteFile (hfile, text->buffer, (DWORD)text->length, &bytes_written, NULL);

	NtClose (hfile);
}

static PR_STRING _app_get_dnsblock_text (
	_In_ PR_STRING blocklist_url
)
{
	PR_STRING cache_path = NULL;
	PR_STRING text = NULL;
	LONG max_age_hours;
	BOOLEAN use_cache;

	if (_r_obj_isstringempty (blocklist_url))
		return NULL;

	use_cache = _r_config_getboolean (L"ChromiumDnsBlocklistCache", TRUE);
	max_age_hours = _r_config_getlong (L"ChromiumDnsBlocklistCacheMaxAgeHours", 24);

	if (max_age_hours < 1 || max_age_hours > 24 * 30)
		max_age_hours = 24;

	if (use_cache)
	{
		cache_path = _app_get_dnsblock_cache_path (blocklist_url);
		text = _app_read_text_cache_file (cache_path, max_age_hours, FALSE);
	}

	if (!text)
	{
		text = _app_download_text_url (blocklist_url->buffer);

		if (text && use_cache && cache_path)
			_app_write_text_cache_file (cache_path, text);
	}

	if (!text && use_cache && cache_path)
		text = _app_read_text_cache_file (cache_path, max_age_hours, TRUE);

	if (cache_path)
		_r_obj_dereference (cache_path);

	return text;
}

static BOOLEAN _app_is_dns_hostname_char (
	_In_ WCHAR ch
)
{
	return ((ch >= L'a' && ch <= L'z') ||
		(ch >= L'A' && ch <= L'Z') ||
		(ch >= L'0' && ch <= L'9') ||
		ch == L'-' ||
		ch == L'.');
}

static BOOLEAN _app_is_dns_hostname_valid (
	_In_ LPCWSTR hostname
)
{
	ULONG_PTR label_length = 0;
	ULONG_PTR total_length;
	WCHAR ch;

	if (!hostname || !hostname[0])
		return FALSE;

	total_length = _r_str_getlength (hostname);

	if (total_length > 253)
		return FALSE;

	if (hostname[0] == L'.' || hostname[0] == L'-')
		return FALSE;

	for (ULONG_PTR i = 0; i < total_length; i++)
	{
		ch = hostname[i];

		if (!_app_is_dns_hostname_char (ch))
			return FALSE;

		if (ch == L'.')
		{
			if (!label_length || label_length > 63 || hostname[i - 1] == L'-')
				return FALSE;

			label_length = 0;
		}
		else
		{
			label_length++;
		}
	}

	return label_length > 0 && label_length <= 63 && hostname[total_length - 1] != L'-';
}

static BOOLEAN _app_read_dns_token (
	_Inout_ LPCWSTR *cursor,
	_Out_writes_z_ (hostname_length) LPWSTR hostname,
	_In_ ULONG_PTR hostname_length
)
{
	ULONG_PTR length = 0;
	LPCWSTR ptr;

	if (!cursor || !*cursor || !hostname || hostname_length < 2)
		return FALSE;

	ptr = *cursor;

	while (*ptr && !_app_is_dns_hostname_char (*ptr))
		ptr++;

	while (_app_is_dns_hostname_char (*ptr) && length + 1 < hostname_length)
		hostname[length++] = *ptr++;

	hostname[length] = UNICODE_NULL;
	*cursor = ptr;

	return _app_is_dns_hostname_valid (hostname);
}

static BOOLEAN _app_extract_dns_hostname_from_line (
	_In_ LPCWSTR line,
	_Out_writes_z_ (hostname_length) LPWSTR hostname,
	_In_ ULONG_PTR hostname_length
)
{
	WCHAR first_token[256];
	WCHAR second_token[256];
	LPCWSTR ptr;

	if (!line || !hostname || hostname_length < 2)
		return FALSE;

	hostname[0] = UNICODE_NULL;
	ptr = line;

	while (*ptr == L' ' || *ptr == L'\t')
		ptr++;

	if (!*ptr || *ptr == L'#' || *ptr == L'!' || *ptr == L'[')
		return FALSE;

	if (ptr[0] == L'/' && ptr[1] == L'/')
		return FALSE;

	if (ptr[0] == L'|' && ptr[1] == L'|')
	{
		ptr += 2;

		if (_app_read_dns_token (&ptr, hostname, hostname_length))
			return TRUE;
	}

	if (!_app_read_dns_token (&ptr, first_token, RTL_NUMBER_OF (first_token)))
		return FALSE;

	if (lstrcmpW (first_token, L"0.0.0.0") == 0 || lstrcmpW (first_token, L"127.0.0.1") == 0 || lstrcmpW (first_token, L"::1") == 0)
	{
		if (_app_read_dns_token (&ptr, second_token, RTL_NUMBER_OF (second_token)))
		{
			_r_str_copy (hostname, hostname_length, second_token);

			return TRUE;
		}
	}

	_r_str_copy (hostname, hostname_length, first_token);

	return TRUE;
}

PR_STRING _app_create_dns_blocklist_arguments ()
{
	PR_STRING blocklist_text;
	PR_STRING blocklist_url;
	PR_STRING next_rules;
	PR_STRING rules;
	PR_STRING secure_dns_arguments;
	PR_STRING sink;
	LPCWSTR ptr;
	LONG max_chars;
	LONG max_rules;
	ULONG count = 0;
	BOOLEAN include_subdomains;
	WCHAR hostname[256];
	WCHAR line_buffer[1024];
	ULONG_PTR line_length;

	secure_dns_arguments = _app_create_secure_dns_arguments_from_config ();

	if (secure_dns_arguments)
		return secure_dns_arguments;

	if (!_r_config_getboolean (L"ChromiumEnableDnsBlocklistUrl", FALSE))
		return NULL;

	blocklist_url = _r_config_getstringexpand (L"ChromiumDnsBlocklistUrl", L"");
	blocklist_text = _app_get_dnsblock_text (blocklist_url);

	if (blocklist_url)
		_r_obj_dereference (blocklist_url);

	if (_r_obj_isstringempty (blocklist_text))
	{
		if (blocklist_text)
			_r_obj_dereference (blocklist_text);

		return NULL;
	}

	sink = _r_config_getstring (L"ChromiumDnsBlocklistSink", CHROMIUM_DNS_BLOCKLIST_SINK);
	max_rules = _r_config_getlong (L"ChromiumDnsBlocklistMaxRules", 256);
	max_chars = _r_config_getlong (L"ChromiumDnsBlocklistMaxCommandLineChars", 12000);
	include_subdomains = _r_config_getboolean (L"ChromiumDnsBlocklistIncludeSubdomains", TRUE);

	if (max_rules <= 0 || max_rules > 2048)
		max_rules = 256;

	if (max_chars < 1024 || max_chars > 30000)
		max_chars = 12000;

	if (_r_obj_isstringempty (sink))
		_r_obj_movereference ((PVOID_PTR)&sink, _r_obj_createstring (CHROMIUM_DNS_BLOCKLIST_SINK));

	rules = _r_obj_createstring (L"");

	if (!rules || _r_obj_isstringempty (sink))
	{
		if (rules)
			_r_obj_dereference (rules);

		if (sink)
			_r_obj_dereference (sink);

		_r_obj_dereference (blocklist_text);

		return NULL;
	}

	ptr = blocklist_text->buffer;

	while (*ptr && count < (ULONG)max_rules)
	{
		line_length = 0;

		while (*ptr && *ptr != L'\r' && *ptr != L'\n')
		{
			if (line_length + 1 < RTL_NUMBER_OF (line_buffer))
				line_buffer[line_length++] = *ptr;

			ptr++;
		}

		line_buffer[line_length] = UNICODE_NULL;

		while (*ptr == L'\r' || *ptr == L'\n')
			ptr++;

		if (!_app_extract_dns_hostname_from_line (line_buffer, hostname, RTL_NUMBER_OF (hostname)))
			continue;

		if (!_app_append_dns_blocklist_rule (&rules, hostname, sink, include_subdomains, max_chars))
			break;

		count++;
	}

	_r_obj_dereference (blocklist_text);

	if (sink)
		_r_obj_dereference (sink);

	if (!count || _r_obj_isstringempty (rules))
	{
		if (rules)
			_r_obj_dereference (rules);

		return NULL;
	}

	next_rules = _r_format_string (L"--host-resolver-rules=\"%s\"", rules->buffer);

	_r_obj_dereference (rules);

	return next_rules;
}
