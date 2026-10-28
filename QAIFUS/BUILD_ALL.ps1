# Build script for Windows (Powershell).

# Build the application itself, app_install, app_uninstall0 and
# app_uninstall, placing the installation bundle in "build/app_bundle".

# This build script uses a standard Qt installation set up by the
# Qt online installer.

# Place the "QAIFUS" folder and a suitable "QAIFUS_$APPLICATION_NAME" folder
# in the root folder of the application's source code.
# Then run this script, e.g.:
#    powershell -ExecutionPolicy Bypass -File BUILD_ALL.ps1 -Argument-List APPLICATION_NAME

# If necessary, the working directory will be changed to run in the directory
# containing this script.

cd "$PSScriptRoot"
. ".\common_base.ps1"

# Read the application name and include its build script
$APPLICATION_NAME = $args[1]
if ( [string]::IsNullOrEmpty($APPLICATION_NAME) )
{
    Write-Host "Application name missing, use:"
    $name = $MyInvocation.MyCommand.Name
    Write-Host "  powershell -ExecutionPolicy Bypass -File $name -Argument-List APPLICATION_NAME"
    exit 1
}
$APPDIR = ".."
$QAIFUS_APPDIR = Resolve-Path -Path "$APPDIR\QAIFUS_$APPLICATION_NAME"
if ( Test-Path -path "$QAIFUS_APPDIR\BUILD.ps1" ) {
    $APPNAME = "$APPLICATION_NAME"
    . "$QAIFUS_APPDIR\BUILD.ps1"
}
else
{
    Write-Host "File not found: $QAIFUS_APPDIR\BUILD.ps1"
    exit 1
}

Write-Host
Write-Host "Build the installer components ..."
Write-Host

$APPNAME = "app_install"
$APPDIR = "$APPNAME"
. ".\common_build.ps1"

$APPNAME = "app_uninstall0"
$APPDIR = "$APPNAME"
. ".\common_build.ps1"

$APPNAME = "app_uninstall"
$APPDIR = "$APPNAME"
. ".\common_build.ps1"

# Ensure that the "_installer_" directory exists
$installerdir = "$BUILDDIR\$INSTALLDIR\_installer_"
if ( -not ( Test-Path -Path "$installerdir" ) ) {
    New-Item -Path "$installerdir" -ItemType Directory
}
# Add the installer app-configuration file
(Get-Content "$QAIFUS_APPDIR\app.conf") `
    -replace '%VERSION%', "$APPLICATION_VERSION" `
    -replace '%APPNAME%', "$APPLICATION_NAME" |
    Set-Content "$installerdir\app.conf"

Copy-Item "$DESKTOP" -Destination "$installerdir\desktop"

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

cd "$BUILDDIR\$INSTALLDIR"
Start-Process -FilePath "$tarx" -Wait -NoNewWindow -ArgumentList "-cJf ..\app_bundle.tar.xz *"

if ( !$? )
{
    Write-Host "***!!!*** app bundle compression failed"
    exit 1
}

Write-Host
Write-Host "Build app installer: $APPLICATION_NAME-$APPLICATION_VERSION.exe"
Write-Host

cd "..\..\appin"
gcc -mwindows -o "..\$APPLICATION_NAME-$APPLICATION_VERSION.exe" appinw.c

if ( !$? )
{
    Write-Host "***!!!*** final (appin) packaging failed"
    exit 1
}

