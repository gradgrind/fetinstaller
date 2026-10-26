#!/bin/bash

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

QTDIR="$HOME/Qt"
# Find newest available Qt version, but exclude 6.12.0, which is broken.
# The lowest acceptable version is 6.10.
QTVERSION=$(ls -r "$QTDIR" | grep -v 6[.]12[.]0 | grep -m 1 ^6[.]1[0-9][.])
if [ -z "$QTVERSION" ]; then
    echo "No suitable Qt installation found in '$QTDIR'"
    exit 1
fi
#QTVERSION="6.11.2"
echo "Using '$QTDIR/$QTVERSION'"
echo

export PATH=$QTDIR/Tools/CMake/bin:$PATH

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR

install_prefix="app_bundle"

# Build app, assuming it is in the parent directory
echo "Starting build process ..."
echo

APPNAME="$1"
if [ -z "$APPNAME" ]; then
    echo "Application name missing, use: ./$(basename "$SCRIPT") APPNAME"
    exit 1
fi
APPDIR=$(readlink -f "../QAIFUS_$APPNAME")
if [ -f "$APPDIR/BUILD.sh" ]; then
    . "$APPDIR/BUILD.sh"
else
    echo "File not found: $APPDIR/BUILD.sh"
    exit 1
fi

echo
# Build the installer
echo "Build the installer components ..."
echo

cmake -B "build/QAIFUS" -DCMAKE_PREFIX_PATH="$QTDIR/$QTVERSION/gcc_64" -DCMAKE_INSTALL_PREFIX="build/$install_prefix"

if [ $? -ne 0 ]; then
    echo "***!!!*** installer components' configuration failed"
    exit 1
fi

cmake --build "build/QAIFUS" --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "***!!!*** installer components build failed"
    exit 1
fi

# Ensure that the "_installer_" directory exists
mkdir -p "build/$install_prefix/_installer_"
# Move the installer executable and copy qt.conf
mv "build/$install_prefix/bin/app_install" "build/$install_prefix/_installer_"
cp -p "build/$install_prefix/bin/qt.conf" "build/$install_prefix/_installer_"
# Add the installer app-configuration file
sed "s/%VERSION%/$APPVERSION/g;s/%APPNAME%/$APPNAME/g" "$APPDIR/app.conf" > "build/$install_prefix/_installer_/app.conf"

echo
echo "Create app bundle archive: build/app_bundle.tar.xz"
echo

cd "build/$install_prefix"
tar -cJf ../app_bundle.tar.xz *

if [ $? -ne 0 ]; then
    echo "***!!!*** app bundle compression failed"
    exit 1
fi

echo
echo "Build app installer: $APPNAME-$APPVERSION.run"
echo

cd ../../appin
musl-gcc -static -o "../$APPNAME-$APPVERSION.run" appin.c

if [ $? -ne 0 ]; then
    echo "***!!!*** final (appin) packaging failed"
    exit 1
fi

