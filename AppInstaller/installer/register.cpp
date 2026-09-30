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
    // Only perform these operations if installing to the "standard" location, an application
    // directory in "~/.local/apps".
    if ( dst_dir.absolutePath() != defaultInstallationPath ) {
        return true;
    }

    print_line("");

    bool ok{true};
    // Add files to ~/.local
    localfiles.clear();
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
            QString ipath{appsDir.filePath(fname)};
            if ( QFile::link(fpath, ipath) ) {
                print_line(tr("Install \"Start\" menu entry '%1'").arg(fname));
                localfiles.append(ipath);
            } else {
                print_line(tr("Couldn't install \"Start\" menu entry '%1'").arg(fname), true);
                ok = false;
            }
            // Desktop starter.
            if ( ui->installDesktopLink->isChecked() ) {
                QString ipath{desktop.filePath(fname)};
                if ( QFile::copy(fpath, ipath) ) {
                    print_line(tr("Install desktop starter '%1'").arg(fname));
                    localfiles.append(ipath);
                } else {
                    print_line(tr("Couldn't install desktop starter '%1'").arg(fname), true);
                    ok = false;
                }
            }
        }
    }

    //+++ Add relevant symlinks in ~/.local/bin
    QDir binDir{QDir::home().absoluteFilePath(".local/bin")};
    binDir.mkdir(binDir.path());
    for ( const auto &dirEntry : QDirListing(
            dst_dir.absoluteFilePath("bin"),
            QDirListing::IteratorFlag::FilesOnly) ) {
        if ( dirEntry.isExecutable() ) {
            if ( dirEntry.baseName().endsWith("_uninstall") )
                continue;
            QString fname{dirEntry.fileName()};
            QString ipath{binDir.filePath(fname)};
            if ( QFile::link(dirEntry.absoluteFilePath(), ipath) ) {
                print_line(tr("Add executable '%1' to PATH").arg(fname));
                localfiles.append(ipath);
            } else {
                print_line(tr("Couldn't link executable '%1'").arg(fname), true);
                ok = false;
            }
        }
    }

    //+++ Add mime-type(s)
    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    for ( const auto &dirEntry : QDirListing(
             dst_dir.absoluteFilePath("share/mime/packages"),
             QDirListing::IteratorFlag::FilesOnly) ) {
        QString ipath{shareDir.absoluteFilePath("mime/packages/") + dirEntry.fileName()};
        if ( QFile::copy(dirEntry.absoluteFilePath(), ipath) ) {
            print_line(tr("Install mime file '%1'").arg(dirEntry.fileName()));
            localfiles.append(ipath);
        } else {
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
    if ( !linkDirectoryHierarchy(
            dst_dir.filePath("share/icons"),
            shareDir.absoluteFilePath("icons")) ) {
        ok = false;
    }

    if ( ok ) {
        // Open file to record files installed outside the installation directory.
        xfilepath = dst_dir.absoluteFilePath("system_files");
        file_log.setFileName(xfilepath);
        if ( file_log.open(QIODevice::WriteOnly | QIODevice::Text) ) {
            QTextStream log_stream(&file_log);
            for ( const auto& f : std:: as_const(localfiles) ) {
                log_stream << f << "\n";
            }
            file_log.close();

            print_line(tr("Update icon cache"));
            QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
            print_line(tr("Update mime database"));
            QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
            print_line(tr("Update desktop database"));
            QProcess::execute("update-desktop-database", QStringList{appsDir.path()});
            return true;
        } else {
            print_line("–––––>>>", true);
            print_line(tr("ERROR, could not create the 'system_files' list"), true);
        }
    }
    print_line(tr("App registration failed."), true);
    for ( const auto& f : std:: as_const(localfiles) ) {
        if ( !QFile::remove(f) ) {
            print_line(tr("(Recovery:) Removal failed: %1").arg(f), true);
        }
    }
    return false;
}

#else

//TODO

#endif

bool Installer::linkDirectoryHierarchy(const QString &srcPath, const QString &dstPath)
{
    // Loop through the directory contents, creating destination directories if necessary.
    // Files are symlinked.
    QDir src{srcPath};
    QDir dst{dstPath};
    bool ok {true};
    for ( const auto &dirEntry : QDirListing(srcPath) ) {
        QString rpath{src.relativeFilePath(dirEntry.absoluteFilePath())};
        QString fpath{dst.absoluteFilePath(rpath)};
        if (dirEntry.isDir()) {
            // Create dir if necessary, copy contents
            dst.mkpath(fpath);
            if ( !linkDirectoryHierarchy(dirEntry.absoluteFilePath(), fpath) )
                ok = false;
        } else {
            // Link file
            if ( QFile::link(dirEntry.absoluteFilePath(), fpath) ) {
                print_line(tr("Create icon link '%1'").arg(fpath));
                localfiles.append(fpath);
            } else {
                print_line(tr("Couldn't create icon link '%1'").arg(fpath));
                ok = false;
            }
        }
    }
    return ok;
}

