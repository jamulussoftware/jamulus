@echo off

rem To set up a new Qt and Visual Studio version
rem - set environment path variable to the correct Qt bin directory (e.g. 32 bit version)
rem - if using Visual Studio Express version, download the redistributable (it is not automatically there)
rem - change the Qt distribute dll names in the installer.nsi file (some contain version numbers)

rem settings and check ---------------------------------------------------------
set NSIS_PATH=%PROGRAMFILES(x86)%\NSIS

if "%VSINSTALLDIR%" == "" goto vsenvproblem

rem create visual studio project file ------------------------------------------
cd ..
set QMAKESPEC=win32-msvc2015
qmake -tp vc

rem clean and compile solution -------------------------------------------------
devenv Jamulus.vcxproj /Clean "Release|x86"
devenv Jamulus.vcxproj /Build "Release|x86"

rem create installer -----------------------------------------------------------
cd windows
"%NSIS_PATH%\makensis.exe" installer.nsi

move Jamulusinstaller.exe ../deploy/Jamulus-version-installer.exe

goto endofskript

:vsenvproblem
echo Use the Visual Studio Command Prompt to call this skript

:endofskript
