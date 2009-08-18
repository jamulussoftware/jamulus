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
  File             "Release\${APP_EXE}"
  WriteUninstaller $INSTDIR\${UNINSTALL_EXE}

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
RMDir  $INSTDIR

SectionEnd 
