#include "appinfo.h"
#include <QCoreApplication>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QProcess>
#include <QRandomGenerator>

#if defined Q_OS_WIN
#include <process.h>
#endif

AppInfo::AppInfo() {}

bool AppInfo::init()
{
    // If there is a command-line argument it must be the address of the installation to remove.
    // This is only supported on Windows, where a program cannot delete its own files.
    // On Windows "app_uninstall0.exe" will copy the uninstaller, together with necessary
    // library and plugin files, to a temporary directory. It will then run "app_uninstall.exe"
    // in the temporary directory, passing the path to the installation to be removed as argument.
    // The switch is done using the C function "_execl" so that the installer replaces the
    // pre-installer.

    // Determine the installation's base directory, starting with the base directory of the
    // uninstaller (this program): "appdir".
    appdir.setPath(QFileInfo(QCoreApplication::applicationFilePath()).canonicalPath());
#if defined Q_OS_WIN
    // The uninstaller is expected to be in the root directory of the installation.
#else
    // The uninstaller is expected to be in the "bin" directory of the installation.
    appdir.cdUp();
#endif

    QStringList args = QCoreApplication::arguments();
    if ( args.length() == 1 ) {
#if defined Q_OS_WIN
        // Get target
        QString target{QFile::symLinkTarget(appdir.filePath("UNINSTALL_TARGET.lnk"))};
        if ( target.isEmpty() ) {
            error = tr("Missing link: '%1'").arg(appdir.filePath("UNINSTALL_TARGET.lnk"));
            return false;
        } else {
            // A simple test for an application folder:
            if ( QFileInfo::exists(target + "/app_uninstall.exe") ) {
                basedir.setPath(target);
            } else {
                error = tr("Invalid application folder: '%1'").arg(target);
                return false;
            }
        }
#else
        basedir = appdir; // The uninstaller should delete itself.
#endif
    } else {
        error = tr("Invalid command line:\n  - %1").arg(args.join("\n  - "));
        return false;
    }
    basename = basedir.dirName();
    return true;
}

bool AppInfo::copyfile(QString frel)
// Copy from appdir to basedir, the file has a relative path.
{
    QString dst{basedir.absoluteFilePath(frel)};
    // Ensure the oarent directory exists.
    QFileInfo dinfo{QFileInfo{dst}.absolutePath()};
    if ( !dinfo.isDir() ) {
        basedir.mkpath(dinfo.absoluteFilePath());
    }
    if ( !QFile::copy(appdir.absoluteFilePath(frel), dst) ) {
        error = tr("Failed: copying file to temporary folder: '%1'").arg(dst);
        return false;
    }
    return true;
}

bool AppInfo::uninstaller2tmp()
{
    // Create a temporary directory.
    QDir tmpdir0{QDir::temp()};
    QString tmprel{QString{"AppUnIn%1"}.arg(QRandomGenerator::global()->generate())};
    if ( !tmpdir0.mkdir(tmprel) ) {
        error = QCoreApplication::translate(
                    "Uninstall0", "Failed: create temporary folder: '%1'")
                    .arg(tmpdir0.absoluteFilePath(tmprel));
        return false;
    }
    basedir.setPath(tmpdir0.absoluteFilePath(tmprel));
    // Copy the uninstaller and supporting libraries to the temporary directory.
    if ( !copyfile("app_install.exe") ) {
        return false;
    }
    if ( !copyfile("qt.conf") ) {
        return false;
    }
    for ( const auto &dirEntry : QDirListing(
             appdir.absolutePath(),
             QDirListing::IteratorFlag::FilesOnly ) ) {
        // Only regular files ...
        const QString fileName = dirEntry.fileName();
        if ( fileName.endsWith(".dll") ) {
            if ( !copyfile(fileName) ) {
                return false;
            }
        }
    }
    // Add plugins
    for ( const auto &dirEntry : QDirListing(
             appdir.absoluteFilePath("plugins"),
             QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::Recursive) ) {
        // Only regular files ...
        if ( !copyfile(appdir.relativeFilePath(dirEntry.absoluteFilePath())) ) {
            return false;
        }
    }
    return true;
}
