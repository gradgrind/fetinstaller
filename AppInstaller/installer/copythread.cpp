#include "copythread.h"
#include <QProcess>

void CopyWorker::copyDirectory(const QDir& srcDir, const QDir& dstDir, const InstallFiles& iFiles) {
    /* ... here is the long-running operation ... */

    // Start by copying the directories, which should be sorted such that parent directories are always
    // before their child directories. Alphabetical sorting should be adequate.
    emit number_of_files(
        iFiles.installationDirs.length()
        + iFiles.installationFiles.length()
        + iFiles.installationLinks.length());

    for ( const auto& d : iFiles.installationDirs ) {
        if ( dstDir.exists(d) ) {
            emit dir_exists(d);
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

    // Set symlinks / Windows shortcuts
    for ( const auto& fx : iFiles.installationLinks ) {
        if ( QFile::link(fx.second, dstDir.filePath(fx.first)) ) {
            emit link_copied(fx);
        } else {
            emit failed_link(fx);
        }
    }

    emit copying_done();
}
