# Build the app which is to be packaged.

$APPLICATION_VERSION = Get-Content  -ErrorAction Stop "$APPDIR\VERSION"

$DESKTOP = "$APPDIR\$APPNAME.desktop"

. ".\common_build.ps1"
