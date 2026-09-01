#ifndef DELETEWORKER_H
#define DELETEWORKER_H

#include <QObject>
#include <QStringList>
#include <QSet>
#include <QDir>

class DeleteWorker : public QObject
{
    Q_OBJECT

public slots:
    void deleteFiles(QDir basedir, QStringList filesList, QSet<QString> dirsSet);

signals:
    void addOutputLine(QString line);
    void tick();
    void finished();
};

#endif // DELETEWORKER_H
