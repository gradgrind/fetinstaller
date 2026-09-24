#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QSettings>
#include <QProcess>

void write_install()
{
    QString APPEXEC{"fetx"};
    QString APPVERSION{"7.10.5"};
    QString APPNAME{"FETX"};
    QString ASSOC_EXT{".fetx"};
    QString ASSOC_PROGID{"FETX.Main"};
    QString ShCtxt{"HKEY_CURRENT_USER"};
    QString UNINFO{ShCtxt + "\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + APPNAME};
    QSettings settings1(UNINFO, QSettings::NativeFormat);
    // Consider using versioned app folders ...
    QString LOCALAPPDATA{QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))};
    QString InstDir{LOCALAPPDATA + "\\Programs\\" + APPNAME + "-" + APPVERSION};

    settings1.setValue("DisplayName", APPNAME);
    settings1.setValue("UninstallString", "\"" + InstDir + "\\app_uninstall.exe\""); //???
    //settings1.setValue("Publisher", "Liviu Lalescu");
    //settings1.setValue("UrlInfoAbout", "https://lalescu.ro/liviu/fet/");
    settings1.setValue("DisplayVersion", APPVERSION);
    settings1.setValue("EstimatedSize", 12345); // Windows uses KiB (1024 bytes)
    settings1.setValue("UninstallLocation", InstDir);

    QSettings settings2(ShCtxt + "\\Software\\Classes", QSettings::NativeFormat);
    settings2.setValue(ASSOC_EXT + "/OpenWithProgIds/" + ASSOC_PROGID, "");
    settings2.setValue(ASSOC_PROGID + "/shell/open/FriendlyAppName", APPNAME + " " + APPVERSION);
    QString AppPath{"\"" + InstDir + "\\" + APPEXEC + ".exe\""};
    settings2.setValue(ASSOC_PROGID + "/shell/open/command/.", AppPath + " \"%1\"");
    settings2.setValue(ASSOC_PROGID + "/DefaultIcon/.", AppPath + ",0");

}

void write_uninstall()
{
    QString APPEXEC{"fetx"};
    QString APPVERSION{"7.10.5"};
    QString APPNAME{"FETX"};
    QString ASSOC_EXT{".fetx"};
    QString ASSOC_PROGID{"FETX.Main"};
    QString ShCtxt{"HKEY_CURRENT_USER"};
    QString UNINFO{ShCtxt + "\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + APPNAME};
    QSettings settings1(UNINFO, QSettings::NativeFormat);
    // Consider using versioned app folders ...
    //QString LOCALAPPDATA{QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))};

    QString InstDir = QDir::toNativeSeparators(QCoreApplication::applicationDirPath());

    QSettings settings2(ShCtxt + "\\Software\\Classes", QSettings::NativeFormat);
    settings2.remove(ASSOC_PROGID);

    qDebug() << "§§0" << settings2.value(ASSOC_EXT + "/.");
    if ( settings2.value(ASSOC_EXT + "/.") == ASSOC_PROGID ) {
        qDebug() << "§§0+";
        settings2.remove(ASSOC_EXT + "/.");
    }
    settings2.beginGroup(ASSOC_EXT + "/OpenWithProgIds");
    qDebug() << "§§1" << settings2.allKeys();
    settings2.endGroup();

    settings2.beginGroup(ASSOC_EXT + "/OpenWithProgIds");
    settings2.remove(ASSOC_PROGID);

    auto l1 = settings2.allKeys();
    qDebug() << "§§2" << l1;
    settings2.endGroup();
    if ( l1.isEmpty() ) {
        qDebug() << "§§3";
        settings2.remove(ASSOC_EXT + "/OpenWithProgIds");

        settings2.beginGroup(ASSOC_EXT);
        auto l2 = settings2.allKeys();
        qDebug() << "§§4" << l2;
        settings2.endGroup();

        if ( l2.isEmpty() ) {
            qDebug() << "§§5";
            settings2.remove(ASSOC_EXT);
        }
    }

    // Remove Start Menu and Desktop launchers

    //QFile::link(const QString &fileName, const QString &linkName)

    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation) + "/" + APPNAME + ".lnk");
    QFile::remove(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation) + "/" + APPNAME + ".lnk");

    settings1.remove("");

    // Delete app folder - needs an external script because open files cannot be deleted
    qDebug() << "InstDir:" << InstDir;

    //return;

    //QString cmd{"Start-Sleep -Milliseconds 5000; Remove-Item -Path \"%1\\*\" -Recurse -Force; Remove-Item -Path \"%1\""};
    QString cmd{"Start-Sleep -Milliseconds 100; Remove-Item -Path \"%1\" -Recurse -Force"};
    qDebug() << "--->>>" << cmd.arg(InstDir);
    QProcess::startDetached(
    //QProcess::execute(
        "powershell",
        QStringList() << "-Command" << cmd.arg(InstDir));
    qDebug() << "Done!";
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Set up code that uses the Qt event loop here.
    // Call QCoreApplication::quit() or QCoreApplication::exit() to quit the application.
    // A not very useful example would be including
    // #include <QTimer>
    // near the top of the file and calling
    // QTimer::singleShot(5000, &a, &QCoreApplication::quit);
    // which quits the application after 5 seconds.

    write_install();

    QString APPNAME{"FETX"};
    QString UNINFO{"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + APPNAME};
    QSettings settings(UNINFO, QSettings::NativeFormat);

    qDebug() << "?1" << settings.value("UninstallString", "---").toString();
    qDebug() << "?2" << settings.value("EstimatedSize", "---").toInt();

    QSettings settings2("HKEY_CURRENT_USER\\Software\\Classes\\.fetx\\OpenWithProgIds", QSettings::NativeFormat);
    qDebug() << "?3" << settings2.value("FETX.Main");

    QSettings settings3("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
    qDebug() << "?4" << settings3.value(".fetx/OpenWithProgIds/FETX.Main");

    QSettings settings4("HKEY_CURRENT_USER\\Software\\Classes\\FETX.Main", QSettings::NativeFormat);
    qDebug() << "?5" << settings3.value("FETX.Main/DefaultIcon/.");
    qDebug() << "?6" << settings4.value("DefaultIcon/.");



    //qDebug() << 1 << QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    //qDebug() << 2 << QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    //qDebug() << 3 << QStandardPaths::standardLocations(QStandardPaths::GenericConfigLocation);
    //qDebug() << 4 << QDir::home().filePath("AppData/Local/Programs");

    // If you do not need a running Qt event loop, remove the call
    // to QCoreApplication::exec() or use the Non-Qt Plain C++ Application template.

    //return QCoreApplication::exec();

    write_uninstall();

}
