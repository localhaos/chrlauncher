@echo off
setlocal EnableExtensions EnableDelayedExpansion

cd /d "%~dp0"

echo ============================================================
echo chrlauncher local MSBuild helper
echo ============================================================
echo Root: "%CD%"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VCVARS="

if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -find VC\Auxiliary\Build\vcvarsall.bat`) do (
        set "VCVARS=%%I"
        goto :found_vcvars
    )
)

for %%V in (18 17) do (
    for %%E in (BuildTools Enterprise Professional Community) do (
        if exist "%ProgramFiles%\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvarsall.bat" (
            set "VCVARS=%ProgramFiles%\Microsoft Visual Studio\%%V\%%E\VC\Auxiliary\Build\vcvarsall.bat"
            goto :found_vcvars
        )
    )
)

echo [ERR] Visual Studio C++ Build Tools not found.
echo Install Visual Studio 2022/2026 Build Tools with workload: Desktop development with C++.
goto :fail

:found_vcvars
echo [INFO] vcvarsall: "%VCVARS%"
call "%VCVARS%" x64
if errorlevel 1 goto :fail

set "TOOLSET_ARG="
if defined VCTargetsPath (
    if exist "%VCTargetsPath%Platforms\Win32\PlatformToolsets\v145\Toolset.props" set "TOOLSET_ARG=/p:PlatformToolset=v145"
    if not defined TOOLSET_ARG if exist "%VCTargetsPath%Platforms\Win32\PlatformToolsets\v143\Toolset.props" set "TOOLSET_ARG=/p:PlatformToolset=v143"
)

if defined TOOLSET_ARG (
    echo [INFO] %TOOLSET_ARG%
) else (
    echo [WARN] No explicit v145/v143 toolset override detected; using project defaults.
)

for %%P in (Win32 x64) do (
    echo.
    echo [BUILD] Release^|%%P
    msbuild chrlauncher.sln /m /p:Configuration=Release /p:Platform=%%P /p:PreferredToolArchitecture=x64 %TOOLSET_ARG% /verbosity:minimal
    if errorlevel 1 goto :fail
)

echo.
echo [OK] Build finished.
echo Expected outputs:
echo   bin\chrlauncher.x86.exe
echo   bin\chrlauncher.x64.exe
goto :end

:fail
echo.
echo [ERR] Build failed.
exit /b 1

:end
endlocal
exit /b 0
