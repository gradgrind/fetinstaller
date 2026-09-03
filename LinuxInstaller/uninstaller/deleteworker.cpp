#include "deleteworker.h"

#include <QFile>
#include <QProcess>

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


    QDir home_dir{QDir::home()};
    if (basedir.path() == home_dir.absoluteFilePath(".local")) {
        emit addOutputLine("");
        emit addOutputLine(tr("Run %1 and %2").arg("update-mime-database", "update-desktop-database"));
        // Update file-type associations
        QProcess::execute("update-mime-database",
                          QStringList() << basedir.absoluteFilePath("share/mime"));
        QProcess::execute("update-desktop-database",
                          QStringList() << basedir.absoluteFilePath("share/applications"));
    }

    emit finished();
}

void DeleteWorker::warn(QString msg)
{
    // Show the message in the main thread, but wait for it to be dismissed
    mutex.lock();
    emit warning(msg);
    waiter.wait(&mutex);
    mutex.unlock();
}
