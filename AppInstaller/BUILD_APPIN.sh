#!/bin/bash

APPNAME="FET"
APPVERSION=$(<../VERSION)

# Build the application itself, app_install and app_uninstall,
# placing the installation bundle in "build/app_bundle".

# The app bundle is then compressed to "app_bundle.tar.xz", which is
# included in the installer starter from "appin". The resulting
# executable is named '$APPNAME-$APPVERSION.run'.

# This build script uses a standard Qt installation set up by the
# Qt online installer.
# For the last step, the construction of the self-extracting installer,
# the "musl" C library and its compiler wrapper "musl-gcc" are required.

# Place the "AppInstaller" folder in the root folder of the application's
# source code.
# Then run this script. It will change its working directory to run in
# the "AppInstaller" directory.

QTVERSION="6.11.2"
QTDIR="$HOME/Qt"

export PATH=$QTDIR/Tools/CMake/bin:$PATH

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR

# Build app, assuming it is in the parent directory
cmake -B build/$APPNAME -S .. -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/app_bundle

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
cmake -B build/installer -S installer -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/app_bundle

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
cmake -B build/uninstaller -S uninstaller -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/app_bundle

if [ $? -ne 0 ]; then
    echo "ABORTING 5"
    exit 1
fi

# Add the installer configuration file
mkdir -p build/app_bundle/_installer_
sed "s/%VERSION%/$APPVERSION/g" app.conf > build/app_bundle/_installer_/app.conf

cmake --build build/uninstaller --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "ABORTING 6"
    exit 1
fi

cd build/app_bundle
tar -cJf ../app_bundle.tar.xz

if [ $? -ne 0 ]; then
    echo "ABORTING 7"
    exit 1
fi

cd ../../appin
musl-gcc -static -o "../$APPNAME-$APPVERSION.run" appin.c

