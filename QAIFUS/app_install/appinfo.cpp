#include "appinfo.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QMessageBox>

AppInfo::AppInfo()
{
    // Get source path
    SOURCE_DIR.setPath(QFileInfo(QCoreApplication::applicationFilePath()).canonicalPath());
    if (QFileInfo::exists(SOURCE_DIR.filePath("app_bundle"))) {
        // Accept an "app_bundle" directory in the same directory as the installer executable
        SOURCE_DIR.cd("app_bundle");
    } else {
#if defined Q_OS_WIN
        // Assume the installer executable is in the root directory of the source directory
#else
        // Assume the installer executable is in the "_installer_" directory of the source directory
        SOURCE_DIR.cdUp();
#endif
    }
    // Path prefixes for OS-specific configuration items
#if defined Q_OS_WIN
    ostype = "Windows";
#elif defined Q_OS_LINUX
    ostype = "Linux";
#endif
}

bool AppInfo::read0(QSettings& appsettings, QString& var, QString key, bool critical)
{
    auto val = appsettings.value(key);
    if ( val.isValid() ) {
        var = val.toString();
        return true;
    }
    if ( critical ) {
        QMessageBox::critical(
            nullptr,
            tr("Critical"),
            tr("Configuration Error, no '%1': '%2'").arg(key, appconf));
    }
    return false;
}

bool AppInfo::reados(QSettings& appsettings, QString& var, QString key)
// Read a setting from the `ostype` section.
{
    appsettings.beginGroup(ostype);
    auto val = appsettings.value(key);
    appsettings.endGroup();
    if ( val.isValid() ) {
        var = val.toString();
        return true;
    }
    return false;
}

bool AppInfo::read(QSettings& appsettings, QString& var, QString key, bool critical)
// Read a setting, trying the `ostype` section first, falling back to the top level.
{
    if ( reados(appsettings, var, key) ) {
        return true;
    }
    return read0(appsettings, var, key, critical);
}

bool AppInfo::init(QLocale locale)
{
    QString about_qaifus = SOURCE_DIR.filePath("_installer_/ABOUT_QAIFUS");
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

    appconf = SOURCE_DIR.filePath("_installer_/app.conf");
    if ( !QFileInfo{appconf}.isFile() ) {
        QMessageBox::critical(nullptr, tr("Critical"), tr("BUG, no application configuration: %1").arg(appconf));
        return false;
    }
    QSettings appsettings{appconf, QSettings::IniFormat};

    if ( !read(appsettings, PROJECTNAME, "PROJECT", true) )
        return false;

    if ( !read(appsettings, PROJECTVERSION, "VERSION", true) )
        return false;

    // PROJECTLONGNAME is optional, if possible, get a translated version.
    if ( !read(
            appsettings,
            PROJECTLONGNAME,
            "GenericName[" + locale.name() + "]") ) {
        if ( !read(
                appsettings,
                PROJECTLONGNAME,
                "GenericName[" + QLocale::languageToCode(locale.language()) + "]") ) {
            read(appsettings, PROJECTLONGNAME, "GenericName");
        }
    }

    // Read the entries for the executables, only in the `ostype` section.
    appsettings.beginGroup(ostype);
    appsettings.beginGroup("EXEC");
    for (const auto& k : appsettings.allKeys()) {
        QString v{appsettings.value(k).toString().trimmed()};
        QStringList vvv = v.split(':');
        if ( vvv.length() != 3 ) {
            QMessageBox::critical(
                nullptr,
                tr("Critical"),
                tr("Invalid EXEC in app.conf: (%1) %2=%3").arg(ostype, k, v));
            return false;
        }
        EXECUTABLES[k] = {vvv[0], vvv[1], vvv[2]};
    }
    appsettings.endGroup();
    appsettings.endGroup();

    // Get "LAUNCH" app.
    if ( read(appsettings, LAUNCH, "LAUNCH") ) {
        if ( !EXECUTABLES.contains(LAUNCH) ) {
            QMessageBox::critical(
                nullptr,
                tr("Critical"),
                tr("No 'EXEC' for LAUNCH=%1 in app.conf.").arg(LAUNCH));
            LAUNCH.clear();
            return false;
        }
    }

    read(appsettings, Publisher, "Publisher");
    read(appsettings, WebLink, "WebLink");

    return true;
}
