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
#include "appinfo.h"

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
    explicit Uninstaller(AppInfo& appinfo, QWidget *parent = nullptr);
    ~Uninstaller();

private slots:
    void page_1();
    void page_2();
    void file_deleted(QString fpath, bool ok);
    void dir_removed(QString fpath, bool ok);
    void done(bool ok);

private:
    Ui::Uninstaller *ui;

    void print_line(QString line, bool bold = false);
    void progressOne();
    void unregisterApp();
    QStringList unregister_failed;
    QDir basedir;
    bool bugflag{false};

signals:
    void deleteFiles(const QString basePath);
};

extern void fatalError(QString msg);

#endif // UNINSTALLER_H
