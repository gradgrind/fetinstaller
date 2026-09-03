# Build script for Windows (Powershell) using a standard Qt installation set up
# by the the Qt online installer.
# Run this in the FET source (root) directory.

$QTVERSION="6.11.2"
$QTDIR="C:\Qt"

$env:Path="$QTDIR\Tools\CMake_64\bin;$QTDIR\Tools\mingw1310_64\bin;" + $env:Path

cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.11.2\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=build\install

cmake --build build --target install --parallel 6

$env:Path="C:\Program Files (x86)\NSIS;" + $env:Path
makensis /INPUTCHARSET UTF8 /V4 fet.nsi

