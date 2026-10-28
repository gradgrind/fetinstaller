### Build "$APPNAME", placing the installation bundle in "$BUILDDIR/$INSTALLDIR"

cmake -B "$BUILDDIR/$APPNAME" -S "$ROOTDIR/$APPNAME" \
    -DCMAKE_PREFIX_PATH="$QTDIR/$QTVERSION/gcc_64" \
    -DCMAKE_INSTALL_PREFIX="$BUILDDIR/$INSTALLDIR"

if [ $? -ne 0 ]; then
    echo "***!!!*** $APPNAME configuration failed"
    exit 1
fi

cmake --build "$BUILDDIR/$APPNAME" --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "***!!!*** $APPNAME build failed"
    exit 1
fi
