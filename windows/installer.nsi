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
  FindProcDLL::FindProc "${APP_EXE}"
  IntCmp $R0 1 0 notRunning
    MessageBox MB_OK|MB_ICONEXCLAMATION "${APP_NAME} is running. Please close it and run the setup again." /SD IDOK
    Abort
  notRunning:

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
  File             "$%QTDIR64%\bin\Qt5Xml.dll"
  File             "$%QTDIR64%\bin\D3DCompiler_47.dll"
  File             "$%QTDIR64%\bin\libEGL.dll"
  File             "$%QTDIR64%\bin\libGLESv2.dll"
  ${Else}
  File             "$%QTDIR32%\bin\Qt5Core.dll"
  File             "$%QTDIR32%\bin\Qt5Gui.dll"
  File             "$%QTDIR32%\bin\Qt5Widgets.dll"
  File             "$%QTDIR32%\bin\Qt5Network.dll"
  File             "$%QTDIR32%\bin\Qt5Xml.dll"
  File             "$%QTDIR32%\bin\D3DCompiler_47.dll"
  File             "$%QTDIR32%\bin\libEGL.dll"
  File             "$%QTDIR32%\bin\libGLESv2.dll"
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
  File             "$%QTDIR64%\plugins\platforms\qminimal.dll"  
  ${Else}
  File             "$%QTDIR32%\plugins\platforms\qwindows.dll"
  File             "$%QTDIR32%\plugins\platforms\qminimal.dll"  
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
Delete $INSTDIR\Qt5Xml.dll
Delete $INSTDIR\D3DCompiler_47.dll
Delete $INSTDIR\libEGL.dll
Delete $INSTDIR\libGLESv2.dll
Delete $INSTDIR\COPYING
Delete $INSTDIR\platforms\qwindows.dll
Delete $INSTDIR\platforms\qminimal.dll
RMDir  $INSTDIR\platforms
RMDir  $INSTDIR

SectionEnd
