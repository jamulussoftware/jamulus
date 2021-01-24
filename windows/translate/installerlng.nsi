; Language configuration

; Additional languages can be added in this file. See https://nsis.sourceforge.io/Examples/Modern%20UI/MultiLanguage.nsi

!insertmacro MUI_LANGUAGE "English" ; The first language is the default
!include "translate\en.nsi" ; include english

!insertmacro MUI_LANGUAGE "German"
!include "translate\de.nsi"

; !insertmacro MUI_LANGUAGE "Italian"
; !include "translate\it.nsi" ; include italian
