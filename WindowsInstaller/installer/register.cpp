#include "installer.h"
#include <QProcess>

// Update file-type associations and desktop files.
// Windows: add app to registry.

#if defined Q_OS_WIN

void Installer::registerApp()
{
    //TODO
}

#elif defined Q_OS_LINUX

void Installer::registerApp()
{
    // Only perform these operations if installing to "~/.local". For them to work with
    // other installation locations, the relevant (modified) files would still need to
    // be placed in "~/.local".

    print_3("");
    print_3("update-mime-database");
    QProcess::execute("update-mime-database",
        QStringList() << dst_dir.absoluteFilePath("share/mime"));
    print_3("update-desktop-database");
    QProcess::execute("update-desktop-database",
        QStringList() << dst_dir.absoluteFilePath("share/applications"));
}

#else

//TODO

#endif