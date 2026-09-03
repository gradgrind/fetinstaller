#!/bin/bash

# Build FET, fet_install and fet_uninstall, placing the installation bundle
# in build/install.

# This build script uses a standard Qt installation set up by the Qt online
# installer.

# Place the fetinstaller folder in the root folder of the FET source code.
# Then run this script in the fetinstaller source (root) directory.

QTVERSION="6.11.2"
QTDIR="$HOME/Qt"

export PATH=$QTDIR/Tools/CMake/bin:$PATH

# Build FET, assuming it is in the parent directory
cmake -B build/FET -S .. -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/install

if [ $? -ne 0 ]; then
    echo "ABORTING 1"
    exit 1
fi

cmake --build build/FET --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "ABORTING 2"
    exit 1
fi

# Build the installer
cmake -B build/installer -S installer -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/install

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
cmake -B build/uninstaller -S uninstaller -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/install

if [ $? -ne 0 ]; then
    echo "ABORTING 5"
    exit 1
fi

cmake --build build/uninstaller --target install --parallel 6

if [ $? -ne 0 ]; then
    echo "ABORTING 6"
    exit 1
fi

version=$(<../VERSION)

makeself --xz --nox11 build/install "build/fet-$version.run" "FET installer" ./_bin/fet_install
