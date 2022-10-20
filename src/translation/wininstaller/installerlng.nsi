; Language configuration

; Additional languages can be added in this file. See https://nsis.sourceforge.io/Examples/Modern%20UI/MultiLanguage.nsi

!insertmacro MUI_LANGUAGE "English" ; The first language is the default
!include "${ROOT_PATH}\src\translation\wininstaller\en_GB.nsi" ; include english, standard language

!insertmacro MUI_LANGUAGE "German"
!include "${ROOT_PATH}\src\translation\wininstaller\de_DE.nsi"

!insertmacro MUI_LANGUAGE "Italian"
!include "${ROOT_PATH}\src\translation\wininstaller\it_IT.nsi"

!insertmacro MUI_LANGUAGE "Dutch"
!include "${ROOT_PATH}\src\translation\wininstaller\nl_NL.nsi"

!insertmacro MUI_LANGUAGE "Polish"
!include "${ROOT_PATH}\src\translation\wininstaller\pl_PL.nsi"

!insertmacro MUI_LANGUAGE "French"
!include "${ROOT_PATH}\src\translation\wininstaller\fr_FR.nsi"

!insertmacro MUI_LANGUAGE "Spanish"
!include "${ROOT_PATH}\src\translation\wininstaller\es_ES.nsi"

!insertmacro MUI_LANGUAGE "Swedish"
!include "${ROOT_PATH}\src\translation\wininstaller\sv_SE.nsi"

!insertmacro MUI_LANGUAGE "PortugueseBR"
!include "${ROOT_PATH}\src\translation\wininstaller\pt_BR.nsi"

!insertmacro MUI_LANGUAGE "Portuguese"
!include "${ROOT_PATH}\src\translation\wininstaller\pt_PT.nsi"

!insertmacro MUI_LANGUAGE "SimpChinese"
!include "${ROOT_PATH}\src\translation\wininstaller\zh_CN.nsi"

!insertmacro MUI_LANGUAGE "Korean"
!include "${ROOT_PATH}\src\translation\wininstaller\ko_KR.nsi"
