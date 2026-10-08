@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tests\verify.ps1" %*
exit /b %errorlevel%
