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

    // Remove files from ~/.local
    QString mylocal{QDir::home().absoluteFilePath(".local")};

    // Remove relevant symlinks in ~/.local/bin
    for ( const auto &dirEntry : QDirListing(
             mylocal + "/bin",
             QDirListing::IteratorFlag::IncludeHidden) ) {
        if ( dirEntry.isSymLink() ) {
            if ( !appinfo->basedir.relativeFilePath(dirEntry.canonicalFilePath()).startsWith("..") ) {
                // a link to a file within the installation
                QFile::remove(dirEntry.filePath());
            }
        }
    }

    // In case a desktop link is present:
    QProcess::execute(
        "xdg-desktop-icon",
        QStringList() << "uninstall" << appinfo->APPFILENAME + ".desktop");
    // alternative: use QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)

    // Remove from "start" menu
    QProcess::execute(
        "xdg-desktop-menu",
        QStringList() << "uninstall" << appinfo->APPFILENAME + ".desktop");
    // alternative: use QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)

    // Remove mime-type
    QProcess::execute(
        "xdg-mime",
        QStringList() << "uninstall" << mylocal + "/share/mime/packages/" + appinfo->APPFILENAME + ".xml");
    // alternative: use QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/mime/packages/"

    // Remove icon(s) ... ???
    QProcess::execute("xdg-icon-resource",
                      QStringList() << "uninstall" << appinfo->APPFILENAME << "--size" << "128" << "--context" << "apps");
    // ??? It's not clear that the "mimetypes" icons are at all necessary ... what do they do?
    // On Cinnamon apparently nothing, on GNOME the "apps" icons are enough for the .fet files to get the fet icon
    // in the file browser.
    QProcess::execute("xdg-icon-resource",
                      QStringList() << "uninstall" << appinfo->APPFILENAME << "--size" << "128" << "--context" << "mimetypes");
    // Maybe copy the svg and 1 png (just in case ...) to the apps folders?
    // alternative: use QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons/hicolor/scalable/"
    //                  and/or + "/icons/hicolor/s128x128/" (for example)
    // Then do "xdg-icon-resource forceupdate".



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