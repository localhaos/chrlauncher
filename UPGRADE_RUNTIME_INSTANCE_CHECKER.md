# Runtime instance checker upgrade

## Cel

Ta aktualizacja rozwiązuje przypadek, w którym GUI launchera nie pojawiało się, gdy uruchomiony był Chrome/Chromium z innej instalacji albo z innego profilu.

## Problem techniczny

Dotychczasowa ścieżka `_app_openbrowser()` bazowała na zajętości pliku binarnego `chrome.exe`:

```c
_r_fs_isfileused(&pbi->binary_path->sr)
```

To jest warunek zbyt szeroki. Zajęty plik binarny nie oznacza, że działa ta sama instancja portable. Efekt uboczny: launcher mógł potraktować obcą instalację albo obcy profil jako własną aktywną instancję i nie pokazać GUI.

## Nowa logika

Dodany patch `patches/runtime-instance-checker.patch` wprowadza detekcję po aktywnym katalogu `--user-data-dir`, a nie po samym pliku EXE. Sprawdzane są standardowe pliki singleton Chromium:

- `SingletonLock`
- `SingletonCookie`
- `SingletonSocket`

Launcher uznaje przeglądarkę za tę samą instancję tylko wtedy, gdy aktywny jest ten sam katalog profilu portable.

## Nowy parametr konfiguracyjny

W każdej sekcji configu dodano:

```ini
ChromiumSameInstanceCheck=true
```

Znaczenie:

- `true` — nowy tryb: rozróżnianie instancji po aktywnym profilu portable.
- `false` — fallback legacy: traktowanie procesu blokującego `ChromiumBinary` jako tej samej instancji.

## Integracja buildowa

Dodano:

- `tools/apply-local-patches.ps1` — idempotentne nakładanie patchy z katalogu `patches`.
- krok `Apply local runtime patches` w `.github/workflows/windows-build.yml`.
- automatyczne nakładanie patchy w `build_vc.bat` przed lokalnym buildem.

Dzięki temu GitHub Actions oraz lokalny `build_vc.bat` budują wariant z poprawioną logiką runtime bez ręcznego `git apply`.

## Semantyka po zmianie

- Chrome/Chromium z innej instalacji lub innego profilu: launcher ma się pojawić.
- Chrome/Chromium z tej samej instancji portable: launcher aktywuje istniejące okno / nie tworzy pustej drugiej instancji.
- URL-e i jawne `--new-window` / `--new-instance` zachowują dotychczasowe zachowanie.
