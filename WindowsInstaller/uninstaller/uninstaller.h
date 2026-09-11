#ifndef UNINSTALLER_H
#define UNINSTALLER_H

#include <QApplication>
#include <QTranslator>
#include <QWidget>
#include <QDir>
#include <QStringList>
#include <QSet>
#include <QThread>
#include "deleteworker.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Uninstaller;
}
QT_END_NAMESPACE

class Uninstaller : public QWidget
{
    Q_OBJECT
    QThread workerThread;
    DeleteWorker* worker;

public:
    explicit Uninstaller(QWidget *parent = nullptr);
    ~Uninstaller();

private slots:
    void page_1();
    void page_2();
    void file_deleted(QString fpath, bool ok);
    void dir_removed(QString fpath, bool ok);
    void done(bool ok);

private:
    Ui::Uninstaller *ui;
    QDir basedir;

    QStringList filesList;
    QStringList dirsList;
    QStringList linksList;

    QStringList failed_files;
    QStringList failed_dirs;

    void print_1(QString line);
    void print_2(QString line);
    void progressOne();
    void unregisterApp();
    bool bugflag{false};
    void warning(QString msg);
    void fatalError(QString msg);

signals:
    void deleteFiles(const QStringList links, const QStringList files, const QStringList dirs);
};

extern void fatalError(QString msg);

#endif // UNINSTALLER_H
