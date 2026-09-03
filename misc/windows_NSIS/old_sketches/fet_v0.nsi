;--------------------------------
Unicode true
; Strong, but slow, compression:
SetCompressor /SOLID lzma
RequestExecutionLevel user

; Includes

;TODO: !include "MUI2.nsh"
!include "LogicLib.nsh"

;--------------------------------
; Custom defines
!define APPNAME "FET"
!define APPFILE "fet.exe"
!define APPEXEC "${APPNAME}\${APPFILE}"
!define /file FETVERSION "VERSION"

!define ARP "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!include "FileFunc.nsh"
;--------------------------------
; General

; rtf or txt file - remember if it is txt, it must be in the DOS text format (\r\n)
LicenseData "COPYING"
; This will be in the installer/uninstaller's title bar
Name "${APPNAME}"
OutFile "${APPNAME}-${FETVERSION}-Setup.exe"
RequestExecutionLevel user

InstallDir "$LOCALAPPDATA\Programs"

;--------------------------------
; UI
Icon icons\fet.ico

; Install
Page License
Page Directory "" "" DirLeave
Page InstFiles

; Function to check for an existing installation
Function DirLeave
    Var /GLOBAL upath
    ReadRegStr $upath ShCtx "${ARP}" "UninstallLocation"
    ${IfNot} $upath == ""
    ${AndIf} ${FileExists} "$upath"
        MessageBox MB_YESNO `"${APPNAME}" is already installed in "$upath". Delete it and continue installing?` IDYES labelyes
            Quit
        labelyes:
        RMDir /r "$upath"
        ;RMDir /r "$STARTMENU\${APPNAME}"
        Delete "$SMPROGRAMS\${APPNAME}.lnk"
    ${EndIf}
FunctionEnd

; ??? Automatically start app after installation
Function .onInstSuccess
    Exec "$InstDir\${APPEXEC}"
FunctionEnd

Section "-install"
    ; Files for the install directory - to build the installer, these should be in the same directory as the install script (this file)
    SetOutPath $InstDir\${APPNAME}
    ; Files added here should be removed by the uninstaller (see section "uninstall")
    File /r "build\install\*.*"

    ; Add any other files for the install directory here

    ; Uninstaller
    WriteUninstaller "$InstDir\${APPNAME}\uninstall.exe"

    ; Start Menu
    ;CreateDirectory "$SMPROGRAMS\${APPNAME}"
    ;CreateShortCut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$InstDir\${APPEXEC}" "" "$InstDir\${APPEXEC}"
    ;createShortCut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$InstDir\${APPNAME}\uninstall.exe"
    CreateShortCut "$SMPROGRAMS\${APPNAME}.lnk" "$InstDir\${APPEXEC}" "" "$InstDir\${APPEXEC}"
SectionEnd

Section "-register app"
    WriteRegStr ShCtx "${ARP}" "DisplayName" "${APPNAME} -- free timetable software"
    WriteRegStr ShCtx "${ARP}" "UninstallString" "$\"$InstDir\${APPNAME}\uninstall.exe$\""

    WriteRegStr ShCtx "${ARP}" "Publisher" "https://lalescu.ro/liviu/fet/"
    WriteRegStr ShCtx "${ARP}" "DisplayVersion" "${FETVERSION}"

    ${GetSize} "$InstDir\${APPNAME}" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD ShCtx "${ARP}" "EstimatedSize" "$0"

    WriteRegStr ShCtx "${ARP}" "UninstallLocation" "$InstDir\${APPNAME}"
SectionEnd

!define ASSOC_EXT ".fet"
!define ASSOC_PROGID "FET.Main"

Section "-register file-type association"
    WriteRegStr ShCtx "SOFTWARE\Classes\${ASSOC_PROGID}\DefaultIcon" "" "$InstDir\${APPEXEC},0"
    WriteRegStr ShCtx "Software\Classes\${ASSOC_PROGID}\shell\${APPNAME}\command" "" '"$InstDir\${APPEXEC}" "%1"'
    WriteRegStr ShCtx "Software\Classes\${ASSOC_EXT}" "" "${ASSOC_PROGID}"
SectionEnd

# Uninstaller

Function un.onInit
    SetShellVarContext Current

    ; Confirm uninstall - last chance to back out
    MessageBox MB_OKCANCEL "Permanantly remove $InstDir?" IDOK next
        Abort
    next:
FunctionEnd

Section "-un.deregister file-type associations"
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
SectionEnd

Section "-uninstall"
    ; Remove Start Menu launchers
    ;RMDir /r "$STARTMENU\${APPNAME}"
    Delete "$SMPROGRAMS\${APPNAME}.lnk"

    ; Delete app folder
    RMDir /r "$InstDir"

    ; Remove uninstaller information from the registry
    DeleteRegKey ShCtx "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
sectionEnd

