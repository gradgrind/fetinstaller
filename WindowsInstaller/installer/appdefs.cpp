#include "appdefs.h"

const QString APPNAME{QStringLiteral("FET")};
const QString APPEXEC{QStringLiteral("fet")};
const QString APPLONGNAME{QStringLiteral("Timetable Generator")};

#ifdef Q_OS_WIN

const QString EXECDIR{QStringLiteral("")};
const QString APPFILES{QStringLiteral("")};

#else

const QString EXECDIR{QStringLiteral("bin/")};
const QString APPFILES{QStringLiteral("share/fet/")};

#endif
