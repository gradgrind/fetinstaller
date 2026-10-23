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
    // On Windows a call with no argument will copy the uninstaller, together with necessary
    // library and plugin files, to a temporary directory. It will then run "itself" again, this
    // time with an argument, the path to the installation to be removed, using the C function
    // "_execl" to switch fully to the new instance.

    // Determine the installation's base directory
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
        // Copy uninstaller to temporary directory and run from there.
        if ( uninstaller2tmp() ) {
            // The uninstaller executable should be in the root directory of the copy, which
            // should be in basedir.
            // The uninstaller is expected to be in the "bin" directory of the installation.
            QByteArray cmdbytes{basedir.absoluteFilePath("/app_uninstall.exe").toLocal8Bit()};
            const char* cmd = cmdbytes.constData();
            QByteArray arg1{appdir.absolutePath().toLocal8Bit()};
            _execl(cmd, cmd, arg1.constData(), (char*) NULL);
            // Doesn't return!
            error = QString{"Failed: switch to uninstaller in '%1'"}.arg(basedir.absolutePath());
        }
        // Remove temporary dictionary.
        if ( basedir != QDir{} && !basedir.removeRecursively() ) {
            QMessageBox::critical(
                nullptr,
                QCoreApplication::translate("Uninstall0", "Critical Error"),
                QCoreApplication::translate("Uninstall0", "Failed: Remove temporary folder: '%1'").arg(tmppath));
        }
        return false;
    } else if ( args.length() == 2 ) {
        QDir xdir{args.at(1)};
        if ( xdir.exists("app_uninstall.exe") ) {
            basedir = xdir;
        } else {
            error = tr("No uninstaller found in '%1'").arg(appdir.absolutePath());
            return false;
        }
#else
        basedir = appdir;
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
