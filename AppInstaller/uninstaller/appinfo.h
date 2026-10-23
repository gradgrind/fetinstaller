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
    bool uninstaller2tmp(); // needed only on Windows
    bool copyfile(QString frel);

    QDir appdir;    // root of running app
    QDir basedir;   // root of installation
    QString basename; // appname + version (= name of basedir)
    QString error;
};

#endif // APPINFO_H
