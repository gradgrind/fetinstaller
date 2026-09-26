#include "appinfo.h"
#include <QCoreApplication>
#include <QMessageBox>
#include <QTemporaryDir>
#include <QProcess>

static const char* NO_FILE_LIST = QT_TRANSLATE_NOOP("AppInfo", R"(
Couldn't read list of installed files:

base directory: %1

"installed_files" path: %2
)");


#if defined Q_OS_WIN

AppInfo::AppInfo()
    : APPNAME{"FET"}
    , APPEXEC{"fet"}
    , EXECDIR{""}
    , APPFILES{""}
    {}

#else

AppInfo::AppInfo()
    : APPNAME{"FET"}
    , APPEXEC{"fet"}
    , EXECDIR{"bin/"}
    , APPFILES{"share/" + APPEXEC + "/"}
{}

#endif

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
    QDir xdir{EXECDIR};
    while ( true ) {
        if ( xdir.path() == "." )
            break;
        appdir.cdUp();
        if ( !xdir.cdUp() ) {
            QMessageBox::critical(nullptr, tr("Critical Error"), tr("Couldn't find uninstaller base folder"));
            return false;
        }
    }

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

    // Default installation path (Windows)
    defaultInstallationPath = QDir::home().absoluteFilePath("AppData/Local/Programs/%1").arg(APPNAME);

#elif defined Q_OS_LINUX

    // Default installation path (Linux)
    defaultInstallationPath = QDir::home().absoluteFilePath(".local/apps/%1").arg(APPNAME);

#endif

    registered = ( basedir == defaultInstallationPath );
    return true;
}
