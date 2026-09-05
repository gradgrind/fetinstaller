#include "copythread.h"
#include <QDir>
#include <QMessageBox>
#include <QApplication>
#include <QDirIterator>
#include <QProcess>

//TODO: If an error occurs, it would be good to remove all installed files.

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

        //qDebug() << "LINK" << sinfo.readSymLink() << "&" << sinfo.symLinkTarget(); // raw & absolute path

        return QFile::link(sinfo.readSymLink(), dFile);
    } else {
        return QFile::copy(sFile, dFile);
    }
}

void CopyWorker::copyDirectory(const QDir& srcDir, const QDir& dstDir, const InstallFiles& iFiles) {
    /* ... here is the long-running operation ... */

    //TODO: Use iFiles ...

    // Collect the files here for copying later.
    // All files except from root directories starting with "_" (currently just "_bin") are copied.
    QStringList files; // relative paths
    QDirIterator it(srcDir.absolutePath(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString apath = it.next();
        QString rpath = srcDir.relativeFilePath(apath);
        if (rpath.startsWith("_")) {
            continue;
        }
        if (it.fileInfo().isFile()) {
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
