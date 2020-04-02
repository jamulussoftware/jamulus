@echo off

rem To set up a new Qt and Visual Studio version
rem - set environment path variable to the correct Qt bin directories:
rem   - QTDIR32: points to the Qt 32 bit binaries, e.g., C:\Qt\5.10.1\msvc2015
rem   - QTDIR64: points to the Qt 64 bin binaries, e.g., C:\Qt\5.10.1\msvc2015_64
rem - if using Visual Studio Express version, download the redistributable (it is not automatically there)
rem - change the Qt distribute dll names in the installer.nsi file (some contain version numbers)

rem settings and check ---------------------------------------------------------
set NSIS_PATH=%PROGRAMFILES(x86)%\NSIS
set VS_REDIST32_EXE=vc_redist.x86.exe
set VS_REDIST64_EXE=VC_redist.x64.exe

if "%VSINSTALLDIR%" == "" goto vsenvproblem
if "%QTDIR32%" == "" goto qtdirproblem
if "%QTDIR64%" == "" goto qtdirproblem
cd ..


rem ########################## 32 bit build ####################################
set QTDIR=%QTDIR32%
set Path=%QTDIR32%\bin;%Path%
rem create visual studio project file ------------------------------------------
qmake -tp vc -spec win32-msvc

rem TODO qmake seems to use the incorrect VS version to create the project file.
rem      As a quick hack I simply replace the toolset version which seems to work.
powershell -Command "(gc jamulus.vcxproj) -replace '<PlatformToolset>v141</PlatformToolset>', '<PlatformToolset>v140</PlatformToolset>' | Out-File -encoding ASCII jamulus.vcxproj"
powershell -Command "(gc jamulus.vcxproj) -replace 'x64', 'Win32' | Out-File -encoding ASCII jamulus.vcxproj"
powershell -Command "(gc jamulus.vcxproj) -replace ';WIN64', '' | Out-File -encoding ASCII jamulus.vcxproj"
powershell -Command "(gc jamulus.vcxproj) -replace '-DWIN64 ', '' | Out-File -encoding ASCII jamulus.vcxproj"

rem clean and compile solution -------------------------------------------------
devenv Jamulus.vcxproj /Clean "Release|x86"
devenv Jamulus.vcxproj /Build "Release|x86"
mkdir release\x86
copy release\Jamulus.exe release\x86\


rem ########################## 64 bit build ####################################
set QTDIR=%QTDIR64%
set Path=%QTDIR64%\bin;%Path%

rem create visual studio project file ------------------------------------------
qmake -tp vc

rem TODO qmake seems to use the incorrect VS version to create the project file.
rem      As a quick hack I simply replace the toolset version which seems to work.
powershell -Command "(gc jamulus.vcxproj) -replace '<PlatformToolset>v141</PlatformToolset>', '<PlatformToolset>v140</PlatformToolset>' | Out-File -encoding ASCII jamulus.vcxproj"

rem clean and compile solution -------------------------------------------------
devenv Jamulus.vcxproj /Clean "Release|x64"
devenv Jamulus.vcxproj /Build "Release|x64"


rem ########################## create installer ################################
cd windows
"%NSIS_PATH%\makensis.exe" installer.nsi
move Jamulusinstaller.exe ../deploy/Jamulus-version-installer.exe
goto endofskript


:vsenvproblem
echo Use the Visual Studio x86 x64 Cross Tools Command Prompt to call this skript
goto endofskript

:qtdirproblem
echo The QTDIR32 and QTDIR64 is not set, please set these environment variables correclty before calling this script
goto endofskript

:endofskript
