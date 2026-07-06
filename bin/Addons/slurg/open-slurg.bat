@echo off
setlocal
set "ROOT=%~dp0..\.."
set "HTML=%~dp0index.html"
set "CHROME=%ROOT%\bin\chrome.exe"

if exist "%CHROME%" (
  start "" "%CHROME%" --user-data-dir="%ROOT%\Data" "%HTML%"
) else (
  start "" "%HTML%"
)
