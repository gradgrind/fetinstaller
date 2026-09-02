#!/bin/bash

# Build script using a standard Qt installation set up by the Qt online installer.
# Run this in the fetinstaller source (root) directory.

QTVERSION="6.11.2"
QTDIR="$HOME/Qt"

export PATH=$QTDIR/Tools/CMake/bin:$PATH

cmake -B build/installer -S installer -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/install

cmake --build build/installer --target install --parallel 6

cmake -B build/uninstaller -S uninstaller -DCMAKE_PREFIX_PATH=$QTDIR/$QTVERSION/gcc_64 -DCMAKE_INSTALL_PREFIX=build/install

cmake --build build/uninstaller --target install --parallel 6

