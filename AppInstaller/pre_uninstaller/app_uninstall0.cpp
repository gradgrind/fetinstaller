#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>
#include <QRandomGenerator>

#if defined Q_OS_WIN
#include <process.h>
#else
#include <unistd.h>
#endif

QDir appdir;
QDir tmpdir;
QString uninstaller2tmp();

void error(QString msg)
{
    QMessageBox::critical(
        nullptr,
        QCoreApplication::translate("Uninstall0", "Error"),
        msg);
}

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
    const QString baseName = "app_uninstall0_" + locale.name();
    if (translator.load(":/i18n/" + baseName)) {
        a.installTranslator(&translator);
    }

    // Test for admin privileges
    qunsetenv("XDG_CONFIG_DIRS"); // needed for some Linux environments, to ensure the test works.
    QSettings settings(QSettings::SystemScope, "gradgrind", "app_installer");
    if ( settings.isWritable() ) {
        error(QCoreApplication::translate("Uninstall0", "The installer must be run as a normal user."));
        return 1;
    }

    // Check no command-line arguments
    QStringList args = QCoreApplication::arguments();
    if ( args.length() != 1 ) {
        QString e = QCoreApplication::translate("Uninstall0", "Invalid command line:\n  - %1");
        error(e.arg(args.join("\n  - ")));
        return 1;
    }

    // Determine the installation's base directory
    appdir.setPath(QFileInfo(QCoreApplication::applicationFilePath()).canonicalPath());

    // Copy uninstaller to temporary directory.
    QString e = uninstaller2tmp();
    if ( !e.isEmpty() ) {
        error(e);
    } else {
        // Transfer control to the uninstaller.
#if defined Q_OS_WIN
        // The uninstaller is expected to be in the root directory of the installation.
        QByteArray cmdbytes{tmpdir.absoluteFilePath("app_uninstall.exe").toLocal8Bit()};
#else
        // The uninstaller is expected to be in the "bin" directory of the installation.
        QByteArray cmdbytes{tmpdir.absoluteFilePath("bin/app_uninstall.exe").toLocal8Bit()};
#endif
        const char* cmd = cmdbytes.constData();
        QByteArray arg1{appdir.absolutePath().toLocal8Bit()};
#if defined Q_OS_WIN
        _execl(cmd, cmd, arg1.constData(), (char*) NULL);
#else
        execl(cmd, cmd, arg1.constData(), (char*) NULL);
#endif
        // execl doesn't return, if it works.
        error(QString{"Failed: Switch to uninstaller in '%1'"}.arg(tmpdir.absolutePath()));
    }

    // Remove temporary dictionary.
    if ( tmpdir != QDir{} && !tmpdir.removeRecursively() ) {
        error(QCoreApplication::translate(
            "Uninstall0", "Failed: Remove temporary folder: '%1'").arg(tmpdir.absolutePath()));
    }

    // If successfully completed, we would never arrive here.
    return 1;
}


QString copyfile(QString frel)
// Copy from appdir to tmpdir, the file has a relative path.
{
    QString dst{tmpdir.absoluteFilePath(frel)};
    // Ensure the oarent directory exists.
    QFileInfo dinfo{QFileInfo{dst}.absolutePath()};
    if ( !dinfo.isDir() ) {
        tmpdir.mkpath(dinfo.absoluteFilePath());
    }
    if ( !QFile::copy(appdir.absoluteFilePath(frel), dst) ) {
        return dst;
    }
    return {};
}

QString uninstaller2tmp()
{
    // Create a temporary directory.
    QDir tmpdir0{QDir::temp()};
    QString tmprel{QString{"AppUnIn%1"}.arg(QRandomGenerator::global()->generate())};
    if ( !tmpdir0.mkdir(tmprel) ) {
        return QCoreApplication::translate(
                "Uninstall0", "Failed: create temporary folder: '%1'")
                    .arg(tmpdir0.absoluteFilePath(tmprel));

    }
    tmpdir.setPath(tmpdir0.absoluteFilePath(tmprel));
    // Copy the uninstaller and supporting libraries to the temporary directory.
    QStringList elist;
    QString e;
    e = copyfile("app_uninstall.exe");
    if ( !e.isEmpty() ) {
        elist.append(e);
    }
    e = copyfile("qt.conf");
    if ( !e.isEmpty() ) {
        elist.append(e);
    }
    for ( const auto &dirEntry : QDirListing(
             appdir.absolutePath(),
             QDirListing::IteratorFlag::FilesOnly ) ) {
        // Only regular files ...
        const QString fileName = dirEntry.fileName();
        if ( fileName.endsWith(".dll") ) {
            e = copyfile(fileName);
            if ( !e.isEmpty() ) {
                elist.append(e);
            }
        }
    }
    // Add plugins
    for ( const auto &dirEntry : QDirListing(
             appdir.absoluteFilePath("plugins"),
             QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::Recursive) ) {
        // Only regular files ...
        e = copyfile(appdir.relativeFilePath(dirEntry.absoluteFilePath()));
        if ( !e.isEmpty() ) {
            elist.append(e);
        }
    }
    if ( elist.isEmpty() ) {
        return {};
    }
    return QCoreApplication::translate(
            "Uninstall0", "Failed: copying file(s) to temporary folder:\n - %1")
                .arg(elist.join("\n - "));
}
