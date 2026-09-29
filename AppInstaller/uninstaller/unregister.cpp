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

    //+++ In case a desktop link is present:
    // "xdg" version:
    //QProcess::execute(
    //    "xdg-desktop-icon",
    //    QStringList() << "uninstall" << appinfo->APPFILENAME + ".desktop");
    if ( QFile::remove(
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
            + "/" + appinfo->APPFILENAME + ".desktop") ) {
        print_line(tr("Remove desktop link"));
    }

    //+++ Remove from "start" menu
    // "xdg" version:
    //QProcess::execute(
    //    "xdg-desktop-menu",
    //    QStringList() << "uninstall" << appinfo->APPFILENAME + ".desktop");
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    if ( QFile::remove(appsDir.filePath(appinfo->APPFILENAME + ".desktop")) ) {
        print_line(tr("Remove from \"Start\" menu"));
    }

    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    //+++ Remove mime-type
    // "xdg" version:
    //QProcess::execute(
    //    "xdg-mime",
    //    QStringList() << "uninstall" << mylocal + "/share/mime/packages/" + appinfo->APPFILENAME + ".xml");
    QFile::remove(shareDir.filePath("mime/packages/" + appinfo->APPFILENAME + ".xml"));

    //+++ Remove icon(s)
    // Here just "apps" icons are removed. Also "mimetypes" icons are possible, but I haven't found any
    // use for them. The documentation says "Icons to be used as file icons should use mimetypes as context",
    // but the file managers in at least GNOME, KDE and XFCE show the icon even if it is only saved in "apps".
    // In Cinnamon this doesn't work, but using the "mimetypes" context doesn't work either.
    // "xdg" version:
    //QProcess::execute("xdg-icon-resource",
    //                  QStringList() << "uninstall" << appinfo->APPFILENAME << "--size" << "128" << "--context" << "apps");
    // xdg-icon-resource can't handle svg files ...
    QFile::remove(shareDir.filePath("icons/hicolor/scalable/apps" + appinfo->APPFILENAME + ".svg"));
    // In case there are png images:
    for ( const auto &dirEntry : QDirListing(
             shareDir.filePath("icons/hicolor"),
             QDirListing::IteratorFlag::DirsOnly) ) {
        QFile::remove(dirEntry.filePath() + "/apps/" + appinfo->APPFILENAME + ".png");
    }

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
