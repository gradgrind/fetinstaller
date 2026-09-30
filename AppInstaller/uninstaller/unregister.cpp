#include "uninstaller.h"
#include <QProcess>
#include <QStandardPaths>

// Update file-type associations and desktop files.
// Windows: remove app from registry.
// unregisterApp() will only be called if the installation was "registered", which the
// installer only does if the installation is to the default location.
// It is assumed that installations to other locations will not have set up file-type
// associations and desktop menu entries.

#if defined Q_OS_WIN

void Uninstaller::unregisterApp()
{
    //TODO: Write to registry

    /*TODO
    ; The RefreshShellIcons functions allow the association of the
        ; icons with the file type to be changed immediately.

        !define SHCNE_ASSOCCHANGED 0x08000000
        !define SHCNF_IDLIST 0

        Function RefreshShellIcons
        ; By jerome tremblay - april 2003
        System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
            (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
        FunctionEnd
    */
}

#elif defined Q_OS_LINUX

void Uninstaller::unregisterApp()
{
    print_line("");

    // Remove system files from ~/.local.
    // Read the list of installed files
    QString installed_files_path{appinfo->basedir.filePath("system_files")};
    QFile textFile{installed_files_path};
    if ( !textFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        print_line(tr("App installed no system files"));
        return;
    }
    // Read file line by line
    QTextStream textStream(&textFile);
    while ( true )
    {
        QString line = textStream.readLine();
        if ( line.isNull() )
            break;    // end of file
        QString fpath{line.trimmed()};
        if ( !fpath.isEmpty() ) {
            if ( QFile::remove(fpath) ) {
                print_line(tr("Removed %1").arg(fpath));
            } else {
                print_line(tr("Couldn't remove %1").arg(fpath), true);
            }
        }
    }

    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    print_line(tr("Update icon cache"));
    QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
    print_line(tr("Update mime database"));
    QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
    print_line(tr("Update desktop database"));
    QProcess::execute("update-desktop-database", QStringList{appsDir.path()});
}

#else

//TODO

#endif
