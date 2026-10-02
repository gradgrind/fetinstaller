#include "appinfo.h"

#if defined Q_OS_WIN

AppInfo::AppInfo()
    : APPNAME{"FET"}
    , APPEXEC{"fet.exe"}
    //, APPFILENAME{"fet"}
    , APPLONGNAME{tr("Timetable Generator")}
    , EXECDIR{""}
    //, APPFILES{""}
    , ASSOC_EXT{".fet"}
    , ASSOC_PROGID{"FET.Main"}
{}

#else

AppInfo::AppInfo()
    : APPNAME{"FET"}
    , APPEXEC{"fet"}
    //, APPFILENAME{"fet"}
    , APPLONGNAME{tr("Timetable Generator")}
    , EXECDIR{"bin/"}
    //, APPFILES{"share/fet/"}
{}

#endif
