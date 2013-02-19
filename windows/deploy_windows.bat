@echo off

rem settings and check ---------------------------------------------------------
set NSIS_PATH=%PROGRAMFILES%\NSIS

if "%VSINSTALLDIR%" == "" goto vsenvproblem

rem create visual studio project file ------------------------------------------
cd ..
set QMAKESPEC=win32-msvc2010
qmake -tp vc

rem clean and compile solution -------------------------------------------------
vcexpress Jamulus.vcxproj /clean "Release|Win32"
vcexpress Jamulus.vcxproj /build "Release|Win32"

rem create installer -----------------------------------------------------------
cd windows
"%NSIS_PATH%\makensis.exe" installer.nsi

move Jamulusinstaller.exe ../deploy/Jamulus-version-installer.exe

goto endofskript

:vsenvproblem
echo Use the Visual Studio Command Prompt to call this skript

:endofskript
