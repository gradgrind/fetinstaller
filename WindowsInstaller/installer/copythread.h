#ifndef COPYTHREAD_H
#define COPYTHREAD_H

#include <QObject>
#include <QDir>
#include <QStringList>

struct InstallFiles // relative paths to the files and directories to be copied
{
    QStringList installationFiles;
    QStringList installationDirs;
    // Symlinks (not Windows): relative links (within the installation) and
    //                         absolute links (outside the installation)
    // Shortcuts (Windows):    absolute links (outside the installation)
    QList<QPair<QString, QString>> installationLinks; // absolute symlinks (outside the installation)
};

class CopyWorker : public QObject
{
    Q_OBJECT

public slots:
    void copyDirectory(const QDir& srcDir, const QDir& dstDir, const InstallFiles& iFiles);
    void removePartial(const QDir& dstDir, const QStringList& dirs, const QStringList &files);

signals:
    void number_of_files(int count);
    void dir_nocopy(QString filepath);
    void dir_written(QString filepath);
    void dir_failed_write(QString filepath);
    void dir_failed_overwrite(QString filepath);
    void file_copied(QString filepath);
    void failed_copy(QString filepath);
    void link_copied(QPair<QString, QString> filepaths);
    void failed_link(QPair<QString, QString> filepaths);
    void copying_done();

    void remove_file(QString f, bool ok);
    void remove_dir(QString f, bool ok);
    void removing_done();
};

#endif // COPYTHREAD_H
