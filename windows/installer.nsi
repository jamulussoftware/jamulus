; llcon NSIS installer script

!define APP_NAME          "Llcon"
!define APP_EXE			  "llcon.exe"
!define AUTORUN_NAME      "llcon server"
!define UNINSTALL_EXE     "Uninstall.exe"
!define INSTALLER_NAME    "llconinstaller.exe"
!define BINARY_PATH       "..\release\"
!define VS_REDIST_PATH    "C:\Program Files\Microsoft Visual Studio 10.0\SDK\v3.5\BootStrapper\Packages\vcredist_x86\"
;!define VS_REDIST_PATH   "C:\Programme\Microsoft Visual Studio 10.0\SDK\v3.5\BootStrapper\Packages\vcredist_x86\"
!define VS_REDIST_EXE     "vcredist_x86.exe"
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
  File             "${BINARY_PATH}${APP_EXE}"

  ; QT dlls
  File             "$%QTDIR%\bin\Qt5Core.dll"
  File             "$%QTDIR%\bin\Qt5Gui.dll"
  File             "$%QTDIR%\bin\Qt5Widgets.dll"
  File             "$%QTDIR%\bin\Qt5Network.dll"
  File             "$%QTDIR%\bin\Qt5Xml.dll"
  File             "$%QTDIR%\bin\D3DCompiler_43.dll"
  File             "$%QTDIR%\bin\icudt49.dll"
  File             "$%QTDIR%\bin\icuin49.dll"
  File             "$%QTDIR%\bin\icuuc49.dll"
  File             "$%QTDIR%\bin\libEGL.dll"
  File             "$%QTDIR%\bin\libGLESv2.dll"

  ; other files
  File             "..\COPYING"

  ; temporarily create Microsoft Visual Studio redistributable,
  File             "${VS_REDIST_PATH}${VS_REDIST_EXE}"
  ExecWait         '"$INSTDIR\${VS_REDIST_EXE}" /Q'

  ; uninstaller
  WriteUninstaller $INSTDIR\${UNINSTALL_EXE}

  ; shortcuts
  CreateShortCut   "$DESKTOP\${APP_NAME}.lnk" "$OUTDIR\${APP_EXE}"

  CreateDirectory  "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${APP_NAME} server.lnk" "$INSTDIR\${APP_EXE}" "-s"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${UNINSTALL_EXE}.lnk" "$INSTDIR\${UNINSTALL_EXE}"

  ; cleanup: remove temporary Microsoft Visual Studio redistributable executable
  Delete           $INSTDIR\${VS_REDIST_EXE}

  ; additional platform dlls
  SetOutPath       $INSTDIR\platforms
  File             "$%QTDIR%\plugins\platforms\qwindows.dll"
  File             "$%QTDIR%\plugins\platforms\qminimal.dll"  

  ; accessible qt plugin
  SetOutPath       $INSTDIR\accessible
  File             "$%QTDIR%\plugins\accessible\qtaccessiblewidgets.dll"

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
Delete $INSTDIR\D3DCompiler_43.dll
Delete $INSTDIR\icudt49.dll
Delete $INSTDIR\icuin49.dll
Delete $INSTDIR\icuuc49.dll
Delete $INSTDIR\libEGL.dll
Delete $INSTDIR\libGLESv2.dll
Delete $INSTDIR\COPYING
Delete $INSTDIR\platforms\qwindows.dll
Delete $INSTDIR\platforms\qminimal.dll
Delete $INSTDIR\accessible\qtaccessiblewidgets.dll
RMDir  $INSTDIR\accessible
RMDir  $INSTDIR

SectionEnd
