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
        if ( !appinfo.temporaryDir.isEmpty() ) {
            // Control passed to secondary uninstaller
            return 0;
        }

        Uninstaller w(&appinfo);
        w.show();
        int cc = QApplication::exec();

#if defined Q_OS_WIN

        // Remove temporary directory
        QString d0{appinfo.appdir.path()};
        QString nd{QDir::toNativeSeparators(d0)};
        QString cmd{"Start-Sleep -Seconds 0.1; rm -r -fo '%1'"};
        QProcess::startDetached(
            "powershell",
            QStringList() << "-Command" << cmd.arg(nd));

#endif

        return cc;
    } else {
        if ( !appinfo.temporaryDir.isEmpty() ) {
            appinfo.clean();
        }
        return 1;
    }
}
