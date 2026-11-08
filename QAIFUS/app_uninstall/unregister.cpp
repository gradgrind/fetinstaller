#include "uninstaller.h"
#include <QProcess>
#include <QStandardPaths>

// Update file-type associations and desktop files.
// Windows: remove app from registry.

void Uninstaller::unregisterApp()
{
    // Read the list of files installed outside the app bundle and also "special" entries.
    QString installed_files_path{base_dir.filePath("_installer_/system_files")};
    QFile textFile{installed_files_path};
    if ( !textFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        print_line(tr("App installed no system files"));
        print_line("");
        return;
    }
    // Read file line by line
    QTextStream in(&textFile);
    QStringList newlines;
    QStringList specials;
    bool appregistered{false};
    while ( !in.atEnd() )
    {
        QString line = in.readLine();
        //qDebug() << "???" << line;
        QString fpath{line.trimmed()};
        if ( !fpath.isEmpty() ) {
            if ( fpath.startsWith("+++") ) {
                specials.append(fpath);
            } else if ( QFile::remove(fpath) ) {
                print_line(tr("Removed %1").arg(fpath));
            } else {
                unregister_failed.append(fpath);
            }
        }
    }
    textFile.close();

    removeSpecials(specials);
    print_line("");
}

#if defined Q_OS_WIN
#include <QSettings>
#include <windows.h>
#include <shlobj.h>

void Uninstaller::removeSpecials(const QStringList specials)
{
    bool shchangenotify{false};
    QSettings regsettings{"HKEY_CURRENT_USER\\Software", QSettings::NativeFormat};
    for ( const auto& line : specials ) {
        if ( line.startsWith("+++reg:") ) {
            auto key = line.sliced(7);
            regsettings.remove(key);
            if ( regsettings.contains(key) ) {
                unregister_failed.append(line);
            }
        } else if ( line.startsWith("+++file-type:") ) {
            QString key{"Classes/" + line.sliced(13)};
            regsettings.beginGroup(key + "/OpenWithProgIds");
            bool empty{regsettings.childKeys().isEmpty()};
            regsettings.endGroup();
            if ( empty && !regsettings.value(key + "/.").isValid() ) {
                regsettings.remove(key);
                if ( regsettings.contains(key) ) {
                    unregister_failed.append(line);
                }
            }
            shchangenotify = true;
        } else if ( line.startsWith("+++dir:") ) {
            QString dirPath{line.sliced(7)};
            if ( QDir{}.rmdir(dirPath) ) {
                print_line(tr("Removed %1").arg(dirPath));
            } else {
                unregister_failed.append(dirPath);
            }
        }
    }
    if ( shchangenotify ) {
        // The SHChangeNotify function allows the association of the
        // icons with the file type to be changed immediately.
        SHChangeNotify(
            SHCNE_ASSOCCHANGED,     // Event ID
            SHCNF_IDLIST,           // Flags
            NULL,                   // Item that changed
            NULL                    // Not used
            );
    }
}

#elif defined Q_OS_LINUX

void Uninstaller::removeSpecials(const QStringList specials)
{
    for ( const auto& line : specials ) {
        if ( line.startsWith("+++file-type:") ) {
            QDir shareDir{QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
            print_line(tr("Update icon cache"));
            QProcess::execute("xdg-icon-resource", QStringList{"forceupdate"});
            print_line(tr("Update mime database"));
            QProcess::execute("update-mime-database", QStringList{shareDir.filePath("mime")});
        }
    }
    QDir appsDir{QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation)};
    print_line(tr("Update desktop database"));
    QProcess::execute("update-desktop-database", QStringList{appsDir.path()});
}

#else

//TODO

#endif
