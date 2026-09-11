#ifndef DELETEWORKER_H
#define DELETEWORKER_H

#include <QObject>
#include <QStringList>

class DeleteWorker : public QObject
{
    Q_OBJECT

public slots:
    void deleteFiles(const QStringList links, const QStringList files, const QStringList dirs);

signals:
    void deletedFile(QString fpath, bool ok);
    void removedDir(QString fpath, bool ok);
    void finished(bool ok);
};

#endif // DELETEWORKER_H
