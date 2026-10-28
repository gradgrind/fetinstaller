# Build the app which is to be packaged.

$APPLICATION_VERSION = Get-Content  -ErrorAction Stop "$APPDIR\VERSION"

$DESKTOP = "$APPDIR\fet.desktop"

. ".\common_build.ps1"
