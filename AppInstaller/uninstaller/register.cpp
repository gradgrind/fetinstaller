#include "uninstaller.h"
#include <QProcess>

// Update file-type associations and desktop files.
// Windows: remove app from registry.

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
    // Only perform these operations if the installation is at the default location.
    // It is assumed that installations to other locations will not have set up file-type
    // associations and desktop menu entries.

    print_line("");

    //TODO: Remove files from ~/.local, remove symlinks

    QFile::remove(QDir::home().absoluteFilePath(".local/bin/" + appinfo->APPEXEC));
    QFile::remove(QDir::home().absoluteFilePath(".local/share/applications/" + appinfo->APPEXEC + ".desktop")); // ???
    QProcess::execute("xdg-icon-resource",
                      QStringList() << "uninstall" << appinfo->APPEXEC << "--size" << "128" << "--context" << "apps");
    // ???
    QProcess::execute("xdg-icon-resource",
                      QStringList() << "uninstall" << appinfo->APPEXEC << "--size" << "128" << "--context" << "mimetypes");

    print_line("update-mime-database");
    QProcess::execute("update-mime-database",
        QStringList() << appinfo->basedir.absoluteFilePath("share/mime"));
    print_line("update-desktop-database");
    QProcess::execute("update-desktop-database",
        QStringList() << appinfo->basedir.absoluteFilePath("share/applications"));

}

#else

//TODO

#endif