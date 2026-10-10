#ifndef APPINFO_H
#define APPINFO_H

#include <QObject>
#include <QDir>
#include <QLocale>
#include <QSettings>
#include <QStringList>

class AppInfo : QObject
{
    Q_OBJECT

    bool read(QString& var, QString key, bool critical=false);
    QSettings* appsettings;
    QString appconf; // configuration file (full path)
    QString ostype;

public:
    AppInfo();
    ~AppInfo() {
        delete appsettings;
    }
    bool init(QLocale locale);

    QDir SOURCE_DIR;
    QString APPNAME;
    QString APPLONGNAME;
    QString APPLICATION; // possibly the same as APPNAME, but might be lower case
    QString APPEXEC;     // probably the same as APPLICATION
    QString APPMIME;     // probably the same as APPLICATION
    QString APPICON;     // probably the same as APPLICATION
    QString EXECDIR;
    QString APPVERSION;
    QStringList BINLINKS;
#if defined Q_OS_WIN
    QString ASSOC_EXT;
    QString ASSOC_PROGID;
    QString Publisher;
    QString WebLink;
#else
    QString EXECLINE; // for .desktop file
#endif
};

#endif // APPINFO_H
