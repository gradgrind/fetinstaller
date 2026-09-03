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

    /* It may be necessary (on Windows?) to copy the uninstaller and its libs and plugins
     * to a temporary directory in order to do a complete job. This might be a start ...
     *
     * On Linux it should not be necessary.
     *

    QStringList arglist{QCoreApplication::arguments()};

    if (arglist.length() == 2) {
        QDir installdir{arglist.at(1)};
        if (!installdir.exists("bin/fet_uninstall")) {
            fatalError(
                QCoreApplication::translate("Uninstaller", "Invalid path to FET installation: ")
                    + installdir.path());
            return 1;
        }

        // Copy executable and libraries to temporary directory

        QTemporaryDir dir;
        if (!dir.isValid()) {
            fatalError(
                QCoreApplication::translate("Uninstaller", "Couldn't create temporary directory"));
            return 1;
        }

        // dir.path() returns the unique directory path
        qDebug() << "TEMP:" << dir.path();

        QDir tdir{dir.path()};
        tdir.mkdir("bin");

        //TODO ...

        // The QTemporaryDir destructor removes the temporary directory
        // as it goes out of scope.
        return 1;

    } else if (arglist.length() != 1) {
            fatalError(
            QCoreApplication::translate("Uninstaller", "Too many command-line arguments: ")
                + arglist.join(" +++ "));
        return 1;
    }

    // Actual uninstaller, now running from temporary directory ...

    // ...

    QProcess::startDetached()

    // ...

    */

    Uninstaller w;
    w.show();
    return QApplication::exec();
}
