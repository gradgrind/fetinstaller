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

Write-Host "`nCreate app_bundle archive: build/app_bundle.tar.xz`n"

# Prefer "bsdtar" because the built-in "tar" on older systems might not
# support xz compression. On such older systems, a "bsdtar.exe" needs
# to be placed on the PATH, e.g. in:
#   "C:\Users\User\AppData\Local\Microsoft\WindowsApps\".
try {
    $tarx = (Get-Command -Name bsdtar.exe -CommandType Application -ErrorAction Stop).Path
}
catch {
    try {
        $tarx = (Get-Command -Name tar.exe -CommandType Application -ErrorAction Stop).Path
    }
    catch {
        Write-Host "No 'tar' program found"
        exit 1
    }
}

cd build\app_bundle
Start-Process -FilePath "$tarx" -Wait -NoNewWindow -ArgumentList "-cJf ..\app_bundle.tar.xz *"

if ( !$? )
{
    Write-Host "ABORTING 7"
    exit 1
}

Write-Host "`Build app installer: $APPNAME-$APPVERSION.exe`n"

cd ../../appin
gcc -static -o "../$APPNAME-$APPVERSION.exe" appinw.c

