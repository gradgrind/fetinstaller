cd "$PSScriptRoot"

$APPNAME = "app_install"
$APPVERSION = "1.0.0"

$APPDIR = "..\$APPNAME"
$DESKTOP = "$APPNAME.desktop"
. "..\common_base.ps1"
. "..\common_build.ps1"

########### FOR TEST PURPOSES ###########
# Ensure that the "_installer_" directory exists
$installerdir = "$BUILDDIR\$INSTALLDIR\_installer_"
if ( -not ( Test-Path -Path "$installerdir" ) ) {
    New-Item -Path "$installerdir" -ItemType Directory
}

# Add the installer app-configuration file
(Get-Content "app.conf") `
    -replace '%VERSION%', "$APPVERSION" `
    -replace '%APPNAME%', "$APPNAME" |
    Set-Content "$installerdir\app.conf"

# Add the "desktop" file
Copy-Item "$DESKTOP" -Destination "$installerdir\desktop"

$Env:QTLANG = "de"
Start-Process -FilePath "$BUILDDIR\$INSTALLDIR\$APPNAME.exe"
