#include "installer.h"
#include "ui_installer.h"
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSettings>

// Update file-type associations and desktop files.
// Windows: add app to registry.

#if defined Q_OS_WIN
#include <windows.h>
#include <shlobj.h>

bool Installer::registerApp()
{
    // Only perform these operations if installing to the "standard" location, an application
    // directory in "~/.local/apps".
    if ( !registered ) {
        return true;
    }

    // Write to registry ...

    QString ShCtxt{"HKEY_CURRENT_USER"};
    QString UNINFO{ShCtxt + "\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appinfo.APPNAME};
    QSettings settings1(UNINFO, QSettings::NativeFormat);
    // Consider using versioned app folders ...
    QString LOCALAPPDATA{QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))};
    QString InstDir{LOCALAPPDATA + "\\Programs\\" + appinfo.APPNAME + "-" + appinfo.APPVERSION};

    settings1.setValue("DisplayName", appinfo.APPNAME);
    settings1.setValue("UninstallString", "\"" + InstDir + "\\app_uninstall.exe\""); //???
    //settings1.setValue("Publisher", "Liviu Lalescu");
    //settings1.setValue("UrlInfoAbout", "https://lalescu.ro/liviu/fet/");
    settings1.setValue("DisplayVersion", appinfo.APPVERSION);

    // Installation size
    qint64 size{0};
    for ( const auto &dirEntry : QDirListing(
             app_dir.path(),
             QDirListing::IteratorFlag::FilesOnly
             | QDirListing::IteratorFlag::Recursive
             | QDirListing::IteratorFlag::IncludeHidden) ) { //TODO: IncludeHidden?
        size += dirEntry.size();
    }
    settings1.setValue("EstimatedSize", size / 1024); // Windows uses KiB (1024 bytes)
    settings1.setValue("UninstallLocation", InstDir);

    QSettings settings2(ShCtxt + "\\Software\\Classes", QSettings::NativeFormat);
    settings2.setValue(ASSOC_EXT + "/OpenWithProgIds/" + ASSOC_PROGID, "");
    settings2.setValue(ASSOC_PROGID + "/shell/open/FriendlyAppName", appinfo.APPNAME + " " + appinfo.APPVERSION);
    QString AppPath{"\"" + InstDir + "\\" + appinfo.APPEXEC + "\""};
    settings2.setValue(ASSOC_PROGID + "/shell/open/command/.", AppPath + " \"%1\"");
    settings2.setValue(ASSOC_PROGID + "/DefaultIcon/.", AppPath + ",0");

    /*TODO--
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

    SHChangeNotify(
        SHCNE_ASSOCCHANGED,     // Event ID
        SHCNF_IDLIST,           // Flags
        NULL,                   // Item that changed
        NULL                    // Not used
        );

}

#elif defined Q_OS_LINUX

bool Installer::registerApp()
{
    if ( ui->unpack_only->isChecked() ) {
        // no "registration" of any sort
        return true;
    }

    //TODO: Add more links (man, doc, ...)?

    print_line("");

    // It is possible that the installation bundle contains more than one executable and
    // '.desktop' file. Only deal with those for the main executable.
    // Anything else may need special treatment, or no handling at all.

    // Add files to ~/.local
    localfiles.clear();

    // For ".desktop" file (start menu, etc.)
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    QString appsrpath{"share/applications/" + appinfo.APPLICATION + ".desktop"};
    QString appsfile{app_dir.absoluteFilePath(appsrpath)};
    if ( !QFileInfo{appsfile}.isFile() ) {
        print_line(tr("'Desktop' file missing in installation bundle: '%1'").arg(appsrpath), true);
        return false;
    }
    appsDir.mkpath(appsDir.path());

    // For executable link in PATH
    QDir binDir{QDir::home().absoluteFilePath(".local/bin")};
    binDir.mkpath(binDir.path());

    //QString execrpath{appinfo.EXECDIR + appinfo.APPEXEC};
    //QString execfile{app_dir.absoluteFilePath(execrpath)};
    //if ( !QFileInfo{execfile}.isExecutable() ) {
    //    print_line(tr("Executable missing in installation bundle: '%1'").arg(execrpath), true);
    //    return false;
    //}

    // If "versioned", add version to executable and desktop file, which then needs editing.
    QString appsdname; // name of ".desktop" file in .local/share/applications
    QString appspath; // full path to ".desktop" file in .local/share/applications
    bool mimefiles{false}; // flag for cache updating
    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    if ( ui->versioned->isChecked() ) {
        // Add versioned links to executables.
        // Note that there will be no mime type or icons if there is no unversioned installation!
        for ( const auto &b : std::as_const(appinfo.BINLINKS) ) {
            QString execrpath{appinfo.EXECDIR + b};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{execfile}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: '%1'").arg(execrpath), true);
                return false;
            }
            QString appexecpath{binDir.filePath(b + "-" + appinfo.APPVERSION)};
            if ( !QFile::link(execfile, appexecpath) ) {
                print_line(tr("Couldn't add executable link to PATH: '%1'").arg(appexecpath), true);
                return false;
            }
            print_line(tr("Add executable to PATH: '%1'").arg(appexecpath));
            localfiles.append(appexecpath);
        }

        // Add versioned .desktop file (for "Start" menu entry).
        // The "Exec" field needs editing.
        QFile f(appsfile);
        if ( f.open(QFile::ReadOnly | QFile::Text) ) {
            QTextStream in(&f);
            QStringList newlines;
            while (!in.atEnd())
            {
                QString line = in.readLine();
                if ( line.startsWith("Exec=") ) {
                    //TODO ...
                    newlines.append(
                        "Exec=" + appinfo.EXECLINE.replace(
                            "%APP%", appinfo.APPEXEC + "-" + appinfo.APPVERSION));
                } else if ( line.startsWith("Name=") ) {
                    newlines.append("Name=" + appinfo.APPNAME + "-" + appinfo.APPVERSION);

                    //TODO: If the Name line isn't APPNAME, I could use the original file
                    // and replace all Name lines by GenericName lines,
                    // delete all GenericName lines and add a single Name line ... but it would probably
                    // be better to change the original!
                } else {
                    newlines.append(line);
                }
            }
            f.close();
            // Write new, versioned, .desktop file
            appsdname = appinfo.APPLICATION + "-" + appinfo.APPVERSION + ".desktop";
            appspath  = appsDir.absoluteFilePath(appsdname);
            QFile fw(appspath);
            if ( fw.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) ) {
                QTextStream out(&fw);
                out << newlines.join("\n") << "\n";
                fw.close();
            } else {
                print_line(tr("Couldn't add versioned 'Start' menu entry: '%1'").arg(appspath), true);
                return false;
            }
        } else {
            print_line(tr("Couldn't read 'desktop' file: '%1'").arg(appsfile), true);
            return false;
        }
        print_line(tr("Add 'Start' menu entry: '%1'").arg(appspath));
        localfiles.append(appspath);
    } else {
        // Add "unversioned" files and links.

        // Executables
        for ( const auto &b : std::as_const(appinfo.BINLINKS) ) {
            QString execrpath{appinfo.EXECDIR + b};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{execfile}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: '%1'").arg(execrpath), true);
                return false;
            }
            QString appexecpath{binDir.filePath(b)};
            if ( !QFile::link(execfile, appexecpath) ) {
                print_line(tr("Couldn't add executable link to PATH: '%1'").arg(appexecpath), true);
                return false;
            }
            print_line(tr("Add executable to PATH: '%1'").arg(appexecpath));
            localfiles.append(appexecpath);
        }

        // "Desktop" file (for start menu entry, etc.)
        appsdname = appinfo.APPLICATION + ".desktop";
        appspath  = appsDir.absoluteFilePath(appsdname);
        if ( !QFile::copy(appsfile, appspath) ) {
            print_line(tr("Couldn't add 'Start' menu entry: '%1'").arg(appspath), true);
            return false;
        }
        print_line(tr("Add 'Start' menu entry: '%1'").arg(appspath));
        localfiles.append(appspath);

        // Add mime-type and icons, if present
        QString mfile{app_dir.absoluteFilePath("share/mime/packages/" + appinfo.APPLICATION + ".xml")};
        if ( QFileInfo{mfile}.isFile() ) {
            QString ipath{shareDir.absoluteFilePath("mime/packages/" + appinfo.APPLICATION + ".xml")};
            if ( QFile::copy(mfile, ipath) ) {
                print_line(tr("Install mime file: '%1'").arg(ipath));
                localfiles.append(ipath);
            } else {
                print_line(tr("Couldn't install mime file: '%1'").arg(ipath), true);
                return false;
            }

            // Add icon(s)
            // In principle there can be "apps" icons and "mimetypes" icons, but I haven't found any
            // use for the "mimetypes" icons. The documentation says "Icons to be used as file icons
            // should use 'mimetypes' as context", but the file managers in at least GNOME, KDE and XFCE
            // show the icon even if it is only in "apps".
            // In Cinnamon this doesn't work, but using the "mimetypes" context doesn't work either.
            // However, it does work in Cinnamon if the app's .xml file in share/mime/packages gets the
            // additional line:
            //    <generic-icon name="$APPLICATION"/> (after the line: <icon name="$APPLICATION"/>)
            if ( !linkDirectoryHierarchy(
                    app_dir.filePath("share/icons"),
                    shareDir.absoluteFilePath("icons")) ) {
                return false;
            }

            mimefiles = true;
        }
    }

    // Desktop starter, if requested.
    // The link points to the start-menu entry so that it counts as "trustworthy".
    if ( ui->installDesktopLink->isChecked() ) {
        QDir desktop{QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)};
        QString dlpath{desktop.filePath(appsdname)};
        if ( QFile::link(appspath, dlpath) ) {
            print_line(tr("Install desktop starter '%1'").arg(appsdname));
            localfiles.append(dlpath);
            // Make link executable
            QFile file(dlpath);
            if ( file.exists() ) {
                QFile::Permissions currentPermissions = file.permissions();
                // Add write permission for the owner
                QFile::Permissions newPermissions =
                    currentPermissions | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther;
                if ( !file.setPermissions(newPermissions) ) {
                    print_line(tr("Failed to make desktop starter executable: '%1'").arg(dlpath), true);
                }
            }
        } else {
            print_line(tr("Couldn't install desktop starter '%1'").arg(dlpath), true);
        }
    }

    // Open file to record files installed outside the installation directory.
    xfilepath = app_dir.absoluteFilePath("system_files");
    file_log.setFileName(xfilepath);
    if ( file_log.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) ) {
        QTextStream log_stream(&file_log);
        for ( const auto& f : std:: as_const(localfiles) ) {
            log_stream << f << "\n";
        }

        if ( mimefiles ) {
            print_line(tr("Update icon cache"));
            QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
            print_line(tr("Update mime database"));
            QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
            log_stream << "+++ registered +++\n";
        }

        file_log.close();

        print_line(tr("Update desktop database"));
        QProcess::execute("update-desktop-database", QStringList{appsDir.path()});
        return true;
    }

    print_line("–––––>>>", true);
    print_line(tr("ERROR, could not create the 'system_files' list"), true);
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

