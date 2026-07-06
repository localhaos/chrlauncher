# chrlauncher split full-config fixed

Generated from uploaded archives.

## Inputs

- x(2).zip SHA256: a34c0eb8ca7f0b614882a24df165a4cd8eb91494f7a084abaaf4da543a762dfe
- not_loading_or_hang.zip SHA256: f4e29a23d0addfd21d578ba6f18b6a79581c5fd58f03f0dd5940db89dea979ef

## Applied changes

1. Rebuilt `bin/Addons/Config.ini`, `bin/Addons/Parameters.ini`, and `bin/Addons/Version.ini` from the old working one-file `bin/Addons/config.ini`.
2. Preserved all three runtime sections: `[chrlauncher]`, `[chrlauncher.x86]`, `[chrlauncher.x64]`.
3. Preserved old one-file comment blocks by moving each option block to the correct split file.
4. Kept new split-only keys that did not exist as active keys in the old file: `ChromePlusDirectory`, `ChromiumEnableSecureDns`, `ChromiumSecureDnsUrl`, `ChromiumUpdateUrl`.
5. Patched split-config reads to use current executable section first, then fallback to `[chrlauncher]`.
6. Patched config writes to use `_r_app_getnameshort()` instead of hardcoded `APP_NAME_SHORT`, matching the old working behavior.
7. Restored section-alias creation for `Config.ini`.

## Not verified

No Windows build was executed in this Linux sandbox. This is a source/config-level deterministic patch.
