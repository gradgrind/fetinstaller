; An auxiliary builder to help fetching the available languages.

Unicode true
OutFile "GetLanguages.exe"
SilentInstall silent
RequestExecutionLevel user

!include "FileFunc.nsh"
 
Section
 
  ; Read all languages in the "Contrib\Language files" folder of the
  ; NSIS installation.

  ; Write to a file for use in main script
  FileOpen $R0 "$EXEDIR\languages.txt" w
  ${Locate} "${NSISDIR}\Contrib\Language files" "/L=F /M=*.nsh" "FoundLanguage"
  FileClose $R0
 
SectionEnd

Function FoundLanguage
    ${GetBaseName} "$R7" $R1
    FileWrite $R0 '!insertmacro MUI_LANGUAGE "$R1"$\r$\n'
    Push ""
FunctionEnd

