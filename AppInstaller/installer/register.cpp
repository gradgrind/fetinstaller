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

QString uninstall_command{"powershell -ExecutionPolicy Bypass -WindowStyle Hidden -File \"%1\\app_uninstall.ps1\""};

bool Installer::isInstalled(QString dir)
{
    if ( dir.isEmpty() ) {
        QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
        QString app{ui->versioned->isChecked() ? versioned_app : appinfo.APPNAME};
        QString UNINFO{"Microsoft/Windows/CurrentVersion/Uninstall/" + app};
        auto ustring = regsettings.value(UNINFO + "/UninstallString");
        if ( !ustring.isNull() ) {
            uninstall_exe = ustring.toString();
            return !uninstall_exe.isEmpty();
        }
        return false;
    }
    uninstall_exe = uninstall_command.arg(QDir::toNativeSeparators(dir));
    // The app counts as "installed" only if the uninstaller exists.
    return QFileInfo{dir + "/" +  "app_uninstall.ps1"}.isFile();
}

bool Installer::missingFileTypeAssociation()
{
    if ( appinfo.ASSOC_EXT.isEmpty() ) return false;
    QSettings regsettings{
        "HKEY_CURRENT_USER\\Software\\Classes\\" + appinfo.ASSOC_EXT + "\\OpenWithProgIds", QSettings::NativeFormat};
    return regsettings.childKeys().isEmpty();
}

bool Installer::preRegister()
{
    localfiles.clear(); // collect files added outside the app bundle an registry entries
    return true;
}

bool Installer::registerApp()
{

    QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
    QString app{ui->versioned->isChecked() ? versioned_app : appinfo.APPNAME};
    // Register uninstaller.
    QString app_uninstall_reg{"Microsoft/Windows/CurrentVersion/Uninstall/" + app};
    regsettings.beginGroup(app_uninstall_reg);
    localfiles.append("***" + app_uninstall_reg)

    // The app folders are versioned regardless of whether the installation is versioned.
    QString idir{QDir::toNativeSeparators(app_dir.path())};

    regsettings.setValue("DisplayName", app);
    regsettings.setValue("UninstallString", uninstall_command.arg(idir));
    regsettings.setValue("Publisher", appinfo.Publisher);
    regsettings.setValue("WebLink", appinfo.WebLink);
    regsettings.setValue("DisplayVersion", appinfo.APPVERSION);
    regsettings.setValue("UninstallLocation", idir);
    // Installation size
    qint64 size{0};
    for ( const auto &dirEntry : QDirListing(
             app_dir.path(),
             QDirListing::IteratorFlag::FilesOnly
                 | QDirListing::IteratorFlag::Recursive
                 | QDirListing::IteratorFlag::IncludeHidden) ) { //TODO: IncludeHidden?
        size += dirEntry.size();
    }
    regsettings.setValue("EstimatedSize", size / 1024); // Windows uses KiB (1024 bytes)

    regsettings.endGroup();

    if ( !appinfo.ASSOC_EXT.isEmpty() && !ui->versioned->isChecked() ) {
        QString key_c{"Classes/"};
        QString key_c1{key_c + appinfo.ASSOC_EXT + "/OpenWithProgIds/" + appinfo.ASSOC_PROGID};
        localfiles.append("+++reg:" + key_c1);
        regsettings.setValue(key_c1, "");

        QString key_c2{key_c + appinfo.ASSOC_PROGID};
        localfiles.append("+++reg:" + key_c2);
        regsettings.beginGroup(key_c2);
        regsettings.setValue("shell/open/FriendlyAppName", appinfo.APPNAME + " " + appinfo.APPVERSION);
        QString appPath{"\"" + idir + "\\" + appinfo.APPEXEC + "\""};
        regsettings.setValue("shell/open/command/.", appPath + " \"%1\"");
        regsettings.setValue("DefaultIcon/.", appPath + ",0");
        regsettings.endGroup();
    }

    // Start menu entry
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    desktopName = app + ".lnk";
    desktopPath = app_dir.absoluteFilePath(appinfo.APPEXEC);
    QString link{appsDir.absoluteFilePath(desktopName)};
    if ( QFile::link(desktopPath, link) ) {
        localfiles.append(link);
    } else {
        print_line(tr("Couldn't install start menu entry: '%1'").arg(link), true);
        return false;
    }

    // Desktop starter, if requested.
    if ( ui->installDesktopLink->isChecked() ) {
        QDir dskDir{QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)};
        QString link{dskDir.absoluteFilePath(desktopName)};
        if ( QFile::link(desktopPath, link) ) {
            localfiles.append(link);
        } else {
            print_line(tr("Couldn't install desktop starter: '%1'").arg(link), true);
            // This doesn't cause the installation to fail.
        }
    }

    return true;
}

QString Installer::addedFileType()
{
    // The SHChangeNotify function allows the association of the
    // icons with the file type to be changed immediately.
        SHChangeNotify(
            SHCNE_ASSOCCHANGED,     // Event ID
            SHCNF_IDLIST,           // Flags
            NULL,                   // Item that changed
            NULL                    // Not used
            );
    return appinfo.ASSOC_EXT;
}

bool Installer::removeSpecial(QString item)
{
    if ( item.startsWith("+++reg:") ) {
        QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
        regsettings.remove(item.sliced(7));
    }
}

#elif defined Q_OS_LINUX

bool Installer::isInstalled(QString dir)
{
    // Assume an uninstaller is in the same directory as the app executable.
    QFileInfo uinfo;
    QString execfile{QDir::home().absoluteFilePath(".local/bin/" + appinfo.APPEXEC)};
    if ( dir.isEmpty() ) {
        if ( ui->versioned->isChecked() )
            execfile += "-" + appinfo.APPVERSION;
        QString xpath{QFileInfo{execfile}.readSymLink()};
        if ( xpath.isEmpty() ) {
            return false; // only if this is a symlink can the app count as "installed"
        }
        // Seek installation directory, and hence uninstaller.
        uinfo = QFileInfo{QFileInfo{xpath}.absolutePath() + "/app_uninstall"};
    } else {
        uinfo = QFileInfo{dir + "/" + appinfo.EXECDIR + "app_uninstall"};
    }
    uninstall_exe = uinfo.filePath();
    // The app counts as "installed" only if this exists and is executable.
    return uinfo.isExecutable();
}

bool Installer::missingFileTypeAssociation()
{
    if ( appinfo.APPMIME.isEmpty() ) return false;
    QString mrpath{"mime/packages/" + appinfo.APPMIME + ".xml"};
    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    return !QFileInfo{shareDir.absoluteFilePath(mrpath)}.isFile();
}

bool Installer::preRegister()
{
    // Collect the locations outside of the application bundle which will be written to.
    // Test that they are not already occupied.

    registrationList.clear();
    localfiles.clear();

    // For ".desktop" file (start menu, etc.)
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};

    // For executable link in PATH
    QDir binDir{QDir::home().absoluteFilePath(".local/bin")};

    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    bool ok{true};

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
                        newlines.append("Name=" + versioned_app);
                    } else if ( line.startsWith("Icon=") ) {
                        //TODO ... if the app has an icon ...
                        //newlines.append("Icon=" + absolute address of app bundle icon);
                        //else
                        newlines.append(line);
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

        // Add mime-type and icons, if available
        if ( !appinfo.APPMIME.isEmpty() ) {
            QString mrpath{"mime/packages/" + appinfo.APPMIME + ".xml"};
            QString mfile{app_dir.absoluteFilePath("share/" + mrpath)};
            if ( !QFileInfo{src_dir.absoluteFilePath("share/" + mrpath)}.isFile() ) {
                print_line(tr("Mime-type file missing in installation bundle: '%1'").arg(mfile), true);
                ok = false;
            } else {
                QString mime_path{shareDir.absoluteFilePath(mrpath)};
                registrationList.append(
                    {mfile, mime_path, tr("Install mime file: '%1'").arg(mime_path), R_COPY});
                localfiles.append("+++file-type:" + addedFileType()); // uninstaller flag

                //TODO: Only register the svg icon?

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
    print_line("");

    // It is possible that the installation bundle contains more than one executable and
    // '.desktop' file. Only deal with those for the main executable.
    // Anything else may need special treatment, or no handling at all.

    // Add files to ~/.local
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
            print_line(tr("Install desktop starter: '%1'").arg(desktopName));
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
            print_line(tr("Couldn't install desktop starter: '%1'").arg(dlpath), true);
        }
    }
    return true;
}

QString Installer::addedFileType()
{
    print_line(tr("Update icon cache"));
    QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
    print_line(tr("Update mime database"));
    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
    return appinfo.APPLICATION;
}

bool Installer::removeSpecial(QString item)
{
    return true; // there are no entries which need acting on here
}

#else

//TODO

#endif
