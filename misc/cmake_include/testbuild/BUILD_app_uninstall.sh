#!/bin/bash

APPNAME="app_uninstall"
APPVERSION="1.0.0"

# If necessary, it will change its working directory to run in the
# directory containing this script.

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR

ROOTDIR=".."
. "$ROOTDIR/common_qt.sh"
. "$ROOTDIR/common_dirs.sh"
. "$ROOTDIR/common_build.sh"

########### FOR TEST PURPOSES ###########
QTLANG=de "$BUILDDIR/$INSTALLDIR/bin/app_uninstall"
