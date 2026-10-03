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
    //const QString APPFILENAME;
    const QString APPLONGNAME;
    const QString EXECDIR;
    //const QString APPFILES;
    QString APPVERSION;
#if defined Q_OS_WIN
    const QString ASSOC_EXT;
    const QString ASSOC_PROGID;
#endif
};

#endif // APPINFO_H
