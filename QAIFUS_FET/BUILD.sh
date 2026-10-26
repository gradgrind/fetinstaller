APPVERSION=$(<../VERSION)

echo "Building $APPNAME-$APPVERSION ..."
echo

cmake -B "build/$APPNAME" -S .. -DCMAKE_PREFIX_PATH="$QTDIR/$QTVERSION/gcc_64" -DCMAKE_INSTALL_PREFIX="build/$install_prefix"

if [ $? -ne 0 ]; then
    echo "***!!!*** $APPNAME configuration failed"
    exit 1
fi

cmake --build "build/$APPNAME" --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "***!!!*** $APPNAME build failed"
    exit 1
fi

