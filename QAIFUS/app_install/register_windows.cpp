#include "installer.h"
#include "ui_installer.h"
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSettings>
#include <QSaveFile>

#if defined Q_OS_WIN
#include <windows.h>
#include <shlobj.h>

// Update file-type associations and desktop files.
// Windows: add app to registry.

// All registration failures should abort the installation process, except
// these, which just report the failure but don't prevent the installation
// completing:
//  - a desktop starter cannot be installed

#define UNINSTALLER_NAME "app_uninstall0.exe"

/* TODO: Add executables to PATH by means of batch files (.cmd) in a PATH directory.
 * The path encoding should be UTF-8. However, there needs to be an easy way of
 * identifying the installation directory. Also, for GUI executables it might
 * be sensible to remove the "/WAIT" option, to avoid unwanted console windows.
 * That assumes of course that there is no need to wait for the GUI app to end.
 */

static const char *BATCH_STARTER = R"(@echo off
chcp 65001 >NUL
set APPBASE=ppp
start "" /B /WAIT "%APPBASE%\xxx.exe" %*
)";

bool Installer::isInstalled()
{
    // Check for registry uninstaller entry for the package name.
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
    QDir udir{QDir::fromNativeSeparators(ustring.toString())};
    uninstall_exe = udir.absoluteFilePath(UNINSTALLER_NAME);
    if ( QFileInfo{uninstall_exe}.isFile() )
        uninstall_name = udir.dirName();
        return true;

    // Check each of the EXECUTABLES:
    //   - Is it a batch file? No: not interesting.
    //   - Yes: check structure for uninstaller target.
    // Assume an uninstaller, if there is one, is in the same directory as
    // the executable (the actual one, not the batch starter).
    // Return true if one of the found executables is a batch file with a
    // path to an app bundle with an uninstaller in the expected place.
    // This alone is no guarantee that the uninstaller will clear the way
    // for an installation, but generally it will do.

    // Collect the executable names for the start-menu test later.
    QStringList enames;
    for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
        QString exec = i.key();
        QString ename = i.value().execName;
        if ( ui->versioned->isChecked() ) {
            exec += "-" + appinfo.PROJECTVERSION;
            ename += "-" + appinfo.PROJECTVERSION;
        }
        QString execpath = QStandardPaths::findExecutable(exec);
        if ( !execpath.isEmpty() ) {
            // Only if this is a particularly structured ".cmd" can the
            // app count as "installed" in this sense. But if there is
            // an executable, it is probably sensible to avoid possible
            // confusion by blocking the new installation.
            if ( execpath.endsWith(".cmd") ) {
                // Read file, seek special line with APPBASE.
                QFile file(execpath);
                if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) )
                    continue; //TODO? report the failure?
                QTextStream in(&file);
                int l = 0;
                while ( !in.atEnd() ) {
                    QString line = in.readLine();
                    auto i = line.indexOf("APPBASE=");
                    if ( i >= 0 ) {
                        QString appbase = QDir::fromNativeSeparators(line.slice(i + 8).trimmed());
                        // Check validity of path
                        if ( QFileInfo{appbase}.isDir() ) {
                            QString x{QStandardPaths::findExecutable(
                                UNINSTALLER_NAME,
                                {appbase})};
                            if ( !x.isEmpty() ) {
                                uninstall_exe = x;
                                uninstall_name = QDir{appbase}.dirName();
                                return true;
                            }
                        }
                        // It is perhaps a bit unexpected to land here,
                        // but not totally impossible. Don't look for
                        // further matching lines.
                        break;
                    }
                    if ( ++l > 4 ) // limit the number of lines read
                        break;
                }
            }

            // Count execpath as a blocker.
            blockers.append(execpath);
        }
    }

    // Search for start-menu items sharing a name with those from this package.
    // 1. Inspect the start-menu items specific to the current user.
    for ( const auto& f : QDirListing(
        QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation),
        QDirListing::IteratorFlag::Recursive) )
    {
        if ( enames.contains(f.completeBaseName()) )
            blockers.append(f.absoluteFilePath());
    }
    // 2. Now inspect system entries.
    for ( const auto& f : QDirListing(
        "C:/ProgramData/Microsoft/Windows/Start Menu/Programs",
        QDirListing::IteratorFlag::Recursive) )
    {
        if ( enames.contains(f.completeBaseName()) )
            blockers.append(f.absoluteFilePath());
    }

    return false;
}

bool Installer::dirHasUninstaller(QString dir)
{
    uninstall_exe = dir + "/" + UNINSTALLER_NAME;
    uninstall_name = QDir{dir}.dirName();
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

class Registration
{
    struct reg_item {
        enum{GROUP, ENDGROUP, SET} type;
        QString key;
        QVariant val;
    };

    QList<reg_item> items;

public:
    void beginGroup(QString name)
    {
        items.append(reg_item{reg_item::GROUP, name, {}});
    }

    void endGroup()
    {
        items.append(reg_item{reg_item::ENDGROUP, {}, {}});
    }

    void setValue(QString key, QVariant val)
    {
        items.append(reg_item{reg_item::SET, key, val});
    }

    QString registerItems()
    {
        QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
        for ( const auto& item : std::as_const(items) ) {
            switch ( item.type ) {

            case reg_item::GROUP:
                regsettings.beginGroup(item.key);
                break;
            case reg_item::ENDGROUP:
                regsettings.endGroup();
                break;
            case reg_item::SET:
                regsettings.setValue(item.key, item.val);
                // Try to be sure that the writes have worked.
                regsettings.sync();
                if ( regsettings.status() != QSettings::NoError
                        || regsettings.value(item.key) != item.val ) {
                    return item.key + "=" + item.val.toString();
                }
                break;

            }
        }
        return {};
    }
};

bool Installer::registerApp()
{
    Registration reg;
    QString proj{appinfo.PROJECTNAME};
    if ( ui->versioned->isChecked() ) {
        proj += "-" + appinfo.PROJECTVERSION;
    }
    // Register uninstaller.
    QString app_uninstall_reg{"Microsoft/Windows/CurrentVersion/Uninstall/" + proj};

    reg.beginGroup(app_uninstall_reg);

    localfiles.append("+++reg:" + app_uninstall_reg);

    // The app folders are versioned regardless of whether the installation is versioned.
    QString idir{QDir::toNativeSeparators(app_dir.absolutePath())};

    print_line(tr("Register project and uninstaller"));
    reg.setValue("DisplayName", proj);
    reg.setValue("UninstallString", '"' + idir + '\\' + UNINSTALLER_NAME + '"');
    reg.setValue("Publisher", appinfo.Publisher);
    reg.setValue("WebLink", appinfo.WebLink);
    reg.setValue("DisplayVersion", appinfo.PROJECTVERSION);
    reg.setValue("UninstallLocation", idir);
    // Installation size
    qint64 size{0};
    for ( const auto &dirEntry : QDirListing(
             app_dir.path(),
             QDirListing::IteratorFlag::FilesOnly
                 | QDirListing::IteratorFlag::Recursive
                 | QDirListing::IteratorFlag::IncludeHidden) ) { //TODO: IncludeHidden?
        size += dirEntry.size();
    }
    reg.setValue("EstimatedSize", static_cast<quint32>(size / 1024)); // Windows uses KiB (1024 bytes)

    reg.endGroup();

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
                reg.setValue(key_c1, "");

                QString key_c2{key_c + assoc_progid};
                localfiles.append("+++reg:" + key_c2);
                reg.beginGroup(key_c2);
                reg.setValue("shell/open/FriendlyAppName", exec + " " + appinfo.PROJECTVERSION);
                QString appPath{"\"" + idir + "\\" + exec + execval.extension + "\""};
                reg.setValue("shell/open/command/.", appPath + " \"%1\"");
                reg.setValue("DefaultIcon/.", appPath + ",0");
                reg.endGroup();

                localfiles.append("+++file-type:" + assoc_ext);
            }
        }
    }

    // Do the actual registry entries.
    QString fail = reg.registerItems();
    if ( !fail.isEmpty() ) {
        print_line(tr("Registry entry failed: %1").arg(fail));
        return false;
    }

    // Start menu entries
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};

    // If more than one entry, make a group.
    {
        bool sme{false};
        for (auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i) {
            if ( !i.value().execName.isEmpty() ) {
                if ( sme ) {
                    // Make a start menu group.
                    if ( appsDir.mkdir(proj) ) {
                        appsDir.cd(proj);
                        localfiles.append("+++dir:" + appsDir.absolutePath());
                        break;
                    }
                    print_line(tr("Couldn't create start menu group: '%1'")
                        .arg(appsDir.absoluteFilePath(proj)), true);
                    return false;
                } else {
                    sme = true;
                }
            }
        }
    }

    // Get the "standard" user entry in PATH.
    QDir cmdDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
    cmdDir.cd("Microsoft/WindowsApps");
    for ( auto i = appinfo.EXECUTABLES.cbegin(), end = appinfo.EXECUTABLES.cend(); i != end; ++i )
    {
        QString exec = i.key();
        auto execval = i.value();
        QString desktopName{execval.execName};

        // Install a batch starter in cmdDir.
        QString exec_exe{exec + execval.extension};
        QString starter{BATCH_STARTER};
        starter.replace("ppp", idir).replace("xxx.exe", exec_exe);
        if ( !desktopName.isEmpty() ) {
            // Assume that this is a GUI application and remove " /WAIT"
            starter.replace(" /WAIT", "");
        }
        QString cmd_name{exec};
        if ( ui->versioned->isChecked() ) {
            cmd_name += "-" + appinfo.PROJECTVERSION;
        }
        QString outfile{cmdDir.absoluteFilePath(cmd_name + ".cmd")};
        if ( QFileInfo::exists(outfile) ) {
            print_line(tr("Starter exists already: %1").arg(outfile), true);
            return false;
        }
        QFile file(outfile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << starter;
            file.close();
            localfiles.append(outfile);
        } else {
            print_line(tr("Couldn't write starter: %1").arg(outfile), true);
            return false;
        }

        if ( desktopName.isEmpty() ) {
            continue;
        }
        QString desktopPath{app_dir.absoluteFilePath(exec_exe)};
        if ( ui->versioned->isChecked() ) {
            desktopName += "-" + appinfo.PROJECTVERSION;
        }
        desktopName += ".lnk";
        QString link{appsDir.absoluteFilePath(desktopName)};
        if ( QFile::link(desktopPath, link) ) {
            print_line(tr("Install start menu entry: '%1'").arg(desktopName));
            localfiles.append(link);
        } else {
            print_line(tr("Couldn't install start menu entry: %1").arg(link), true);
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
                print_line(tr("Couldn't install desktop starter: %1").arg(link), true);
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
    // There is no return value.
}

QStringList Installer::removeSpecials(QStringList specials)
{
    QStringList failed;
    QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
    for ( const auto& line : specials ) {
        if ( line.startsWith("+++reg:") ) {
            auto key = line.sliced(7);
            regsettings.remove(key);
            // Try to be sure that the removal has worked.
            regsettings.sync();
            if ( regsettings.status() != QSettings::NoError
                    || regsettings.contains(key) ) {
                failed.append(line);
            }
        } else if ( line.startsWith("+++file-type:") ) {
            QString key{"Classes/" + line.sliced(13)};
            regsettings.beginGroup(key + "/OpenWithProgIds");
            bool empty{regsettings.childKeys().isEmpty()};
            regsettings.endGroup();
            if ( empty && !regsettings.value(key + "/.").isValid() ) {
                regsettings.remove(key);
                // Try to be sure that the removal has worked.
                regsettings.sync();
                if ( regsettings.status() != QSettings::NoError
                        || regsettings.contains(key) ) {
                    failed.append(line);
                }
            }
        } else if ( line.startsWith("+++dir:") ) {
            QString dirPath{line.sliced(7)};
            if ( QDir{}.rmdir(dirPath) ) {
                print_line(tr("Removed %1").arg(dirPath));
            } else {
                failed.append(dirPath);
            }
        }
    }
    return failed;
}

bool Installer::isInstallBundle(QDir dir)
{
    return dir.exists(UNINSTALLER_NAME);
}

void Installer::update_desktop_database()
{
    // No action is necessary.
}

#endif
