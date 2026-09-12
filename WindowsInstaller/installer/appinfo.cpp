#include "appinfo.h"

#if defined Q_OS_WIN

AppInfo::AppInfo()
    : APPNAME{"FET"}
    , APPEXEC{"fet"}
    , APPLONGNAME{tr("Timetable Generator")}
    , EXECDIR{""}
    , APPFILES{""}
{}

#else

AppInfo::AppInfo()
    : APPNAME{"FET"}
    , APPEXEC{"fet"}
    , APPLONGNAME{tr("Timetable Generator")}
    , EXECDIR{"bin/"}
    , APPFILES{"share/" + APPEXEC + "/"}
{}

#endif
