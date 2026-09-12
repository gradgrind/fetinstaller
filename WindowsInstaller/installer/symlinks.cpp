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

static const char *ABS_NO_TARGET = QT_TRANSLATE_NOOP(
    "SYMLINKS",
    "WARNING, absolute symlink: %1 -> %2 (doesn't exist!)");

static const char *ABS_WITHIN = QT_TRANSLATE_NOOP(
    "SYMLINKS",
    "ERROR, absolute symlink within package: %1 -> %2");

static const char *WINDOWS_SYMLINK = QT_TRANSLATE_NOOP(
    "SYMLINKS",
    "ERROR, Windows symlinks are not supported: %1");

#ifdef Q_OS_WIN

// On Windows there are symlinks and "shortcuts" (.lnk).
// At present symlinks are not permitted – one difficulty is that
// they normally need administrator piveleges for creation.
// Windows shortcuts can be created by QFile::link.
// It looks like only absolute shortcuts are possible.

linktest Installer::testLink(QString rpath)
{
    const QFileInfo finfo{src_dir.filePath(rpath)};
    if ( !finfo.isShortcut() ) {
        if ( finfo.isSymLink() ) {
            return {tr(WINDOWS_SYMLINK).arg(rpath)};
        }
        return {};
    }

    QString linkPath{finfo.symLinkTarget()}; // target path, absolute only
    bool linkTargetExists = finfo.exists();

    // An absolute link within the install package is an error.
    // An absolute link outside the package will be accepted, but a warning will be issued.
    if ( src_dir.relativeFilePath(linkPath).startsWith("..") ) {
        // outside the package
        QString x;
        if ( linkTargetExists ) {
            return {tr(ABS_LINK).arg(rpath, linkPath), rpath, linkPath};
        }
        return {tr(ABS_NO_TARGET).arg(rpath, linkPath), rpath, linkPath};
    } else {
        // inside the package
        return {tr(ABS_WITHIN).arg(rpath, linkPath)};
    }
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
        // An absolute link within the install package is an error.
        // An absolute link outside the package will be accepted, but a warning will be issued.
        if ( src_dir.relativeFilePath(linkPath).startsWith("..") ) {
            // outside the package
            QString x;
            if ( linkTargetExists ) {
                return {tr(ABS_LINK).arg(rpath, linkPath), rpath, linkPath};
            }
            return {tr(ABS_NO_TARGET).arg(rpath, linkPath), rpath, linkPath};
        } else {
            // inside the package
            return {tr(ABS_WITHIN).arg(rpath, linkPath)};
        }
    }
}

#endif
