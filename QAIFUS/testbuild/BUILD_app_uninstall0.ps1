cd "$PSScriptRoot"

$PROJECT_VERSION = Get-Content  -ErrorAction Stop "..\VERSION"

$PROJECTNAME = "AppUninstall0"
$APPNAME = "app_uninstall0"

$APPDIR = "..\$APPNAME"
. "..\common_base.ps1"
. "..\common_build.ps1"

########### FOR TEST PURPOSES ###########
$Env:QTLANG = "de"
Start-Process -FilePath "$BUILDDIR\$INSTALLDIR\$APPNAME.exe"
