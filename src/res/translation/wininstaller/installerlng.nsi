; Language configuration

; Additional languages can be added in this file. See https://nsis.sourceforge.io/Examples/Modern%20UI/MultiLanguage.nsi

!insertmacro MUI_LANGUAGE "English" ; The first language is the default
!include "${ROOT_PATH}\src\res\translation\wininstaller\en.nsi" ; include english

!insertmacro MUI_LANGUAGE "German"
!include "${ROOT_PATH}\src\res\translation\wininstaller\de.nsi"

; !insertmacro MUI_LANGUAGE "Italian"
; !include "${ROOT_PATH}\src\res\translation\wininstaller\it.nsi" ; include italian

!insertmacro MUI_LANGUAGE "Dutch"
!include "${ROOT_PATH}\src\res\translation\wininstaller\nl.nsi"
