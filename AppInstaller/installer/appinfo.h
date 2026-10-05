#ifndef APPINFO_H
#define APPINFO_H

#include <QObject>
#include <QDir>
#include <QLocale>
#include <QSettings>

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
#if defined Q_OS_WIN
    const QString ASSOC_EXT;
    const QString ASSOC_PROGID;
#endif
};

#endif // APPINFO_H
