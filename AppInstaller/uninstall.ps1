$APPNAME="FET"

$tempdir = ""
while ($true) {
    $r = Get-Random -Minimum 100 -Maximum 999
    $tempdir = "$env:temp\$APPNAME-$r"
    if (-not (Test-Path -Path $tempdir)) {
        break
    }
}
Write-Output "TEMP: $tempdir"

cd "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\$APPNAME"
Get-Item .
$p1=(Get-ItemProperty . -Name UninstallString).UninstallString
Write-Output "Uninstaller: $p1"

$l0 = (Get-ItemProperty . -Name UninstallLocation).UninstallLocation
Write-Output "Installation root: $l0"
if ("$l0" -eq "$PSScriptRoot") {
    Write-Output "Uninstall"
} else {
    Write-Output "Not installed"
}

Start-Process -Wait -FilePath "$PSScriptRoot\app_uninstall.exe" -ArgumentList "-c", "$tempdir"
Write-Output "Done 1"

#Remove-ItemProperty . -Name TEST

#Remove-Item "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\FETX"

#Remove-Item $PSScriptRoot -Recurse -Force

Add-Type -AssemblyName System.Windows.Forms
[void] [System.Windows.Forms.MessageBox]::Show( "Uninstalled '$PSScriptRoot' ?", "Script completed", "OK", "Information" )
