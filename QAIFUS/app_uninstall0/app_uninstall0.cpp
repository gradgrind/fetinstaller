#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
#include <QLocale>
#include <QProcess>
#include <QSettings>
#include <QMessageBox>
#include "../app_uninstall/checkinuse.h"

#if defined Q_OS_WIN
//#include <process.h> // not using _execl
#else
#include <unistd.h> // actually not relevant, the app is Windows only!
#endif

#define TMP_NAME "gradgrind_AppUnin"

static const char* TMP_BUSY = QT_TRANSLATE_NOOP("Uninstall0", R"(
The uninstaller's temporary folder is already in use:
   %1

Before trying to run this uninstaller again, please stop the process,
probably another uninstaller, with ID %2.)");

static const char* TMP_REMAINS = QT_TRANSLATE_NOOP("Uninstall0", R"(
The uninstaller's temporary folder exists already:
   %1

It is probably just a remnant of an earlier unsuccessful uninstaller
process, but please check before allowing it to be removed.

Remove the folder?)");

QDir appdir;
QDir tmpdir;
bool copy_only{false};
bool uninstaller2tmp();
bool delTmp();

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

    // Check command-line arguments
    QStringList args = QCoreApplication::arguments();
    if ( args.length() == 2 && args.at(1) == "--copy-only" ) {
        // The "copy-only" switch is to make it possible on Windows for the installer to
        // wait for the uninstaller to complete. It can perform the two steps (copy to temporary
        // folder, then uninstall) separately, waiting for each to complete.
        copy_only = true;
    } else if ( args.length() != 1 ) {
        error(QCoreApplication::translate(
            "Uninstall0",
            "Invalid command line:\n  - %1").arg(args.join("\n  - ")));
        return 1;
    }

    // Determine the installation's base directory
    appdir.setPath(QFileInfo(QCoreApplication::applicationFilePath()).canonicalPath());

    // On Windows, there is an "_execl" function, but it does not work quite like the Unix
    // "execl": on Windows the new process has a new pid, so it doesn't really act like it
    // was a continuation of the original process, and wait functions will not wait for the
    // new process to complete. So on Windows we might as well use QProcess.

    // As it is perhaps not really necessary to allow two uninstallers to run at the same time,
    // simpler code can be used, with a fixed temporary folder.

    QDir tmpdir0{QDir::temp()};
    tmpdir.setPath(tmpdir0.absoluteFilePath(TMP_NAME));
    if ( !tmpdir0.mkdir(TMP_NAME) ) {
        auto pid = dirInUse(tmpdir, -1);
        if ( pid != 0 ) {
            error(QCoreApplication::translate(
                "Uninstall0",
                TMP_BUSY).arg(QDir::toNativeSeparators(tmpdir.path())).arg(pid));
            return 1;
        }

        if ( QMessageBox::question(
            nullptr,
            QCoreApplication::translate("Uninstall0", "Delete Folder?"),
            QCoreApplication::translate("Uninstall0", TMP_REMAINS)
                .arg(QDir::toNativeSeparators(tmpdir.path()))) == QMessageBox::Yes )
        {
            tmpdir.removeRecursively();
            if ( !tmpdir0.mkdir(TMP_NAME) ) {
                error(QCoreApplication::translate(
                "Uninstall0",
                "Failed: delete temporary folder: %1")
                    .arg(QDir::toNativeSeparators(tmpdir.path())));
                return 1;
            }
        } else {
            return 1;
        }
    }

    // Copy uninstaller to temporary directory.
    if ( uninstaller2tmp() ) {

#if defined Q_OS_WIN
        // Create a link to appdir.absolutePath() in tmpdir
        if ( !QFile::link(appdir.absolutePath(), tmpdir.filePath("UNINSTALL_TARGET.lnk")) ) {
            error(QCoreApplication::translate(
                "Uninstall0",
                "Failed: link to target: %1 -> %2")
                    .arg(QDir::toNativeSeparators(tmpdir.filePath("UNINSTALL_TARGET.lnk"))
                        , QDir::toNativeSeparators(appdir.absolutePath())));
            goto recover;
        }
#endif

        if ( copy_only ) {
            return 0;
        }

        // Start the uninstaller: on Windows return 0, on Linux don't return (use execl).
#if defined Q_OS_WIN
        // The uninstaller is expected to be in the root directory of the installation.
        QString cmd{tmpdir.absoluteFilePath("app_uninstall.exe")};
        if ( QProcess::startDetached(cmd) ) {
            return 0;
        }
        error(QCoreApplication::translate(
            "Uninstall0",
            "Couldn't start uninstaller: %1").arg(QDir::toNativeSeparators(cmd)));
#else
        // Actually, this bit is superfluous because the whole app is only for Windows!
        // The uninstaller is expected to be in the "_installer_" directory of the installation.
        QByteArray cmdbytes{tmpdir.absoluteFilePath("_installer_/app_uninstall.exe").toLocal8Bit()};
        const char* cmd = cmdbytes.constData();
        QByteArray arg1{appdir.absolutePath().toLocal8Bit()};
        execl(cmd, cmd, arg1.constData(), (char*) NULL);
        // execl doesn't return, if it works.
        error(QCoreApplication::translate(
            "Uninstall0",
            "Failed: switch to uninstaller in %1").arg(tmpdir.absolutePath()));
#endif
    }

recover:
    // Remove temporary dictionary.
    delTmp();

    // If successfully completed, we would never arrive here.
    return 1;
}

bool delTmp() {
    if ( tmpdir.removeRecursively() )
        return true;
    error(QCoreApplication::translate(
        "Uninstall0",
        "Failed: Remove temporary folder: %1")
            .arg(QDir::toNativeSeparators(tmpdir.absolutePath())));
    return false;
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
        return QDir::toNativeSeparators(dst);
    }
    return {};
}

bool uninstaller2tmp()
// Copy the uninstaller and supporting libraries to the temporary directory.
{
#if defined Q_OS_WIN
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
        QDirListing::IteratorFlag::FilesOnly ) )
    {
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
        QDirListing::IteratorFlag::FilesOnly | QDirListing::IteratorFlag::Recursive) )
    {
        // Only "regular" files are copied
        e = copyfile(appdir.relativeFilePath(dirEntry.absoluteFilePath()));
        if ( !e.isEmpty() ) {
            elist.append(e);
        }
    }
    if ( elist.isEmpty() ) {
        return true;
    }
    error(QCoreApplication::translate(
        "Uninstall0",
        "Failed: copy file(s) to temporary folder:\n - %1")
            .arg(elist.join("\n - ")));
    return false;
#else
    error("Not implemented: uninstaller2tmp()");
    return false;
#endif
}
