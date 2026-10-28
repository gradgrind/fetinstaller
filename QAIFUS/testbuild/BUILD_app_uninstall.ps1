cd "$PSScriptRoot"

$APPNAME = "app_uninstall"

$APPDIR = "..\$APPNAME"
. "..\common_base.ps1"
. "..\common_build.ps1"

########### FOR TEST PURPOSES ###########
$Env:QTLANG = "de"
Start-Process -FilePath "$BUILDDIR\$INSTALLDIR\$APPNAME.exe"
