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
    // Remove system files from ~/.local.
    // Read the list of installed files
    QString installed_files_path{basedir.filePath("system_files")};
    QFile textFile{installed_files_path};
    if ( !textFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        print_line(tr("App installed no system files"));
        print_line("");
        return;
    }
    // Read file line by line
    QTextStream in(&textFile);
    QStringList newlines;
    bool appregistered{false};
    while ( !in.atEnd() )
    {
        QString line = in.readLine();
        qDebug() << "???" << line;
        QString fpath{line.trimmed()};
        if ( !fpath.isEmpty() ) {
            if ( fpath.startsWith("+++") ) {
                appregistered = true;
            } else if ( QFile::remove(fpath) ) {
                print_line(tr("Removed %1").arg(fpath));
            } else {
                unregister_failed.append(fpath);
            }
        }
    }
    textFile.close();

    if ( appregistered ) {
        QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
        print_line(tr("Update icon cache"));
        QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
        print_line(tr("Update mime database"));
        QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
    }
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    print_line(tr("Update desktop database"));
    QProcess::execute("update-desktop-database", QStringList{appsDir.path()});

    print_line("");
}

#else

//TODO

#endif
