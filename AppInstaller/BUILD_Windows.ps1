# Build script for Windows (Powershell) using a standard Qt installation set up
# by the the Qt online installer.

$QTDIR = "C:\Qt"
$QTVERSION = Get-ChildItem -Path "$QTDIR" -Filter "6.1*" | Select-Object -ExpandProperty Name -Last 1
#$QTVERSION = "6.12.0"
if ( [string]::IsNullOrEmpty($QTVERSION) )
{
    Write-Host "No suitable Qt installation found in '$QTDIR'"
    exit 1
}
Write-Host "Using '$QTDIR\$QTVERSION'"

cd "$PSScriptRoot"
$version = Get-Content "..\VERSION"

$env:Path = "$QTDIR\Tools\CMake_64\bin;$QTDIR\Tools\mingw1310_64\bin;" + $env:Path

$install_prefix = "app_bundle"

cmake -B build\FET -S .. -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="build\$install_prefix"

if ( !$? )
{
    Write-Host "ABORTING 1"
    exit 1
}

cmake --build build\FET --target install --parallel 6

if ( !$? )
{
    Write-Host "ABORTING 2"
    exit 1
}

# Build the installer
cmake -B build\installer -S installer -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="build\$install_prefix"

if ( !$? )
{
    Write-Host "ABORTING 3"
    exit 1
}

cmake --build build\installer --target install --parallel 6

if ( !$? )
{
    Write-Host "ABORTING 4"
    exit 1
}

# Add the installer configuration file
$installerdir = "build\app_bundle\_installer_"
if ( -not ( Test-Path -Path "$installerdir" ) ) {
    New-Item -Path "$installerdir" -ItemType Directory
}
(Get-Content app.conf).Replace('%VERSION%', $version) | Set-Content build\app_bundle\_installer_\app.conf


Copy-Item "..\fet.desktop" -Destination "$installerdir\desktop"

# Build the uninstaller
cmake -B build\uninstaller -S uninstaller -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="build\$install_prefix"

if ( !$? )
{
    Write-Host "ABORTING 5"
    exit 1
}

cmake --build build\uninstaller --target install --parallel 6

if ( !$? )
{
    Write-Host "ABORTING 6"
    exit 1
}

# Bundle up with 7-zip ...
# It is apparently necessary to do this in the "build" directory,
# otherwise the paths in the final package may be wrong.
cd build

if (Test-Path -Path "archive.7z") {
    Remove-Item "archive.7z"
}
$vapp = "fet-$version.exe"
if (Test-Path -Path "$vapp") {
    Remove-Item "$vapp"
}
..\bin\x64\7zr a archive.7z app_bundle -mx -mf=BCJ2
cmd /c "copy /b ..\bin\7zSDu.sfx + ..\7z_config.txt +  archive.7z  $vapp"

