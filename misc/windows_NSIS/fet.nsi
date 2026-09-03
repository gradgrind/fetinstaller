; Because of included international strings, run this with:
;   MakeNSIS /INPUTCHARSET UTF8 fet.nsi

; TODO: Check errors after file extraction, registry writes, and directory removal.
;--------------------------------
Unicode true
; Strong, but slow, compression:
SetCompressor /SOLID lzma
RequestExecutionLevel user

; Included modules

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"

;--------------------------------
; Custom defines
!define APPNAME "FET"
!define APPFILE "fet.exe"
!define APPEXEC "${APPNAME}\${APPFILE}"
!define /file FETVERSION "VERSION"
!define ASSOC_EXT ".fet"
!define ASSOC_PROGID "FET.Main"

; Registry key for uninstaller
!define UNINFO "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

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
  
  ; "Create desktop shortcut" checkbox for final page –
  ; this is not "built in" so use the "Show README" checkbox
  ; with a modified text
  !define MUI_FINISHPAGE_SHOWREADME ""
  !define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
  !define MUI_FINISHPAGE_SHOWREADME_TEXT "$(desktopLnk)"
  !define MUI_FINISHPAGE_SHOWREADME_FUNCTION FinishPageAction

  ; "Run FET" checkbox for final page
  !define MUI_FINISHPAGE_RUN
  !define MUI_FINISHPAGE_RUN_FUNCTION "LaunchLink"

  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  
Function FinishPageAction
    ; Make a Desktop shortcut
    CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$InstDir\${APPEXEC}"
FunctionEnd

Function LaunchLink
  ExecShell "" "$SMPROGRAMS\${APPNAME}.lnk"
FunctionEnd
  
;--------------------------------
;Languages
!include "installer_i18n.txt"

; Uncomment this to show a language choice dialog – without it, the
; system language will be used, if possible:
#Function .onInit
#    !insertmacro MUI_LANGDLL_DISPLAY
#FunctionEnd

; Function to check for an existing installation
Function OldExists
    ; Get uninstaller path from registry (store in variable $0)
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
        ; Assume the uninstaller is installed in the application,
        ; get the root directory in variable $1
        ${GetParent} "$0" $1
        MessageBox MB_YESNO|MB_ICONQUESTION "$(uninstallPrevious) ($1)" /SD IDYES IDYES labelyes
            Quit
          labelyes:
            ; Run the uninstaller, wait for completion
            ExecWait '"$0" /S _?=$1' $2
            ${If} $2 == 0
                ; OK, delete the uninstaller and the installation
                ; directory – these won't have been deleted because
                ; they were in use by the uninstaller
                Delete "$0"
                RMDir "$1"
            ${Else}
                MessageBox MB_OK|MB_ICONSTOP "$(uninstallFailed): $1"
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
    ; Check for previous installation and remove it
    Call OldExists
    ; Files for the install directory - to build the installer
    ; these should be in the same directory as the install script
    ; (this file).
    SetOutPath $InstDir\${APPNAME}
    ; Files added here should be removed by the uninstaller (see
    ; section "Uninstall")
    File /r "build\install\*.*"

    ; Build uninstaller
    WriteUninstaller "$InstDir\${APPNAME}\uninstall.exe"

    ; Create Start Menu link
    CreateShortCut "$SMPROGRAMS\${APPNAME}.lnk" "$InstDir\${APPEXEC}" "" "$InstDir\${APPEXEC}"

    ; Register uninstaller
    WriteRegStr ShCtx "${UNINFO}" "DisplayName" "${APPNAME} -- free timetable software"
    WriteRegStr ShCtx "${UNINFO}" "UninstallString" '"$InstDir\${APPNAME}\uninstall.exe"'

    WriteRegStr ShCtx "${UNINFO}" "Publisher" "Liviu Lalescu"
    WriteRegStr ShCtx "${UNINFO}" "UrlInfoAbout" "https://lalescu.ro/liviu/fet/"
    WriteRegStr ShCtx "${UNINFO}" "DisplayVersion" "${FETVERSION}"

    ${GetSize} "$InstDir\${APPNAME}" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD ShCtx "${UNINFO}" "EstimatedSize" "$0"

    WriteRegStr ShCtx "${UNINFO}" "UninstallLocation" "$InstDir\${APPNAME}"

    ; Register file-type association
    WriteRegStr ShCtx "Software\Classes\${ASSOC_EXT}\OpenWithProgIds" "${ASSOC_PROGID}" ""
    WriteRegStr ShCtx "Software\Classes\${ASSOC_PROGID}\shell\open" "FriendlyAppName" "${APPNAME} ${FETVERSION}"
    WriteRegStr ShCtx "Software\Classes\${ASSOC_PROGID}\shell\open\command" "" '"$InstDir\${APPEXEC}" "%1"'

    WriteRegStr ShCtx "SOFTWARE\Classes\${ASSOC_PROGID}\DefaultIcon" "" '"$InstDir\${APPEXEC}",0'
    ; Update file-type associations
    Call RefreshShellIcons
SectionEnd

; Uninstaller

Section "Uninstall"
    ; Remove file-type associations
    ClearErrors
    DeleteRegKey ShCtx "Software\Classes\${ASSOC_PROGID}\"
    IfErrors 0 +2
        MessageBox MB_OK "Failed to delete ${ASSOC_PROGID}\"

    ReadRegStr $0 ShCtx "Software\Classes\${ASSOC_EXT}" ""
    ${If} $0 == "${ASSOC_PROGID}"
        DeleteRegValue ShCtx "Software\Classes\${ASSOC_EXT}" ""
    ${EndIf}

    ClearErrors
    DeleteRegValue ShCtx "Software\Classes\${ASSOC_EXT}\OpenWithProgIds" "${ASSOC_PROGID}"
    ${IfNot} ${Errors}
        DeleteRegKey /IfEmpty ShCtx "Software\Classes\${ASSOC_EXT}\OpenWithProgIds\"
        ${IfNot} ${Errors}
            DeleteRegKey /IfEmpty ShCtx "Software\Classes\${ASSOC_EXT}\"
        ${EndIf}
    ${EndIf}

    ; Remove Start Menu and Desktop launchers
    Delete "$SMPROGRAMS\${APPNAME}.lnk"
    Delete "$DESKTOP\${APPNAME}.lnk"
    
    ; Delete app folder
    RMDir /r "$InstDir"

    ; Remove uninstaller information from the registry
    ClearErrors
    DeleteRegKey ShCtx "${UNINFO}\" 
    IfErrors 0 +2
        MessageBox MB_OK "Failed to delete ${UNINFO}\"

    ; Update file-type associations
    Call un.RefreshShellIcons
SectionEnd

