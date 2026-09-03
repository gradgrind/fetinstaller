#ifndef DELETEWORKER_H
#define DELETEWORKER_H

#include <QObject>
#include <QStringList>
#include <QSet>
#include <QDir>
#include <QWaitCondition>
#include <QMutex>

class DeleteWorker : public QObject
{
    Q_OBJECT

private:
    void warn(QString msg);

public slots:
    void deleteFiles(QDir basedir, QStringList filesList, QSet<QString> dirsSet);

signals:
    void addOutputLine(QString line);
    void tick();
    void finished();
    void warning(QString msg);
};

extern QMutex mutex;
extern QWaitCondition waiter;


#endif // DELETEWORKER_H
