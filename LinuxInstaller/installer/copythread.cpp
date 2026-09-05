#include "copythread.h"
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QDirIterator>
#include <QProcess>

//TODO: If an error occurs, it would be good to remove all installed files.

void CopyWorker::copyDirectory(const QDir& srcDir, const QDir& dstDir, const InstallFiles& iFiles) {
    /* ... here is the long-running operation ... */

    // StartCopy with the directories, which should be sorted such that parent directories are always
    // before their child directories. Alphabetical sorting should be adequate.
    emit number_of_files(
        iFiles.installationDirs.length()
        + iFiles.installationFiles.length()
        + iFiles.installationLinksAbs.length()
        + iFiles.installationLinksRel.length());

    for ( const auto& d : iFiles.installationDirs ) {
        if ( dstDir.exists(d) ) {
            QFileInfo dd{dstDir.filePath(d)};
            if ( dd.isDir() && dd.isWritable() ) {
                emit dir_nocopy(d);
            } else {
                emit dir_failed_overwrite(d);
            }
        } else if ( dstDir.mkdir(d) ) {
            emit dir_written(d);
        } else {
            emit dir_failed_write(d);
        }
    }

    // Copy the regular files
    for ( const auto& f : iFiles.installationFiles ) {
        if ( QFile::copy(srcDir.filePath(f), dstDir.filePath(f)) ) {
            emit file_copied(f);
        } else {
            emit failed_copy(f);
        }
    }

    // Set symlinks
    for ( const auto& fx : iFiles.installationLinksAbs ) {
        if ( QFile::link(fx.second, dstDir.filePath(fx.first)) ) {
            emit link_copied(fx);
        } else {
            emit failed_link(fx);
        }
    }
    for ( const auto& fx : iFiles.installationLinksRel ) {
        if ( QFile::link(fx.second, dstDir.filePath(fx.first)) ) {
            emit link_copied(fx);
        } else {
            emit failed_link(fx);
        }
    }

    QDir home_dir{QDir::home()};
    QString done_message;
    if (dstDir.absolutePath() == home_dir.absoluteFilePath(".local")) {
        // Only perform these operations if installing to "~/.local". For them to work with
        // other installation locations, the relevant (modified) files would still need to
        // be placed in "~/.local".

        done_message = tr("Run %1 and %2").arg("update-mime-database", "update-desktop-database");

        // Update file-type associations
        QProcess::execute("update-mime-database",
                          QStringList() << dstDir.absoluteFilePath("share/mime"));
        QProcess::execute("update-desktop-database",
                          QStringList() << dstDir.absoluteFilePath("share/applications"));
    }

    emit copying_done(done_message);
}
