BUILDDIR="build"
INSTALLDIR="app_bundle"

### This build script uses a standard Qt installation set up by the
### Qt online installer.

QTDIR="$HOME/Qt"
# Find newest available Qt version, but exclude 6.12.0, which is broken.
# The lowest acceptable version is 6.10.
QTVERSION=$(ls -r "$QTDIR" | grep -v 6[.]12[.]0 | grep -m 1 ^6[.]1[0-9][.])
if [ -z "$QTVERSION" ]; then
    echo "No suitable Qt installation found in '$QTDIR'"
    exit 1
fi
#QTVERSION="6.12.0"
echo "Using '$QTDIR/$QTVERSION'"
echo

export PATH="$QTDIR/Tools/CMake/bin:$PATH"
