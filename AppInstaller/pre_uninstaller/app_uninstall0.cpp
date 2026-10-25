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
//#include <process.h> // not using _execl
#else
#include <unistd.h>
#endif

#define TMP_NAME "gradgrind_QAIFUS"

static const char* TMP_BUSY = QT_TRANSLATE_NOOP("Uninstall0", R"(
The temporary folder '%1' is already in use.
It could be in use by another uninstaller – if so,         TODO
don't delete it!
If it is in use by a completely different application (unlikely), don't delete it!

The folder might have been left by a faulty run of the uninstaller.

Only if you are sure it is no longer in use should you delete it.

Delete this folder?)");

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

    // Check no command-line arguments
    QStringList args = QCoreApplication::arguments();
    if ( args.length() == 2 && args.at(1) == "--copy-only" ) {
        // The "copy-only" switch is to make it also possible on Windows for the installer to
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
    QString tmpPath{tmpdir0.absoluteFilePath(TMP_NAME)};
    if ( QFileInfo::exists(tmpPath) ) {
        if ( QMessageBox::question(
            nullptr,
            QCoreApplication::translate("Uninstall0", "Delete Folder?"),
            QCoreApplication::translate("Uninstall0", TMP_BUSY)) == QMessageBox::Yes )
        {
            if ( tmpdir0.mkdir(TMP_NAME) ) {
                tmpdir.setPath(tmpPath);
            } else {
                error(QCoreApplication::translate(
                    "Uninstall0",
                    "Failed: create temporary folder: '%1'").arg(tmpPath));
                return 1;
            }
        }
    } else
        return 1;

/* This is a sketch allowing multiple temporary folders. The inserted number could be returned as
 * completion code (for copy_only). Of course this is rather misusing the completion code idea,
 * but it is a rather simple way of handling the problem of how to return the temporary folder.

    QString fmt{"AppUnIn_%1"};
    int nn{0};
    for ( int n = 10; n < 20; ++n ) {
        QString tmpname = fmt.arg(n);
        if ( tmpdir0.mkdir(tmpname) ) {
            tmpdir.setPath(tmpdir0.absoluteFilePath(tmpname));
            nn = n;
            break;
        }
    }
    if ( nn == 0 ) {
        error(QCoreApplication::translate(
            "Uninstall0",
            "Couldn't create temporary folder.\nPerhaps too many are already in use:\n  '%1'")
                .arg(tmpdir0.absoluteFilePath(fmt.arg("nn"))));
        return 1;
    }
*/

    // Copy uninstaller to temporary directory.
    if ( uninstaller2tmp() ) {


#if defined Q_OS_WIN
        // Create a link to appdir.absolutePath() in tmpdir
        if ( !QFile::link(appdir.absolutePath(), tmpdir.filePath("UNINSTALL_TARGET.lnk")) ) {
            error(QCoreApplication::translate(
                "Uninstall0",
                "Failed: link to target: '%1' -> '%2'")
                    .arg(tmpdir.filePath("UNINSTALL_TARGET.lnk"), appdir.absolutePath()));
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
        if ( QProcess::startDetached(cmd, QStringList{appdir.absolutePath()}) ) {
            return 0;
        }
        error(QCoreApplication::translate(
            "Uninstall0",
            "Couldn't start uninstaller: '%1'").arg(cmd));
#else
        // The uninstaller is expected to be in the "bin" directory of the installation.
        QByteArray cmdbytes{tmpdir.absoluteFilePath("bin/app_uninstall.exe").toLocal8Bit()};
        const char* cmd = cmdbytes.constData();
        QByteArray arg1{appdir.absolutePath().toLocal8Bit()};
        execl(cmd, cmd, arg1.constData(), (char*) NULL);
        // execl doesn't return, if it works.
        error(QCoreApplication::translate(
            "Uninstall0",
            "Failed: switch to uninstaller in '%1'").arg(tmpdir.absolutePath()));
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
        "Failed: Remove temporary folder: '%1'").arg(tmpdir.absolutePath()));
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
        return dst;
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
        return true;
    }
    error(QCoreApplication::translate(
        "Uninstall0",
        "Failed: copy file(s) to temporary folder:\n - %1")
            .arg(elist.join("\n - ")));
    return false;
}
#else
    error("Not implemented: uninstaller2tmp()");
    return false;
#endif
}
