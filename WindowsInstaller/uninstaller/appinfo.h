#ifndef APPINFO_H
#define APPINFO_H

#include <QObject>

class AppInfo : QObject
{
    Q_OBJECT
public:
    AppInfo();

    const QString APPNAME;
    const QString APPEXEC;
    const QString EXECDIR;
    const QString APPFILES;
};

#endif // APPINFO_H
