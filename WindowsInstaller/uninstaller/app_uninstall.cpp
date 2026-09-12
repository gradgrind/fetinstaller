#include "uninstaller.h"

#include <QLocale>

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
    if ( !appinfo.init() )
        return 1;

    Uninstaller w(&appinfo);
    w.show();
    return QApplication::exec();
}
