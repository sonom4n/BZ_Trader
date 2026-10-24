@echo off
rem BZ_Trader - Traveling_Trader on Wheels: the editor for the server this folder is in, with a local server (this PC only).
rem BZ_Trader: el editor para el server donde esta esta carpeta, con un servidor local (solo en esta PC).
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Editor.ps1" %*
if errorlevel 1 pause
