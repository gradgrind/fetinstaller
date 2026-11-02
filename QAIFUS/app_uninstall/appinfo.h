#ifndef APPINFO_H
#define APPINFO_H

#include <QObject>
#include <QString>
#include <QDir>

class AppInfo : QObject
{
    Q_OBJECT
public:
    AppInfo();
    bool init();

    QString QAIFUS_VERSION;
    QString QAIFUS_INFO;

    QDir appdir;    // root of running app
    QDir basedir;   // root of installation
    QString basename; // appname + version (= name of basedir)
    QString error;
};

#endif // APPINFO_H
