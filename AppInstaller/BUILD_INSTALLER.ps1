# Build script for Windows (Powershell) using a standard Qt installation set up
# by the the Qt online installer.
# This is a Powershell script. To run it, it may be necessary to do something like:
# "powershell -ExecutionPolicy Bypass -File BUILD_INSTALLER.ps1 -Argument-List MyApp"

cd "$PSScriptRoot"

$APPNAME = $args[1]
if ( [string]::IsNullOrEmpty($APPNAME) )
{
    Write-Host "Application name missing, use:"
    $name = $MyInvocation.MyCommand.Name
    Write-Host "  powershell -ExecutionPolicy Bypass -File $name -Argument-List APPNAME"
    exit 1
}
else
{
    Write-Host "Got `"$APPNAME`""
}

$QTDIR = "C:\Qt"
$QTVERSION = Get-ChildItem -Path "$QTDIR" -Filter "6.1*" | Select-Object -ExpandProperty Name -Last 1
#$QTVERSION = "6.12.0"
if ( [string]::IsNullOrEmpty($QTVERSION) )
{
    Write-Host "No suitable Qt installation found in '$QTDIR'"
    exit 1
}
Write-Host "Using '$QTDIR\$QTVERSION'"

$env:Path = "$QTDIR\Tools\CMake_64\bin;$QTDIR\Tools\mingw1310_64\bin;" + $env:Path

$install_prefix = "app_bundle"

# Build app, assuming it is in the parent directory
Write-Host "Starting build process ..."
Write-Host

$APPDIR = Resolve-Path -Path "..\QAIFUS_$APPNAME"
if ( Test-Path -path "$APPDIR\BUILD.ps1" ) {
    . "$APPDIR\BUILD.sh"
}
else
{
    Write-Host "File not found: $APPDIR\BUILD.sh"
    exit 1
}

Write-Host
Write-Host "Build the installer app..."
Write-Host

cmake -B build\installer -S installer -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="build\$install_prefix"

if ( !$? )
{
    Write-Host "***!!!*** installer app configuration failed"
    exit 1
}

cmake --build build\installer --target install --parallel 6

if ( !$? )
{
    Write-Host "***!!!*** installer app build failed"
    exit 1
}

# Add the installer configuration files
$installerdir = "build\app_bundle\_installer_"
if ( -not ( Test-Path -Path "$installerdir" ) ) {
    New-Item -Path "$installerdir" -ItemType Directory
}
(Get-Content "$APPDIR\app.conf") `
    -replace '%VERSION%', "$APPVERSION" `
    -replace '%APPNAME%', "$APPNAME" |
    Set-Content build\app_bundle\_installer_\app.conf

Copy-Item "$DESKTOP" -Destination "$installerdir\desktop"

Write-Host
Write-Host "Build the uninstaller app ..."
Write-Host

cmake -B build\uninstaller -S uninstaller -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="build\$install_prefix"

if ( !$? )
{
    Write-Host "***!!!*** uninstaller app configuration failed"
    exit 1
}

cmake --build build\uninstaller --target install --parallel 6

if ( !$? )
{
    Write-Host "***!!!*** uninstaller app build failed"
    exit 1
}

# Build the pre-uninstaller
cmake -B build\pre_uninstaller -S pre_uninstaller -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="build\$install_prefix"

if ( !$? )
{
    Write-Host "***!!!*** pre_uninstaller app configuration failed"
    exit 1
}

cmake --build build\pre_uninstaller --target install --parallel 6

if ( !$? )
{
    Write-Host "***!!!*** pre_uninstaller app build failed"
    exit 1
}

Write-Host
Write-Host "Create app bundle archive: build\app_bundle.tar.xz"
Write-Host

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
    Write-Host "***!!!*** app bundle compression failed"
    exit 1
}

Write-Host
Write-Host "Build app installer: $APPNAME-$APPVERSION.exe"
Write-Host

cd ..\..\appin
gcc -mwindows -o "..\$APPNAME-$APPVERSION.exe" appinw.c

if ( !$? )
{
    Write-Host "***!!!*** final (appin) packaging failed"
    exit 1
}

