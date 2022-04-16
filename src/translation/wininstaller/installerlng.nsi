; Language configuration

; Additional languages can be added in this file. See https://nsis.sourceforge.io/Examples/Modern%20UI/MultiLanguage.nsi

!insertmacro MUI_LANGUAGE "English" ; The first language is the default
!include "${ROOT_PATH}\src\translation\wininstaller\en.nsi" ; include english

!insertmacro MUI_LANGUAGE "German"
!include "${ROOT_PATH}\src\translation\wininstaller\de.nsi"

!insertmacro MUI_LANGUAGE "Italian"
!include "${ROOT_PATH}\src\translation\wininstaller\it.nsi" ; include italian

!insertmacro MUI_LANGUAGE "Dutch"
!include "${ROOT_PATH}\src\translation\wininstaller\nl.nsi"

!insertmacro MUI_LANGUAGE "Polish"
!include "${ROOT_PATH}\src\translation\wininstaller\pl.nsi"

!insertmacro MUI_LANGUAGE "French"
!include "${ROOT_PATH}\src\translation\wininstaller\fr.nsi"

!insertmacro MUI_LANGUAGE "Spanish"
!include "${ROOT_PATH}\src\translation\wininstaller\es.nsi"

!insertmacro MUI_LANGUAGE "Swedish"
!include "${ROOT_PATH}\src\translation\wininstaller\se.nsi"

!insertmacro MUI_LANGUAGE "PortugueseBR"
!include "${ROOT_PATH}\src\translation\wininstaller\pt_br.nsi"

!insertmacro MUI_LANGUAGE "Portuguese"
!include "${ROOT_PATH}\src\translation\wininstaller\pt.nsi"

!insertmacro MUI_LANGUAGE "SimpChinese"
!include "${ROOT_PATH}\src\translation\wininstaller\zh_cn.nsi"
