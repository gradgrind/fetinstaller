#include "appinfo.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
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
    ostype = "Windows/";
#elif defined Q_OS_LINUX
    ostype = "Linux/";
#endif
}

bool AppInfo::read(QString& var, QString key, bool critical)
{
    auto val = appsettings->value(ostype + key);
    if ( val.isValid() ) {
        var = val.toString();
        return true;
    } else {
        val = appsettings->value(key);
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
}

bool AppInfo::init(QLocale locale)
{
    appconf = SOURCE_DIR.filePath("_installer_/app.conf");
    if ( !QFileInfo{appconf}.isFile() ) {
        QMessageBox::critical(nullptr, tr("Critical"), tr("BUG, no application configuration: %1").arg(appconf));
        return false;
    }

    appsettings = new QSettings(appconf, QSettings::IniFormat);

    if ( !read(APPNAME, "APPNAME", true) )
        return false;

    if ( !read(EXECDIR, "EXECDIR", true) )
        return false;

    if ( !read(APPVERSION, "VERSION", true) )
        return false;

    if ( !read(APPLICATION, "APPLICATION") ) {
        APPLICATION = APPNAME;
    }
    if ( !read(APPEXEC, "APPEXEC") ) {
        APPEXEC = APPLICATION;
    }
    if ( !read(APPMIME, "APPMIME") ) {
        APPMIME = APPLICATION;
    }
    if ( !read(APPICON, "APPICON") ) {
        APPICON = APPLICATION;
    }
    QString binlinks;
    if ( read(binlinks, "BINLINKS") ) {
        BINLINKS = binlinks.split(',');
    }
#if defined Q_OS_WIN
    //TODO
    if ( !read(ASSOC_EXT, "ASSOC_EXT") ) {
        //ASSOC_EXT = ???;
    }
    if ( !read(ASSOC_PROGID, "ASSOC_PROGID") ) {
        //ASSOC_PROGID = ???;
    }
    if ( !read(Publisher, "Publisher") ) {
        //Publisher = ???;
    }
    if ( !read(WebLink, "WebLink") ) {
        //WebLink = ???;
    }
#else
    if ( !read(EXECLINE, "EXECLINE") ) {
        //TODO: Versioned Exec lines not possible
    }
#endif

    // Get the long name for the application from the .desktop file.
    //TODO: On Windows this will probably need modifying!

    QString dconf{SOURCE_DIR.filePath("share/applications/" + APPLICATION + ".desktop")};
    if ( !QFileInfo{dconf}.isFile() ) {
        QMessageBox::critical(nullptr, tr("Critical"), tr("BUG, no 'desktop' file: %1").arg(dconf));
        return false;
    }
    QSettings dsettings(dconf, QSettings::IniFormat);
    dsettings.beginGroup("Desktop Entry");

    // Check Name entry
    auto val = dsettings.value("Name");
    if ( val.toString() != APPNAME ) {
        QMessageBox::critical(
            nullptr,
            tr("Critical"),
            tr("APPNAME in app.conf doesn't match 'Name' in 'desktop' file: %1").arg(dconf));
        return false;
    }

    // If possible, get a translated version.
    auto l = locale.name();
    val = dsettings.value("GenericName[" + l + "]");
    if ( !val.isValid() ) {
        l = QLocale::languageToCode(locale.language());
        val = dsettings.value("GenericName[" + l + "]");
        if ( !val.isValid() ) {
            val = dsettings.value("GenericName");
        }
    }
    APPLONGNAME = val.toString();

    delete appsettings;
    appsettings = nullptr;
    return true;
}
