#include "uninstaller.h"
#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto locale = QLocale::system();
    QTranslator translator;
    const QString baseName = "app_uninstall_" + locale.name();
    if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
    }

    // Test for admin privileges
    qunsetenv("XDG_CONFIG_DIRS"); // needed for some Linux environments, to ensure the test works.
    QSettings settings(QSettings::SystemScope, "gradgrind", "app_installer");
    if ( settings.isWritable() ) {
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("main", "ADMIN user"),
            QCoreApplication::translate("main", "The installer must be run as a normal user."));
        return 1;
    }

    // Initialize paths
    AppInfo appinfo;
    if ( appinfo.init() ) {
        if ( !appinfo.appcopy.isEmpty() ) {
            // Copy only
            return 0;
        }

        Uninstaller w(appinfo);
        w.show();
        int cc = QApplication::exec();
        return cc;
    } else {
        return 1;
    }
}
