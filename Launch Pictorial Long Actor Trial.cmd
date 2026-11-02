@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\run-open-world-research.ps1" -Mode PictorialLongActorTrial
if errorlevel 1 pause
