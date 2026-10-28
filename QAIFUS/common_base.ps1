$BUILDDIR = "build"
$INSTALLDIR = "app_bundle"

### This build script uses a standard Qt installation set up by the
### Qt online installer.

$QTDIR = "C:\Qt"
$QTVERSION = Get-ChildItem -Path "$QTDIR" -Filter "6.1*" | Select-Object -ExpandProperty Name -Last 1
#$QTVERSION = "6.12.0"
if ( [string]::IsNullOrEmpty($QTVERSION) )
{
    Write-Host "No suitable Qt installation found in '$QTDIR'"
    exit 1
}
Write-Host "Using '$QTDIR\$QTVERSION'"
Write-Host

$env:Path = "$QTDIR\Tools\CMake_64\bin;$QTDIR\Tools\mingw1310_64\bin;" + $env:Path
