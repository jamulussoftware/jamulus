; Jamulus NSIS Installer with Modern User Interface

; Includes
!include "x64.nsh"       ; 64bit architecture support
!include "MUI2.nsh"      ; Modern UI
!include "LogicLib.nsh"  ; Logical operators
!include "Sections.nsh"  ; Support for section selection
!include "nsDialogs.nsh" ; Support custom pages with dialogs

; Compile-time definitions
!define VC_REDIST32_EXE   "vc_redist.x86.exe"
!define VC_REDIST64_EXE   "vc_redist.x64.exe"
!define APP_INSTALL_KEY   "Software\${APP_NAME}"
!define APP_INSTALL_VALUE "InstallFolder"
!define APP_INSTALL_ICON  "InstallDtIcon"
!define AUTORUN_NAME      "${APP_NAME} Server"
!define AUTORUN_KEY       "Software\Microsoft\Windows\CurrentVersion\Run"
!define APP_EXE           "${APP_NAME}.exe"
!define UNINSTALL_EXE     "Uninstall.exe"
!define APP_UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP_NAME}"

!define SF_USELECTED  0

; General
SetCompressor         bzip2  ; Compression mode
Unicode               true   ; Support all languages via Unicode
RequestExecutionLevel admin  ; Administrator privileges are required for installation

; Installer name and file
Name         "${APP_NAME}"
OutFile      "${DEPLOY_PATH}\${APP_NAME}-${APP_VERSION}-installer-win.exe"
Caption      "${APP_NAME} ${APP_VERSION} Installer"
BrandingText "${APP_NAME} Make music online. With friends. For free."

 ; Additional plugin location (for nsProcess)
!addplugindir "${WINDOWS_PATH}"

; Installer graphical element configuration
!define MUI_ICON                       "${WINDOWS_PATH}\mainicon.ico"
!define MUI_UNICON                     "${WINDOWS_PATH}\mainicon.ico"
!define SERVER_ICON                    "${WINDOWS_PATH}\jamulus-server-icon-2020.ico"
!define MUI_HEADERIMAGE
!define MUI_HEADERIMAGE_BITMAP         "${WINDOWS_PATH}\installer-banner.bmp"
!define MUI_WELCOMEFINISHPAGE_BITMAP   "${WINDOWS_PATH}\installer-welcome.bmp"
!define MUI_UNWELCOMEFINISHPAGE_BITMAP "${WINDOWS_PATH}\installer-welcome.bmp"

; Store the installer language - must be placed before the installer page configuration
!define MUI_LANGDLL_REGISTRY_ROOT      HKLM
!define MUI_LANGDLL_REGISTRY_KEY       "${APP_INSTALL_KEY}"
!define MUI_LANGDLL_REGISTRY_VALUENAME "InstallLanguage"

; Installer page configuration
!define MUI_PAGE_CUSTOMFUNCTION_PRE AbortOnRunningApp
!insertmacro MUI_PAGE_WELCOME

Page Custom ASIOCheckInstalled ExitASIOInstalled

!insertmacro MUI_PAGE_LICENSE "${ROOT_PATH}\COPYING"
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE ValidateDestinationFolder
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_PAGE_CUSTOMFUNCTION_SHOW FinishPage.Show
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_SHOWREADME ""
!define MUI_FINISHPAGE_SHOWREADME_CHECKED
!define MUI_FINISHPAGE_SHOWREADME_TEXT "$(DESKTOP_SET_SHORTCUT)"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION createdesktopshortcut
!insertmacro MUI_PAGE_FINISH

; Uninstaller page configuration
!define MUI_PAGE_CUSTOMFUNCTION_PRE un.AbortOnRunningApp
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

; Supported languages configuration
; Additional languages can be added in the file installerlng.nsi in the wininstaller folder, see https://nsis.sourceforge.io/Examples/Modern%20UI/MultiLanguage.nsi

!include "${ROOT_PATH}\src\res\translation\wininstaller\installerlng.nsi"

; Abort the installer/uninstaller if Jamulus is running

!macro _AbortOnRunningApp

    nsProcess::_FindProcess "${APP_EXE}"
    Pop $R0

    ${If} $R0 = 0
        MessageBox MB_OK|MB_ICONEXCLAMATION "$(RUNNING_APP_MSG)" /sd IDOK
        Quit
    ${EndIf}

!macroend

; Define Dialog variables

Var Dialog
Var Label
Var Button

; Define user choices

Var bInstallDtIcon

; Installer

!macro InstallApplication buildArch
    !define prefix "${DEPLOY_PATH}\${buildArch}"
    !tempfile files

    ; Find target folders (Probably here's an issue with quoting. If ${prefix} contains spaces, the installer folders aren't created in the right way.)
    !system 'cmd.exe /v /c "for /f "usebackq" %d in (`dir /b /s /ad "${prefix}"`) do \
        @(set "_d=%d" && echo CreateDirectory "$INSTDIR\!_d:${prefix}\=!" >> "${files}")"'

    ; Find target files
    !system 'cmd.exe /v /c "for /r "${prefix}" %f in (*.*) do \
        @(set "_f=%f" && echo File "/oname=$INSTDIR\!_f:${prefix}\=!" "!_f!" >> "${files}")"'

    ; to allow jumping in macros, NSIS recommends to define unique IDs for labels https://nsis.sourceforge.io/Tutorial:_Using_labels_in_macro%27s
    !define UniqueID ${__LINE__}

    InitPluginsDir ; see https://stackoverflow.com/questions/24595887/waiting-for-nsis-uninstaller-to-finish-in-nsis-installer-either-fails-or-the-uni
    IfFileExists "$INSTDIR\${UNINSTALL_EXE}" 0 continue_${UniqueID}

        ; Make sure plugins do not conflict with a old uninstaller
        CreateDirectory "$pluginsdir\unold"
        CopyFiles /SILENT /FILESONLY "$INSTDIR\${UNINSTALL_EXE}" "$pluginsdir\unold"
        ExecWait '"$pluginsdir\unold\${UNINSTALL_EXE}" /S _?=$INSTDIR' $0

        ${IfNot} $0 == 0
            MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "$(OLD_VER_REMOVE_FAILED)" /sd IDCANCEL IDOK continue_${UniqueID}
            Abort
        ${EndIf}

    continue_${UniqueID}:
    !undef UniqueID

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

    ; Add the Start Menu shortcuts
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortCut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"           "$INSTDIR\${APP_EXE}"
    CreateShortCut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME} Server.lnk"    "$INSTDIR\${APP_EXE}" "-s" "$INSTDIR\servericon.ico"
    CreateShortCut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME} Uninstall.lnk" "$INSTDIR\${UNINSTALL_EXE}"

!macroend

!macro SecSelect SecId ; See https://nsis.sourceforge.io/Managing_Sections_on_Runtime
    Push $0
    IntOp $0 ${SF_SELECTED} | ${SF_RO}
    SectionSetFlags ${SecId} $0
    SectionSetInstTypes ${SecId} 1
    Pop $0
!macroend

!define SelectSection '!insertmacro SecSelect'

!macro SecUnSelect SecId
  Push $0
  IntOp $0 ${SF_USELECTED} | ${SF_RO}
  SectionSetFlags ${SecId} $0
  SectionSetText  ${SecId} ""
  Pop $0
!macroend

!define UnSelectSection '!insertmacro SecUnSelect'

Section "Install_64Bit" INST_64
    ; check if old, wrongly installed Jamulus exists. See https://stackoverflow.com/questions/27839860/nsis-check-if-registry-key-value-exists#27841158
    IfFileExists "$PROGRAMFILES32\Jamulus\Uninstall.exe" 0 continueinstall

        MessageBox MB_YESNOCANCEL|MB_ICONEXCLAMATION "$(OLD_WRONG_VER_FOUND)" /sd IDYES IDNO idontcare IDCANCEL quit
            goto removeold

        idontcare: ; Clicked no
            MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(OLD_WRONG_VER_FOUND_CONFIRM)" /sd IDNO IDYES continueinstall
            goto removeold

        removeold: ; Remove it
            ExecWait '"$PROGRAMFILES32\Jamulus\Uninstall.exe" /S' $0
            ${IfNot} $0 == 0
                MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION "$(OLD_VER_REMOVE_FAILED)" /sd IDCANCEL IDOK continueinstall
                goto quit
            ${EndIf}
            goto continueinstall

        quit:
            Abort

    continueinstall:

    ; Install the main application
    !insertmacro InstallApplication x86_64
    !insertmacro SetupShortcuts

    ; Install Microsoft Visual Studio redistributables and remove the installer afterwards
    ExecWait "$\"$INSTDIR\${VC_REDIST64_EXE}$\" /q /norestart"
    Delete   "$INSTDIR\${VC_REDIST64_EXE}"
SectionEnd

Section "Install_32Bit" INST_32

    ; Install the main application
    !insertmacro InstallApplication x86
    !insertmacro SetupShortcuts

    ; Install Microsoft Visual Studio redistributables and remove the installer afterwards
    ExecWait "$\"$INSTDIR\${VC_REDIST32_EXE}$\" /q /norestart"
    Delete   "$INSTDIR\${VC_REDIST32_EXE}"

SectionEnd

Function .onInit

    ; Set up registry access, installation folder and installer section for current architecture
    ${If} ${RunningX64}
        SetRegView      64

        ; Set default installation folder, retrieve from registry if available
        ReadRegStr $INSTDIR HKLM "${APP_INSTALL_KEY}" "${APP_INSTALL_VALUE}"
        IfErrors   0 +2
        StrCpy     $INSTDIR "$PROGRAMFILES64\${APP_NAME}"

        ; enable the 64 bit install section
        ${SelectSection} ${INST_64}
        ${UnSelectSection} ${INST_32}

    ${Else}
        SetRegView      32

        ; Set default installation folder, retrieve from registry if available
        ReadRegStr $INSTDIR HKLM "${APP_INSTALL_KEY}" "${APP_INSTALL_VALUE}"
        IfErrors   0 +2
        StrCpy     $INSTDIR "$PROGRAMFILES32\${APP_NAME}"

        ; enable the 32 bit install section
        ${SelectSection} ${INST_32}
        ${UnSelectSection} ${INST_64}
    ${EndIf}

    ; Install for all users
    SetShellVarContext all

    ; get user choices (open program, dt icon,...)
    ReadRegStr $bInstallDtIcon  HKLM "${APP_INSTALL_KEY}" "${APP_INSTALL_ICON}"
    IfErrors   0 +2
    StrCpy $bInstallDtIcon "1"

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

Function FinishPage.Show ; set the user choices if they were remembered

    WriteRegStr HKLM "${APP_INSTALL_KEY}" "${APP_INSTALL_ICON}" "0" ; this will be overwritten if the box is checked
    ${If} $bInstallDtIcon == 1 ; Check the install desktop icon checkbox
        SendMessage $mui.FinishPage.Showreadme ${BM_SETCHECK} ${BST_CHECKED} 0
    ${Else}
        SendMessage $mui.FinishPage.Showreadme ${BM_SETCHECK} ${BST_UNCHECKED} 0
    ${EndIf}

    ShowWindow $mui.FinishPage.Showreadme 1

FunctionEnd

Function AbortOnRunningApp
    !insertmacro _AbortOnRunningApp
FunctionEnd

Function createdesktopshortcut
   WriteRegStr HKLM "${APP_INSTALL_KEY}" "${APP_INSTALL_ICON}" "1" ; remember that icon should be installed next time
   CreateShortCut  "$DESKTOP\${APP_NAME}.lnk" "$INSTDIR\${APP_EXE}"
FunctionEnd

Function ASIOCheckInstalled

    ; insert ASIO install page if no ASIO driver was found
    ClearErrors
    EnumRegKey $0 HKLM "SOFTWARE\ASIO" 0

    IfErrors 0 ASIOExists
        !insertmacro MUI_HEADER_TEXT "$(ASIO_DRIVER_HEADER)" "$(ASIO_DRIVER_SUB)"
        nsDialogs::Create 1018
        Pop $Dialog
        ${If} $Dialog == error
            Abort
        ${Endif}

        ${NSD_CreateLabel} 0 0 100% 12u "$(ASIO_DRIVER_EXPLAIN)"
        Pop $Label
        ${NSD_CreateButton} 0 13u 100% 13u "$(ASIO_DRIVER_MORE_INFO)"
        Pop $Button
        ${NSD_OnClick} $Button OpenASIOHelpPage

        nsDialogs::Show

    ASIOExists:

FunctionEnd

Function OpenASIOHelpPage
    ExecShell "open" "$(ASIO_DRIVER_MORE_INFO_URL)"
FunctionEnd

Function ExitASIOInstalled
    ClearErrors
    EnumRegKey $0 HKLM "SOFTWARE\ASIO" 0
    IfErrors 0 SkipMessage
        MessageBox MB_YESNO|MB_ICONEXCLAMATION "$(ASIO_EXIT_NO_DRIVER)" /sd IDNO IDYES SkipMessage
            Abort
   SkipMessage:

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
    IfFileExists "$DESKTOP\${APP_NAME}.lnk" 0 skipshortcut
        Delete   "$DESKTOP\${APP_NAME}.lnk"
    skipshortcut:

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
