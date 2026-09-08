#include "copythread.h"
#include <QProcess>

void CopyWorker::copyDirectory(const QDir& srcDir, const QDir& dstDir, const InstallFiles& iFiles) {
    /* ... here is the long-running operation ... */

    // Start by copying the directories, which should be sorted such that parent directories are always
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

    emit copying_done();
}


void CopyWorker::removePartial(const QDir& dstDir, const QStringList& dirs, const QStringList& files)
{
    /* ... here is the long-running operation ... */

    QDir d0{dstDir};
    // Remove installed files and directories
    int xdirs{0}; // not uninstalled directories
    int xfiles{0}; // not uninstalled files
    // Remove the files in reverse order (starting with the symlinks)
    for ( auto it = files.rbegin(); it != files.rend(); ++it ) {
        emit remove_file(*it, d0.remove(*it));
    }
    // Remove the directories in reverse order (starting at the leaves)
    for ( auto it = dirs.rbegin(); it != dirs.rend(); ++it ) {
        emit remove_dir(*it, d0.rmdir(*it));
    }
    emit removing_done();
}