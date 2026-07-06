[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
$appDns = Join-Path $repoRoot 'src\app_dns.c'

if (-not (Test-Path -LiteralPath $appDns)) {
    throw "Missing source file: $appDns"
}

$text = Get-Content -LiteralPath $appDns -Raw

$text = $text.Replace("PR_STRING _app_download_text_url (`r`n`t_In_ LPCWSTR url`r`n);", "PR_STRING _app_download_text_url (`r`n`t_In_ PR_STRING url`r`n);")
$text = $text.Replace("PR_STRING _app_download_text_url (`n`t_In_ LPCWSTR url`n);", "PR_STRING _app_download_text_url (`n`t_In_ PR_STRING url`n);")
$text = $text.Replace('text = _app_download_text_url (blocklist_url->buffer);', 'text = _app_download_text_url (blocklist_url);')
$text = $text.Replace("if (max_rules <= 0 || max_rules > 2048)`r`n`t`tmax_rules = 256;", "if (max_rules <= 0 || max_rules > 512)`r`n`t`tmax_rules = 256;")
$text = $text.Replace("if (max_rules <= 0 || max_rules > 2048)`n`t`tmax_rules = 256;", "if (max_rules <= 0 || max_rules > 512)`n`t`tmax_rules = 256;")
$text = $text.Replace("if (max_chars < 1024 || max_chars > 30000)`r`n`t`tmax_chars = 12000;", "if (max_chars < 1024 || max_chars > 12000)`r`n`t`tmax_chars = 12000;")
$text = $text.Replace("if (max_chars < 1024 || max_chars > 30000)`n`t`tmax_chars = 12000;", "if (max_chars < 1024 || max_chars > 12000)`n`t`tmax_chars = 12000;")

if ($text -notmatch '_app_is_supported_dns_blocklist_url') {
    $marker = @'
PR_STRING _app_create_secure_dns_arguments_from_config ()
{
	PR_STRING arguments;
	PR_STRING dns_template;

	if (!_app_config_getboolean (L"ChromiumEnableSecureDns", FALSE))
		return NULL;

	dns_template = _app_config_getstringexpand (L"ChromiumSecureDnsUrl", L"");

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

'@

    $insert = @'
PR_STRING _app_create_secure_dns_arguments_from_config ()
{
	PR_STRING arguments;
	PR_STRING dns_template;

	if (!_app_config_getboolean (L"ChromiumEnableSecureDns", FALSE))
		return NULL;

	dns_template = _app_config_getstringexpand (L"ChromiumSecureDnsUrl", L"");

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

static BOOLEAN _app_is_http_text_url (
	_In_opt_ PR_STRING url
)
{
	LPCWSTR ptr;

	if (_r_obj_isstringempty (url))
		return FALSE;

	if (url->length > (2048 * sizeof (WCHAR)))
		return FALSE;

	if (_wcsnicmp (url->buffer, L"https://", 8) != 0 && _wcsnicmp (url->buffer, L"http://", 7) != 0)
		return FALSE;

	for (ptr = url->buffer; *ptr; ptr++)
	{
		if (*ptr <= L' ' || *ptr == L'"')
			return FALSE;
	}

	return TRUE;
}

static BOOLEAN _app_is_rethinkdns_stamp_url (
	_In_ PR_STRING url
)
{
	if (_r_obj_isstringempty (url))
		return FALSE;

	return wcsstr (url->buffer, L"sky.rethinkdns.com/1:") != NULL ||
		wcsstr (url->buffer, L"rethinkdns.com/1:") != NULL;
}

static BOOLEAN _app_is_supported_dns_blocklist_url (
	_In_opt_ PR_STRING url
)
{
	if (!_app_is_http_text_url (url))
		return FALSE;

	// RethinkDNS sky.rethinkdns.com/1:... is a DoH stamp/template URL, not a
	// hosts/adblock text list. Use ChromiumSecureDnsUrl with ChromiumEnableSecureDns=true for DoH.
	if (_app_is_rethinkdns_stamp_url (url))
		return FALSE;

	return TRUE;
}

static BOOLEAN _app_is_dnsblock_text_size_valid (
	_In_opt_ PR_STRING text
)
{
	if (_r_obj_isstringempty (text))
		return FALSE;

	return text->length <= (1024 * 1024 * sizeof (WCHAR));
}

'@

    if (-not $text.Contains($marker)) {
        throw 'Cannot locate secure DNS function block in app_dns.c.'
    }

    $text = $text.Replace($marker, $insert)
}

$text = $text.Replace("`tif (_r_obj_isstringempty (blocklist_url))`r`n`t`treturn NULL;", "`tif (_r_obj_isstringempty (blocklist_url) || !_app_is_supported_dns_blocklist_url (blocklist_url))`r`n`t`treturn NULL;")
$text = $text.Replace("`tif (_r_obj_isstringempty (blocklist_url))`n`t`treturn NULL;", "`tif (_r_obj_isstringempty (blocklist_url) || !_app_is_supported_dns_blocklist_url (blocklist_url))`n`t`treturn NULL;")

$oldCreate = @'
	blocklist_url = _app_config_getstringexpand (L"ChromiumDnsBlocklistUrl", L"");
	blocklist_text = _app_get_dnsblock_text (blocklist_url);

	if (blocklist_url)
'@
$newCreate = @'
	blocklist_url = _app_config_getstringexpand (L"ChromiumDnsBlocklistUrl", L"");

	if (!_app_is_supported_dns_blocklist_url (blocklist_url))
	{
		if (blocklist_url)
			_r_obj_dereference (blocklist_url);

		return NULL;
	}

	blocklist_text = _app_get_dnsblock_text (blocklist_url);

	if (blocklist_url)
'@
$text = $text.Replace($oldCreate, $newCreate)

$oldSize = @'
	if (!text && use_cache && cache_path)
		text = _app_read_text_cache_file (cache_path, max_age_hours, TRUE);

	if (cache_path)
'@
$newSize = @'
	if (!text && use_cache && cache_path)
		text = _app_read_text_cache_file (cache_path, max_age_hours, TRUE);

	if (!_app_is_dnsblock_text_size_valid (text))
	{
		if (text)
			_r_obj_dereference (text);

		text = NULL;
	}

	if (cache_path)
'@
$text = $text.Replace($oldSize, $newSize)

Set-Content -LiteralPath $appDns -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] v7 DNS blocklist crash fixes applied.'
