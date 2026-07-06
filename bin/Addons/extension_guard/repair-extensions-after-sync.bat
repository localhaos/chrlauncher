@echo off
setlocal
set "SCRIPT=%~dp0repair-extensions-after-sync.ps1"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
pause
