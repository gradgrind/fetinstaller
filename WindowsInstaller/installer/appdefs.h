#ifndef APPDEFS_H
#define APPDEFS_H

#include <QDir>
#include <QString>

extern const QString APPNAME;
extern const QString APPEXEC;
extern const QString EXECDIR;
extern const QString APPLONGNAME;
extern const QString APPFILES;

// Test whether a directory could be a valid app installation (not at all exhaustive!)
bool checkAppDir(QDir d);

#endif // APPDEFS_H
