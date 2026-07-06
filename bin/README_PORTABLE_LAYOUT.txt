chrlauncher portable layout
==========================

This branch builds a conservative low-memory restore artifact based on the
last runner-root version that behaved correctly for local path resolution.

Run chrlauncher.x64.exe from the artifact root directory only, or use:

  RUN_CHRLAUNCHER_X64.bat

Do not move the executable into bin\aa, bin\chrome, or any nested directory.

Correct layout after extracting the GitHub Actions artifact:

  chrlauncher.x64.exe
  RUN_CHRLAUNCHER_X64.bat
  Addons\config.ini
  profile\                  created/used by Chromium
  bin\chrome.exe             downloaded/installed by chrlauncher
  bin.old\                   temporary old install folder during update

Low-memory profile
------------------

The low-memory artifact disables optional Chromium startup features that can
increase memory pressure on systems with a small or disabled pagefile:

  ChromiumEnableLosslessOptimization=false
  ChromiumEnableTextPerformanceFixes=false
  ChromiumIgnoreGpuBlocklist=false
  ChromiumEnableScrollableTabs=false
  ChromiumEnableCloseTabsRightExtension=false
  ChromiumEnableAutofillPasswordFixes=false

This is intended to reduce STATUS_NO_MEMORY / 0xC0000017 during startup.

Why this matters
----------------

chrlauncher calculates its portable root from the directory that contains
chrlauncher.x64.exe. If you run it from:

  C:\cr\x\bin\aa\chrlauncher.x64.exe

then the application root becomes:

  C:\cr\x\bin\aa

and the launcher will look for:

  C:\cr\x\bin\aa\Addons\config.ini
  C:\cr\x\bin\aa\bin\chrome.exe

That is not the intended layout and can produce misleading startup errors.

Expected behavior when bin\chrome.exe is missing
------------------------------------------------

Missing bin\chrome.exe is not fatal in the known-good baseline.
The launcher should show its GUI/updater and allow Chromium to be downloaded
or repaired.

If bin.old is locked
--------------------

Error 0xC0000043 means Windows denied access because another process still
holds files from the local Chromium install.

Close local Chrome/Chromium processes from this portable instance, then remove:

  bin.old
  bin.update

Suggested commands:

  taskkill /F /IM chrome.exe
  taskkill /F /IM chrlauncher.x64.exe
  rmdir /S /Q bin.old
  rmdir /S /Q bin.update

Then run RUN_CHRLAUNCHER_X64.bat again.
