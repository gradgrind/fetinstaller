#include "installer.h"

#include <QLocale>
#include <QMessageBox>
#include <QSettings>
#include <QtEnvironmentVariables>
#include "../app_uninstall/checkinuse.h"

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
        // Check for "stale" temporary directories.
        //qInfo() << "src_dir:" << w.src_dir.absolutePath();
        QDir tmp{w.src_dir.absoluteFilePath("..")};
        //qInfo() << "tmp:" << tmp.absolutePath();
        QDirListing dirList(tmp.absolutePath(), QDirListing::IteratorFlag::DirsOnly);
        for ( const auto &dirEntry : dirList ) {
            //qInfo() << " ++" << dirEntry.absoluteFilePath();
            if ( dirEntry.fileName().startsWith("tmp_gradgrind_AppIn_")
                    && dirEntry.absoluteFilePath() != w.src_dir.absolutePath() ) {
                QDir oldtmp{dirEntry.absoluteFilePath()};
                qint64 pid1 = dirInUse(oldtmp, -1);
                if ( pid1 == 0 ) {
                    // The directory is (apparently) unused, offer to delete it.
                    if ( QMessageBox::question(
                        nullptr,
                        QCoreApplication::translate("main", "Delete Folder?"),
                        QCoreApplication::translate(
                            "main",
                             "A temporary folder seems to be left over from an\n"
                             "earlier installation:\n   %1\n\n Delete it?")
                                .arg(QDir::toNativeSeparators(
                                    oldtmp.absolutePath()))) == QMessageBox::Yes )
                    {
                        oldtmp.removeRecursively();
                        if ( oldtmp.exists() ) {
                            QMessageBox::critical(
                                nullptr,
                                QCoreApplication::translate("main", "Failed"),
                                QCoreApplication::translate(
                                    "main",
                                    "Failed: delete temporary folder: %1")
                                        .arg(QDir::toNativeSeparators(oldtmp.absolutePath()))
                            );
                        }
                    }
                } else {
                    // This is probably another installer.
                    QMessageBox::critical(
                        nullptr,
                        QCoreApplication::translate("main", "Blocked"),
                        QCoreApplication::translate(
                            "main",
                            "There seems to be another installer active,\n"
                            "using the temporary folder: %1\n"
                            "Please finish the process with ID %2 and try again.")
                                .arg(QDir::toNativeSeparators(oldtmp.absolutePath()))
                                .arg(pid1)
                    );
                    return 1;
                }
            }
        }

        w.show();
        return QApplication::exec();
    }
    return 1;
}
