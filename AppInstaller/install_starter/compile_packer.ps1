# Build script for Windows (Powershell) using a standard Qt installation set up
# by the the Qt online installer.

$QTVERSION = "6.11.2"
$QTDIR = "C:\Qt"

cd "$PSScriptRoot"

$env:Path = "$QTDIR\Tools\CMake_64\bin;$QTDIR\Tools\mingw1310_64\bin;" + $env:Path

gcc -o win_unpacker.exe win_unpacker.c
