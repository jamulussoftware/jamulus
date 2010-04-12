@echo off

rem settings and check ---------------------------------------------------------
set NSIS_PATH=%PROGRAMFILES%\NSIS

if "%VSINSTALLDIR%" == "" goto vsenvproblem

rem clean and compile solution -------------------------------------------------
devenv llcon.sln /clean "Release|Win32"
devenv llcon.sln /build "Release|Win32"

rem create installer -----------------------------------------------------------
%NSIS_PATH%\makensis.exe installer.nsi

move llconinstaller.exe ../deploy/llcon-version-installer.exe

goto endofskript

:vsenvproblem
echo Use the Visual Studio Command Prompt to call this skript

:endofskript
