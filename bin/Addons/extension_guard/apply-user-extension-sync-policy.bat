@echo off
setlocal
set "SCRIPT=%~dp0apply-user-extension-sync-policy.ps1"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
pause
