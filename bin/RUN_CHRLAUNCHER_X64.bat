@echo off
setlocal EnableExtensions DisableDelayedExpansion

set "START_DIR=%~dp0"
set "ROOT="

for %%I in ("%START_DIR%.") do set "CUR=%%~fI"

:find_root
for %%B in ("%CUR%") do set "CUR_NAME=%%~nxB"
if /I "%CUR_NAME%"=="Addons" goto :next_parent

if exist "%CUR%\chrlauncher.x64.exe" if exist "%CUR%\Addons\config.ini" (
    set "ROOT=%CUR%"
    goto :root_found
)

:next_parent
for %%I in ("%CUR%\..") do set "NEXT=%%~fI"
if /I "%NEXT%"=="%CUR%" goto :root_missing
set "CUR=%NEXT%"
goto :find_root

:root_missing
echo ============================================================
echo chrlauncher portable x64 runner
echo ============================================================
echo [ERR] Portable root was not found from: "%START_DIR%"
echo.
echo Expected layout at the same level or in a parent directory:
echo.
echo   chrlauncher.x64.exe
echo   Addons\config.ini
echo   profile\                 ^(created/used by Chromium^)
echo   Chromium\chrome.exe       ^(downloaded/installed by launcher^)
echo.
echo Reserved folder names:
echo   Addons  - configuration folder, never portable root
echo   bin     - legacy/runner output folder, avoid as install root
echo.
echo Do not extract the whole package into C:\good\Addons.
echo Use for example C:\good\chrlauncher or C:\cr\x.
echo.
pause
exit /b 3

:root_found
cd /d "%ROOT%"

echo ============================================================
echo chrlauncher portable x64 runner
echo ============================================================
echo BAT:  "%START_DIR%"
echo ROOT: "%ROOT%"
echo EXE:  "%ROOT%\chrlauncher.x64.exe"
echo CFG:  "%ROOT%\Addons\config.ini"
echo CHR:  "%ROOT%\Chromium\chrome.exe"
echo.

if exist "%ROOT%\bin.old" (
    echo [WARN] bin.old exists. If update/install fails with 0xC0000043,
    echo        close local Chrome processes and remove bin.old before retrying.
    echo.
)

if exist "%ROOT%\Chromium.old" (
    echo [WARN] Chromium.old exists. If update/install fails, close local Chrome
    echo        processes and remove Chromium.old before retrying.
    echo.
)

start "" "%ROOT%\chrlauncher.x64.exe" %*
endlocal
exit /b 0
