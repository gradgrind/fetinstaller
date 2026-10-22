#include "installer.h"

#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QtEnvironmentVariables>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Allow the language for translation to be read from the environment variable "QTLANG".
    auto locale = QLocale(qEnvironmentVariable("QTLANG"));
    if ( locale.language() == QLocale::C ) {
        // No valid language was found.
        locale = QLocale::system();
    }
    QTranslator translator;
    const QString baseName = "app_install_" + locale.name();
    if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
    }

    //qDebug() << "&&&1" << locale.name();
    //qDebug() << "&&&2" << QLocale::languageToCode(locale.language());

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
    Installer w(locale);
    if ( w.app_initialized ) {
        w.show();
        return QApplication::exec();
    }
    return 1;
}
