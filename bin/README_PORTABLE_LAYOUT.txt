chrlauncher portable layout
==========================

Recommended start method:

  RUN_CHRLAUNCHER_X64.bat

The BAT uses %~dp0 as its starting directory, then walks upward until it finds
both files required to identify the portable root:

  chrlauncher.x64.exe
  Addons\config.ini

After the root is found, the BAT switches to that directory and starts:

  chrlauncher.x64.exe

Correct layout after extracting the GitHub Actions artifact:

  chrlauncher.x64.exe
  RUN_CHRLAUNCHER_X64.bat
  Addons\config.ini
  profile\                  created/used by Chromium
  bin\chrome.exe             downloaded/installed by chrlauncher
  bin.old\                   temporary old install folder during update

Important
---------

%~dp0 exists only in batch scripts. The native EXE does not have %~dp0.
The EXE calculates its own portable root from its executable directory.

This means:

  C:\cr\x\chrlauncher.x64.exe

uses:

  C:\cr\x

as root, but:

  C:\cr\x\bin\aa\chrlauncher.x64.exe

uses:

  C:\cr\x\bin\aa

as root.

So do not move only chrlauncher.x64.exe into bin\aa. Keep the full artifact
layout and run the BAT. The BAT can be launched from its own location and will
resolve the root using %~dp0 plus parent-directory probing.

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
