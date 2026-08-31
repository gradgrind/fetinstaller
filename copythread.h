#ifndef COPYTHREAD_H
#define COPYTHREAD_H

#include <QObject>
#include <QDir>

class CopyWorker : public QObject
{
    Q_OBJECT

public slots:
    //void doWork(const QDir& srcDir, const QDir& dstDir, QTextStream& log_stream);
    void copyDirectory(const QDir& srcDir, const QDir& dstDir);

signals:
    void number_of_files(int count);
    void copied(QString filepath);
    void failed_copy(QString filepath);
    void progress(int count);
    void copying_done();
};

#endif // COPYTHREAD_H
