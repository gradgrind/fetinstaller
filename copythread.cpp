#include "copythread.h"
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QDirIterator>

// Copy source file to destination path, creating path directories if necessary.
// Return true if successful.
bool copyFile(QString sFile, QString dFile)
{
    QFileInfo df{dFile};
    if (df.exists()) {
        if (!QFile::remove(dFile)) return false;
    } else {
        QDir dd{df.dir()};
        if (!dd.exists()) {
            dd.mkpath(dd.absolutePath());
        }
    }
    QFileInfo sinfo{sFile};
    if (sinfo.isSymLink()) {
        //TODO: This will not handle absolute links correctly!
        return QFile::link(sinfo.readSymLink(), dFile);
    } else {
        return QFile::copy(sFile, dFile);
    }
}

void CopyWorker::copyDirectory(const QDir& srcDir, const QDir& dstDir) {
    /* ... here is the long-running operation ... */

    // Collect the files here for copying later.
    QStringList files; // relative paths
    QDirIterator it(srcDir.absolutePath(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString apath = it.next();
        if (it.fileInfo().isFile()) {
            auto rpath = srcDir.relativeFilePath(apath);
            files.append(rpath);
        }
    }
    emit number_of_files(files.length());

    // Now copy the files.
    for (const auto& rpath : files) {
        QString dpath{dstDir.absoluteFilePath(rpath)};
        if (copyFile(srcDir.absoluteFilePath(rpath), dpath)) {
            emit copied(dpath);
        } else {
            emit failed_copy(dpath);
            return;
        }
    }

    emit copying_done();
}

    /* OLD ...

    if (!dstDir.exists()) {
        dstDir.mkpath(dst); // Create destination directory if it doesn't exist
    }

    int count{0};
    const QFileInfoList entries = srcDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo& entry : entries) {
        QString newDestPath = dstDir.filePath(entry.fileName());
        if (entry.isDir()) {
            if (!copyDirectory(entry.absoluteFilePath(), newDestPath, log_stream)) {
                return false;
            }
        } else {
            if (QFile::exists(newDestPath)) {
                QFile::remove(newDestPath);
            }
            if (QFile::copy(entry.absoluteFilePath(), newDestPath)) {
                // save to list file
                log_stream << newDestPath << "\n";
                emit progress(count);
                //ui->installProgress->setValue(ui->installProgress->value() + 1);
            } else {
                //TODO
                QMessageBox::critical(nullptr, tr("FATAL_ERROR"), tr("Failed to copy file to: ") + newDestPath);
                qApp->exit(1);
                return false;
            }
        }
    }
    return true;
    */
//}
