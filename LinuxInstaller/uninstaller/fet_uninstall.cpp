#include "uninstaller.h"

#include <QApplication>
#include <QTranslator>
#include <QTemporaryDir>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    auto locale = QLocale::system();
    QTranslator translator;
    const QString baseName = "fet_uninstall_" + locale.name();
    if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
    }
    Uninstaller w;
    w.show();
    return QApplication::exec();
}
