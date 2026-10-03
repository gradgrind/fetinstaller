#!/bin/bash

APPNAME="FET"
APPVERSION=$(<../VERSION)

# Build the application itself, app_install and app_uninstall,
# placing the installation bundle in 'build/$APPNAME-$APPVERSION'.

# This build script uses a standard Qt installation set up by the
# Qt online installer.

# Place the "AppInstaller" folder in the root folder of the application's
# source code.
# Then run this script. It will run in the "AppInstaller" directory.

QTVERSION="6.11.2"
QTDIR="$HOME/Qt"

export PATH=$QTDIR/Tools/CMake/bin:$PATH

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR

# Build app, assuming it is in the parent directory
cmake -B build/$APPNAME -S .. -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/$APPNAME-$APPVERSION

if [ $? -ne 0 ]; then
    echo "ABORTING 1"
    exit 1
fi

cmake --build build/$APPNAME --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "ABORTING 2"
    exit 1
fi

# Build the installer
cmake -B build/installer -S installer -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/$APPNAME-$APPVERSION

if [ $? -ne 0 ]; then
    echo "ABORTING 3"
    exit 1
fi

cmake --build build/installer --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "ABORTING 4"
    exit 1
fi

# Build the uninstaller
cmake -B build/uninstaller -S uninstaller -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/$APPNAME-$APPVERSION

if [ $? -ne 0 ]; then
    echo "ABORTING 5"
    exit 1
fi

cmake --build build/uninstaller --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "ABORTING 6"
    exit 1
fi

makeself --xz --nox11 build/install "build/$APPNAME-$APPVERSION.run" "$APPNAME installer" ./_installer_/app_install
