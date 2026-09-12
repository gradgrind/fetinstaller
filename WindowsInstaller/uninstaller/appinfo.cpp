#include "appinfo.h"
#include <QCoreApplication>
#include <QMessageBox>

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
    // Find the installation's base directory
    basedir.setPath(QCoreApplication::applicationDirPath());
    basedir.makeAbsolute();
    QDir xdir{EXECDIR};
    while ( true ) {
        if ( xdir.path() == "." )
            break;
        basedir.cdUp();
        if ( !xdir.cdUp() ) {
            QMessageBox::critical(nullptr, tr("Critical Error"), tr("Couldn't find installation base folder"));
            return false;
        }
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

    // On Windows the uninstaller must be run from a temporary folder.
#if defined Q_OS_WIN
#endif

    return true;
}
