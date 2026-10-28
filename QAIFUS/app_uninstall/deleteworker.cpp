#include "deleteworker.h"

#include <QFile>
#include <QDir>

bool DeleteWorker::removeDirectory(const QString &dirPath) {
    // Loop through the directory contents
    for ( const auto &dirEntry : QDirListing(dirPath, QDirListing::IteratorFlag::IncludeHidden) ) {
        QString fpath{dirEntry.absoluteFilePath()};
        if (dirEntry.isDir()) {
            removeDirectory(fpath);
        } else {
            emit deletedFile(fpath, QFile::remove(fpath));
        }
    }

    // After all contents are removed, delete the directory itself
    if ( QDir().rmdir(dirPath) ) {
        emit removedDir(dirPath, true);
        return true;
    } else {
        emit removedDir(dirPath, false);
        return false;
    }
}

void DeleteWorker::deleteFiles(const QString basePath)
{
    emit done(removeDirectory(basePath));
}
