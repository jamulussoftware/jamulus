; Jamulus NSIS Installer with Modern User Interface

; Includes
!include "x64.nsh"       ; 64bit architecture support
!include "MUI2.nsh"      ; Modern UI
!include "LogicLib.nsh"  ; Logical operators
!include "Sections.nsh"  ; Support for section selection

; Compile-time definitions
!define VC_REDIST32_EXE   "vc_redist.x86.exe"
!define VC_REDIST64_EXE   "vc_redist.x64.exe"
!define APP_INSTALL_KEY   "Software\${APP_NAME}"
!define APP_INSTALL_VALUE "InstallFolder"
!define AUTORUN_NAME      "${APP_NAME} Server"
!define AUTORUN_KEY       "Software\Microsoft\Windows\CurrentVersion\Run"
!define APP_EXE           "${APP_NAME}.exe"
!define UNINSTALL_EXE     "Uninstall.exe"
!define APP_UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

; General
SetCompressor         bzip2  ; Compression mode
Unicode               true   ; Support all languages via Unicode
RequestExecutionLevel admin  ; Administrator privileges are required for installation

; Installer name and file
Name         "${APP_NAME}"
OutFile      "${DEPLOY_PATH}\${APP_NAME}-${APP_VERSION}-installer-win.exe"
Caption      "${APP_NAME} ${APP_VERSION} Installer"
BrandingText "${APP_NAME} powers your online jam session"

 ; Additional plugin location (for nsProcess)
!addplugindir "${WINDOWS_PATH}"

; Installer graphical element configuration
!define MUI_ICON                       "${WINDOWS_PATH}\mainicon.ico"
!define MUI_UNICON                     "${WINDOWS_PATH}\uninsticon.ico"
!define SERVER_ICON                    "${WINDOWS_PATH}\jamulus-server-icon-2020.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP         "${WINDOWS_PATH}\installer-banner.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP   "${WINDOWS_PATH}\installer-welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${WINDOWS_PATH}\uninstaller-welcome.bmp"

; Store the installer language - must be placed before the installer page configuration
!define MUI_LANGDLL_REGISTRY_ROOT      HKLM
!define MUI_LANGDLL_REGISTRY_KEY       "${APP_INSTALL_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "InstallLanguage"

; Installer page configuration
!define MUI_PAGE_CUSTOMFUNCTION_PRE AbortOnRunningApp
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${ROOT_PATH}\COPYING"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE ValidateDestinationFolder
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!insertmacro MUI_PAGE_FINISH

; Uninstaller page configuration
!define MUI_PAGE_CUSTOMFUNCTION_PRE un.AbortOnRunningApp
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Supported languages configuration - languages other than English are disabled for now
; Additional languages can be added below, see https://nsis.sourceforge.io/Examples/Modern%20UI/MultiLanguage.nsi
!insertmacro MUI_LANGUAGE "English" ; The first language is the default
; !insertmacro MUI_LANGUAGE "Italian"

LangString INVALID_FOLDER_MSG ${LANG_ENGLISH} \
    "The destination folder already exists. Please enter a new destination folder."
; LangString INVALID_FOLDER_MSG ${LANG_ITALIAN} \
;   "La cartella di destinazione esiste già. Selezionare una nuova cartella di destinazione."

LangString RUNNING_APP_MSG ${LANG_ENGLISH} \
    "${APP_NAME} is running. Please close it and run the setup again."
; LangString RUNNING_APP_MSG ${LANG_ITALIAN} \
;   "${APP_NAME} è in esecuzione. Chiudere l'applicazione prima di eseguire l'installazione."

LangString OLD_WRONG_REG_FOUND_EMPTY ${LANG_ENGLISH} \
    "Due to a bug, an old version of Jamulus might be installed to a wrong path on your computer. We couldn't detect the path of the uninstaller, so there might also be another problem. If you ignore this message the installer will continue, but you might end up with multiple Jamulus installations. You could try to search for the Uninstaller of the old version in your Program Files or Program Files (x86) directory."

LangString OLD_WRONG_REG_FOUND ${LANG_ENGLISH} \
    "Due to a bug, an old version of Jamulus might be installed to a wrong path on your computer. If you ignore this message the installer will continue, but you might end up with multiple Jamulus installations. It is recommended to abort and restart the installer. To remove this old Jamulus version, please do so either via the Windows Uninstaller in settings or by launching the following file: "

LangString OLD_WRONG_REG_FOUND_CONFIRM ${LANG_ENGLISH} \
    "If you continue without removing it, your installation might be broken! Are you sure you don't want to remove the old version?"

; Abort the installer/uninstaller if Jamulus is running
!macro _AbortOnRunningApp

    nsProcess::_FindProcess "${APP_EXE}"
    Pop $R0

    ${If} $R0 = 0
        MessageBox MB_OK|MB_ICONEXCLAMATION "$(RUNNING_APP_MSG)" /sd IDOK
        Quit
    ${EndIf}

!macroend


; Installer
!macro InstallApplication buildArch
    !define prefix "${DEPLOY_PATH}\${buildArch}"
    !tempfile files

    ; Find target folders
    !system 'cmd.exe /v /c "for /f "usebackq" %d in (`dir /b /s /ad "${prefix}"`) do \
        @(set "_d=%d" && echo CreateDirectory "$INSTDIR\!_d:${prefix}\=!" >> "${files}")"'

    ; Find target files
    !system 'cmd.exe /v /c "for /r "${prefix}" %f in (*.*) do \
        @(set "_f=%f" && echo File "/oname=$INSTDIR\!_f:${prefix}\=!" "!_f!" >> "${files}")"'

    ; Install folders and files
    CreateDirectory "$INSTDIR"
    !include "${files}"

    ; Add the redistribution license
    File "/oname=$INSTDIR\COPYING" "${ROOT_PATH}\COPYING"
    File "/oname=$INSTDIR\servericon.ico" "${SERVER_ICON}"
    ; Cleanup
    !delfile "${files}"
    !undef files
    !undef prefix

!macroend

!macro SetupShortcuts

    ; Add the registry key to store the installation folder
    WriteRegStr HKLM "${APP_INSTALL_KEY}" "${APP_INSTALL_VALUE}" "$INSTDIR"

    ; Add the registry keys so that software appears in Windows "Add/Remove Software"
    WriteRegStr HKLM "${APP_UNINSTALL_KEY}" "DisplayName"     "${APP_NAME}"
    WriteRegStr HKLM "${APP_UNINSTALL_KEY}" "DisplayIcon"     "$INSTDIR\${APP_EXE},0"
    WriteRegStr HKLM "${APP_UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\${UNINSTALL_EXE}"'

    ; Add the uninstaller
    WriteUninstaller "$INSTDIR\${UNINSTALL_EXE}"

    ; Add the Start Menu and desktop shortcuts
    CreateShortCut  "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortCut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"           "$INSTDIR\${APP_EXE}"
    CreateShortCut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME} Server.lnk"    "$INSTDIR\${APP_EXE}" "-s" "$INSTDIR\servericon.ico"  
    CreateShortCut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME} Uninstall.lnk" "$INSTDIR\${UNINSTALL_EXE}"

!macroend

SectionGroup "InstallGroup"

    Section "Install" Install_x86_64

        ; Install the main application
        !insertmacro InstallApplication x86_64
        !insertmacro SetupShortcuts

        ; Install Microsoft Visual Studio redistributables and remove the installer afterwards
        ExecWait "$\"$INSTDIR\${VC_REDIST64_EXE}$\" /q /norestart"
        Delete   "$INSTDIR\${VC_REDIST64_EXE}"

    SectionEnd

    Section "Install" Install_x86

        ; Install the main application
        !insertmacro InstallApplication x86
        !insertmacro SetupShortcuts

        ; Install Microsoft Visual Studio redistributables and remove the installer afterwards
        ExecWait "$\"$INSTDIR\${VC_REDIST32_EXE}$\" /q /norestart"
        Delete   "$INSTDIR\${VC_REDIST32_EXE}"

    SectionEnd

SectionGroupEnd

Function .onInit

    ; Set up registry access, installation folder and installer section for current architecture
    ${If} ${RunningX64}
        SetRegView      64
        SectionSetFlags ${Install_x86_64} ${SF_SELECTED}
        SectionSetFlags ${Install_x86}    ${SECTION_OFF}

        ; Set default installation folder, retrieve from registry if available
        ReadRegStr $INSTDIR HKLM "${APP_INSTALL_KEY}" "${APP_INSTALL_VALUE}"
        IfErrors   0 +2
        StrCpy     $INSTDIR "$PROGRAMFILES64\${APP_NAME}"
        retrywrong:
            ; check if old, wrongly installed jamulus exists
             ClearErrors
             Var /GLOBAL OLD_WRONG_PATH_UNINST
             StrCpy $OLD_WRONG_PATH_UNINST ""
             ReadRegStr $OLD_WRONG_PATH_UNINST HKLM "SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Jamulus" "UninstallString"
             ${If} ${Errors}
             ${Else}
             ${IF} $UNINST_PATH == ""
                   MessageBox MB_ABORTRETRYIGNORE|MB_ICONEXCLAMATION "$(OLD_WRONG_REG_FOUND_EMPTY)" /sd IDABORT IDIGNORE idontcare IDRETRY retrywrong
                   goto quit
             ${ELSE}
                   MessageBox MB_ABORTRETRYIGNORE|MB_ICONEXCLAMATION "$(OLD_WRONG_REG_FOUND) $OLD_WRONG_PATH_UNINST" /sd IDABORT IDIGNORE idontcare IDRETRY retrywrong
                    goto quit
             ${ENDIF}
             idontcare:
                 MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(OLD_WRONG_REG_FOUND_CONFIRM)" /sd IDNO IDYES continueinstall
                 goto quit
              quit:
                  Abort
              continueinstall:
            ${EndIf}
        ${Else}
        SetRegView      32
        SectionSetFlags ${Install_x86}    ${SF_SELECTED}
        SectionSetFlags ${Install_x86_64} ${SECTION_OFF}

        ; Set default installation folder, retrieve from registry if available
        ReadRegStr $INSTDIR HKLM "${APP_INSTALL_KEY}" "${APP_INSTALL_VALUE}"
        IfErrors   0 +2
        StrCpy     $INSTDIR "$PROGRAMFILES32\${APP_NAME}"

    ${EndIf}
    ; Install for all users
    SetShellVarContext all

    ; Select installer language
    !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

; Ensure Jamulus is installed into a new folder only, unless Jamulus is already installed there
Function ValidateDestinationFolder

    ${If} ${FileExists} "$INSTDIR\*"
    ${AndIfNot} ${FileExists} "$INSTDIR\${APP_EXE}"
        StrCpy $INSTDIR "$INSTDIR\${APP_NAME}"
        MessageBox MB_OK|MB_ICONEXCLAMATION "$(INVALID_FOLDER_MSG)" /sd IDOK
        Abort
    ${endIf}

FunctionEnd

Function AbortOnRunningApp
    !insertmacro _AbortOnRunningApp
FunctionEnd


; Uninstaller
!macro un.InstallFiles buildArch

    !define prefix "${DEPLOY_PATH}\${buildArch}"
    !tempfile files

    ; Find target files
    !system 'cmd.exe /v /c "for /r "${prefix}" %f in (*.*) do \
        @(set "_f=%f" && echo Delete "$INSTDIR\!_f:${prefix}\=!" >> "${files}")"'

    ; Find target folders in reverse order to ensure they can be deleted when empty
    !system 'cmd.exe /v /c "for /f "usebackq" %d in \
        (`dir /b /s /ad "${prefix}" ^| C:\Windows\System32\sort.exe /r`) do \
        @(set "_d=%d" && echo RMDir "$INSTDIR\!_d:${prefix}\=!" >> "${files}")"'

    ; Remove files and folders
    !include "${files}"

    Delete "$INSTDIR\COPYING"
    Delete "$INSTDIR\servericon.ico"
    Delete "$INSTDIR\${UNINSTALL_EXE}"
    RMDir  "$INSTDIR"

    ; Cleanup
    !delfile "${files}"
    !undef files
    !undef prefix

!macroend

Section "un.Install"
    ; Delete the main application
    ${If} ${RunningX64}
        !insertmacro un.InstallFiles x86_64
    ${Else}
        !insertmacro un.InstallFiles x86
    ${EndIf}

    ; Remove the Start Menu and desktop shortcuts
    Delete   "$DESKTOP\${APP_NAME}.lnk"
    RMDir /r "$SMPROGRAMS\${APP_NAME}"

    ; There may be an auto run entry in the registry for the server, remove it
    DeleteRegValue HKCU "${AUTORUN_KEY}" "${AUTORUN_NAME}"

    ; Remove the remaining registry keys
    DeleteRegKey HKLM "${APP_UNINSTALL_KEY}"
    DeleteRegKey HKLM "${APP_INSTALL_KEY}"

SectionEnd

Function un.onInit

    ; Set up registry access for current architecture
    ${If} ${RunningX64}
        SetRegView 64
    ${Else}
        SetRegView 32
    ${EndIf}

    ; Uninstall for all users
    SetShellVarContext all

    ; Retrieve installer language
    !insertmacro MUI_UNGETLANGUAGE

FunctionEnd

Function un.AbortOnRunningApp
    !insertmacro _AbortOnRunningApp
FunctionEnd
