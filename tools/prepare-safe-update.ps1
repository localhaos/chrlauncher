[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$repoRoot = Resolve-Path -LiteralPath (Join-Path $PSScriptRoot '..')
$main = Join-Path $repoRoot 'src\main.c'

if (-not (Test-Path -LiteralPath $main)) {
    throw "Missing source file: $main"
}

$text = Get-Content -LiteralPath $main -Raw

if ($text -match 'STATUS_FILE_LOCKED_WITH_ONLY_READERS') {
    Write-Host '[PREP] Safe update patch already present.'
    exit 0
}

$oldMoveBackup = @'
			if (_r_fs_exists (&pbi->binary_dir->sr))
			{
				status = _r_fs_movefile (&pbi->binary_dir->sr, &backup_dir->sr, FALSE);

				if (NT_SUCCESS (status))
					is_backup_created = TRUE;
				else
					_r_show_errormessage (hwnd, NULL, status, backup_dir->buffer, ET_NATIVE);
			}
'@

$newMoveBackup = @'
			if (_r_fs_exists (&pbi->binary_dir->sr))
			{
				status = _r_fs_movefile (&pbi->binary_dir->sr, &backup_dir->sr, FALSE);

				if (NT_SUCCESS (status))
				{
					is_backup_created = TRUE;
				}
				else
				{
					// Existing Chromium can be running and holding DLL/PAK/EXE files.
					// Do not treat a locked install directory as a launcher failure.
					// Leave the installed browser intact, skip this update, and allow the
					// normal RunAtEnd path to start the existing chrome.exe.
					*is_error_ptr = FALSE;
					status = STATUS_FILE_LOCKED_WITH_ONLY_READERS;
					goto CleanupExit;
				}
			}
'@

if (-not $text.Contains($oldMoveBackup)) {
    throw 'Cannot locate binary_dir -> backup_dir move block.'
}

$text = $text.Replace($oldMoveBackup, $newMoveBackup)

$oldMoveStaging = @'
			if (NT_SUCCESS (status))
			{
				status = _r_fs_movefile (&staging_dir->sr, &pbi->binary_dir->sr, FALSE);

				if (!NT_SUCCESS (status))
					_r_show_errormessage (hwnd, NULL, status, pbi->binary_dir->buffer, ET_NATIVE);
			}
'@

$newMoveStaging = @'
			if (NT_SUCCESS (status))
			{
				status = _r_fs_movefile (&staging_dir->sr, &pbi->binary_dir->sr, FALSE);

				if (!NT_SUCCESS (status))
				{
					// If placing the new staging directory fails, keep rollback quiet when
					// the old installation can be restored. This prevents startup from
					// becoming fatal while Chrome is still running.
					*is_error_ptr = FALSE;
				}
			}
'@

if (-not $text.Contains($oldMoveStaging)) {
    throw 'Cannot locate staging_dir -> binary_dir move block.'
}

$text = $text.Replace($oldMoveStaging, $newMoveStaging)

$oldErrorAssignment = @'
	*is_error_ptr = (status != SZ_OK);

	_r_queuedlock_releaseshared (&lock_download);
'@

$newErrorAssignment = @'
	if (status == STATUS_FILE_LOCKED_WITH_ONLY_READERS)
		*is_error_ptr = FALSE;
	else
		*is_error_ptr = (status != SZ_OK);

	_r_queuedlock_releaseshared (&lock_download);
'@

if (-not $text.Contains($oldErrorAssignment)) {
    throw 'Cannot locate is_error_ptr assignment.'
}

$text = $text.Replace($oldErrorAssignment, $newErrorAssignment)

$oldDownloadedFileUsed = @'
		else
		{
			_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

			is_stayopen = TRUE;
		}
'@

$newDownloadedFileUsed = @'
		else
		{
			// Update package exists but portable Chromium is running.
			// Keep launcher usable instead of treating this as a fatal update state.
			if (_app_config_getboolean (L"ChromiumRunAtEnd", TRUE) && !pbi->is_onlyupdate)
				_app_openbrowser (hwnd, pbi);

			_r_ctrl_enable (hwnd, IDC_START_BTN, TRUE);

			is_stayopen = TRUE;
		}
'@

if (-not $text.Contains($oldDownloadedFileUsed)) {
    throw 'Cannot locate downloaded-file-used branch.'
}

$text = $text.Replace($oldDownloadedFileUsed, $newDownloadedFileUsed)

Set-Content -LiteralPath $main -Value $text -NoNewline -Encoding UTF8
Write-Host '[PREP] Safe update patch applied.'
