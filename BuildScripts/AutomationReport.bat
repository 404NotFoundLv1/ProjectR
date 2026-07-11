@echo off
setlocal
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0ProjectRAutomation.ps1" -Operation AutomationReport %*
set "PR_EXIT=%ERRORLEVEL%"
endlocal & exit /b %PR_EXIT%
