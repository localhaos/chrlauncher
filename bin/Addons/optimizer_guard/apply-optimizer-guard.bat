@echo off
setlocal
set "SCRIPT=%~dp0apply-optimizer-guard.ps1"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT%" %*
pause
