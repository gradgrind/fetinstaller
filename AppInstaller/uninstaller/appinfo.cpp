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

    // Default installation path
#if defined Q_OS_WIN
    defaultInstallationPath = QDir::home().absoluteFilePath("AppData/Local/Programs/%1").arg(APPNAME);
#else
    defaultInstallationPath = QDir::home().absoluteFilePath(".local");
#endif

    // Read the list of installed files
    installed_files_path = basedir.filePath(APPFILES + "installed_files");
    QFile textFile{installed_files_path};
    //print_line(tr("Reading file list from: %1").arg(filespath));
    //print_line("");
    if ( !textFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        QMessageBox::critical(
            nullptr,
            tr("Critical Error"),
            tr(NO_FILE_LIST).arg(basedir.path(), installed_files_path));
        return false;
    }
    // Read file line by line
    QTextStream textStream(&textFile);
    installed_files.clear();
    while ( true )
    {
        QString line = textStream.readLine();
        if ( line.isNull() )
            break;    // end of file
        QString rpath{line.trimmed()};
        if ( !rpath.isEmpty() )
            installed_files.append(rpath);
    }

    // On Windows the uninstaller must be run from a temporary folder because it can't delete
    // the files belonging to the running process.

#if defined Q_OS_WIN

    if ( !appcopy.isEmpty() ) {
        // Copy the necessary files to a temporary directory and run the uninstaller from there.
        QDir tdir{appcopy};
        for ( const auto& fpath: std::as_const(installed_files) ) {
            QFileInfo finfo{fpath};
            QString fname{finfo.fileName()};
            if ( finfo.suffix() == "dll"             // specifically for Windows
                     || fname == "qt.conf"
                     || finfo.baseName().endsWith("_uninstall") ) {

                QString drel{finfo.path()};
                if ( !tdir.exists(drel) )
                    tdir.mkpath(drel);
                // In the case of a link this copies the target file, on Windows there are unlikely
                // to be links among the files copied here (on Linux there would almost ceretainly
                // be links):
                if ( !QFile::copy(basedir.filePath(fpath), tdir.filePath(fpath)) ) {
                    QMessageBox::critical(
                        nullptr,
                        tr("Critical Error"),
                        (tr("Copying uninstaller to temporary folder failed:")
                            + "\n  %1 -> %2\n  +++ %3")
                            .arg(basedir.filePath(fpath), tdir.filePath(fpath), drel));
                    return false;
                }
            }
        }
    }

#endif

    return true;
}
