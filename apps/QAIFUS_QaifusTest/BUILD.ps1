# Build the app which is to be packaged.

$PROJECT_VERSION = Get-Content  -ErrorAction Stop "$APPDIR\VERSION"

$DESKTOP = "$APPDIR\$APPNAME.desktop"

. ".\common_build.ps1"
