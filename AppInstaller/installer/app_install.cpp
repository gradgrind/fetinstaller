#include "installer.h"

#include <QLocale>
#include <QMessageBox>
#include <QSettings>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto locale = QLocale::system();
    QTranslator translator;
    const QString baseName = "app_install_" + locale.name();
    if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
    }

    // Test for admin privileges
    QSettings settings(QSettings::SystemScope, "gradgrind", "app_installer");
    if ( settings.isWritable() ) {
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("main", "ADMIN user"),
            QCoreApplication::translate("main", "The installer must be run as a normal user."));
        return 1;
    }
    Installer w;
    w.show();
    return QApplication::exec();
}
