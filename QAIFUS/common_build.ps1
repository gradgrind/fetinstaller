### Build "$APPNAME", placing the installation bundle in "$BUILDDIR/$INSTALLDIR"

Write-Host
Write-Host "Building $APPNAME..."
Write-Host

cmake -B "$BUILDDIR\$APPNAME" -S "$APPDIR" `
    -DCMAKE_PREFIX_PATH="$QTDIR\$QTVERSION\mingw_64" `
    -DCMAKE_GENERATOR="MinGW Makefiles" `
    -DCMAKE_INSTALL_PREFIX="$BUILDDIR\$INSTALLDIR"

if ( !$? )
{
    Write-Host "***!!!*** $APPNAME configuration failed"
    exit 1
}

cmake --build "$BUILDDIR\$APPNAME" --target install --parallel 6

if ( !$? )
{
    Write-Host "***!!!*** $APPNAME build failed"
    exit 1
}
