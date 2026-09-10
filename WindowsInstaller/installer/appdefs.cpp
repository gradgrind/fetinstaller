#include "appdefs.h"

const QString APPNAME{QStringLiteral("FET")};
const QString APPEXEC{QStringLiteral("fet")};
const QString APPLONGNAME{QStringLiteral("Timetable Generator")};

#ifdef _WIN32

bool checkAppDir(QDir d)
{
    return QFileInfo::exists(d.filePath("fet"));
}

#else

bool checkAppDir(QDir d)
{
    return QFileInfo::exists(d.filePath("bin/fet"))
        && QFileInfo::exists(d.filePath("share/fet"));
}

#endif
