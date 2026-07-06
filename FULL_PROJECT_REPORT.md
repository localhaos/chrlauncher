# chrlauncher full project fix

## Scope

This package restores the old working per-executable configuration semantics and keeps the newer split-config layout.

## Fixed behavior

- `src/app_config.c` resolves config keys in this order:
  1. section matching current executable name from `_r_app_getnameshort()`;
  2. fallback section `[chrlauncher]`;
  3. legacy single-file fallback for non-base split keys;
  4. in-memory parsed config/default value.
- `src/app_paths.c` writes config through `_r_app_getnameshort()` instead of hard-coded `APP_NAME_SHORT`.
- `Config.ini`, `Parameters.ini`, `Version.ini` include full `[chrlauncher]`, `[chrlauncher.x86]`, `[chrlauncher.x64]` sections.
- Local and CI builds target `Win32` and `x64` only. ARM64 was removed from the default build matrix because the project package contains x86/x64 binary library paths and the requested target was x86/x64.

## Added/updated build layer

- `build_vc.bat` detects Visual Studio 2026/2022 via `vswhere` and fallback paths.
- `build_vc.bat` selects `v145` when available, then `v143`, otherwise uses project defaults.
- `.github/workflows/windows-build.yml` detects available MSVC toolset and publishes separate x86/x64 artifacts.

## Known limitation

The sandbox used to prepare this package cannot execute Windows/MSVC builds. The fix is source-level and CI-level, with deterministic files and checksums.
