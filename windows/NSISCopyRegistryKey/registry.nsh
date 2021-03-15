; EDIT by ann0see: See the README.txt file in this folder
!ifndef REGISTRY_NSH
!define REGISTRY_NSH
!include "${NSISDIR}\Examples\System\system.nsh"

!define HKEY_CLASSES_ROOT_ENUM           0x80000000
!define HKEY_CURRENT_USER_ENUM           0x80000001
!define HKEY_LOCAL_MACHINE_ENUM          0x80000002
!define HKEY_USERS_ENUM                  0x80000003
!define HKEY_PERFORMANCE_DATA_ENUM       0x80000004
!define HKEY_CURRENT_CONFIG_ENUM         0x80000005
!define HKEY_DYN_DATA_ENUM               0x80000006

!define KEY_ALL_ACCESS          0x0002003F

!define RegCreateKeyEx 'advapi32::RegCreateKeyEx(i, t, i, t, i, i, i, *i, *i) i'
!define RegOpenKeyEx 'advapi32::RegOpenKeyEx(i, t, i, i, *i) i'
!define RegCloseKey  'advapi32::RegCloseKey(i) i'
!define SHCopyKey 'shlwapi::SHCopyKey(i, t, i, i) i'

####################################################################################################
!macro SET_HKEY_ENUM HKEY_NAME HKEY_ENUM LABEL
    StrCmp ${HKEY_NAME} HKCR 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_CLASSES_ROOT_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKEY_CLASSES_ROOT 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_CLASSES_ROOT_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKCU 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_CURRENT_USER_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKEY_CURRENT_USER 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_CURRENT_USER_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKLM 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_LOCAL_MACHINE_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKEY_LOCAL_MACHINE 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_LOCAL_MACHINE_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKU 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_USERS_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKEY_USERS 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_USERS_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKPD 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_PERFORMANCE_DATA_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKEY_PERFORMANCE_DATA 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_PERFORMANCE_DATA_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKCC 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_CURRENT_CONFIG_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKEY_CURRENT_CONFIG 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_CURRENT_CONFIG_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKDD 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_DYN_DATA_ENUM}
    GoTo ${LABEL}
    StrCmp ${HKEY_NAME} HKEY_DYN_DATA 0 +3
    StrCpy ${HKEY_ENUM} ${HKEY_DYN_DATA_ENUM}
${LABEL}:
!macroend
####################################################################################################
!macro COPY_REGISTRY_KEY_BASE TYPE
!ifndef ${TYPE}COPY_REGISTRY_KEY_DEFINE
!define ${TYPE}COPY_REGISTRY_KEY_DEFINE
Function ${TYPE}CopyRegistryKey
    Exch $1 ;Target subkey
    Exch
    Exch $2 ;Target key name
    Exch
    Exch 2
    Exch $3 ;Source subkey
    Exch
    Exch 2
    Exch 3
    Exch $4 ;Source key name
    Push $5 ;Source key handle
    Push $6 ;Target key handle
    Push $7 ;return value
    Push $8 ;HKEY enum for target key
    Push $9 ;HKEY enum for source key

!insertmacro SET_HKEY_ENUM $2 $8 next
!insertmacro SET_HKEY_ENUM $4 $9 next2

     SetPluginUnload alwaysoff

     StrCpy $5 0
     System::Call '${RegOpenKeyEx}(i r9, t r3, 0, ${KEY_ALL_ACCESS}, .r5) .r7'
     StrCmp $7 0 continue
     DetailPrint "Registry key $4\$3 not found."
     StrCpy $1 $7
     GoTo done

continue:
     StrCpy $6 0
     System::Call '${RegCreateKeyEx}(i r8, t r1, 0, 0, 0, ${KEY_ALL_ACCESS}, 0, .r6, 0) .r7'
     StrCmp $7 0 copy
     MessageBox MB_OK|MB_ICONSTOP "Error $7 opening registry key $2\$1."
     StrCpy $1 $7
     GoTo done

copy:
     System::Call '${SHCopyKey}(i r5, "", i r6, 0) .r7'
     StrCmp $7 0 +2
     MessageBox MB_OK|MB_ICONSTOP "Error $7 copying registry key."
     StrCpy $1 $7

done:
     System::Call '${RegCloseKey}(i r5) .r7'
     System::Call '${RegCloseKey}(i r6) .r7'
     SetPluginUnload manual
     System::Free 0

     Pop $9
     Pop $8
     Pop $7
     Pop $6
     Pop $5
     Pop $4
     Pop $3
     Pop $2
     Exch $1
FunctionEnd
!endif
!macroend
####################################################################################################
!macro COPY_REGISTRY_KEY
!insertmacro COPY_REGISTRY_KEY_BASE ""
!macroend
####################################################################################################
!macro UN.COPY_REGISTRY_KEY
!insertmacro COPY_REGISTRY_KEY_BASE "Un."
!macroend
####################################################################################################
!macro CALL_COPY_REGISTRY_KEY_BASE TYPE SOURCEROOTKEY SOURCESUBKEY TARGETROOTKEY TARGETSUBKEY
!ifdef ${TYPE}COPY_REGISTRY_KEY_DEFINE
    Push $R1
    ReadRegStr $R1 "${TARGETROOTKEY}" "${TARGETSUBKEY}" ""
    IfErrors 0 +2
    WriteRegStr "${TARGETROOTKEY}" "${TARGETSUBKEY}" "" ""
    Pop $R1
    Push "${SOURCEROOTKEY}"
    Push "${SOURCESUBKEY}"
    Push "${TARGETROOTKEY}"
    Push "${TARGETSUBKEY}"
    Call ${TYPE}CopyRegistryKey
!else
!error "Macro ${TYPE}COPY_REGISTRY_KEY not inserted"
!endif
!macroend
####################################################################################################
!macro CALL_COPY_REGISTRY_KEY SOURCEROOTKEY SOURCESUBKEY TARGETROOTKEY TARGETSUBKEY
!insertmacro CALL_COPY_REGISTRY_KEY_BASE "" "${SOURCEROOTKEY}" "${SOURCESUBKEY}" "${TARGETROOTKEY}" "${TARGETSUBKEY}"
!macroend
####################################################################################################
!macro CALL_UN.COPY_REGISTRY_KEY SOURCEROOTKEY SOURCESUBKEY TARGETROOTKEY TARGETSUBKEY
!insertmacro CALL_COPY_REGISTRY_KEY_BASE "Un." "${SOURCEROOTKEY}" "${SOURCESUBKEY}" "${TARGETROOTKEY}" "${TARGETSUBKEY}"
!macroend
####################################################################################################
!define COPY_REGISTRY_KEY "!insertmacro CALL_COPY_REGISTRY_KEY"
!define UN.COPY_REGISTRY_KEY "!insertmacro UN.CALL_COPY_REGISTRY_KEY"
!endif
