#include "installer.h"

#include <QLocale>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto locale = QLocale::system();
    QTranslator translator;
    const QString baseName = "fet_install_" + locale.name();
    if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
    }

    if (qgetenv("USER") == "root") {
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("main", "'root' user"),
            QCoreApplication::translate("main", "The installer must be run as a normal user."));
        return 1;
    }
    Installer w;
    w.show();
    return QApplication::exec();
}
