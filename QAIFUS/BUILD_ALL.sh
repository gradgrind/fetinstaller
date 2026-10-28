#!/bin/bash

# Build the application itself, app_install and app_uninstall, placing the
# installation bundle in "build/app_bundle".

# The app bundle is then compressed to "app_bundle.tar.xz", which is
# included in the installer-starter from "appin". The resulting
# executable is named '$APPLICATION_NAME-$APPLICATION_VERSION.run'.

# This build script uses a standard Qt installation set up by the
# Qt online installer.
# For the last step, the construction of the self-extracting installer,
# the "musl" C library and its compiler wrapper "musl-gcc" are required.

# Place the "QAIFUS" folder and a suitable "QAIFUS_$APPLICATION_NAME" folder
# in the root folder of the application's source code.
# Then run this script, e.g.:
#    ./BUILD_ALL.sh $APPLICATION_NAME

# If necessary, the working directory will be changed to run in the directory
# containing this script.

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR
. "common_base.sh"

# Read the application name and include its build script
APPLICATION_NAME="$1"
if [ -z "$APPLICATION_NAME" ]; then
    echo "Application name missing, use: ./$(basename "$SCRIPT") APPLICATION_NAME"
    exit 1
fi
APPDIR=".."
QAIFUS_APPDIR=$(readlink -f "$APPDIR/QAIFUS_$APPLICATION_NAME")
if [ -f "$QAIFUS_APPDIR/BUILD.sh" ]; then
    APPNAME="$APPLICATION_NAME"
    . "$QAIFUS_APPDIR/BUILD.sh"
else
    echo "File not found: $QAIFUS_APPDIR/BUILD.sh"
    exit 1
fi

echo
echo "Build the installer components ..."
echo

APPNAME="app_install"
APPDIR="$APPNAME"
. "common_build.sh"

APPNAME="app_uninstall"
APPDIR="$APPNAME"
. "common_build.sh"

# Ensure that the "_installer_" directory exists
installerdir="$BUILDDIR/$INSTALLDIR/_installer_"
mkdir -p "$installerdir"
# Add the installer app-configuration file
sed "s/%VERSION%/$APPLICATION_VERSION/g;s/%APPNAME%/$APPLICATION_NAME/g" \
    "$QAIFUS_APPDIR/app.conf" > "$installerdir/app.conf"

echo
echo "Create app bundle archive: build/app_bundle.tar.xz"
echo

cd "$BUILDDIR/$INSTALLDIR"
tar -cJf ../app_bundle.tar.xz *

if [ $? -ne 0 ]; then
    echo "***!!!*** app bundle compression failed"
    exit 1
fi

echo
echo "Build app installer: $APPLICATION_NAME-$APPLICATION_VERSION.run"
echo

cd ../../appin
musl-gcc -static -o "../$APPLICATION_NAME-$APPLICATION_VERSION.run" appin.c

if [ $? -ne 0 ]; then
    echo "***!!!*** final (appin) packaging failed"
    exit 1
fi

