; llcon NSIS installer script

!define APP_NAME       "Llcon"
!define APP_EXE        "llcon.exe"
!define UNINSTALL_EXE  "Uninstall.exe"
!define INSTALLER_NAME "llconinstaller.exe"
;!define VS_REDIST_PATH "C:\Program Files\Microsoft Visual Studio 8\SDK\v2.0\BootStrapper\Packages\vcredist_x86\"
!define VS_REDIST_PATH "C:\Programme\Microsoft Visual Studio 8\SDK\v2.0\BootStrapper\Packages\vcredist_x86\"
!define VS_REDIST_EXE  "vcredist_x86.exe"
!define UNINST_KEY     "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"


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

  ; add reg keys so that software appears in Windows "Add/Remove Software"
  WriteRegStr HKLM "${UNINST_KEY}" "DisplayName" "${APP_NAME} (remove only)"
  WriteRegStr HKLM "${UNINST_KEY}" "UninstallString" '"$INSTDIR\${UNINSTALL_EXE}"'

  SetOutPath       $INSTDIR
  
  ; main application
  File             "Release\${APP_EXE}"

  ; QT dlls
  File             "$%QTDIR%\bin\QtCore4.dll"
  File             "$%QTDIR%\bin\QtGui4.dll"
  File             "$%QTDIR%\bin\QtNetwork4.dll"
  File             "$%QTDIR%\bin\QtXml4.dll"

  ; other files
  File             "..\COPYING"

  ; temporarily create Microsoft Visual Studio redistributable,
  ; call it, and delete it afterwards
  File             "${VS_REDIST_PATH}${VS_REDIST_EXE}"
  ExecWait         '"$INSTDIR\${VS_REDIST_EXE}" /Q'
  Delete           $INSTDIR\${VS_REDIST_EXE}
  
  ; uninstaller
  WriteUninstaller $INSTDIR\${UNINSTALL_EXE}

  ; shortcuts
  CreateShortCut   "$DESKTOP\${APP_NAME}.lnk" "$OUTDIR\${APP_EXE}"

  CreateDirectory  "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${UNINSTALL_EXE}.lnk" "$INSTDIR\${UNINSTALL_EXE}"

SectionEnd


Section "Uninstall"

DeleteRegKey HKLM "${UNINST_KEY}"

Delete "$DESKTOP\${APP_NAME}.lnk"
Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
Delete "$SMPROGRAMS\${APP_NAME}\${UNINSTALL_EXE}.lnk"
RMDIR  "$SMPROGRAMS\${APP_NAME}"

Delete $INSTDIR\${UNINSTALL_EXE}
Delete $INSTDIR\${APP_EXE}
Delete $INSTDIR\QtCore4.dll
Delete $INSTDIR\QtGui4.dll
Delete $INSTDIR\QtNetwork4.dll
Delete $INSTDIR\QtXml4.dll
Delete $INSTDIR\COPYING
RMDir  $INSTDIR

SectionEnd 
