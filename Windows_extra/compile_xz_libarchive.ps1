# For older Windows releases, bsdtar may be needed. It can be be compiled from
# source. Unpack the xz and libarchive source packages here and run this script.

# Build script for Powershell using a standard Qt installation set up
# by the the Qt online installer.
# Only build/install/bin/bsdtar.exe is required for QAIFUS. It can be placed in
# C:\Users\<User>\AppData\Local\Microsoft\WindowsApps.

$QTVERSION = "6.11.3"
$QTDIR = "C:\Qt"

cd "$PSScriptRoot"

$env:Path = "$QTDIR\Tools\CMake_64\bin;$QTDIR\Tools\mingw1310_64\bin;" + $env:Path

cmake -B build\xz -S xz-5.8.4 -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=build\install

if ( !$? )
{
    Write-Host "ABORTING 1";
    exit 1
}

cmake --build build\xz --target install --parallel 6

if ( !$? )
{
    Write-Host "ABORTING 2";
    exit 1
}

cmake -B build\la -S libarchive-3.8.11 -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=build\install

if ( !$? )
{
    Write-Host "ABORTING 1";
    exit 1
}

cmake --build build\la --target install --parallel 6

if ( !$? )
{
    Write-Host "ABORTING 2";
    exit 1
}
