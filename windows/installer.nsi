;/******************************************************************************\
; * Copyright (c) 2004-2020
; *
; * Author(s):
; *  Volker Fischer
; *
; ******************************************************************************
; *
; * This program is free software; you can redistribute it and/or modify it under
; * the terms of the GNU General Public License as published by the Free Software
; * Foundation; either version 2 of the License, or (at your option) any later
; * version.
; *
; * This program is distributed in the hope that it will be useful, but WITHOUT
; * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
; * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
; * details.
; *
; * You should have received a copy of the GNU General Public License along with
; * this program; if not, write to the Free Software Foundation, Inc.,
; * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
; *
;\******************************************************************************/

; Jamulus NSIS installer script
!include LogicLib.nsh
!include x64.nsh

!define APP_NAME          "Jamulus"
!define APP_EXE           "Jamulus.exe"
!define AUTORUN_NAME      "Jamulus server"
!define UNINSTALL_EXE     "Uninstall.exe"
!define INSTALLER_NAME    "Jamulusinstaller.exe"
!define BINARY_PATH       "..\release\"
!define VS_REDIST_EXE     "vc_redist.x86.exe"
!define UNINST_KEY        "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"
!define AUTORUN_KEY       "Software\Microsoft\Windows\CurrentVersion\Run"


SetCompressor lzma
Name          "${APP_NAME}"
Caption       "${APP_NAME}"
OutFile       "${INSTALLER_NAME}"
InstallDir    "$PROGRAMFILES\${APP_NAME}"

LicenseText   "License"
LicenseData   "..\COPYING"

Page license
Page directory
Page instfiles


Section

  ; check if software is currently running
  !addplugindir ..\windows
  nsProcess::_FindProcess "${APP_EXE}"
  Pop $R0
  ${If} $R0 = 0
      MessageBox MB_OK|MB_ICONEXCLAMATION "${APP_NAME} is running. Please close it and run the setup again." /sd IDOK
      Quit
  ${EndIf}

  ; add reg keys so that software appears in Windows "Add/Remove Software"
  WriteRegStr HKLM "${UNINST_KEY}" "DisplayName" "${APP_NAME} (remove only)"
  WriteRegStr HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\${UNINSTALL_EXE}"'

  SetOutPath       $INSTDIR

  ; main application
  ${If} ${RunningX64}
  File             "${BINARY_PATH}${APP_EXE}"
  ${Else}
  File             "${BINARY_PATH}x86\${APP_EXE}"
  ${EndIf}

  ; QT dlls
  ${If} ${RunningX64}
  File             "$%QTDIR64%\bin\Qt5Core.dll"
  File             "$%QTDIR64%\bin\Qt5Gui.dll"
  File             "$%QTDIR64%\bin\Qt5Widgets.dll"
  File             "$%QTDIR64%\bin\Qt5Network.dll"
  File             "$%QTDIR64%\bin\Qt5Svg.dll"
  File             "$%QTDIR64%\bin\Qt5Concurrent.dll"
  File             "$%QTDIR64%\bin\Qt5Xml.dll"
  ${Else}
  File             "$%QTDIR32%\bin\Qt5Core.dll"
  File             "$%QTDIR32%\bin\Qt5Gui.dll"
  File             "$%QTDIR32%\bin\Qt5Widgets.dll"
  File             "$%QTDIR32%\bin\Qt5Network.dll"
  File             "$%QTDIR32%\bin\Qt5Svg.dll"
  File             "$%QTDIR32%\bin\Qt5Concurrent.dll"
  File             "$%QTDIR32%\bin\Qt5Xml.dll"
  ${EndIf}

  ; other files
  File             "..\COPYING"

  ; temporarily create Microsoft Visual Studio redistributable
  ${If} ${RunningX64}
  File             "$%VS_REDIST64_EXE%"
  ExecWait         '"$INSTDIR\$%VS_REDIST64_EXE%" /q /norestart'
  ${Else}
  File             "$%VS_REDIST32_EXE%"
  ExecWait         '"$INSTDIR\$%VS_REDIST32_EXE%" /q /norestart'
  ${EndIf}

  ; uninstaller
  WriteUninstaller $INSTDIR\${UNINSTALL_EXE}

  ; shortcuts
  CreateShortCut   "$DESKTOP\${APP_NAME}.lnk" "$OUTDIR\${APP_EXE}"

  CreateDirectory  "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${APP_NAME} Server.lnk" "$INSTDIR\${APP_EXE}" "-s"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${UNINSTALL_EXE}.lnk" "$INSTDIR\${UNINSTALL_EXE}"

  ; cleanup: remove temporary Microsoft Visual Studio redistributable executable
  ${If} ${RunningX64}
  Delete           $INSTDIR\$%VS_REDIST64_EXE%
  ${Else}
  Delete           $INSTDIR\$%VS_REDIST32_EXE%
  ${EndIf}

  ; additional platform dlls
  SetOutPath       $INSTDIR\platforms
  ${If} ${RunningX64}
  File             "$%QTDIR64%\plugins\platforms\qwindows.dll"
  ${Else}
  File             "$%QTDIR32%\plugins\platforms\qwindows.dll"
  ${EndIf}

  ; additional styles dlls
  SetOutPath       $INSTDIR\styles
  ${If} ${RunningX64}
  File             "$%QTDIR64%\plugins\styles\qwindowsvistastyle.dll"
  ${Else}
  File             "$%QTDIR32%\plugins\styles\qwindowsvistastyle.dll"
  ${EndIf}

  ; additional imageformats dlls
  SetOutPath       $INSTDIR\imageformats
  ${If} ${RunningX64}
  File             "$%QTDIR64%\plugins\imageformats\qgif.dll"
  File             "$%QTDIR64%\plugins\imageformats\qicns.dll"
  File             "$%QTDIR64%\plugins\imageformats\qico.dll"
  File             "$%QTDIR64%\plugins\imageformats\qjpeg.dll"
  File             "$%QTDIR64%\plugins\imageformats\qsvg.dll"
  File             "$%QTDIR64%\plugins\imageformats\qtga.dll"
  File             "$%QTDIR64%\plugins\imageformats\qtiff.dll"
  File             "$%QTDIR64%\plugins\imageformats\qwbmp.dll"
  File             "$%QTDIR64%\plugins\imageformats\qwebp.dll"
  ${Else}
  File             "$%QTDIR32%\plugins\imageformats\qgif.dll"
  File             "$%QTDIR32%\plugins\imageformats\qicns.dll"
  File             "$%QTDIR32%\plugins\imageformats\qico.dll"
  File             "$%QTDIR32%\plugins\imageformats\qjpeg.dll"
  File             "$%QTDIR32%\plugins\imageformats\qsvg.dll"
  File             "$%QTDIR32%\plugins\imageformats\qtga.dll"
  File             "$%QTDIR32%\plugins\imageformats\qtiff.dll"
  File             "$%QTDIR32%\plugins\imageformats\qwbmp.dll"
  File             "$%QTDIR32%\plugins\imageformats\qwebp.dll"
  ${EndIf}

SectionEnd


Section "Uninstall"

DeleteRegKey HKLM "${UNINST_KEY}"

; the software may have written an auto run entry in the registry, remove it
DeleteRegValue HKCU "${AUTORUN_KEY}" "${AUTORUN_NAME}"

Delete "$DESKTOP\${APP_NAME}.lnk"
Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME} server.lnk"
Delete "$SMPROGRAMS\${APP_NAME}\${UNINSTALL_EXE}.lnk"
RMDIR  "$SMPROGRAMS\${APP_NAME}"

Delete $INSTDIR\${UNINSTALL_EXE}
Delete $INSTDIR\${APP_EXE}
Delete $INSTDIR\Qt5Core.dll
Delete $INSTDIR\Qt5Gui.dll
Delete $INSTDIR\Qt5Widgets.dll
Delete $INSTDIR\Qt5Network.dll
Delete $INSTDIR\Qt5Svg.dll
Delete $INSTDIR\Qt5Concurrent.dll
Delete $INSTDIR\Qt5Xml.dll
Delete $INSTDIR\COPYING
Delete $INSTDIR\platforms\qwindows.dll
RMDir  $INSTDIR\platforms
Delete $INSTDIR\styles\qwindowsvistastyle.dll
RMDir  $INSTDIR\styles
Delete $INSTDIR\imageformats\qgif.dll
Delete $INSTDIR\imageformats\qicns.dll
Delete $INSTDIR\imageformats\qico.dll
Delete $INSTDIR\imageformats\qjpeg.dll
Delete $INSTDIR\imageformats\qsvg.dll
Delete $INSTDIR\imageformats\qtga.dll
Delete $INSTDIR\imageformats\qtiff.dll
Delete $INSTDIR\imageformats\qwbmp.dll
Delete $INSTDIR\imageformats\qwebp.dll
RMDir  $INSTDIR\imageformats
RMDir  $INSTDIR

SectionEnd
