; Because of included international strings, run this with:
;   MakeNSIS /INPUTCHARSET UTF8 fet_mui2.nsi

;--------------------------------
Unicode true
; Strong, but slow, compression:
SetCompressor /SOLID lzma
RequestExecutionLevel user

; Includes

!include "MUI2.nsh"
!include "LogicLib.nsh"

;--------------------------------
; Custom defines
!define APPNAME "FET"
!define APPFILE "fet.exe"
!define APPEXEC "${APPNAME}\${APPFILE}"
!define /file FETVERSION "VERSION"
!define ASSOC_EXT ".fet"
!define ASSOC_PROGID "FET.Main"

!define UNINFO "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!include "FileFunc.nsh"
;--------------------------------
; General

; rtf or txt file - remember if it is txt, it must be in the DOS text format (\r\n)
LicenseData "COPYING"
; This will be in the installer/uninstaller's title bar
Name "${APPNAME}"
OutFile "${APPNAME}-${FETVERSION}-Setup.exe"

InstallDir "$LOCALAPPDATA\Programs"

;--------------------------------
; Interface Settings

  !define MUI_ABORTWARNING
  !define MUI_ICON icons\fet.ico

;--------------------------------
; Pages

  !insertmacro MUI_PAGE_LICENSE "COPYING"
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  ; Create desktop shortcut
  !define MUI_FINISHPAGE_SHOWREADME ""
  !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
  !define MUI_FINISHPAGE_SHOWREADME_TEXT "$(desktopLnk)"
  !define MUI_FINISHPAGE_SHOWREADME_FUNCTION FinishPageAction

  !define MUI_FINISHPAGE_RUN
  ;!define MUI_FINISHPAGE_RUN_TEXT "$(startFet)"
  !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"

  !insertmacro MUI_PAGE_FINISH

Function FinishPageAction
    ; Make a Desktop shortcut
    CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$InstDir\${APPEXEC}"
FunctionEnd

Function LaunchLink
  ExecShell "" "$SMPROGRAMS\${APPNAME}.lnk"
FunctionEnd
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
;--------------------------------
;Languages
  ; Languages to be supported need to be "inserted" here. This
  ; looks something like:

  ;!insertmacro MUI_LANGUAGE "English"
  ;!insertmacro MUI_LANGUAGE "French"
  ;!insertmacro MUI_LANGUAGE "German"

  ; All available languages can, however, be fetched from the NSIS
  ; installation by using a further build script to produce a
  ; small .exe file which generates a list of these insertion
  ; commands to a text file. The resulting text file is then
  ; included in this build script.
!makensis "get_languages.nsi"
!system "GetLanguages.exe"
!include "languages.txt"
; Optional cleanup
!delfile "GetLanguages.exe"
!delfile "languages.txt"

!include "installer_i18n.txt"

; Uncomment this to add a language choice page. Without it, the
; system language will be used, if possible:
;Function .onInit
;    !insertmacro MUI_LANGDLL_DISPLAY
;FunctionEnd

; Function to check for an existing installation
Function OldExists
    ReadRegStr $0 ShCtx "${UNINFO}" "UninstallString"

    ; Strip single leading and trailing '"'
    StrCpy $1 $0 1
    StrCmp $1 `"` 0 +2
      StrCpy $0 $0 `` 1
    StrCpy $1 $0 1 -1
    StrCmp $1 `"` 0 +2
      StrCpy $0 $0 -1

    ${IfNot} $0 == ""
    ${AndIf} ${FileExists} "$0"
        ; Assume the uninstaller is installed in the application root directory
        ${GetParent} "$0" $1
        MessageBox MB_YESNO|MB_ICONQUESTION "$(uninstallPrevious) ($1)" /SD IDYES IDYES labelyes
            Quit
          labelyes:
            ; Run the uninstaller
            ExecWait '"$0" /S _?=$1' $2
            ${If} $2 == 0
                ; OK, delete the uninstaller and the installation directory
                Delete "$0"
                RMDir "$1"
            ${Else}
                MessageBox MB_OK|MB_ICONEXCLAMATION "$(uninstallFailed): $1"
                Quit
            ${EndIf}
    ${EndIf}
FunctionEnd

; The RefreshShellIcons functions allow the association of the
; icons with the file type to be changed immediately.

  !define SHCNE_ASSOCCHANGED 0x08000000
  !define SHCNF_IDLIST 0

Function RefreshShellIcons
  ; By jerome tremblay - april 2003
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd

Function un.RefreshShellIcons
  ; By jerome tremblay - april 2003
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd

Section "Install"
    Call OldExists
    ; Files for the install directory - to build the installer,
    ; these should be in the same directory as the install script
    ; (this file).
    SetOutPath $InstDir\${APPNAME}
    ; Files added here should be removed by the uninstaller (see
    ; section "Uninstall")
    File /r "build\install\*.*"

    ; Build uninstaller
    WriteUninstaller "$InstDir\${APPNAME}\uninstall.exe"

    ; Start Menu link
    CreateShortCut "$SMPROGRAMS\${APPNAME}.lnk" "$InstDir\${APPEXEC}" "" "$InstDir\${APPEXEC}"

    ; Register app
    WriteRegStr ShCtx "${UNINFO}" "DisplayName" "${APPNAME} -- free timetable software"
    WriteRegStr ShCtx "${UNINFO}" "UninstallString" '"$InstDir\${APPNAME}\uninstall.exe"'

    WriteRegStr ShCtx "${UNINFO}" "Publisher" "https://lalescu.ro/liviu/fet/"
    WriteRegStr ShCtx "${UNINFO}" "DisplayVersion" "${FETVERSION}"

    ${GetSize} "$InstDir\${APPNAME}" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD ShCtx "${UNINFO}" "EstimatedSize" "$0"

    WriteRegStr ShCtx "${UNINFO}" "UninstallLocation" "$InstDir\${APPNAME}"

    ; Register file-type association
    WriteRegStr ShCtx "SOFTWARE\Classes\${ASSOC_PROGID}\DefaultIcon" "" "$InstDir\${APPEXEC}"
    WriteRegStr ShCtx "Software\Classes\${ASSOC_PROGID}\shell\${APPNAME}\command" "" '"$InstDir\${APPEXEC}" "%1"'
    WriteRegStr ShCtx "Software\Classes\${ASSOC_EXT}" "" "${ASSOC_PROGID}"
    
    Call RefreshShellIcons
SectionEnd

; Uninstaller

Section "Uninstall"
    ; Remove file associations
    ClearErrors
    DeleteRegKey ShCtx "Software\Classes\${ASSOC_PROGID}\shell\${APPNAME}"
    DeleteRegKey /IfEmpty ShCtx "Software\Classes\${ASSOC_PROGID}\shell"
    ${IfNot} ${Errors}
        DeleteRegKey ShCtx "Software\Classes\${ASSOC_PROGID}\DefaultIcon"
    ${EndIf}
    ReadRegStr $0 ShCtx "Software\Classes\${ASSOC_EXT}" ""
    DeleteRegKey /IfEmpty ShCtx "Software\Classes\${ASSOC_PROGID}"
    ${IfNot} ${Errors}
    ${AndIf} $0 == "${ASSOC_PROGID}"
        DeleteRegValue ShCtx "Software\Classes\${ASSOC_EXT}" ""
        DeleteRegKey /IfEmpty ShCtx "Software\Classes\${ASSOC_EXT}"
    ${EndIf}

    ; Remove Start Menu and Desktop launchers
    Delete "$SMPROGRAMS\${APPNAME}.lnk"
    Delete "$DESKTOP\${APPNAME}.lnk"
    
    ; Delete app folder
    RMDir /r "$InstDir"

    ; Remove uninstaller information from the registry
    DeleteRegKey ShCtx "${UNINFO}" 

    Call un.RefreshShellIcons
SectionEnd

