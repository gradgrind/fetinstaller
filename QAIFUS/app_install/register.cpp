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

#define UNINSTALLER_NAME "app_uninstall0.exe"

bool Installer::isInstalled(QString dir)
{
    if ( dir.isEmpty() ) {
        QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
        QString proj{appinfo.PROJECTNAME};
        if ( ui->versioned->isChecked() ) {
            proj += "-" + appinfo.PROJECTVERSION;
        }
        QString UNINFO{"Microsoft/Windows/CurrentVersion/Uninstall/" + proj};
        auto ustring = regsettings.value(UNINFO + "/UninstallLocation");
        if ( ustring.isNull() ) {
            return false;
        }
        uninstall_exe = QDir::fromNativeSeparators(ustring.toString()) + "/" + UNINSTALLER_NAME;
    } else {
        uninstall_exe = dir + "/" + UNINSTALLER_NAME;
    }
    // The app counts as "installed" only if the uninstaller exists.
    return QFileInfo{uninstall_exe}.isFile();
}

bool Installer::hasFileTypeAssociation()
{
    for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
        if ( !i.value().extra.isEmpty() ) {
            return true;
        }
    }
    return false;
}

bool Installer::preRegister()
{
    localfiles.clear(); // collect files added outside the app bundle, and registry entries
    return true;
}

bool Installer::registerApp()
{
    QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
    QString proj{appinfo.PROJECTNAME};
    if ( ui->versioned->isChecked() ) {
        proj += "-" + appinfo.PROJECTVERSION;
    }
    // Register uninstaller.
    QString app_uninstall_reg{"Microsoft/Windows/CurrentVersion/Uninstall/" + proj};

    regsettings.beginGroup(app_uninstall_reg);

    localfiles.append("+++reg:" + app_uninstall_reg);

    // The app folders are versioned regardless of whether the installation is versioned.
    QString idir{QDir::toNativeSeparators(app_dir.path())};

    print_line(tr("Register project and uninstaller"));
    regsettings.setValue("DisplayName", proj);
    regsettings.setValue("UninstallString", '"' + idir + '\\' + UNINSTALLER_NAME + '"');
    regsettings.setValue("Publisher", appinfo.Publisher);
    regsettings.setValue("WebLink", appinfo.WebLink);
    regsettings.setValue("DisplayVersion", appinfo.PROJECTVERSION);
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
    regsettings.setValue("EstimatedSize", static_cast<quint32>(size / 1024)); // Windows uses KiB (1024 bytes)

    regsettings.endGroup();

    if ( !ui->versioned->isChecked() ) {
        // File-type association
        QString key_c{"Classes/"};
        for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
            QString exec = i.key();
            auto execval = i.value();
            if ( execval.extra.isEmpty() ) {
                continue;
            }
            for ( const auto& fass : execval.extra.split(",") ) {
                auto fassx = fass.split(">");
                if ( fassx.length() != 2 ) {
                    print_line(tr("Invalid EXEC entry in app.conf: %1=%2:%3:%4")
                                   .arg(exec, execval.extension, execval.execName, execval.extra), true);
                    return false;
                }
                QString assoc_ext{fassx.at(0)};
                QString assoc_progid{fassx.at(1)};
                print_line(tr("Register file-type association for '%1'").arg(assoc_ext));
                QString key_c1{key_c + assoc_ext + "/OpenWithProgIds/" + assoc_progid};
                localfiles.append("+++reg:" + key_c1);
                regsettings.setValue(key_c1, "");

                QString key_c2{key_c + assoc_progid};
                localfiles.append("+++reg:" + key_c2);
                regsettings.beginGroup(key_c2);
                regsettings.setValue("shell/open/FriendlyAppName", exec + " " + appinfo.PROJECTVERSION);
                QString appPath{"\"" + idir + "\\" + exec + execval.extension + "\""};
                regsettings.setValue("shell/open/command/.", appPath + " \"%1\"");
                regsettings.setValue("DefaultIcon/.", appPath + ",0");
                regsettings.endGroup();

                localfiles.append("+++file-type:" + assoc_ext);
            }
        }
    }

    // Start menu entries
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
        QString exec = i.key();
        auto execval = i.value();
        QString desktopName{execval.execName};
        if ( desktopName.isEmpty() ) {
            continue;
        }
        if ( !ui->versioned->isChecked() ) {
            desktopName + "-" + appinfo.PROJECTVERSION;
        }
        desktopName += ".lnk";
        QString desktopPath{app_dir.absoluteFilePath(exec + execval.extension)};
        QString link{appsDir.absoluteFilePath(desktopName)};
        if ( QFile::link(desktopPath, link) ) {
            print_line(tr("Install start menu entry: '%1'").arg(desktopName));
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
                print_line(tr("Install desktop starter: '%1'").arg(desktopName));
                localfiles.append(link);
            } else {
                print_line(tr("Couldn't install desktop starter: '%1'").arg(link), true);
                // This doesn't cause the installation to fail.
            }
        }
    }
    return true;
}

void Installer::addedFileType()
{
    // The SHChangeNotify function allows the association of the
    // icons with the file type to be changed immediately.
        SHChangeNotify(
            SHCNE_ASSOCCHANGED,     // Event ID
            SHCNF_IDLIST,           // Flags
            NULL,                   // Item that changed
            NULL                    // Not used
            );
}

bool Installer::removeSpecial(QString item)
{
    if ( item.startsWith("+++reg:") ) {
        QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
        auto key = item.sliced(7);
        regsettings.remove(key);
        return !regsettings.contains(key);
    }
    return true;
}

bool Installer::isInstallBundle(QDir dir)
{
    return dir.exists("app_uninstall.exe");
}

void Installer::update_desktop_database()
{
    // No action is necessary.
}

#elif defined Q_OS_LINUX
#include <QXmlStreamReader>

static const char *OTHER_INSTALLATION = QT_TRANSLATE_NOOP("Installer", R"(
'%1' is already in the PATH: '%2'

Apparently it has already been installed in some other way.)");

bool Installer::isInstalled(QString dir)
{
    // Assume an uninstaller is in the same directory as the app executable.
    QFileInfo uinfo;
    if ( dir.isEmpty() ) {
        // Check each of the EXECUTABLES:
        //   - Is it a symlink? No: not interesting.
        //   - Yes: check for uninstaller in its target's directory.
        for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
            QString exec = i.key();
            if ( ui->versioned->isChecked() ) {
                exec += "-" + appinfo.PROJECTVERSION;
            }
            auto execval = i.value();
            if ( !execval.extension.isEmpty() ) {
                exec += execval.extension;
            }
            QString execpath{QDir::home().absoluteFilePath(".local/bin/" + exec)};
            QString xpath{QFileInfo{execpath}.readSymLink()};
            if ( xpath.isEmpty() ) {
                return false; // only if this is a symlink can the app count as "installed"
            }
            // Seek installation directory, and hence uninstaller.
            uinfo = QFileInfo{QFileInfo{xpath}.absolutePath() + "../_installer_/app_uninstall"};
            if ( uinfo.isExecutable() ) {
                uninstall_exe = uinfo.absoluteFilePath();
                // The app counts as "installed" if this exists and is executable.
                return true;
            }
        }
        return false;
    }
    uinfo = QFileInfo{dir + "/_installer_/app_uninstall"};
    uninstall_exe = uinfo.filePath();
    // The app counts as "installed" only if this exists and is executable.
    return uinfo.isExecutable();
}

bool Installer::hasFileTypeAssociation()
{
    return !QDir{src_dir.absoluteFilePath("share/mime/packages")}.isEmpty();
}

bool Installer::execInPath(QString xname)
{
    QString xpath = QStandardPaths::findExecutable(xname);
    if ( xpath.isEmpty() )
        return false;
    print_line(tr(OTHER_INSTALLATION).arg(xname, xpath), true);
    return true;
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
        // VERSIONED INSTALLATION
        // Add versioned links to executables.
        // Note that there will be no mime type or icons if there is no unversioned installation!
        for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
            QString execbase{i.key()};
            auto execval = i.value();
            QString execv{execbase + "-" + appinfo.PROJECTVERSION + execval.extension};
            if ( execInPath(execv) ) {
                ok = false;
            }
            QString execfull{execbase + execval.extension};
            QString execrpath{"bin/" + execfull};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{src_dir.filePath(execrpath)}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: '%1'").arg(execrpath), true);
                ok = false;
            }
            QString appexecpath{binDir.filePath(execv)};
            registrationList.append(
                {execfile, appexecpath, tr("Add executable to PATH: '%1'").arg(appexecpath), R_LINK});

            if ( !execval.execName.isEmpty() ) {
                // Add versioned .desktop file (for "Start" menu entry).
                // The "Exec", "Name" and, possibly, "Icon" fields needs editing.
                QString appicon;
                if ( !desktop_icon(appicon, execbase) ) {
                    ok = false;
                } else {
                    QString desktopName{execbase + "-" + appinfo.PROJECTVERSION + ".desktop"};
                    QString desktopPath{appsDir.absoluteFilePath(desktopName)};
                    // Edit the source file.
                    QString dtfile{src_dir.absoluteFilePath("share/applications/" + execbase + ".desktop")};
                    QFile f(dtfile);
                    if ( !f.open(QFile::ReadOnly | QFile::Text) ) {
                        print_line("BUG, Repeated read in installation bundle failed: share/applications/"
                                       + execbase + ".desktop", true);
                        ok = false;
                    } else {
                        QTextStream in(&f);
                        QStringList newlines;
                        while (!in.atEnd())
                        {
                            QString line = in.readLine();
                            if ( line.startsWith("Exec=") ) {
                                newlines.append("Exec=" + execval.extra.replace("%APP%", execv));
                                continue;
                            }
                            if ( line.startsWith("Name=") ) {
                                newlines.append("Name=" + execval.execName + "-" + appinfo.PROJECTVERSION);
                                continue;
                            }
                            if ( line.startsWith("Icon=") && !appicon.isEmpty() ) {
                                // Use icon's absolute path.
                                QString iconpathx{"share/icons/hicolor/scalable/apps/" + appicon + ".svg"};
                                line = "Icon=" + app_dir.absoluteFilePath(iconpathx);
                            }
                            newlines.append(line);
                        }
                        f.close();
                        // Queue the new file for writing.
                        registrationList.append(
                            {newlines.join("\n") + "\n", desktopPath,
                             tr("Add 'Start' menu entry: '%1'").arg(desktopPath), R_WRITE});
                    }
                }
            }
        } // END VERSIONED INSTALLATION
    } else {
        // UNVERSIONED INSTALLATION
        QStringList icons; // collect icon names to register
        // Add unversioned links to executables.
        for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
            QString execbase{i.key()};
            auto execval = i.value();
            QString execfull{execbase + execval.extension};
            if ( execInPath(execfull) ) {
                ok = false;
            }
            QString execrpath{"bin/" + execfull};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{src_dir.filePath(execrpath)}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: '%1'").arg(execrpath), true);
                ok = false;
            }
            QString appexecpath{binDir.filePath(execfull)};
            registrationList.append(
                {execfile, appexecpath, tr("Add executable to PATH: '%1'").arg(appexecpath), R_LINK});

            if ( !execval.execName.isEmpty() ) {
                // Add unversioned .desktop file (for "Start" menu entry).
                QString appicon;
                if ( !desktop_icon(appicon, execbase) ) {
                    ok = false;
                } else {
                    QString desktopName{execbase + ".desktop"};
                    QString desktopPath{appsDir.absoluteFilePath(desktopName)};
                    // The desktop file is not edited. It is assumed that it's content is correct.
                    registrationList.append(
                        {app_dir.absoluteFilePath("share/applications/" + desktopName), desktopPath,
                         tr("Add 'Start' menu entry: '%1'").arg(desktopPath), R_COPY});
                    if ( !appicon.isEmpty() ) {
                        icons.append(appicon);
                    }

                    // Add mime-type and icons, if available.
                    // The mime info file must have the same name as the executable.
                    QString mrpath{"mime/packages/" + execbase + ".xml"};
                    QString mxrpath{"share/" + mrpath};
                    QString mdfile{app_dir.absoluteFilePath(mxrpath)};
                    QString msfile{src_dir.absoluteFilePath(mxrpath)};
                    if ( QFileInfo{msfile}.isFile() ) {
                        // Read the file as XML, seek the icon entries.
                        // This just deals with <icon name= .../> entries.
                        QFile f(msfile);
                        if ( !f.open(QFile::ReadOnly | QFile::Text) ) {
                            print_line(tr("Couldn't read mime-type file in installation bundle: '%1'").arg(mxrpath), true);
                            ok = false;
                        } else {
                            QXmlStreamReader xml(&f);
                            while ( !xml.atEnd() ) {
                                if ( xml.readNext() == QXmlStreamReader::StartElement ) {
                                    if (xml.name() == "icon") {
                                        QString iconname{xml.attributes().value("name")};
                                        if ( !iconname.isEmpty() && !icons.contains(iconname) ) {
                                            icons.append(iconname);
                                        }
                                    }
                                }
                            }
                            if ( xml.hasError() ) {
                                print_line(tr("Invalid mime-type file in installation bundle: '%1'").arg(mxrpath), true);
                                print_line(tr("  ... in line %1: %2").arg(xml.lineNumber()).arg(xml.errorString()), true);
                                ok = false;
                            } else {
                                // Register the mime file.
                                QString mime_path{shareDir.absoluteFilePath(mrpath)};
                                registrationList.append(
                                    {mdfile, mime_path, tr("Install mime file: '%1'").arg(mime_path), R_COPY});
                                localfiles.append("+++file-type:" + execbase); // uninstaller flag
                            }
                        }
                    }
                }
            }
        }

        // Add icon link(s).
        // In principle there can be "apps" icons and "mimetypes" icons, but I haven't found any
        // use for the "mimetypes" icons. The documentation says "Icons to be used as file icons
        // should use 'mimetypes' as context", but it seems to work in at least GNOME, KDE and XFCE
        // using just "apps".
        // In Cinnamon this doesn't work, but does if the app's .xml file in share/mime/packages
        // gets the additional line:
        //    <generic-icon name="icon-name"/> (after the line: <icon name="icon-name"/>)
        // Register only the svg icon(s) for the mime type (and desktop file).
        for ( const auto& iname : std::as_const(icons) ) {
            QString iconpath{"icons/hicolor/scalable/apps/" + iname + ".svg"};
            QString iconpathx{"share/" + iconpath};
            QString sfile{src_dir.absoluteFilePath(iconpathx)};
            if ( !QFileInfo::exists(sfile) ) {
                print_line(tr("Icon missing in installation bundle: '%1'").arg(iconpathx), true);
                ok = false;
            } else {
                QString dpath{shareDir.absoluteFilePath(iconpath)};
                registrationList.append(
                    {app_dir.absoluteFilePath(iconpathx), dpath, tr("Add icon file: '%1'").arg(dpath), R_LINK});
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
    } // END UNVERSIONED INSTALLATION

    for ( const auto& reg : std::as_const(registrationList) ) {
        if ( QFileInfo::exists(reg.destination) || QFileInfo{reg.destination}.isSymbolicLink() ) {
            print_line(tr("Can't install item to: '%1'").arg(reg.destination), true);
            ok = false;
        }
    }
    return ok;
}

bool Installer::desktop_icon(QString &icon, QString execbase)
{
    // Get Icon name from .desktop file if there is a corresponding svg icon in the app bundle.
    // The name is stored to the 'icon' parameter. Return 'false' if an error occurred.
    // Note that the result can be 'true' without setting 'icon' (if the icon is not provided at
    // the correct location in the app bundle, as an SVG).
    QString appsrpath{"share/applications/" + execbase + ".desktop"};
    QString dtfile{src_dir.filePath(appsrpath)};
    if ( !QFileInfo{dtfile}.isFile() ) {
        print_line(tr("'Desktop' file missing in installation bundle: '%1'").arg(appsrpath), true);
        return false;
    }
    // Read the file, seek the "Icon=" line.
    QFile f(dtfile);
    if ( !f.open(QFile::ReadOnly | QFile::Text) ) {
        print_line(tr("Couldn't read 'desktop' file in installation bundle: '%1'").arg(appsrpath), true);
        return false;
    }
    QTextStream in(&f);
    while (!in.atEnd())
    {
        QString line = in.readLine();
        if ( line.startsWith("Icon=") ) {
            line.slice(5);
            QString iconpath{"share/icons/hicolor/scalable/apps/" + line + ".svg"};
            QString sfile{src_dir.absoluteFilePath(iconpath)};
            if ( QFileInfo::exists(sfile) ) {
                icon = line;
            } else {
                print_line(tr("Warning, icon missing in installation bundle: '%1'").arg(iconpath), true);
            }
            break;
        }
    }
    f.close();
    return true;
}

bool Installer::registerApp()
{
    print_line("");
    // Add files to ~/.local
    QDir d0;
    bool ok{true};
    QStringList desktopFiles; // for desktop starter links
    for ( const auto& reg : std::as_const(registrationList) ) {
        d0.mkpath(QFileInfo{reg.destination}.absolutePath()); // ensure that containing directory exists
        if ( reg.destination.endsWith(".desktop") ) {
            desktopFiles.append(reg.destination);
        }
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
                localfiles.append(reg.destination);
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
        for ( const auto& d : desktopFiles ) {
            QString desktopName{QFileInfo{d}.fileName()};
            QString dlpath{desktop.filePath(desktopName)};
            if ( QFile::link(d, dlpath) ) {
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
    }
    return true;
}

void Installer::addedFileType()
{
    print_line(tr("Update icon cache"));
    QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
    print_line(tr("Update mime database"));
    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
}

bool Installer::removeSpecial(QString item)
{
    return true; // there are no entries which need acting on here
}

bool Installer::isInstallBundle(QDir dir)
{
    return dir.exists("_installer_/app_uninstall");
}

void Installer::update_desktop_database()
{
    print_line(tr("Update desktop database"));
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    QProcess::execute("update-desktop-database", QStringList{appsDir.path()});
}

#else

//TODO

#endif
