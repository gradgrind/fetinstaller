#ifndef APPINFO_H
#define APPINFO_H

#include <QObject>
#include <QStringList>
#include <QDir>

class AppInfo : QObject
{
    Q_OBJECT
public:
    AppInfo();
    bool init();

    const QString APPNAME;
    const QString APPEXEC;
    const QString EXECDIR;
    const QString APPFILES;

    QDir appdir;
    QDir basedir;
    QString defaultInstallationPath;
    QString appcopy;
    bool registered;
};

#endif // APPINFO_H
