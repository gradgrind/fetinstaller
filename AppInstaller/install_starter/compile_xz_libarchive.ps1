# Build script for Windows (Powershell) using a standard Qt installation set up
# by the the Qt online installer.

$QTVERSION = "6.11.2"
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

cmake -B build\la -S libarchive-3.8.10 -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=build\install

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
