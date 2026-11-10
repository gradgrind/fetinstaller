# Build script for Windows (Powershell) using a standard Qt installation set up
# by the the Qt online installer.

# The source packages should be unpacked in the same directory as the build script, which
# should then be started using:
#    powershell -ExecutionPolicy Bypass -File  compile_xz_libarchive.ps1

$QTVERSION = "6.11.3"
$QTDIR = "C:\Qt"

cd "$PSScriptRoot"

$env:Path = "$QTDIR\Tools\Ninja;$QTDIR\Tools\CMake_64\bin;$QTDIR\Tools\mingw1310_64\bin;" + $env:Path

cmake -B build\xz -S xz-5.8.4 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="Ninja" -DCMAKE_INSTALL_PREFIX=build\install

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

cmake -B build\la -S libarchive-3.8.11 -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="Ninja" -DCMAKE_INSTALL_PREFIX=build\install

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
