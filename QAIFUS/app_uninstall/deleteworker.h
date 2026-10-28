#ifndef DELETEWORKER_H
#define DELETEWORKER_H

#include <QObject>
#include <QStringList>

class DeleteWorker : public QObject
{
    Q_OBJECT

    bool removeDirectory(const QString &dirPath);

public slots:
    void deleteFiles(const QString basePath);

signals:
    void deletedFile(QString fpath, bool ok);
    void removedDir(QString fpath, bool ok);
    void done(bool ok);
};

#endif // DELETEWORKER_H
