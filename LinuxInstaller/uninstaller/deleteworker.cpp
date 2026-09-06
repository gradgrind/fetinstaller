#include "deleteworker.h"

#include <QFile>
#include <QDir>

void DeleteWorker::deleteFiles(const QStringList links, const QStringList files, const QStringList dirs)
{
    // Delete the links
    for (const auto& fpath : links) {
        emit deletedFile(fpath, QFile::remove(fpath));
    }
    // Delete the files
    for (const auto& fpath : files) {
        emit deletedFile(fpath, QFile::remove(fpath));
    }
    // Delete the directories. These should already be sorted correctly (children first), but
    // collect failures in case the list was sorted wrongly, to then try again afterwards.
    QStringList dirfails;
    QDir d0;
    for (const auto& fpath : dirs) {
        if ( d0.rmdir(fpath) ) {
            emit removedDir(fpath, true);
        } else {
            dirfails.append(fpath);
        }
    }
    if ( !dirfails.isEmpty() ) {
        // First sort the list.
        dirfails.sort();
        // Do a reverse iteration, to get the child directories first.
        // List the directories in reverse order (starting at the leaves)
        for (auto it = dirfails.rbegin(); it != dirfails.rend(); ++it) {
            emit removedDir(*it, d0.rmdir(*it));
        }
    }

    emit finished();
}
