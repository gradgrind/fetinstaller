#!/bin/bash

# Build FET, fet_install and fet_uninstall, placing the installation bundle
# build/install.

# This build script uses a standard Qt installation set up by the Qt online
# installer.

# Place the fetinstaller folder in the root folder of the FET source code.
# Then run this script in the fetinstaller source (root) directory.

QTVERSION="6.11.2"
QTDIR="$HOME/Qt"

export PATH=$QTDIR/Tools/CMake/bin:$PATH

# Build FET, assuming it is in the parent directory
cmake -B build/FET -S .. -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/install

cmake --build build/FET --target install --parallel 6

# Build the installer
cmake -B build/installer -S installer -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/install

cmake --build build/installer --target install --parallel 6

# Build the uninstaller
cmake -B build/uninstaller -S uninstaller -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/install

cmake --build build/uninstaller --target install --parallel 6

