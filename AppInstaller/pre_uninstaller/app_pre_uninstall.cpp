#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>
#include <QRandomGenerator>

#include <unistd.h>

QString installpath;
bool init(QString& path, QString& error);

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
            QCoreApplication::translate("Uninstall0", "ADMIN user"),
            QCoreApplication::translate("Uninstall0", "The installer must be run as a normal user."));
        return 1;
    }

    // Initialize paths, copy uninstaller to temporary directory.

    QString tmppath;
    QString error;

    if ( !init(tmppath, error) ) {
        QMessageBox::critical(
            nullptr,
            QCoreApplication::translate("Uninstall0", "Critical Error"),
            error);

        // Remove temporary dictionary.
        if ( !tmppath.isEmpty() && !QDir{tmppath}.removeRecursively() ) {
            QMessageBox::critical(
                nullptr,
                QCoreApplication::translate("Uninstall0", "Critical Error"),
                QCoreApplication::translate("Uninstall0", "Failed: Remove temporary folder: '%1'").arg(tmppath));
        }

        return 1;
    }

    // Transfer control to the uninstaller.
#if defined Q_OS_WIN
    // The uninstaller is expected to be in the root directory of the installation.
#else
    // The uninstaller is expected to be in the "bin" directory of the installation.
    QByteArray cmdbytes{QString{tmppath + "/bin/app_uninstall"}.toLocal8Bit()};
    const char* cmd = cmdbytes.constData();
    QByteArray arg1{installpath.toLocal8Bit()};
    execlp(cmd, cmd, arg1.constData(), (char*) NULL);
#endif
    // Doesn't return!
}

bool init(QString& path, QString &error)
{
    // Determine the installation's base directory
    QDir appdir(QFileInfo(QCoreApplication::applicationFilePath()).canonicalPath());
#if defined Q_OS_WIN
    // The uninstaller is expected to be in the root directory of the installation.
#else
    // The uninstaller is expected to be in the "bin" directory of the installation.
    appdir.cdUp();
#endif
    installpath = appdir.absolutePath();

    QStringList args = QCoreApplication::arguments();
    if ( args.length() != 1 ) { // There should be no command-line parameters
        error = QCoreApplication::translate(
                    "Uninstall0", "Invalid command line:\n  - %1").arg(args.join("\n  - "));
        return false;
    }

    QDir bindir{appdir};
    // Simple check for validity of root directory
#if defined Q_OS_WIN
    if ( !bindir.exists("app_uninstall.exe") ) {
#else
    // Actually not necessary because this whole app is only needed on Windows
    bindir.cd("bin");
    if ( !bindir.exists("app_uninstall") ) {
#endif
        error = QCoreApplication::translate(
                    "Uninstall0", "No uninstaller found in '%1'").arg(appdir.absolutePath());
        return false;
    }

    // Create a temporary directory.
    QDir tmpdir0{QDir::temp()};
    QString tmprel{QString{"AppUnIn%1"}.arg(QRandomGenerator::global()->generate())};
    if ( !tmpdir0.mkdir(tmprel) ) {
        error = QCoreApplication::translate(
                    "Uninstall0", "Failed: create temporary folder: '%1'")
                        .arg(tmpdir0.absoluteFilePath(tmprel));
        return false;
    }
    QDir tmpdir{tmpdir0};

    // Copy the uninstaller and supporting libraries to the temporary directory.
    tmpdir.cd(tmprel);
#if defined Q_OS_WIN
    QDir tmpbindir{tmpdir};
#else
    // Actually not necessary because this whole app is only needed on Windows
    QString librel;
    // Add libraries
    if ( appdir.exists("lib") ) {
        librel = "lib";
    } else if ( appdir.exists("lib64") ) {
        librel = "lib64";
    } else {
        error = QCoreApplication::translate(
                    "Uninstall0", "Dynamic libraries not found in '%1'").arg(appdir.absolutePath());
        return false;
    }
    tmpdir.mkdir(librel);
    // This will copy all files in the directory (not recursively!) and also deal with symlinks, but only
    // within this directory.
    QDir libdir{tmpdir.absoluteFilePath(librel)};
    for ( const auto &dirEntry : QDirListing(
             appdir.absoluteFilePath(librel),
             QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::ResolveSymlinks ) ) {
        const QString fileName = dirEntry.fileName();
        if ( dirEntry.isSymLink() ) {
            QFileInfo finfo{dirEntry.canonicalFilePath()};
            QString target{finfo.fileName()};
            if ( finfo.absolutePath() != dirEntry.absolutePath() ) {
                error = (QCoreApplication::translate(
                             "Uninstall0", "Symlink target outside lib folder:") + "\n  '%1' -> '%2'")
                                .arg(dirEntry.absoluteFilePath(), finfo.absoluteFilePath());
                return false;
            }
            if ( !QFile::link(target, libdir.filePath(fileName)) ) {
                error = (QCoreApplication::translate(
                             "Uninstall0", "Failed: make symlink:") + "\n  '%1' -> '%2'")
                                .arg(dirEntry.absoluteFilePath(), target);
                return false;
            }
        } else {
            // Only regular files ...
             if ( !QFile::copy(dirEntry.absoluteFilePath(), libdir.absoluteFilePath(fileName)) ) {
                error = (QCoreApplication::translate(
                             "Uninstall0", "Failed: copying uninstaller to temporary folder:") + "\n  %1 -> %2")
                                .arg(dirEntry.absoluteFilePath(), libdir.absoluteFilePath(fileName));
                return false;
            }
        }
    }

    QDir tmpbindir{tmpdir.absoluteFilePath("bin")};
    tmpdir.mkdir("bin");
#endif
    for ( const auto &dirEntry : QDirListing(
             bindir.absolutePath(),
             QDirListing::IteratorFlag::FilesOnly) ) {
        // (faster than using name filters)
        const QString fileName = dirEntry.fileName();
        if ( fileName.endsWith(".dll") || fileName == "qt.conf" || fileName.startsWith("app_uninstall") ) {
            // Only regular files ...
            if ( !QFile::copy(dirEntry.filePath(), tmpbindir.absoluteFilePath(fileName)) ) {
                error = (QCoreApplication::translate(
                            "Uninstall0", "Failed: copying uninstaller to temporary folder:") + "\n  %1 -> %2")
                                .arg(dirEntry.absoluteFilePath(), tmpbindir.absoluteFilePath(fileName));
                return false;
            }
        }
    }

    // Add plugins
    for ( const auto &dirEntry : QDirListing(
             appdir.absoluteFilePath("plugins"),
             QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::Recursive) ) {
        const QString filepath = dirEntry.absoluteFilePath();
        // Only regular files ...
        tmpdir.mkpath(appdir.relativeFilePath(dirEntry.absolutePath()));
        const QString target{tmpdir.absoluteFilePath(appdir.relativeFilePath(filepath))};
        if ( !QFile::copy(filepath, target) ) {
            error = (QCoreApplication::translate(
                        "Uninstall0",
                        "Copying uninstaller to temporary folder failed:") + "\n  %1 -> %2")
                            .arg(filepath, target);
            return false;
        }
    }
    path = tmpdir.absolutePath();
    return true;
}
