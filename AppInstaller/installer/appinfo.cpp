#include "appinfo.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QMessageBox>

#if defined Q_OS_WIN

AppInfo::AppInfo()
    : APPNAME{"FET"}
    , APPEXEC{"fet.exe"}
    //, APPFILENAME{"fet"}
    , APPLONGNAME{tr("Timetable Generator")}
    , EXECDIR{""}
    //, APPFILES{""}
    , ASSOC_EXT{".fet"}
    , ASSOC_PROGID{"FET.Main"}
{}

#else

AppInfo::AppInfo()
{
    // Get source path
    SOURCE_DIR = QFileInfo(QCoreApplication::applicationDirPath()).canonicalFilePath();
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
    ostype = "Windows/"
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
    appconf = SOURCE_DIR.filePath("app.conf");
    if ( !QFileInfo{appconf}.isFile() ) {
        QMessageBox::critical(nullptr, tr("Critical"), tr("BUG, no application configuration: %1").arg(appconf));
        return false;
    }

    appsettings = new QSettings(SOURCE_DIR.filePath("app.conf"), QSettings::IniFormat);
    //appsettings.beginGroup("Linux");
    //appsettings.setValue("APPNAME", "FET");
    //appsettings.setValue("EXECDIR", "bin/");
    //for (const auto& k : appsettings.allKeys()) {
    //    qDebug() << "§++" << k;
    //}

    if ( !read(APPNAME, "APPNAME", true) )
        return false;

    if ( !read(EXECDIR, "EXECDIR", true) )
        return false;

    if ( !read(APPVERSION, "VERSION", true) )
        return false;

    auto l = locale.name();
    //qDebug() << "$$1" << l;
    if ( !read(APPLONGNAME, "Name[" + l + "]") ) {
        l = QLocale::languageToCode(locale.language());
        //qDebug() << "$$2" << l;
        if ( !read(APPLONGNAME, "Name[" + l + "]") ) {
            read(APPLONGNAME, "Name");
        }
    }

    if ( !read(APPLICATION, "APPLICATION") ) {
        APPLICATION = APPNAME;
    }
    if ( !read(APPEXEC, "APPEXEC") ) {
        APPEXEC = APPLICATION;
    }

    delete appsettings;
    appsettings = nullptr;
    return true;
}

#endif
