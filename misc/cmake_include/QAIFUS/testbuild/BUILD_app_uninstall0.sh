#!/bin/bash
#NOTE: This is just here for "completeness". It can't work because this app
# is for Windows only.
APPNAME="app_uninstall0"

# If necessary, it will change its working directory to run in the
# directory containing this script.

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR

APPDIR="../$APPNAME"
. "../common_base.sh"
. "../common_build.sh"

########### FOR TEST PURPOSES ###########
QTLANG=de "$BUILDDIR/$INSTALLDIR/bin/$APPNAME"
