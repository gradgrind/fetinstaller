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

bool Installer::isInstalled(QString dir)
{
    QFileInfo uinfo;
    QString execfile{QDir::home().absoluteFilePath(".local/bin/" + appinfo.APPEXEC)};
    if ( dir.isEmpty() ) {
        QString xpath{QFileInfo{execfile}.readSymLink()};
        if ( xpath.isEmpty() ) {
            return false; // only if this is a symlink can the app count as "installed"
        }
        // Seek installation directory, and hence uninstaller.
        uinfo = QFileInfo{QFileInfo{xpath}.absolutePath() + "/app_uninstall"};
    } else {
        uinfo = QFileInfo{dir + "/" + appinfo.EXECDIR + "/app_uninstall"};
    }
    uninstall_exe = uinfo.filePath();
    // The app counts as "installed" only if this exists and is executable.
    return uinfo.isExecutable();
}

bool Installer::preRegister()
{
    // Collect the locations outside of the application bundle which will be written to.
    // Test that they are not already occupied.
    registrationList.clear();
    mimefiles = false; // flag for cache updating

    if ( ui->unpack_only->isChecked() ) {
        // no "registration" of any sort
        return true;
    }

    bool ok{true};

    // For ".desktop" file (start menu, etc.)
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};

    // For executable link in PATH
    QDir binDir{QDir::home().absoluteFilePath(".local/bin")};

    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    if ( ui->versioned->isChecked() ) {
        // Add versioned links to executables.
        // Note that there will be no mime type or icons if there is no unversioned installation!
        for ( const auto &b : std::as_const(appinfo.BINLINKS) ) {
            QString execrpath{appinfo.EXECDIR + b};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{src_dir.filePath(execrpath)}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: '%1'").arg(execrpath), true);
                ok = false;
            }
            QString appexecpath{binDir.filePath(b + "-" + appinfo.APPVERSION)};
            registrationList.append(
                {execfile, appexecpath, tr("Add executable to PATH: '%1'").arg(appexecpath), R_LINK});
        }

        // Add versioned .desktop file (for "Start" menu entry).
        // The "Exec" field needs editing.
        QString appsrpath{"share/applications/" + appinfo.APPLICATION + ".desktop"};
        QString appsfile{app_dir.absoluteFilePath(appsrpath)};
        QString dtfile{src_dir.filePath(appsrpath)};
        if ( !QFileInfo{dtfile}.isFile() ) {
            print_line(tr("'Desktop' file missing in installation bundle: '%1'").arg(appsrpath), true);
            ok = false;
        } else {
            // New, versioned, .desktop file
            desktopName = appinfo.APPLICATION + "-" + appinfo.APPVERSION + ".desktop";
            desktopPath = appsDir.absoluteFilePath(desktopName);
            // The "Exec" field needs editing.
            QFile f(dtfile);
            if ( f.open(QFile::ReadOnly | QFile::Text) ) {
                QTextStream in(&f);
                QStringList newlines;
                while (!in.atEnd())
                {
                    QString line = in.readLine();
                    if ( line.startsWith("Exec=") ) {
                        newlines.append(
                            "Exec=" + appinfo.EXECLINE.replace(
                                "%APP%", appinfo.APPEXEC + "-" + appinfo.APPVERSION));
                    } else if ( line.startsWith("Name=") ) {
                        newlines.append("Name=" + appinfo.APPNAME + "-" + appinfo.APPVERSION);
                    } else {
                        newlines.append(line);
                    }
                }
                f.close();
                registrationList.append(
                    {newlines.join("\n") + "\n", desktopPath,
                     tr("Add 'Start' menu entry: '%1'").arg(desktopPath), R_WRITE});
            } else {
                print_line(tr("Couldn't read 'desktop' file in installation bundle: '%1'").arg(appsrpath), true);
                ok = false;
            }
        }
    } else {
        // Add "unversioned" links to executables.
        for ( const auto &b : std::as_const(appinfo.BINLINKS) ) {
            QString execrpath{appinfo.EXECDIR + b};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{src_dir.filePath(execrpath)}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: '%1'").arg(execrpath), true);
                ok = false;
            }
            QString appexecpath{binDir.filePath(b)};
            registrationList.append(
                {execfile, appexecpath, tr("Add executable to PATH: '%1'").arg(appexecpath), R_LINK});
        }

        // Add unversioned .desktop file (for "Start" menu entry).
        QString appsrpath{"share/applications/" + appinfo.APPLICATION + ".desktop"};
        QString appsfile{app_dir.absoluteFilePath(appsrpath)};
        if ( !QFileInfo{src_dir.filePath(appsrpath)}.isFile() ) {
            print_line(tr("'Desktop' file missing in installation bundle: '%1'").arg(appsrpath), true);
            ok = false;
        } else {
            desktopName = appinfo.APPLICATION + ".desktop";
            desktopPath = appsDir.absoluteFilePath(desktopName);
            registrationList.append(
                {appsfile, desktopPath,
                 tr("Add 'Start' menu entry: '%1'").arg(desktopPath), R_COPY});
        }

        // Add mime-type and icons, if present
        QString mrpath{"mime/packages/" + appinfo.APPLICATION + ".xml"};
        QString mfile{app_dir.absoluteFilePath("share/" + mrpath)};
        if ( QFileInfo{src_dir.absoluteFilePath("share/" + mrpath)}.isFile() ) {
            mimefiles = true;
            QString ipath{shareDir.absoluteFilePath(mrpath)};
            registrationList.append(
                {mfile, ipath, tr("Install mime file: '%1'").arg(ipath), R_COPY});

            // Add icon link(s).
            // In principle there can be "apps" icons and "mimetypes" icons, but I haven't found any
            // use for the "mimetypes" icons. The documentation says "Icons to be used as file icons
            // should use 'mimetypes' as context", but the file managers in at least GNOME, KDE and XFCE
            // show the icon even if it is only in "apps".
            // In Cinnamon this doesn't work, but using the "mimetypes" context doesn't work either.
            // However, it does work in Cinnamon if the app's .xml file in share/mime/packages gets the
            // additional line:
            //    <generic-icon name="$APPLICATION"/> (after the line: <icon name="$APPLICATION"/>)
            // NOTE: Only icons needed for the the mime-type should be in "share/icons".
            QDir iconsource{src_dir.absoluteFilePath("share/icons")};
            QDir iconinsrc{app_dir.absoluteFilePath("share/icons")};
            QDir icondest{shareDir.absoluteFilePath("icons")};
            for (const auto& f : QDirListing(
                     iconsource.path(),
                     QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::Recursive)) {
                QString rpath{iconsource.relativeFilePath(f.absoluteFilePath())};
                QString sfile{iconinsrc.absoluteFilePath(rpath)};
                QString dpath{icondest.absoluteFilePath(rpath)};
                registrationList.append(
                    {sfile, dpath, tr("Add icon file: '%1'").arg(dpath), R_LINK});
            }
        }

        // man page link(s)
        QDir mansource{src_dir.absoluteFilePath("share/man")};
        QDir maninsrc{app_dir.absoluteFilePath("share/man")};
        QDir mandest{shareDir.absoluteFilePath("man")};
        for (const auto& f : QDirListing(
                 mansource.path(),
                 QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::Recursive)) {
            QString rpath{mansource.relativeFilePath(f.absoluteFilePath())};
            QString sfile{maninsrc.absoluteFilePath(rpath)};
            QString dpath{mandest.absoluteFilePath(rpath)};
            registrationList.append(
                {sfile, dpath, tr("Add man file: '%1'").arg(dpath), R_LINK});
        }
    }

    for ( const auto& reg : std::as_const(registrationList) ) {
        if ( QFileInfo::exists(reg.destination) || QFileInfo{reg.destination}.isSymbolicLink() ) {
            print_line(tr("Can't install item to: '%1'").arg(reg.destination), true);
            ok = false;
        }
    }
    return ok;
}

bool Installer::registerApp()
{
    if ( ui->unpack_only->isChecked() ) {
        // no "registration" of any sort
        return true;
    }

    print_line("");

    // It is possible that the installation bundle contains more than one executable and
    // '.desktop' file. Only deal with those for the main executable.
    // Anything else may need special treatment, or no handling at all.

    // Add files to ~/.local
    localfiles.clear();
    QDir d0;
    bool ok{true};
    for ( const auto& reg : std::as_const(registrationList) ) {
        d0.mkpath(QFileInfo{reg.destination}.absolutePath()); // ensure that containing directory exists
        if ( reg.type == R_COPY ) {
            // Copy the file
            if ( QFile::copy(reg.source, reg.destination) ) {
                localfiles.append(reg.destination);
                print_line(reg.message);
                continue;
            }
        } else if ( reg.type == R_LINK ) {
            // Link the file
            if ( QFile::link(reg.source, reg.destination) ) {
                localfiles.append(reg.destination);
                print_line(reg.message);
                continue;
            }

        } else if ( reg.type == R_WRITE ) {
            // Write the file
            QFile fw(reg.destination);
            if ( fw.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) ) {
                QTextStream out(&fw);
                out << reg.source;
                fw.close();
                print_line(reg.message);
                continue;
            }
        }
        ok = false;
        print_line(tr("Failed: ") + reg.message, true);
    }
    if ( !ok ) return false;

    // Desktop starter, if requested.
    // The link points to the start-menu entry so that it counts as "trustworthy", where relevant.
    if ( ui->installDesktopLink->isChecked() ) {
        QDir desktop{QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)};
        QString dlpath{desktop.filePath(desktopName)};
        if ( QFile::link(desktopPath, dlpath) ) {
            print_line(tr("Install desktop starter '%1'").arg(desktopName));
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
    QFile file_log{app_dir.absoluteFilePath("system_files")};
    if ( file_log.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) ) {
        QTextStream log_stream(&file_log);
        for ( const auto& f : std:: as_const(localfiles) ) {
            log_stream << f << "\n";
        }

        if ( mimefiles ) {
            print_line(tr("Update icon cache"));
            QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
            print_line(tr("Update mime database"));
            QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
            QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
            log_stream << "+++ registered +++\n";
        }

        file_log.close();

        print_line(tr("Update desktop database"));
        QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
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
