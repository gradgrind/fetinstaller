# Build script for Windows (Powershell) using a standard Qt installation set up
# by the the Qt online installer.

$QTVERSION="6.11.2"
$QTDIR="C:\Qt"

$env:APPEXEC = "fet"

cd "$PSScriptRoot"

$env:Path="$QTDIR\Tools\CMake_64\bin;$QTDIR\Tools\mingw1310_64\bin;" + $env:Path

cmake -B build\FET -S .. -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=build\install

if ( !$? )
{
    Write-Host "ABORTING 1";
    exit 1
}

cmake --build build\FET --target install --parallel 6

if ( !$? )
{
    Write-Host "ABORTING 2";
    exit 1
}

# Build the installer
cmake -B build\installer -S installer -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=build\install

if ( !$? )
{
    Write-Host "ABORTING 3";
    exit 1
}

cmake --build build\installer --target install --parallel 6

if ( !$? )
{
    Write-Host "ABORTING 4";
    exit 1
}

# Build the uninstaller
cmake -B build\uninstaller -S uninstaller -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=build\install

if ( !$? )
{
    Write-Host "ABORTING 5";
    exit 1
}

cmake --build build\uninstaller --target install --parallel 6

if ( !$? )
{
    Write-Host "ABORTING 6";
    exit 1
}

$version = Get-Content "..\VERSION"
