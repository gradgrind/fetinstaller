#include "deleteworker.h"

#include <QFile>

QMutex mutex;
QWaitCondition waiter;

void DeleteWorker::deleteFiles(QDir basedir, QStringList filesList, QSet<QString> dirsSet)
{
    // Delete the files in filesList
    int fileCount{0};
    QStringList errorFiles;

    for (const auto& fpath : filesList) {
        if (QFile::remove(fpath)) {
            fileCount++;
            emit addOutputLine(" - " + fpath);
            //ui->output->appendPlainText(" - " + fpath);
        } else {
            errorFiles.append(fpath);
        }
        emit tick();
    }

    // Report files not removed
    for (const auto& fpath : errorFiles) {
        emit addOutputLine("??? " + fpath);
    }
    if (!errorFiles.isEmpty()) {
        warn(tr("%1 files could not be deleted (lines starting with \"???\")").arg(errorFiles.length()));
    }

    // Remove empty directories
    QStringList dirsList{dirsSet.values()};
    dirsList.sort();
    int dirCount{0};
    for (auto it = dirsList.rbegin(); it != dirsList.rend(); ++it) {
        // Use reverse iteration to get the deepest directories first
        if (basedir.rmdir(*it)) {
            dirCount++;
            emit addOutputLine(" --- " + *it + '/');
        }
        emit tick();
    }
    if (basedir.rmdir(basedir.path())) {
        dirCount++;
        emit addOutputLine(" --- " + basedir.path());
    }

    emit addOutputLine("");
    emit addOutputLine(tr("%1 files deleted").arg(fileCount));
    emit addOutputLine(tr("%1 directories removed").arg(dirCount));

    emit finished();
}

void DeleteWorker::warn(QString msg)
{
    // Show the message in the main thread, but wait for it to be dismissed
    mutex.lock();
    emit warning("Just testing!");
    waiter.wait(&mutex);
    mutex.unlock();
}
