#include "uninstaller.h"

#include <QLocale>
#include <QProcess>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto locale = QLocale::system();
    QTranslator translator;
    const QString baseName = "app_uninstall_" + locale.name();
    if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
    }

    // Initialize paths
    AppInfo appinfo;
    if ( appinfo.init() ) {
        if ( !appinfo.appcopy.isEmpty() ) {
            // Copy only
            return 0;
        }

        Uninstaller w(&appinfo);
        w.show();
        int cc = QApplication::exec();
        return cc;
    } else {
        return 1;
    }
}
