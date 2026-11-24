#include "installer.h"
#include "ui_installer.h"
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSettings>
#include <QSaveFile>

#if defined Q_OS_LINUX
#include <QXmlStreamReader>

// Update file-type associations and desktop files.

// All registration failures should abort the installation process, except
// these, which just report the failure but don't prevent the installation
// completing:
//  - a desktop starter cannot be installed

#define UNINSTALLER_NAME "_installer_/app_uninstall"

static const char *OTHER_INSTALLATION = QT_TRANSLATE_NOOP("Installer", R"(
'%1' is already in the PATH: %2

Apparently it has already been installed in some other way.)");

bool Installer::getUninstaller(QString xpath)
{
    // Seek installation directory, and hence uninstaller.
    QDir ubase{QFileInfo{xpath}.absolutePath()};
    ubase.cdUp();
    QFileInfo uinfo = QFileInfo{ubase.absoluteFilePath(UNINSTALLER_NAME)};
    if ( uinfo.isExecutable() ) {
        uninstall_exe = uinfo.absoluteFilePath();
        uninstall_name = ubase.dirName();
        // The app counts as "installed" if this exists and is executable.
        return true;
    }
    return false;
}

bool Installer::isInstalled()
{
    // Assume an uninstaller is in the _installer_ directory.
    // Return true if one of the executables is a symlink to an app bundle
    // with an uninstaller in the expected place. This alone is no guarantee
    // that the uninstaller will clear the way for an installation, but generally
    // it will do.

    // Check each of the EXECUTABLES:
    //   - Is it a symlink? No: not interesting.
    //   - Yes: check for uninstaller in its target.
    // Collect the executable names for the start-menu test later.
    QStringList enames;
    // Collect the desktop file names.
    QStringList dnames;
    for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
        QString exec = i.key();
        QString ename = i.value().execName;
        if ( ui->versioned->isChecked() ) {
            exec += "-" + appinfo.PROJECTVERSION;
            if ( !ename.isEmpty() )
                ename += "-" + appinfo.PROJECTVERSION;
        }
        if ( !ename.isEmpty() ) {
            enames.append(ename);
            dnames.append(exec + ".desktop");
        }
        exec += i.value().extension;
        QString execpath = QStandardPaths::findExecutable(exec);
        if ( !execpath.isEmpty() ) {
            // Only if this is a symlink can the app count as "installed" in this sense.
            // But if there is an executable, it is probably sensible to avoid possible
            // confusion by blocking the new installation.
            QString xpath{QFileInfo{execpath}.readSymLink()};
            if ( !xpath.isEmpty() && getUninstaller(xpath) )
                return true;
            // Count execpath as a blocker.
            blockers.append(execpath);
        }
        // It is possible that "$HOME/.local/bin/" is not (yet) in the PATH, so check
        // that explicitly.
        execpath = QStandardPaths::findExecutable(
            exec,
            {QDir::home().absoluteFilePath(".local/bin")}
        );
        if ( !execpath.isEmpty() ) {
            QString xpath = QFileInfo{execpath}.readSymLink();
            if ( !xpath.isEmpty() && getUninstaller(xpath) )
                return true;
            if ( !blockers.contains(execpath) )
                blockers.append(execpath);
        }
    }

    // Search for start-menu items sharing a name with those from this package.
    // Consider both desktop file names and their "Name=" entries.
    for ( const auto& smpath : QStandardPaths::standardLocations(
        QStandardPaths::ApplicationsLocation) )
    {
        for ( const auto& f : QDirListing(
            smpath,
            QDirListing::IteratorFlag::FilesOnly) )
        {
            QString dpath{f.absoluteFilePath()};
            if ( dnames.contains(f.fileName()) ) {
                blockers.append(dpath);
            } else {
                // Check the display name.
                QFile file(dpath);
                if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) )
                    continue; //TODO? report the failure?
                QTextStream in(&file);
                while ( !in.atEnd() ) {
                    QString line = in.readLine();
                    if ( line.startsWith("Name=") ) {
                        if ( enames.contains(line.slice(5).trimmed()) )
                            blockers.append(dpath);
                        break;
                    }
                }
                file.close();
            }
        }
    }

    return false;
}

bool Installer::dirHasUninstaller(QString dir)
{
    QDir udir{dir};
    QFileInfo uinfo = QFileInfo{udir.absoluteFilePath(UNINSTALLER_NAME)};
    // The app counts as "installed" only if this exists and is executable.
    if ( uinfo.isExecutable() ) {
        uninstall_exe = uinfo.filePath();
        uninstall_name = udir.dirName();
        return true;
    }
    return false;
}

bool Installer::hasFileTypeAssociation()
{
    return !QDir{src_dir.absoluteFilePath("share/mime/packages")}.isEmpty();
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
            QString execfull{execbase + execval.extension};
            QString execrpath{"bin/" + execfull};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{src_dir.filePath(execrpath)}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: %1").arg(execrpath), true);
                ok = false;
            }
            QString appexecpath{binDir.filePath(execv)};
            registrationList.append(
                {execfile, appexecpath, tr("Add executable to PATH: %1").arg(appexecpath), R_LINK});

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
                             tr("Add 'Start' menu entry: %1").arg(desktopPath), R_WRITE});
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
            QString execrpath{"bin/" + execfull};
            QString execfile{app_dir.absoluteFilePath(execrpath)};
            if ( !QFileInfo{src_dir.filePath(execrpath)}.isExecutable() ) {
                print_line(tr("Executable missing in installation bundle: %1").arg(execrpath), true);
                ok = false;
            }
            QString appexecpath{binDir.filePath(execfull)};
            registrationList.append(
                {execfile, appexecpath, tr("Add executable to PATH: %1").arg(appexecpath), R_LINK});

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
                         tr("Add 'Start' menu entry: %1").arg(desktopPath), R_COPY});
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
                            print_line(tr("Couldn't read mime-type file in installation bundle: %1").arg(mxrpath), true);
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
                                print_line(tr("Invalid mime-type file in installation bundle: %1").arg(mxrpath), true);
                                print_line(tr("  ... in line %1: %2").arg(xml.lineNumber()).arg(xml.errorString()), true);
                                ok = false;
                            } else {
                                // Register the mime file.
                                QString mime_path{shareDir.absoluteFilePath(mrpath)};
                                registrationList.append(
                                    {mdfile, mime_path, tr("Install mime file: %1").arg(mime_path), R_COPY});
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
                print_line(tr("Icon missing in installation bundle: %1").arg(iconpathx), true);
                ok = false;
            } else {
                QString dpath{shareDir.absoluteFilePath(iconpath)};
                registrationList.append(
                    {app_dir.absoluteFilePath(iconpathx), dpath, tr("Add icon file: %1").arg(dpath), R_LINK});
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
                {sfile, dpath, tr("Add man file: %1").arg(dpath), R_LINK});
        }
    } // END UNVERSIONED INSTALLATION

    for ( const auto& reg : std::as_const(registrationList) ) {
        if ( QFileInfo::exists(reg.destination) || QFileInfo{reg.destination}.isSymbolicLink() ) {
            print_line(tr("Can't install item to: %1").arg(reg.destination), true);
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
        print_line(tr("'Desktop' file missing in installation bundle: %1").arg(appsrpath), true);
        return false;
    }
    // Read the file, seek the "Icon=" line.
    QFile f(dtfile);
    if ( !f.open(QFile::ReadOnly | QFile::Text) ) {
        print_line(tr("Couldn't read 'desktop' file in installation bundle: %1").arg(appsrpath), true);
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
                print_line(tr("Warning, icon missing in installation bundle: %1").arg(iconpath), true);
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
            QSaveFile fw(reg.destination);
            if ( fw.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text) ) {
                QTextStream out(&fw);
                out << reg.source;
                if ( fw.commit() ) {
                    localfiles.append(reg.destination);
                    print_line(reg.message);
                    continue;
                }
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
                        print_line(tr("Failed to make desktop starter executable: %1").arg(dlpath), true);
                    }
                }
            } else {
                print_line(tr("Couldn't install desktop starter: %1").arg(dlpath), true);
            }
        }
    }
    return true;
}

void Installer::addedFileType()
{
    if ( QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"}) == 0 ) {
        print_line(tr("Updated icon cache"));
    } else {
        print_line(tr("Failed: Update icon cache"), true);
    }

    QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    if ( QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")}) == 0 ) {
        print_line(tr("Updated mime database"));
    } else {
        print_line(tr("Failed: Update mime database"), true);
    }
}

QStringList Installer::removeSpecials(QStringList specials)
{
    return {}; // there are no entries which need acting on here
}

bool Installer::isInstallBundle(QDir dir)
{
    return dir.exists(UNINSTALLER_NAME);
}

void Installer::update_desktop_database()
{
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    if ( QProcess::execute("update-desktop-database", QStringList{appsDir.path()}) == 0 ) {
        print_line(tr("Updated desktop database"));;
    } else {
        print_line(tr("Failed: Update desktop database"), true);
    }
}

#endif
