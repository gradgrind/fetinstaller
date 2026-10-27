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
        QString app{ui->versioned->isChecked() ? versioned_app : appinfo.APPNAME};
        QString UNINFO{"Microsoft/Windows/CurrentVersion/Uninstall/" + app};
        auto ustring = regsettings.value(UNINFO + "/UninstallLocation");
        if ( ustring.isNull() ) {
            return false;
        }
        uninstall_exe = QDir::fromNativeSeparators(ustring.toString() + "/" + UNINSTALLER_NAME);
    } else {
        uninstall_exe = dir + "/" + UNINSTALLER_NAME;
    }
    // The app counts as "installed" only if the uninstaller exists.
    return QFileInfo{uninstall_exe}.isFile();
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
    localfiles.clear(); // collect files added outside the app bundle, and registry entries
    return true;
}

bool Installer::registerApp()
{
    QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
    QString app{ui->versioned->isChecked() ? versioned_app : appinfo.APPNAME};
    // Register uninstaller.
    QString app_uninstall_reg{"Microsoft/Windows/CurrentVersion/Uninstall/" + app};
    regsettings.beginGroup(app_uninstall_reg);
    localfiles.append("+++reg:" + app_uninstall_reg);

    // The app folders are versioned regardless of whether the installation is versioned.
    QString idir{QDir::toNativeSeparators(app_dir.path())};

    print_line(tr("Register app and uninstaller"));
    regsettings.setValue("DisplayName", app);
    regsettings.setValue("UninstallString", '"' + idir + '\\' + UNINSTALLER_NAME + '"');
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
    regsettings.setValue("EstimatedSize", static_cast<quint32>(size / 1024)); // Windows uses KiB (1024 bytes)

    regsettings.endGroup();

    // File-type association
    if ( !appinfo.ASSOC_EXT.isEmpty() && !ui->versioned->isChecked() ) {
        print_line(tr("Register file-type association"));
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

        localfiles.append("+++file-type:" + appinfo.ASSOC_EXT);
    }

    // Start menu entry
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    desktopName = app + ".lnk";
    desktopPath = app_dir.absoluteFilePath(appinfo.APPEXEC);
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

QString Installer::get_execpath(QDir dir)
{
    return dir.absoluteFilePath(appinfo.APPEXEC);
}

void Installer::update_desktop_database()
{
    // No action is necessary.
}

#elif defined Q_OS_LINUX
#include <QXmlStreamReader>

static const char *OTHER_INSTALLATION = QT_TRANSLATE_NOOP("Installer", R"(
"'%1' is already in the PATH: '%2'

Apparently it has already been installed in some other way.)");

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
        uinfo = QFileInfo{dir + "/bin/app_uninstall"};
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
        // Add versioned links to executables.
        // Note that there will be no mime type or icons if there is no unversioned installation!
        for ( const auto &b : std::as_const(appinfo.BINLINKS) ) {
            if ( execInPath(b + "-" + appinfo.APPVERSION) )
                ok = false;

            QString execrpath{"bin/" + b};
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
        // The "Exec", "Icon" and "Name" fields needs editing.
        QString appicon;
        if ( !desktop_icon(appicon) ) {
            ok = false;
        } else {
            desktopName = appinfo.APPLICATION + "-" + appinfo.APPVERSION + ".desktop";
            desktopPath = appsDir.absoluteFilePath(desktopName);
            QString dtfile{src_dir.absoluteFilePath("share/applications/" + appinfo.APPLICATION + ".desktop")};
            // The "Exec" field needs editing.
            QFile f(dtfile);
            if ( !f.open(QFile::ReadOnly | QFile::Text) ) {
                print_line("BUG, Repeated read in installation bundle failed: share/applications/"
                               + appinfo.APPLICATION + ".desktop", true);
                ok = false;
            } else {
                QTextStream in(&f);
                QStringList newlines;
                while (!in.atEnd())
                {
                    QString line = in.readLine();
                    if ( line.startsWith("Exec=") ) {
                        newlines.append(
                            "Exec=" + appinfo.EXECLINE.replace(
                                "%APP%", appinfo.APPEXEC + "-" + appinfo.APPVERSION));
                        continue;
                    }
                    if ( line.startsWith("Name=") ) {
                        newlines.append("Name=" + versioned_app);
                        continue;
                    }
                    if ( line.startsWith("Icon=") && !appicon.isEmpty() ) {
                        // Use icon's absolute path.
                        QString iconpathx{"share/icons/hicolor/scalable/apps/" + appicon + ".svg"};
                        QString sfile{src_dir.absoluteFilePath(iconpathx)};
                        if ( !QFileInfo::exists(sfile) ) {
                            print_line(tr("Icon missing in installation bundle: '%1'").arg(iconpathx), true);
                            ok = false;
                        } else {
                            line = "Icon=" + app_dir.absoluteFilePath(iconpathx);
                        }
                    }
                    newlines.append(line);
                }
                f.close();
                registrationList.append(
                    {newlines.join("\n") + "\n", desktopPath,
                     tr("Add 'Start' menu entry: '%1'").arg(desktopPath), R_WRITE});
            }
        }
    } else {
        // Add "unversioned" links to executables.
        for ( const auto &b : std::as_const(appinfo.BINLINKS) ) {
            if ( execInPath(b) )
                ok = false;

            QString execrpath{"bin/" + b};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{src_dir.filePath(execrpath)}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: '%1'").arg(execrpath), true);
                ok = false;
            }
            QString appexecpath{binDir.filePath(b)};
            registrationList.append(
                {execfile, appexecpath, tr("Add executable to PATH: '%1'").arg(appexecpath), R_LINK});
        }

        QStringList icons; // collect icon names to register

        // Add unversioned .desktop file (for "Start" menu entry).
        QString appicon;
        if ( !desktop_icon(appicon) ) {
            ok = false;
        } else {
            desktopName = appinfo.APPLICATION + ".desktop";
            desktopPath = appsDir.absoluteFilePath(desktopName);
            registrationList.append(
                {app_dir.absoluteFilePath("share/applications/" + desktopName), desktopPath,
                 tr("Add 'Start' menu entry: '%1'").arg(desktopPath), R_COPY});
            if ( !appicon.isEmpty() ) {
                icons.append(appicon);
            }
        }

        // Add mime-type and icons, if available
        if ( !appinfo.APPMIME.isEmpty() ) {
            QString mrpath{"mime/packages/" + appinfo.APPMIME + ".xml"};
            QString mxrpath{"share/" + mrpath};
            QString mdfile{app_dir.absoluteFilePath(mxrpath)};
            QString msfile{src_dir.absoluteFilePath(mxrpath)};
            if ( !QFileInfo{msfile}.isFile() ) {
                print_line(tr("Mime-type file missing in installation bundle: '%1'").arg(mxrpath), true);
                ok = false;
            } else {
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
                        localfiles.append("+++file-type:" + appinfo.APPMIME); // uninstaller flag
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
        if ( !icons.isEmpty() ) {
            localfiles.append("+++icons"); // uninstaller flag
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

bool Installer::desktop_icon(QString &icon)
{
    // Get Icon name from .desktop file if there is a corresponding svg icon in the app bundle.
    // The name is stored to the 'icon' parameter. Return 'false' if an error occurred.
    // Note that the result can be 'true' but not set 'icon' (if the icon is not provided at
    // the correct location in the app bundle.
    QString appsrpath{"share/applications/" + appinfo.APPLICATION + ".desktop"};
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
            QString iconpath{"icons/hicolor/scalable/apps/" + line + ".svg"};
            QString sfile{src_dir.absoluteFilePath("share/" + iconpath)};
            if ( QFileInfo::exists(sfile) ) {
                icon = line;
                break;
            }
        }
    }
    f.close();
    return true;
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

void Installer::addedIcons()
{
    print_line(tr("Update icon cache"));
    QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
}

void Installer::addedFileType()
{
    print_line(tr("Update mime database"));
    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
}

bool Installer::removeSpecial(QString item)
{
    return true; // there are no entries which need acting on here
}

QString Installer::get_execpath(QDir dir)
{
    return dir.absoluteFilePath("bin/" + appinfo.APPEXEC);
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
