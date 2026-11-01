#!/bin/bash

PROJECT_VERSION=$(<../VERSION)

PROJECTNAME="AppInstall"
APPNAME="app_install"

# If necessary, it will change its working directory to run in the
# directory containing this script.

SCRIPT=$(readlink -f "$0")
BASEDIR=$(dirname "$SCRIPT")
cd $BASEDIR

APPDIR="../$APPNAME"
. "../common_base.sh"
. "../common_build.sh"

########### FOR TEST PURPOSES ###########
# Add the app conf file
sed "s/%VERSION%/$PROJECT_VERSION/g;s/%PROJECT%/$PROJECTNAME/g" "app.conf" > \
    "$BUILDDIR/$INSTALLDIR/_installer_/app.conf"
# Add the "desktop" file
mkdir -p "$BUILDDIR/$INSTALLDIR/share/applications"
cp "$APPNAME.desktop" "$BUILDDIR/$INSTALLDIR/share/applications"

QTLANG=de "$BUILDDIR/$INSTALLDIR/_installer_/$APPNAME"
