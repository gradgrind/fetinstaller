#include "installer.h"
#include <ui_installer.h>
#include <QProcess>
#include <QStandardPaths>

// Update file-type associations and desktop files.
// Windows: add app to registry.

#if defined Q_OS_WIN

void Installer::registerApp()
{
    //TODO: Write to regstry

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

bool Installer::registerApp()
{
    //TODO: The uninstaller can't easily manage multiple copies. Symlinks can be traced back to
    // the installation directory, but copies can't.
    // Perhaps there should be a list of copied files (absolute paths), perhaps it would help to
    // include the links too?

    // Only perform these operations if installing to the "standard" location, an application
    // directory in "~/.local/apps".
    if ( dst_dir.absolutePath() != defaultInstallationPath ) {
        return true;
    }

    bool ok{true};
    // Add files to ~/.local
    QString mylocal{QDir::home().absoluteFilePath(".local")};

    print_line("");

    //+++ Install .desktop link(s) (for "Start" menu entry), checking they don't already exist.
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    appsDir.mkpath(appsDir.path());
    //+++ If requested, install desktop "link(s)" (actually a copy of the .desktop file).
    // Don't use a link here, because editing is quite easy and in the case of a link
    // that would change the installation file.
    QDir desktop{QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)};
    for ( const auto &dirEntry : QDirListing(
            dst_dir.absoluteFilePath("share/applications"),
            QDirListing::IteratorFlag::FilesOnly) ) {
        QString fpath{dirEntry.absoluteFilePath()};
        if ( fpath.endsWith(".desktop") ) {
            QString fname{dirEntry.fileName()};
            // Start menu entry.
            print_line(tr("Install \"Start\" menu entry '%1'").arg(fname));
            if ( !QFile::link(fpath, appsDir.filePath(fname)) ) {
                print_line(tr("Couldn't install \"Start\" menu entry '%1'").arg(fname), true);
                ok = false;
            }
            // Desktop starter.
            if ( ui->installDesktopLink->isChecked() ) {
                print_line(tr("Install desktop starter '%1'").arg(fname));
                if ( !QFile::copy(fpath, desktop.filePath(fname)) ) {
                    print_line(tr("Couldn't install desktop starter '%1'").arg(fname), true);
                    ok = false;
                }
            }
        }
    }

    //+++ Add relevant symlinks in ~/.local/bin
    QDir binDir{mylocal + "/bin"};
    binDir.mkdir(binDir.path());
    for ( const auto &dirEntry : QDirListing(
            dst_dir.absoluteFilePath("bin"),
            QDirListing::IteratorFlag::FilesOnly) ) {
        if ( dirEntry.isExecutable() ) {
            if ( dirEntry.baseName().endsWith("_uninstall") )
                continue;
            print_line(tr("Add executable '%1' to PATH").arg(dirEntry.fileName()));
            if ( !QFile::link(
                    dirEntry.absoluteFilePath(),
                    binDir.filePath(dirEntry.fileName())) ) {
                print_line(tr("Couldn't link executable '%1'").arg(dirEntry.fileName()), true);
                ok = false;
            }
        }
    }

    //+++ Add mime-type(s)
    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    for ( const auto &dirEntry : QDirListing(
             dst_dir.absoluteFilePath("share/mime/packages"),
             QDirListing::IteratorFlag::FilesOnly) ) {
        if ( !QFile::copy(
                dirEntry.absoluteFilePath(),
                shareDir.absoluteFilePath("mime/packages/") + dirEntry.fileName()) ) {
            print_line(tr("Couldn't install mime file '%1'").arg(dirEntry.fileName()), true);
            ok = false;
        }
    }

    //+++ Add icon(s)
    // In principle there can be "apps" icons and "mimetypes" icons, but I haven't found any
    // use for the "mimetypes" icons. The documentation says "Icons to be used as file icons
    // should use 'mimetypes' as context", but the file managers in at least GNOME, KDE and XFCE
    // show the icon even if it is only in "apps".
    // In Cinnamon this doesn't work, but using the "mimetypes" context doesn't work either.
    const auto elist = linkDirectoryHierarchy(
        dst_dir.filePath("share/icons"),
        shareDir.absoluteFilePath("icons"));
    if ( elist.isEmpty() ) {
        print_line(tr("Icons linked"));
    } else {
        ok = false;
        for ( const auto& e : elist ) {
            print_line(e, true);
        }
    }

    print_line(tr("Update icon cache"));
    QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
    print_line(tr("Update mime database"));
    QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
    print_line(tr("Update desktop database"));
    QProcess::execute("update-desktop-database", QStringList{appsDir.path()});

    return ok;
}

#else

//TODO

#endif

QStringList Installer::linkDirectoryHierarchy(const QString &srcPath, const QString &dstPath) {
    // Loop through the directory contents, creating destination directories if necessary.
    // Files are symlinked.
    QStringList errors;
    QDir src{srcPath};
    QDir dst{dstPath};
    for ( const auto &dirEntry : QDirListing(srcPath) ) {
        QString rpath{src.relativeFilePath(dirEntry.absoluteFilePath())};
        QString fpath{dst.absoluteFilePath(rpath)};
        if (dirEntry.isDir()) {
            // Create dir if necessary, copy contents
            dst.mkpath(fpath);
            errors += linkDirectoryHierarchy(dirEntry.absoluteFilePath(), fpath);
        } else {
            // Link file
            if ( !QFile::link(dirEntry.absoluteFilePath(), fpath) ) {
                errors.append(tr("Couldn't create icon link '%1'").arg(fpath));
            }
        }
    }
    return errors;
}

