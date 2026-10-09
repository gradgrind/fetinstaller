#include "installer.h"

static const char *REL_OUTSIDE = QT_TRANSLATE_NOOP(
    "SYMLINKS",
    "ERROR, relative symlink outside package: %1 -> %2");

static const char *REL_NO_TARGET = QT_TRANSLATE_NOOP(
    "SYMLINKS",
    "ERROR, target missing for relative symlink: %1 -> %2");

static const char *ABS_LINK = QT_TRANSLATE_NOOP(
    "SYMLINKS",
    "WARNING, absolute symlink: %1 -> %2");

static const char *WINDOWS_SYMLINK = QT_TRANSLATE_NOOP(
    "SYMLINKS",
    "ERROR, Windows symlinks are not supported: %1");

static const char *WINDOWS_SHORTCUT = QT_TRANSLATE_NOOP(
    "SYMLINKS",
    "ERROR, Windows shortcuts are not supported: %1");

#ifdef Q_OS_WIN

// On Windows there are symlinks and "shortcuts" (.lnk).
// At present symlinks are not permitted – one difficulty is that
// they normally need administrator privileges for creation.
// Windows shortcuts may only be absolute and are not permitted here.

linktest Installer::testLink(QString rpath)
{
    const QFileInfo finfo{src_dir.filePath(rpath)};
    if ( finfo.isShortcut() ) {
        return {tr(WINDOWS_SHORTCUT).arg(rpath)};
    } else if ( finfo.isSymLink() ) {
        return {tr(WINDOWS_SYMLINK).arg(rpath)};
    }
    return {};
}

#else

linktest Installer::testLink(QString rpath)
{
    const QFileInfo finfo{src_dir.filePath(rpath)};
    if ( !finfo.isSymLink() )
        return {};

    QString linkPath{finfo.readSymLink()}; // target path, relative or absolute
    bool linkTargetExists = finfo.exists();
    if ( QFileInfo(linkPath).isRelative() ) {
        // A relative link within the install package is acceptable, as long as its target exists.
        // A relative link outside the package is an error.
        QString lrpath = src_dir.relativeFilePath(finfo.symLinkTarget());
        if ( lrpath.startsWith("..") ) {
            // outside the package
            return {tr(REL_OUTSIDE).arg(rpath, linkPath)};
        } else {
            // within the package
            if ( linkTargetExists ) {
                return {{}, rpath, linkPath};
            } else {
                return {tr(REL_NO_TARGET).arg(rpath, linkPath)};
            }
        }
    } else {
        // Absolute links are not acceptable.
        return {tr(ABS_LINK).arg(rpath, linkPath), rpath, linkPath};
    }
}

#endif
