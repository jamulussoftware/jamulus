; llcon NSIS installer script

!define APP_NAME       "Llcon"
!define APP_EXE        "llcon.exe"
!define UNINSTALL_EXE  "Uninstall.exe"
!define INSTALLER_NAME "llconinstaller.exe"


SetCompressor lzma
Name          "${APP_NAME}"
Caption       "${APP_NAME}"
OutFile       "${INSTALLER_NAME}"
InstallDir    "$PROGRAMFILES\${APP_NAME}"

Page directory
Page instfiles


Section

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

  ; uninstaller
  WriteUninstaller $INSTDIR\${UNINSTALL_EXE}

  ; shortcuts
  CreateShortCut   "$DESKTOP\${APP_NAME}.lnk" "$OUTDIR\${APP_EXE}"

  CreateDirectory  "$SMPROGRAMS\${APP_NAME}"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
  CreateShortCut   "$SMPROGRAMS\${APP_NAME}\${UNINSTALL_EXE}.lnk" "$INSTDIR\${UNINSTALL_EXE}"

SectionEnd


Section "Uninstall"

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
