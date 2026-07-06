[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
$path = Join-Path $repoRoot 'src\app_dns.c'

if (-not (Test-Path -LiteralPath $path)) {
    throw "Missing source file: $path"
}

$text = Get-Content -LiteralPath $path -Raw

if ($text -match '_app_get_dnsblock_args_cache_key') {
    Write-Host '[PREP] DNS fast cache patch already present.'
    exit 0
}

$newDnsTextBlock = @'
static BOOLEAN _app_is_http_url (
	_In_opt_ PR_STRING url
)
{
	if (_r_obj_isstringempty (url))
		return FALSE;

	return _wcsnicmp (url->buffer, L"https://", 8) == 0 || _wcsnicmp (url->buffer, L"http://", 7) == 0;
}

static PR_STRING _app_read_local_dnsblock_text (
	_In_ PR_STRING local_path
)
{
	PR_STRING resolved_path;
	PR_STRING text = NULL;

	if (_r_obj_isstringempty (local_path))
		return NULL;

	resolved_path = _app_resolve_configured_path (local_path, NULL, FALSE);

	if (!resolved_path)
		return NULL;

	text = _app_read_text_cache_file (resolved_path, 0, TRUE);

	_r_obj_dereference (resolved_path);

	return text;
}

static PR_STRING _app_get_dnsblock_args_cache_key (
	_In_ PR_STRING blocklist_url,
	_In_ PR_STRING sink,
	_In_ LONG max_rules,
	_In_ LONG max_chars,
	_In_ BOOLEAN include_subdomains
)
{
	return _r_format_string (
		L"args|%s|%s|%d|%d|%d",
		_r_obj_getstring (blocklist_url),
		_r_obj_getstring (sink),
		max_rules,
		max_chars,
		include_subdomains ? 1 : 0
	);
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

	if (!_app_is_http_url (blocklist_url))
		return _app_read_local_dnsblock_text (blocklist_url);

	use_cache = _app_config_getboolean (L"ChromiumDnsBlocklistCache", TRUE);
	max_age_hours = _app_config_getlong (L"ChromiumDnsBlocklistCacheMaxAgeHours", 24);

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

static BOOLEAN _app_is_dns_hostname_char
'@

$text = [regex]::Replace(
    $text,
    'static PR_STRING _app_get_dnsblock_text \([\s\S]*?\r?\n}\r?\n\r?\nstatic BOOLEAN _app_is_dns_hostname_char',
    [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $newDnsTextBlock },
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

$newCreateBlock = @'
PR_STRING _app_create_dns_blocklist_arguments ()
{
	PR_STRING args_cache_key = NULL;
	PR_STRING args_cache_path = NULL;
	PR_STRING blocklist_text = NULL;
	PR_STRING blocklist_url;
	PR_STRING cached_arguments = NULL;
	PR_STRING next_rules;
	PR_STRING rules = NULL;
	PR_STRING secure_dns_arguments;
	PR_STRING sink;
	LPCWSTR ptr;
	LONG max_age_hours;
	LONG max_chars;
	LONG max_rules;
	ULONG count = 0;
	BOOLEAN include_subdomains;
	BOOLEAN use_cache;
	WCHAR hostname[256];
	WCHAR line_buffer[1024];
	ULONG_PTR line_length;

	secure_dns_arguments = _app_create_secure_dns_arguments_from_config ();

	if (secure_dns_arguments)
		return secure_dns_arguments;

	if (!_app_config_getboolean (L"ChromiumEnableDnsBlocklistUrl", FALSE))
		return NULL;

	blocklist_url = _app_config_getstringexpand (L"ChromiumDnsBlocklistUrl", L"");

	if (_app_is_rethinkdns_template (blocklist_url))
	{
		secure_dns_arguments = _app_create_secure_dns_arguments (blocklist_url);
		_r_obj_dereference (blocklist_url);

		return secure_dns_arguments;
	}

	sink = _app_config_getstring (L"ChromiumDnsBlocklistSink", CHROMIUM_DNS_BLOCKLIST_SINK);
	max_rules = _app_config_getlong (L"ChromiumDnsBlocklistMaxRules", 256);
	max_chars = _app_config_getlong (L"ChromiumDnsBlocklistMaxCommandLineChars", 12000);
	max_age_hours = _app_config_getlong (L"ChromiumDnsBlocklistCacheMaxAgeHours", 24);
	include_subdomains = _app_config_getboolean (L"ChromiumDnsBlocklistIncludeSubdomains", TRUE);
	use_cache = _app_config_getboolean (L"ChromiumDnsBlocklistCache", TRUE);

	if (max_rules <= 0 || max_rules > 2048)
		max_rules = 256;

	if (max_chars < 1024 || max_chars > 30000)
		max_chars = 12000;

	if (max_age_hours < 1 || max_age_hours > 24 * 30)
		max_age_hours = 24;

	if (_r_obj_isstringempty (sink))
		_r_obj_movereference ((PVOID_PTR)&sink, _r_obj_createstring (CHROMIUM_DNS_BLOCKLIST_SINK));

	if (_r_obj_isstringempty (blocklist_url) || _r_obj_isstringempty (sink))
		goto CleanupExit;

	if (use_cache)
	{
		args_cache_key = _app_get_dnsblock_args_cache_key (blocklist_url, sink, max_rules, max_chars, include_subdomains);

		if (args_cache_key)
		{
			args_cache_path = _app_get_dnsblock_cache_path (args_cache_key);
			cached_arguments = _app_read_text_cache_file (args_cache_path, max_age_hours, FALSE);

			if (!_r_obj_isstringempty (cached_arguments))
				goto CleanupExit;

			if (cached_arguments)
				_r_obj_clearreference (&cached_arguments);
		}
	}

	blocklist_text = _app_get_dnsblock_text (blocklist_url);

	if (_r_obj_isstringempty (blocklist_text))
		goto CleanupExit;

	rules = _r_obj_createstring (L"");

	if (!rules)
		goto CleanupExit;

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

	if (!count || _r_obj_isstringempty (rules))
		goto CleanupExit;

	next_rules = _r_format_string (L"--host-resolver-rules=\"%s\"", rules->buffer);

	if (next_rules && args_cache_path)
		_app_write_text_cache_file (args_cache_path, next_rules);

	_r_obj_movereference (&cached_arguments, next_rules);

CleanupExit:

	if (args_cache_key)
		_r_obj_dereference (args_cache_key);

	if (args_cache_path)
		_r_obj_dereference (args_cache_path);

	if (blocklist_text)
		_r_obj_dereference (blocklist_text);

	if (blocklist_url)
		_r_obj_dereference (blocklist_url);

	if (rules)
		_r_obj_dereference (rules);

	if (sink)
		_r_obj_dereference (sink);

	return cached_arguments;
}
'@

$text = [regex]::Replace(
    $text,
    'PR_STRING _app_create_dns_blocklist_arguments \([\s\S]*?\r?\n}\s*$',
    [System.Text.RegularExpressions.MatchEvaluator]{ param($m) $newCreateBlock },
    [System.Text.RegularExpressions.RegexOptions]::Singleline
)

Set-Content -LiteralPath $path -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] DNS local-file + generated-rules cache patch applied.'
