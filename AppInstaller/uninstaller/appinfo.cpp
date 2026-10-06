#include "appinfo.h"
#include <QCoreApplication>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QProcess>

AppInfo::AppInfo() {}

bool AppInfo::init()
{
    QStringList args = QCoreApplication::arguments();

    // Windows: With command-line switch -c, copy a subset of the files to a temporary
    // directory, passed as command-line argument.
    // With command-line switch -u, use the path passed as command-line argument
    // instead of the root path of the uninstall executable as the installation
    // root.

    // Determine the installation's base directory
    appdir.setPath(QCoreApplication::applicationDirPath());
    appdir.makeAbsolute();
#if defined Q_OS_WIN
    // The uninstaller is expected in the root directory of the installation.
#else
    // The uninstaller is expected in the "bin" directory of the installation.
    appdir.cdUp();
#endif

    //TODO: Check validity of root directory, if possible!

    //QMessageBox::critical(nullptr, tr("Critical Error"), tr("Couldn't find uninstaller base folder"));
    //return false;

#if defined Q_OS_WIN
    if ( args.length() == 3 ) {
        if ( args.at(1) == "-c" ) {
            basedir = appdir;
            appcopy = args.at(2);
        } else if ( args.at(1) == "-u" ) {
            basedir.setPath(args.at(2));
        } else
            goto err;
    }
#else
    if ( args.length() == 1 ) {
        basedir = appdir;
    }
#endif
    else {
err:
        QMessageBox::critical(
            nullptr,
            tr("Critical Error"),
            tr("Invalid command line:\n  - %1").arg(args.join("\n  - ")));
        return false;
    }

#if defined Q_OS_WIN

    // On Windows the uninstaller must be run from a temporary folder because it can't delete
    // the files belonging to the running process.

    if ( !appcopy.isEmpty() ) {
        // Copy the necessary files to a temporary directory so that the uninstaller can be run from there.
        QDir tdir{appcopy};
        for ( const auto &dirEntry : QDirListing(basedir.path(), QDirListing::IteratorFlag::FilesOnly) ) {
            // (faster than using name filters)
            const QString fileName = dirEntry.fileName();
            if ( fileName.endsWith(".dll") || fileName == "qt.conf" || fileName == "app_uninstall.exe") ) {
                // In the case of a link this copies the target file, on Windows there are unlikely
                // to be links among the files copied here (on Linux there would almost certainly
                // be links):
                if ( !QFile::copy(dirEntry.filePath(), tdir.filePath(fileName)) ) {
                    QMessageBox::critical(
                        nullptr,
                        tr("Critical Error"),
                        (tr("Copying uninstaller to temporary folder failed:")
                         + "\n  %1 -> %2")
                            .arg(dirEntry.filePath(), tdir.filePath(fileName)));
                    return false;
                }
            }
        }
    }

#endif

    basename = basedir.dirName();
    return true;
}
