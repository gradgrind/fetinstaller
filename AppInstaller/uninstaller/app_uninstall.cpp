#include "uninstaller.h"
#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>

#if defined Q_OS_WIN
#include <process.h>
#endif

// A Powershell command to remove the temporary directory, reporting an error if it didn't work.
const QString PSCMD{R"(
if ( Test-Path -Path '%1' ) {
    Remove-Item '%1' -Recurse -Force;
    if ( -not $? ) {
        Add-Type -AssemblyName PresentationCore,PresentationFramework;
        [System.Windows.MessageBox]::Show('%3', '%2', 0, 16);
    }
})"};

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

    int cc{1};
    // Initialize paths
    AppInfo appinfo;
    if ( appinfo.init() ) {
        Uninstaller w(appinfo);
        w.show();
        cc = QApplication::exec();
#if defined Q_OS_WIN
        // Remove temporary directory.
        QString dpath{QDir::toNativeSeparators(appinfo.appdir.absolutePath())};
        QString emsg{QCoreApplication::translate("main", "Failed: delete folder '%1'").arg(dpath)};
        QString cmd{PSCMD.arg(dpath, QCoreApplication::translate("main", "Error"), emsg)};
        QByteArray cmdbytes{cmd.toLocal8Bit()};
        const char* cmdx = cmdbytes.constData();
        /*
        QMessageBox::information(
            nullptr,
            QCoreApplication::translate("main", "Powershell"),
            cmd);
        */
        _execlp("powershell", "powershell", "-WindowStyle", "Hidden", "-Command", cmdx, (char*) NULL);
        //_execlp("powershell", "powershell", "-NoExit", "-Command", cmdx, (char*) NULL);
        // Shouldn't return!
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("main", "Tidying"),
            QCoreApplication::translate("main", "Removing the temporary folder failed: '%1'")
                .arg(dpath));
        cc = 100;
#endif
    } else {
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("main", "Critical Error"),
            appinfo.error);
    }
    return cc;
}
