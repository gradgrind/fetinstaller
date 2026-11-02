#include "appinfo.h"
#include <QCoreApplication>
#include <QMessageBox>
#include <QSettings>

#if defined Q_OS_WIN
#include <process.h>
#endif

AppInfo::AppInfo() {}

bool AppInfo::init()
{
    // If there is a command-line argument it must be the address of the installation to remove.
    // This is only supported on Windows, where a program cannot delete its own files.
    // On Windows "app_uninstall0.exe" will copy the uninstaller, together with necessary
    // library and plugin files, to a temporary directory. It will then run "app_uninstall.exe"
    // in the temporary directory, passing the path to the installation to be removed as argument.
    // The switch is done using the C function "_execl" so that the installer replaces the
    // pre-installer.

    // Determine the installation's base directory, starting with the base directory of the
    // uninstaller (this program): "appdir".
    appdir.setPath(QFileInfo(QCoreApplication::applicationFilePath()).canonicalPath());
#if defined Q_OS_WIN
    // The uninstaller is expected to be in the root directory of the installation.
#else
    // The uninstaller is expected to be in the "_installer_" directory of the installation.
    appdir.cdUp();
#endif

    QStringList args = QCoreApplication::arguments();
    if ( args.length() == 1 ) {
#if defined Q_OS_WIN
        // Get target
        QString target{QFile::symLinkTarget(appdir.filePath("UNINSTALL_TARGET.lnk"))};
        if ( target.isEmpty() ) {
            error = tr("Missing link: '%1'").arg(appdir.filePath("UNINSTALL_TARGET.lnk"));
            return false;
        } else {
            // A simple test for an application folder:
            if ( QFileInfo::exists(target + "/app_uninstall.exe") ) {
                basedir.setPath(target);
            } else {
                error = tr("Invalid application folder: '%1'").arg(target);
                return false;
            }
        }
#else
        basedir = appdir; // The uninstaller should delete itself.
#endif
    } else {
        error = tr("Invalid command line:\n  - %1").arg(args.join("\n  - "));
        return false;
    }
    basename = basedir.dirName();

    QString about_qaifus = basedir.filePath("_installer_/ABOUT_QAIFUS");
    if ( !QFileInfo{about_qaifus}.isFile() ) {
        QMessageBox::critical(nullptr, tr("Critical"), tr("BUG, no ABOUT_QAIFUS file: %1").arg(about_qaifus));
        return false;
    }
    QSettings qinfo{about_qaifus, QSettings::IniFormat};
    {
        auto val = qinfo.value("QAIFUS_VERSION");
        if ( val.isValid() ) {
            QAIFUS_VERSION = val.toString();
        } else {
            QMessageBox::critical(nullptr, tr("Critical"), tr("BUG, no QAIFUS_VERSION: %1").arg(about_qaifus));
            return false;
        }
        val = qinfo.value("QAIFUS_INFO");
        if ( val.isValid() ) {
            QAIFUS_INFO = val.toString();
        } else {
            QMessageBox::critical(nullptr, tr("Critical"), tr("BUG, no QAIFUS_INFO: %1").arg(about_qaifus));
            return false;
        }
    }

    return true;
}
