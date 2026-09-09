#include "appdefs.h"

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
