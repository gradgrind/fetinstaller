#include "uninstaller.h"
#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>

#if defined Q_OS_WIN
#include <process.h>

// A Powershell command to remove the temporary directory, reporting an error if it didn't work.
const QString PSCMD{R"(
if ( Test-Path -LiteralPath '%1' ) {
    Remove-Item -LiteralPath '%1' -Recurse -Force;
    if ( -not $? ) {
        Add-Type -AssemblyName PresentationCore,PresentationFramework;
        [System.Windows.MessageBox]::Show('%3', '%2', 0, 16);
    }
} else {
    Add-Type -AssemblyName PresentationCore,PresentationFramework;
    [System.Windows.MessageBox]::Show('%4', '%2', 0, 16);
})"};

#endif

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
    } else {
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("main", "Critical Error"),
            appinfo.error);
    }
#if defined Q_OS_WIN
    // Remove temporary directory.
    QString dpath{QDir::toNativeSeparators(appinfo.appdir.absolutePath())};
    QString msgFail{QCoreApplication::translate("main", "Failed: Remove the temporary folder, %1")
            .arg(dpath)};
    dpath.replace('\'', "''");    // escape apostrophes
    QString emsg{QCoreApplication::translate(
                     "main",
                     "Failed: delete folder %1",
                     "Do not use apostrophe, it will break the Powershell script!").arg(dpath)};
    QString emsg0{QCoreApplication::translate(
                     "main",
                     "Temporary folder not found: %1",
                     "Do not use apostrophe, it will break the Powershell script!").arg(dpath)};
    QString cmd{PSCMD.arg(dpath, QCoreApplication::translate("main", "Error"), emsg, emsg0)};
    /*
        QMessageBox::information(
            nullptr,
            QCoreApplication::translate("main", "Powershell"),
            cmd);
    */
    _wexeclp(L"powershell", L"powershell", L"-WindowStyle", L"Hidden", L"-Command", cmd.data(), (wchar_t*) NULL);
    //_wexeclp(L"powershell", L"powershell", L"-NoExit", L"-Command", cmd.data(), (wchar_t*) NULL);
    // Shouldn't return!
    QMessageBox::critical(
        nullptr,
        QCoreApplication::translate("main", "Tidying"),
        msgFail);
    cc = 100;
#endif
    return cc;
}
