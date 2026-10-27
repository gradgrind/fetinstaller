#!/bin/bash

APPNAME="app_install"
APPVERSION="1.0.0"
INSTALLDIR="app_bundle_test"

# Build "$APPNAME", placing the installation bundle in "build/$INSTALLDIR".

# This build script uses a standard Qt installation set up by the
# Qt online installer.

# If necessary, it will change its working directory to run in the
# directory containing this script.

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR

QTDIR="$HOME/Qt"
# Find newest available Qt version, but exclude 6.12.0, which is broken.
# The lowest acceptable version is 6.10.
QTVERSION=$(ls -r "$QTDIR" | grep -v 6[.]12[.]0 | grep -m 1 ^6[.]1[0-9][.])
if [ -z "$QTVERSION" ]; then
    echo "No suitable Qt installation found in '$QTDIR'"
    exit 1
fi
#QTVERSION="6.12.0"
echo "Using '$QTDIR/$QTVERSION'"
echo

export PATH="$QTDIR/Tools/CMake/bin:$PATH"

cmake -B "build/$APPNAME" -S "$APPNAME" \
    -DCMAKE_PREFIX_PATH="$QTDIR/$QTVERSION/gcc_64" \
    -DCMAKE_INSTALL_BINDIR="_installer_" \
    -DCMAKE_INSTALL_PREFIX="build/$INSTALLDIR"

if [ $? -ne 0 ]; then
    echo "***!!!*** $APPNAME configuration failed"
    exit 1
fi

cmake --build "build/$APPNAME" --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "***!!!*** $APPNAME build failed"
    exit 1
fi

########### FOR TEST PURPOSES ###########
# Add the app conf file
sed "s/%VERSION%/$APPVERSION/g;s/%APPNAME%/$APPNAME/g" "app.conf" > "build/$INSTALLDIR/_installer_/app.conf"
# Add the "desktop" file
mkdir -p "build/$INSTALLDIR/share/applications"
cp "$APPNAME.desktop" "build/$INSTALLDIR/share/applications"

