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

    QDir appdir;    // root of running app
    QDir basedir;   // root of installation
    QString basename; // appname + version (= name of basedir)
    QString appcopy;
};

#endif // APPINFO_H
