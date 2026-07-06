#include "cfgread.h"

#include <wchar.h>

#define CFGREAD_MAX_FILE_SIZE (8 * 1024 * 1024)

struct _CFGREAD
{
	PWSTR data;
	PWSTR end;
};

static PVOID cfg_alloc (
	_In_ SIZE_T size
)
{
	return HeapAlloc (GetProcessHeap (), HEAP_ZERO_MEMORY, size);
}

static VOID cfg_free (
	_In_opt_ PVOID ptr
)
{
	if (ptr)
		HeapFree (GetProcessHeap (), 0, ptr);
}

static PWSTR cfg_next (
	_In_ PCFGREAD file,
	_In_ PWSTR ptr
)
{
	ptr += wcslen (ptr);

	while (ptr < file->end && *ptr == UNICODE_NULL)
		ptr++;

	return ptr;
}

static VOID cfg_trim_back (
	_In_ PCFGREAD file,
	_In_ PWSTR ptr
)
{
	while (ptr >= file->data && (*ptr == L' ' || *ptr == L'\t' || *ptr == L'\r'))
		*ptr-- = UNICODE_NULL;
}

static PWSTR cfg_discard_line (
	_In_ PCFGREAD file,
	_In_ PWSTR ptr
)
{
	while (ptr < file->end && *ptr != L'\n')
		*ptr++ = UNICODE_NULL;

	return ptr;
}

static PWSTR cfg_unescape_quoted_value (
	_In_ PCFGREAD file,
	_In_ PWSTR ptr
)
{
	PWSTR out = ptr;

	ptr++;

	while (ptr < file->end && *ptr != L'"' && *ptr != L'\r' && *ptr != L'\n')
	{
		if (*ptr == L'\\')
		{
			ptr++;

			switch (*ptr)
			{
				case L'r':
					*out = L'\r';
					break;

				case L'n':
					*out = L'\n';
					break;

				case L't':
					*out = L'\t';
					break;

				case L'\r':
				case L'\n':
				case UNICODE_NULL:
					goto CleanupExit;

				default:
					*out = *ptr;
					break;
			}
		}
		else
		{
			*out = *ptr;
		}

		out++;
		ptr++;
	}

CleanupExit:

	return out;
}

static VOID cfg_split_data (
	_In_ PCFGREAD file
)
{
	PWSTR line_start;
	PWSTR value_start;
	PWSTR ptr = file->data;

	while (ptr < file->end)
	{
		switch (*ptr)
		{
			case L'\r':
			case L'\n':
			case L'\t':
			case L' ':
				*ptr = UNICODE_NULL;
				ptr++;
				break;

			case UNICODE_NULL:
				ptr++;
				break;

			case L'[':
				ptr += wcscspn (ptr, L"]\n");

				if (ptr < file->end)
					*ptr = UNICODE_NULL;

				break;

			case L';':
			case L'#':
				ptr = cfg_discard_line (file, ptr);
				break;

			default:
				line_start = ptr;
				ptr += wcscspn (ptr, L"=\n");

				if (ptr >= file->end || *ptr != L'=')
				{
					ptr = cfg_discard_line (file, line_start);
					break;
				}

				cfg_trim_back (file, ptr - 1);

				do
				{
					*ptr++ = UNICODE_NULL;
				}
				while (ptr < file->end && (*ptr == L' ' || *ptr == L'\r' || *ptr == L'\t'));

				if (ptr >= file->end || *ptr == L'\n' || *ptr == UNICODE_NULL)
				{
					ptr = cfg_discard_line (file, line_start);
					break;
				}

				if (*ptr == L'"')
				{
					value_start = ptr;
					ptr = cfg_unescape_quoted_value (file, ptr);

					if (ptr == value_start)
					{
						ptr = cfg_discard_line (file, line_start);
						break;
					}

					ptr = cfg_discard_line (file, ptr);
				}
				else
				{
					ptr += wcscspn (ptr, L"\n");
					cfg_trim_back (file, ptr - 1);
				}

				break;
		}
	}
}

static BOOLEAN cfg_convert_utf16le (
	_In_reads_bytes_ (bytes_length) PBYTE bytes,
	_In_ DWORD bytes_length,
	_Outptr_result_maybenull_ PWSTR *text,
	_Out_ DWORD *text_length
)
{
	DWORD char_count;

	*text = NULL;
	*text_length = 0;

	if (bytes_length < 2)
		return FALSE;

	char_count = (bytes_length - 2) / sizeof (WCHAR);
	*text = cfg_alloc (((SIZE_T)char_count + 1) * sizeof (WCHAR));

	if (!*text)
		return FALSE;

	CopyMemory (*text, bytes + 2, (SIZE_T)char_count * sizeof (WCHAR));
	(*text)[char_count] = UNICODE_NULL;
	*text_length = char_count;

	return TRUE;
}

static BOOLEAN cfg_convert_multibyte (
	_In_reads_bytes_ (bytes_length) PBYTE bytes,
	_In_ DWORD bytes_length,
	_In_ UINT codepage,
	_In_ DWORD flags,
	_Outptr_result_maybenull_ PWSTR *text,
	_Out_ DWORD *text_length
)
{
	INT required;
	INT converted;
	PBYTE source = bytes;
	DWORD source_length = bytes_length;

	*text = NULL;
	*text_length = 0;

	if (bytes_length >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
	{
		source += 3;
		source_length -= 3;
	}

	required = MultiByteToWideChar (codepage, flags, (LPCCH)source, (INT)source_length, NULL, 0);

	if (required <= 0)
		return FALSE;

	*text = cfg_alloc (((SIZE_T)required + 1) * sizeof (WCHAR));

	if (!*text)
		return FALSE;

	converted = MultiByteToWideChar (codepage, flags, (LPCCH)source, (INT)source_length, *text, required);

	if (converted <= 0)
	{
		cfg_free (*text);
		*text = NULL;
		return FALSE;
	}

	(*text)[converted] = UNICODE_NULL;
	*text_length = (DWORD)converted;

	return TRUE;
}

PMINI_INI cfgread_open (
	_In_ PR_STRING path
)
{
	LARGE_INTEGER file_size;
	PCFGREAD file = NULL;
	PBYTE bytes = NULL;
	HANDLE handle;
	DWORD bytes_read = 0;
	DWORD text_length = 0;
	PWSTR text = NULL;
	BOOLEAN converted = FALSE;

	if (_r_obj_isstringempty (path))
		return NULL;

	handle = CreateFileW (path->buffer, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle == INVALID_HANDLE_VALUE)
		return NULL;

	if (!GetFileSizeEx (handle, &file_size) || file_size.QuadPart <= 0 || file_size.QuadPart > CFGREAD_MAX_FILE_SIZE)
		goto CleanupExit;

	bytes = cfg_alloc ((SIZE_T)file_size.QuadPart);

	if (!bytes)
		goto CleanupExit;

	if (!ReadFile (handle, bytes, (DWORD)file_size.QuadPart, &bytes_read, NULL) || bytes_read != (DWORD)file_size.QuadPart)
		goto CleanupExit;

	if (bytes_read >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE)
		converted = cfg_convert_utf16le (bytes, bytes_read, &text, &text_length);

	if (!converted)
		converted = cfg_convert_multibyte (bytes, bytes_read, CP_UTF8, MB_ERR_INVALID_CHARS, &text, &text_length);

	if (!converted)
		converted = cfg_convert_multibyte (bytes, bytes_read, CP_ACP, 0, &text, &text_length);

	if (!converted || !text)
		goto CleanupExit;

	file = cfg_alloc (sizeof (*file));

	if (!file)
		goto CleanupExit;

	file->data = text;
	file->end = text + text_length;
	text = NULL;

	cfg_split_data (file);

CleanupExit:

	if (handle != INVALID_HANDLE_VALUE)
		CloseHandle (handle);

	cfg_free (bytes);
	cfg_free (text);

	return file;
}

VOID cfgread_close (
	_In_opt_ PCFGREAD file
)
{
	if (!file)
		return;

	cfg_free (file->data);
	cfg_free (file);
}

LPCWSTR cfgread_get (
	_In_opt_ PCFGREAD file,
	_In_opt_ LPCWSTR section_name,
	_In_ LPCWSTR key_name
)
{
	LPCWSTR current_section = L"";
	PWSTR value;
	PWSTR ptr;

	if (!file || !file->data || !key_name || !key_name[0])
		return NULL;

	ptr = file->data;

	if (*ptr == UNICODE_NULL)
		ptr = cfg_next (file, ptr);

	while (ptr < file->end)
	{
		if (*ptr == L'[')
		{
			current_section = ptr + 1;
		}
		else
		{
			value = cfg_next (file, ptr);

			if ((!section_name || !section_name[0] || lstrcmpiW (section_name, current_section) == 0) && lstrcmpiW (ptr, key_name) == 0)
				return value;

			ptr = value;
		}

		ptr = cfg_next (file, ptr);
	}

	return NULL;
}
