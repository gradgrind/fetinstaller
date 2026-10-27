$APPVERSION = Get-Content  -ErrorAction Stop "..\VERSION"
$DESKTOP = "..\fet.desktop"

Write-Host "Building $APPNAME-$APPVERSION ..."
Write-Host

cmake -B "build\$APPNAME" -S .. -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" -DCMAKE_GENERATOR="MinGW Makefiles" -DCMAKE_INSTALL_PREFIX="build\$install_prefix"

if ( !$? )
{
    Write-Host "***!!!*** $APPNAME configuration failed"
    exit 1
}

cmake --build "build\$APPNAME" --target install --parallel 6

if ( !$? )
{
    Write-Host "***!!!*** $APPNAME build failed"
    exit 1
}

