cd "$PSScriptRoot\.."

$tempdir = ""
while ($true) {
    $r = Get-Random -Minimum 100 -Maximum 999
    $tempdir = "$env:temp\Uninstaller-$r"
    if (-not (Test-Path -Path $tempdir)) {
        break
    }
}
Write-Output "TEMP: $tempdir"
New-Item -Path "$tempdir" -ItemType Directory

$pc = Start-Process -PassThru -Wait -FilePath "$PSScriptRoot\app_uninstall.exe" -ArgumentList "-c", "$tempdir"
$exitCode = $pc.ExitCode
#Write-Output "Done 1: $exitCode"

if ( $exitCode -eq 0 ) {
    $pu = Start-Process -PassThru -Wait -FilePath "$tempdir\app_uninstall.exe" -ArgumentList "-u", "$PSScriptRoot"
    #$exitCode = $pu.ExitCode
    #Write-Output "Done 2: $exitCode"
}

#Add-Type -AssemblyName System.Windows.Forms
#[void] [System.Windows.Forms.MessageBox]::Show( "Uninstalled '$PSScriptRoot' ?", "Script completed", "OK", "Information" )

Remove-Item $tempdir -Recurse -Force


